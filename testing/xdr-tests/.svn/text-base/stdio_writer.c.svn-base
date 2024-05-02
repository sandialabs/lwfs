
#include <stdio.h>
#include <rpc/rpc.h> /* xdr is a sub-library of the rpc library */

#include "support/logger/logger.h"
#include "xdr_tests_common.h"
#include "test_xdr.h"

   
int main(int argc, char **argv)                /* writer.c */
{
	XDR xdrs;
	int rc;  /* return code */

	/* initialize the logger */
	logger_set_file(stderr); 
	logger_set_default_level(LOG_DEBUG); 
	
	/* initialize the tests */
	log_info(xdr_debug_level, "initializing data\n");
	xdr_test_init();  

	/* create xdrs for standard output */
	log_info(xdr_debug_level, "creating xdrs\n");
	xdrstdio_create(&xdrs, stdout, XDR_ENCODE);

	//rc = xdr_array_encode(&xdrs); 

	/* perform the encode tests */
	rc = xdr_test_encode(&xdrs); 

	/* encode an opaque array */
	/*
	opaque_array array;   
	snprintf(array, OPAQUE_ARRAY_LEN, "ron");
	if (!xdr_opaque_array(&xdrs, array)) {
		log_error(xdr_debug_level, "unable to encode opaque array");
		return -1; 
	}
	*/

	/* finialize the tests */
	xdr_test_fini();

	return rc; 
}
