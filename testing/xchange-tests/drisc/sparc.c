#include "drisc.h"
#include "drisc_internal.h"
#include "sparc.h"
#include "config.h"
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define sparc_save(c, r) sparc_FORM3_arith(c, 0x3c, 0x2, _sp, _sp, r)
#define sparc_savei(c, imm) sparc_FORM3imm_arith(c, 0x3c, 0x2, _sp, _sp, imm)
#define sparc_sethi(c, r, imm) INSN_OUT(c, HDR(0)|RD(r)|OP2(0x4)|imm)
#define sparc_ori(c, dest, src, imm) INSN_OUT(c, HDR(0x2)|OP3(0x2)|IM|RD(dest)|RS1(src)|imm)
#define sparc_xori(c, dest, src, imm) INSN_OUT(c, HDR(0x2)|OP3(0x3)|IM|RD(dest)|RS1(src)|imm)
#define sparc_andi(c, dest, src, imm) INSN_OUT(c, HDR(0x2)|OP3(0x1)|IM|RD(dest)|RS1(src)|imm)
#define sparc_or(c, dest, src1, src2) INSN_OUT(c, HDR(0x2)|OP3(0x2)|RD(dest)|RS1(src1)|RS2(src2))
#define sparc_movi(c, dest, src) sparc_or(c, dest, src, _g0)
#define sparc_int_mov(c, dest, src) sparc_or(c, dest, _g0, src)
#define sparc_movf(c, dest, src) INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(1)|RS2(src))
#define sparc_movd(c, dest, src) INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(2)|RS2(src))
#define sparc_simple_ret(c) INSN_OUT(c, HDR(0x2)|OP3(0x38)|RD(_g0)|RS1(_i7)|IM|SIMM13(0x8))
#define sparc_restore(c) INSN_OUT(c, HDR(0x2)|OP3(0x3d)|RD(_g0)|RS1(_g0)|RS2(_g0));
#define sparc_lshi(c, dest, src1,imm) INSN_OUT(c, HDR(0x2)|OP3(0x25)|RD(dest)|RS1(src1)|IM|SIMM13(imm));
#define sparc_xlshi(c, dest, src1,imm) INSN_OUT(c, HDR(0x2)|OP3(0x25)|RD(dest)|RS1(src1)|IM|SIMM13(imm)|(1<<12) );
#define sparc_rshi(c, dest, src1,imm) INSN_OUT(c, HDR(0x2)|OP3(0x26)|RD(dest)|RS1(src1)|IM|SIMM13(imm));
#define sparc_xrshi(c, dest, src1,imm) INSN_OUT(c, HDR(0x2)|OP3(0x26)|RD(dest)|RS1(src1)|IM|SIMM13(imm)|(1<<12) );
#define sparc_rshai(c, dest, src1,imm) INSN_OUT(c, HDR(0x2)|OP3(0x27)|RD(dest)|RS1(src1)|IM|SIMM13(imm));
#define sparc_xrshai(c, dest, src1,imm) INSN_OUT(c, HDR(0x2)|OP3(0x27)|RD(dest)|RS1(src1)|IM|SIMM13(imm)|(1<<12) );
#define sparc_rsh(c, dest, src1, src2) 	INSN_OUT(c, HDR(0x2)|OP3(0x27)|RD(dest)|RS1(src1)|RS2(src2));

#define sparc_nop(c) INSN_OUT(c, OP2(0x4));

#define IREG 0
#define FREG 1

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

int sparc_type_align[] = {
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
	sizeof(long), /* EC */
};

int sparc_type_size[] = {
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
	sizeof(long), /* EC */
};

extern int
sparc_local(drisc_ctx c, int type)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;

    smi->act_rec_size += roundup(type_info[type].size, smi->stack_align);
    return (-smi->act_rec_size) + smi->stack_constant_offset;
}

extern int
sparc_localb(drisc_ctx c, int size)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if (size < 0) size = 0;
    smi->act_rec_size = roundup(smi->act_rec_size, size);

    smi->act_rec_size += roundup(size, smi->stack_align);
    return (-smi->act_rec_size) - 8 + smi->stack_constant_offset;
}

extern int sparc_local_op(drisc_ctx c, int flag, int val)
{
    int size = val;
    if (flag == 0) {
	size = type_info[val].size;
    }
    if (size < 0) size = 0;
    return sparc_localb(c, size);
}	

static int 
is_temp(int ireg)
{
    return ((ireg <= _g7) && (ireg >= _g0));
}

extern void
sparc_save_restore_op(drisc_ctx c, int save_restore, int type, int reg)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if (save_restore == 0) { /* save */
	switch (type) {
	case DR_D: case DR_F:
	    sparc_pstorei(c, type, 0, reg, _sp, smi->fp_save_offset + reg * smi->stack_align + smi->stack_constant_offset);
	    break;
	default:
	    if (is_temp(reg)) {
		sparc_pstorei(c, type, 0, reg, _sp, smi->gp_save_offset + (reg - _g0) * smi->stack_align + smi->stack_constant_offset);
	    }
	    break;
	}
    } else {  /* restore */
	switch (type) {
	case DR_D: case DR_F:
	    sparc_ploadi(c, type, 0, reg, _sp, smi->fp_save_offset + reg * smi->stack_align + smi->stack_constant_offset);
	    break;
	default:
	    if (is_temp(reg)) {
		sparc_ploadi(c, type, 0, reg, _sp, smi->gp_save_offset + (reg - _g0) * smi->stack_align + smi->stack_constant_offset);
	    }
	    break;
	}
    }
}	

static void
sparc_movi2f(drisc_ctx c, int dest, int src)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    sparc_pstorei(c, DR_I, 0, src, _fp, smi->conversion_word);
    sparc_ploadi(c, DR_F, 0, dest, _fp, smi->conversion_word);
}
    
static void
sparc_movf2i(drisc_ctx c, int dest, int src)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    sparc_pstorei(c, DR_F, 0, src, _fp, smi->conversion_word);
    sparc_ploadi(c, DR_I, 0, dest, _fp, smi->conversion_word);
}
    
static void
sparc_movd2i(drisc_ctx c, int dest, int src)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    sparc_pstorei(c, DR_D, 0, src, _fp, smi->conversion_word);
    if (smi->stack_align == 8) {
	sparc_ploadi(c, DR_L, 0, dest, _fp, smi->conversion_word);
    } else {
	sparc_ploadi(c, DR_I, 0, dest, _fp, smi->conversion_word);
	sparc_ploadi(c, DR_I, 0, dest+1, _fp, smi->conversion_word+4);
    }
}
    
static void
sparc_movi2d(drisc_ctx c, int dest, int src)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if (smi->stack_align == 8) {
	sparc_pstorei(c, DR_L, 0, src, _fp, smi->conversion_word);
    } else {
	sparc_pstorei(c, DR_I, 0, src, _fp, smi->conversion_word);
	sparc_pstorei(c, DR_I, 0, src+1, _fp, smi->conversion_word+4);
    }
    sparc_ploadi(c, DR_D, 0, dest, _fp, smi->conversion_word);
}
    
