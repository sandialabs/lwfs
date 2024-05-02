#include <stdio.h>             /* standard I/O routines                      */
#include <pthread.h>           /* pthread functions and data structures      */
#include <stdlib.h>            /* rand() and srand() functions               */
#include <unistd.h>            /* sleep()                                    */
#include <assert.h>            /* assert()                                   */
#include <string.h>

#include "requests_queue.h"         /* requests queue routines/structs       */
#include "handler_thread.h"         /* handler thread functions/structs      */
#include "handler_threads_pool.h"   /* handler thread list functions/structs */
#include "thread_pool.h"
#include "thread_pool_debug.h"


/* number of initial threads used to service requests, and max number */
/* of handler threads to create during "high pressure" times.         */
#define MIN_HANDLER_THREADS 3
#define MAX_HANDLER_THREADS 14

/* number of requests on the queue warranting creation of new threads */
#define LOW_WATERMARK 3
#define HIGH_WATERMARK 15

/*
 *  On BSD (MacOS), pthread_mutex_t is not recursive by default, and so the static
 *  initializer here doesn't work.  Instead, we have to play a little compiler trick, 
 *  getting gcc to generate a file-scope constructor for it.  I'm going to
 *  leave the bail-out #error if we're not being compiled with GCC, since porting
 *  to another compiler should make for a lot of work in other places, not just here.
 */
#if (defined(__APPLE__) && defined(__GNUC__))

static pthread_mutex_t proto_mutex;

static void __attribute__ ((constructor)) thunk(void)
{
#if 0
  pthread_mutexattr_t attr;
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
  pthread_mutex_init( &proto_mutex, &attr );
#endif
}

#else
static pthread_mutex_t proto_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

static pthread_cond_t  proto_cond  = PTHREAD_COND_INITIALIZER;

static pthread_key_t rank_key; 

static void init_rank_key() {
    static int initialized = 0;

    if (!initialized) {
        pthread_key_create(&rank_key, free);
        initialized = 1;
    }
}

static void delete_rank_key() {
    static int initialized = 0;

    if (!initialized) {
        pthread_key_delete(rank_key);
        initialized = 1;
    }
}

void lwfs_thread_pool_setrank(const int rank) 
{
    init_rank_key(); 

    if (pthread_getspecific(rank_key) == NULL) {
        int *data = (int *)malloc(sizeof(int));
        *data = rank; 
        pthread_setspecific(rank_key, data); 
    }
}

int lwfs_thread_pool_getrank() 
{
    init_rank_key(); 

    int *rank = pthread_getspecific(rank_key); 

    if (rank == NULL) {
        return -1; 
    }
    else 
        return *rank; 
}

/**
 * @brief  initialize the pool and request queue
 *
 * If the args were small enough to fit into the short
 * request buffer, we extract the args directly from
 * the buffer.  Otherwise, we GET the arguments from 
 * a remote memory descriptor on the client. 
 *
 * @param initial_thread_count  
 * @param 
 * @param 
 * @param 
 */
int lwfs_thread_pool_init(lwfs_thread_pool *pool,
                          const lwfs_thread_pool_args *pool_args)
{
    int i;  /* loop counter */

    /* note that we use a RECURSIVE mutex, since a handler      */
    /* thread might try to lock it twice consecutively.         */
    memcpy(&pool->request_mutex, &proto_mutex, sizeof(pthread_mutex_t));
    memcpy(&pool->got_request, &proto_cond, sizeof(pthread_cond_t));
    pool->args.min_thread_count = pool_args->min_thread_count;
    pool->args.max_thread_count = pool_args->max_thread_count;

    if (pool->args.min_thread_count < 1) {
    	pool->args.min_thread_count = MIN_HANDLER_THREADS;
    }

    if (pool->args.max_thread_count < pool->args.min_thread_count) {
    	pool->args.max_thread_count = pool->args.min_thread_count + (MAX_HANDLER_THREADS-MIN_HANDLER_THREADS);
    }

    pool->args.initial_thread_count = pool_args->initial_thread_count;
    if (pool->args.initial_thread_count < pool->args.min_thread_count) {
    	pool->args.initial_thread_count = pool->args.min_thread_count;
    } else if (pool->args.initial_thread_count > pool->args.max_thread_count) {
    	pool->args.initial_thread_count = pool->args.max_thread_count;
    }


    pool->args.low_watermark    = pool_args->low_watermark;
    pool->args.high_watermark   = pool_args->high_watermark;
    if (pool->args.low_watermark < 1) {
    	pool->args.low_watermark = LOW_WATERMARK;
    }
    if (pool->args.high_watermark < pool->args.low_watermark) {
    	pool->args.high_watermark = pool->args.low_watermark + (HIGH_WATERMARK-LOW_WATERMARK);
    }
    
    pool->shutdown_now = 0;

    /* initialize the rank key */
    init_rank_key(); 
    
    /* create the requests queue */
    pool->requests = init_requests_queue(&pool->request_mutex, &pool->got_request);

    /*trace_set_count(TRACE_PENDING_REQS, 0 ,"pending++");*/

    assert(pool->requests);

    /* create the handler threads list */
	pool->handler_threads =
		init_handler_threads_pool(&pool->request_mutex, &pool->got_request, pool->requests);
	assert(pool->handler_threads);

    /* create the request-handling threads */
	for (i=0; i<pool->args.initial_thread_count; i++) {
		add_handler_thread(pool);
	}

	//fprintf(stderr, "main: high-watermark: requested=%d, actual=%d\n", 
	//		pool_args->high_watermark, pool->args.high_watermark);
    
    return(0);
}

int lwfs_thread_pool_add_request(lwfs_thread_pool *pool,
                                 void *client_data, 
                                 request_handler_proc handler)
{
    int num_requests; // number of requests waiting to be handled.
    int num_threads;  // number of active handler threads.
    
    struct lwfs_thread_pool_request *a_request;
    
    a_request=calloc(1,sizeof(struct lwfs_thread_pool_request));
    if (a_request == NULL)
    {
    	log_error(thread_debug_level, "failed to create request");
    	return(1);
    }
    a_request->client_data = client_data;
    a_request->handler = handler;

    add_request(pool->requests, a_request);

    num_requests = get_requests_number(pool->requests);
    num_threads = get_handler_threads_number(pool);

    /* if there are too many requests on the queue, spawn new threads */
    /* if there are few requests and too many handler threads, cancel */
    /* a handler thread.          					  */
	//log_debug(thread_debug_level, "main: num-reqs=%d, high-watermark=%d\n", 
	//		num_requests, pool->args.high_watermark);

    if (num_requests > pool->args.high_watermark &&
        num_threads < pool->args.max_thread_count) {
        log_debug(thread_debug_level, "Adding thread: '%d' requests, '%d' threads\n",
               num_requests, num_threads);
        add_handler_thread(pool);
    }
    if (num_requests < pool->args.low_watermark &&
        num_threads > pool->args.min_thread_count) {
        log_debug(thread_debug_level, "Deleting thread: '%d' requests, '%d' threads\n",
               num_requests, num_threads);
        //delete_handler_thread(pool);
    }
    
    return(0);
}

int lwfs_thread_pool_fini(lwfs_thread_pool *pool)
{
    /* cancel threads and wait for completion */
	cancel_handler_threads(pool);

    delete_requests_queue(pool->requests);

    delete_handler_threads_pool(pool);
    
    /* delete the rank key */
    delete_rank_key(); 
    
    return(0);
}
