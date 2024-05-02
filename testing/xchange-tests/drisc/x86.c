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

#include "assert.h"

#ifndef LINUX_KERNEL_MODULE
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#else
#include "kdrisc.h"
#define fprintf(fmt, args...)	printk(args)
#define printf	printk
#define malloc (void *)DAllocMM
#define free(a) DFreeMM((addrs_t) a)
#endif

#ifdef KPLUGINS_INTEGRATION
int dr_kplugins_integration = 1;
#else
int dr_kplugins_integration = 0;
#endif
/*
 * GANEV: note that we have to include "x86.h" _after_ including
 * "kdrisc.h" because it needs to #undef and then re-#define a few
 * symbols (namely, EAX, EBX, etc. defined in <asm/ptrace.h>
 */
#include "drisc.h"
#include "drisc_internal.h"
#include "x86.h"

#define MOV32 0x89
#define Mod(x) ((x)<<6)
#define RegOp(x) ((x)<<3)
#define RM(x) (x)
#define ModRM(mod,reg,rm) (Mod(mod)|RegOp(reg)|RM(rm))
#define SIB(scale, index, base) (((scale)<<6) | ((index)<<3) | (base))

#define INSN_OUT(c, i) printf("Bad opout, line %d\n", __LINE__)
#define x86_savei(c, imm) 
#define x86_seti(c, r, val) BYTE_OUT1I(c, 0xb8 + r, val);
#define x86_rshai(c, dest, src, imm) if (dest !=src) x86_movi(c, dest, src); BYTE_OUT3(c, 0xc1, ModRM(0x3, 0x7, dest), imm & 0xff);
#define x86_rshi(c, dest, src, imm) if (dest !=src) x86_movi(c, dest, src); BYTE_OUT3(c, 0xc1, ModRM(0x3, 0x5, dest), imm & 0xff);
#define x86_lshi(c, d, s, imm) if (d !=s) x86_movi(c, d, s); BYTE_OUT3(c, 0xc1, ModRM(0x3, 0x4, d), imm & 0xff);
#define x86_andi(c, dest, src, imm) x86_arith3i(c, 0x4, 0x2, dest, src, imm)
#define x86_movi(c, dest, src) 	do { if (src != dest) BYTE_OUT2(c, MOV32, ModRM(0x3, src, dest)); } while (0)

#define x86_push_reg(c, src) BYTE_OUT1(c, 0x50+src)
#define x86_pop_reg(c, src) BYTE_OUT1(c, 0x58+src)
#define x86_simple_ret(c) do {x86_pop_reg(c, EBX);x86_pop_reg(c, ESI);x86_pop_reg(c, EDI);x86_movi(c, ESP, EBP);x86_pop_reg(c, EBP); BYTE_OUT1(c, 0xc3);} while(0)
#define x86_nop(c) BYTE_OUT1(c, 0x90)

#define IREG 0
#define FREG 1

#define _temp_reg EAX
#define _frame_reg EBP
#define roundup(a,b) ((a + (b-1)) & (-b))

static 
struct basic_type_info 
{   char size;
    char align;
    char reg_type;
} type_info[] = {
    { 1, 1, IREG},  /* C */
    { 1, 1, IREG},  /* UC */
    { 2, 2, IREG},  /* S */
    { 2, 2, IREG},  /* US */
    { 4, 4, IREG},  /* I */
    { 4, 4, IREG},  /* U */
    { sizeof(long), sizeof(long), IREG},  /* UL */
    { sizeof(long), sizeof(long), IREG},  /* L */
    { sizeof(char*), sizeof(char*), IREG},  /* P */
    { sizeof(float), sizeof(float), FREG},  /* F */
    { sizeof(double), sizeof(double), FREG},  /* D */
    { 0, 8, IREG}, /* V */
    { -1, 8, IREG}, /* B */
    { sizeof(long), sizeof(long), IREG}, /* EC */
};

int x86_type_align[] = {
        1, /* C */
        1, /* UC */
        2, /* S */
        2, /* US */
        4, /* I */
        4, /* U */
        sizeof(unsigned long), /* UL */
        sizeof(long), /* L */
        sizeof(char*), /* P */
        4, /* F */
        4, /* D */
        4, /* V */
        4, /* B */
        sizeof(char*), /* EC */
};

int x86_type_size[] = {
        1, /* C */
        1, /* UC */
        2, /* S */
        2, /* US */
        4, /* I */
        4, /* U */
        sizeof(unsigned long), /* UL */
        sizeof(long), /* L */
        sizeof(char*), /* P */
        4, /* F */
        8, /* D */
        4, /* V */
        0, /* B */
        sizeof(char*), /* EC */
};

extern int
x86_local(drisc_ctx c, int type)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;

    smi->act_rec_size += roundup(type_info[type].size, smi->stack_align);
    return (-smi->act_rec_size) + smi->stack_constant_offset;
}

extern int
x86_localb(drisc_ctx c, int size)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    smi->act_rec_size = roundup(smi->act_rec_size, size);

    smi->act_rec_size += roundup(size, smi->stack_align);
    return (-smi->act_rec_size) + smi->stack_constant_offset;
}

extern int x86_local_op(drisc_ctx c, int flag, int val)
{
    int size = val;
    if (flag == 0) {
	size = type_info[val].size;
    }
    return x86_localb(c, size);
}	

extern void
x86_save_restore_op(drisc_ctx c, int save_restore, int type, int reg)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    if (save_restore == 0) { /* save */
	switch (type) {
	case DR_D: case DR_F:
	    /* should never happen on x86 */
	    break;
	default:
	    x86_pstorei(c, type, 0, reg, _frame_reg, smi->save_base + (reg) * smi->stack_align + smi->stack_constant_offset);
	    break;
	}
    } else {  /* restore */
	switch (type) {
	case DR_D: case DR_F:
	    /* should never happen on x86 */
	    break;
	default:
	    x86_ploadi(c, type, 0, reg, _frame_reg, smi->save_base + reg * smi->stack_align + smi->stack_constant_offset);
	    break;
	}
    }
}	

