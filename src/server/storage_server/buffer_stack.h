/**  
 *   @file buffer_stack.h
 * 
 *   @brief Prototypes and structure definitions for the stack data structure. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#ifndef _B_STK_H_
#define _B_STK_H_

#include <pthread.h>
//#include <semaphore.h>
#include "io_buffer.h"
#include "storage_server.h"

	struct stack_node {
		struct io_buffer *buf;
		struct stack_node *next; 
	};

	struct buffer_stack {
		struct stack_node *node; 
		pthread_mutex_t mutex; 
		pthread_cond_t cond; 
	};

    typedef struct buffer_stack buffer_stack_t; 


#if defined(__STDC__) || defined(__cplusplus)

	extern int buffer_stack_init(struct buffer_stack *stack);

	extern int buffer_stack_destroy(struct buffer_stack *stack);

	extern lwfs_bool buffer_stack_empty(struct buffer_stack *stack); 

	extern struct io_buffer *buffer_stack_pop(struct buffer_stack *stack);

	extern int buffer_stack_push(struct buffer_stack *stack, struct io_buffer *buf);

#endif

#endif
