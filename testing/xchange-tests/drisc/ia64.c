#include "config.h"
#include <ctype.h>

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
#include <malloc.h>
#include <string.h>
#else
#include "kdrisc.h"
#define fprintf(fmt, args...)	printk(args)
#define printf	printk
#define malloc (void *)DAllocMM
#define free(a) DFreeMM((addrs_t) a)
#endif

#include "drisc.h"
#include "drisc_internal.h"
#include "ia64.h"

#define MOV32 0x89
#define Mod(x) ((x)<<6)
#define RegOp(x) ((x)<<3)
#define RM(x) (x)
#define ModRM(mod,reg,rm) (Mod(mod)|RegOp(reg)|RM(rm))
#define SIB(scale, index, base) (((scale)<<6) | ((index)<<3) | (base))

#define nop_m 0x00008000000
#define nop_i 0x00008000000
#define FORMAT_A1(x2a, ve, x4, x2b, r3, r2, r1, qp) (((long)0x8)<<37| ((long)x2a) << 34 | ((long)(ve) << 33) | (x4) << 29 |((long)(x2b))<<27| (r3) << 20 | (r2) << 13 | (r1) << 6 | qp)

#define FORMAT_A3(s, x2a, ve, x4, x2b, r3, imm7b, r1, qp) (((long)0x8)<<37| ((long)(s))<<36 |((long)x2a) << 34 | ((long)(ve) << 33) | ((long)(x4)) << 29 | ((long)(x2b)) << 27 | (r3) << 20 | (imm7b) << 13 | (r1) << 6 | qp)

#define FORMAT_A4(s, x2a, ve, imm6d, r3, imm7b, r1, qp) (((long)0x8)<<37| ((long)(s))<<36 |((long)x2a) << 34 | ((long)(ve) << 33) | ((long)(imm6d)) << 27 | (r3) << 20 | (imm7b) << 13 | (r1) << 6 | qp)

#define FORMAT_A5(s, imm9d, imm5c, r3, imm7b, r1, qp) (((long)0x9)<<37| ((long)(s))<<36 | ((long)(imm9d))<<27 | ((long)(imm5c))<<22 | ((r3)&3) << 20 | (imm7b) << 13 | (r1) << 6 | qp)

#define FORMAT_A6(op, tb, x2, ta, p2, r3, r2, c, p1, qp) (((long)op)<<37| ((long)(tb))<<36| ((long)(x2))<<34| ((long)(ta))<<33| ((long)(p2))<<27 | ((r3)) << 20 | ((long)(r2))<<13| (c) << 12 | (p1) << 6 | qp)

#define FORMAT_I3(za, x2a, zb, ve, x2c, x2b, mbt4c, r2, r1, qp) (((long)7) << 37 | ((long)(za))<<36 | ((long)(x2a))<< 34 | ((long)zb)<<33 | ((long)ve) << 22 |((long)x2c) << 30 | ((long)x2b)<< 28 | ((long)(mbt4c)) << 20 |((long)r2) << 13 | ((long)(r1)) << 6 |  qp)
#define FORMAT_I5(za, x2a, zb, ve, x2c, x2b, r3, r2, r1, qp) (((long)7) << 37 | ((long)(za))<<36 | ((long)(x2a))<< 34 | ((long)zb)<<33 | ((long)ve) << 22 |((long)x2c) << 30 | ((long)x2b)<< 28 | ((long)(r3)) << 20 |((long)r2) << 13 | ((long)(r1)) << 6 |  qp)
#define FORMAT_I7(za, x2a, zb, ve, x2c, x2b, r3, r2, r1, qp) (((long)7) << 37 | ((long)(za))<<36 | ((long)(x2a))<< 34 | ((long)zb)<<33 | ((long)ve) << 32 |((long)x2c) << 30 | ((long)x2b)<< 28 | ((long)(r3)) << 20 |((long)r2) << 13 | ((long)(r1)) << 6 |  qp)
#define FORMAT_I11(x2, x, len, r3, pos6b, y, r1, qp) (((long)5) << 37 | ((long)(x2))<< 34 | ((long)x)<<33 | ((long)len) << 27 |((long)y) << 13 | ((long)pos6b)<< 14 | ((long)r3) << 20 | ((long)(r1)) << 6 |  qp)
#define FORMAT_I12(x2, x, len, y, cpos6c, r2, r1, qp) (((long)5) << 37 | ((long)(x2))<< 34 | ((long)x)<<33 | ((long)len) << 27 |((long)y) << 26 | ((long)cpos6c)<< 20 | ((long)r2) << 13 | ((long)(r1)) << 6 |  qp)

#define FORMAT_I21(x3, timm9c, ih, x, wh, b2, r1, qp) (((long)0) << 37 | ((long)(x3))<<33 | ((long)(timm9c))<< 24 | ((long)ih)<<23 | ((long)x) << 22 | ((long)wh)<< 20 | ((long)(b2)) << 13 | ((long)(r1)) << 6 |  qp)

#define FORMAT_I22(x3, x6, b2, r1, qp) (((long)0) << 37 | ((long)(x3))<<33 | ((long)(x6))<< 27 | ((long)(b2)) << 13 | ((long)(r1)) << 6 |  qp)

#define FORMAT_I26(x3, x6, ar3, r2, qp) (((long)0) << 37 | ((long)(x3))<<33 | ((long)(x6))<< 27 | ((long)(ar3)) << 20 | ((long)(r2)) << 13 |  qp)
#define FORMAT_I29(x3, x6, r3, r1, qp) (((long)0) << 37 | ((long)(x3))<<33 | ((long)(x6))<< 27 | ((long)(r3)) << 20 | ((long)(r1)) << 6 |  qp)

#define FORMAT_B1(s, d, wh, imm20b, p, btype, qp) (((long)(4))<<37 |((long)(s))<<36 |((long)(d))<<35 | ((long)(wh)&3) << 33 | ((long)(imm20b)) << 13 | (p)<< 12 | (btype) << 6 | qp)
#define FORMAT_B4(d, wh, x6, b2, p, btype, qp) (((long)(d))<<35 | ((long)(wh)&3) << 33 | ((long)(x6)) << 27 | ((long)(b2)) << 13 | (p)<< 12 | (btype) << 6 | qp)
#define FORMAT_B5(d, wh, b2, p, b1, qp) (((long)1) << 37 | ((long)(d))<<35 | ((long)(wh)&3) << 32 | ((long)(b2)) << 13 | (p)<< 12 | (b1) << 6 | qp)

#define FORMAT_X2(i, imm9d, imm5c, ic, vc, imm7b, r1, qp) (((long)6) << 37 | ((long)(i))<<36 | ((long)(imm9d)) << 27 | ((long)(imm5c)) << 22|((long)(ic))<< 21 | ((long)(vc)) << 20 | ((long)(imm7b)<<13) | ((long) (r1) << 6) |  qp)

#define FORMAT_F1(op, x, sf, f4, f3, f2, f1, qp) (((long)op) << 37 | ((long)(x))<<36 | ((long)(sf)) << 34 | ((long)(f4)) << 27|((long)(f3))<< 20 | ((long)(f2)) << 13 | ((long)(f1)<<6) |  qp)

#define FORMAT_F2(x, x2, f4, f3, f2, f1, qp) (((long)0xe) << 37 | ((long)(x))<<36 | ((long)(x2)) << 34 | ((long)(f4)) << 27|((long)(f3))<< 20 | ((long)(f2)) << 13 | ((long)(f1)<<6) |  qp)

#define FORMAT_F4(rb, sf, ra, p2, f3, f2, ta, p1, qp) (((long)4)<<37| ((long)(rb))<<36| ((long)(sf))<<34| ((long)(ra))<<33| ((long)(p2))<<27 | ((f3)) << 20 | ((long)(f2))<<13| (ta) << 12 | (p1) << 6 | qp)

#define FORMAT_F9(op, x, x6, f3, f2, f1, qp) (((long)op) << 37 | ((long)(x))<<33 | ((long)(x6)) << 27 |((long)(f3))<< 20 | ((long)(f2)) << 13 | ((long)(f1)<<6) |  qp)
#define FORMAT_F10(op, sf, x, x6, f2, f1, qp) (((long)op) << 37 | ((long)sf)<<34 | ((long)(x))<<33 | ((long)(x6)) << 27 | ((long)(f2)) << 13 | ((long)(f1)<<6) |  qp)
#define FORMAT_F11(x, x6, f2, f1, qp) (((long)0) << 37 | ((long)(x))<<33 | ((long)(x6)) << 27 | ((long)(f2)) << 13 | ((long)(f1)<<6) |  qp)

