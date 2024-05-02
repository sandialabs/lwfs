/**  @file stats.c
 *   
 *   @brief Functions for statistics calculations.
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 1391 $
 *   @date $Date: 2007-04-24 11:55:02 -0600 (Tue, 24 Apr 2007) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <math.h>

#include <unistd.h>
#include "stats.h"
#include "stats-debug.h"
#include "support/logger/logger.h"


double stats_max(const double *vals, const int len, int *index)
{
	int i; 
	double max = vals[0]; 
	*index = 0; 

	for (i=0; i<len; i++) {
		if (vals[i] > max) {
			*index = i; 
			max = vals[i]; 
		}
	}

	return max;
}

double stats_min(const double *vals, const int len, int *index)
{
	int i; 
	double min = vals[0]; 
	*index = 0; 

	for (i=0; i<len; i++) {
		if (vals[i] < min) {
			*index = i; 
			min = vals[i];
		}
	}

	return min;
}

double stats_mean(const double *vals, const int len)
{
	int i; 
	double sum = vals[0]; 

	for (i=1; i<len; i++) {
		sum += vals[i]; 
	}

	return sum/len; 
}

double stats_variance(const double *vals, const int len)
{
	int i; 
	double mean = stats_mean(vals, len); 
	double sumsq = (vals[0]-mean)*(vals[0]-mean); 

	for (i=1; i<len; i++) {
		sumsq += (vals[i]-mean)*(vals[i]-mean); 
	}

	return sumsq/len; 
}

double stats_stddev(const double *vals, const int len)
{
	return sqrt(stats_variance(vals, len)); 
}