extern void
x86_proc_start(drisc_ctx c, char *subr_name, int arg_count, arg_info_list args,
	     dr_reg_t *arglist)
{
    int i;
    int cur_arg_offset = 0;

    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    smi->pending_prefix = 0;
    x86_push_reg(c, EBP);
    x86_movi(c, EBP, ESP);
    smi->backpatch_offset = (char*)c->p->cur_ip - (char*)c->p->code_base;

    /* make local space reservation constant big so we have a word to patch */
    /* use the nop op code so that if we don't use all of it we get nops */
    dr_subii(c, ESP, ESP, 0x90909090);

    x86_push_reg(c, EDI);   /* callee is supposed to save these */
    x86_push_reg(c, ESI);
    x86_push_reg(c, EBX);

    /* leave some space */ x86_local(c, DR_D);
    smi->conversion_word = x86_local(c, DR_D);
    smi->fcu_word = x86_local(c, DR_I);
    smi->save_base = x86_localb(c, 8 * 4);

    cur_arg_offset = 8;
    for (i = 0; i < arg_count; i++) {
	/* at most 2 args moved into registers */
	if ((args[i].type != DR_F) && (args[i].type != DR_D) && (i < 2)) {
	    int reg;
	    args[i].is_register = dr_getreg(c, &reg, DR_L, DR_VAR);
	    args[i].in_reg = reg;
	    args[i].out_reg = -1;
	} else {
	    args[i].is_register = 0;
	}
	args[i].offset = cur_arg_offset;
	cur_arg_offset += roundup(type_info[(int)args[i].type].size, smi->stack_align);
    }

    for (i = 0; i < arg_count; i++) {
	if (args[i].is_register && ((args[i].type != DR_F) && 
				    (args[i].type != DR_D))) {
	    if (arglist != NULL) arglist[i] = args[i].in_reg;
	    x86_ploadi(c, DR_I, 0, args[i].in_reg, EBP, args[i].offset);
	} else {
	    /* well, just leave it on the stack */
	}
    }
}

static unsigned char ld_opcodes[] = {
    0x8a, /* DR_C */
    0x8a, /* DR_UC */
    0x8b, /* DR_S */
    0x8b, /* DR_US */
    0x8b, /* DR_I */
    0x8b, /* DR_U */
    0x8b, /* DR_L */
    0x8b, /* DR_UL */
    0x8b, /* DR_P */
    0xd9, /* DR_F */
    0xdd, /* DR_D */
    0x00, /* DR_V */
    0x00, /* DR_B */
    0x8b, /* DR_EC */
};

static void x86_clear(c, dest)
drisc_ctx c;
int dest;
{
    BYTE_OUT2(c, 0x33, ModRM(0x3, dest, dest));  /* xor dest, dest */
}

extern void
x86_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    unsigned char opcode = ld_opcodes[type];
    int tmp_dest = dest;
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    switch (type) {
    case DR_F:
	opcode = 0xd9;
	tmp_dest = dest = 0;
	break;
    case DR_D:
	opcode = 0xdd;
	tmp_dest = dest = 0;
	break;
    case DR_C:
    case DR_UC:
	if (dest >= ESP) {
	    /* movb doesn't work for big regs, load to eax */
	    tmp_dest = EAX;
	}
	if (type == DR_UC) {
	    /* clear high bytes */
	    if (src == tmp_dest) {
		/* don't destroy source */
		tmp_dest = EAX;
	    }
	    x86_clear(c, tmp_dest);
	}
	break;
    case DR_S: case DR_US:
	if (type == DR_US) {
	    /* clear high bytes */
	    if (src == tmp_dest) {
		/* don't destroy source */
		tmp_dest = EAX;
	    }
	    x86_clear(c, tmp_dest);
	}
	BYTE_OUT1(c, 0x66);
	break;
    case DR_L: case DR_UL: case DR_P:
    /* fall through */
    default:
	break;
    }
    if (smi->pending_prefix != 0) {
        BYTE_OUT1(c, smi->pending_prefix);
	smi->pending_prefix = 0;
    }
    if (((long)offset <= 127) && ((long)offset > -128)) {
	BYTE_OUT3(c, opcode, ModRM(0x1, tmp_dest, src), offset & 0xff);
    } else {
	BYTE_OUT2I(c, opcode, ModRM(0x2, tmp_dest, src), offset);
    }
    switch(type){
    case DR_C:
	x86_lshi(c, dest, tmp_dest, 24);
	x86_rshi(c, dest, dest, 24);
	break;
    case DR_S:
	x86_lshi(c, dest, tmp_dest, 16);
	x86_rshi(c, dest, dest, 16);
	break;
    case DR_UC: case DR_US:
	if (dest != tmp_dest)
	    x86_movi(c, dest, tmp_dest);
	break;
    }
}

extern void
x86_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    unsigned char opcode = ld_opcodes[type];
    int tmp_dest = dest;
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    switch(type){
    case DR_C:
    case DR_UC:
	if (dest >= ESP) {
	    /* movb doesn't work for big regs, load to eax */
	    tmp_dest = EAX;
	}
	if (type == DR_UC) {
	    /* clear high bytes */
	    if ((src1 == tmp_dest) || (src2 == tmp_dest)) {
		/* don't destroy source */
		tmp_dest = EAX;
	    }
	    x86_clear(c, tmp_dest);
	}
	break;
    case DR_S: case DR_US:
	if (type == DR_US) {
	    /* clear high bytes */
	    if ((src1 == tmp_dest) || (src2 == tmp_dest)) {
		/* don't destroy source */
		tmp_dest = EAX;
	    }
	    x86_clear(c, tmp_dest);
	}
	BYTE_OUT1(c, 0x66);
	break;
    case DR_F:
	if (smi->pending_prefix != 0) {
	    BYTE_OUT1(c, smi->pending_prefix);
	    smi->pending_prefix = 0;
	}
	BYTE_OUT3(c, 0xd9, ModRM(0x0, 0x0, 0x4), SIB(0, src1, src2));
	return;
	break;
    case DR_D:
	if (smi->pending_prefix != 0) {
	    BYTE_OUT1(c, smi->pending_prefix);
	    smi->pending_prefix = 0;
	}
	BYTE_OUT3(c, 0xdd, ModRM(0x0, 0x0, 0x4), SIB(0, src1, src2));
	return;
	break;
    case DR_L: case DR_UL: case DR_P:
	if (smi->stack_align == 4) {
	    type = DR_I;
	}
    }
    if (smi->pending_prefix != 0) {
        BYTE_OUT1(c, smi->pending_prefix);
	smi->pending_prefix = 0;
    }
    BYTE_OUT3(c, opcode, ModRM(0x0, tmp_dest, 0x4), SIB(0,src1,src2));
    switch(type){
    case DR_C:
	x86_lshi(c, dest, tmp_dest, 24);
	x86_rshi(c, dest, dest, 24);
	break;
    case DR_S:
	x86_lshi(c, dest, tmp_dest, 16);
	x86_rshi(c, dest, dest, 16);
	break;
    case DR_UC: case DR_US:
	if (dest != tmp_dest)
	    x86_movi(c, dest, tmp_dest);
	break;
    }
}