/*
 *    sparc stack frame organization
 *         HIGH MEMORY 
 *
 *   %fp (old sp) -> -------------------------
 *      %fp -offset      automatic arrays, aggregates 
 *			 and scalar automatic (local vars)
 *                   -------------------------
 *			 alloca space (if any)
 *                   -------------------------
 *      %sp + offset   compiler temps + saved float regs
 *                   -------------------------
 *      %sp + offset   outgoing parameters past the 6th (if any)
 *                   -------------------------
 *      %sp + offset   6 words into which callee may store reg args
 *                   -------------------------
 *      %sp + offset   one word hidden parameter
 *                   -------------------------
 *	%sp + offset   16 words in which to save register window
 *  %sp ->           -------------------------
 *          		| 
 *			V    stack growth
 *         LOW MEMORY   
 */


extern void sparc_FORM3_arith(c, sparc_op3, use_ext_form, dest, src1, src2)
drisc_ctx c;
int sparc_op3;
int use_ext_form;
int dest;
int src1;
int src2;
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    /* format 3 */
    if ((smi->stack_align == 8) && use_ext_form) {
	INSN_OUT(c, HDR(0x2)|OP3(sparc_op3)|1<<12|RD(dest)|RS1(src1)|RS2(src2));
    } else {	
	INSN_OUT(c, HDR(0x2)|OP3(sparc_op3)|RD(dest)|RS1(src1)|RS2(src2));
    }
}

extern void sparc_FORM3imm_arith(c, op3, use_ext_form, dest, src1, imm)
drisc_ctx c;
int op3;
int use_ext_form;
int dest;
int src1;
long imm;
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    int sparcv9 = 0;
    if ((smi->stack_align == 8) && use_ext_form) {
	sparcv9 = 0x1;
    }
    if (((op3 >= 0x25) && (op3 <= 0x27)) && !use_ext_form) {
	imm &= 0x1f;  /* 32-bit reg shift.  limit imm to 31 or less */
    }
    if (((long)imm) < 4096 && ((long)imm) >= -4096) {
	/* format 3 */
	INSN_OUT(c, HDR(0x2)|OP3(op3)|RD(dest)|RS1(src1)|IM|SIMM13(imm)|sparcv9<<12);
    } else {
	sparc_set(c, _g1, imm);
	/* format 3 */
	INSN_OUT(c, HDR(0x2)|OP3(op3)|RD(dest)|RS1(src1)|RS2(_g1)|sparcv9<<12);
    }
}

extern void
sparc_proc_start(drisc_ctx c, char *subr_name, int arg_count, arg_info_list args,
	     dr_reg_t *arglist)
{
    int i;
    int max_in_reg = _i0;
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    int cur_arg_offset = 0;
    /* emit start insns */
    INSN_OUT(c, 0x10000);
    INSN_OUT(c, 0x10000);
    INSN_OUT(c, 0x10000);
    INSN_OUT(c, 0x10000);
    smi->save_insn_offset = (long)c->p->cur_ip - (long)c->p->code_base;
    sparc_savei(c, 0);

    smi->conversion_word = sparc_local(c, DR_D);
    smi->conversion_word = sparc_local(c, DR_D);

    /* load params from regs */
    for (i = 0; i < arg_count; i++) {
	switch (args[i].type) {
	case DR_F: case DR_D:
	    if (smi->stack_align == 8) {
		/* How about the limit on FP registers?  Fix this. */
		int reg;
		args[i].is_register = 1;
		if (args[i].type == DR_F) {
		    reg = _f0 + cur_arg_offset / 4 + 1;
		} else {
		    reg = _f0 + cur_arg_offset / 4;
		}
		dr_rawget(c, _f0 +cur_arg_offset / 4, args[i].type, DR_TEMP);
		args[i].in_reg = args[i].out_reg = reg;
		break;
	    }
	    /* falling through */
	default:
	    if (cur_arg_offset < 6 * smi->stack_align) {
		args[i].is_register = 1;
		args[i].in_reg = _i0 + cur_arg_offset/smi->stack_align;
		args[i].out_reg = _o0 + cur_arg_offset/smi->stack_align;
		max_in_reg = args[i].in_reg;
	    } else {
		args[i].is_register = 0;
	    }
	    break;
	}
	args[i].offset = cur_arg_offset;
	cur_arg_offset += roundup(type_info[(int)args[i].type].size, smi->stack_align);
    }
    
    for (i=_i0 ; i < _i6; i++) {
	if (i <= max_in_reg) {
	    dr_rawget(c, i, DR_I, DR_VAR);
	} else {
	    dr_rawput(c, i, DR_I, DR_VAR);
	}
    }
    for (i = 0; i < arg_count; i++) {
	int tmp_reg;
	if (smi->stack_align != 8) {
	    /* 32-bit sparc */
	    /* only do nothing for int params in registers */
	    if (args[i].is_register && ((args[i].type != DR_F) && 
					(args[i].type != DR_D))) {
		if (arglist != NULL) arglist[i] = args[i].in_reg;
		continue;
	    }
	} else {
	    /* 64-bit sparc do nothing for anything in a register */
	    if (args[i].is_register) {
		if (arglist != NULL) arglist[i] = args[i].in_reg;
		continue;
	    }
	}	    
	if (!dr_getreg(c, &tmp_reg, args[i].type, DR_VAR)) {
	    fprintf(stderr, "not enough registers for parameter %d\n", i);
	    exit(1);
	}
	if (arglist != NULL) arglist[i] = tmp_reg;
	if (args[i].is_register) {
	    /* must be float */
	    if (args[i].type == DR_F) {
		sparc_movi2f(c, tmp_reg, args[i].in_reg);
		dr_rawput(c, args[i].in_reg, DR_I, DR_VAR);
	    } else {
		/* sparcv8 boundary condition, half in register */
		if (args[i].offset == 5*4) {
		    int real_offset = args[i].offset + 68; 
		    sparc_pstorei(c, DR_I, 0, args[i].in_reg, _fp, 
				  real_offset);
		    sparc_ploadi(c, DR_F, 0, tmp_reg, _fp, real_offset);
		    sparc_ploadi(c, DR_F, 0, tmp_reg+1, _fp, real_offset+4);
		} else {
		    sparc_movi2d(c, tmp_reg, args[i].in_reg);
		    dr_rawput(c, args[i].in_reg, DR_I, DR_VAR);
		    if (smi->stack_align == 4) {
			dr_rawput(c, args[i].in_reg, DR_I, DR_VAR);

		    }
		}
	    }
	} else {
	    /* general offset from fp*/
	    int real_offset = args[i].offset + 8 +15*smi->stack_align; 
	    if (type_info[(int)args[i].type].size < smi->stack_align) {
		real_offset += smi->stack_align - type_info[(int)args[i].type].size;
	    }
	    real_offset += smi->stack_constant_offset;
	    if (args[i].type != DR_D) {
		sparc_ploadi(c, args[i].type, 0, tmp_reg, _fp, 
			     real_offset);
	    } else {
		sparc_ploadi(c, DR_I, 0, _g1, _fp, real_offset);
		sparc_pstorei(c, DR_I, 0, _g1, _fp, smi->conversion_word);
		sparc_ploadi(c, DR_I, 0, _g1, _fp, real_offset+4);
		sparc_pstorei(c, DR_I, 0, _g1, _fp, smi->conversion_word+4);
		sparc_ploadi(c, DR_D, 0, tmp_reg, _fp, smi->conversion_word);
	    }
	}
	args[i].in_reg = tmp_reg;
	args[i].is_register = 1;
    }
}

