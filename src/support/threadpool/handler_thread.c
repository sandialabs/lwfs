#include <stdio.h>       /* standard I/O routines                     */
#include <pthread.h>     /* pthread functions and data structures     */
#include <stdlib.h>      /* malloc() and free()                       */
#include <assert.h>      /* assert()                                  */

#include "handler_thread.h"   /* handler thread functions/structs     */
#include "thread_pool_debug.h"
#include "support/logger/logger.h"
#include "support/signal/lwfs_signal.h"
#include "support/trace/trace.h"

/*
 * function cleanup_free_mutex(): free the mutex, if it's locked.
 * input:     pointer to a mutex structure.
 * output:    none.
 */
static void
cleanup_free_mutex(void* a_mutex)
{
    pthread_mutex_t* p_mutex = (pthread_mutex_t*)a_mutex;

    if (p_mutex)
        pthread_mutex_unlock(p_mutex);
}

/*
 * function handle_requests_loop(): infinite loop of requests handling
 * algorithm: forever, if there are requests to handle, take the first
 *            and handle it. Then wait on the given condition variable,
 *            and when it is signaled, re-do the loop.
 *            increases number of pending requests by one.
 * input:     id of thread, for printing purposes.
 * output:    none.
 */
void*
handle_requests_loop(void* thread_params)
{
	int rc;	                    /* return code of pthreads functions.  */
	struct lwfs_thread_pool_request* a_request;   /* pointer to a request. */
	struct handler_thread_params *data;
	/* hadler thread's parameters */

	/* sanity check -make sure data isn't NULL */
	data = (struct handler_thread_params*)thread_params;
	assert(data);

	lwfs_thread_pool_setrank(data->thread_id+1);
	data->requests->id = data->thread_id+1; 

	log_debug(thread_debug_level, "Starting thread. thread_pool id is '%d'.  Assigned rank is '%d'.\n", 
		data->thread_id, lwfs_thread_pool_getrank());

	/* set my cancel state to 'enabled', and cancel type to 'defered'. */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	/* set thread cleanup handler */
	pthread_cleanup_push(cleanup_free_mutex, (void*)data->request_mutex);

	/* lock the mutex, to access the requests list exclusively. */
	rc = pthread_mutex_lock(data->request_mutex);

	/* do forever.... */
	while (!lwfs_exit_now()) {

		int num_requests = get_requests_number(data->requests);

		if (num_requests > 0) { /* a request is pending */
			a_request = get_request(data->requests);
			if (a_request) { 

				/* got a request - handle it and free it */
				/* unlock mutex - so other threads would be able to handle */
				/* other reqeusts waiting in the queue paralelly.          */
				rc = pthread_mutex_unlock(data->request_mutex);

				//log_debug(thread_debug_level, "thread '%d' processing request", data->thread_id);
				a_request->handler(a_request, lwfs_thread_pool_getrank());
				free(a_request);

				/* and lock the mutex again. */
				rc = pthread_mutex_lock(data->request_mutex);

				/* ADDED BY RON --- wake threads, just in case we missed a signal */
				//pthread_cond_broadcast(data->got_request); 
				/* END ADDED BY RON */
			}
		}
		else {
			/* the thread checks the flag before waiting            */
			/* on the condition variable.                           */
			/* if no new requests are going to be generated, exit.  */
			if (*data->shutdown_now) {
				goto cleanup;
			}
			/* wait for a request to arrive. note the mutex will be */
			/* unlocked here, thus allowing other threads access to */
			/* requests list.                                       */
			rc = pthread_cond_wait(data->got_request, data->request_mutex);
			/* and after we return from pthread_cond_wait, the mutex  */
			/* is locked again, so we don't need to lock it ourselves */
		}
	}

cleanup:
	pthread_mutex_unlock(data->request_mutex);

	/* remove thread cleanup handler. never reached, but we must use */
	/* it here, according to pthread_cleanup_push's manual page.     */
	pthread_cleanup_pop(0);

	log_debug(thread_debug_level, "thread '%d' exiting\n", data->thread_id);

	return NULL;
}
