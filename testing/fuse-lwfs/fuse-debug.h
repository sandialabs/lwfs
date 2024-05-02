/**  
 *   @file fuse-debug.h
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1073 $
 *   $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
 *
 */

#ifndef _FUSE_DEBUG_H_
#define _FUSE_DEBUG_H_

#include "logger/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern log_level fuse_debug_level;
	extern log_level fuse_call_debug_level;

#ifdef __cplusplus
}
#endif

#endif 
