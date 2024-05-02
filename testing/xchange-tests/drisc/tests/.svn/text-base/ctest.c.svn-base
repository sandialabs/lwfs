#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "drisc.h"

int main(int argc, char **argv) 
{ 
    dr_reg_t r;
    int i;
    drisc_ctx c = dr_init();
    void (*func)();
    int verbose = 0, no_float = 0;

    for (i=1; i < argc; i++) {
	if (strcmp(argv[i], "-no_float") == 0) {
	    no_float++;
	} else if (strcmp(argv[i], "-v") == 0) {
	    verbose++;
	}
    }

    dr_proc(c, "foo", DR_V);
    dr_getreg(c, &r, DR_I, DR_TEMP);
	
    dr_push_init(c);
    if (!dr_do_reverse_vararg_push(c)) {
	dr_push_argpi(c, "Hello: %d %d %d %d\n");
	dr_seti(c, r, 10);
	dr_push_argi(c, r);
	dr_seti(c, r,20);
	dr_push_argi(c, r);
	dr_seti(c, r,30);
	dr_push_argi(c, r);
	dr_seti(c, r,40);
	dr_push_argi(c, r);
    } else {
	dr_seti(c, r,40);
	dr_push_argi(c, r);
	dr_seti(c, r,30);
	dr_push_argi(c, r);
	dr_seti(c, r,20);
	dr_push_argi(c, r);
	dr_seti(c, r, 10);
	dr_push_argi(c, r);
	dr_push_argpi(c, "Hello: %d %d %d %d\n");
    }
    dr_callv(c, (void*)printf);
    func = (void (*)())dr_end(c);
    if (verbose) dr_dump(c);
    func();

    dr_proc(c, "foo", DR_V);
    dr_push_init(c);
    if (!dr_do_reverse_vararg_push(c)) {
	dr_push_argpi(c, "Hello: %d %d %d %d\n");
	dr_push_argii(c, 10);
	dr_push_argii(c, 20);
	dr_push_argii(c, 30);
	dr_push_argii(c, 40);
    } else {
	dr_push_argii(c, 40);
	dr_push_argii(c, 30);
	dr_push_argii(c, 20);
	dr_push_argii(c, 10);
	dr_push_argpi(c, "Hello: %d %d %d %d\n");
    }
    dr_callv(c, (void*)printf);
    func = (void (*)()) dr_end(c);
    if (verbose) dr_dump(c);
    func();

    dr_proc(c, "foo", DR_V);
    dr_push_init(c);
    if (!dr_do_reverse_vararg_push(c)) {
	dr_push_argpi(c, "Hello: %d %d %d %d %d %d %d %d %d %d\n");
	dr_push_argii(c, 10);
	dr_push_argii(c, 20);
	dr_push_argii(c, 30);
	dr_push_argii(c, 40);
	dr_push_argii(c, 50);
	dr_push_argii(c, 60);
	dr_push_argii(c, 70);
	dr_push_argii(c, 80);
	dr_push_argii(c, 90);
	dr_push_argii(c, 100);
    } else {
	dr_push_argii(c, 100);
	dr_push_argii(c, 90);
	dr_push_argii(c, 80);
	dr_push_argii(c, 70);
	dr_push_argii(c, 60);
	dr_push_argii(c, 50);
	dr_push_argii(c, 40);
	dr_push_argii(c, 30);
	dr_push_argii(c, 20);
	dr_push_argii(c, 10);
	dr_push_argpi(c, "Hello: %d %d %d %d %d %d %d %d %d %d\n");
    }
    dr_callv(c, (void*)printf);
    func = (void (*)())dr_end(c);
    if (verbose) dr_dump(c);
    func();

    if (!no_float) {
	dr_proc(c, "foo", DR_V);
	dr_push_init(c);
	if (!dr_do_reverse_vararg_push(c)) {
	    dr_push_argpi(c, "Hello: %e %e %e %e %e %e %e %e %e %e\n");
	    dr_push_argdi(c, 10.0);
	    dr_push_argdi(c, 20.0);
	    dr_push_argdi(c, 30.0);
	    dr_push_argdi(c, 40.0);
	    dr_push_argdi(c, 50.0);
	    dr_push_argdi(c, 60.0);
	    dr_push_argdi(c, 70.0);
	    dr_push_argdi(c, 80.0);
	    dr_push_argdi(c, 90.0);
	    dr_push_argdi(c, 100.0);
	} else {
	    dr_push_argdi(c, 100.0);
	    dr_push_argdi(c, 90.0);
	    dr_push_argdi(c, 80.0);
	    dr_push_argdi(c, 70.0);
	    dr_push_argdi(c, 60.0);
	    dr_push_argdi(c, 50.0);
	    dr_push_argdi(c, 40.0);
	    dr_push_argdi(c, 30.0);
	    dr_push_argdi(c, 20.0);
	    dr_push_argdi(c, 10.0);
	    dr_push_argpi(c, "Hello: %e %e %e %e %e %e %e %e %e %e\n");
	}
	dr_callv(c, (void*)printf);
	func = (void (*)())dr_end(c);
	if (verbose) dr_dump(c);
	func();
    } else {
	/* just do the printf so the output is the same */
	printf("Hello: %e %e %e %e %e %e %e %e %e %e\n", 10.0, 20.0, 30.0, 
	       40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0);
    }
    return 0;
}