static char ld_opcodes[] = {
    0x09, /* DR_C */
    0x01, /* DR_UC */
    0x0a, /* DR_S */
    0x02, /* DR_US */
    0x08, /* DR_I */
    0x00, /* DR_U */
    0x0b, /* DR_L */
    0x0b, /* DR_UL */
    0x0b, /* DR_P */
    0x0,  /* DR_F */
    0x0,  /* DR_D */
    0x00, /* DR_V */
    0x00, /* DR_B */
    0x0b, /* DR_EC */
};
extern void
sparc_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    if  (((long)offset) >= 4096 || ((long)offset) < -4096) {
	sparc_set(c, _g1, offset);
	sparc_pload(c, type, junk, dest, src, _g1);
	return;
    }

    switch (type) {
    case DR_F:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x20)|RS1(src)|IM|SIMM13(offset));
	break;
    case DR_D:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x23)|RS1(src)|IM|SIMM13(offset));
	break;
    case DR_L: case DR_UL: case DR_P:{
	sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
	if (smi->stack_align == 4) {
	    type = DR_I;
	}
    }
    /* fall through */
    default:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(ld_opcodes[type])|RS1(src)|IM|SIMM13(offset));
	break;
    }
}

extern void
sparc_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    switch (type) {
    case DR_F:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x20)|RS1(src1)|RS2(src2));
	break;
    case DR_D:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x23)|RS1(src1)|RS2(src2));
	break;
    case DR_L: case DR_UL: case DR_P:
    {
	sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
	if (smi->stack_align == 4) {
	    type = DR_I;
	}
	/* fall through */
    }
    default:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(ld_opcodes[type])|RS1(src1)|RS2(src2));
	break;
    }
}

static char ld_bs_opcodes[] = {  /* load from alternate space */
    0x19, /* DR_C */
    0x11, /* DR_UC */
    0x1a, /* DR_S */
    0x12, /* DR_US */
    0x18, /* DR_I */
    0x10, /* DR_U */
    0x1b, /* DR_L */
    0x1b, /* DR_UL */
    0x1b, /* DR_P */
/*        DR_F */
/*        DR_D */
};
extern void
sparc_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    if (offset == 0) {
	sparc_pbsload(c, type, junk, dest, src, _g0);
    } else {
	sparc_set(c, _g1, offset);
	sparc_pbsload(c, type, junk, dest, src, _g1);
    }
}


extern void
sparc_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    switch (type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen bsload\n");
	break;
    case DR_L: case DR_UL: case DR_P:
    {
	sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
	if (smi->stack_align == 4) {
	    type = DR_I;
	}
	/* fall through */
    }
    default:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(ld_bs_opcodes[type])|RS1(src1)|RS2(src2)|ASI(0x88));
	break;
    }
}

static char st_opcodes[] = {
    0x05, /* DR_C */
    0x05, /* DR_UC */
    0x06, /* DR_S */
    0x06, /* DR_US */
    0x04, /* DR_I */
    0x04, /* DR_U */
    0x0e, /* DR_L */
    0x0e, /* DR_UL */
    0x0e, /* DR_P */
    0x0,  /* DR_F */
    0x0,  /* DR_D */
    0x00, /* DR_V */
    0x00, /* DR_B */
    0x0e, /* DR_EC */
};
extern void
sparc_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset)
{
    if  (((long)offset) >= 4096 || ((long)offset) < -4096) {
	sparc_set(c, _g1, offset);
	sparc_pstore(c, type, junk, dest, src, _g1);
	return;
    }

    switch (type) {
    case DR_F:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x24)|RS1(src)|IM|SIMM13(offset));
	break;
    case DR_D:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x27)|RS1(src)|IM|SIMM13(offset));
	break;
    case DR_L: case DR_UL: case DR_P:{
	sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
	if (smi->stack_align == 4) {
	    type = DR_I;
	}
    }
    /* fall through */
    default:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(st_opcodes[type])|RS1(src)|IM|SIMM13(offset));
	break;
    }
}

extern void
sparc_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2)
{
    switch (type) {
    case DR_F:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x24)|RS1(src1)|RS2(src2));
	break;
    case DR_D:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(0x27)|RS1(src1)|RS2(src2));
	break;
    case DR_L: case DR_UL: case DR_P:
    {
	sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
	if (smi->stack_align == 4) {
	    type = DR_I;
	}
	/* fall through */
    }
    default:
	INSN_OUT(c, HDR(0x3)|RD(dest)|OP3(st_opcodes[type])|RS1(src1)|RS2(src2));
	break;
    }
}

static long drisc_hidden_modi(int a, int b)
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

extern void sparc_mod(drisc_ctx c, int data1, int type_long, int dest, 
		      int src1, int src2)
{
    int return_reg;
    if (data1 == 1) {
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

extern void sparc_modi(drisc_ctx c, int data1, int data2, int dest, int src1, 
		      long imm)
{
    sparc_set(c, _g1, imm);
    sparc_mod(c, data1, data2, dest, src1, _g1);
}

extern long drisc_hidden_udiv(unsigned long a, unsigned long b)
{ return a / b; }

extern void sparc_div(drisc_ctx c, int op3, int type_long, int dest, int src1,
		      int src2)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if ((op3 == 0x0d /* udiv */) && (type_long == 1)) {
	int return_reg;
	return_reg = dr_scalll(c, (void*)&drisc_hidden_udiv, "%l%l", src1, src2);
	dr_movl(c, dest, return_reg);
	return;

    }
    if (op3 == 0x0d) {
	INSN_OUT(c, HDR(0x2)|RD(0)|OP3(0x30)|RS1(_g0)|RS2(_g0));/*wry(_g0, _g0);*/
    } else {
	sparc_rshai(c, _g1, src1, 31);
	INSN_OUT(c, HDR(0x2)|RD(0)|OP3(0x30)|RS1(_g0)|RS2(_g1));/*wry(_g0, _g1);*/
    }
    if ((type_long == 0) || (smi->stack_align != 8)) {
	if (op3 == 0x0d) {
	    op3 = 0x0e;
	} else {
	    op3 = 0x1f;
	}
    }
    sparc_nop(c);
    sparc_nop(c);
    sparc_nop(c);
    INSN_OUT(c, HDR(2)|OP3(op3)|RD(dest)|RS1(src1)|RS2(src2));
    if (op3 == 0x1f) {
	INSN_OUT(c, HDR(0)|1<<29|COND(0x7)|2<<22|2);
	sparc_sethi(c, dest, 1<<21);
    }
}

extern void sparc_divi(drisc_ctx c, int sparc_op3, int type_long, 
		       int dest, int src, long imm)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if ((sparc_op3 == 0x0d /* udiv */) && (type_long == 1)) {
	sparc_set(c, _g1, imm);
	sparc_div(c, sparc_op3, type_long, dest, src, _g1);
	return;
    }
    if (sparc_op3 == 0x0d) {
	INSN_OUT(c, HDR(0x2)|RD(0)|OP3(0x30)|RS1(_g0)|RS2(_g0));/*wry(_g0, _g0);*/
    } else {
	sparc_rshai(c, _g1, src, 31);
	INSN_OUT(c, HDR(0x2)|RD(0)|OP3(0x30)|RS1(_g0)|RS2(_g1));/*wry(_g0, _g1);*/
    }
    if ((type_long == 0) || (smi->stack_align != 8)) {
	if (sparc_op3 == 0x0d) {
	    sparc_op3 = 0x0e;
	} else {
	    sparc_op3 = 0x1f;
	}
    }
    sparc_set(c, _g1, imm);
    sparc_nop(c);
    sparc_nop(c);
    INSN_OUT(c, HDR(2)|OP3(sparc_op3)|RD(dest)|RS1(src)|RS2(_g1));
    if (sparc_op3 == 0x1f) {
	INSN_OUT(c, HDR(0)|1<<29|COND(0x7)|2<<22|2);
	sparc_sethi(c, dest, 1<<21);
    }
}

