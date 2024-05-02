/*-------------------------------------------------------------------------*/
/**  @file sysmon.h
 *   
 *   @brief Method prototypes for the sysmon API.
 *   
 *   The sysmon API is a simple API for getting high level system statistics.  
 *   This include uptime, memory,  loadavg, vm, disk and slab info statistics.  
 *   This info is provided by the proc lib that is part of procps package.  
 *   Find it here: http://procps.sf.net/procps-3.2.7.tar.gz
 *   
 *   @author Todd Kordenbrock (thkorde\@sandia.gov)
 *   $Revision: $
 *   $Date: $
 *
 */
 
#ifndef _SYSMON_H_
#define _SYSMON_H_

#include <stdio.h>

#include "support/logger/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the functions */

#if defined(__STDC__) || defined(__cplusplus)

void log_meminfo(const log_level debug_level);
unsigned long main_memory_used(void);

#endif


#ifdef __cplusplus
}
#endif

#endif /* !_SYSMON_H_ */
