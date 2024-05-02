#include "config.h"

#ifdef LINUX_KERNEL_MODULE
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#endif

#include "drisc.h"
#include "drisc_internal.h"

#ifndef LINUX_KERNEL_MODULE
#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef USE_MMAP_CODE_SEG
#include <sys/mman.h>
#endif
#else
#include <linux/string.h>
#include <linux/ctype.h>
#include "kdrisc.h"

#define malloc (void *)DAllocMM
#define free(a) DFreeMM((addrs_t)a)
#define realloc(a,b) (void *)DReallocMM((addrs_t)a, b)
#define fprintf(fmt, args...)	printk(args)
#define printf	printk
#define exit sys_exit
#undef USE_MMAP_CODE_SEG
extern char *getenv(const char *name);
#endif

static void init_code_block(drisc_ctx c);
static void free_code_blocks(drisc_ctx c);

drisc_ctx static_ctx = (drisc_ctx)0;

static void
reset_regset(reg_set* regs)
{
    *(regs->avail) = *(regs->init_avail);
    memset(&regs->used, 0, sizeof(regs->used));
    memset(&regs->mustsave, 0, sizeof(regs->mustsave));
}

static void
dr_register_init(drisc_ctx c)
{
    reset_regset(&c->p->var_i);
    reset_regset(&c->p->tmp_i);
    reset_regset(&c->p->var_f);
    reset_regset(&c->p->tmp_f);
}

static void
dr_branch_init(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    int i;

    t->next_label = 0;
    for (i=0; i<t->max_alloc; i++) {
	t->label_locs[i] = -1;
    }
    t->branch_count = 0;
    t->data_mark_count = 0;
}

static void
dr_call_init(drisc_ctx c)
{
    struct call_table *t = &c->p->call_table;

    t->call_count = 0;
}

static void
dr_ret_init(drisc_ctx c)
{
    struct ret_table *t = &c->p->ret_table;

    t->ret_count = 0;
}

static void
reset_context(drisc_ctx c)
{
    c->p->mach_reset(c);
    c->p->cur_ip = c->p->code_base;
    dr_register_init(c);
    dr_branch_init(c);
    dr_call_init(c);
    dr_ret_init(c);
}

extern void dr_sparc_init(drisc_ctx c);
extern void dr_sparcv9_init(drisc_ctx c);
extern void dr_x86_init(drisc_ctx c);
extern void dr_x86_64_init(drisc_ctx c);
extern void dr_arm_init(drisc_ctx c);
extern void dr_ia64_init(drisc_ctx c);

static void null_init(drisc_ctx c) {}

static int
set_mach_reset(drisc_ctx c, char *arch)
{
#if defined(MULTI_TARGET) || defined(HOST_SPARC) || defined(HOST_SPARCV9)
    if (strcmp(arch, "sparc") == 0) {
	c->p->mach_reset = dr_sparc_init;
	return 1;
    } else if (strcmp(arch, "sparcv9") == 0) {
	c->p->mach_reset = dr_sparcv9_init;
	return 1;
    } else 
#endif
#if defined(MULTI_TARGET) || defined(HOST_X86)
    if (strcmp(arch, "x86") == 0) {
	c->p->mach_reset = dr_x86_init;
	return 1;
    } else 
#endif
#if defined(MULTI_TARGET) || defined(HOST_X86_64)
    if (strcmp(arch, "x86_64") == 0) {
	c->p->mach_reset = dr_x86_64_init;
	return 1;
    } else 
#endif
#if defined(MULTI_TARGET) || defined(HOST_IA64)
    if (strcmp(arch, "ia64") == 0) {
	c->p->mach_reset = dr_ia64_init;
	return 1;
    } else 
#endif
#if defined(MULTI_TARGET) || defined(HOST_ARM)
    if (strcmp(arch, "arm5") == 0) {
	c->p->mach_reset = dr_arm_init;
	return 1;
    }
#endif
#if defined(EMULATION_ONLY)
    c->p->mach_reset = null_init;
    return 1;
#endif
    return 0;
}

extern void
dr_free_context(drisc_ctx c)
{
    if (c->p->branch_table.label_locs) free(c->p->branch_table.label_locs);
    if (c->p->branch_table.branch_locs) free(c->p->branch_table.branch_locs);
    if (c->p->branch_table.data_marks) free(c->p->branch_table.data_marks);
    free(c->p->call_table.call_locs);
    free(c->p->ret_table.ret_locs);
    free(c->p->c_param_regs);
    free(c->p->c_param_args);
    free(c->p->c_param_structs);
    free_code_blocks(c);
    if (c->p->mach_info) free(c->p->mach_info);
    if (c->p->vregs) free(c->p->vregs);
    if (c->p->virtual.mach_info) free(c->p->virtual.mach_info);
    if (c->p->native.mach_info) free(c->p->native.mach_info);
    free(c->p);
    free(c);
}