#define FORMAT_M1(m, x6, hint, x, r3, r1, qp) (((long)4) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(x))<< 27 | ((long)(r3)) << 20 | ((long)(r1)<<6) |  qp)
#define FORMAT_M2(m, x6, hint, x, r3, r2, r1, qp) (((long)4) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(x))<< 27 | ((long)(r3)) << 20 | ((long)(r2)) << 13 | ((long)(r1)<<6) |  qp)
#define FORMAT_M3(s, x6, hint, i, r3, imm7b, r1, qp) (((long)5) << 37 | ((long)(s))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(i))<< 27 | ((long)(r3)) << 20 | ((long)(imm7b)) << 13 | ((long)(r1)<<6) |  qp)
#define FORMAT_M4(m, x6, hint, x, r3, r2, qp) (((long)4) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(x))<< 27 | ((long)(r3)) << 20 | ((long)(r2)<<13) |  qp)
#define FORMAT_M5(s, x6, hint, i, r3, r2, imm7a, qp) (((long)5) << 37 | ((long)(s))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(i))<< 27 | ((long)(r3)) << 20 | ((long)(r2)) << 13 | ((long)(imm7a)<<6) |  qp)
#define FORMAT_M6(m, x6, hint, x, r3, f1, qp) (((long)6) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(x))<< 27 | ((long)(r3)) << 20 | ((long)(f1)<<6) |  qp)
#define FORMAT_M8(s, x6, hint, i, r3, imm7b, f1, qp) (((long)7) << 37 | ((long)(s))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(i))<< 27 | ((long)(r3)) << 20 | ((long)(imm7b)) << 13 | ((long)(f1)<<6) |  qp)
#define FORMAT_M9(m, x6, hint, x, r3, f2, qp) (((long)6) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(x))<< 27 | ((long)(r3)) << 20 | ((long)(f2)<<13) |  qp)
#define FORMAT_M10(s, x6, hint, i, r3, f2, imm7a, qp) (((long)7) << 37 | ((long)(s))<<36 | ((long)(x6)) << 30 |((long)(hint))<< 28 |((long)(i))<< 27 | ((long)(r3)) << 20 | ((long)(f2)) << 13 | ((long)(imm7a)<<6) |  qp)
#define FORMAT_M18(m, x6, x, r2, f1, qp) (((long)6) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(x))<< 27 | ((long)(r2)) << 13 | ((long)(f1)<<6) |  qp)
#define FORMAT_M19(m, x6, x, f2, r1, qp) (((long)4) << 37 | ((long)(m))<<36 | ((long)(x6)) << 30 |((long)(x))<< 27 | ((long)(f2)) << 13 | ((long)(r1)<<6) |  qp)
#define FORMAT_M29(x3, x6, ar3, r2, qp) (((long)1) << 37 | ((long)(x3))<<33 | ((long)(x6))<< 27 | ((long)(ar3)) << 20 | ((long)(r2)) << 13 |  qp)
#define FORMAT_M34(x3, sor, sol, sof, r1, qp) (((long)1) << 37 | ((long)(x3))<<33 | ((long)(sor))<< 27 | ((long)(sol)) << 20 | ((long)(sof)) << 13 | ((long)(r1)<<6) |  qp)


#define A_MOVR(src, dest) FORMAT_A1(0, 0, 0, 0, R0, src, dest, 0)
#define B_RET(reg) FORMAT_B4(/*d*/0, 0, 0x21, /*b2*/reg, 1, 4, 0)
#define I_RESTORE_PFS(reg) FORMAT_I26(0, 0x2a, /*ar.pfs*/0x40, reg, 0)
#define I_MOVtoB(src, dest) FORMAT_I21(0x7, /*timm9c*/0, /*ih*/0, 0, /*wh*/1, src, dest, 0)
#define I_MOVfromB(src, dest) FORMAT_I22(0x0, 0x31, src, dest, 0)
#define I_Sext8(src, dest) FORMAT_I29(0x0, 0x14, src, dest, 0)
#define I_Sext16(src, dest) FORMAT_I29(0x0, 0x15, src, dest, 0)
#define I_Sext32(src, dest) FORMAT_I29(0x0, 0x16, src, dest, 0)
#define I_Zext8(src, dest) FORMAT_I29(0x0, 0x10, src, dest, 0)
#define I_Zext16(src, dest) FORMAT_I29(0x0, 0x11, src, dest, 0)
#define I_Zext32(src, dest) FORMAT_I29(0x0, 0x12, src, dest, 0)
#define I_SHL(dest, src1, src2) FORMAT_I7(1, 0, 1, 0, 1, 0, src2, src1, dest, 0)
#define I_SHRi(dest, src1, src2) FORMAT_I5(1, 0, 1, 0, 0, 2, src1, src2, dest, 0)
#define I_SHRu(dest, src1, src2) FORMAT_I5(1, 0, 1, 0, 0, 0, src1, src2, dest, 0)
#define I_DEPz(dest, src, pos, len) FORMAT_I12(1, 1, len, 0, pos-1, src, dest, 0)
#define I_EXTR(dest, src, pos, len) FORMAT_I11(1, 0, len, src, pos, 1, dest, 0)
#define I_EXTRu(dest, src, pos, len) FORMAT_I11(1, 0, len, src, pos, 0, dest, 0)
#define I_SHLi(dest, src, imm) I_DEPz(dest, src, 64-imm, 64-imm)
#define I_SHRii(dest, src, imm) I_EXTR(dest, src, imm, 63-imm)
#define I_SHRui(dest, src, imm) I_EXTRu(dest, src, imm, 63-imm)
#define MIIs(c, m1, i1, i2) TEMPLATE_OUT3(c, 0x1, m1, i1, i2)
#define MIsIs(c, m1, i1, i2) TEMPLATE_OUT3(c, 0x3, m1, i1, i2)
#define MLXs(c, m1, l1, x1) TEMPLATE_OUT3(c, 0x5, m1, l1, x1)
#define MMIs(c, m1, i1, i2) TEMPLATE_OUT3(c, 0x9, m1, i1, i2)
#define MsMIs(c, m1, i1, i2) TEMPLATE_OUT3(c, 0xb, m1, i1, i2)
#define MFIs(c, m1, f1, i1) TEMPLATE_OUT3(c, 0xd, m1, f1, i1)
#define MIBs(c, m1, i1, i2) TEMPLATE_OUT3(c, 0x11, m1, i1, i2)
#define BBBs(c, b1, b2, b3) TEMPLATE_OUT3(c, 0x17, b1, b2, b3)

static void
TEMPLATE_OUT3(drisc_ctx c, int tmpl, long op0, long op1, long op2)
{
    if (c->p->cur_ip >= c->p->code_limit) {
	expand_drisc_buffer(c);
    }
    *(((unsigned long*)c->p->cur_ip)) = (unsigned long)((((unsigned long)(op1)) & ((unsigned long)0x3ffff)) << 46 | (((long)(op0)) << 5) | (tmpl));
    *(((unsigned long*)c->p->cur_ip + 1)) = (unsigned long)(((long)(op2)) << 23 | (((op1) >> 18) & 0x7fffff));
    if (c->drisc_debug) dump_cur_drisc_insn(c);
    c->p->cur_ip = ((char*)c->p->cur_ip)+16;
}

#define ia64_savei(c, imm) 
#define ia64_movl(c, dest, src) 	do { if (src != dest) \
    MIIs(c, FORMAT_A1(0, 0, 0, 0, R0, src, dest, 0), nop_i, nop_i);\
} while (0)


#define ia64_addli(c, dest, src, imm) ia64_arith3i(c, 0, 0, dest, src, imm)
#define ia64_addl(c, dest, src1, src2) ia64_arith3(c, 0, 0, dest, src1, src2)
#define ia64_subli(c, dest, src, imm) ia64_arith3i(c, 9, 1, dest, src, imm)


extern void ia64_setl(drisc_ctx c, int r, long val);

#define IREG 0
#define FREG 1

#define _temp_reg R14
#define _frame_reg L0
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

int ia64_type_align[] = {
        1, /* C */
        1, /* UC */
        2, /* S */
        2, /* US */
        4, /* I */
        4, /* U */
        sizeof(unsigned long), /* UL */
        sizeof(long), /* L */
        sizeof(char*), /* P */
        sizeof(float), /* F */
        sizeof(double), /* D */
        4, /* V */
        8, /* B */
        sizeof(char*), /* EC */
};

int ia64_type_size[] = {
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
        8, /* B */
        sizeof(char*), /* EC */
};

static void ia64_spill_fill(drisc_ctx c, int spill);

extern int
ia64_local(drisc_ctx c, int type)
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;

    smi->act_rec_size += roundup(type_info[type].size, smi->stack_align);
    return (-smi->act_rec_size) + smi->stack_constant_offset;
}

extern int
ia64_localb(drisc_ctx c, int size)
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    smi->act_rec_size = roundup(smi->act_rec_size, size);

    smi->act_rec_size += roundup(size, smi->stack_align);
    return (-smi->act_rec_size) + smi->stack_constant_offset;
}

extern int ia64_local_op(drisc_ctx c, int flag, int val)
{
    int size = val;
    if (flag == 0) {
	size = type_info[val].size;
    }
    return ia64_localb(c, size);
}	

extern void
ia64_save_restore_op(drisc_ctx c, int save_restore, int type, int reg)
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    if (save_restore == 0) { /* save */
	switch (type) {
	case DR_D: case DR_F:
	    if ((reg >= F6) && (reg <= F7)) {
		ia64_pstorei(c, type, 0, reg, _frame_reg, smi->save_base + 8*4/*iregs*/ + reg * smi->stack_align + smi->stack_constant_offset);
	    }		
	    break;
	default:
	    if (((reg >= IN0) && (reg <= L7)) || 
		((reg >= R4) && (reg <= R7))) {
		/* reg is automatically preserved if it's IN or local */
		return;
	    }
	    ia64_pstorei(c, type, 0, reg, _frame_reg, smi->save_base + (reg) * smi->stack_align + smi->stack_constant_offset);
	    break;
	}
    } else {  /* restore */
	switch (type) {
	case DR_D: case DR_F:
	    if ((reg >= F6) && (reg <= F7)) {
		ia64_ploadi(c, type, 0, reg, _frame_reg, smi->save_base + 8*4/*iregs*/ + reg * smi->stack_align + smi->stack_constant_offset);
	    }
	    break;
	default:
	    if ((reg >= IN0) && (reg <= L7)) {
		/* reg is automatically preserved if it's IN or local */
		return;
	    }
	    ia64_ploadi(c, type, 0, reg, _frame_reg, smi->save_base + reg * smi->stack_align + smi->stack_constant_offset);
	    break;
	}
    }
}	


