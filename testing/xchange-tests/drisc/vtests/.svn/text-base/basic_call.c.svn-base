#include "stdio.h"
#include "drisc.h"

static int verbose = 0;

int gg(int a, int b) {
    printf("In gg  a=%d, b=%d\n", a, b);
     return a+b;
}
int ff(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    printf("In ff  a=%d, b=%d, c=%d, d=%d, e=%d, f=%d, g=%d, h=%d, i=%d, j=%d\n",
	   a, b, c, d, e, f, g, h, i, j);
     return a+b+c+d+e+f+g+h+i+j;
}

void a () {
    drisc_ctx c = dr_vinit();
     dr_reg_t a,b,p3,d,e,f,g,h,i,j,w,z,cnt;
/*     dr_reg_t func;*/
     int L1;
     int (*ip)();

     dr_proc(c, "a_gen", DR_I);
     L1 = dr_genlabel(c);
/*     func = dr_getvreg(c, DR_P);*/
     cnt = dr_getvreg(c, DR_P);
     a = dr_getvreg(c, DR_I);
     b = dr_getvreg(c, DR_I);
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

/*     dr_setp(c, func, ff);*/
     dr_seti(c, a, 1);
     dr_seti(c, b, 2);
     dr_seti(c, p3, 3);
     dr_seti(c, d, 4);
     dr_seti(c, e, 5);
     dr_seti(c, f, 6);
     dr_seti(c, g, 7);
     dr_seti(c, h, 8);
     dr_seti(c, i, 9);
     dr_seti(c, j, 0);
     dr_seti(c, z, 0);
     dr_seti(c, cnt, 0);
     dr_label(c, L1);
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, a);
	 dr_push_argi(c, b);
	 dr_push_argi(c, p3);
	 dr_push_argi(c, d);
	 dr_push_argi(c, e);
	 dr_push_argi(c, f);
	 dr_push_argi(c, g);
	 dr_push_argi(c, h);
	 dr_push_argi(c, i);
	 dr_push_argi(c, j);
     } else {
	 dr_push_argi(c, j);
	 dr_push_argi(c, i);
	 dr_push_argi(c, h);
	 dr_push_argi(c, g);
	 dr_push_argi(c, f);
	 dr_push_argi(c, e);
	 dr_push_argi(c, d);
	 dr_push_argi(c, p3);
	 dr_push_argi(c, b);
	 dr_push_argi(c, a);
     }	 
     w = dr_calli(c, (void*)ff);
     dr_addi(c, z,z,w);
     dr_addii(c, cnt,cnt,1);
     dr_bltii(c, cnt,2,L1);
     dr_addi(c, z, z, a);
     dr_addi(c, z, z, b);
     dr_addi(c, z, z, p3);
     dr_addi(c, z, z, d);
     dr_addi(c, z, z, e);
     dr_addi(c, z, z, f);
     dr_addi(c, z, z, g);
     dr_addi(c, z, z, h);
     dr_addi(c, z, z, i);
     dr_addi(c, z, z, j);
     dr_reti(c, z);			/* (9*10/2)*3 = 135 */
     ip = (int(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("**135=%d\n", (*ip)());
     dr_free_context(c);
}

void b () {
     dr_reg_t f;
     void *(*pp)();
     
     drisc_ctx c = dr_vinit();

     dr_proc_params(c, "b_gen", DR_I, "%i%i");
     f = dr_getvreg(c, DR_P);
     dr_setp(c, f, (long)gg);
     dr_retp(c, f);
     pp = (void *(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("**3=%d\n", (*(int (*)(int, int))(long)((*pp)()))(1,2));
     dr_free_context(c);
}

void c () {
     dr_reg_t a,b,f;
     int (*ip)();
     
     drisc_ctx c = dr_vinit();

     dr_proc(c, "c_gen", DR_I);
     a = dr_getvreg(c, DR_I);
     b = dr_getvreg(c, DR_I);
     f = dr_getvreg(c, DR_I);

     dr_setp(c, f, (long)gg);
     dr_seti(c, a, 1);
     dr_seti(c, b, 2);  
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, a);
	 dr_push_argi(c, b);
     } else {
	 dr_push_argi(c, b);
	 dr_push_argi(c, a);
     }
     a = dr_callri(c, f);
     dr_reti(c, a);
     ip = (int(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("**3=%d\n", (*ip)());
     dr_free_context(c);
}

void d () {
     dr_reg_t a,b;
     int (*ip)();
     
     drisc_ctx c = dr_vinit();

     dr_proc(c, "d_gen", DR_I);
     a = dr_getvreg(c, DR_I);
     b = dr_getvreg(c, DR_I);

     dr_seti(c, a, 1);
     dr_seti(c, b, 2);  
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, a);
	 dr_push_argi(c, b);
     } else {
	 dr_push_argi(c, b);
	 dr_push_argi(c, a);
     }
     a = dr_calli(c, (void*)gg);
     dr_reti(c, a);
     ip = (int(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("**3=%d\n", (*ip)());
     dr_free_context(c);
}

void e () {
     dr_reg_t a,b,p3,d,e,f,g,h,i,j,k,l;
     int (*ip)(int (*)(int,int),int,int,int,int,int,int,int,int,int);
     
     drisc_ctx c = dr_vinit();

     dr_proc_params(c, "e_gen", DR_I, "%p%i%i%i%i%i%i%i%i%i");
     f = dr_vparam(c, 0);
     a = dr_vparam(c, 1);
     b = dr_vparam(c ,2);
     p3 = dr_vparam(c, 3);
     d = dr_vparam(c, 4);
     e = dr_vparam(c, 5);
     g = dr_vparam(c, 6);
     h = dr_vparam(c, 7);
     i = dr_vparam(c, 8);
     j = dr_vparam(c, 9);
     k = dr_getvreg(c, DR_I);
     l = dr_getvreg(c, DR_I);

     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, a);
	 dr_push_argi(c, b);
     } else {
	 dr_push_argi(c, b);
	 dr_push_argi(c, a);
     }
     k = dr_callri(c, f);
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, p3);
	 dr_push_argi(c, d);
     } else {
	 dr_push_argi(c, d);
	 dr_push_argi(c, p3);
     }
     d = dr_callri(c, f);
     dr_addi(c, k,k,d);
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, e);
	 dr_push_argi(c, g);
     } else {
	 dr_push_argi(c, g);
	 dr_push_argi(c, e);
     }
     g = dr_callri(c, f);
     dr_addi(c, k,k,g);
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argi(c, h);
	 dr_push_argi(c, i);
     } else {
	 dr_push_argi(c, i);
	 dr_push_argi(c, h);
     }
     i = dr_callri(c, f);
     dr_addi(c, k,k,i);
     dr_addi(c, k,k,j);
     dr_addii(c, l,k,3);
     dr_reti(c, l);
     ip = (int(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("**48=%d\n", (*ip)(gg,1,2,3,4,5,6,7,8,9));
     dr_free_context(c);
}

void f () {
     dr_reg_t a, b, d;
     double dp = 3.14159;
     int ip = 5;
     char *pp = "hello!";
     void *(*proc)();
     
     drisc_ctx c = dr_vinit();

     dr_proc_params(c, "f_gen", DR_I, "");
     a = dr_getvreg(c, DR_D);
     b = dr_getvreg(c, DR_I);
     d = dr_getvreg(c, DR_P);
     dr_setd(c, a, dp);
     dr_seti(c, b, ip);
     dr_setp(c, d, (long)pp);
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argpi(c, (void*)"values are %d, %g, %s\n");
	 dr_push_argi(c, b);
	 dr_push_argd(c, a);
	 dr_push_argp(c, d);
     } else {
	 dr_push_argp(c, d);
	 dr_push_argd(c, a);
	 dr_push_argi(c, b);
	 dr_push_argpi(c, (void*)"values are %d, %g, %s\n");
     }
     a = dr_calli(c, (void*)printf);
     dr_reti(c, a);
     proc = (void *(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("expect: values are %d, %g, %s\n", ip, dp, pp);
     proc();
     dr_free_context(c);
}

#ifdef NOTDEF
void g () {
     dr_reg_t a, b, d, e, f, g, h, i;
     double da = 3.1;
     double db = 4.1;
     double dd = 5.1;
     double de = 6.1;
     double df = 7.1;
     double dg = 8.1;
     double dh = 9.1;
     double di = 10.1;
     void *(*proc)();
     
     drisc_ctx c = dr_vinit();

     dr_proc_params(c, "g_gen", "");
     a = dr_getvreg(c, DR_D);
     b = dr_getvreg(c, DR_D);
     d = dr_getvreg(c, DR_D);
     e = dr_getvreg(c, DR_D);
     f = dr_getvreg(c, DR_D);
     g = dr_getvreg(c, DR_D);
     h = dr_getvreg(c, DR_D);
     i = dr_getvreg(c, DR_D);
     dr_setd(c, a, da);
     dr_setd(c, b, db);
     dr_setd(c, d, dd);
     dr_setd(c, e, de);
     dr_setd(c, f, df);
     dr_setd(c, g, dg);
     dr_setd(c, h, dh);
     dr_setd(c, i, di);
     dr_push_init(c);
     if (!dr_do_reverse_vararg_push(c)) {
	 dr_push_argpi(c, (void*)"values are %g, %g, %g, %g, %g, %g, %g, %g, %g\n");
	 dr_push_argd(c, a);
	 dr_push_argd(c, b);
	 dr_push_argd(c, d);
	 dr_push_argd(c, e);
	 dr_push_argd(c, f);
	 dr_push_argd(c, g);
	 dr_push_argd(c, h);
	 dr_push_argd(c, i);
     }
     a = dr_calli(c, (void*)printf);
     dr_reti(c, a);
     proc = (void *(*)())dr_end(c);

     if (verbose) dr_dump(c);

     printf("expect: values are %g, %g, %g, %g, %g, %g, %g, %g\n", da, db, dd, de, df, dg, dh, di);
     proc();
     dr_free_context(c);
}
#endif

int 
main(int argc, char **argv)
{
    if (argc > 1) verbose++;
    printf("########## A\n");
    a();
    printf("########## B\n");
    b();
    printf("########## C\n");
    c();
    printf("########## D\n");
    d();
    printf("########## E\n");
    e();
    printf("########## F\n");
    f();
    printf("########## end\n");
    return 0;
}