extern drisc_ctx
dr_cross_init(char *arch)
{
    drisc_ctx c = (drisc_ctx) malloc(sizeof(struct drisc_ctx_s));
    char *env = getenv("DRISC_DEBUG");
    struct branch_table *bt;
    struct call_table *ct;
    struct ret_table *rt;
    c->p = (private_ctx) malloc(sizeof(struct drisc_private_ctx));
    memset(c->p, 0, sizeof(struct drisc_private_ctx));
    if (env == NULL) {
	c->drisc_debug = 0;
    } else {
	c->drisc_debug = 1;
    }
    c->p->mach_info = NULL;
    if (!set_mach_reset(c, arch)) {
	fprintf(stderr, "DRISC support for architecture %s not found.\n", arch);
	free(c->p);
	free(c);
	return NULL;
    }
    init_code_block(c);
    c->p->cur_ip = c->p->code_base;
    bt = &c->p->branch_table;
    bt->max_alloc = 1;
    bt->label_locs = malloc(sizeof(bt->label_locs[0]));
    bt->branch_alloc = 1;
    bt->branch_locs = malloc(sizeof(bt->branch_locs[0]));
    bt->data_mark_count = 0;
    bt->data_marks = malloc(sizeof(bt->data_marks[0]));
    ct = &c->p->call_table;
    ct->call_alloc = 1;
    ct->call_count = 0;
    ct->call_locs = malloc(sizeof(ct->call_locs[0]));
    rt = &c->p->ret_table;
    rt->ret_alloc = 1;
    rt->ret_count = 0;
    rt->ret_locs = malloc(sizeof(rt->ret_locs[0]));
    c->p->c_param_count = 0;
    c->p->c_param_regs = NULL;
    c->p->c_param_args = NULL;
    c->p->c_param_structs = NULL;
    c->p->vreg_count = 0;
    c->p->vregs = malloc(1);
    reset_context(c);
    c->p->native.mach_jump = c->j;
    c->p->native.mach_reset = c->p->mach_reset;
    c->p->native.mach_info = NULL;
    c->p->native.code_base = NULL;
    c->p->native.cur_ip = c->p->cur_ip;
    c->p->native.code_limit = c->p->code_limit;
    c->p->virtual.mach_jump = NULL;
    c->p->virtual.mach_reset = NULL;
    c->p->virtual.mach_info = NULL;
    c->p->virtual.code_base = NULL;
    c->p->virtual.cur_ip = NULL;
    c->p->virtual.code_limit = NULL;
    c->p->unavail_called = 0;
    return c;
}

extern void dr_virtual_init(drisc_ctx c);



extern drisc_ctx
dr_vinit()
{
    drisc_ctx c;
    c = dr_cross_init(NATIVE_ARCH);
    c->p->native_mach_reset = c->p->mach_reset;
    c->p->native.code_base = c->p->code_base;
    c->p->native.mach_info = c->p->mach_info;
    c->p->mach_reset = dr_virtual_init;
    init_code_block(c);
    c->p->cur_ip = c->p->code_base;
    c->p->mach_reset = dr_virtual_init;
    c->p->mach_info = NULL;

    c->p->virtual.mach_jump = c->j;
    c->p->virtual.mach_reset = c->p->mach_reset;
    c->p->virtual.mach_info = c->p->mach_info;
    c->p->virtual.code_base = c->p->code_base;
    c->p->virtual.cur_ip = c->p->cur_ip;
    c->p->virtual.code_limit = c->p->code_limit;
    return c;
}
    

extern drisc_ctx
dr_init()
{
    return dr_cross_init(NATIVE_ARCH);
}

static void
extend_params(drisc_ctx c, int argno)
{
    int i;
    if (c->p->c_param_count == 0) {
	c->p->c_param_regs = malloc(sizeof(c->p->c_param_regs[0]) * (argno + 2));
	c->p->c_param_args = malloc(sizeof(c->p->c_param_args[0]) * (argno + 2));
	c->p->c_param_structs = malloc(sizeof(c->p->c_param_structs[0]) * (argno + 2));
    } else if (c->p->c_param_count <= (argno + 1)) {
	c->p->c_param_regs = realloc(c->p->c_param_regs,
				  sizeof(c->p->c_param_regs[0]) * (argno + 2));
	c->p->c_param_args = realloc(c->p->c_param_args,
				  sizeof(c->p->c_param_args[0]) * (argno + 2));
	c->p->c_param_structs = realloc(c->p->c_param_structs,
				     sizeof(c->p->c_param_structs[0]) * (argno + 2));
    }
    for (i = c->p->c_param_count; i <= argno; i++) {
	c->p->c_param_regs[i] = NULL;
	c->p->c_param_args[i].type = DR_V;
	c->p->c_param_args[i].is_register = 0;
	c->p->c_param_args[i].is_immediate = 0;
	c->p->c_param_args[i].in_reg = 0;
	c->p->c_param_args[i].out_reg = 0;
	c->p->c_param_args[i].offset = 0;
	c->p->c_param_structs[i] = NULL;
    }
    c->p->c_param_count = (argno + 1);
}

extern dr_reg_t
dr_vparam(drisc_ctx c, int param_no)
{
    return param_no;
}

extern int
dr_param(drisc_ctx c, int argno)
{
    if (argno >= c->p->c_param_count) {
	printf("Warning, dr_param requested param %d, largest is %d\n", 
	       argno, c->p->c_param_count-1);
	return -1;
    }
    return c->p->c_param_args[argno].in_reg;
}

