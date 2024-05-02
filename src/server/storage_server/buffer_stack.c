/**  
 *   @file buffer_stack.c
 * 
 *   @brief Implementation of an interface to manage a stack of I/O buffers.
 *
 *   The storage servers use the I/O buffers when reading and writing data
 *   from the client.  For example, when a client initiates a write request, 
 *   a storage server thread "pops" a new buffer off the stack, fetches the 
 *   data from the client (copying into the buffer), then sends the buffer 
 *   to the writer thread.  The server thread continues this loop until all
 *   of the client code is transferred to storage.  When the writer thread
 *   completes the I/O operation, it "pushes" the buffer back onto the 
 *   buffer stack and makes it available. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1189 $
 *   $Date: 2007-02-07 14:30:35 -0700 (Wed, 07 Feb 2007) $
 */
#include "config.h" 

#include <pthread.h>
#include <errno.h>

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif
#include "storage_server.h"
#include "buffer_stack.h"
#include "io_buffer.h"

#define TIMEOUT 5

/**
 * @brief Create a stack of buffers.
 *
 * This function allocates a set of buffers and fills 
 * and pushes those buffers on a \ref struct buffer stack.
 *
 * @param  count  @input The number of buffers to allocate.
 * @param  stack  @output The resulting buffer_stack.
 */
int buffer_stack_init(struct buffer_stack *stack) {

	/* initialize the stack data structure */
	memset(stack, 0, sizeof(struct buffer_stack));
	stack->node = NULL;
	pthread_mutex_init(&stack->mutex, NULL); 
	pthread_cond_init(&stack->cond, NULL); 
	
	return LWFS_OK;
}


/**
 * @brief Destroy a stack of buffers.
 *
 * This function allocates a set of buffers and fills 
 * and pushes those buffers on a \ref struct buffer stack.
 *
 * @param  stack  @input The buffer_stack.
 */
int buffer_stack_destroy(struct buffer_stack *stack)
{
	int rc = LWFS_OK;
	struct io_buffer *iobuf; 
	int buf_count=0;
	int buf_rec_count=0;

	log_debug(ss_debug_level, "destroying the buffer stack");
	while (!buffer_stack_empty(stack)) {
		iobuf = buffer_stack_pop(stack);
		if (iobuf != NULL) {
			if (iobuf->buf != NULL) {
				log_debug(ss_debug_level, "freeing a buffer");
				buf_count++;
				free(iobuf->buf);
			}
			log_debug(ss_debug_level, "freeing a buffer record");
			buf_rec_count++;
			free(iobuf);
		}
	}
	log_debug(ss_debug_level, "freed %d buffer records", buf_rec_count);
	log_debug(ss_debug_level, "freed %d buffers", buf_count);

	if (pthread_mutex_destroy(&stack->mutex) != 0) {
		log_error(ss_debug_level, "unable to destroy stack mutex");
		return LWFS_ERR; 
	}

	if (pthread_cond_destroy(&stack->cond) != 0) {
		log_error(ss_debug_level, "unable to destroy stack mutex");
		return LWFS_ERR; 
	}


	return rc; 
}

/**
 * @brief Returns true if the stack is empty.
 *
 * @param stack  @input  The stack.
 *
 * @returns True if the stack is empty, false otherwise. 
 */
lwfs_bool buffer_stack_empty(struct buffer_stack *stack) {
	return (stack->node == NULL);
}


/**
 * @brief Pop a buffer off of the stack.
 *
 * @param buffer_stack @input The stack. 
 */
struct io_buffer *buffer_stack_pop(struct buffer_stack *stack) 
{
	int rc = LWFS_OK;
	struct stack_node *node = NULL; 
	struct io_buffer *result = NULL; 
	struct timespec ts; 
	struct timeval tp;

	if (pthread_mutex_lock(&stack->mutex) != 0) {
		log_error(ss_debug_level, "unable to lock stack mutex");
		return NULL; 
	}

#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_REALTIME, &ts);
#else
	gettimeofday( &tp, NULL );
	ts.tv_sec = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;
#endif
	ts.tv_sec += TIMEOUT; 

	/* if the stack is empty, wait for someone to push an item */
	while (buffer_stack_empty(stack)) {
		rc = pthread_cond_timedwait(&stack->cond, &stack->mutex, &ts);
		if (rc == ETIMEDOUT) {
			log_warn(ss_debug_level, "timed out on \"pop\"");
			result = NULL;
			goto unlock;
		}
	}

	/* remove the top item from the list */
	node = stack->node; 
	stack->node = node->next; 
	result = node->buf; 

	/* free the node */
	free(node); 

unlock:
	if (pthread_mutex_unlock(&stack->mutex) != 0) {
		log_error(ss_debug_level, "unable to unlock stack mutex");
		return NULL; 
	}

	return result; 
}

int buffer_stack_push(struct buffer_stack *stack, struct io_buffer *buf) 
{
	int rc = LWFS_OK;
	struct stack_node *node = NULL; 

	if (pthread_mutex_lock(&stack->mutex) != 0) {
		log_error(ss_debug_level, "unable to lock stack mutex");
		return LWFS_ERR; 
	}

	/* allocate a new node */
	node = (struct stack_node *)malloc(1*sizeof(struct stack_node));
	if (!node) {
		log_error(ss_debug_level, "unable to allocate stack node");
		goto unlock;
	}

	node->buf = buf; 

	/* put the entry on the top of the list */
	node->next = stack->node; 
	stack->node = node; 

	/* signal any waiting threads */
	pthread_cond_signal(&stack->cond);

unlock:
	if (pthread_mutex_unlock(&stack->mutex) != 0) {
		log_error(ss_debug_level, "unable to unlock stack mutex");
		return LWFS_ERR; 
	}

	return rc; 
}
