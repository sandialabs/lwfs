#include "drisc.h"
#include "drisc_internal.h"
#include "arm5.h"
#include "config.h"
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INSN_OUT(c, insn) do {\
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(int*)c->p->cur_ip = (unsigned int)insn;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = (void*)(((long)c->p->cur_ip)+4);\
} while (0)\

 #define COND(x)	((unsigned)((x)&0xf) << 28)
 #define CLASS(x)	(((x)&0x7) << 25)
 #define OPCODE(x)	(((x)&0xf) << 21) /* opcode field */
 #define S(x)		(((x)&0x1) << 20) /* update cond codes? */
 #define RN(x)		(((x)&0xf) << 16) /* Rn field */
 #define RD(x)		(((x)&0xf) << 12) /* Rd field */
 #define RM(x)		(((x)&0xf) << 0) /* Rm field */
 #define SHIFTI(x,t)	((((x)&0x1f) << 7) | ((t)&0x3)<<5)
 #define SHIFTR(r,t)	((((r)&0xf) << 8) | ((t)&0x3)<<5| 1<<4)

 #define IMM(x,r)	(((x)&0xff) | ((((32-(r))>>1)&0xf)<< 8) | (1<<25)) /* simm8 field */
 #define IM		0x2000
 #define P(x)  (((x)&0x1)<<19)

#define arm_savei(c, imm) arm_dproci(c, SUB, 0, _sp, _sp, ar_size);
#define arm_andi(c, dest, src, imm) arm_dproci(c, AND, 0, dest, src, imm)
#define arm_movi(c, dest, src) arm_dproc(c, MOV, 0, dest, 0, src) 
#define arm_movf(c, dest, src) arm_fproc2(c, 0x1, 0, dest, src)
#define arm_movd(c, dest, src) arm_fproc2(c, 0x1, 1, dest, src)
#define arm_lshi(c, dest, src,imm) arm_dproci(c, MOV, LLshift, dest, src, imm)
#define arm_rshi(c,dest,src,imm) arm_dproci(c, MOV, LRshift, dest, src, imm)
#define arm_rshai(c,dest,src,imm) arm_dproci(c, MOV, ARshift, dest, src, imm)

#define arm_nop(c) arm_movi(c, _r0, _r0)

#define IREG 0
#define FREG 1

#define roundup(a,b) ((a + (b-1)) & (-b))

static void
arm_pldsti(drisc_ctx c, int type, int ls, int dest, int src, long offset);
static void
arm_pldst(drisc_ctx c, int type, int ls, int dest, int src1, int src2);

static void
arm_bswap(drisc_ctx c, int type, int reg)
{
    int shift_ror_16 = (16<<7)|(0x3<<5);
    int shift_ror_8 = (8<<7)|(0x3<<5);
    int shift_lsr_8 = (8<<7)|(0x1<<5);
    switch(type) {
    case DR_L: case DR_UL: case DR_I: case DR_U:
	/* eor	_v1, reg, reg, ror #16 */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(1)|S(0)|RN(reg)|RD(_v1)|RM(reg)|shift_ror_16 );
	/* bic	_v1, _v1, #0x00ff0000 */
	INSN_OUT(c, COND(AL)|CLASS(0x1)|OPCODE(0xe)|S(0)|RN(_v1)|RD(_v1)|0x8ff);
	/* mov	reg, reg, ror #8 */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(0xd)|S(0)|RN(reg)|RD(reg)|RM(reg)|shift_ror_8);
	/* eor	reg, reg, _v1, lsr #8 */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(1)|S(0)|RN(reg)|RD(reg)|RM(_v1)|shift_lsr_8);
	break;
    case DR_US: case DR_S:
        /* and	_v1, reg, #0xff */
	INSN_OUT(c, COND(AL)|CLASS(0x1)|OPCODE(0x0)|S(0)|RN(reg)|RD(_v1)|0xff);
	/* mov	reg, reg, lsr #8 */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(0xd)|S(0)|RN(reg)|RD(reg)|RM(reg)|shift_lsr_8);
	/* orr	reg, reg, _v1, lsl #8 */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(0xc)|S(0)|RN(reg)|RD(reg)|RM(_v1)|shift_lsr_8);
	break;
    case DR_C: case DR_UC:
	/* nothing to do */
	break;
    case DR_D: case DR_F:
	assert(0);
	break;
    }
}

extern void
arm_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    arm_pldsti(c, type, 1, dest, src, offset);
    arm_bswap(c, type, dest);
}


extern void
arm_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    arm_pldst(c, type, 1, dest, src1, src2);
    arm_bswap(c, type, dest);
}

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
    { sizeof(double), 4, FREG},  /* D */
    { 0, 8, IREG}, /* V */
    { -1, 8, IREG}, /* B */
    { 4, 8, IREG}, /* EC */
};

int arm_type_align[] = {
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
	1, /* V */
        4, /* B */
	sizeof(long), /* EC */
};

int arm_type_size[] = {
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
	1, /* V */
        8, /* B */
        sizeof(char*), /* EC */
};

extern void arm_dproc(c, op, shift_code, dest, src1, src2)
drisc_ctx c;
int op;
int shift_code;
int dest;
int src1;
int src2;
{
    int shift = 0;
    if (shift_code != 0) {
	shift_code &= 0x3;
	shift = SHIFTR(src2, shift_code);
	src2 = 0;
    }
    INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(op)|S(0)|RN(src1)|RD(dest)|RM(src2)|shift);
}

extern void arm_dproc2(c, op, fop, dest, src)
drisc_ctx c;
int op;
int fop;
int dest;
int src;
{
    if (op == RSB) {
	arm_dproci(c, RSB, 0, dest, src, 0);
    } else if (op == CMN) {  /* !a */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(CMP)|S(1)|RN(src)|RD(src)|IMM(0,0));
	INSN_OUT(c, COND(NE)|CLASS(0x0)|OPCODE(MOV)|S(0)|RN(0)|RD(dest)|IMM(0, 0));
	INSN_OUT(c, COND(EQ)|CLASS(0x0)|OPCODE(MOV)|S(0)|RN(0)|RD(dest)|IMM(1, 0));
    } else {
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(op)|S(0)|RN(src)|RD(dest)|RM(src));
    }
}

extern void arm_fproc2(c, arm_op, fd, dest, src)
drisc_ctx c;
int arm_op;
int fd;
int dest;
int src;
{
    INSN_OUT(c, COND(AL)|CLASS(0x7)|((arm_op&0x1e)<<19)|(arm_op&0x1)<<15|(dest&0x7)<<12|0x1<<8|(fd&1)<<7|(src&7));
}

