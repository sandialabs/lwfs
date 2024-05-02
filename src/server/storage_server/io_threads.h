/**  
 *   @file io_threads.h
 * 
 *   @brief Prototypes and structure definitions for io_threads. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */


#ifndef _IO_THREADS_H_
#define _IO_THREADS_H_

#include <pthread.h>
#include "storage_server.h"
#include "buffer_stack.h"
#include "io_buffer.h"


#ifdef __cplusplus
extern "C" {
#endif

	enum io_req_state {
		IO_REQ_PENDING,
		IO_REQ_PROCESSING, 
		IO_REQ_ERROR, 
		IO_REQ_COMPLETE
	};

	struct io_req {
		pthread_cond_t cond; 
		pthread_mutex_t mutex; 
		enum io_req_state state; 
		lwfs_rma src_addr; 
		struct io_buffer *iobuf;
		lwfs_obj *obj;
		lwfs_size offset;
		lwfs_size len;
	};

#if defined(__STDC__) || defined(__cplusplus)

	extern int start_io_threads(
			const char *root, 
			buffer_stack_t *bufs,
			const lwfs_bool simdisk);

	extern int stop_io_threads();

	extern int writer_add_req(struct io_req *ioreq);

	extern int reader_add_req(struct io_req *ioreq);

#else 

#endif

#ifdef __cplusplus
}
#endif

#endif
