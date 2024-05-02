#ifndef _ARM5_H
#define _ARM5_H
enum arm_cond {EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL, NV};
enum arm_opcode {AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN};

extern void arm_dproc(drisc_ctx c, int op, int shift_code, int dest, int src1, int src2);
extern void arm_dproc2(drisc_ctx c, int op3, int op, int dest, int src);
extern void arm_dproci(drisc_ctx c, int op, int shift_code, int dest, int src1, long imm);
extern void arm_fproc(drisc_ctx c, int op3, int op, int dest, int src1, int src2);
extern void arm_fproc2(drisc_ctx c, int op3, int op, int dest, int src);
extern void arm_fproci(drisc_ctx c, int op3, int op, int dest, int src1, long imm);

enum {
    _r0,  _r1,  _r2,  _r3,  _r4,  _r5,  _r6,  _r7, /* globals */
    _r8,  _r9,  _r10,  _r11,  _r12,  _r13,  _r14,  _r15, /* globals */

    _sp = _r13,	/* stack pointer */
    _link = _r14,	/* link address */
    _pc = _r15,	/* program counter */
    _fp = _r11,
    _a1 = _r0, _a2 = _r1, _a3 = _r2, _a4 = _r3,
    _v1 = _r4, _v2 = _r5, _v3 = _r6, _v4 = _r7, _v5 = _r8, _v6 = _r9, 
    _v7 = _r10,

    /* floating point */
    _f0=0,  _f1,  _f2,  _f3,  _f4,  _f5,  _f6,  _f7
};

#define LLshift 0x10
#define LRshift 0x11
#define ARshift 0x12

typedef struct arm_mach_info {
    int act_rec_size;
    int stack_align;
    int stack_constant_offset;
    int gp_save_offset;
    int fp_save_offset;
    int fp_save_end;
    int conversion_word;
    int cur_arg_offset;
    int save_insn_offset;
    int max_arg_size;
} *arm_mach_info;

extern int arm_type_align[];
extern int arm_type_size[];
extern void *gen_arm_mach_info(drisc_ctx c, int v9);
extern void arm_set(drisc_ctx c, int r, long imm);
extern void arm_proc_start(drisc_ctx c, char *subr_name, int arg_count, 
			 arg_info_list args, dr_reg_t *arglist);
extern void arm_end(drisc_ctx c);
extern void *arm_clone_code(drisc_ctx c, void *base, int size);
extern void arm_ret(drisc_ctx c, int data1, int data2, int src);
extern void arm_reti(drisc_ctx c, int data1, int data2, long imm);
extern int arm_getreg(drisc_ctx c, dr_reg_t *reg_p, int type, int class);
extern int arm_putreg(drisc_ctx c, dr_reg_t reg, int type);
extern void
arm_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
arm_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
arm_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
arm_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
arm_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
arm_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
arm_modi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
arm_mod(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
arm_divi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
arm_mul(drisc_ctx c, int unsign, int junk, int dest, int src1, int src2);
extern void
arm_muli(drisc_ctx c, int unsign, int junk, int dest, int src, long imm);
extern void
arm_div(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
arm_convert(drisc_ctx c, int from_type, int to_type, int dest, int src);
extern void
arm_mov(drisc_ctx c, int type, int junk, int dest, int src);
extern void
arm_pset(drisc_ctx c, int type, int junk, int dest, long imm);
extern void
arm_setf(drisc_ctx c, int type, int junk, int dest, double imm);
extern void
arm_branch(drisc_ctx c, int op, int type, int src1, int src2, int label);
extern void
arm_branchi(drisc_ctx c, int op, int type, int src, long imm, int label);
extern void 
arm_lea(drisc_ctx c, int junk, int junk1, int dest, int src, long imm);
extern void arm_jump_to_label(drisc_ctx c, unsigned long label);
extern void arm_jump_to_reg(drisc_ctx c, unsigned long reg);
extern void arm_jump_to_imm(drisc_ctx c, unsigned long imm);
extern void arm_jal(drisc_ctx c, int return_addr_reg, int target);
extern int arm_calli(drisc_ctx c, int type, void *xfer_address);
extern int arm_callr(drisc_ctx c, int type, int src);
extern void arm_push(drisc_ctx c, int type, int reg);
extern void arm_pushi(drisc_ctx c, int type, long value);
extern void arm_pushfi(drisc_ctx c, int type, double value);
extern int arm_local_op(drisc_ctx c, int flag, int val);
extern int arm_local(drisc_ctx c, int type);
extern int arm_localb(drisc_ctx c, int size);
extern void arm_save_restore_op(drisc_ctx c, int save_restore, int type,
				 int reg);
extern int arm_init_disassembly_info(drisc_ctx c, void * ptr);
extern int arm_print_insn(drisc_ctx c, void *info_ptr, void *insn);
extern int arm_count_insn(drisc_ctx c, int start, int end);
extern void arm_print_reg(drisc_ctx c, int typ, int reg);
#endif