extern int
arm_local(drisc_ctx c, int type)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;

    ami->act_rec_size += roundup(type_info[type].size, ami->stack_align);
    return (-ami->act_rec_size)  - 14 * 4 /* int regs to save */ 
	- 8 * 3 * 4 /* float regs to save */;
}

extern int
arm_localb(drisc_ctx c, int size)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    if (size < 0) size = 0;
    ami->act_rec_size = roundup(ami->act_rec_size, size);

    ami->act_rec_size += roundup(size, ami->stack_align);
    return (-ami->act_rec_size) - 14 * 4 /* int regs to save */ 
	- 8 * 3 * 4 /* float regs to save */;
}

extern int arm_local_op(drisc_ctx c, int flag, int val)
{
    int size = val;
    if (flag == 0) {
	size = type_info[val].size;
    }
    if (size < 0) size = 0;
    return arm_localb(c, size);
}	

static int 
is_temp(int ireg)
{
    return (ireg <= _r4);  /* higher regs are saved by the callee */
}

static int 
is_ftemp(int freg)
{
    return (freg <= _f4);  /* higher regs are saved by the callee */
}

extern void
arm_save_restore_op(drisc_ctx c, int save_restore, int type, int reg)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    if (save_restore == 0) { /* save */
	switch (type) {
	case DR_D: case DR_F:
	    if (is_ftemp(reg)) {
		arm_pstorei(c, type, 0, reg, _fp, - 13*4 - reg * 12);
	    }
	    break;
	default:
	    if (is_temp(reg)) {
		arm_pstorei(c, type, 0, reg, _sp, ami->gp_save_offset + (reg - _r0) * ami->stack_align);
	    }
	    break;
	}
    } else {  /* restore */
	switch (type) {
	case DR_D: case DR_F:
	    if (is_ftemp(reg)) {
		arm_ploadi(c, type, 0, reg, _fp, -13*4 - reg * 12);
	    }
	    break;
	default:
	    if (is_temp(reg)) {
		arm_ploadi(c, type, 0, reg, _sp, ami->gp_save_offset + (reg - _r0) * ami->stack_align);
	    }
	    break;
	}
    }
}	

static void
arm_movi2f(drisc_ctx c, int dest, int src)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    arm_pstorei(c, DR_I, 0, src, _fp, ami->conversion_word);
    arm_ploadi(c, DR_F, 0, dest, _fp, ami->conversion_word);
}
    
static void
arm_movf2i(drisc_ctx c, int dest, int src)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    arm_pstorei(c, DR_F, 0, src, _fp, ami->conversion_word);
    arm_ploadi(c, DR_I, 0, dest, _fp, ami->conversion_word);
}
    
static void
arm_movd2i(drisc_ctx c, int dest, int src)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    arm_pstorei(c, DR_D, 0, src, _fp, ami->conversion_word);
    if (ami->stack_align == 8) {
	arm_ploadi(c, DR_L, 0, dest, _fp, ami->conversion_word);
    } else {
	arm_ploadi(c, DR_I, 0, dest, _fp, ami->conversion_word);
	arm_ploadi(c, DR_I, 0, dest+1, _fp, ami->conversion_word+4);
    }
}
    
static void
arm_movi2d(drisc_ctx c, int dest, int src)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    if (ami->stack_align == 8) {
	arm_pstorei(c, DR_L, 0, src, _fp, ami->conversion_word);
    } else {
	arm_pstorei(c, DR_I, 0, src, _fp, ami->conversion_word);
	arm_pstorei(c, DR_I, 0, src+1, _fp, ami->conversion_word+4);
    }
    arm_ploadi(c, DR_D, 0, dest, _fp, ami->conversion_word);
}
    
extern void arm_fproc(c, arm_op, fd, dest, src1, src2)
drisc_ctx c;
int arm_op;
int fd;
int dest;
int src1;
int src2;
{
    INSN_OUT(c, COND(AL)|CLASS(0x7)|((arm_op&0x1e)<<19)|(arm_op&0x1)<<15|(src1&0x7)<<16|(dest&0x7)<<12|0x1<<8|(fd&1)<<7|(src2&0x7));
}

extern void arm_dproci(c, op, shift_code, dest, src1, imm)
drisc_ctx c;
int op;
int shift_code;
int dest;
int src1;
long imm;
{
    int shift = 0;
    int setcc = 0;
    if (op == CMP) setcc = 1;
    if (shift_code != 0) {
	/* must already be a mov op */
	shift_code &= 0x3;
	shift = SHIFTI(imm, shift_code);
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(op)|S(0)|RN(src1)|RD(dest)|shift|RM(src1));
	return;
    }
    if ((imm >= 0) && (imm < 256)) {
	/* arith format */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(op)|S(setcc)|RN(src1)|RD(dest)|IMM(imm, 0));
    } else {
	arm_set(c, _v1, imm);
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(op)|S(setcc)|RN(src1)|RD(dest)|RM(_v1));
    }
}

/*
 *	ARM stack frame organization
 *	
 *		pushed args
 *			------------  SP value at entry    FP value
 *		callee-saved int regs 
 *		pushed with stmdb	space for 14
 *		callee-saved float regs space for 8
 *
 *	 		------------  SP value after STMDB
 *		local variables
 *			------------  final SP value
 */

