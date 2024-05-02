/* Test that save/restore and locals do not overlap in the activation record. */
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>

#include "../config.h"
#include "drisc.h"
#ifdef USE_MMAP_CODE_SEG
#include "sys/mman.h"
#endif

void * mk_test(drisc_ctx c) {
	int i, regs, fregs;
	int l[100];
	int abortl1;
	int abortl2;
	int abortl3;

	dr_reg_t r[32];
	dr_reg_t fr[32];

	dr_proc_params(c, "foo", DR_I, "%i%d");

	abortl1 = dr_genlabel(c);
	abortl2 = dr_genlabel(c);
	abortl3 = dr_genlabel(c);

	for(i = 0; i < 5 && dr_getreg(c, &r[i], DR_I, DR_TEMP); i++) {
/*		(void)("allocated register %d\n", i);*/
		dr_seti(c, r[i], i);
		dr_savei(c, r[i]); 
	}
	regs = i;

	for(i = 0; dr_getreg(c, &fr[i], DR_D, DR_TEMP); i++) {
/*		(void)("allocated register %d\n", i);*/
		dr_setd(c, fr[i], (double)i);
		dr_saved(c, fr[i]); 
	}
	fregs = i;

	for(i = 0; i < 10; i++) {
		l[i] = dr_local(c, DR_I);
			
		dr_seti(c, dr_param(c,0), 100 + i);
		dr_stii(c, dr_param(c,0), dr_lp(c), l[i]);
	}

	/* call a procedure, then verify that everything is in place. */
	dr_scallv(c, (void*)printf, "%P", "hello world!\n");


	for(i = 0; i < regs; i++) {
		dr_restorei(c, r[i]); 
		dr_bneii(c, r[i], i, abortl1);
	}

	for(i = 0; i < fregs; i++) {
/*		(void)("allocated register %d\n", i);*/
		dr_restored(c, fr[i]); 
		dr_setd(c, dr_param(c,1), (double)i);
		dr_bned(c, fr[i], dr_param(c,1), abortl2);
	}

	for(i = 0; i < 10; i++) {
		dr_ldii(c, dr_param(c, 0), dr_lp(c), l[i]);
		dr_bneii(c, dr_param(c, 0), 100 + i, abortl3);
	}
	dr_retii(c, 0);

	dr_label(c, abortl1);
	dr_retii(c, 1);		/* failure. */
	dr_label(c, abortl2);
	dr_retii(c, 2);		/* failure. */
	dr_label(c, abortl3);
	dr_retii(c, 3);		/* failure. */

	return dr_end(c);
}

int main() { 
	int (*ip)();
	drisc_ctx c = dr_init();
	int ret;
	char *target;

	ip = (int (*)()) mk_test(c);
	ret = ip();

	if(ret == 0) {
		printf("success!\n");
	} else {
		dr_dump(c);
		printf("failure at point %d!\n", ret);
	}
	ip = (int (*)()) mk_test(c);
	ret = ip();
	if(ret == 0)
		printf("success!\n");
	else {
		dr_dump(c);
		printf("failure at point %d (second)\n", ret);
	}
#ifdef USE_MMAP_CODE_SEG
	{
	    int size = dr_code_size(c);
	    static unsigned long ps = -1;
	    if (ps == -1) {
	        ps = (getpagesize ());
	    }
	    if (ps > size) size = ps;
	    target = (void*)mmap(0, size, 
				 PROT_EXEC | PROT_READ | PROT_WRITE, 
				 MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	}
	if (target == (void*)-1) perror("mmap");
#else
	target = (void*)malloc(dr_code_size(c));
#endif
	ip = (int (*)()) dr_clone_code(c, target, dr_code_size(c));
	dr_free_context(c);
	ret = ip();
	if(ret == 0)
		printf("success!\n");
	else {
		dr_dump(c);
		printf("failure at point %d (third)\n", ret);
	}

	return 0;
}