extern void
x86_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    x86_ploadi(c, type, junk, dest, src, offset);
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen bsload\n");
	break;
    case DR_L: case DR_UL: case DR_P: case DR_I: case DR_U:
	BYTE_OUT2(c, 0x0f, 0xc8 + dest);   /* byteswap dest */
	break;
    case DR_S: case DR_US:
	BYTE_OUT2(c, 0x86, (0xc0 | (dest << 3)) | (dest+4)); /* xchange dest */
	break;
    case DR_C: case DR_UC:
	break;
    }
}


extern void
x86_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    x86_pload(c, type, junk, dest, src1, src2);
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen bsload\n");
	break;
    case DR_L: case DR_UL: case DR_P: case DR_I: case DR_U:
	BYTE_OUT2(c, 0x0f, 0xc8 + dest);   /* byteswap dest */
	break;
    case DR_S: case DR_US:
	BYTE_OUT2(c, 0x86, (0xc0 | (dest << 3)) | (dest+4)); /* xchange dest */
	break;
    case DR_C: case DR_UC:
	break;
    }
}

static unsigned char st_opcodes[] = {
    0x88, /* DR_C */
    0x88, /* DR_UC */
    0x89, /* DR_S */
    0x89, /* DR_US */
    0x89, /* DR_I */
    0x89, /* DR_U */
    0x89, /* DR_L */
    0x89, /* DR_UL */
    0x89, /* DR_P */
    0xd9, /* DR_F */
    0xdd, /* DR_D */
    0x00, /* DR_V */
    0x00, /* DR_B */
    0x89, /* DR_EC */
};
extern void
x86_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    switch(type) {
    case DR_C:
    case DR_UC:
	if (dest >= ESP) {
	    /* movb doesn't work for big regs, move to eax */
	    x86_movi(c, EAX, dest);
	    dest = EAX;
	}
	break;
    }
    switch (type) {
    case DR_F:
    case DR_D:
	dest = 3;
	break;
    case DR_S: case DR_US:
	BYTE_OUT1(c, 0x66);
	break;
    default:
	break;
    }
    if (smi->pending_prefix != 0) {
        BYTE_OUT1(c, smi->pending_prefix);
	smi->pending_prefix = 0;
    }
    if (((long)offset <= 127) && ((long)offset > -128)) {
	BYTE_OUT3(c, st_opcodes[type], ModRM(0x1, dest, src), offset & 0xff);
    } else {
	BYTE_OUT2I(c, st_opcodes[type], ModRM(0x2, dest, src), offset);
    }
}

extern void
x86_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    switch(type) {
    case DR_C:
    case DR_UC:
	if (dest >= ESP) {
	    /* movb doesn't work for big regs, move to eax */
	    x86_movi(c, EAX, dest);
	    dest = EAX;
	}
	break;
    }
    switch (type) {
    case DR_F:
    case DR_D:
	dest = 3;
	break;
    case DR_S: case DR_US:
	BYTE_OUT1(c, 0x66);
	break;
    default:
	break;
    }
    if (smi->pending_prefix != 0) {
        BYTE_OUT1(c, smi->pending_prefix);
	smi->pending_prefix = 0;
    }
    BYTE_OUT3(c, st_opcodes[type], ModRM(0x0, dest, 0x4), SIB(0,src1,src2));
}

static long drisc_hidden_mod(long a, long b)
{ return a % b; }
static long drisc_hidden_umod(unsigned long a, unsigned long b)
{ return a % b; }

extern void x86_mod(drisc_ctx c, int data1, int data2, int dest, int src1, 
		      int src2)
{
    int return_reg;
    if (data1 == 1) {
	/* signed case */
	return_reg = dr_scalll(c, (void*)drisc_hidden_mod, "%l%l", src1, src2);
	dr_movl(c, dest, return_reg);
    } else {
	/* unsigned case */
	return_reg = dr_scalll(c, (void*)drisc_hidden_umod, "%l%l", src1, src2);
	dr_movul(c, dest, return_reg);
    }
}

extern void x86_modi(drisc_ctx c, int data1, int data2, int dest, int src1, 
		      long imm)
{
    x86_seti(c, _temp_reg, imm);
    x86_mod(c, data1, data2, dest, src1, _temp_reg);
}

extern void x86_div(drisc_ctx c, int op3, int op, int dest, int src1, 
		      int src2)
{
}

extern void x86_divi(drisc_ctx c, int op3, int op, int dest, int src, 
		      long imm)
{
}

extern void
x86_mov(drisc_ctx c, int type, int junk, int dest, int src)
{
    if (src == dest) return;
    switch(type) {
    case DR_D:
    case DR_F:
	break;
    default:
	BYTE_OUT2(c, MOV32, ModRM(0x3, src, dest));
    }
}

