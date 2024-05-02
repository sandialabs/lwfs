
#include <stdio.h>
#include <rpc/rpc.h> /* xdr is a sub-library of the rpc library */
#include <support/logger/logger.h>
#include "xdr_tests_common.h"
#include "test_xdr.h"
   
int main(int argc, char **argv)                /* writer.c */
{
	XDR xdrs;
	int success;  /* return code */

	/* initialize the logger */
	logger_set_file(stderr); 
	logger_set_default_level(LOG_DEBUG); 
	
	/* initialize the tests */
	xdr_test_init();  

	/* create xdrs for standard input */
	xdrstdio_create(&xdrs, stdin, XDR_DECODE);

	//success = xdr_array_decode(&xdrs);

	/* decode the data structures */
	success = xdr_test_decode(&xdrs); 

	/* decode an opaque array */
	/*
	opaque_array array;   
	if (!xdr_opaque_array(&xdrs, array)) {
		log_error(xdr_debug_level, "unable to encode opaque array");
		return -1; 
	}
	log_info(xdr_debug_level, "decoded array = %s", array);
	*/

	/* finialize the tests */
	xdr_test_fini();

	return success; 
}
