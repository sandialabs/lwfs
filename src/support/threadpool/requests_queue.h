#ifndef REQUESTS_QUEUE_H
# define REQUESTS_QUEUE_H

#include <stdio.h>       /* standard I/O routines                     */
#include <pthread.h>     /* pthread functions and data structures     */

#include "thread_pool.h"

/* structure for a requests queue */
struct requests_queue {
    struct lwfs_thread_pool_request* requests;       /* head of linked list of requests. */
    struct lwfs_thread_pool_request* last_request;   /* pointer to last request.         */
    int num_requests;		    /* number of requests in queue.     */
    int id;                         /* identifier for this queue        */
    pthread_mutex_t* p_mutex;	    /* queue's mutex.                   */
    pthread_cond_t*  p_cond_var;    /* queue's condition variable.      */
};

/*
 * create a requests queue. associate it with the given mutex
 * and condition variables.
 */
extern struct requests_queue*
init_requests_queue(pthread_mutex_t* p_mutex, pthread_cond_t*  p_cond_var);

/* add a request to the requests list */
extern void
add_request(struct requests_queue* queue, struct lwfs_thread_pool_request *a_request);

/* get the first pending request from the requests list */
extern struct lwfs_thread_pool_request*
get_request(struct requests_queue* queue);

/* get the number of requests in the list */
extern int
get_requests_number(struct requests_queue* queue);

/* free the resources taken by the given requests queue */
extern void
delete_requests_queue(struct requests_queue* queue);

#endif /* REQUESTS_QUEUE_H */