extern void x86_arith3(c, op, commut, dest, src1, src2)
drisc_ctx c;
int op;
int commut;
int dest;
int src1;
int src2;
{
    if (commut && (dest == src1)) {
	BYTE_OUT2(c, op, ModRM(0x3, dest, src2));
    } else if (commut && (dest == src2)) {
	BYTE_OUT2(c, op, ModRM(0x3, dest, src1));
    } else if (dest == src2) {
	assert(op == 0x2b);	/* must be subtract */
	BYTE_OUT2(c, 0xf7, ModRM(0x3, 0x3, dest));   /* neg src2/dest */
	BYTE_OUT2(c, 0x03, ModRM(0x3, dest, src1));  /* add src1, dest */
    } else {
	BYTE_OUT2(c, MOV32, ModRM(0x3, src1, dest));
	BYTE_OUT2(c, op, ModRM(0x3, dest, src2));
    }
}

extern void x86_arith2(c, op, subop, dest, src)
drisc_ctx c;
int op;
int subop;
int dest;
int src;
{
    if (op == 0) {
	int tmp_dest = dest;
	/* must be not */
	if (dest >= ESP) {
	    tmp_dest = EAX;
	}
	BYTE_OUT3(c, 0x83, ModRM(0x3, 0x7, src), 0);  /* cmp */
	x86_seti(c, tmp_dest, 0);
	BYTE_OUT3(c, 0x0f, 0x94, ModRM(0x3, src, tmp_dest));  /* sete dest */
	if (tmp_dest != dest) {
	    x86_movi(c, dest, tmp_dest);
	}	    
    } else {
	if (src != dest) {
	    BYTE_OUT2(c, MOV32, ModRM(0x3, src, dest));
	}
	BYTE_OUT2(c, op, ModRM(0x3, subop, dest));
    }
}

extern void x86_mul(c, sign, imm, dest, src1, src2)
drisc_ctx c;
int sign;
int imm;
int dest;
int src1;
int src2;
{
    /* make src1 be EAX */
    if (dest != EAX) {
	x86_push_reg(c, EAX);
    }
    if (dest != EDX) {
	x86_push_reg(c, EDX);
    }
	
    if ((src2 == EAX) && !imm){
	int tmp = src2;
	src1 = src2;
	src2 = tmp;
    }
    if (src1 != EAX) {
	x86_movi(c, EAX, src1);
    }
    if (imm == 0) {
	BYTE_OUT2(c, 0xf7, ModRM(0x3, sign ? 0x5 : 0x4, src2));
    } else {
	/* src2 is really immediate */
	if (sign) {
	    BYTE_OUT2I(c, 0x69, ModRM(0x3, 0, EAX), src2);
	} else {
	    x86_seti(c, EDX, src2);
	    BYTE_OUT2(c, 0xf7, ModRM(0x3, 0x4, EDX));
	}
    }
    if (dest != EDX) {
	x86_pop_reg(c, EDX);
    }
    if (dest != EAX) {
	x86_movi(c, dest, EAX);
	x86_pop_reg(c, EAX);
    }
}

extern void x86_div_modi(c, sign, div, dest, src1, imm)
drisc_ctx c;
int sign;
int div;
int dest;
int src1;
long imm;
{
    x86_push_reg(c, EBP);
    x86_seti(c, EBP, imm);
    x86_div_mod(c, sign, div, dest, src1, EBP);
    x86_pop_reg(c, EBP);
}

extern void x86_div_mod(c, sign, div, dest, src1, src2)
drisc_ctx c;
int sign;
int div;
int dest;
int src1;
int src2;
{
    int tmp_src2 = src2;

    /* make src1 be EAX */
    if (dest != EAX) {
	x86_push_reg(c, EAX);
    }
    if (dest != EDX) {
	x86_push_reg(c, EDX);
    }
	
    if (src1 != EAX) {
	x86_movi(c, EAX, src1);
    }
    if (src2 == EDX) {
	tmp_src2 = EBP;
	x86_push_reg(c, EBP);
	x86_movi(c, EBP, src2);
    }
    if (sign) {
	x86_rshai(c, EDX, EAX, 31);
    } else {
	x86_seti(c, EDX, 0);
    }
    BYTE_OUT2(c, 0xf7, ModRM(0x3, sign ? 0x7 : 0x6, tmp_src2));
    if (src2 == EDX) {
	x86_pop_reg(c, EBP);
    }
    if (div && (dest != EAX)) {
	x86_movi(c, dest, EAX);
    }
    if (!div && (dest != EDX)) {
	x86_movi(c, dest, EDX);
    }
    if (dest != EDX) {
	x86_pop_reg(c, EDX);
    }
    if (dest != EAX) {
	x86_pop_reg(c, EAX);
    }
	
}

static int group1_eax_op[] = {
    0x05 /* add */,
    0x0d /* or */,
    0x15 /* adc */,
    0x1d /* sbb */,
    0x25 /* and */,
    0x2d /* sub */,
    0x35 /* xor */,
    0x3d /* cmp */
};

extern void x86_arith3i(c, op, junk, dest, src, imm)
drisc_ctx c;
int op;
int junk;
int dest;
int src;
long imm;
{
    if (dest != src) {
	BYTE_OUT2(c, MOV32, ModRM(0x3, src, dest));
    }
    if ((imm <= 127) && (imm > -128)) {
	BYTE_OUT3(c, 0x83, ModRM(0x3, op, dest), imm & 0xff);
	return;
    }
    if (dest == EAX) {
	BYTE_OUT1I(c, group1_eax_op[op], imm);
    } else {
	BYTE_OUT2I(c, 0x81, ModRM(0x3, op, dest), imm);
    }
}

extern void x86_shift(c, op, junk, dest, src1, src2)
drisc_ctx c;
int op;
int junk;
int dest;
int src1;
int src2;
{
    int tmp_dest = dest;
    if ((dest == ECX) || (dest == src2)) {
	x86_push_reg(c, EAX);
	tmp_dest = EAX;
    }
    if (tmp_dest != src1) {
	x86_movi(c, tmp_dest, src1);
    }
    if (src2 != ECX) {
	x86_push_reg(c, ECX);
	x86_movi(c, ECX, src2);
    }
    BYTE_OUT2(c, 0xd3, ModRM(0x3, op, tmp_dest));
    if (src2 != ECX) {
	x86_pop_reg(c, ECX);
    }
    if ((dest == ECX) || (dest == src2)) {
	x86_movi(c, dest, tmp_dest);
	x86_pop_reg(c, EAX);
    }
}