extern void
dr_param_alloc(drisc_ctx c, int argno, int type, dr_reg_t *reg_p)
{
    extend_params(c, argno);
    c->p->c_param_regs[argno] = reg_p;
    c->p->c_param_args[argno].type = type;
}

extern void
dr_param_struct_alloc(drisc_ctx c, int argno, int type, 
		     drisc_parameter_type *struct_p)
{
    extend_params(c, argno);
    c->p->c_param_structs[argno] = struct_p;
    c->p->c_param_args[argno].type = type;
}

extern void
dr_proc(drisc_ctx c, char *subr_name, int ret_type)
{
    int i;
    if (!c->p->unavail_called) reset_context(c);
    c->p->ret_type = ret_type;
    c->p->unavail_called = 0;
    (c->j->proc_start)(c, subr_name, c->p->c_param_count, c->p->c_param_args,
		       NULL);
    for (i=0; i < c->p->c_param_count; i++) {
	if (c->p->c_param_regs[i] != NULL) {
	    *c->p->c_param_regs[i] = c->p->c_param_args[i].in_reg;
	}
	if (c->p->c_param_structs[i] != NULL) {
	    c->p->c_param_structs[i]->is_register =
		c->p->c_param_args[i].is_register;
	    c->p->c_param_structs[i]->reg = c->p->c_param_args[i].in_reg;
	    c->p->c_param_structs[i]->offset = c->p->c_param_args[i].offset;
	}
    }	    
    c->p->c_param_count = 0;
    if (c->p->c_param_regs) {
	free(c->p->c_param_regs);
	c->p->c_param_regs = NULL;
    }
    if (c->p->c_param_args) {
	free(c->p->c_param_args);
	c->p->c_param_args = NULL;
    }
    if (c->p->c_param_structs) {
	free(c->p->c_param_structs);
	c->p->c_param_structs = NULL;
    }
}

extern void *
dr_get_fp(drisc_ctx c)
{
    return (void *) c->p->fp;
}

extern void *
dr_end(drisc_ctx c)
{
    (c->j->end)(c);
    c->p->save_param_count = c->p->c_param_count;
    c->p->c_param_count = 0;
    return dr_get_fp(c);
}

extern int
dr_code_size(drisc_ctx c)
{
    long native_base = (long) c->p->native.code_base;
    if (native_base == 0) {
        native_base = (long) c->p->code_base;
    }
    return  ((long)c->p->cur_ip) - native_base;
}

extern void *
dr_clone_code(drisc_ctx c, void *new_base, int size)
{
    return (c->j->clone_code)(c, new_base, size);
}

extern void *
dr_take_code(drisc_ctx c)
{
    void *ret = c->p->code_base;
    c->p->code_base = NULL;
    c->p->native.code_base = NULL;
    return ret;
}

extern int
dr_genlabel(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    if (t->next_label == t->max_alloc) {
	t->max_alloc++;
	t->label_locs = realloc(t->label_locs, sizeof(int)*t->max_alloc);
    }
    t->label_locs[t->next_label] = -1;
    return t->next_label++;
}

extern void dr_label(drisc_ctx c, int label)
{
    struct branch_table *t = &c->p->branch_table;
    int label_loc = (long)c->p->cur_ip - (long)c->p->code_base;
    t->label_locs[label] = label_loc;
}

extern void dr_mark_branch_location(drisc_ctx c, int label)
{
    struct branch_table *t = &c->p->branch_table;
    int branch_loc = (long)c->p->cur_ip - (long)c->p->code_base;

    if (t->branch_count == t->branch_alloc) {
	t->branch_alloc++;
	t->branch_locs = realloc(t->branch_locs, 
				 sizeof(struct branch_location)*t->branch_alloc);
    }
    t->branch_locs[t->branch_count].label = label;
    t->branch_locs[t->branch_count].loc = branch_loc;
    t->branch_count++;
}

extern void dr_mark_ret_location(drisc_ctx c)
{
    struct ret_table *t = &c->p->ret_table;
    int ret_loc = (long)c->p->cur_ip - (long)c->p->code_base;

    if (t->ret_count == t->ret_alloc) {
	t->ret_alloc++;
	t->ret_locs = realloc(t->ret_locs, sizeof(int)*t->ret_alloc);
    }
    t->ret_locs[t->ret_count] = ret_loc;
    t->ret_count++;
}

extern void dr_mark_call_location(drisc_ctx c, char *xfer_name, 
				 void *xfer_address)
{
    struct call_table *t = &c->p->call_table;
    int call_loc = (long)c->p->cur_ip - (long)c->p->code_base;

    if (t->call_count == t->call_alloc) {
	t->call_alloc++;
	t->call_locs = realloc(t->call_locs, 
			       sizeof(struct call_location)*t->call_alloc);
    }
    t->call_locs[t->call_count].loc = call_loc;
    t->call_locs[t->call_count].xfer_addr = xfer_address;
    t->call_locs[t->call_count].xfer_name = xfer_name;
    t->call_locs[t->call_count].mach_info = NULL;
    t->call_count++;
}

