
#include <stdio.h>
#include <rpc/rpc.h> /* xdr is a sub-library of the rpc library */
#include "support/logger/logger.h"
#include "xdr_tests_common.h"
#include "test_xdr.h"

int main(int argc, char **argv)                /* writer.c */
{
	XDR xdrs;
	int success;  /* return code */
	int size; 

	/* initialize the logger */
	logger_set_file(stderr); 
	logger_set_default_level(LOG_DEBUG); 
	
	/* initialize the tests */
	log_info(xdr_debug_level, "initializing data");
	xdr_test_init();  

	/* figure out how big the buffer is */
	size = xdr_test_size(); 
	//size = 1048576;  // one MB

	/* create xdrs that write to a buffer */
	log_info(xdr_debug_level, "creating xdrs, size = %d",size);
	char *buf = (char *)malloc(size); 

	xdrmem_create(&xdrs, buf, size, XDR_ENCODE);

	/* perform the encode tests */
	success = xdr_test_encode(&xdrs); 

	/* now we have to write the buffer to stdout */
	FILE *fp = stdout; 
	int len = xdr_getpos(&xdrs); 

	fwrite(buf, len, 1, fp); 

	/* finialize the tests */
	xdr_test_fini();

	return success; 
}
