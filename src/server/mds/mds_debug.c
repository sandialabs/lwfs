/*-------------------------------------------------------------------------*/
/**  
 *   @file mds_debug.c
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 *
 */

#include "logger/logger.h"
#include "mds/mds_debug.h"

/* set to LOG_UNDEFINED -- log commands will use default level */
log_level mds_debug_level = LOG_UNDEFINED; 