extern void
dr_dmark(drisc_ctx c, void *addr, int label)
{
    struct branch_table *t = &c->p->branch_table;
    t->data_marks = malloc(sizeof(t->data_marks[0]) * (t->data_mark_count+1));
    t->data_marks[t->data_mark_count].addr = addr;
    t->data_marks[t->data_mark_count].label = label;
    t->data_mark_count++;
}

static
arg_info_list
translate_arg_str(char *string, int *count)
{
    int cnt = 0;
    arg_info_list list = malloc(sizeof(list[0]));
    
    while (string && (*string != 0)) {
	if (*string != '%') {
	    fprintf(stderr, "invalid format, expected %%, got \"%c\"\n", 
		    *string);
	    return NULL;
	}
	string++;
	list[cnt].is_register = 0;
	list[cnt].is_immediate = (isupper((int)*string) != 0);
	list[cnt].in_reg = 0;
	list[cnt].out_reg = 0;
	list[cnt].offset = 0;
	list[cnt].type = 0;
	switch(tolower(*string)) {
        case 'u':
	    string++;
	    switch(tolower(*string)) {
	    case 'l': list[cnt].type = DR_UL; string++; break;
	    case 's': list[cnt].type = DR_US; string++; break;
	    case 'c': list[cnt].type = DR_UC; string++; break;
	    case '%': case '\0':
		list[cnt].type = DR_U; break;
	    default:
		fprintf(stderr, "invalid format, unexpect char \"%c\" after %%u\n", *string);
	    }
	    break;
        case 'p': list[cnt].type = DR_P; string++; break;
        case 'l': list[cnt].type = DR_L; string++; break;
        case 'i': list[cnt].type = DR_I; string++; break;
        case 's': list[cnt].type = DR_S; string++; break;
        case 'c': list[cnt].type = DR_C; string++; break;
        case 'b': list[cnt].type = DR_B; string++; break;
        case 'f': list[cnt].type = DR_F; string++; break;
        case 'd': list[cnt].type = DR_D; string++; break;
        case 'e': 
	    string++;
	    if (tolower(*string) == 'c') {
		if (cnt == 0) {
		    list[cnt].type = DR_EC; string++; break;
		} else {
		    fprintf(stderr, "%%ec format must be first format\n");
		}
	    } else {
		fprintf(stderr, "invalid format, unexpect char \"%c\" after %%e\n", *string);
	    }
	    string++;
	    break;
        default:
	    fprintf(stderr, "invalid format, unexpect char \"%c\" after %%\n", *string);
	    string++;
	}
	cnt++;
	list = realloc(list, sizeof(list[0]) * (cnt+1));
    }
    *count = cnt;
    return list;
}

extern void
dr_proc_params(drisc_ctx c, char *name, int ret_type, char *arg_str)
{
    int arg_count = 0;
    arg_info_list args;
    if (!c->p->unavail_called) reset_context(c);
    c->p->c_param_count = 0;
    c->p->ret_type = ret_type;
    if (c->p->c_param_args != NULL) {
	free (c->p->c_param_args);
	c->p->c_param_args = NULL;
    }
    args = c->p->c_param_args = translate_arg_str(arg_str, &arg_count);
    c->p->c_param_count = arg_count;
    (c->j->proc_start)(c, name, arg_count, args, NULL);
}