extern void
arm_proc_start(drisc_ctx c, char *subr_name, int arg_count, arg_info_list args,
	     dr_reg_t *arglist)
{
    int i;
    int max_in_reg = _a4;
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    int cur_arg_offset = 0;
    /* emit start insns */
    INSN_OUT(c, 0xFF000000);
    INSN_OUT(c, 0xFF000000);
    INSN_OUT(c, 0xFF000000);
    arm_movi(c, _r12, _sp);
    /* stmdb sp!, {r11, r12, lr, pc} */
    INSN_OUT(c, COND(AL)|CLASS(4)|1<<24/*p*/|RN(_sp)|1<<_r11|1<<_r12|1<<_link|1<<_pc);
    arm_dproci(c, SUB, 0, _sp, _sp, 14*4 + 8 *3*4); /* instead of write back */
    arm_nop(c);  /* placeholder for float save */
    arm_dproci(c, SUB, 0, _r11, _r12, 4);
    ami->save_insn_offset = (long)c->p->cur_ip - (long)c->p->code_base;
    arm_nop(c);	/* room for largest stack adjust insn, 5 nops */
    arm_nop(c);
    arm_nop(c);
    arm_nop(c);
    arm_nop(c);
    ami->conversion_word = arm_local(c, DR_D);
    ami->conversion_word = arm_local(c, DR_D);
    ami->conversion_word = arm_local(c, DR_D);

    /* load params from regs */
    for (i = 0; i < arg_count; i++) {
	switch (args[i].type) {
	case DR_F: case DR_D:
	    /* falling through */
	default:
	    if (cur_arg_offset < 4 * ami->stack_align) {
		args[i].is_register = 1;
		args[i].in_reg = _r0 + cur_arg_offset/ami->stack_align;
		args[i].out_reg = _r0 + cur_arg_offset/ami->stack_align;
		max_in_reg = args[i].in_reg;
	    } else {
		args[i].is_register = 0;
	    }
	    break;
	}
	args[i].offset = cur_arg_offset;
	cur_arg_offset += roundup(type_info[(int)args[i].type].size, ami->stack_align);
    }
    
    for (i = 0; i < arg_count; i++) {
	int tmp_reg;
	if (i < 4) {
	    /* only first 4 moved into registers */
	    if (!dr_getreg(c, &tmp_reg, args[i].type, DR_VAR)) {
		/* not enough regs for this, store it to the stack */
		int real_offset = - args[i].offset - 4*4; 
		if (arglist != NULL) arglist[i] = -1;
		arm_pstorei(c, DR_I, 0, args[i].in_reg, _fp, 
				    real_offset);
		args[i].in_reg = -1;
		args[i].out_reg = -1;
		args[i].offset = real_offset;
		args[i].is_register = 0;
		continue;
	    }
	    if (args[i].is_register) {
		if ((args[i].type != DR_F) && (args[i].type != DR_D)) {
		    arm_movi(c, tmp_reg, args[i].in_reg);
		} else if (args[i].type == DR_F) {	    /* must be float */
		    arm_movi2f(c, tmp_reg, args[i].in_reg);
		} else {
		    /* arm boundary condition, half in register */
		    if (args[i].offset == 3*4) {
			int real_offset = args[i].offset + 68; 
			arm_pstorei(c, DR_I, 0, args[i].in_reg, _fp, 
				    real_offset);
			arm_ploadi(c, DR_F, 0, tmp_reg, _fp, real_offset);
			arm_ploadi(c, DR_F, 0, tmp_reg+1, _fp, real_offset+4);
		    } else {
			arm_movi2d(c, tmp_reg, args[i].in_reg);
		    }
		}
	    } else {
		/* general offset from fp*/
		int real_offset = args[i].offset - 3*4; 
		arm_ploadi(c, args[i].type, 0, tmp_reg, _fp, real_offset);
	    }
	    if (arglist != NULL) arglist[i] = tmp_reg;
	    args[i].in_reg = tmp_reg;
	    args[i].is_register = 1;
	} else {
	    /* leave it on the stack */
	    int real_offset = args[i].offset - 3*4; 
	    if (arglist != NULL) arglist[i] = -1;
	    args[i].in_reg = -1;
	    args[i].out_reg = -1;
	    args[i].offset = real_offset;
	}
    }
}


extern void
arm_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    arm_pldsti(c, type, 1, dest, src, offset);
}

extern void
arm_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    arm_pldst(c, type, 1, dest, src1, src2);
}

/* byte and whole word version */
#define ARM_LDSTI(c,u,b,ls,rn,rd,offset) INSN_OUT(c, COND(AL)|CLASS(2)|(1<<24)|((u&1)<<23)|((b&1)<<22)|(ls&1)<<20|RN(rn)|RD(rd)|(0x7ff&offset))

/* halfword version */
#define ARM_LDSTHI(c,u,ls,rn,rd,sh,offset) INSN_OUT(c, COND(AL)|CLASS(0)|(1<<24)|((u&1)<<23)|(1<<22)|(ls&1)<<20|RN(rn)|RD(rd)|(1<<7)|((sh&0x3)<<5)|(1<<4)|(0xf&offset)|((offset&0xf0)<<4))

/* float version */
#define ARM_LDSTFI(c,u,fd,ls,rn,rd,offset) INSN_OUT(c, COND(AL)|CLASS(6)|(1<<24)|((u&1)<<23)|(ls&1)<<20|RN(rn)|(fd&1)<<15|RD(rd)|(0x1<<8)|(0xff&(offset>>2)))

extern void
arm_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    arm_pldsti(c, type, 0, dest, src, offset);
}

static void
arm_pldsti(drisc_ctx c, int type, int ls, int dest, int src, long offset)
{
    int u = 1;
    int max_offset;

    switch (type) {
    case DR_S: case DR_US: case DR_D: case DR_F:
	max_offset = 256;
	break;
    default:
	max_offset = 2048;
	break;
    }
    if  (((long)offset) >= max_offset || ((long)offset) < -max_offset) {
	arm_set(c, _v1, offset);
	arm_pldst(c, type, ls, dest, src, _v1);
	return;
    }
    if (offset < 0) {
	u = 0;
	offset = -offset;
    }

    switch (type) {
    case DR_F:
	ARM_LDSTFI(c, u, 0, ls, src, dest, offset);
	break;
    case DR_D:
	ARM_LDSTFI(c, u, 1, ls, src, dest, offset);
	break;
    case DR_C:
    case DR_UC:
	ARM_LDSTI(c, u, 1, ls, src, dest, offset);
	break;
    case DR_I:
    case DR_U:
    case DR_L:
    case DR_UL:
    case DR_P:
	ARM_LDSTI(c, u, 0, ls, src, dest, offset);
	break;
    case DR_S:
	if (ls == 1) { /* this is a load */
	    ARM_LDSTHI(c,u,ls,src,dest,0x3,offset);
	    break;
	}
	/* fall through */
    case DR_US:
	ARM_LDSTHI(c,u,ls,src,dest,0x1,offset);
	break;
    default:
	break;
    }
}
#define ARM_LDST(c,u,b,ls,rn,rd,rm) INSN_OUT(c, COND(AL)|CLASS(3)|(1<<24)|((u&1)<<23)|((b&1)<<22)|(ls&1)<<20|RN(rn)|RD(rd)|(0xf&rm))
#define ARM_LDSTH(c,u,ls,rn,rd,sh,rm) INSN_OUT(c, COND(AL)|CLASS(0)|(1<<24)|((u&1)<<23)|(ls&1)<<20|RN(rn)|RD(rd)|(1<<7)|((sh&0x3)<<5)|(1<<4)|(0xf&rm))

extern void
arm_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    arm_pldst(c, type, 0, dest, src1, src2);
}