extern void
sparc_mov(drisc_ctx c, int type, int junk, int dest, int src)
{
    if (src == dest) return;
    switch(type) {
    case DR_D:
	sparc_movd(c, dest, src);
	break;
    case DR_F:
	sparc_movf(c, dest, src);
	break;
    default:
	sparc_ori(c, dest, src, 0x0);
    }
}

extern void
sparc_lea(drisc_ctx c, int j1, int j2, int dest, int src, long imm)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if (src != _fp) {
	/* sparc_add */
	sparc_FORM3imm_arith(c, 0, 0, dest, src, imm);
    } else {
	sparc_FORM3imm_arith(c, 0, 0, dest, src, 
			     imm  + smi->stack_constant_offset);
    }
}
	
static void
sparc_saverestore_floats(drisc_ctx c, int saverestore)
{
    int i;
    for (i=2; i <32 ; i+=2) {
	if (drisc_mustsave(&c->p->tmp_f, i)) {
	    sparc_save_restore_op(c, saverestore, DR_D, i);
	}
    }
}

#define CONV(x,y) ((x*100)+y)
extern void
sparc_convert(drisc_ctx c, int from_type, int to_type, 
	      int dest, int src)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    int word_size = smi->stack_align << 3;

    from_type &= 0xf;
    to_type &= 0xf;
    switch(CONV(from_type, to_type)) {
    case CONV(DR_I, DR_L):
	if (word_size == 64) {
	    /* sign extend */
	    sparc_xlshi(c, dest, src, 32);
	    sparc_xrshai(c, dest, dest, 32);
	    return;
	}
	/* fall through to mov */
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
	sparc_movi(c, dest,src);
	break;
    case CONV(DR_F,DR_D):
	INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(0xc9)|RS2(src)); /*fstod*/
	break;
    case CONV(DR_F,DR_L):
	if (smi->stack_align == 8) {
	    INSN_OUT(c, HDR(0x2)|RD(src)|OP3(0x34)|OPF(0x81)|RS2(src));/*fstox*/
	    sparc_pstorei(c, DR_D, 0, src, _fp, smi->conversion_word);
	    sparc_ploadi(c, DR_L, 0, dest, _fp, smi->conversion_word);
	    break;
	}
	/* falling through */
    case CONV(DR_F,DR_I):
	INSN_OUT(c, HDR(0x2)|RD(src)|OP3(0x34)|OPF(0xd1)|RS2(src));/*fstoi*/
	sparc_movf2i(c, dest, src);
	break;
    case CONV(DR_F,DR_U):
        {
	    int ret;
	    sparc_saverestore_floats(c, 0);
	    ret = dr_scallu(c, (void*)drisc_hidden_ftou, "%f", src);
	    sparc_saverestore_floats(c, 1);
	    sparc_mov(c, DR_UL, 0, dest, ret);
	}
	break;
	/* fallthrough */
    case CONV(DR_F,DR_UL):
        {
	    int ret;
	    sparc_saverestore_floats(c, 0);
	    ret = dr_scallul(c, (void*)drisc_hidden_ftoul, "%f", src);
	    sparc_saverestore_floats(c, 1);
	    sparc_mov(c, DR_UL, 0, dest, ret);
	}
	break;
    case CONV(DR_D,DR_F):
	INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(0xc6)|RS2(src)); /*fdtos*/
	break;
    case CONV(DR_D,DR_L):
	if (smi->stack_align == 8) {
	    INSN_OUT(c, HDR(0x2)|RD(src)|OP3(0x34)|OPF(0x82)|RS2(src));/*fdtox*/
	    sparc_pstorei(c, DR_D, 0, src, _fp, smi->conversion_word);
	    sparc_ploadi(c, DR_L, 0, dest, _fp, smi->conversion_word);
	    break;
	}
	/* falling through */
    case CONV(DR_D,DR_I):
	INSN_OUT(c, HDR(0x2)|RD(src)|OP3(0x34)|OPF(0xd2)|RS2(src));/*fdtoi*/
	sparc_movf2i(c, dest, src);
	break;
    case CONV(DR_D,DR_U):
        {
	    int ret;
	    sparc_saverestore_floats(c, 0);
	    ret = dr_scallu(c, (void*)drisc_hidden_dtou, "%d", src);
	    sparc_saverestore_floats(c, 1);
	    sparc_mov(c, DR_U, 0, dest, ret);
	}
	break;
    case CONV(DR_D,DR_UL):
        {
	    int ret;
	    sparc_saverestore_floats(c, 0);
	    ret = dr_scallul(c, (void*)drisc_hidden_dtoul, "%d", src);
	    sparc_saverestore_floats(c, 1);
	    sparc_mov(c, DR_UL, 0, dest, ret);
	}
	break;
    case CONV(DR_I,DR_D):
	sparc_rshi(c, _g1, src, 0);
	src = _g1;
	/* fall through */
    case CONV(DR_L,DR_D):
	sparc_movi2f(c, dest, src);
	INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(0xc8)|RS2(dest));/*fitod*/
	break;
    case CONV(DR_U,DR_D):
	if (smi->stack_align == 8) { 
	    sparc_rshi(c, _g1, src, 0);
	    src = _g1;
	/* fall through */
	    sparc_pstorei(c, DR_UL, 0, src, _fp, smi->conversion_word);
	    sparc_ploadi(c, DR_D, 0, dest, _fp, smi->conversion_word);
	    INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(0x88)|RS2(dest));/*fxtod*/
	    break;
	}
	/* fallthrough */
    case CONV(DR_UL,DR_D): 
        {
	    int ret;
	    sparc_saverestore_floats(c, 0);
	    ret = dr_scalld(c, (void*)drisc_hidden_ultod, "%l", src);
	    sparc_saverestore_floats(c, 1);
	    sparc_mov(c, DR_D, 0, dest, ret);
	}
	break;
    case CONV(DR_I,DR_F):
    case CONV(DR_L,DR_F):
	sparc_movi2f(c, dest, src);
	INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(0xc4)|RS2(dest));/*fitos*/
	break;
    case CONV(DR_U,DR_F):
	if (smi->stack_align == 8) { 
	    sparc_rshi(c, _g1, src, 0);
	    src = _g1;
	    sparc_pstorei(c, DR_UL, 0, src, _fp, smi->conversion_word);
	    sparc_ploadi(c, DR_D, 0, dest, _fp, smi->conversion_word);
	    INSN_OUT(c, HDR(0x2)|RD(dest)|OP3(0x34)|OPF(0x84)|RS2(dest));/*fxtos*/
	    break;
	}
	/* fallthrough */
    case CONV(DR_UL,DR_F):
        {
	    int ret;
	    sparc_saverestore_floats(c, 0);
	    ret = dr_scalld(c, (void*)drisc_hidden_ultof, "%l", src);
	    sparc_saverestore_floats(c, 1);
	    sparc_mov(c, DR_D, 0, dest, ret);
	}
	break;
    case CONV(DR_C,DR_UL):
    case CONV(DR_C,DR_L):
    case CONV(DR_C,DR_I):
    case CONV(DR_C,DR_U):
	sparc_lshi(c, dest, src, 24);
	sparc_rshai(c, dest, dest, 24);
	break;
    case CONV(DR_I, DR_C):
    case CONV(DR_U, DR_C):
    case CONV(DR_L, DR_C):
    case CONV(DR_UL, DR_C):
	sparc_andi(c, dest, src, 0xff);
	break;
    case CONV(DR_S,DR_L):
    case CONV(DR_S,DR_UL):
    case CONV(DR_S,DR_I):
    case CONV(DR_S,DR_U):
	sparc_lshi(c, dest, src, 16);
	sparc_rshai(c, dest, dest, 16);
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
	sparc_lshi(c, dest, src, 16);
	sparc_rshi(c, dest, dest, 16);
	break;
    default:
	printf("Unknown case in sparc convert %d\n", CONV(from_type,to_type));
    }
}