static void
ia64_simple_ret(drisc_ctx c) 
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    ia64_spill_fill(c, 0);
    ia64_ploadi(c, DR_L, 0, smi->branch_reg, _frame_reg, 0);
    ia64_ploadi(c, DR_L, 0, smi->frame_reg, _frame_reg, 8);
    MIIs(c, nop_m, I_RESTORE_PFS(smi->frame_reg),
	 I_MOVtoB(smi->branch_reg, B0));
    MIIs(c, A_MOVR(/*src*/L0, /*dest*/SP), nop_i, nop_i);
    MIBs(c, nop_m, nop_i, B_RET(B0));
}

static void
ia64_alloc(drisc_ctx c, int inputs, int locals, int outputs, int rotate, 
	   int r1)
{
    MsMIs(c, FORMAT_M34(0x6, rotate >> 3, inputs + locals, inputs+locals+outputs, r1, 0),
		  nop_m, nop_i);
}

/*
 *   ia64 procedure frames
____________________________________________________________
| scratch  |  outgoing  |  frame    |  dynamic |   local   |
|  area    |   params   | marker    |  alloc   | storage   |
| 16 bytes | (beyond 8) | for unwind| (alloca) |           |
------------------------------------------------------------
<-- lower addresses					   ^- previous SP
^- current SP						      stored in L0

Registers R4-R7, F2-F5 and F16-F31 must be preserved across calls.
(I.E. they must be preserved with st8.spill and stf.spill.  
stf.spill writes 16 bytes.   4-8 byte ints  + 20 16-byte floats.
*/


extern void
ia64_proc_start(drisc_ctx c, char *subr_name, int arg_count, arg_info_list args,
	     dr_reg_t *arglist)
{
    int i, max_in_reg = 0;
    int cur_arg_offset = 0;
    int frame, breg;
    int float_count = 0;

    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    smi->act_rec_size = 0;
    smi->max_arg_offset = 0;

    /* leave some space */ ia64_local(c, DR_D);
    smi->conversion_word = ia64_local(c, DR_D);
    smi->fcu_word = ia64_local(c, DR_I);
    smi->save_base = ia64_localb(c, 8 * 4 /* IREGS*/ + 20 * 16 /* FREGS */);

    for (i = 0; i < arg_count; i++) {
	switch (args[i].type) {
	case DR_F: case DR_D:
	    if (cur_arg_offset < 8 * 8) {
		int reg;
		args[i].is_register = 1;
		reg = F8 + float_count;
		float_count++;
		dr_rawget(c, reg, args[i].type, DR_TEMP);
		args[i].in_reg = args[i].out_reg = reg;
	    } else {
		args[i].is_register = 0;
	    }
	    break;
	default:
	    if (cur_arg_offset < 8 * 8) {
		args[i].is_register = 1;
		args[i].in_reg = IN0 + cur_arg_offset/8;
		args[i].out_reg = OUT0 + cur_arg_offset/8;
		max_in_reg = args[i].in_reg;
	    } else {
		args[i].is_register = 0;
	    }
	    break;
	}
	args[i].offset = cur_arg_offset;
	cur_arg_offset += roundup(type_info[(int)args[i].type].size, smi->stack_align);
    }
    
    for (i=IN0 ; i < IN7; i++) {
	if (i <= max_in_reg) {
	    dr_rawget(c, i, DR_I, DR_VAR);
	} else {
	    dr_rawput(c, i, DR_I, DR_VAR);
	}
    }

    /* emit start code */
    c->p->cur_ip = (char*)c->p->cur_ip + 16;
    if (!dr_getreg(c, &frame, DR_L, DR_VAR)) {
	    fprintf(stderr, "not enough for frame register\n");
    }
    if (!dr_getreg(c, &breg, DR_L, DR_VAR)) {
	    fprintf(stderr, "not enough for frame register\n");
    }
    smi->frame_reg = R14;
    smi->branch_reg = R15;

    ia64_alloc(c, 8, /* locals */ 8, /* outputs */ 8, 0, smi->frame_reg);
    MIIs(c, FORMAT_A1(0, 0, 0, 0, R0, SP, L0, 0), I_MOVfromB(B0, smi->branch_reg),
	 nop_i);
		    
    ia64_subli(c, SP, SP, 0);  /* filled on via backpatch */
    MIIs(c, nop_m, nop_i, nop_i);  /* in case we need the extra space */
    MIIs(c, nop_m, nop_i, nop_i);  /* in case we need the extra space */
    MIIs(c, nop_m, nop_i, nop_i);  /* in case we need the extra space */
    MIIs(c, nop_m, nop_i, nop_i);  /* in case we need the extra space */
    for(i = R4; i <= R7; i+= 2) {
	MMIs(c, nop_m, nop_i, nop_i);  /* int stores */
    }
    for(i=F2; i<=F5; i+=2) {
	MMIs(c, nop_m, nop_i, nop_i);  /* float stores */
    }	
    for(i=F16; i<=F31; i+=2) {
	MMIs(c, nop_m, nop_i, nop_i);  /* float stores */
    }	


    smi->backpatch_offset = (char*)c->p->cur_ip - (char*)c->p->code_base;
    ia64_pstorei(c, DR_L, 0, smi->branch_reg, _frame_reg, 0);
    ia64_pstorei(c, DR_L, 0, smi->frame_reg, _frame_reg, 8);
    for (i = 0; i < arg_count; i++) {
	int tmp_reg;
	/* only do nothing for int params in registers */
	if (args[i].is_register && ((args[i].type != DR_F) && 
				    (args[i].type != DR_D))) {
	    if (arglist != NULL) arglist[i] = args[i].in_reg;
	    continue;
	}
	if (!dr_getreg(c, &tmp_reg, args[i].type, DR_VAR)) {
	    fprintf(stderr, "not enough registers for parameter %d\n", i);
	}
	if (arglist != NULL) arglist[i] = tmp_reg;
	if (args[i].is_register) {
	    /* must be float */
/*  	    if (args[i].type == DR_F) { */
/*  		sparc_movi2f(c, tmp_reg, args[i].in_reg); */
/*  		dr_rawput(c, args[i].in_reg, DR_I, DR_VAR); */
/*  	    } else { */
/*  		sparc_movi2d(c, tmp_reg, args[i].in_reg); */
/*  		dr_rawput(c, args[i].in_reg, DR_I, DR_VAR); */
/*  		if (smi->stack_align == 4) { */
/*  		    dr_rawput(c, args[i].in_reg, DR_I, DR_VAR); */
/*  		} */
/*  	    } */
	    /* fix this */
	    tmp_reg = args[i].in_reg;
	} else {
	    /* general offset from fp*/
	    int real_offset = args[i].offset - 8*8 + 16;
	    ia64_ploadi(c, args[i].type, 0, tmp_reg, _frame_reg, 
			 real_offset);
	}
	args[i].in_reg = tmp_reg;
	args[i].is_register = 1;
    }

}

static unsigned char ld_x6[] = {
    0x0, /* DR_C */
    0x0, /* DR_UC */
    0x1, /* DR_S */
    0x1, /* DR_US */
    0x2, /* DR_I */
    0x2, /* DR_U */
    0x3, /* DR_L */
    0x3, /* DR_UL */
    0x3, /* DR_P */
    0x2, /* DR_F */
    0x3, /* DR_D */
    0x0, /* DR_V */
    0x0, /* DR_B */
    0x3, /* DR_EC */
};

extern void
ia64_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    if (offset != 0) {
	ia64_addli(c, R2, src, offset);
	src = R2;
    }
    if ((type != DR_F) && (type != DR_D)) {
	MIIs(c, FORMAT_M1(/*m*/0, ld_x6[type], /*hint*/ 0, /*x*/0, src, dest, 0),
		     nop_m, nop_i);
    } else {
	MIIs(c, FORMAT_M6(/*m*/0, ld_x6[type], /*hint*/ 0, /*x*/0, src, dest, 0),
		     nop_m, nop_i);
    }
/*    switch(type){
    case DR_C:
	ia64_lshi(c, dest, dest, 56);
	ia64_rshi(c, dest, dest, 56);
	break;
    case DR_S:
	ia64_lshi(c, dest, dest, 48);
	ia64_rshi(c, dest, dest, 48);
	break;
    case DR_I:
	ia64_lshi(c, dest, dest, 32);
	ia64_rshi(c, dest, dest, 32);
	break;
	}*/
}

extern void
ia64_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    ia64_addl(c, R2, src1, src2);
    if ((type != DR_F) && (type != DR_D)) {
	MIIs(c, FORMAT_M1(/*m*/0, ld_x6[type], /*hint*/ 0, /*x*/0, R2, dest, 0),
	     nop_m, nop_i);
    } else {
	MIIs(c, FORMAT_M6(/*m*/0, ld_x6[type], /*hint*/ 0, /*x*/0, R2, dest, 0),
		     nop_m, nop_i);
    }
    switch(type){
    case DR_C:
/*	ia64_lshi(c, dest, dest, 24);
	ia64_rshi(c, dest, dest, 24);*/
	break;
    case DR_S:
/*	ia64_lshi(c, dest, dest, 16);
	ia64_rshi(c, dest, dest, 16);*/
	break;
    case DR_UC: case DR_US:
	break;
    }
}