#define END_OF_CODE_BUFFER 60
static void
init_code_block(drisc_ctx c)
{
    static unsigned long size = INIT_CODE_SIZE;
#ifdef USE_MMAP_CODE_SEG
    static unsigned long ps = -1;
    if (ps == -1) {
        ps = (getpagesize ());
    }
    if (ps > size) size = ps;
    c->p->code_base = (void*)mmap(0, 4096/*INIT_CODE_SIZE*/, 
				  PROT_EXEC | PROT_READ | PROT_WRITE, 
				  MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (c->p->code_base == (void*)-1) perror("mmap");
#else
    c->p->code_base = (void*)malloc(size);
#endif
    c->p->code_limit = ((char*)c->p->code_base) + size - END_OF_CODE_BUFFER;
}

static void
free_code_blocks(drisc_ctx c)
{
#ifdef USE_MMAP_CODE_SEG
    if (c->p->code_base) {
	int size = (long)c->p->code_limit - (long)c->p->code_base + END_OF_CODE_BUFFER;
        if (munmap(c->p->code_base, size) == -1) perror("unmap 1");
    }
    if (c->p->virtual.code_base) {
	int vsize = (long)c->p->virtual.code_limit - (long)c->p->virtual.code_base + END_OF_CODE_BUFFER;
        if (munmap(c->p->code_base, vsize) == -1) perror("unmap v");
    }
    if (c->p->native.code_base) {
	int nsize = (long)c->p->native.code_limit - (long)c->p->native.code_base + END_OF_CODE_BUFFER;
        if (munmap(c->p->code_base, nsize) == -1) perror("unmap n");
    }
#else
    if (c->p->code_base) free(c->p->code_base);
    if (c->p->virtual.code_base) free(c->p->virtual.code_base);
    if (c->p->native.code_base) free(c->p->native.code_base);
#endif
}

extern void
expand_drisc_buffer(drisc_ctx c)
{
    int size = (long)c->p->code_limit - (long)c->p->code_base + END_OF_CODE_BUFFER;
    int cur_ip = (long)c->p->cur_ip - (long)c->p->code_base;
    int new_size = size * 2;
#ifdef USE_MMAP_CODE_SEG
    {
	void *old = c->p->code_base;
	void *new = mmap(0, new_size,
			 PROT_EXEC | PROT_READ | PROT_WRITE, 
			 MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	if (new == (void*)-1) perror("mmap1");
	memcpy(new, old, size);
	c->p->code_base = new;
	if (munmap(old, size) == -1) perror("munmap exp");
    }
#else
    c->p->code_base = realloc(c->p->code_base, new_size);
#endif
    c->p->cur_ip = ((char*)c->p->code_base) + cur_ip;
    c->p->code_limit = ((char*)c->p->code_base) + new_size - END_OF_CODE_BUFFER;
}

extern jmp_table alloc_drisc_jump_table()
{
    jmp_table tmp = malloc(sizeof(struct jmp_table_s));
    memset(tmp, 0, sizeof(struct jmp_table_s));
    tmp->jmp_a3 = malloc(sizeof(arith_op3) * (dr_jmp_a3_size + 1));
    memset(tmp->jmp_a3, 0, sizeof(arith_op3) * (dr_jmp_a3_size + 1));
    tmp->a3_data = malloc(sizeof(jmp_data) * (dr_jmp_a3_size + 1));
    memset(tmp->a3_data, 0, sizeof(jmp_data) * (dr_jmp_a3_size + 1));
    tmp->jmp_a3i = malloc(sizeof(arith_op3i) * (dr_jmp_a3_size + 1));
    memset(tmp->jmp_a3i, 0, sizeof(arith_op3i) * (dr_jmp_a3_size + 1));
    tmp->a3i_data = malloc(sizeof(jmp_data) * (dr_jmp_a3_size + 1));
    memset(tmp->a3i_data, 0, sizeof(jmp_data) * (dr_jmp_a3_size + 1));
    tmp->jmp_a2 = malloc(sizeof(arith_op2) * (dr_jmp_a2_size + 1));
    memset(tmp->jmp_a2, 0, sizeof(arith_op2) * (dr_jmp_a2_size + 1));
    tmp->a2_data = malloc(sizeof(jmp_data) * (dr_jmp_a2_size + 1));
    memset(tmp->a2_data, 0, sizeof(jmp_data) * (dr_jmp_a2_size + 1));
    tmp->jmp_b = malloc(sizeof(branch_op) * (dr_jmp_branch_size + 1));
    memset(tmp->jmp_b, 0, sizeof(branch_op) * (dr_jmp_branch_size + 1));
    tmp->jmp_bi = malloc(sizeof(branch_op) * (dr_jmp_branch_size + 1));
    memset(tmp->jmp_bi, 0, sizeof(branch_op) * (dr_jmp_branch_size + 1));
    tmp->b_data = malloc(sizeof(jmp_data) * (dr_jmp_branch_size + 1));
    memset(tmp->b_data, 0, sizeof(jmp_data) * (dr_jmp_branch_size + 1));
    return tmp;
}

#define BIT_ON(i, bitvec) (((((long)1)<<reg) & bitvec[0]) == (((long)1)<<reg))
#define SET_BIT(i, bitvec) (bitvec[0] |= (((long)1)<<reg))
#define RESET_BIT(i, bitvec) (bitvec[0] &= (~(((long)1)<<reg)))

int 
drisc_mustsave(reg_set *regs, int reg)
{
    return BIT_ON(reg, regs->mustsave);
}

int 
drisc_wasused(reg_set *regs, int reg)
{
    return BIT_ON(reg, regs->used);
}

static int
reg_alloc(reg_set *regs)
{
    int reg;
    if (regs->avail[0] == 0) {
	return -1;
    }
    reg = 0;
    while (!BIT_ON(reg, regs->avail)) {
	reg++;
    }
    RESET_BIT(reg, regs->avail);
    return reg;
}

extern void
dr_rawput(drisc_ctx c, dr_reg_t reg, int type, int class)
{
    switch(type) {
    case DR_D: case DR_F:
	if (class == DR_VAR) {
	    SET_BIT(reg, c->p->var_f.avail);
	    RESET_BIT(reg, c->p->var_f.mustsave);
	} else {
	    SET_BIT(reg, c->p->tmp_f.avail);
	}
	break;
    default:
	if (class == DR_VAR) {
	    SET_BIT(reg, c->p->var_i.avail);
	    RESET_BIT(reg, c->p->var_i.mustsave);
	} else {
	    SET_BIT(reg, c->p->tmp_i.avail);
	}
	break;
    }
}

extern void
dr_rawget(drisc_ctx c, dr_reg_t reg, int type, int class)
{
    switch(type) {
    case DR_D: case DR_F:
	if (class == DR_VAR) {
	    RESET_BIT(reg, c->p->var_f.avail);
	    SET_BIT(reg, c->p->var_f.mustsave);
	} else {
	    RESET_BIT(reg, c->p->tmp_f.avail);
	}
	break;
    default:
	if (class == DR_VAR) {
	    RESET_BIT(reg, c->p->var_i.avail);
	    SET_BIT(reg, c->p->var_i.mustsave);
	} else {
	    RESET_BIT(reg, c->p->tmp_i.avail);
	}
	break;
    }
}

extern void
dr_mk_unavail(drisc_ctx c, int type, dr_reg_t reg)
{
    if (c->p->unavail_called == 0) {
	reset_context(c);
	c->p->unavail_called = 1;
    }
    switch(type) {
    case DR_D: case DR_F:
	RESET_BIT(reg, c->p->var_f.avail);
	RESET_BIT(reg, c->p->tmp_f.avail);
	break;
    default:
	RESET_BIT(reg, c->p->var_i.avail);
	RESET_BIT(reg, c->p->tmp_i.avail);
	break;
    }
}

extern void
dr_mk_avail(drisc_ctx c, int type, dr_reg_t reg)
{
    switch(type) {
    case DR_D: case DR_F:
	if (BIT_ON(reg, c->p->tmp_f.members)) {
	    SET_BIT(reg, c->p->tmp_f.avail);
	} else if (BIT_ON(reg, c->p->var_f.members)) {
	    SET_BIT(reg, c->p->var_f.avail);
	} else {
	    printf("mk avail not in set error %d\n", reg);
	}
	break;
    default:
	if (BIT_ON(reg, c->p->tmp_i.members)) {
	    SET_BIT(reg, c->p->tmp_i.avail);
	} else if (BIT_ON(reg, c->p->var_i.members)) {
	    SET_BIT(reg, c->p->var_i.avail);
	} else {
	    printf("mk avail not in set error %d\n", reg);
	}
	break;
    }
}

extern int
dr_getvreg(drisc_ctx c, int typ)
{
    c->p->vregs = realloc(c->p->vregs, 
			  (c->p->vreg_count +1) * sizeof(vreg_info));
    c->p->vregs[c->p->vreg_count].typ = typ;
    c->p->vregs[c->p->vreg_count].use_info.use_count = 0;
    c->p->vregs[c->p->vreg_count].use_info.def_count = 0;
    c->p->vregs[c->p->vreg_count].offset = 0xdeadbeef;
    return ((c->p->vreg_count++) + 100);
}

extern int
dr_getvblock(drisc_ctx c, int size)
{
    c->p->vregs = realloc(c->p->vregs, 
			  (c->p->vreg_count +1) * sizeof(vreg_info));
    c->p->vregs[c->p->vreg_count].typ = DR_B;
    c->p->vregs[c->p->vreg_count].offset = size;
    c->p->vregs[c->p->vreg_count].use_info.use_count = 0;
    c->p->vregs[c->p->vreg_count].use_info.def_count = 0;
    return ((c->p->vreg_count++) + 100);
}

extern int 
dr_getreg(drisc_ctx c, dr_reg_t *reg_p, int type, int class)
{
    int reg = -1;
    switch (type) {
    case DR_D: case DR_F:
	if (class == DR_VAR) {
	    if ((reg = reg_alloc(&c->p->var_f)) == -1) {
		reg = reg_alloc(&c->p->tmp_f);
	    }
	    if (reg != -1) {
		SET_BIT(reg, c->p->tmp_f.mustsave);
		SET_BIT(reg, c->p->tmp_f.used);
	    }
	    *reg_p = reg;
	    return (reg != -1);
	} else {
	    if ((reg = reg_alloc(&c->p->tmp_f)) == -1) {
		reg = reg_alloc(&c->p->var_f);
	    }
	    if (reg != -1) {
		SET_BIT(reg, c->p->tmp_f.used);
	    }
	    *reg_p = reg;
	    return (reg != -1);
	}
	break;
    default:
	if (class == DR_VAR) {
	    if ((reg = reg_alloc(&c->p->var_i)) == -1) {
		reg = reg_alloc(&c->p->tmp_i);
		if (reg != -1) {
		    SET_BIT(reg, c->p->tmp_i.mustsave);
		    SET_BIT(reg, c->p->tmp_i.used);
		}
	    }
	    *reg_p = reg;
	    return (reg != -1);
	} else {
	    if ((reg = reg_alloc(&c->p->tmp_i)) == -1) {
		reg = reg_alloc(&c->p->var_i);
	    }
	    if (reg != -1) {
		SET_BIT(reg, c->p->tmp_i.used);
	    }
	    *reg_p = reg;
	    return (reg != -1);
	}
	break;
    }
}	

extern void
dr_putreg(drisc_ctx c, dr_reg_t reg, int type)
{
    switch (type) {
    case DR_F: case DR_D:
	if (BIT_ON(reg, c->p->tmp_f.members)) {
	    dr_rawput(c, reg, type, DR_TEMP);
	} else if (BIT_ON(reg, c->p->var_f.members)) {
	    dr_rawput(c, reg, type, DR_VAR);
	} else {
	    /*
	     * special case, if the put reg is invalid (-1) and 
	     * the member set is empty, give no warning.
	     */
	    if ((c->p->var_f.members[0] != 0) || (reg != -1)) {
		printf("Putreg not in set error %d\n", reg);
	    }
	}
	break;
    default:
	if (BIT_ON(reg, c->p->tmp_i.members)) {
	    dr_rawput(c, reg, type, DR_TEMP);
	} else if (BIT_ON(reg, c->p->var_i.members)) {
	    dr_rawput(c, reg, type, DR_VAR);
	} else {
	    printf("Putreg not in set error %d\n", reg);
	}
	break;
    }
}	

extern int 
dr_do_reverse_vararg_push(drisc_ctx c)
{
    if (c->j->do_reverse_push) {
	c->p->doing_reverse_push = 1;
	return 1;
    }
    return 0;
}

extern void dr_end_vararg_push(drisc_ctx c)
{
    c->p->doing_reverse_push = 0;
}

typedef union {
    unsigned int u;
    int i;
    long l;
    unsigned long ul;
    void *p;
    float f;
    double d;
} type_union;

static void 
do_vararg_push(drisc_ctx c, char *arg_str, va_list ap)
{
    int i, arg_count;
    int reverse = 0;
    arg_info_list args = translate_arg_str(arg_str, &arg_count);
    type_union value[256];

    dr_push_init(c);

    reverse = dr_do_reverse_vararg_push(c);

    for(i = 0; i < arg_count; i++) {
	if(!args[i].is_immediate) {
	    value[i].i = va_arg(ap, int);
	} else {
	    switch(args[i].type) {
	    case DR_UC: case DR_US: case DR_U: 
		value[i].u = va_arg(ap, unsigned);
		break;
	    case DR_C:  case DR_S:  case DR_I:
		value[i].i = va_arg(ap, int);
		break;
	    case DR_L:
		value[i].l = va_arg(ap, long);
		break;
	    case DR_UL:
		value[i].ul = va_arg(ap, unsigned long);
		break;
	    case DR_P:
		value[i].p = va_arg(ap, void *);
		break;
	    case DR_F:
		value[i].f = (float)va_arg(ap, double);
		break;
	    case DR_D:
		value[i].d = va_arg(ap, double);
		break;
	    default:
		fprintf(stderr, 
			"do_push_args: unknown type\n"); 
		exit(1);
	    }
	}
    }

    /* push all arguments */
    for(i = 0; i < arg_count; i++) {
	int arg = i;
	if (reverse) {
	    arg = arg_count - i - 1;
	}
	if(!args[arg].is_immediate) {
	    dr_push_arg(c, args[arg].type, value[arg].i);
	} else {
	    switch(args[arg].type) {
	    case DR_UC: case DR_US: case DR_U: 
		dr_push_argui(c, value[arg].u);
		break;
	    case DR_C:  case DR_S:  case DR_I:
		dr_push_argii(c, value[arg].i);
		break;
	    case DR_L:
		dr_push_argli(c, value[arg].l);
		break;
	    case DR_UL:
		dr_push_arguli(c, value[arg].ul);
		break;
	    case DR_P:
		dr_push_argpi(c, value[arg].p);
		break;
	    case DR_F:
		dr_push_argfi(c, value[arg].f);
		break;
	    case DR_D:
		dr_push_argdi(c, value[arg].d);
		break;
	    default:
		fprintf(stderr, 
			"do_push_args: unknown type\n"); 
		exit(1);
	    }
	}
    }
    free(args);
}


void dr_scallv(drisc_ctx c, void *ptr, char *arg_str, ...)
{
    va_list ap;

    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    (void) c->j->calli(c, DR_V, ptr);
    va_end(ap);
}

int dr_scalli(drisc_ctx c, void* ptr, char *arg_str, ...)
{
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_I, ptr);
    va_end(ap);
    return ret_reg;
}

int dr_scallu(drisc_ctx c, void *ptr, char *arg_str, ...) {
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_U, ptr);
    va_end(ap);
    return ret_reg;
}

