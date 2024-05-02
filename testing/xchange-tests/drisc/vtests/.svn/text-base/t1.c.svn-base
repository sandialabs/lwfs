#include "../config.h"
#include "stdio.h"
#include "malloc.h"
#include "unistd.h"

#include "drisc.h"
#ifdef USE_MMAP_CODE_SEG
#include "sys/mman.h"
#endif

static int verbose = 0;

void a () {
    drisc_ctx c = dr_vinit();
    char *target;
    dr_reg_t a,b,p3,d,e,f,g,h,i,j,w,z;
    drisc_exec_ctx ec;
     int (*ip)();

     dr_proc_params(c, "a_gen", DR_I, "%EC%i%i");

     a = dr_vparam(c, 1);
     b = dr_vparam(c, 2);
     p3 = dr_getvreg(c, DR_I);
     d = dr_getvreg(c, DR_I);
     e = dr_getvreg(c, DR_I);
     f = dr_getvreg(c, DR_I);
     g = dr_getvreg(c, DR_I);
     h = dr_getvreg(c, DR_I);
     i = dr_getvreg(c, DR_I);
     j = dr_getvreg(c, DR_I);
     z = dr_getvreg(c, DR_I);
     w = dr_getvreg(c, DR_I);

     dr_addii(c, p3, a, 5);
     dr_addi(c, d, a, b);
     dr_addi(c, e, d, p3);
     dr_movi(c, f, e);

     p3 = dr_getvreg(c, DR_I);
     d = dr_getvreg(c, DR_I);
     e = dr_getvreg(c, DR_I);
     dr_addii(c, p3, a, 5);
     dr_addi(c, d, a, b);
     dr_addi(c, e, d, p3);

     dr_addi(c, f, f, e);
     dr_reti(c, f);
     ip = (int(*)())dr_end(c);

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
     if (verbose) dr_dump(c);

     ec = dr_get_exec_context(c);
     printf("**18=%d\n", (*ip)(ec, 1, 2));
     dr_free_context(c);
}

int 
main(int argc, char **argv)
{
    if (argc > 1) verbose++;
    printf("########## A\n");
    a();
    printf("########## end\n");
    return 0;
}