extern void
ia64_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    ia64_ploadi(c, type, junk, dest, src, offset);
    if  ((type == DR_C) || (type == DR_UC)) return;
    /* mux1@rev */
    MIIs(c, nop_m, FORMAT_I3(0, 3, 0, 0, 2, 2, 0xb, dest, dest, 0), nop_i);
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen bsload\n");
	break;
    case DR_UL: case DR_L: case DR_P:
    case DR_C: case DR_UC:
	/* nop */
	break;
    case DR_I:
	MIsIs(c, nop_m, nop_i, I_SHRii(dest, dest, 32));
	break;
    case DR_U:
	MIsIs(c, nop_m, nop_i, I_SHRui(dest, dest, 32));
	break;
    case DR_S:
	MIsIs(c, nop_m, nop_i, I_SHRii(dest, dest, 48));
	break;
    case DR_US:
	MIsIs(c, nop_m, nop_i, I_SHRui(dest, dest, 48));
	break;
    }
}


extern void
ia64_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    ia64_pload(c, type, junk, dest, src1, src2);
    if  ((type == DR_C) || (type == DR_UC)) return;
    /* mux1@rev */
    MIIs(c, nop_m, FORMAT_I3(0, 3, 0, 0, 2, 2, 0xb, dest, dest, 0), nop_i);
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen bsload\n");
	break;
    case DR_UL: case DR_L: case DR_P:
    case DR_C: case DR_UC:
	/* nop */
	break;
    case DR_I:
	MIsIs(c, nop_m, nop_i, I_SHRii(dest, dest, 32));
	break;
    case DR_U:
	MIsIs(c, nop_m, nop_i, I_SHRui(dest, dest, 32));
	break;
    case DR_S:
	MIsIs(c, nop_m, nop_i, I_SHRii(dest, dest, 48));
	break;
    case DR_US:
	MIsIs(c, nop_m, nop_i, I_SHRui(dest, dest, 48));
	break;
    }
}

static unsigned char st_x6[] = {
    0x30, /* DR_C */
    0x30, /* DR_UC */
    0x31, /* DR_S */
    0x31, /* DR_US */
    0x32, /* DR_I */
    0x32, /* DR_U */
    0x33, /* DR_L */
    0x33, /* DR_UL */
    0x33, /* DR_P */
    0x32, /* DR_F */
    0x33, /* DR_D */
    0x00, /* DR_V */
    0x00, /* DR_B */
    0x33, /* DR_EC */
};

extern void
ia64_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{

    if (offset != 0) {
	ia64_addli(c, R2, src, offset);
	src = R2;
    }
    if ((type != DR_D) && (type != DR_F)) {
	MIIs(c, FORMAT_M4(/*m*/0, st_x6[type], /*hint*/ 0, /*x*/0, src, dest, 0),
		     nop_m, nop_i);
    } else {
	MIIs(c, FORMAT_M9(/*m*/0, st_x6[type], /*hint*/0, /*x*/0, src, dest, 0),
	     nop_m, nop_i);
    }
}

extern void
ia64_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    ia64_addl(c, R2, src1, src2);
    if ((type != DR_D) && (type != DR_F)) {
	MIIs(c, FORMAT_M4(/*m*/0, st_x6[type], /*hint*/0, /*x*/0, R2, dest, 0),
	     nop_m, nop_i);
    } else {
	MIIs(c, FORMAT_M9(/*m*/0, st_x6[type], /*hint*/0, /*x*/0, R2, dest, 0),
	     nop_m, nop_i);
    }
}

static long drisc_hidden_mod(long a, long b)
{ return a % b; }
static long drisc_hidden_umod(unsigned long a, unsigned long b)
{ return a % b; }

extern void ia64_mod(drisc_ctx c, int data1, int data2, int dest, int src1, 
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



extern void ia64_setl(drisc_ctx c, int r, long val)
{
    if ((val < 0x1fffff) && (val > -(0x1fffff))) {
	int imm7b = val & 0x7f;
	int imm9d = (val >> 7) & 0x1ff;
	int imm5c = (val >> 16) & 0x1f;
	int s = (val >> 21) & 0x1;
	MIIs(c, FORMAT_A5(s, imm9d, imm5c, R0, imm7b, r, 0), nop_i, nop_i);
    } else {
	int imm7b = val & 0x7f;
	int imm9d = (val >> 7) & 0x1ff;
	int imm5c = (val >> 16) & 0x1f;
	int ic = (val >> 21) & 0x1;
	long slot1 = (val >> 22) & 0x1ffffffffff;
	int i = (val >> 63) & 0x1;
	MLXs(c, nop_m, slot1, FORMAT_X2(i, imm9d, imm5c, ic, 0, imm7b, r, 0));
    }
}

extern void ia64_modi(drisc_ctx c, int data1, int data2, int dest, int src1, 
		      long imm)
{
    ia64_setl(c, _temp_reg, imm);
    ia64_mod(c, data1, data2, dest, src1, _temp_reg);
}

extern void
ia64_mov(drisc_ctx c, int type, int junk, int dest, int src)
{
    if (src == dest) return;
    switch(type) {
    case DR_D:
    case DR_F:
	MFIs(c, nop_m, FORMAT_F9(0, /*x*/0, /*x6*/0x10, src, src, dest, 0), nop_i);
	break;
    default:
	ia64_movl(c, dest, src);
    }
}

extern void ia64_arith3(c, x4, x2b, dest, src1, src2)
drisc_ctx c;
int x4;
int x2b;
int dest;
int src1;
int src2;
{
    MIIs(c, FORMAT_A1(/*x2a*/0, /*ve*/0, x4, x2b, src2, src1, dest, 0), nop_i, nop_m);
}

extern void ia64_arith2(c, op, subop, dest, src)
drisc_ctx c;
int op;
int subop;
int dest;
int src;
{
    if (op == 0) {
/*	cmp4.eq p6, p7 = 0, src
	;;
	(p6) addl dest = 1, r0
	(p7) mov dest = r0
*/
	MIIs(c, nop_m, FORMAT_A6(0xe,0, 0, 0, P6, src, R0, 0, P7, 0), nop_i);
	MIIs(c, nop_m, FORMAT_A5(0, 0, 0, R0, 0, dest, P6), FORMAT_A5(0, 0, 0, R0, 1, dest, P7));
    } else if (op == 1) {
	/* andcm -1 = comm*/  
	MIIs(c, nop_m, FORMAT_A3(1, /*x2a*/0, /*ve*/0, 0xb, 1, src, 0x7f, dest, 0), nop_i);
    } else {
	/* neg */
	MIIs(c, FORMAT_A1(/*x2a*/0, /*ve*/0, /*x4*/1, /*x2b*/1, src, R0, dest, 0), nop_i, nop_m);
    }
}

extern void ia64_mul(c, sign, imm, dest, src1, src2)
drisc_ctx c;
int sign;
int imm;
int dest;
int src1;
int src2;
{
    int tmpf1  = F6, tmpf2 = F7, dstf = F6;

	/* SETF.SIG */
    MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src1, tmpf1, 0),
		  FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src2, tmpf2, 0),
		  nop_i);
    /* xma.l */
    MFIs(c, nop_m, FORMAT_F2(/*x*/1, /*x2*/0, tmpf1, tmpf2, F0, dstf, 0), nop_i);
    /*     getf.sig   */
    MMIs(c, FORMAT_M19(/*m*/0, /*x6*/0x1c, /*x*/1, dstf, dest, 0),
		  nop_m, nop_i);
}

extern void ia64_muli(c, sign, i, dest, src, imm)
drisc_ctx c;
int sign;
int i;
int dest;
int src;
long imm;
{
    int tmpf1 = F6, tmpf2 = F7, dstf = F6;
    int src2 = R2;

    ia64_setl(c, src2, imm);
	/* SETF.SIG */
    MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src, tmpf1, 0),
		  FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src2, tmpf2, 0),
		  nop_i);
    /* xma.l */
    MFIs(c, nop_m, FORMAT_F2(/*x*/1, /*x2*/0, tmpf1, tmpf2, F0, dstf, 0), nop_i);
    /*     getf.sig   */
    MIIs(c, FORMAT_M19(/*m*/0, /*x6*/0x1c, /*x*/1, dstf, dest, 0),
		  nop_m, nop_i);
}

static long drisc_hidden_div(long a, long b)
{ return a / b; }
static long drisc_hidden_udiv(unsigned long a, unsigned long b)
{ return a / b; }

extern void ia64_div_modi(c, type, div, dest, src1, imm)
drisc_ctx c;
int type;
int div;
int dest;
int src1;
long imm;
{
    int tmp = R2;
    ia64_setl(c, tmp, imm);
    ia64_div_mod(c, type, div, dest, src1, tmp);
}

extern void ia64_div_mod(c, type, div, dest, src1, src2)
drisc_ctx c;
int type;
int div;
int dest;
int src1;
int src2;
{
    int return_reg;
    if (type == DR_I) {
	MIIs(c, nop_m, I_Sext32(src1, src1), I_Sext32(src2, src2));
    }
    if (type == DR_U) {
	MIIs(c, nop_m, I_Zext32(src1, src1), I_Zext32(src2, src2));
    }
    if (div == 1) {
	if ((type == DR_U) || (type == DR_UL)) {
	    return_reg = dr_scalll(c, (void*)drisc_hidden_udiv, "%l%l", src1, src2);
	    dr_movl(c, dest, return_reg);
	} else {
	    return_reg = dr_scalll(c, (void*)drisc_hidden_div, "%ul%ul", src1, src2);
	    dr_movl(c, dest, return_reg);
	}
	    
    } else {
	if ((type == DR_U) || (type == DR_UL)) {
	    return_reg = dr_scalll(c, (void*)drisc_hidden_umod, "%l%l", src1, src2);
	    dr_movl(c, dest, return_reg);
	} else {
	    return_reg = dr_scalll(c, (void*)drisc_hidden_mod, "%l%l", src1, src2);
	    dr_movl(c, dest, return_reg);
	}
    }
}