static signed char op_conds[] = {
    0x01, /* dr_beq_code */  /* signed */
    0x0b, /* dr_bge_code */
    0x0a, /* dr_bgt_code */
    0x02, /* dr_ble_code */
    0x03, /* dr_blt_code */
    0x09, /* dr_bne_code */

    0x01, /* dr_beq_code */  /* unsigned */
    0x00, /* dr_bge_code */ /* no unsigned version */
    0x0c, /* dr_bgt_code */ 
    0x04, /* dr_ble_code */
    0x00, /* dr_blt_code */ /* no unsigned version */
    0x09, /* dr_bne_code */
};

static char fop_conds[] = {
    0x09, /* dr_beq_code */
    0x0b, /* dr_bge_code */
    0x06, /* dr_bgt_code */
    0x0d, /* dr_ble_code */
    0x04, /* dr_blt_code */
    0x01, /* dr_bne_code */
};

extern void
sparc_branch(drisc_ctx c, int op, int type, int src1, int src2, int label)
{
    switch(type) {
    case DR_F:
	INSN_OUT(c, HDR(0x2)|OP3(0x35)|RS1(src1)|OPF(0x51)|RS2(src2));/*fcmps*/
	dr_mark_branch_location(c, label);
	INSN_OUT(c, HDR(0)|COND(fop_conds[op])|(0x5<<22)|CC(0x0)|P(1)|/*disp */0);/* fbp*/
	break;
    case DR_D:
	INSN_OUT(c, HDR(0x2)|OP3(0x35)|RS1(src1)|OPF(0x52)|RS2(src2));
	dr_mark_branch_location(c, label);
	INSN_OUT(c, HDR(0)|COND(fop_conds[op])|(0x5<<22)|CC(0x0)|P(1)|/*disp */0);/* fbp*/
	break;
    case DR_U:
    case DR_UL:
	switch(op) {
	case dr_bge_code: {
	    int tmp = src1; src1 = src2; src2 = tmp;  /* swap operands */
	    op = dr_ble_code;
	    break;
	}
	case dr_blt_code: {
	    int tmp = src1; src1 = src2; src2 = tmp;  /* swap operands */
	    op = dr_bgt_code;
	    break;
	}
	}
	op += 6; /* second set of codes */
	/* fall through */
    default:
	INSN_OUT(c, HDR(0x2)|RD(_g0)|OP3(0x14)|RS1(src1)|RS2(src2)); /* subcc */
	dr_mark_branch_location(c, label);
	INSN_OUT(c, HDR(0)|COND(op_conds[op])|(2<<22)|/*disp */0);/* bp*/
    }
    sparc_nop(c);
}

extern void 
sparc_jump_to_label(drisc_ctx c, unsigned long label)
{
    dr_mark_branch_location(c, label);
    INSN_OUT(c, HDR(0)|COND(8)|(2<<22)|/*disp */0);/* bp always*/
    sparc_nop(c);
}

extern void sparc_jump_to_reg(drisc_ctx c, unsigned long reg)
{
    INSN_OUT(c, HDR(0x2)|OP3(0x38)|RD(_g0)|RS1(reg)|IM|SIMM13(0x0));
    sparc_nop(c);
}

extern void sparc_jump_to_imm(drisc_ctx c, unsigned long imm)
{
    INSN_OUT(c, HDR(0x2)|OP3(0x38)|RD(_g0)|RS1(_i7)|IM|SIMM13(imm));
    sparc_nop(c);
}

extern void 
sparc_jal(drisc_ctx c, int return_addr_reg, int target)
{
    INSN_OUT(c, HDR(0x2)|OP3(0x38)|RD(return_addr_reg)|RS1(target)|IM|SIMM13(0x0));
}

