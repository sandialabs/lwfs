#include <assert.h>
#include <stdio.h>
#include "drisc.h"
#include <stdlib.h>

int main(int argc, char **argv) 
{ 
    drisc_ctx c = dr_init();
    int (*func)();
    int verbose = 0;

    {
	int result;
	dr_proc(c, "foo", DR_I);
	dr_retii(c, 5);
	func = (int (*)())dr_end(c);
	if (verbose) dr_dump(c);
	result = func();

	if (result != 5) {
	    printf("Test 1 failed, got %d instead of 5\n", result);
	    exit(1);
	}
    }

    return 0;
}