extern void ia64_arith3i(c, x4, x2b, dest, src, imm)
drisc_ctx c;
int x4;
int x2b;
int dest;
int src;
long imm;
{
    if ((x4 == 9) && (x2b == 1)) { /* sub */
	/* negate the immediate and use add */
	imm = -imm;
	x4 = x2b = 0;
    }
    if ((x4 != 0) || (x2b != 0)) {  /* not add */
	if ((imm < 0x7f) && (imm > -(0x7f))) {
	    int imm7b = imm & 0x7f;
	    int s = (imm >> 7 ) & 1;
	    MIIs(c, nop_m, FORMAT_A3(s, /*x2a*/0, /*ve*/0, x4, x2b, src, imm7b, dest, 0), nop_i);
	} else {
	    ia64_setl(c, R2, imm);
	    ia64_arith3(c, x4-8, x2b, dest, src, R2);
	}
    } else {
	/* add */
	if ((imm < 0x1fff) && (imm > -(0x1fff))) {
	    int imm7b = imm & 0x7f;
	    int imm6d = (imm >> 7) & 0x3f;
	    int s = (imm >> 13 ) & 1;
	    MIIs(c, FORMAT_A4(s, /*x2a*/2, /*ve*/0, imm6d, src, imm7b, dest, 0), nop_i, nop_m);
	} else {
	    ia64_setl(c, R2, imm);
	    ia64_arith3(c, x4, x2b, dest, src, R2);
	}
    }
}

extern void ia64_farith(c, opcode, x, dest, src1, src2)
drisc_ctx c;
int opcode;
int x;
int dest;
int src1;
int src2;
{
    int mult4 = F1;
    int mult3 = src1;
    int add1 = src2;
    if ((opcode & 0x10) == 0x10) {
	/* really mult */
	mult4 = src2;
	add1 = F0;
	opcode &= 0xf;
    }
    MFIs(c, nop_m, FORMAT_F1(opcode, x, /*sf*/0, mult4, mult3, add1, dest, 0), nop_i);
}

static double drisc_hidden_fdiv(double a, double b)
{ return a / b; }

extern void ia64_fdiv(c, type, junk, dest, src1, src2)
drisc_ctx c;
int type;
int junk;
int dest;
int src1;
int src2;
{
    int return_reg, x = 0;
    if (type == DR_D) x = 0;
    if (type == DR_F) x = 1;

    return_reg = dr_scalld(c, (void*)drisc_hidden_fdiv, "%d%d", src1, src2);
    MFIs(c, nop_m, FORMAT_F1(0x8, x, /*sf*/0, return_reg, F1, F0, dest, 0), nop_i);

}

extern void ia64_farith2(c, b1, b2, dest, src)
drisc_ctx c;
int b1;
int b2;
int dest;
int src;
{
    /* neg */
    /*fmerge.ns dest, src, src*/
    MFIs(c, nop_m, FORMAT_F9(/*opcode*/0, /*x*/0, /*x6*/0x11, src, src, dest, 0), nop_i);
}

extern void ia64_shift(c, left, type, dest, src1, src2)
drisc_ctx c;
int left;
int type;
int dest;
int src1;
int src2;
{
    if (left == 1) {
	MIIs(c, nop_m, I_SHL(dest, src1, src2), nop_i);
    } else {
	long ext_insn = nop_i;
	switch(type) {
	case DR_I:
	    ext_insn = I_Sext32(src1, dest);
	    src1 = dest;
	case DR_L:
	    MIsIs(c, nop_m, ext_insn, I_SHRi(dest, src1, src2));
	    break;
	case DR_U:
	    ext_insn = I_Zext32(src1, dest);
	    src1 = dest;
	case DR_UL:
	    MIsIs(c, nop_m, ext_insn, I_SHRu(dest, src1, src2));
	    break;
	}
    }
}

extern void ia64_shifti(c, left, type, dest, src, imm)
drisc_ctx c;
int left;
int type;
int dest;
int src;
long imm;
{
    if (left == 1) {
	MIIs(c, nop_m, I_SHLi(dest, src, imm), nop_i);
    } else {
	long ext_insn = nop_i;
	switch(type) {
	case DR_I:
	    ext_insn = I_Sext32(src, dest);
	    src = dest;
	case DR_L:
	    MIsIs(c, nop_m, ext_insn, I_SHRii(dest, src, imm));
	    break;
	case DR_U:
	    ext_insn = I_Zext32(src, dest);
	    src = dest;
	case DR_UL:
	    MIsIs(c, nop_m, ext_insn, I_SHRui(dest, src, imm));
	    break;
	}
    }

}

#define CONV(x,y) ((x*100)+y)
extern void
ia64_convert(drisc_ctx c, int from_type, int to_type, 
	      int dest, int src)
{
    switch(CONV(from_type, to_type)) {
    case CONV(DR_U, DR_UL):
    case CONV(DR_U, DR_L):
	MIIs(c, nop_m, I_Zext32(src, dest), nop_i);
	break;
    case CONV(DR_I, DR_U):
    case CONV(DR_U, DR_I):
    case CONV(DR_UL,DR_I):
    case CONV(DR_UL,DR_U):
    case CONV(DR_UL,DR_L):
    case CONV(DR_L,DR_U):
    case CONV(DR_L,DR_UL):
    case CONV(DR_L,DR_I):
    case CONV(DR_P,DR_UL):
    case CONV(DR_UL,DR_P):
	if(src == dest) return;
	ia64_movl(c, dest,src);
	break;
    case CONV(DR_I,DR_UL):
    case CONV(DR_I, DR_L):
	MIIs(c, nop_m, I_Sext32(src, dest), nop_i);
	break;
    case CONV(DR_D,DR_F):
	MFIs(c, nop_m, FORMAT_F1(0x8, 1, /*sf*/0, src, F1, F0, dest, 0), nop_i);
	break;
    case CONV(DR_F,DR_D):
	MFIs(c, nop_m, FORMAT_F9(0, /*x*/0, /*x6*/0x10, src, src, dest, 0), nop_i);
	break;
    case CONV(DR_F,DR_U):
    case CONV(DR_F,DR_UL):
    case CONV(DR_D,DR_U):
    case CONV(DR_D,DR_UL):
    {
	int ftmp = F6;
	/* fcvt.xuf.d */
	MFIs(c, nop_m, FORMAT_F10(0, 0, 0, 0x1b, src, ftmp, 0), nop_i);
	MMIs(c, FORMAT_M19(/*m*/0, /*x6*/0x1c, /*x*/1, ftmp, dest, 0),
	     nop_m, nop_i);
	break;
    }
    case CONV(DR_F,DR_I):
    case CONV(DR_F,DR_L):
    case CONV(DR_D,DR_I):
    case CONV(DR_D,DR_L):
    {
	int ftmp = F6;
	/* fcvt.xf.d */
	MFIs(c, nop_m, FORMAT_F10(0, 0, 0, 0x1a, src, ftmp, 0), nop_i);
	MMIs(c, FORMAT_M19(/*m*/0, /*x6*/0x1c, /*x*/1, ftmp, dest, 0),
	     nop_m, nop_i);
	break;
    }
    case CONV(DR_I,DR_D):
    case CONV(DR_I,DR_F):
	MIIs(c, nop_m, I_Sext32(src, src), nop_i);
    case CONV(DR_L,DR_D):
	MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src, dest, 0),
	     nop_m, nop_i);
	/*fcvt.xf */
	MFIs(c, nop_m, FORMAT_F11(0, 0x1c, dest, dest, 0), nop_i);
	break;
    case CONV(DR_L,DR_F):
	MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src, dest, 0),
	     nop_m, nop_i);
	/*fcvt.xf */
	MFIs(c, nop_m, FORMAT_F11(0, 0x1c, dest, dest, 0), nop_i);
	/* fnorm.s */
	MFIs(c, nop_m, FORMAT_F1(0x8, /*x*/1, /*sf*/0, dest, F1, F0, dest, 0), nop_i);
	break;
    case CONV(DR_U,DR_D):
    case CONV(DR_UL,DR_D):
	MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src, dest, 0),
	     nop_m, nop_i);
	MFIs(c, nop_m, FORMAT_F1(0x9, 0, /*sf*/0, dest, F1, F0, dest, 0), nop_i);
	break;
    case CONV(DR_U,DR_F):
    case CONV(DR_UL,DR_F):
	MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1c, /*x*/1, src, dest, 0),
	     nop_m, nop_i);
	MFIs(c, nop_m, FORMAT_F1(0x8, 1, /*sf*/0, dest, F1, F0, dest, 0), nop_i);
	break;
    case CONV(DR_C,DR_I):
    case CONV(DR_C,DR_L):
    case CONV(DR_C,DR_U):
    case CONV(DR_C,DR_UL):
	MIIs(c, nop_m, I_Sext8(src, dest), nop_i);
	break;
    case CONV(DR_I, DR_C):
    case CONV(DR_U, DR_C):
    case CONV(DR_L, DR_C):
    case CONV(DR_UL, DR_C):
	/* */
	if(src == dest) return;
	ia64_movl(c, dest,src);
	break;
    case CONV(DR_S,DR_I):
    case CONV(DR_S,DR_L):
    case CONV(DR_S,DR_U):
    case CONV(DR_S,DR_UL):
	/* signext16 - lsh16, rsha16 */
	MIIs(c, nop_m, I_Sext16(src, dest), nop_i);
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
	if(src == dest) return;
	ia64_movl(c, dest,src);
	break;
    default:
	printf("Unknown case in ia64 convert %d\n", CONV(from_type,to_type));
    }
}

