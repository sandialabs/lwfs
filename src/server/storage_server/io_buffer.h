/**  
 *   @file io_buffer.h
 * 
 *   @brief Prototypes and structure definitions for the stack data structure. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#ifndef _IO_BUFFER_H_
#define _IO_BUFFER_H_

#include "buffer_stack.h"


#ifdef __cplusplus
extern "C" {
#endif

	struct io_buffer {
		void *buf;
		ssize_t len; 
	};

#if defined(__STDC__) || defined(__cplusplus)

#else 

#endif

#ifdef __cplusplus
}
#endif

#endif
