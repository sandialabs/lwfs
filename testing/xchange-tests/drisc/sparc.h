#ifndef _SPARC_H
#define _SPARC_H
 #define OP2(x)		(((x)&0x7) << 22) /* op2 field of format2 insns */
 #define OP3(x)		(((x)&0x3f) << 19) /* op3 field of format3 insns */
 #define HDR(x)		((unsigned)((x)&0x3) << 30) /* hdr field of all insns */
 #define ASI(asi) 	((asi) << 5)
 #define OPF(x)		(((x)&0x1ff) << 5) /* opf field of float insns */
 #define RD(x)		(((x)&0x1f) << 25) /* destination register field */
 #define RS1(x)		(((x)&0x1f) << 14) /* rs1 field */
 #define RS2(x)		((x)&0x1f) /* rs2 field */
 #define SIMM13(x)	((x)&0x1fff) /* simm13 field */
 #define IM		0x2000
 #define COND(x) (((x)&0xf)<<25)
 #define CC(x)  (((x)&0x3)<<20)
 #define P(x)  (((x)&0x1)<<19)
extern void sparc_FORM3_arith(drisc_ctx c, int op3, int op, int dest, int src1, int src2);
extern void sparc_FORM3imm_arith(drisc_ctx c, int op3, int op, int dest, int src1, long imm);
extern void sparc_FORM3_farith(drisc_ctx c, int op3, int op, int dest, int src1, int src2);
extern void sparc_FORM2_farith(drisc_ctx c, int op3, int op, int dest, int src);

#define INSN_OUT(c, insn) do {\
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(int*)c->p->cur_ip = (unsigned int)insn;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = (void*)(((long)c->p->cur_ip)+4);\
} while (0)\

enum {
    _g0,  _g1,  _g2,  _g3,  _g4,  _g5,  _g6,  _g7, /* globals */
    _o0,  _o1,  _o2,  _o3,  _o4,  _o5,  _o6,  _o7, /* outs */
    _l0,  _l1,  _l2,  _l3,  _l4,  _l5,  _l6,  _l7, /* locals */
    _i0,  _i1,  _i2,  _i3,  _i4,  _i5,  _i6,  _i7, /* ins */

    _fp = _i6,	/* framepointer */
    _ra = _i7,	/* return address */
    _sp = _o6,	/* stack pointer */

    /* floating point */
    _f0=0,  _f1,  _f2,  _f3,  _f4,  _f5,  _f6,  _f7, /* floats */
    _f8,    _f9,  _f10, _f11, _f12, _f13, _f14, _f15,
    _f16,   _f17, _f18, _f19, _f20, _f21, _f22, _f23,
    _f24,   _f25, _f26, _f27, _f28, _f29, _f30, _f31
};

typedef struct sparc_mach_info {
    int act_rec_size;
    int stack_align;
    int stack_constant_offset;
    int gp_save_offset;
    int fp_save_offset;
    int fp_save_end;
    int conversion_word;
    int cur_arg_offset;
    int save_insn_offset;
} *sparc_mach_info;

extern int sparc_type_align[];
extern int sparc_type_size[];
extern void *gen_sparc_mach_info(drisc_ctx c, int v9);
extern void sparc_set(drisc_ctx c, int r, long imm);
extern void sparc_proc_start(drisc_ctx c, char *subr_name, int arg_count, 
			 arg_info_list args, dr_reg_t *arglist);
extern void sparc_end(drisc_ctx c);
extern void * sparc_clone_code(drisc_ctx c, void *new_base, int available_size);
extern void sparc_ret(drisc_ctx c, int data1, int data2, int src);
extern void sparc_reti(drisc_ctx c, int data1, int data2, long imm);
extern int sparc_getreg(drisc_ctx c, dr_reg_t *reg_p, int type, int class);
extern int sparc_putreg(drisc_ctx c, dr_reg_t reg, int type);
extern void
sparc_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
sparc_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
sparc_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
sparc_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
sparc_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
sparc_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
sparc_modi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
sparc_mod(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
sparc_divi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
sparc_div(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
sparc_converti(drisc_ctx c, int from_type, int to_type, int dest, long src);
extern void
sparc_convert(drisc_ctx c, int from_type, int to_type, int dest, int src);
extern void
sparc_mov(drisc_ctx c, int type, int junk, int dest, int src);
extern void
sparc_pset(drisc_ctx c, int type, int junk, int dest, long imm);
extern void
sparc_setf(drisc_ctx c, int type, int junk, int dest, double imm);
extern void
sparc_branch(drisc_ctx c, int op, int type, int src1, int src2, int label);
extern void
sparc_branchi(drisc_ctx c, int op, int type, int src, long imm, int label);
extern void 
sparc_lea(drisc_ctx c, int junk, int junk1, int dest, int src, long imm);
extern void sparc_jump_to_label(drisc_ctx c, unsigned long label);
extern void sparc_jump_to_reg(drisc_ctx c, unsigned long reg);
extern void sparc_jump_to_imm(drisc_ctx c, unsigned long imm);
extern void sparc_jal(drisc_ctx c, int return_addr_reg, int target);
extern int sparc_calli(drisc_ctx c, int type, void *xfer_address);
extern int sparc_callr(drisc_ctx c, int type, int src);
extern void sparc_push(drisc_ctx c, int type, int reg);
extern void sparc_pushi(drisc_ctx c, int type, long value);
extern void sparc_pushfi(drisc_ctx c, int type, double value);
extern int sparc_local_op(drisc_ctx c, int flag, int val);
extern int sparc_local(drisc_ctx c, int type);
extern int sparc_localb(drisc_ctx c, int size);
extern void sparc_save_restore_op(drisc_ctx c, int save_restore, int type,
				 int reg);
extern int sparc_init_disassembly_info(drisc_ctx c, void * ptr);
extern int sparc_print_insn(drisc_ctx c, void *info_ptr, void *insn);
extern int sparc_count_insn(drisc_ctx c, int start, int end);
extern void sparc_print_reg(drisc_ctx c, int typ, int reg);
#endif
