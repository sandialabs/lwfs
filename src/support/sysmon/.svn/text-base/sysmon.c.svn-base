/**  @file trace.c
 *   
 *   @brief A simple library for generating timing traces. 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 406 $
 *   @date $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysmon.h"
#include "meminfo.h"


#include "support/logger/logger.h"


void log_meminfo(const log_level debug_level)
{
	struct lwfs_meminfo mi;

	int show_high=0;
	int show_total=0;

	meminfo(&mi);

	log_debug(debug_level, "               total         used         free       shared      buffers       cached");
	log_debug(debug_level, 
		"%-7s %10lukB %10lukB %10lukB %10lukB %10lukB %10lukB", 
		"Mem:",
		mi.kb_main_total,
		mi.kb_main_used,
		mi.kb_main_free,
		mi.kb_main_shared,
		mi.kb_main_buffers,
		mi.kb_main_cached
	);
	// Print low vs. high information, if the user requested it.
	// Note we check if low_total==0: if so, then this kernel does
	// not export the low and high stats.  Note we still want to
	// print the high info, even if it is zero.
	if (show_high) {
		log_debug(debug_level, 
			"%-7s %10lukB %10lukB %10lukB", 
			"Low:",
			mi.kb_low_total,
			mi.kb_low_total - mi.kb_low_free,
			mi.kb_low_free
		);
		log_debug(debug_level, 
			"%-7s %10lukB %10lukB %10lukB", 
			"High:",
			mi.kb_high_total,
			mi.kb_high_total - mi.kb_high_free,
			mi.kb_high_free
		);
	}
	log_debug(debug_level, 
		"%-7s %10lukB %10lukB %10lukB", 
		"Swap:",
		mi.kb_swap_total,
		mi.kb_swap_used,
		mi.kb_swap_free
	);
	if(show_total){
		log_debug(debug_level, 
			"%-7s %10lukB %10lukB %10lukB", 
			"Total:",
			mi.kb_main_total + mi.kb_swap_total,
			mi.kb_main_used  + mi.kb_swap_used,
			mi.kb_main_free  + mi.kb_swap_free
		);
	}
}

unsigned long main_memory_used(void)
{
	struct lwfs_meminfo mi;

	meminfo(&mi);

	return mi.kb_main_used;
}
