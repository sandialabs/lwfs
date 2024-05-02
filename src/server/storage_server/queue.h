/**  
 *   @file queue.h
 * 
 *   @brief A simple thread-safe queue implementation. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#include <pthread.h>
#include <semaphore.h>
#include "storage_server.h"
#include "io_buffer.h"
#include "io_threads.h"


#ifndef _QUEUE_H_
#define _QUEUE_H_


#ifdef __cplusplus
extern "C" {
#endif

	struct queue_node {
		void *data;
		struct queue_node *next; 
		struct queue_node *prev; 
	};

	struct queue {
		struct queue_node *head; 
		struct queue_node *tail; 
		pthread_mutex_t mutex; 
		pthread_cond_t cond; 
	};

#if defined(__STDC__) || defined(__cplusplus)

	extern int queue_init(struct queue *q);

	extern int queue_destroy(struct queue *q, void (*free_op)(void *));

	extern lwfs_bool queue_empty(struct queue *q); 

	extern void *queue_pop(struct queue *q);

	extern int queue_append(struct queue *q, void *node);

#else 

#endif

#ifdef __cplusplus
}
#endif

#endif