static void internal_push(drisc_ctx c, int type, int immediate, 
			  void *value_ptr)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
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
	
    if (smi->cur_arg_offset < 6 * smi->stack_align) {
	arg.is_register = 1;
	if ((smi->stack_align == 8) && ((type == DR_F) || (type == DR_D))) {
	    arg.out_reg = _f0  + smi->cur_arg_offset/4;
	    if (type == DR_F) arg.out_reg++;
	    arg.in_reg = _o0   + smi->cur_arg_offset/8;
	} else {
	    /* sparcv8 */
	    arg.in_reg = _i0 + smi->cur_arg_offset/smi->stack_align;
	    arg.out_reg = _o0 + smi->cur_arg_offset/smi->stack_align;
	}
    } else {
	if ((smi->stack_align == 8) && ((type == DR_F) || (type == DR_D)) &&
	    (smi->cur_arg_offset <= 10 * smi->stack_align)) {
	    /* floating arg can go in a float reg, but not int reg */
	    arg.is_register = 1;
	    arg.out_reg = _f0  + smi->cur_arg_offset/4;
	    arg.in_reg = -1;
	} else {
	    arg.is_register = 0;
	}
    }

    arg.offset = smi->cur_arg_offset;
    smi->cur_arg_offset += 
	roundup(type_info[(int)arg.type].size, smi->stack_align);
    real_offset = arg.offset + 8 + 15*smi->stack_align + 
	smi->stack_constant_offset;
    if (smi->stack_align == 4) {
	/* sparcv8 */
	if (arg.is_register == 0) {
	    /* store it on the stack only */
	    if (arg.is_immediate) {
		if (type != DR_D) {
		    if (type == DR_F) {
			float f = (float) *(double*)value_ptr;
			sparc_set(c, _g1, *(int*)&f);
		    } else {
			sparc_set(c, _g1, *(long*)value_ptr);
		    }
		    sparc_pstorei(c, arg.type, 0, _g1, _sp, real_offset);
		} else {
		    sparc_set(c, _g1, *(int*)value_ptr);
		    sparc_pstorei(c, DR_I, 0, _g1, _sp, real_offset);
		    sparc_set(c, _g1, *(((int*)value_ptr)+1));
		    sparc_pstorei(c, DR_I, 0, _g1, _sp, real_offset+4);
		}		
	    } else {
		if (type != DR_D) {
		    sparc_pstorei(c, arg.type, 0, *(int*)value_ptr, _sp, real_offset);
		} else {
		    sparc_pstorei(c, DR_F, 0, *(int*)value_ptr, _sp, real_offset);
		    sparc_pstorei(c, DR_F, 0, (*(int*)value_ptr)+1, _sp, 
				  real_offset + 4);
		}
	    }
	} else {
	    if ((type != DR_F) && (type != DR_D)) {
		if (arg.is_immediate) {
		    sparc_set(c, arg.out_reg, *(long*)value_ptr);
		} else {
		    sparc_mov(c, type, 0, arg.out_reg, *(int*) value_ptr);
		}
	    } else {
		if (arg.is_immediate) {
		    if (type == DR_F) {
			float f = (float) *(double*)value_ptr;
			sparc_set(c, arg.out_reg, *(int*)&f);
		    } else {
			sparc_set(c, arg.out_reg, *(int*)value_ptr);
			if (arg.out_reg != _o5) {
			    sparc_set(c, arg.out_reg+1, *(((int*)value_ptr)+1));
			} else {
			    /* sparcv8 boundary condition */
			    sparc_set(c, _g1, *(((int*)value_ptr)+1));
			    sparc_pstorei(c, DR_I, 0, _g1, _sp, real_offset + 4);
			}
		    }
		} else {
		    if (type == DR_F) {
			sparc_movf2i(c, arg.out_reg, *(int*)value_ptr);
		    } else {
			if (arg.out_reg != _o5) {
			    sparc_movd2i(c, arg.out_reg, *(int*)value_ptr);
			} else {
			    /* sparcv8 boundary condition */
			    sparc_movf2i(c, arg.out_reg, *(int*)value_ptr);
			    sparc_pstorei(c, DR_F, 0, (*(int*)value_ptr)+1, _sp, 
					  real_offset + 4);
			}
		    }
		}

	    }
	}
    } else {
	/* sparcv9 */
	if (arg.is_register == 0) {
	    /* store it on the stack only */
	    if (arg.is_immediate) {
		if (type == DR_F) {
		    float f = (float) *(double*)value_ptr;
		    sparc_set(c, _g1, *(int*)&f);
		} else {
		    sparc_set(c, _g1, *(long*)value_ptr);
		}
		sparc_pstorei(c, arg.type, 0, _g1, _sp, real_offset);
	    } else {
		sparc_pstorei(c, arg.type, 0, *(int*)value_ptr, _sp, 
			      real_offset);
	    }
	} else {
	    if ((type != DR_F) && (type != DR_D)) {
		if (arg.is_immediate) {
		    sparc_set(c, arg.out_reg, *(long*)value_ptr);
		} else {
		    sparc_mov(c, type, 0, arg.out_reg, *(int*) value_ptr);
		}
	    } else {
		if (arg.is_immediate) {
		    if ((type == DR_F) || (type == DR_D)) {
			/* set appropriate register */
			sparc_setf(c, type, 0, arg.out_reg, 
				   *(double*)value_ptr);
		    } else {
			sparc_set(c, arg.out_reg, *(int*)value_ptr);
		    }
		} else {
		    /* move to the appropriate float reg */
		    sparc_mov(c, type, 0, arg.out_reg, *(int*)value_ptr);
		}
		if (arg.in_reg != -1) {
		    /* put value in int regs too */
		    if (type == DR_D) {
			sparc_movd2i(c, arg.in_reg, arg.out_reg);
		    } else {
			sparc_movf2i(c, arg.in_reg, arg.out_reg);
		    }
		} else {
		    /* put it on the stack as well */
		    sparc_pstorei(c, arg.type, 0, arg.out_reg, _sp,
				  real_offset);
		}
	    }
	}
    }		
}

static void push_init(drisc_ctx c)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    smi->cur_arg_offset = 0;
}

extern void sparc_push(drisc_ctx c, int type, int reg)
{
    if ((type == DR_V) && (reg == -1)) {
	push_init(c);
    } else {
	internal_push(c, type, 0, &reg);
    }
}

extern void sparc_pushi(drisc_ctx c, int type, long value)
{
    internal_push(c, type, 1, &value);
}

extern void sparc_pushfi(drisc_ctx c, int type, double value)
{
    internal_push(c, type, 1, &value);
}

extern int sparc_calli(drisc_ctx c, int type, void *xfer_address)
{
    int caller_side_ret_reg = _o0;

    /* save temporary registers */
    dr_mark_call_location(c, NULL, xfer_address);
    INSN_OUT(c, HDR(0x1)|0);
    sparc_nop(c);
    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
	caller_side_ret_reg = _f0;
    }
    push_init(c);
    return caller_side_ret_reg;
}

extern int sparc_callr(drisc_ctx c, int type, int src)
{
    int caller_side_ret_reg = _o0;

    /* save temporary registers */
    sparc_jal(c, _o7, src);
    sparc_nop(c);
    /* restore temporary registers */
    if ((type == DR_D) || (type == DR_F)) {
	caller_side_ret_reg = _f0;
    }
    push_init(c);
    return caller_side_ret_reg;
}

extern void
sparc_branchi(drisc_ctx c, int op, int type, int src, long imm, int label)
{
    switch(type) {
    case DR_F:
    case DR_D:
	fprintf(stderr, "Shouldn't happen\n");
	break;
    case DR_U:
    case DR_UL:
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
	op += 6; /* second set of codes */
	/* fall through */
    default:
	sparc_FORM3imm_arith(c, 0x14, 0, _g0, src, imm); /* subcc */
	dr_mark_branch_location(c, label);
	INSN_OUT(c, HDR(0)|COND(op_conds[op])|(2<<22)|/*disp */0);/* bp*/
	sparc_nop(c);
    }
}

extern void sparc_ret(drisc_ctx c, int data1, int data2, int src)
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
	if (src != _i0) sparc_int_mov(c, _i0, src);
	break;
    case DR_F:
	if (src != _f0) sparc_movf(c, _f0, src);
	break;
    case DR_D:
	if (src != _f0) sparc_movd(c, _f0, src);
	break;
    }
    sparc_simple_ret(c);
    sparc_restore(c);
}

