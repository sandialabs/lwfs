/**  @file thread_pool_debug.c
 *   
 *   @brief Set the global debug level for the authorization service.
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1.4 $.
 *   $Date: 2005/03/23 23:56:59 $.
 *
 */

#include "thread_pool_debug.h"
#include "support/logger/logger.h"

/* set to LOG_UNDEFINED -- log commands will use default level */
log_level thread_debug_level = LOG_UNDEFINED; 
