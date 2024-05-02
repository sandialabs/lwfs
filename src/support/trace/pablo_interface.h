/**  @file pablo_interface.h
 *   
 *   @brief A simple tracing API. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 406 $.
 *   $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $.
 *
 */
 
#ifndef _PABLO_INTERFACE_H_
#define _PABLO_INTERFACE_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

    /* initialization and wrapup functions */
    extern int pablo_interface_init(const char *fname, const int ftype);

    extern int pablo_interface_fini(void);

    extern int pablo_output_generic_event(
	    const int eventID,
	    const int pid,
	    const char *data);

    extern int pablo_output_interval_event(
	    const int eventID,
	    const int pid,
	    const int level,
	    const char *data,
	    double duration);

    extern int pablo_output_tput_event(
	    const int eventID,
	    const int pid,
	    const int level,
	    const char *data,
	    double duration,
	    const long num_processed);

    extern int pablo_output_count_event(
	    const int eventID,
	    const int pid,
	    const char *data,
	    const int count);


#endif


#ifdef __cplusplus
}
#endif

#endif