extern void
ia64_branch(drisc_ctx c, int op, int type, int src1, int src2, int label)
{
    int p1 = P6, p2 = P0;
    switch(op) {
    case dr_beq_code:
    case dr_blt_code:
	break;
    case dr_bne_code:  /* switch predicates, use eq */
	p1 = P0; p2 = P6;  op = dr_beq_code;
	break;
    case dr_bge_code:  /* switch predicates, use lt */
	p1 = P0; p2 = P6;  op = dr_blt_code;
	break;
    case dr_bgt_code:  /* switch operands, use lt */
        {int tmp = src1;	src1 = src2; src2 = tmp; op = dr_blt_code;}
	break;
    case dr_ble_code:  /* switch operands, switch predicates, use lt */
        {int tmp = src1;	src1 = src2; src2 = tmp; op = dr_blt_code;}
	p1 = P0; p2 = P6;  op = dr_blt_code;
	break;
    }
    if ((type == DR_F) || (type == DR_D)) {
	int rb = 0;
	int sf = 0;
	int ra = 0;
	switch(op) {
	case dr_beq_code:
	    break;
	case dr_blt_code:
	    rb = 1;
	    break;
	}
	MFIs(c, nop_m, FORMAT_F4(rb, sf, ra, p2, src2, src1, 0, p1, 0), nop_i);
	dr_mark_branch_location(c, label);
	MIBs(c, nop_m, nop_i, FORMAT_B1(/*s*/0, /*d*/0, /*wh*/0, /*imm*/0, 
		/*p*/0, /*btype*/ 0, P6));
	return;
    } else {
	int x2 = 0;
	int opcode = 0;
	switch(op) {
	case dr_beq_code:
	    opcode = 0xe;
	    break;
	case dr_bge_code:
	case dr_bgt_code:
	case dr_ble_code:
	case dr_blt_code:
	    opcode = 0xc;
	    if ((type == DR_U) || (type == DR_UL)) opcode = 0xd;
	    break;
	case dr_bne_code:
	    break;
	}
	if ((type == DR_U) || (type == DR_I)) {
	    x2 = 1;
	}
	MIIs(c, nop_m, FORMAT_A6(opcode,0, x2, 0, p2, src2, src1, 0, p1, 0), nop_i);
	dr_mark_branch_location(c, label);
	MIBs(c, nop_m, nop_i, FORMAT_B1(/*s*/0, /*d*/0, /*wh*/0, /*imm*/0, 
		/*p*/0, /*btype*/ 0, P6));
    }
}

extern void 
ia64_jump_to_label(drisc_ctx c, unsigned long label)
{
    dr_mark_branch_location(c, label);
    MIBs(c, nop_m, nop_i, FORMAT_B1(/*s*/0, /*d*/0, /*wh*/0, /*imm*/0, 
				    /*p*/0, /*btype*/ 0, P0));
}

extern void ia64_jump_to_reg(drisc_ctx c, unsigned long reg)
{
    MIIs(c, nop_m, I_MOVtoB(reg, B6),  nop_i);
    BBBs(c, FORMAT_B4(/*d*/0, /*wh*/1, 0x20, B6, 0, 0, 0), nop_m, nop_i);
}

extern void ia64_jump_to_imm(drisc_ctx c, unsigned long imm)
{
    ia64_setl(c, R2, imm);
    ia64_jump_to_reg(c, R2);
}

extern void 
ia64_jal(drisc_ctx c, int return_addr_reg, int target)
{
/* jump, source addr to return_addr_reg */
}

static void
ia64_movd2i(drisc_ctx c, int dest, int src)
{
    /*     getf.d   */
    MIIs(c, FORMAT_M19(/*m*/0, /*x6*/0x1f, /*x*/1, src, dest, 0),
		  nop_m, nop_i);
}

static void
ia64_movf2i(drisc_ctx c, int dest, int src)
{
    /*     getf.d   */
    MIIs(c, FORMAT_M19(/*m*/0, /*x6*/0x1e, /*x*/1, src, dest, 0),
		  nop_m, nop_i);
}

static void internal_push(drisc_ctx c, int type, int immediate, 
			  void *value_ptr)
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    struct arg_info arg;
    int real_offset;

    arg.is_immediate = immediate;
    switch(type) {
    case DR_C: case DR_S:  case DR_I: case DR_L:
	arg.type = DR_L;
	break;
    case DR_UC: case DR_US: case DR_U: case DR_UL:
	arg.type = DR_UL;
	break;
    default:
	arg.type = type;
    }
	
    if (smi->cur_arg_offset < 8 * smi->stack_align) {
	if ((type == DR_F) || (type == DR_D)) {
	    /* floating arg in float reg and int reg */
	    arg.is_register = 1;
	    arg.out_reg = F8  + smi->float_count;
	    arg.in_reg = OUT0 + smi->cur_arg_offset/smi->stack_align;
	    smi->float_count++;
	} else {
	    arg.is_register = 1;
	    arg.in_reg = IN0 + smi->cur_arg_offset/smi->stack_align;
	    arg.out_reg = OUT0 + smi->cur_arg_offset/smi->stack_align;
	}
    } else {
	arg.is_register = 0;
    }

    arg.offset = smi->cur_arg_offset;
    smi->cur_arg_offset += 
	roundup(type_info[(int)arg.type].size, smi->stack_align);
    if (smi->cur_arg_offset > smi->max_arg_offset) {
	smi->max_arg_offset = smi->cur_arg_offset;
    }
    real_offset = arg.offset - 8 * 8 + 16;
    if (arg.is_register == 0) {
	/* store it on the stack only */
	if (arg.is_immediate) {
	    if (type == DR_F) {
		float f = (float) *(double*)value_ptr;
		ia64_setl(c, R8, *(int*)&f);
	    } else {
		ia64_setl(c, R8, *(long*)value_ptr);
	    }
	    ia64_pstorei(c, DR_L, 0, R8, SP, real_offset);
	} else {
	    ia64_pstorei(c, arg.type, 0, *(int*)value_ptr, SP, 
			 real_offset);
	}
    } else {
	if ((type != DR_F) && (type != DR_D)) {
	    if (arg.is_immediate) {
		ia64_setl(c, arg.out_reg, *(long*)value_ptr);
	    } else {
		ia64_mov(c, type, 0, arg.out_reg, *(int*) value_ptr);
	    }
	} else {
	    if (arg.is_immediate) {
		if ((type == DR_F) || (type == DR_D)) {
		    /* set appropriate register */
		    ia64_setf(c, type, 0, arg.out_reg, 
			      *(double*)value_ptr);
		} else {
		    ia64_setl(c, arg.out_reg, *(int*)value_ptr);
		}
	    } else {
		/* move to the appropriate float reg */
		ia64_mov(c, type, 0, arg.out_reg, *(int*)value_ptr);
	    }
	    if (arg.in_reg != -1) {
		/* put value in int regs too */
		if (type == DR_D) {
		    ia64_movd2i(c, arg.in_reg, arg.out_reg);
		} else {
		    ia64_movf2i(c, arg.in_reg, arg.out_reg);
		}
	    } else {
		/* put it on the stack as well */
		ia64_pstorei(c, arg.type, 0, arg.out_reg, SP,
			     real_offset);
	    }
	}
    }
}

static void push_init(drisc_ctx c)
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    smi->cur_arg_offset = 0;
    smi->float_count = 0;
}

extern void ia64_push(drisc_ctx c, int type, int reg)
{
    if ((type == DR_V) && (reg == -1)) {
	push_init(c);
    } else {
	internal_push(c, type, 0, &reg);
    }
}

extern void ia64_pushi(drisc_ctx c, int type, long value)
{
    internal_push(c, type, 1, &value);
}

extern void ia64_pushfi(drisc_ctx c, int type, double value)
{
    internal_push(c, type, 1, &value);
}

extern int ia64_calli(drisc_ctx c, int type, void *xfer_address)
{
    int caller_side_ret_reg = R8;
    int creg = R14, gpsave = R2, gpload = R3;
    int i;

    /* save temporary registers */
    for (i=R14; i < R31 ; i+=1) {
	if (drisc_mustsave(&c->p->tmp_i, i)) {
	    ia64_save_restore_op(c, 0, DR_L, i);
	}
    }

    ia64_setl(c, R8, (long)xfer_address);
    MIIs(c, FORMAT_M1(/*m*/0, ld_x6[DR_L], /*hint*/ 0, /*x*/0, R8, creg, 0),
	 nop_m, FORMAT_A4(0, /*x2a*/2, /*ve*/0, 0, R8, 0x8, gpload, 0));
    if ((type == DR_D) || (type == DR_F)) {
/*	caller_side_ret_reg = _f0;*/
    }
    MIIs(c, nop_m, I_MOVtoB(creg, B6),  A_MOVR(/*src*/R1, /*dest*/gpsave));
    ia64_ploadi(c, DR_L, 0, R1, gpload, 0);
    BBBs(c, FORMAT_B5(/*d*/0, /*wh*/1, B6, 1, B0, 0), nop_m, nop_i);
    MIIs(c, nop_m, nop_i, A_MOVR(/*src*/gpsave, /*dest*/R1));

    /* restore temporary registers */
    for (i=R14; i < R31 ; i+=1) {
	if (drisc_mustsave(&c->p->tmp_i, i)) {
	    ia64_save_restore_op(c, 1, DR_L, i);
	}
    }

    return caller_side_ret_reg;
}