static void
arm_pldst(drisc_ctx c, int type, int ls, int dest, int src1, int src2)
{
    switch (type) {
    case DR_F:
	arm_dproc(c, ADD, 0, _v1, src1, src2);
	ARM_LDSTFI(c, 0, 0, ls, _v1, dest, 0);
	break;
    case DR_D:
	arm_dproc(c, ADD, 0, _v1, src1, src2);
	ARM_LDSTFI(c, 0, 1, ls, _v1, dest, 0);
	break;
    case DR_L: case DR_UL: case DR_P: case DR_I: case DR_U: case DR_EC:
	ARM_LDST(c,1,0,ls,src1,dest,src2);
	break;
    case DR_S:
	if (ls == 1) { /* this is a load */
	    ARM_LDSTH(c,1,ls,src1,dest,0x3,src2);
	    break;
	}
	/* fall through */
    case DR_US:
	ARM_LDSTH(c,1,ls,src1,dest,0x1,src2);
	break;
    case DR_C: case DR_UC:
	ARM_LDST(c,1,1,ls,src1,dest,src2);
	break;
    default:
	break;
    }
}

static int drisc_hidden_modi(int a, int b)
{ return a % b; }
static long drisc_hidden_mod(long a, long b)
{ return a % b; }
static unsigned long drisc_hidden_umod(unsigned long a, unsigned long b)
{ return a % b; }
static unsigned int drisc_hidden_umodi(unsigned int a, unsigned int b)
{ return a % b; }
static double drisc_hidden_ultod(unsigned long a)
{ return (double) a; }
static float drisc_hidden_ultof(unsigned long a)
{ return (float) a; }
static unsigned long drisc_hidden_dtoul(double a)
{ return (unsigned long) a; }
static unsigned int drisc_hidden_dtou(double a)
{ return (unsigned int) a; }
static unsigned long drisc_hidden_ftoul(float a)
{ return (unsigned long) a; }
static unsigned int drisc_hidden_ftou(float a)
{ return (unsigned int) a; }

extern void arm_mod(drisc_ctx c, int sign, int type_long, int dest, 
		      int src1, int src2)
{
    int return_reg;
    if (sign == 1) {
	/* signed case */
	if (type_long) {
	    return_reg = dr_scalll(c, (void*)drisc_hidden_mod, "%l%l", src1, src2);
	    dr_movl(c, dest, return_reg);
	} else {
	    return_reg = dr_scalli(c, (void*)drisc_hidden_modi, "%i%i", src1, src2);
	    dr_movi(c, dest, return_reg);
	}
    } else {
	/* unsigned case */
	if (type_long) {
	    return_reg = dr_scalll(c, (void*)drisc_hidden_umod, "%l%l", src1, src2);
	    dr_movul(c, dest, return_reg);
	} else {
	    return_reg = dr_scallu(c, (void*)drisc_hidden_umodi, "%u%u", src1, src2);
	    dr_movu(c, dest, return_reg);
	}
    }
}

extern void arm_modi(drisc_ctx c, int data1, int data2, int dest, int src1, 
		      long imm)
{
    arm_set(c, _v1, imm);
    arm_mod(c, data1, data2, dest, src1, _v1);
}

static unsigned long drisc_hidden_udiv(unsigned long a, unsigned long b)
{ return a / b; }
static long drisc_hidden_div(long a, long b)
{ return a / b; }

extern void arm_div(drisc_ctx c, int unsign, int junk, int dest, int src1,
		      int src2)
{
    int return_reg;
    void *routine = (void*) &drisc_hidden_div;
    if (unsign) routine = (void*) &drisc_hidden_udiv;

    return_reg = dr_scalll(c, routine, "%l%l", src1, src2);
    dr_movl(c, dest, return_reg);
}

#define MUL(c,A,S,Rd,Rs,Rm) INSN_OUT(c, COND(AL)|(A&1)<<21|(S&1)<<20|RN(Rd)|RD(0)|(Rs&0xf)<<8|0x90|(Rm&0xf))

extern void arm_mul(drisc_ctx c, int unsign, int junk, int dest, int src1,
		      int src2)
{
    MUL(c, 0, 0, dest, src1, src2);
}

extern void arm_muli(drisc_ctx c, int unsign, int junk, int dest, int src,
		      long imm)
{
    arm_set(c, _v1, imm);
    MUL(c, 0, 0, dest, src, _v1);
}

extern void arm_divi(drisc_ctx c, int unsign, int junk, int dest, int src, 
		      long imm)
{
    arm_set(c, _v1, imm);
    arm_div(c, unsign, junk, dest, src,	_v1);
}

extern void
arm_mov(drisc_ctx c, int type, int junk, int dest, int src)
{
    if (src == dest) return;
    switch(type) {
    case DR_D:
	arm_movd(c, dest, src);
	break;
    case DR_F:
	arm_movf(c, dest, src);
	break;
    default:
	arm_movi(c, dest, src);
    }
}


static void
arm_saverestore_floats(drisc_ctx c, int saverestore)
{
    int i;
    for (i=1; i <8; i++) {
	if (drisc_mustsave(&c->p->tmp_f, i)) {
	    arm_save_restore_op(c, saverestore, DR_D, i);
	}
    }
}