int dr_scallp(drisc_ctx c, void *ptr, char *arg_str, ...) {
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_P, ptr);
    va_end(ap);
    return ret_reg;
}

int dr_scallul(drisc_ctx c, void *ptr, char *arg_str, ...) {
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_UL, ptr);
    va_end(ap);
    return ret_reg;
}

int dr_scalll(drisc_ctx c, void *ptr, char *arg_str, ...) {
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_L, ptr);
    va_end(ap);
    return ret_reg;
}

int dr_scallf(drisc_ctx c, void *ptr, char *arg_str, ...) {
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_F, ptr);
    va_end(ap);
    return ret_reg;
}

int dr_scalld(drisc_ctx c, void *ptr, char *arg_str, ...) {
    int ret_reg;
    va_list ap;
    va_start(ap, arg_str);
    do_vararg_push(c, arg_str, ap);
    ret_reg = c->j->calli(c, DR_D, ptr);
    va_end(ap);
    return ret_reg;
}

extern void
dr_pbr(drisc_ctx c, int op_type, int data_type, dr_reg_t src1, dr_reg_t src2, 
       int label)
{
    int typ, index;
    if ((op_type < dr_beq_code ) || (op_type > dr_bne_code)) {
	printf("Bad op type in dr_pbr\n");
    }
    switch (data_type) {
    case DR_I: typ = 0; break;
    case DR_U: typ = 1; break;
    case DR_L: typ = 2; break;
    case DR_UL: typ = 3; break;
    case DR_P: typ = 4; break;
    case DR_D: typ = 5; break;
    case DR_F: typ = 6; break;
    default:
	printf("BAD  data type in dr_pbr\n");
	typ = 0;
    }
    index = typ + 7 * op_type;
    (c->j->jmp_b)[index](c, c->j->b_data[index].data1, 
			 c->j->b_data[index].data2, src1, src2, label);
}