extern int ia64_callr(drisc_ctx c, int type, int src)
{
    int caller_side_ret_reg = R8;
    int creg = R14, gpsave = R2, gpload = R3;

    /* save temporary registers */
    MIIs(c, FORMAT_M1(/*m*/0, ld_x6[DR_L], /*hint*/ 0, /*x*/0, src, creg, 0),
	 nop_m, FORMAT_A4(0, /*x2a*/2, /*ve*/0, 0, src, 0x8, gpload, 0));
    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
/*	caller_side_ret_reg = _f0;*/
    }
    MIIs(c, nop_m, I_MOVtoB(creg, B6),  A_MOVR(/*src*/R1, /*dest*/gpsave));
    ia64_ploadi(c, DR_L, 0, R1, gpload, 0);
    BBBs(c, FORMAT_B5(/*d*/0, /*wh*/1, B6, 1, B0, 0), nop_m, nop_i);
    MIIs(c, nop_m, nop_i, A_MOVR(/*src*/gpsave, /*dest*/R1));
    return caller_side_ret_reg;
}

extern void
ia64_branchi(drisc_ctx c, int op, int type, int src, long imm, int label)
{
    if (imm == 0) {
	ia64_branch(c, op, type, src, R0, label);
    } else {
	ia64_setl(c, R2, imm);
	ia64_branch(c, op, type, src, R2, label);
    }
}

extern void ia64_ret(drisc_ctx c, int data1, int data2, int src)
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
	if (src != R8) ia64_movl(c, R8, src);
	break;
    case DR_F:
    case DR_D:
	if (src != F8) {
	    /*ia64_movf(c, f8, src);*/
	    MFIs(c, nop_m, FORMAT_F9(0, /*x*/0, /*x6*/0x10, src, src, F8, 0), nop_i);
	}
    }
    ia64_simple_ret(c);
}

extern void ia64_reti(drisc_ctx c, int data1, int data2, long imm)
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
	ia64_setl(c, R8, imm);
	break;
    }
    ia64_simple_ret(c);
}

extern void ia64_retf(drisc_ctx c, int data1, int data2, double imm)
{
    union {
	double d;
	long l;
    } a;

    a.d = imm;
    ia64_setl(c, R8, a.l);
    /*setf.d */
    MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1f, /*x*/1, R8, F8, 0),
	 nop_m, nop_i);
    ia64_simple_ret(c);
}

static void
ia64_data_link(drisc_ctx c)
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
ia64_branch_link(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    int i;

    for(i=0; i< t->branch_count; i++) {
	int label = t->branch_locs[i].label;
	int label_offset = t->label_locs[label] - t->branch_locs[i].loc;
	char *branch_addr = (char*)((char *)c->p->code_base + 
				  t->branch_locs[i].loc);
	int offset = label_offset >> 4;
	long imm20b = offset & 0xfffff;
	long s = (offset >> 20) & 0x1;
	long op2_or = ((s << 36) | (imm20b << 13));
	*(((unsigned long*)branch_addr + 1)) |= (unsigned long)(((long)(op2_or)) << 23 );
    }
}

static void
ia64_call_link(drisc_ctx c)
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
ia64_spill_fill(drisc_ctx c, int spill)
{
    int save_insns = 0;
    int reg;
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;

    for(reg = R4; reg <= R7; reg+= 2) {
	if (drisc_wasused(&c->p->tmp_i, reg) || 
	    drisc_wasused(&c->p->tmp_i, reg+1)) {
	    save_insns++;
	}
    }
    for(reg = F2; reg <= F5; reg+= 2) {
	if (drisc_wasused(&c->p->tmp_f, reg) || 
	    drisc_wasused(&c->p->tmp_f, reg+1)) {
	    save_insns++;
	}
    }
    for(reg = F16; reg <= F31; reg+= 2) {
	if (drisc_wasused(&c->p->tmp_f, reg) || 
	    drisc_wasused(&c->p->tmp_f, reg+1)) {
	    save_insns++;
	}
    }

   if (save_insns > 0) {
       ia64_addli(c, R2, L0, smi->save_base);
       ia64_addli(c, R3, R2, 8);
	
	for(reg = R4; reg <= R7; reg+= 2) {
	    if (drisc_wasused(&c->p->tmp_i, reg) || 
		drisc_wasused(&c->p->tmp_i, reg+1)) {
		if (spill) {
		    /* st8.spill */
		    MMIs(c, FORMAT_M5(0, 0x3b, 0, 0, R2, reg, 8, 0),
			 FORMAT_M5(0, 0x3b, 0, 0, R3, reg + 1, 8, 0), nop_i);
		} else {
		    /* ld8.fill */
		    MMIs(c, FORMAT_M3(0, 0x1b, 0, 0, R2, 8, reg, 0),
			 FORMAT_M3(0, 0x1b, 0, 0, R3, 8, reg + 1, 0), nop_i);
		}
	    }
	}
       ia64_addli(c, R3, R2, 16);
	for(reg = F2; reg <= F5; reg+= 2) {
	    if (drisc_wasused(&c->p->tmp_f, reg) || 
		drisc_wasused(&c->p->tmp_f, reg+1)) {
		if (spill) {
		    /* stf.spill */
		    MMIs(c, FORMAT_M10(/*m*/0, 0x3b, 0, 0, R2, reg, 32, 0),
			 FORMAT_M10(0, 0x3b, 0, 0, R3, reg + 1, 32, 0), nop_i);
		} else {
		    /* ldf.fill */
		    MMIs(c, FORMAT_M8(/*m*/0, 0x1b, 0, 0, R2, 32, reg, 0),
			 FORMAT_M8(0, 0x1b, 0, 0, R3, 32, reg + 1, 0), nop_i);
		}
	    }
	}
	
	for(reg = F16; reg <= F31; reg+= 2) {
	    if (drisc_wasused(&c->p->tmp_f, reg) || 
		drisc_wasused(&c->p->tmp_f, reg+1)) {
		if (spill) {
		    /* stf.spill */
		    MMIs(c, FORMAT_M10(/*m*/0, 0x3b, 0, 0, R2, reg, 32, 0),
			 FORMAT_M10(0, 0x3b, 0, 0, R3, reg + 1, 32, 0), nop_i);
		} else {
		    /* ldf.fill */
		    MMIs(c, FORMAT_M8(/*m*/0, 0x1b, 0, 0, R2, 32, reg, 0),
			 FORMAT_M8(0, 0x1b, 0, 0, R3, 32, reg + 1, 0), nop_i);
		}
	    }
	}
    }
}

static void
ia64_emit_save(drisc_ctx c)
{
    ia64_mach_info smi = (ia64_mach_info) c->p->mach_info;
    void *save_ip = c->p->cur_ip;
    int ar_size = smi->act_rec_size;
    int reg;

    int code_start = 16;   /* skip indirect code pointer and GP */
    int save_insns = 0;

    ar_size = roundup(ar_size + 16 +16/*good measure*/+ smi->max_arg_offset, 16);
    c->p->cur_ip = (char*)c->p->code_base;

    for(reg = R4; reg <= R7; reg+= 2) {
	if (drisc_wasused(&c->p->tmp_i, reg) || 
	    drisc_wasused(&c->p->tmp_i, reg+1)) {
	    save_insns++;
	}
    }
    for(reg = F2; reg <= F5; reg+= 2) {
	if (drisc_wasused(&c->p->tmp_f, reg) || 
	    drisc_wasused(&c->p->tmp_f, reg+1)) {
	    save_insns++;
	}
    }
    for(reg = F16; reg <= F31; reg+= 2) {
	if (drisc_wasused(&c->p->tmp_f, reg) || 
	    drisc_wasused(&c->p->tmp_f, reg+1)) {
	    save_insns++;
	}
    }

    if (save_insns > 0) {
	save_insns += 3;  /* to fill in regs */
    }

    code_start = smi->backpatch_offset - (save_insns + 4)*16;
    if (code_start < 16) {
	printf("Consistency problems.  Tell Greg\n");
	printf("code start is at %d, backpatch %d, save_insns %d (%d bytes)\n",
	       code_start, smi->backpatch_offset, save_insns, (save_insns+3)*16);
    }
    *((void**)c->p->cur_ip) = (char*)c->p->code_base + code_start;
    *((void**)c->p->cur_ip+1) = 0;


    c->p->cur_ip = (char*)c->p->code_base + 16;
    /* wipe out prior alloc, etc. */
    MIIs(c, nop_m, nop_i, nop_i);
    MIIs(c, nop_m, nop_i, nop_i);
    MIIs(c, nop_m, nop_i, nop_i);
    
    c->p->cur_ip = (char*)c->p->code_base + code_start;
    /* do local space reservation */
    ia64_alloc(c, 8, /* locals */ 8, /* outputs */ 8, 0, smi->frame_reg);
    MIIs(c, FORMAT_A1(0, 0, 0, 0, R0, SP, L0, 0), I_MOVfromB(B0, smi->branch_reg),
	 nop_i);

    ia64_subli(c, SP, SP, ar_size);

    ia64_spill_fill(c, 1);
    {
	int remaining_noops = smi->backpatch_offset - 
	    ((char*)c->p->cur_ip - (char*)c->p->code_base);
	if ((remaining_noops != 0) && (remaining_noops != 16)) {
	    printf("Spill's end at %d, backpatch is at %d\n", 
		   (int)((char*)c->p->cur_ip - (char*)c->p->code_base),
		   smi->backpatch_offset);
	    printf("Consistency problems.  Tell Greg, arsize %x\n",ar_size);
	    printf("code start is at %d, backpatch %d, save_insns %d (%d bytes)\n",
		   code_start, smi->backpatch_offset, save_insns, (save_insns+3)*16);
	    
	    c->p->cur_ip = save_ip;
	    dr_dump(c);
	}
    }
    c->p->fp = (char*)c->p->code_base;
    c->p->cur_ip = save_ip;
}
    
