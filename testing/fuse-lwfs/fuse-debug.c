/**  @file fuse-debug.c
 *   
 *   @brief Set the global debug level for fuse-lwfs.
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1073 $.
 *   $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $.
 *
 */

#include "fuse-debug.h"
#include "logger/logger.h"

/* set to LOG_UNDEFINED -- log commands will use default level */
log_level fuse_debug_level = LOG_UNDEFINED; 
log_level fuse_call_debug_level = LOG_UNDEFINED; 
