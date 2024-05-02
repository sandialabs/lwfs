/**  @file naming_debug.c
 *   
 *   @brief Set the global debug level for the naming service.
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

#include "stats-debug.h"
#include "support/logger/logger.h"

/* set to LOG_UNDEFINED -- log commands will use default level */
log_level stats_debug_level = LOG_UNDEFINED; 