extern void x86_shifti(c, op, junk, dest, src, imm)
drisc_ctx c;
int op;
int junk;
int dest;
int src;
long imm;
{
    if (dest != src) {
	x86_movi(c, dest, src);
    }
    BYTE_OUT3(c, 0xc1, ModRM(0x3, op, dest), imm & 0xff);
}

#define CONV(x,y) ((x*100)+y)
extern void
x86_convert(drisc_ctx c, int from_type, int to_type, 
	      int dest, int src)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;

    switch(CONV(from_type, to_type)) {
    case CONV(DR_I, DR_U):
    case CONV(DR_I,DR_UL):
    case CONV(DR_I, DR_L):
    case CONV(DR_U,DR_I):
    case CONV(DR_U,DR_UL):
    case CONV(DR_U,DR_L):
    case CONV(DR_UL,DR_I):
    case CONV(DR_UL,DR_U):
    case CONV(DR_UL,DR_L):
    case CONV(DR_L,DR_U):
    case CONV(DR_L,DR_UL):
    case CONV(DR_L,DR_I):
    case CONV(DR_P,DR_UL):
    case CONV(DR_UL,DR_P):
	if(src == dest) return;
	x86_movi(c, dest,src);
	break;
    case CONV(DR_D,DR_F):
    case CONV(DR_F,DR_D):
	break;
    case CONV(DR_F,DR_U):
    case CONV(DR_F,DR_UL):
    case CONV(DR_D,DR_U):
    case CONV(DR_D,DR_UL):
    case CONV(DR_F,DR_I):
    case CONV(DR_F,DR_L):
    case CONV(DR_D,DR_I):
    case CONV(DR_D,DR_L):
	/* use dest for float control word */
	/* fstcw (store control word) */
	BYTE_OUT3(c, 0xd9, ModRM(0x1, 0x7, _frame_reg), smi->fcu_word);
	x86_push_reg(c, EAX);
	/* movsw eax, fcu(ebx)   Load control word to reg */
	BYTE_OUT3I(c, 0x66, 0x8b, ModRM(0x2, EAX, _frame_reg), smi->fcu_word);
	/* modify control word OR it with 0x60 to set RC to 11 */
	BYTE_OUT1(c, 0x66);
	BYTE_OUT3(c, 0x0d,0x0, 0xc);
	/* movsw fcu(ebx), eax   store mod control word */
	BYTE_OUT3I(c, 0x66, 0x89, ModRM(0x2, EAX, _frame_reg), smi->fcu_word +2);
	/* fldcw (load modified control word) */
	BYTE_OUT3(c, 0xd9, ModRM(0x1, 0x5, _frame_reg), smi->fcu_word + 2);
	x86_pop_reg(c, EAX);
	/* fistpl (do the conversion) */
	switch(to_type){
	case DR_U: case DR_UL:
	    BYTE_OUT3(c, 0xdf, ModRM(0x1, 0x7, _frame_reg), smi->conversion_word);
	    break;
	case DR_I: case DR_L:
	    BYTE_OUT3(c, 0xdb, ModRM(0x1, 0x3, _frame_reg), smi->conversion_word);
	    break;
	}
	/* fldcw (restore original) */
	BYTE_OUT3(c, 0xd9, ModRM(0x1, 0x5, _frame_reg), smi->fcu_word);
	x86_ploadi(c, DR_I, 0, dest, _frame_reg, smi->conversion_word);
	break;
    case CONV(DR_I,DR_D):
    case CONV(DR_L,DR_D):
    case CONV(DR_I,DR_F):
    case CONV(DR_L,DR_F):
	x86_pstorei(c, DR_I, 0, src, _frame_reg, smi->conversion_word);
	BYTE_OUT3(c, 0xdb, ModRM(0x1, 0x0, _frame_reg), smi->conversion_word);
	break;
    case CONV(DR_U,DR_D):
    case CONV(DR_UL,DR_D):
    case CONV(DR_U,DR_F):
    case CONV(DR_UL,DR_F):
	x86_pstorei(c, DR_I, 0, src, _frame_reg, smi->conversion_word);
	BYTE_OUT3I(c, 0xc7, ModRM(0x1, 0x0, _frame_reg), 
		   smi->conversion_word + 4, 0);
	BYTE_OUT3(c, 0xdf, ModRM(0x1, 0x5, _frame_reg), smi->conversion_word);
	break;
    case CONV(DR_C,DR_I):
    case CONV(DR_C,DR_L):
    case CONV(DR_C,DR_U):
    case CONV(DR_C,DR_UL):
	/* signext24 - lsh24, rsha24 */
	x86_lshi(c, dest, src, 24);
	x86_rshai(c, dest, dest, 24);
	break;
    case CONV(DR_I, DR_C):
    case CONV(DR_U, DR_C):
    case CONV(DR_L, DR_C):
    case CONV(DR_UL, DR_C):
	x86_andi(c, dest, src, 0xff);
	break;
    case CONV(DR_S,DR_I):
    case CONV(DR_S,DR_L):
    case CONV(DR_S,DR_U):
    case CONV(DR_S,DR_UL):
	/* signext16 - lsh16, rsha16 */
	x86_lshi(c, dest, src, 16);
	x86_rshai(c, dest, dest, 16);
	break;
    case CONV(DR_US,DR_I):
    case CONV(DR_US,DR_L):
    case CONV(DR_US,DR_U):
    case CONV(DR_US,DR_UL):
    case CONV(DR_I, DR_S):
    case CONV(DR_U, DR_S):
    case CONV(DR_L, DR_S):
    case CONV(DR_UL, DR_S):
    case CONV(DR_I, DR_US):
    case CONV(DR_U, DR_US):
    case CONV(DR_L, DR_US):
    case CONV(DR_UL, DR_US):
	/* zero uppper 16 - lsh16, rsh16 */
	x86_lshi(c, dest, src, 16);
	x86_rshi(c, dest, dest, 16);
	break;
    default:
	printf("Unknown case in x86 convert %d\n", CONV(from_type,to_type));
    }
}