extern void sparc_reti(drisc_ctx c, int data1, int data2, long imm)
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
	sparc_set(c, _i0, imm);
	break;
    case DR_F:
    case DR_D:
	break;/* no return immediate of floats */
    }
    sparc_simple_ret(c);
    sparc_restore(c);
}

static void
sparc_data_link(drisc_ctx c)
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
sparc_branch_link(drisc_ctx c)
{
    struct branch_table *t = &c->p->branch_table;
    int i;

    for(i=0; i< t->branch_count; i++) {
	int label = t->branch_locs[i].label;
	int label_offset = t->label_locs[label] - t->branch_locs[i].loc;
	int *branch_addr = (int*)((char *)c->p->code_base + 
				  t->branch_locs[i].loc);
        /* div addr diff by 4 for sparc offset value */
	label_offset = label_offset >> 2;  
	*branch_addr &= 0xffc00000;
	*branch_addr |= (label_offset & 0x3fffff);
    }
}

/*
 * on 64-bit solaris 8, we need a procedure linkage table to manage 
 * calls to DLLs in an address range that is typically more than 32 bits
 * away from malloc'd memory.  We emit a PLT that is basically a set_reg, 
 * then jump_through_reg for each routine.  Later, during call linkage, 
 * we'll call to the PLT entry rather than directly to the routine.
 */
static void
sparc_PLT_emit(drisc_ctx c)
{
    call_t *t = &c->p->call_table;
    int i;

    for(i=0; i< t->call_count; i++) {
	int *call_addr = (int*) ((unsigned long)c->p->code_base + 
				 t->call_locs[i].loc);
	long call_offset = (unsigned long)t->call_locs[i].xfer_addr - 
	    (unsigned long)call_addr;
        /* div addr diff by 4 for sparc offset value */
	call_offset = call_offset >> 2;
	call_offset = call_offset >> 30;
	if ((call_offset != 0) && (call_offset != -1)) {
	    t->call_locs[i].mach_info = (void*)
		((long)c->p->cur_ip - (long)c->p->code_base);
	    sparc_set(c, _g1, (unsigned long)t->call_locs[i].xfer_addr);
	    sparc_jump_to_reg(c, _g1);
	    sparc_nop(c);
	}
    }
}

static void
sparc_call_link(drisc_ctx c)
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
	
	    /* div addr diff by 4 for sparc offset value */
	    call_offset = call_offset >> 2;
	    *call_addr &= 0xc0000000;
	    *call_addr |= (call_offset & 0x3fffffff);
	} else {
	    /* call through PLT */
	    unsigned long PLT_addr = (unsigned long)c->p->code_base + 
				      (unsigned long)t->call_locs[i].mach_info;
	    int call_offset = PLT_addr - (unsigned long)call_addr;
	    
	    call_offset = call_offset >> 2;
	    *call_addr &= 0xc0000000;
	    *call_addr |= (call_offset & 0x3fffffff);
	}
    }
}

static void
sparc_flush(void *base, void *limit)
{
#if defined(HOST_SPARC) || defined(HOST_SPARCV9)
    {
	volatile void *ptr = base;

#ifdef __GNUC__
	/* flush every 8 bytes of preallocated insn stream. */
	while((char*)ptr < (char*) limit) {
	    asm volatile ("iflush %0" : /* */ : "r" (ptr));
	    ptr = (char *)ptr + 8;
	}
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
#else
	int nbytes = (char*)limit - (char*)base;
	for(; nbytes > 0;nbytes -= 8) {
	    asm("add %i0, 8, %i0");
	    asm ("iflush %i0");
	}

	asm ("nop");
	asm ("nop");
	asm ("nop");
	asm ("nop");
	asm ("nop");
#endif
#ifdef USE_MEMBAR
	asm("membar #Sync");
#endif
    }
#endif
}    

static void
sparc_emit_save(drisc_ctx c)
{
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    void *save_ip = c->p->cur_ip;
    int ar_size = smi->fp_save_end + smi->act_rec_size;
    ar_size = roundup(ar_size, 16) + 16;

    c->p->cur_ip = (char*)c->p->code_base + smi->save_insn_offset;
    sparc_savei(c, -ar_size);
    c->p->fp = (char*)c->p->code_base + smi->save_insn_offset;
    c->p->cur_ip = save_ip;
}
    
extern void
sparc_end(c)
drisc_ctx c;
{
    sparc_simple_ret(c);
    sparc_restore(c);
    sparc_PLT_emit(c);   /* must be done before linking */
    sparc_branch_link(c);
    sparc_call_link(c);
    sparc_data_link(c);
    sparc_emit_save(c);
    sparc_flush(c->p->code_base, c->p->code_limit);
}

extern void *
sparc_clone_code(c, new_base, available_size)
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
    c->p->cur_ip = (void*)((long)new_base + size);
    c->p->fp = new_base;
    sparc_branch_link(c);
    sparc_call_link(c);
    sparc_data_link(c);
    sparc_flush(c->p->code_base, c->p->code_limit);
    c->p->code_base = old_base;
    c->p->cur_ip = (void*)((long) old_base + size);
    c->p->fp = old_base;
    while (*(int*)new_base == 0x10000) {
	/* skip UNIMPs */
	new_base = (void*) ((long) new_base + 4);
    }
    return new_base;
}

extern void
sparc_pset(drisc_ctx c, int type, int junk, int dest, long imm)
{
    sparc_set(c, dest, imm);
}	

extern void
sparc_setf(drisc_ctx c, int type, int junk, int dest, double imm)
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
    sparc_mach_info smi = (sparc_mach_info) c->p->mach_info;
    if (type == DR_F) {
	a.f = (float) imm;
	sparc_set(c, _g1, a.i);
	sparc_movi2f(c, dest, _g1);
    } else if (smi->stack_align == 4) {
	b.d = imm;
	sparc_set(c, _g1, b.i[0]);
	sparc_movi2f(c, dest, _g1);
	sparc_set(c, _g1, b.i[1]);
	sparc_movi2f(c, dest+1, _g1);
    } else {
	/* double sparcv9 */
	b.d = imm;
	sparc_set(c, _g1, b.l);
	sparc_movi2d(c, dest, _g1);
    }
}	

#define SIMM34_P(im) (((long)im) < (long)1<<33 ) && (((long)im) >= -((long)1<<33) )
/* upper 22 of 64 bits */
#define hh(x) (((unsigned long) x) >> 42)
/* bits 11-32 */
#define lm(x) hi((x) & (((unsigned long)1<<32) -1))
/* lower 10 of upper 32 */
#define hm(x) lo( ((unsigned long) x)>>32)

/* upper 22 bits of 32 */
#define hi(x) (((unsigned long)x) >> 10)
/* lower 10 bits */
#define lo(x) ((x) & ((1 << 11) - 1))