#define CONV(x,y) ((x*100)+y)
extern void
arm_convert(drisc_ctx c, int from_type, int to_type, 
	      int dest, int src)
{
    from_type &= 0xf;
    to_type &= 0xf;
    switch(CONV(from_type, to_type)) {
    case CONV(DR_I, DR_L):
    case CONV(DR_I, DR_U):
    case CONV(DR_I,DR_UL):
    case CONV(DR_UL,DR_I):
    case CONV(DR_UL,DR_U):
    case CONV(DR_L,DR_U):
    case CONV(DR_U,DR_UL):
    case CONV(DR_U,DR_L):
    case CONV(DR_L,DR_I):
    case CONV(DR_UL,DR_L):
    case CONV(DR_L,DR_UL):
    case CONV(DR_P,DR_UL):
    case CONV(DR_UL,DR_P):
    case CONV(DR_U,DR_I):
	if(src == dest) return;
	arm_movi(c, dest,src);
	break;
    case CONV(DR_F,DR_D):
	arm_movd(c, dest, src);
	break;
    case CONV(DR_F,DR_L):
    case CONV(DR_F,DR_I):
	INSN_OUT(c, COND(AL)|CLASS(7)|1<<20|(dest&0xf)<<12|1<<8|7<<4|(src&0x7));/*fixzs*/
	break;
    case CONV(DR_F,DR_U):
        {
	    int ret;
	    arm_saverestore_floats(c, 0);
	    ret = dr_scallu(c, (void*)drisc_hidden_ftou, "%f", src);
	    arm_saverestore_floats(c, 1);
	    arm_mov(c, DR_UL, 0, dest, ret);
	}
	break;
	/* fallthrough */
    case CONV(DR_F,DR_UL):
        {
	    int ret;
	    arm_saverestore_floats(c, 0);
	    ret = dr_scallul(c, (void*)drisc_hidden_ftoul, "%f", src);
	    arm_saverestore_floats(c, 1);
	    arm_mov(c, DR_UL, 0, dest, ret);
	}
	break;
    case CONV(DR_D,DR_F):
	arm_movf(c, dest, src);
	break;
    case CONV(DR_D,DR_L):
    case CONV(DR_D,DR_I):
	INSN_OUT(c, COND(AL)|CLASS(7)|1<<20|(dest&0xf)<<12|1<<8|0xf<<4|(src&0x7));/*fixzd*/
	break;
    case CONV(DR_D,DR_U):
        {
	    int ret;
	    arm_saverestore_floats(c, 0);
	    ret = dr_scallu(c, (void*)drisc_hidden_dtou, "%d", src);
	    arm_saverestore_floats(c, 1);
	    arm_mov(c, DR_U, 0, dest, ret);
	}
	break;
    case CONV(DR_D,DR_UL):
        {
	    int ret;
	    arm_saverestore_floats(c, 0);
	    ret = dr_scallul(c, (void*)drisc_hidden_dtoul, "%d", src);
	    arm_saverestore_floats(c, 1);
	    arm_mov(c, DR_UL, 0, dest, ret);
	}
	break;
    case CONV(DR_I,DR_D):
    case CONV(DR_L,DR_D):
	INSN_OUT(c, COND(AL)|CLASS(7)|0<<20|(dest&0x7)<<16|(src&0xf)<<12|1<<8|9<<4);/*fltd*/
	break;
    case CONV(DR_U,DR_D):
    case CONV(DR_UL,DR_D): 
        {
	    int ret;
	    arm_saverestore_floats(c, 0);
	    ret = dr_scalld(c, (void*)drisc_hidden_ultod, "%l", src);
	    arm_saverestore_floats(c, 1);
	    arm_mov(c, DR_D, 0, dest, ret);
	}
	break;
    case CONV(DR_I,DR_F):
    case CONV(DR_L,DR_F):
	INSN_OUT(c, COND(AL)|CLASS(7)|0<<20|(dest&0x7)<<16|(src&0xf)<<12|1<<8|1<<4);/*flts*/
	break;
    case CONV(DR_U,DR_F):
    case CONV(DR_UL,DR_F):
        {
	    int ret;
	    arm_saverestore_floats(c, 0);
	    ret = dr_scallf(c, (void*)drisc_hidden_ultof, "%l", src);
	    arm_saverestore_floats(c, 1);
	    arm_mov(c, DR_D, 0, dest, ret);
	}
	break;
    case CONV(DR_C,DR_UL):
    case CONV(DR_C,DR_L):
    case CONV(DR_C,DR_I):
    case CONV(DR_C,DR_U):
	arm_lshi(c, dest, src, 24);
	arm_rshai(c, dest, dest, 24);
	break;
    case CONV(DR_I, DR_C):
    case CONV(DR_U, DR_C):
    case CONV(DR_L, DR_C):
    case CONV(DR_UL, DR_C):
	arm_andi(c, dest, src, 0xff);
	break;
    case CONV(DR_S,DR_L):
    case CONV(DR_S,DR_UL):
    case CONV(DR_S,DR_I):
    case CONV(DR_S,DR_U):
	arm_lshi(c, dest, src, 16);
	arm_rshai(c, dest, dest, 16);
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
	arm_lshi(c, dest, src, 16);
	arm_rshi(c, dest, dest, 16);
	break;
    default:
	printf("Unknown case in arm convert %d\n", CONV(from_type,to_type));
    }
}

static signed char op_conds[] = {
    EQ, /* dr_beq_code */  /* signed */
    GE, /* dr_bge_code */
    GT, /* dr_bgt_code */
    LE, /* dr_ble_code */
    LT, /* dr_blt_code */
    NE, /* dr_bne_code */

    EQ, /* dr_beq_code */  /* unsigned */
    CS, /* dr_bge_code */
    HI, /* dr_bgt_code */ 
    LS, /* dr_ble_code */
    CC, /* dr_blt_code */
    NE, /* dr_bne_code */
};

#define CMF 0x4
extern void
arm_branch(drisc_ctx c, int op, int type, int src1, int src2, int label)
{
    switch(type) {
    case DR_D:
    case DR_F:
	INSN_OUT(c, COND(AL)|CLASS(7)|CMF<<21|1<<20|(src1&0x7)<<16|0xf11<<4|(src2&0x7));
	dr_mark_branch_location(c, label);
	INSN_OUT(c, COND(op_conds[op])|CLASS(0x5)|/*disp */0);/* b*/
	break;
	break;
    case DR_U:
    case DR_UL:
	op += 6; /* second set of codes */
	/* fall through */
    default:
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(CMP)|S(1)|RN(src1)|RD(0)|RM(src2));
	dr_mark_branch_location(c, label);
	INSN_OUT(c, COND(op_conds[op])|CLASS(0x5)|/*disp */0);/* b*/
    }
    /*    arm_nop(c);*/
}

extern void 
arm_jal(drisc_ctx c, int return_addr_reg, int target)
{

}

extern void 
arm_jump_to_label(drisc_ctx c, unsigned long label)
{
    dr_mark_branch_location(c, label);
    INSN_OUT(c, COND(AL)|CLASS(5)|(1<<24)/*link*/);
}

extern void arm_jump_to_reg(drisc_ctx c, unsigned long reg)
{
    arm_dproc(c, MOV, 0, _link, _pc, _pc);
    arm_dproc(c, MOV, 0, _pc, reg, reg);
}

extern void arm_jump_to_imm(drisc_ctx c, unsigned long imm)
{

}