static unsigned char op_conds[] = {
    0x84, /* dr_beq_code */  /* signed */
    0x8d, /* dr_bge_code */
    0x8F, /* dr_bgt_code */
    0x8e, /* dr_ble_code */
    0x8c, /* dr_blt_code */
    0x85, /* dr_bne_code */

    0x84, /* dr_beq_code */  /* unsigned */
    0x83, /* dr_bge_code */
    0x87, /* dr_bgt_code */ 
    0x86, /* dr_ble_code */
    0x82, /* dr_blt_code */
    0x85, /* dr_bne_code */
};

static unsigned char fop_conds[] = {
    0x84, /* dr_beq_code */   /* z = 1*/
    0x86, /* dr_bge_code */
    0x82, /* dr_bgt_code */
    0x83, /* dr_ble_code */   /* c = 0 */
    0x87, /* dr_blt_code */   
    0x85, /* dr_bne_code */
};

extern void
x86_branch(drisc_ctx c, int op, int type, int src1, int src2, int label)
{
    switch(type) {
    case DR_F:
    case DR_D:
	BYTE_OUT2(c, 0xde, 0xd9);   /* fcompp */
	BYTE_OUT2(c, 0xdf, 0xe0);   /* fnstsw ax */
	BYTE_OUT1(c, 0x9e);	    /* sahf */
	dr_mark_branch_location(c, label);
	BYTE_OUT2I(c, 0x0f, fop_conds[op], 0);
	break;
    case DR_U:
    case DR_UL:
	op += 6; /* second set of codes */
	/* fall through */
    default:
	BYTE_OUT2(c, 0x39, ModRM(0x3, src2, src1));
	dr_mark_branch_location(c, label);
	BYTE_OUT2I(c, 0x0f, op_conds[op], 0);
    }
    x86_nop(c);
}

extern void 
x86_jump_to_label(drisc_ctx c, unsigned long label)
{
    dr_mark_branch_location(c, label);
    BYTE_OUT1I(c, 0xe9, 0);
}

extern void x86_jump_to_reg(drisc_ctx c, unsigned long reg)
{
    BYTE_OUT2(c, 0xff, ModRM(0x3, 0x4, reg));
}

extern void x86_jump_to_imm(drisc_ctx c, unsigned long imm)
{
    x86_seti(c, EAX, imm);
    BYTE_OUT2(c, 0xff, ModRM(0x3, 0x4, EAX));
}

extern void 
x86_jal(drisc_ctx c, int return_addr_reg, int target)
{
/* jump, source addr to return_addr_reg */
}

extern void 
x86_special(drisc_ctx c, special_operations type, long param)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;

    switch (type) {
    case DR_NOP:
	break;
    case DR_SEGMENTED_FOLLOWS:
	switch(param) {
	case DR_X86_CS_PREFIX:
	    smi->pending_prefix = 0x2e;
	    break;
	case DR_X86_SS_PREFIX:
	    smi->pending_prefix = 0x36;
	    break;
	case DR_X86_DS_PREFIX:
	    smi->pending_prefix = 0x3e;
	    break;
	case DR_X86_ES_PREFIX:
	    smi->pending_prefix = 0x26;
	    break;
	case DR_X86_FS_PREFIX:
	    smi->pending_prefix = 0x64;
	    break;
	case DR_X86_GS_PREFIX:
	    smi->pending_prefix = 0x65;
	    break;
	default:
	    fprintf(stderr, "Unknown x86 segment prefix!\n");
	}
	break;
    }
}

static void internal_push(drisc_ctx c, int type, int immediate, 
			  void *value_ptr)
{
    if (!immediate) {
	x86_push_reg(c, *(int*)value_ptr);
    } else {
	BYTE_OUT1I(c, 0x68, *(int*)value_ptr);
    }
}

static void push_init(drisc_ctx c)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    smi->cur_arg_offset = 0;
}

extern void x86_push(drisc_ctx c, int type, int reg)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    if ((type == DR_V) && (reg == -1)) {
	push_init(c);
    } else if ((type == DR_F) || (type == DR_D)) {
	x86_pstorei(c, DR_D, 0, reg, _frame_reg, smi->conversion_word);
	BYTE_OUT3(c, 0xff, ModRM(0x1, 0x6, _frame_reg), smi->conversion_word+4);
	BYTE_OUT3(c, 0xff, ModRM(0x1, 0x6, _frame_reg), smi->conversion_word);
	smi->cur_arg_offset += 8;
    } else {
	internal_push(c, type, 0, &reg);
	smi->cur_arg_offset += 4;
    }
}

extern void x86_pushi(drisc_ctx c, int type, long value)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    internal_push(c, type, 1, &value);
    smi->cur_arg_offset += 4;
}

extern void x86_pushfi(drisc_ctx c, int type, double value)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    internal_push(c, type, 1, &value);
    smi->cur_arg_offset += 8;
}

extern int x86_calli(drisc_ctx c, int type, void *xfer_address)
{
    int caller_side_ret_reg = EAX;
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;

    /* save temporary registers */
    dr_mark_call_location(c, NULL, xfer_address);
    BYTE_OUT1I(c, 0xe8, 0);
    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
/*	caller_side_ret_reg = _f0;*/
    }
    dr_addii(c, ESP, ESP, smi->cur_arg_offset);
    return caller_side_ret_reg;
}

extern int x86_callr(drisc_ctx c, int type, int src)
{
    int caller_side_ret_reg = EAX;
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;

    /* save temporary registers */
    /* call through reg */
    BYTE_OUT2(c, 0xff, ModRM(0x3, 0x2, src));
    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
/*	caller_side_ret_reg = _f0;*/
    }
    dr_addii(c, ESP, ESP, smi->cur_arg_offset);
    return caller_side_ret_reg;
}

