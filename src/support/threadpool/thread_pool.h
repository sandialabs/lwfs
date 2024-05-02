#ifndef THREAD_POOL_H
# define THREAD_POOL_H

#include <pthread.h>           /* pthread functions and data structures      */
#include "thread_pool_options.h"

enum tp_trace_ids {
    TRACE_PENDING_REQS = 10
};


typedef struct {
    /* global mutex for our program. */
    pthread_mutex_t request_mutex;

    /* global condition variable for our program. */
    pthread_cond_t  got_request;

    lwfs_thread_pool_args args;
    
    int shutdown_now;

    struct requests_queue*       requests;         /* pointer to requests queue */
    struct handler_threads_pool* handler_threads;  /* list of handler threads */
} lwfs_thread_pool;

struct lwfs_thread_pool_request;
typedef int (*request_handler_proc)(struct lwfs_thread_pool_request *a_request, const int thread_id);
struct lwfs_thread_pool_request {
    void *client_data;            /* data to process                              */
    request_handler_proc handler; /* a re-entrant function to process the request */
    int number;                   /* number of the request                        */
    struct lwfs_thread_pool_request* next;         /* pointer to next request, NULL if none.       */
} ;

/**
 * @brief  initialize the pool
 *
 * @param pool  a pointer to the new pool
 * @param initial_thread_count  number of the threads to put in the pool to start with
 * @param min_thread_count  min number of threads in the pool
 * @param max_thread_count  max number of threads in the pool
 * @param low_watermark  request queue size at which we start killing threads
 * @param high_watermark  request queue size at which we start creating threads
 */
int lwfs_thread_pool_init(lwfs_thread_pool *pool,
                          const lwfs_thread_pool_args *pool_args);

/**
 * @brief  add a request to the pool
 *
 * @param pool  a pointer to the pool
 * @param client_data  any data required by the thread to process this request
 * @param handler  the function the thread will call to process this request
 */
int lwfs_thread_pool_add_request(lwfs_thread_pool *pool,
                                 void *client_data, 
                                 request_handler_proc handler);

/**
 * @brief  cleanup the pool
 *
 * @param pool  a pointer to the new pool
 */
int lwfs_thread_pool_fini(lwfs_thread_pool *pool);

/**
 * @brief Set the rank of a thread (only happens once).
 */
extern void lwfs_thread_pool_setrank(const int rank); 

/**
 * @brief get the rank of the current thread. 
 */
extern int lwfs_thread_pool_getrank(); 

#endif /* THREAD_POOL_H */
