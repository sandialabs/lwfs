/**
 * @file io_threads.c
 *
 * The code in this file implements the reader and writer threads
 * used by the LWFS storage server. 
 *
 * @author Ron Oldfield (raoldfi\@sandia.gov)
 */

#include <pthread.h>
#include <sched.h>

#include "storage_server.h"
#include "sysio_obj.h"
#include "io_threads.h"
#include "buffer_stack.h"
#include "queue.h"

static char root_dir[256]; 

struct buffer_stack *buffer_stack=NULL;

static pthread_t       reader_thread; 
static struct queue    reader_queue; 

static pthread_t       writer_thread; 
static struct queue    writer_queue; 

/* Ctrl-C sets this value to 1 */
extern volatile int exit_now; 

static lwfs_bool use_simdisk = FALSE; 


/**
 *  This code processes requests from the writer queue.  The request
 *  structure includes a buffer that has already been filled by data
 *  from a remote client. 
 */ 
void *run_writer()
{
    log_debug(ss_debug_level, "starting IO Writer");

    while (!exit_now) {

	/* get next ioreq (if empty, this waits for a push) */
	log_debug(ss_debug_level, "waiting for next write request; exit_now==%d", exit_now);
	struct io_req *ioreq = (struct io_req *)queue_pop(&writer_queue); 

	if (!exit_now && ioreq) {
	    int rc = LWFS_OK; 

	    log_debug(ss_debug_level, "processing write request");

	    if (!use_simdisk) {
		if (!my_objs_write_obj(root_dir, ioreq->obj, ioreq->offset, 
			    ioreq->iobuf->buf, ioreq->len)) {
		    log_error(ss_debug_level, "unable to write buffer");
		    rc = LWFS_ERR_STORAGE;
		}
	    }

	    /* add the buffer back to the buffer_stack */
	    buffer_stack_push(buffer_stack, ioreq->iobuf);
	    if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to push buffer onto stack");
	    }

	    /* free the ioreq */
	    free(ioreq); 

	    /* decrement the number of pending reqs */
	    my_objs_decrement_pending(ioreq->obj);

	}
    }

    log_debug(ss_debug_level, "stopping IO Writer");
    return NULL;
}

/**
 *  This code processes requests from the reader queue.  The request
 *  structure includes a buffer that has already been filled by data
 *  from a remote client. 
 */ 
void *run_reader()
{
    log_debug(ss_debug_level, "starting IO Reader");

    while (!exit_now) {

	/* get next ioreq (if empty, this waits for a push) */
	//struct io_req *ioreq = (struct io_req *)queue_pop(&reader_queue); 

	if (!exit_now) {
	    struct io_req *ioreq; 

	    /* process the request */
	    if (ioreq != NULL) {
		/* read */
	    }
	}
    }

    log_debug(ss_debug_level, "stopping IO Reader");
    return NULL;
}



int start_io_threads(
		const char *root, 
		struct buffer_stack *bufs, 
		const lwfs_bool simdisk)
{
    int rc = LWFS_OK;
    strncpy(root_dir, root, sizeof(root_dir));

    /* should we simulate I/O? */
    use_simdisk = simdisk; 

    buffer_stack = bufs; 

    queue_init(&writer_queue);
    queue_init(&reader_queue); 

    /* start the writer */
    pthread_create(&writer_thread, NULL, run_writer, NULL);

    /* start the reader */
    pthread_create(&reader_thread, NULL, run_reader, NULL);

    return rc; 
}


int stop_io_threads()
{
    int rc = LWFS_OK;

    log_debug(ss_debug_level, "stopping IO threads");

    exit_now = TRUE; 

    /* wake the threads */
    //	pthread_cond_broadcast(&reader_queue.cond); 
    //	pthread_cond_broadcast(&writer_queue.cond);

    pthread_join(writer_thread, NULL); 
    pthread_join(reader_thread, NULL);

    log_debug(ss_debug_level, "stopped IO threads");

    return rc; 
}


int writer_add_req(struct io_req *ioreq)
{
    int rc = LWFS_OK;

    /* increment the pending count */
    my_objs_increment_pending(ioreq->obj);

    /* add the request to the writer queue. */
    queue_append(&writer_queue, ioreq); 

    log_debug(ss_debug_level, "submitted write request");

    return rc; 
}


int reader_add_req(struct io_req *ioreq)
{
    int rc = LWFS_OK;

    /* increment the pending count */
    my_objs_increment_pending(ioreq->obj);

    /* add the request to the writer queue */
    queue_append(&reader_queue, ioreq); 

    return rc; 
}