extern void
x86_branchi(drisc_ctx c, int op, int type, int src, long imm, int label)
{
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen\n");
	break;
    case DR_U:
    case DR_UL:
/*
	switch(op) {
	case dr_bge_code: {
	    imm = imm-1;
	    op = dr_bgt_code;
	    break;
	}
	case dr_blt_code: {
	    imm = imm-1;
	    op = dr_ble_code;
	    break;
	}
	}
*/
	op += 6; /* second set of codes */
	/* fall through */
    default:
/*	BYTE_OUT2(c, 0x39, ModRM(0x3, src2, src1));*/
	BYTE_OUT2I(c, 0x81, ModRM(0x3, 0x7, src), imm);  /* cmp */
	dr_mark_branch_location(c, label);
	BYTE_OUT2I(c, 0x0f, op_conds[op], 0);
    }
}

extern void x86_ret(drisc_ctx c, int data1, int data2, int src)
{
    switch (data1) {
    case DR_C:
    case DR_UC:
    case DR_S:
    case DR_US:
    case DR_I:
    case DR_U:
    case DR_L:
    case DR_UL:
    case DR_P:
	if (src != EAX) x86_movi(c, EAX, src);
	break;
/*    case DR_F:
	if (src != _f0) x86_movf(c, _f0, src);
    case DR_D:
	if (src != _f0) x86_movd(c, _f0, src);
*/
    }
    x86_simple_ret(c);
}

extern void x86_reti(drisc_ctx c, int data1, int data2, long imm)
{
    switch (data1) {
    case DR_C:
    case DR_UC:
    case DR_S:
    case DR_US:
    case DR_I:
    case DR_U:
    case DR_L:
    case DR_UL:
    case DR_P:
	x86_seti(c, EAX, imm);
	break;
/*    case DR_F:
	x86_setf(c, _f0, imm);
    case DR_D:
    x86_setd(c, _f0, imm);*/
    }
    x86_simple_ret(c);
}

static void
x86_data_link(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    int i;
    for (i=0; i < t->data_mark_count; i++) {
	int label = t->data_marks[i].label;
	void *label_addr = t->label_locs[label] + (char*)c->p->code_base;
	*t->data_marks[i].addr = (long) label_addr;
    }
}

static void
x86_branch_link(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    int i;

    for(i=0; i< t->branch_count; i++) {
	int label = t->branch_locs[i].label;
	int label_offset = t->label_locs[label] - t->branch_locs[i].loc;
	char *branch_addr = (char*)((char *)c->p->code_base + 
				  t->branch_locs[i].loc);
	int offset;
	if (*branch_addr == 0x0f) {
	    branch_addr +=2; /* conditional */
	    offset = 6;
	} else {
	    branch_addr++; /* unconditional */
	    offset = 5;
	}
	*(int*)branch_addr = label_offset - offset;
    }
}

static void
x86_call_link(drisc_ctx c)
{
    call_t *t = &c->p->call_table;
    int i;

    for(i=0; i< t->call_count; i++) {
	int *call_addr = (int*) ((unsigned long)c->p->code_base + 
				 t->call_locs[i].loc + 1);
	int call_offset = (unsigned long)t->call_locs[i].xfer_addr - 
	    (unsigned long)((char*)call_addr + 4);  /* add len of call insn */

	*call_addr = call_offset;
    }
}

static void
x86_emit_save(drisc_ctx c)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    void *save_ip = c->p->cur_ip;
    int ar_size = smi->act_rec_size;
    ar_size = roundup(ar_size, 8) + 16;

    c->p->cur_ip = (char*)c->p->code_base + smi->backpatch_offset;

    /* do local space reservation */
    dr_subii(c, ESP, ESP, ar_size);

    c->p->fp = (char*)c->p->code_base;
    c->p->cur_ip = save_ip;
}
    
extern void
x86_end(c)
drisc_ctx c;
{
    x86_simple_ret(c);
    x86_branch_link(c);
    x86_call_link(c);
    x86_data_link(c);
    x86_emit_save(c);
}

extern void *
x86_clone_code(c, new_base, available_size)
drisc_ctx c;
void *new_base;
int available_size;
{
    int size = dr_code_size(c);
    void *old_base = c->p->code_base;
    void *native_base = c->p->code_base;
    if (available_size < size) {
	return NULL;
    }
    if (native_base == NULL) native_base = c->p->native.code_base;
    memcpy(new_base, native_base, size);
    c->p->code_base = new_base;
    c->p->cur_ip = new_base + size;
    c->p->fp = new_base;
    x86_branch_link(c);
    x86_call_link(c);
    x86_data_link(c);
    c->p->code_base = old_base;
    c->p->cur_ip = old_base + size;
    c->p->fp = old_base;
    return new_base;
}

extern void
x86_pset(drisc_ctx c, int type, int junk, int dest, long imm)
{
    x86_seti(c, dest, imm);
}	

extern void
x86_setf(drisc_ctx c, int type, int junk, int dest, double imm)
{
    x86_mach_info smi = (x86_mach_info) c->p->mach_info;
    union {
	float f;
	int i;
    } a;
    union {
	double d;
	int i[2];
    } b;
    if (type == DR_F) {
	a.f = (float) imm;
	BYTE_OUT3I(c, 0xc7, ModRM(0x1, 0x0, _frame_reg), 
		   smi->conversion_word, a.i);
	/* flds */
	BYTE_OUT3(c, 0xd9, ModRM(0x1, 0x0, _frame_reg), smi->conversion_word);
    } else {
	b.d = imm;
	BYTE_OUT3I(c, 0xc7, ModRM(0x1, 0x0, _frame_reg), 
		   smi->conversion_word, b.i[0]);
	BYTE_OUT3I(c, 0xc7, ModRM(0x1, 0x0, _frame_reg), 
		   smi->conversion_word + 4, b.i[1]);
	/* fldd */
	BYTE_OUT3(c, 0xdd, ModRM(0x1, 0x0, _frame_reg), smi->conversion_word);
    }
}	


#define bit_R(x) (1<<x)

