/*-------------------------------------------------------------------------*/
/**  @file stats.h
 *   
 *   @brief API for calculating statistics. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */
 
#include <stdio.h>
#ifndef _STATS_H_
#define _STATS_H_

#include <stdio.h>
#include "stats-debug.h"

#ifdef __cplusplus
extern "C" {
#endif


#if defined(__STDC__) || defined(__cplusplus)


	extern double stats_max(
			const double *vals, 
			const int len, 
			int *index); 

	extern double stats_min(
			const double *vals, 
			const int len, 
			int *index);

	extern double stats_mean(
			const double *vals, 
			const int len);

	extern double stats_variance(
			const double *vals, 
			const int len);

	extern double stats_stddev(
			const double *vals, 
			const int len);

#endif


#ifdef __cplusplus
}
#endif

#endif