static void
ia64_flush(void *start, char *end)
{
    char *tmp;
    for (tmp = start; tmp < end ; tmp += 32) {
	__asm__ __volatile__ ("fc %0;;;"::"r" (tmp):"memory");
    }
    __asm__ __volatile__ ("sync.i;;; mf;;; srlz.i;;;");
/*
fc.i l1
    ;;
sync.i
    ;;
srlz.i
;;*/
/*static inline void _svmf_iflush (_svmt_word *pword) {
 __asm__ __volatile__ ("fc %0;;; sync.i;;; mf;;; srlz.i;;;"::
                        "r" (pword):"memory");
			}*/
}

extern void
ia64_end(c)
drisc_ctx c;
{
    ia64_simple_ret(c);
    ia64_branch_link(c);
    ia64_call_link(c);
    ia64_data_link(c);
    ia64_emit_save(c);
    ia64_flush(c->p->code_base, c->p->code_limit);
}

extern void *
ia64_clone_code(c, new_base, available_size)
drisc_ctx c;
void *new_base;
int available_size;
{
    int size = dr_code_size(c);
    void *old_base = c->p->code_base;
    void *native_base = c->p->code_base;
    int code_start_offset;
    long code_start;
    if (available_size < size) {
	return NULL;
    }
    if (native_base == NULL) native_base = c->p->native.code_base;
    code_start = *(long*)native_base;
    code_start_offset = code_start - (long)native_base;
    memcpy(new_base, native_base, size);
    c->p->code_base = new_base;
    c->p->cur_ip = (char*)new_base + size;
    c->p->fp = new_base;
    *((char**) new_base) =  (char*)new_base + code_start_offset;
    ia64_branch_link(c);
    ia64_call_link(c);
    ia64_data_link(c);
    c->p->code_base = old_base;
    c->p->cur_ip = (char*)old_base + size;
    c->p->fp = old_base;
    return new_base;
}

extern void
ia64_pset(drisc_ctx c, int type, int junk, int dest, long imm)
{
    ia64_setl(c, dest, imm);
}	

extern void
ia64_setf(drisc_ctx c, int type, int junk, int dest, double imm)
{
    union {
	float f;
	int i;
    } a;
    union {
	double d;
	long l;
    } b;
    if (type == DR_F) {
	a.f = (float) imm;
	ia64_setl(c, R2, a.i);
	/*setf.s */
	MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1e, /*x*/1, R2, dest, 0),
	     nop_m, nop_i);
    } else {
	b.d = (double)imm;
	ia64_setl(c, R2, b.l);
	/*setf.d */
	MMIs(c, FORMAT_M18(/*m*/0, /*x6*/0x1f, /*x*/1, R2, dest, 0),
	     nop_m, nop_i);
    }
}	


#define bit_R(x) (((long)1)<<x)

extern void
ia64_reg_init(drisc_ctx c)
{
    c->p->var_i.init_avail[0] = (/*bit_R(R4)|bit_R(R5)|bit_R(R6)|bit_R(R7)|*/
				 bit_R(IN0)|bit_R(IN1)|bit_R(IN2)|bit_R(IN3)|
				 bit_R(IN4)|bit_R(IN5)|bit_R(IN6)|bit_R(IN7)|
/*				 bit_R(L1)|bit_R(L2)|bit_R(L3)|bit_R(L4)|*/
				 bit_R(L5)|bit_R(L6)|bit_R(L7));

    c->p->var_i.members[0] = c->p->var_i.init_avail[0];
    c->p->tmp_i.init_avail[0] = (bit_R(R15)|bit_R(R16)|bit_R(R17)|
				 bit_R(R18)|bit_R(R19)|bit_R(R20)|bit_R(R21)|
				 bit_R(R22)|bit_R(R23)|bit_R(R24)|bit_R(R25)|
				 bit_R(R26)|bit_R(R27)|bit_R(R28)|bit_R(R29)|
				 bit_R(R30)|bit_R(R31));
    c->p->tmp_i.members[0] = c->p->tmp_i.init_avail[0] | bit_R(R1);
    c->p->var_f.init_avail[0] = (bit_R(F2)|bit_R(F3)|bit_R(F4)|bit_R(F5)|
				 bit_R(F16)|bit_R(F17)|bit_R(F18)|bit_R(F19)|
				 bit_R(F20)|bit_R(F21)|bit_R(F22)|bit_R(F23)|
				 bit_R(F24)|bit_R(F25)|bit_R(F26)|bit_R(F27)|
				 bit_R(F28)|bit_R(F29)|bit_R(F30)|bit_R(F31));
    c->p->var_f.members[0] = c->p->var_f.init_avail[0];
    c->p->tmp_f.init_avail[0] = (0);
    c->p->tmp_f.members[0] = (bit_R(F6)|bit_R(F7)|bit_R(F8)|bit_R(F9)|bit_R(F10)|bit_R(F11)|bit_R(F12)|bit_R(F13)|bit_R(F14)|bit_R(F15));
}

extern void*
gen_ia64_mach_info(c)
drisc_ctx c;
{
    ia64_mach_info smi = malloc(sizeof(*smi));
    if (c->p->mach_info != NULL) {
	free(c->p->mach_info);
	c->p->mach_info = NULL;
	c->p->native.mach_info = NULL;
    }
    ia64_reg_init(c);
    smi->act_rec_size = 0;
    smi->stack_align = 8;
    smi->stack_constant_offset = 0; /* 2047 for ia64v9 */
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
ia64_init_disassembly_info(drisc_ctx c, void * ptr)
{
    struct disassemble_info *i = ptr;
#ifdef INIT_DISASSEMBLE_INFO_THREE_ARG
    INIT_DISASSEMBLE_INFO(*i, stdout, FPRINTF_FUNCTION);
    i->endian = BFD_ENDIAN_LITTLE;
#else
    INIT_DISASSEMBLE_INFO(*i, stdout);
#endif
    i->mach = bfd_mach_ia64_elf64;
    if (c->p->code_base != NULL) {
	i->buffer = (bfd_byte *)c->p->code_base;
	i->buffer_vma = (bfd_vma)c->p->code_base;
    } else {
	i->buffer = (bfd_byte *)c->p->native.code_base;
	i->buffer_vma = (bfd_vma)c->p->native.code_base;
    }
    i->buffer_length = MAXLENGTH;
#ifdef HAVE_PRINT_INSN_IA64
    return 1;
#else
    return 0;
#endif
}

extern int
ia64_print_insn(drisc_ctx c, void *info_ptr, void *insn)
{
#ifdef HAVE_PRINT_INSN_IA64
    long slot[3];
    long len = 0;
    long t0 = bfd_getl64 (insn);
    long t1 = bfd_getl64 ((void*)((char*)insn + 8));
    int template = (t0 >> 1) & 0xf;
    int i = 0;
    unsigned int offset = (char*)insn - (char*)c->p->code_base;
    if (offset >= 32) offset = (char*)insn - (char*)c->p->native.code_base;
    if (offset == 0) {
	printf("\nProcedure prefix, indirect pointer = 0x%lx",
	       *(long*)insn);
	printf("\n                    global pointer = 0x%lx\n",
	       *(((long*)insn) + 1));
	return 16;
    }

    slot[0] = (t0 >>  5) & 0x1ffffffffffLL;
    slot[1] = ((t0 >> 46) & 0x3ffff) | ((t1 & 0x7fffff) << 18);
    slot[2] = (t1 >> 23) & 0x1ffffffffffLL;
    while (len < 16) {
	if (slot[i] != nop_i) {
	    char *real_insn = ((char*)insn) + len;
	    int inc;
	    long insn = slot[2];
	    inc = print_insn_ia64((bfd_vma)real_insn, info_ptr);
	    if (template == 0x2) { /* 64-bit load */ 
		char* value = (char*)(((insn >> 13) & 0x7f) | (((insn >> 27) & 0x1ff) << 7)
		    | (((insn >> 22) & 0x1f) << 16) | (((insn >> 21) & 0x1) << 21)
		    | (slot[1] << 22) | (((insn >> 36) & 0x1) << 63));
		int opcode = slot[2] >> 37;
		if ((opcode == 0x6) &&
		    isprint(((char*)value)[0]) && isprint(((char*)value)[1]) &&
		    isprint(((char*)value)[2]) && isprint(((char*)value)[3]) &&
		    isprint(((char*)value)[4]) && isprint(((char*)value)[5])) {
		    printf("  -> \"%s\"", value);
		}
	    }
	    if (inc == 0) {
	        len += 6;
	    } else {
	        len += inc;
	    }
	} else {
	    len += 6;
	}
	i++;
    }
/*    printf("\n\t slot[0] = %011lx, slot[1] = %011lx, slot[2] = %011lx\n", 
      slot[0], slot[1], slot[2]);*/
    return 16;
#else
    return -1;
#endif
}

extern int
ia64_count_insn(drisc_ctx c, int start, int end)
{
    return (end - start) >> 4;
}
#else
extern int
ia64_count_insn(drisc_ctx c, int start, int end)
{   /* no print insn, just return the buffer length */
    return end - start;
}
extern int
ia64_init_disassembly_info(drisc_ctx c, void * ptr){return 0;}
extern int ia64_print_insn(drisc_ctx c, void *info_ptr, void *insn){return 0;}
#endif

char *char_regs[] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
char *short_regs[] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
char *int_regs[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

extern void
ia64_print_reg(drisc_ctx c, int typ, int reg)
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
	