extern void
x86_reg_init(drisc_ctx c)
{
    c->p->var_i.init_avail[0] = (bit_R(EBX)|bit_R(ESI)|bit_R(EDI));
    c->p->var_i.members[0] = c->p->var_i.init_avail[0];
    c->p->tmp_i.init_avail[0] = (bit_R(EDX)|bit_R(ECX));
    c->p->tmp_i.members[0] = c->p->tmp_i.init_avail[0] | bit_R(EAX);
    c->p->var_f.init_avail[0] = 0;
    c->p->var_f.members[0] = c->p->var_f.init_avail[0];
    c->p->tmp_f.init_avail[0] = 0;
    c->p->tmp_f.members[0] = c->p->tmp_f.init_avail[0];
}

extern void*
gen_x86_mach_info(c)
drisc_ctx c;
{
    x86_mach_info smi = malloc(sizeof(*smi));
    if (c->p->mach_info != NULL) {
	free(c->p->mach_info);
	c->p->mach_info = NULL;
	c->p->native.mach_info = NULL;
    }
    x86_reg_init(c);
    smi->act_rec_size = 0;
    smi->stack_align = 4; /* 8 for x86v9 */
    smi->stack_constant_offset = 0; /* 2047 for x86v9 */
    smi->conversion_word = 0;
    smi->fcu_word = 0;
    smi->save_base = 0;
    smi->backpatch_offset = 0;
    return smi;
}
#if defined(HAVE_DIS_ASM_H) && !defined(NO_DISASSEMBLER)
/* GENERIC BINUTILS DISASSEMBLER */
#include "dis-asm.h"

#define MAXLENGTH (1<<23) /* Max length of function that can be disassembled */

#ifdef LINUX_KERNEL_MODULE
extern int
kfprintf(FILE *file, const char *fmt, ...)
{
    static char printk_buf[1024];
    int val;
    va_list ap;
    va_start(ap, fmt);
    val = vsnprintf(printk_buf, sizeof(printk_buf), fmt, ap);
    printk("%s", printk_buf);
    va_end(ap);
    return val;
}

#undef  stdout
#define stdout (FILE*)0
#define FPRINTF_FUNCTION kfprintf
#else
#define FPRINTF_FUNCTION fprintf
#endif
extern int
x86_init_disassembly_info(drisc_ctx c, void * ptr)
{
    struct disassemble_info *i = ptr;
#ifdef INIT_DISASSEMBLE_INFO_THREE_ARG
    INIT_DISASSEMBLE_INFO(*i, stdout, FPRINTF_FUNCTION);
    i->endian = BFD_ENDIAN_LITTLE;
#else
    INIT_DISASSEMBLE_INFO(*i, stdout);
#endif
    i->mach = bfd_mach_i386_i386;
    if (c->p->code_base != NULL) {
	i->buffer = (bfd_byte *)c->p->code_base;
	i->buffer_vma = (bfd_vma)c->p->code_base;
    } else {
	i->buffer = (bfd_byte *)c->p->native.code_base;
	i->buffer_vma = (bfd_vma)c->p->native.code_base;
    }
    i->buffer_length = MAXLENGTH;
#ifdef HAVE_PRINT_INSN_I386
    return 1;
#else
    return 0;
#endif
}

extern int
x86_print_insn(drisc_ctx c, void *info_ptr, void *insn)
{
#ifdef HAVE_PRINT_INSN_I386
    return print_insn_i386((bfd_vma)insn, info_ptr);
#else
    return 0;
#endif
}

extern void null_func(){}
extern int
x86_count_insn(drisc_ctx c, int start, int end)
{
#ifdef HAVE_PRINT_INSN_I386
    struct disassemble_info i;
    int count;
    char *insn_ptr;
#ifdef INIT_DISASSEMBLE_INFO_THREE_ARG
    INIT_DISASSEMBLE_INFO(i, stdout, (fprintf_ftype) null_func);
    i.endian = BFD_ENDIAN_LITTLE;
#else
    INIT_DISASSEMBLE_INFO(i, stdout);
#endif
    i.mach = bfd_mach_i386_i386;
    if (c->p->code_base != NULL) {
	i.buffer = (bfd_byte *)c->p->code_base;
	i.buffer_vma = (bfd_vma)c->p->code_base;
    } else {
	i.buffer = (bfd_byte *)c->p->native.code_base;
	i.buffer_vma = (bfd_vma)c->p->native.code_base;
    }
    i.buffer_length = MAXLENGTH;
    count = 0;
    insn_ptr = i.buffer + start;
    while((long)insn_ptr < (long)i.buffer + end) {
	insn_ptr += print_insn_i386((bfd_vma)insn_ptr, &i);
	count++;
    }
    return count;
#else
   /* no print insn, just return the buffer length */
    return end - start;
#endif
}
#else
extern int
x86_count_insn(drisc_ctx c, int start, int end)
{   /* no print insn, just return the buffer length */
    return end - start;
}
extern int
x86_init_disassembly_info(drisc_ctx c, void * ptr){return 0;}
extern int x86_print_insn(drisc_ctx c, void *info_ptr, void *insn){return 0;}
#endif

char *char_regs[] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
char *short_regs[] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
char *int_regs[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

extern void
x86_print_reg(drisc_ctx c, int typ, int reg)
{
    switch(typ) {
    case DR_C: case DR_UC:
	if ((reg > 0) && (reg < sizeof(char_regs)/sizeof(char_regs[0]))) {
	    printf("%s", char_regs[reg]);
	    return;
	}
	break;
    case DR_S: case DR_US:
	if ((reg > 0) && (reg < sizeof(short_regs)/sizeof(short_regs[0]))) {
	    printf("%s", short_regs[reg]);
	    return;
	}
	break;
/*    case DR_C: case DR_UC: case DR_S: case DR_US:*/
    case DR_I: case DR_U: case DR_L: case DR_UL:
	if ((reg > 0) && (reg < sizeof(int_regs)/sizeof(int_regs[0]))) {
	    printf("%s", int_regs[reg]);
	    return;
	}
	break;
    case DR_F: case DR_D:
	printf("Fstack");
	return;
    }
    printf("NoReg(%d)", reg);
}
	