static void internal_push(drisc_ctx c, int type, int immediate, 
			  void *value_ptr)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
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
	
    if (ami->cur_arg_offset < 4 * ami->stack_align) {
	arg.is_register = 1;
	arg.in_reg = _a1 + ami->cur_arg_offset/ami->stack_align;
	arg.out_reg = _a1 + ami->cur_arg_offset/ami->stack_align;
    } else {
	arg.is_register = 0;
    }

    arg.offset = ami->cur_arg_offset;
    ami->cur_arg_offset += 
	roundup(type_info[(int)arg.type].size, ami->stack_align);
    real_offset = arg.offset - 4*4; /* first 16 bytes in regs */

    if (ami->cur_arg_offset > ami->max_arg_size) {
	ami->max_arg_size = ami->cur_arg_offset;
    }
    if (arg.is_register == 0) {
	/* store it on the stack only */
	if (arg.is_immediate) {
	    if (type != DR_D) {
		if (type == DR_F) {
		    union {
		      float f;
		      int i;
		    } u;
		    u.f = (float) *(double*)value_ptr;
		    arm_set(c, _v1, u.i);
		} else {
		    arm_set(c, _v1, *(long*)value_ptr);
		}
		arm_pstorei(c, arg.type, 0, _v1, _sp, real_offset);
	    } else {
		arm_set(c, _v1, *(int*)value_ptr);
		arm_pstorei(c, DR_I, 0, _v1, _sp, real_offset);
		arm_set(c, _v1, *(((int*)value_ptr)+1));
		arm_pstorei(c, DR_I, 0, _v1, _sp, real_offset+4);
	    }		
	} else {
	    arm_pstorei(c, arg.type, 0, *(int*)value_ptr, _sp, real_offset);
	}
    } else {
	if ((type != DR_F) && (type != DR_D)) {
	    if (arg.is_immediate) {
		arm_set(c, arg.out_reg, *(long*)value_ptr);
	    } else {
		arm_mov(c, type, 0, arg.out_reg, *(int*) value_ptr);
	    }
	} else {
	    if (arg.is_immediate) {
		if (type == DR_F) {
		    union {
		      float f;
		      int i;
		    }u;
		    u.f = *(double*)value_ptr;
		    arm_set(c, arg.out_reg, u.i);
		} else {
		    arm_set(c, arg.out_reg, *(int*)value_ptr);
		    if (arg.out_reg != _a4) {
			arm_set(c, arg.out_reg+1, *(((int*)value_ptr)+1));
		    } else {
			/* boundary condition */
			arm_set(c, _v1, *(((int*)value_ptr)+1));
			arm_pstorei(c, DR_I, 0, _v1, _sp, real_offset + 4);
		    }
		}
	    } else {
		if (type == DR_F) {
		    arm_movf2i(c, arg.out_reg, *(int*)value_ptr);
		} else {
		    if (arg.out_reg != _a4) {
			arm_movd2i(c, arg.out_reg, *(int*)value_ptr);
		    } else {
			/* boundary condition */
			arm_movf2i(c, arg.out_reg, *(int*)value_ptr);
			arm_pstorei(c, DR_F, 0, (*(int*)value_ptr)+1, _sp, 
				    real_offset + 4);
		    }
		}
	    }
	}
    }
}

static void push_init(drisc_ctx c)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    ami->cur_arg_offset = 0;
}

extern void arm_push(drisc_ctx c, int type, int reg)
{
    if ((type == DR_V) && (reg == -1)) {
	push_init(c);
    } else {
	internal_push(c, type, 0, &reg);
    }
}

extern void arm_pushi(drisc_ctx c, int type, long value)
{
    internal_push(c, type, 1, &value);
}

extern void arm_pushfi(drisc_ctx c, int type, double value)
{
    internal_push(c, type, 1, &value);
}

extern int arm_calli(drisc_ctx c, int type, void *xfer_address)
{
    int caller_side_ret_reg = _a1;

    /* save temporary registers */
    dr_mark_call_location(c, NULL, xfer_address);
    INSN_OUT(c, COND(AL)|CLASS(5)|(1<<24)/*link*/);
    /*    arm_nop(c);*/
    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
	caller_side_ret_reg = _f0;
    }
    push_init(c);
    return caller_side_ret_reg;
}

extern int arm_callr(drisc_ctx c, int type, int src)
{
    int caller_side_ret_reg = _a1;

    arm_dproc(c, MOV, 0, _link, _pc, _pc);
    arm_dproc(c, MOV, 0, _pc, src, src);

    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
	caller_side_ret_reg = _f0;
    }
    push_init(c);
    return caller_side_ret_reg;
}

extern void
arm_branchi(drisc_ctx c, int op, int type, int src, long imm, int label)
{
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen\n");
	break;
    case DR_U:
    case DR_UL:
	op += 6; /* second set of codes */
	/* fall through */
    default:
	arm_dproci(c, CMP, 0, 0/*dest*/, src, imm);
	dr_mark_branch_location(c, label);
	INSN_OUT(c, COND(op_conds[op])|CLASS(0x5)|/*disp */0);/* b*/
    }
}


static void
arm_simple_ret(drisc_ctx c)
{
    dr_mark_ret_location(c);
    INSN_OUT(c, COND(AL)|CLASS(4)|1<<24/*p*/|1<<20/*l*/|RN(_r11)|1<<_r11|1<<_sp|1<<_pc);
    arm_nop(c);  /* ldmea may slide back here if we have to restore floats */
}

extern void arm_ret(drisc_ctx c, int data1, int data2, int src)
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
	if (src != _a1) arm_movi(c, _a1, src);
	break;
    case DR_F:
	if (src != _f0) arm_movf(c, _f0, src);
	break;
    case DR_D:
	if (src != _f0) arm_movd(c, _f0, src);
	break;
    }
    arm_simple_ret(c);
}

extern void arm_reti(drisc_ctx c, int data1, int data2, long imm)
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
	arm_set(c, _a1, imm);
	break;
    case DR_F:
    case DR_D:
	break;/* no return immediate of floats */
    }
    arm_simple_ret(c);
}

static void
arm_data_link(drisc_ctx c)
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
arm_branch_link(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    int i;

    for(i=0; i< t->branch_count; i++) {
	int label = t->branch_locs[i].label;
	int label_offset = t->label_locs[label] - t->branch_locs[i].loc;
	int *branch_addr = (int*)((char *)c->p->code_base + 
				  t->branch_locs[i].loc);
	/* compensate for arm PC lookahead */
	label_offset = label_offset - 8;
        /* div addr diff by 4 for arm offset value */
	label_offset = label_offset >> 2;  
	*branch_addr &= 0xff000000;
	*branch_addr |= (label_offset & 0xffffff);
    }
}

/*
 * on ARM, we need a procedure linkage table to manage 
 * calls to DLLs in an address range that is typically more than 26 bits
 * away from malloc'd memory.  We emit a PLT that is basically a set_reg, 
 * then jump_through_reg for each routine.  Later, during call linkage, 
 * we'll call to the PLT entry rather than directly to the routine.
 */
static void
arm_PLT_emit(drisc_ctx c)
{
    call_t *t = &c->p->call_table;
    int i;

    for(i=0; i< t->call_count; i++) {
	int *call_addr = (int*) ((unsigned long)c->p->code_base + 
				 t->call_locs[i].loc);
	long call_offset = (unsigned long)t->call_locs[i].xfer_addr - 
	    (unsigned long)call_addr;
        /* div addr diff by 4 for arm offset value */
	call_offset = call_offset >> 2;
	call_offset = call_offset >> 24;
	if ((call_offset != 0) && (call_offset != -1)) {
	    t->call_locs[i].mach_info = (void*)
		((long)c->p->cur_ip - (long)c->p->code_base);
	    arm_set(c, _v1, (unsigned long)t->call_locs[i].xfer_addr);
	    arm_dproc(c, MOV, 0, _pc, _v1, _v1);
	    /*	    arm_nop(c);*/
	}
    }
}

