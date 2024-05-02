/**  @file timer.c
 *   
 *   @brief An lwfs timer.
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 406 $
 *   @date $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <sys/time.h>
#include "support/logger/logger.h"


/** 
 * @brief Return the time in seconds.
 *
 */
double lwfs_get_time()
{
	double seconds = 0.0; 

#if defined(HAVE_CLOCK_GETTIME)

 	struct timespec tp; 
	clockid_t clockid = CLOCK_REALTIME;
	//clockid_t clockid = CLOCK_MONOTONIC;
	//clockid_t clockid = CLOCK_PROCESS_CPUTIME_ID;
	//clockid_t clockid = CLOCK_THREAD_CPUTIME_ID;
	
	static int initialized = 0;
	static struct timespec res; 

	if (!initialized) {
		clock_getres(clockid, &res);
		initialized = 1; 
	}

	clock_gettime(clockid, &tp);

	seconds = ((double) tp.tv_sec + 1.0e-9*(double) tp.tv_nsec);

#elif defined(HAVE_GETTIMEOFDAY)

	struct timeval tp;
	gettimeofday( &tp, NULL );
	seconds = ((double) tp.tv_sec + 1.0e-6*(double) tp.tv_usec);

#else
	seconds = -1.0;
#endif
	
	return seconds;
}

/**
 * @brief Return the time in milliseconds.
 */
long lwfs_get_time_ns()
{
	double t_ns = lwfs_get_time()*1.0e9; 
	return (long)t_ns; 
}