extern void
dr_dump_reg(drisc_ctx c, int typ, int reg)
{
    c->j->print_reg(c, typ, reg);
}

#if defined(HAVE_DIS_ASM_H) && !defined(NO_DISASSEMBLER)
/* GENERIC BINUTILS DISASSEMBLER */
/* include some things from libbfd so we don't have to have it all */
#include "dis-asm.h"
#ifndef NO_INLINED_BFD_PROCS
#if defined(BFD64)
bfd_vma
bfd_getl64 (addr)
#ifdef BFD_BYTE
     register const bfd_byte *addr;
#else
     const void *addr;
#endif
{
#if SIZEOF_LONG == 8
  register const unsigned char *addrp = addr;
  bfd_vma low, high;
  high= (((((((addrp[7] << 8) |
              addrp[6]) << 8) |
            addrp[5]) << 8) |
          addrp[4]));

  low = ((((((((bfd_vma)addrp[3] << 8) |
              addrp[2]) << 8) |
            addrp[1]) << 8) |
          addrp[0]) );

  return high << 32 | low;
#else
  return 0;
#endif

}
#endif

bfd_vma
bfd_getb32 (addr)
#ifdef BFD_BYTE
     register const bfd_byte *addr;
#else
     const void *addr;
#endif
{
  register const unsigned char *addrp = addr;
  return (((((bfd_vma)addrp[0] << 8) | addrp[1]) << 8)
	  | addrp[2]) << 8 | addrp[3];
}

