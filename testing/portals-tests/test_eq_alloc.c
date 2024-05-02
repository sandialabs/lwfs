/* hello world server */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <portals3.h>
#include <p3nal_utcp.h>
#include <p3rt/p3rt.h>

/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 

int main(int argc, char **argv)
{
	int rc; 
	int num_if; 
	ptl_handle_ni_t ni_h;
	ptl_handle_eq_t eq1_h, eq2_h; 

	/* these must be set, otherwisze, PtlNIInit segfaults */
	utcp_api_out = stdout; 
	utcp_lib_out = stdout; 

	/* Initialize the library */
	if (PtlInit(&num_if) != PTL_OK) {
		fprintf(stderr, "PtlInit() failed\n");
		exit(1); 
	}

	/* Initialize the interface (PIDS client=98, server=99) */
	if (PtlNIInit(PTL_IFACE_DEFAULT, PTL_PID_ANY, 
				NULL, NULL, &ni_h) != PTL_OK) {
		fprintf(stderr, "PtlNIInit() failed\n"); 
		exit(1); 
	}

	/* create an event queue  */
	if ((rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq1_h)) != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s\n",
			PtlErrorStr(rc)); 
		exit(-1); 
	}

	if ((rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq2_h)) != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s\n",
			PtlErrorStr(rc)); 
		exit(-1); 
	}

	/* free the event queue */

	/* free the event queue */
	if ((rc = PtlEQFree(eq2_h)) != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s \n",
			PtlErrorStr(rc)); 
		exit(-1); 
	}

	if ((rc = PtlEQFree(eq1_h)) != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s \n",
			PtlErrorStr(rc)); 
		exit(-1); 
	}


	/* cleanup (valgrind should report no leaks) */
	PtlNIFini(ni_h);
	PtlFini();

	return 0;
}