static void
arm_call_link(drisc_ctx c)
{
    call_t *t = &c->p->call_table;
    int i;

    for(i=0; i< t->call_count; i++) {
	int *call_addr = (int*) ((unsigned long)c->p->code_base + 
				 t->call_locs[i].loc);
	if (t->call_locs[i].mach_info == NULL) {
	    /* no PLT */
	    int call_offset = (unsigned long)t->call_locs[i].xfer_addr -
		(unsigned long)call_addr;
	
	    /* compensate for arm PC lookahead */
	    call_offset = call_offset - 8;
	    /* div addr diff by 4 for arm offset value */
	    call_offset = call_offset >> 2;
	    *call_addr &= 0xff000000;
	    *call_addr |= (call_offset & 0xffffff);
	} else {
	    /* call through PLT */
	    unsigned long PLT_addr = (unsigned long)c->p->code_base + 
				      (unsigned long)t->call_locs[i].mach_info;
	    int call_offset = PLT_addr - (unsigned long)call_addr;
	    
	    /* compensate for arm PC lookahead */
	    call_offset = call_offset - 8;
	    call_offset = call_offset >> 2;
	    *call_addr &= 0xff000000;
	    *call_addr |= (call_offset & 0x00ffffff);
	}
    }
}


/* Clear the instruction cache from `beg' to `end'.  This makes an
   inline system call to SYS_cacheflush.  */
#define CLEAR_INSN_CACHE(BEG, END)                                      \
{                                                                       \
  register unsigned long _beg __asm ("a1") = (unsigned long) (BEG);     \
  register unsigned long _end __asm ("a2") = (unsigned long) (END);     \
  register unsigned long _flg __asm ("a3") = 0;                         \
  __asm __volatile ("swi 0x9f0002               @ sys_cacheflush"       \
                    : "=r" (_beg)                                       \
                    : "0" (_beg), "r" (_end), "r" (_flg));              \
}
/*
 *  Cache flush code grabbed from a Dec 1999 posting on libc-hacker 
 *  mailing list
 */
static void
arm_flush(void *base, void *limit)
{
#ifdef HOST_ARM
    CLEAR_INSN_CACHE(base, limit);
#endif
}    

static void
arm_emit_save(drisc_ctx c)
{
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    void *save_ip = c->p->cur_ip;
    int ar_size = ami->act_rec_size + ami->max_arg_size;
    int float_count = 0;
    int int_count = 3;  /* fp, ip, lr */

    int reg;
    int mask = 0;
    ret_t *t = &c->p->ret_table;
    int i;

    ar_size += 14 * 4 /* int save */ + 8 * 3 * 4 /* float save */;
    
    ar_size = roundup(ar_size, 8);
    
    switch(ami->max_arg_size) {
    case 0: case 4:
	mask |= 1<<_a2;
    case 8:
	mask |= 1<<_a3;
    case 12:
	mask |= 1<<_a4;
    default:
	/* save nothing */
	break;
    }
    mask |= 1<< _v1;
    for (reg = _v2; reg <= _v7; reg++) {
	if (drisc_wasused(&c->p->tmp_i, reg)) {
	    mask |= (1<<reg);
	    int_count++;
	}
    }
    for (reg = _f4; reg <= _f7; reg++) {
	if (drisc_wasused(&c->p->tmp_f, reg)) {
	    float_count = reg - _f4 + 1;
	}
    }
    c->p->cur_ip = (char*)c->p->code_base + ami->save_insn_offset - 16;
    INSN_OUT(c, COND(AL)|CLASS(4)|1<<24/*p*/|RN(_sp)| mask|1<<_r11|1<<_r12|1<<_link|1<<_pc;);
    c->p->cur_ip = ((char*)c->p->cur_ip) + 4; /* skip sub of sp*/
    if (float_count > 0) {
	int n1 = (float_count & 0x2) >> 1;
	int n0 = (float_count &0x1);
	INSN_OUT(c, COND(AL)|CLASS(6)|1<<24|n1<<22|1<<21|RN(_sp)|n0<<15|_f4<<12|0x2<<8|0x6); /*sfm*/
    } else {
	arm_nop(c);
    }
    c->p->cur_ip = (char*)c->p->code_base + ami->save_insn_offset;
    arm_savei(c, -ar_size);

    for(i=0; i< t->ret_count; i++) {
	c->p->cur_ip = (int*)((char *)c->p->code_base + t->ret_locs[i]);
	if (float_count > 0) {
	    int n1 = (float_count & 0x2) >> 1;
	    int n0 = (float_count &0x1);
	    int offset = 4 * 12 + 14*4 - 4;
	    INSN_OUT(c, COND(AL)|CLASS(6)|1<<24|n1<<22|1<<20|RN(_r11)|n0<<15|_f4<<12|0x2<<8|offset>>2);  /* lfm */
	}
        INSN_OUT(c, COND(AL)|CLASS(4)|1<<24/*p*/|1<<20/*l*/|RN(_r11)|1<<_r11|1<<_sp|1<<_pc|mask);
    }
    c->p->fp = (char*)c->p->code_base + 12; /* skip 3 swinv */
    c->p->cur_ip = save_ip;
}
    
extern void
arm_end(c)
drisc_ctx c;
{
    arm_nop(c);
    arm_simple_ret(c);
    arm_PLT_emit(c);   /* must be done before linking */
    arm_branch_link(c);
    arm_call_link(c);
    arm_data_link(c);
    arm_emit_save(c);
    arm_flush(c->p->code_base, c->p->code_limit);
}

extern void *
arm_clone_code(c, new_base, available_size)
drisc_ctx c;
void *new_base;
int available_size;
{
    int size = dr_code_size(c);
    if (available_size < size) {
	return NULL;
    }
    void *old_base = c->p->code_base;
    void *native_base = c->p->code_base;
    if (native_base == NULL) native_base = c->p->native.code_base;
    memcpy(new_base, native_base, size);
    c->p->code_base = new_base;
    c->p->cur_ip = new_base + size;
    c->p->fp = new_base;
    arm_branch_link(c);
    arm_call_link(c);
    arm_data_link(c);
    arm_flush(new_base, (void*)((long)new_base + size));
    c->p->code_base = old_base;
    c->p->cur_ip = old_base + size;
    c->p->fp = old_base;
    while (*(int*)new_base == 0xFF000000) {
	/* skip UNIMPs */
	new_base = (void*)((long) new_base + 4);
    }
    return new_base;
}

extern void
arm_pset(drisc_ctx c, int type, int junk, int dest, long imm)
{
    arm_set(c, dest, imm);
}	