bfd_vma
bfd_getl32 (addr)
#ifdef BFD_BYTE
     register const bfd_byte *addr;
#else
     const void *addr;
#endif
{
  register const unsigned char *addrp = addr;
  return (((((bfd_vma)addrp[3] << 8) | addrp[2]) << 8)
	  | addrp[1]) << 8 | addrp[0];
}

bfd_vma
bfd_getb16 (addr)
#ifdef BFD_BYTE
     register const bfd_byte *addr;
#else
     const void *addr;
#endif
{
  register const unsigned char *addrp = addr;
  return (addrp[0] << 8) | addrp[1];
}

bfd_vma
bfd_getl16 (addr)
#ifdef BFD_BYTE
     register const bfd_byte *addr;
#else
     const void *addr;
#endif
{
  register const unsigned char *addrp = addr;
  return (addrp[1] << 8) | addrp[0];
}

enum bfd_architecture
bfd_get_arch (abfd)
     bfd *abfd;
{
    return abfd->arch_info->arch;
}

void *
xmalloc(size)
int size;
{
    return (void*)malloc(size);
}

char *
xstrdup (s)
     const char *s;
{
  int len;
  char *ret;

  len = strlen (s);
  ret = xmalloc (len + 1);
  strcpy (ret, s);
  return ret;
}
#endif

extern void
dump_drisc_insn(drisc_ctx c, void *p)
{
    struct disassemble_info info;
    int l;

    if (c->j->init_disassembly(c, &info) == 0) return;

    printf("%lx  - %x - ", (unsigned long)p, (unsigned int)*(int*)p);
    l = c->j->print_insn(c, &info, p);
    printf("\n");
}

extern void
dump_cur_drisc_insn(drisc_ctx c)
{
    dump_drisc_insn(c, c->p->cur_ip);
}

extern void
dr_dump(drisc_ctx c)
{
    struct disassemble_info info;
    void *base = c->p->code_base;

    if (base == NULL) {
	base = c->p->native.code_base;
    }
    if (c->j->init_disassembly(c, &info) == 0) {
	printf("No disassembler provided 1\n");
    } else {
	void *p;
	int l;
	int insn_count = 0;
	for (p =base; p < c->p->cur_ip;) {
	    int i;
	    struct branch_table *t = &c->p->branch_table;
	    for (i=0; i < t->next_label; i++) {
		if (t->label_locs[i] == 
		    ((char*)p - (char*)base)) {
		    printf("L%d:\n", i);
		}
	    }
	    printf("%lx  - %x - ", (unsigned long)p, (unsigned)*(int*)p);
	    l = c->j->print_insn(c, &info, (void *)p);
	    printf("\n");
	    if (l == -1) return;
	    p = (char*)p + l;
	    insn_count++;
	}
	printf("\nDumped %d instructions\n\n", insn_count);
    }
}

#else
extern void
dump_cur_drisc_insn(drisc_ctx c)
{
}
extern void 
dr_dump(drisc_ctx c)
{
    printf("No disassembler provided\n");
}

#endif
