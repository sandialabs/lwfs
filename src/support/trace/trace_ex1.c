
#include <unistd.h>
#include "support/trace/trace.h"

static void testFunction( int n )
{
  	int	j;
	int pid = 0;

  	trace_event(3, pid, "entered testFunction");

  	for (j = 0; j < n; j++) {
    	   trace_event(2, pid, "inside for(j) loop");
  	}

  	trace_event(4, pid, "exiting testFunction");
}

int main(int argc, char *argv[])
{
	int i;
	int end = 3; 
	int level = 0; 
	int eventID = 0; 
	int pid = 0;
	char fname[256];

	sprintf(fname, "%s.sddf", argv[0]);

	/* turn on tracing */
	trace_enable();

	trace_set_file_name(fname);

	trace_event(eventID, pid, "start");

	trace_start_interval(0);

	for (i = 1; i <= end; i++) {
		trace_start_interval(i);
		trace_inc_count(2, pid, "loop counter");
		trace_event(1, pid, "top of for(i) loop");
		testFunction(i);
		sleep(end-i);
		trace_end_interval(i, 0, pid+i, level+1, "internal loop interval");
	}

	trace_end_interval(0, eventID, pid, level, "outside loop interval");

	trace_put_all_counts();

	trace_fini();

	return 0;
}