extern void
arm_setf(drisc_ctx c, int type, int junk, int dest, double imm)
{
    union {
	float f;
	int i;
    } a;
    union {
	double d;
	long l;
	int i[2];
    } b;
    arm_mach_info ami = (arm_mach_info) c->p->mach_info;
    if (type == DR_F) {
	a.f = (float) imm;
	arm_set(c, _v1, a.i);
	arm_movi2f(c, dest, _v1);
    } else {
	b.d = imm;
	arm_set(c, _v1, b.i[0]);
	arm_pstorei(c, DR_I, 0, _v1, _fp, ami->conversion_word);
	arm_set(c, _v1, b.i[1]);
	arm_pstorei(c, DR_I, 0, _v1, _fp, ami->conversion_word+4);
	arm_ploadi(c, DR_D, 0, dest, _fp, ami->conversion_word);
    }
}	


extern void
arm_set(c, r, val)
drisc_ctx c;
int r;
long val;
{
    arm_dproci(c, MOV, 0, r, 0, val & 0xff);
    if ((val & 0xff00) != 0) {
	int imm = (val >> 8) & 0xff;
	/* or in the byte */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(ORR)|S(0)|RN(r)|RD(r)|IMM(imm, 8));
    }
    if ((val & 0xff0000) != 0) {
	int imm = (val >> 16) & 0xff;
	/* or in the byte */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(ORR)|S(0)|RN(r)|RD(r)|IMM(imm, 16));
    }
    if ((val & 0xff000000) != 0) {
	int imm = (val >> 24) & 0xff;
	/* or in the byte */
	INSN_OUT(c, COND(AL)|CLASS(0x0)|OPCODE(ORR)|S(0)|RN(r)|RD(r)|IMM(imm, 24));
    }
}

#define bit_R(x) ((unsigned long)1<<x)

extern void
arm_reg_init(drisc_ctx c)
{
    c->p->var_i.init_avail[0] = 0;
    c->p->var_i.members[0] = c->p->var_i.init_avail[0];
    c->p->tmp_i.init_avail[0] = (bit_R(_v2)|bit_R(_v3)|bit_R(_v4)|
				 bit_R(_v5)|bit_R(_v6)|bit_R(_v7));
    c->p->tmp_i.members[0] = c->p->tmp_i.init_avail[0] | bit_R(_v1) |
	(bit_R(_a1)|bit_R(_a2)|bit_R(_a3)|bit_R(_a4));
    c->p->var_f.init_avail[0] = 0;
    c->p->var_f.members[0] = c->p->var_f.init_avail[0];
    c->p->tmp_f.init_avail[0] = (bit_R(_f0)|bit_R(_f1)|bit_R(_f2)|bit_R(_f3)|
				 bit_R(_f4)|bit_R(_f5)|bit_R(_f6)|bit_R(_f7));

    c->p->tmp_f.members[0] = c->p->tmp_f.init_avail[0];
}

extern void*
gen_arm_mach_info(c, v9)
drisc_ctx c;
int v9;
{
    arm_mach_info ami = malloc(sizeof(*ami));
    if (c->p->mach_info != NULL) {
	free(c->p->mach_info);
	c->p->mach_info = NULL;
	c->p->native.mach_info = NULL;
    }
    arm_reg_init(c);
    ami->act_rec_size = 0;
    ami->conversion_word = 0;
    ami->gp_save_offset = 0;
    ami->cur_arg_offset = 0;
    ami->stack_align = 4;
    ami->stack_constant_offset = 0;
    ami->fp_save_offset = ami->gp_save_offset + 8 * ami->stack_align;
    ami->fp_save_end = ami->fp_save_offset + 8 * 8;
    ami->max_arg_size = 0;
    return ami;
}

#if defined(HAVE_DIS_ASM_H) && !defined(NO_DISASSEMBLER)
/* GENERIC BINUTILS DISASSEMBLER */
#include "dis-asm.h"

#define MAXLENGTH (1<<23) /* Max length of function that can be disassembled */

extern int
arm_init_disassembly_info(drisc_ctx c, void * ptr)
{
    struct disassemble_info *i = ptr;
#ifdef INIT_DISASSEMBLE_INFO_THREE_ARG
    INIT_DISASSEMBLE_INFO(*i, stdout,fprintf);
    i->endian = BFD_ENDIAN_BIG;
#else
    INIT_DISASSEMBLE_INFO(*i, stdout);
#endif
#ifdef bfd_mach_arm_5
    i->mach = bfd_mach_arm_5;
#elif defined (bfd_mach_arm_4)
    i->mach = bfd_mach_arm_4;
#elif defined (bfd_mach_arm_3)
    i->mach = bfd_mach_arm_3;
#endif
    if (c->p->code_base != NULL) {
	i->buffer = (bfd_byte *)c->p->code_base;
	i->buffer_vma = (bfd_vma)c->p->code_base;
    } else {
	i->buffer = (bfd_byte *)c->p->native.code_base;
	i->buffer_vma = (bfd_vma)c->p->native.code_base;
    }
    i->buffer_length = MAXLENGTH;
#ifdef HAVE_PRINT_INSN_ARM
    return 1;
#elif HAVE_PRINT_INSN_LITTLE_ARM
    return 1;
#else
    return 0;
#endif
}

extern int
arm_print_insn(drisc_ctx c, void *info_ptr, void *insn)
{
#ifdef HAVE_PRINT_INSN_ARM
    return print_insn_arm((unsigned long) insn, (disassemble_info*)info_ptr);
#elif HAVE_PRINT_INSN_LITTLE_ARM
    return print_insn_little_arm((unsigned long) insn, (disassemble_info*)info_ptr);
#else
    return 0;
#endif
}
#else
extern int
arm_init_disassembly_info(drisc_ctx c, void * ptr){return 0;}
extern int arm_print_insn(drisc_ctx c, void *info_ptr, void *insn){return 0;}
#endif

extern void
arm_print_reg(drisc_ctx c, int typ, int reg)
{
    switch(typ) {
    case DR_C: case DR_UC:
    case DR_S: case DR_US:
    case DR_I: case DR_U: case DR_L: case DR_UL:
	if (reg == _sp) {
	    printf("sp");
	    return;
	} else if (reg == _link) {
	    printf("link");
	    return;
	} else if (reg == _pc) {
	    printf("pc");
	    return;
	} else if (reg == _fp) {
	    printf("fp");
	    return;
	} else if (reg <= _r3) {
	    printf("r%d(a%d)\n", reg, reg +1);
	    return;
	} else if (reg <= _r10) {
	    printf("r%d(v%d)\n", reg, reg - 3);
	    return;
	}
	break;
    case DR_F: case DR_D:
	printf("F%d", reg);
	return;
    }
    printf("NoReg(%d)", reg);
}

extern int
arm_count_insn(drisc_ctx c, int start, int end)
{
    return (end - start)>>2;
}