extern void
sparc_set(c, r, val)
drisc_ctx c;
int r;
long val;
{
    if  (((long)val) < 4096 && ((long)val) >= -4096) {
	sparc_ori(c, r, _g0, (val&0x1fff));
#if SIZEOF_LONG == 8
    } else if ((val >> 34) == 0) {
	/* fits in 34 positive bits */
	sparc_sethi(c, r, val>>12);
	sparc_lshi(c, r,r,2);
	sparc_ori(c, r,r,val & 0xfff);
    } else if ((val >> 34) == -1) {
	/* fits in 34 negative bits */
	sparc_sethi(c, r, ~(val>>12));
	sparc_lshi(c, r,r,2);
	sparc_xori(c, r,r,(val & 0xfff)|0x1000);
    } else {
	if (r != _g1) {
	    /* case where we can use g1 as a temporary */
	    sparc_sethi(c, r,hh(val));
	    sparc_sethi(c, _g1,lm(val));
/*	    printf("lm (%lx) = %lx\n", val, lm(val));*/
	    sparc_ori(c, r,r,hm(val));
	    sparc_ori(c, _g1,_g1,lo(val));
	    sparc_xlshi(c, r,r,32);
	    sparc_or(c, r,r,_g1);
	    /* can't use g1! */
	} else {
	    /* set hi, set low */
	    sparc_sethi(c, r, hh(val));
	    sparc_ori(c, r, r, hm(val));
	    if ((val & 0xFF000000) != 0) {
		/* top 8 of low 32 are non-zero */
		sparc_lshi(c, r,r,8);
		sparc_ori(c, r, r, (val & 0xFF000000) >> 24);
		sparc_lshi(c, r,r,12);
	    } else {
		sparc_lshi(c, r,r, 20);
	    }
	    sparc_ori(c, r, r, (val & 0x00fff000) >> 12);
	    sparc_lshi(c, r,r,12);
	    sparc_ori(c, r, r, val & 0xfff);
	}
    }
#else
    } else if (! (val & 0x3ff)) {
	/* no low bits */
	sparc_sethi(c, r, ((val >> 10) & 0x3fffff));
    } else {
	sparc_sethi(c, r, ((val >> 10) & 0x3fffff));
	sparc_ori(c, r, r, (val & 0x3ff));
    }        
#endif
}

#define bit_R(x) ((unsigned long)1<<x)

extern void
sparc_reg_init(drisc_ctx c)
{
    c->p->var_i.init_avail[0] = (bit_R(_l0)|bit_R(_l1)|bit_R(_l2)|bit_R(_l3)|
				 bit_R(_l4)|bit_R(_l5)|bit_R(_l6)|bit_R(_l7));
    c->p->var_i.members[0] = c->p->var_i.init_avail[0] |
	(bit_R(_i0)|bit_R(_i1)|bit_R(_i2)|bit_R(_i3)|
	 bit_R(_i4)|bit_R(_i5)|bit_R(_i6)|bit_R(_i7));
    c->p->tmp_i.init_avail[0] = (bit_R(_g2)|bit_R(_g3));
    c->p->tmp_i.members[0] = c->p->tmp_i.init_avail[0] | bit_R(_g1);
    c->p->var_f.init_avail[0] = 0;
    c->p->var_f.members[0] = c->p->var_f.init_avail[0];
    c->p->tmp_f.init_avail[0] = (bit_R(_f2)|bit_R(_f4)|bit_R(_f6)|
				 bit_R(_f8)|bit_R(_f10)|bit_R(_f12)|bit_R(_f14)|
				 bit_R(_f16)|bit_R(_f18)|bit_R(_f20)|bit_R(_f22)|
				 bit_R(_f24)|bit_R(_f26)|bit_R(_f28)|bit_R(_f30));
    c->p->tmp_f.members[0] = c->p->tmp_f.init_avail[0];
}

extern void*
gen_sparc_mach_info(c, v9)
drisc_ctx c;
int v9;
{
    sparc_mach_info smi = malloc(sizeof(*smi));
    if (c->p->mach_info != NULL) {
	free(c->p->mach_info);
	c->p->mach_info = NULL;
	c->p->native.mach_info = NULL;
    }
    sparc_reg_init(c);
    smi->act_rec_size = 0;
    smi->conversion_word = 0;
    smi->cur_arg_offset = 0;
    if (v9) {
	smi->stack_align = 8; /* 8 for sparcv9 */
	smi->stack_constant_offset = 2047; 
    } else {
	smi->stack_align = 4; /* 8 for sparcv9 */
	smi->stack_constant_offset = 0; /* 2047 for sparcv9 */
    }
    smi->gp_save_offset = (16 + 1 + 6 + 19 /* args */) * smi->stack_align; /*184;*/
    smi->fp_save_offset = smi->gp_save_offset + 8 * smi->stack_align;
    smi->fp_save_end = smi->fp_save_offset + 32 * smi->stack_align + 16;
    return smi;
}

#if defined(HAVE_DIS_ASM_H) && !defined(NO_DISASSEMBLER)
/* GENERIC BINUTILS DISASSEMBLER */
#include "dis-asm.h"

#define MAXLENGTH (1<<23) /* Max length of function that can be disassembled */

extern int
sparc_init_disassembly_info(drisc_ctx c, void * ptr)
{
    struct disassemble_info *i = ptr;
#ifdef INIT_DISASSEMBLE_INFO_THREE_ARG
    INIT_DISASSEMBLE_INFO(*i, stdout,fprintf);
    i->endian = BFD_ENDIAN_BIG;
#else
    INIT_DISASSEMBLE_INFO(*i, stdout);
#endif
    i->mach = bfd_mach_sparc_v9;
    if (c->p->code_base != NULL) {
	i->buffer = (bfd_byte *)c->p->code_base;
	i->buffer_vma = (bfd_vma)(long)c->p->code_base;
    } else {
	i->buffer = (bfd_byte *)c->p->native.code_base;
	i->buffer_vma = (bfd_vma)(long)c->p->native.code_base;
    }
    i->buffer_length = MAXLENGTH;
#ifdef HAVE_PRINT_INSN_SPARC
    return 1;
#else
    return 0;
#endif
}

extern int
sparc_print_insn(drisc_ctx c, void *info_ptr, void *insn)
{
#ifdef HAVE_PRINT_INSN_SPARC
    return print_insn_sparc((unsigned long) insn, (disassemble_info*)info_ptr);
#else
    return 0;
#endif
}
#else
extern int
sparc_init_disassembly_info(drisc_ctx c, void * ptr){return 0;}
extern int sparc_print_insn(drisc_ctx c, void *info_ptr, void *insn){return 0;}
#endif

extern void
sparc_print_reg(drisc_ctx c, int typ, int reg)
{
    switch(typ) {
    case DR_C: case DR_UC:
    case DR_S: case DR_US:
    case DR_I: case DR_U: case DR_L: case DR_UL:
	if (reg == _fp) {
	    printf("fp");
	    return;
	} else if (reg == _ra) {
	    printf("ra");
	    return;
	} else if (reg == _sp) {
	    printf("sp");
	    return;
	} else if (reg <= _g7) {
	    printf("g%d\n", reg - _g0);
	    return;
	} else if (reg <= _o7) {
	    printf("o%d\n", reg - _o0);
	    return;
	} else if (reg <= _l7) {
	    printf("l%d\n", reg - _l0);
	    return;
	} else if (reg <= _i7) {
	    printf("i%d\n", reg - _i0);
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
sparc_count_insn(drisc_ctx c, int start, int end)
{
    return (end - start)>>2;
}
