#ifndef _IA64_H
#define _IA64_H

extern void ia64_farith(drisc_ctx c, int op3, int op, int dest, int src1, int src2);
extern void ia64_fdiv(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void ia64_farith2(drisc_ctx c, int op3, int op, int dest, int src);

#define BYTE_OUT1(c, insn1)  printf("Bad opout, line %d\n", __LINE__)


#define BYTE_OUT2(c, insn1, insn2)  printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT3(c, insn1, insn2, insn3) printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT4(c, insn1, insn2, insn3, insn4)  printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT3I(c, insn1, insn2, insn3, imm32) printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT3S(c, insn1, insn2, insn3, imm16) printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT1I(c, insn1, imm32) printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT2I(c, insn1, insn2,imm32) printf("Bad opout, line %d\n", __LINE__)

#define BYTE_OUT2II(c, insn1, insn2,imm32, imm32_2) printf("Bad opout, line %d\n", __LINE__)

enum {
    R0, R1, R2, R3, R4, R5, R6, R7, R8, R9,
    R10, R11, SP=12, TP=13, R14, R15, R16, R17, R18, R19,
    R20, R21, R22, R23, R24, R25, R26, R27, R28, R29,
    R30, R31, 
    IN0, IN1, IN2, IN3, IN4, IN5, IN6, IN7, 
    L0, L1, L2, L3, L4, L5, L6, L7,
    OUT0 = 48, OUT1, OUT2, OUT3, OUT4, OUT5, OUT6, OUT7, OUT8, OUT9,
    F0=0, F1, F2, F3, F4, F5, F6, F7, F8, F9,
    F10, F11, F12, F13, F14, F15, F16, F17, F18, F19,
    F20, F21, F22, F23, F24, F25, F26, F27, F28, F29,
    F30, F31,
    B0 = 0, B1, B2, B3, B4, B5, B6,
    P0 = 0, P1, P2, P3, P4, P5, P6, P7};


typedef struct ia64_mach_info {
    int act_rec_size;
    int stack_align;
    int stack_constant_offset;
    int conversion_word;
    int fcu_word;
    int save_base;
    int cur_arg_offset;
    int max_arg_offset;
    int float_count;
    int backpatch_offset;
    int frame_reg;
    int branch_reg;
} *ia64_mach_info;

extern int ia64_type_align[];
extern int ia64_type_size[];
extern void *gen_ia64_mach_info();
extern void ia64_arith3(drisc_ctx c, int op, int commut, int dest, int src1, int src2);
extern void ia64_arith2(drisc_ctx c, int op, int subop, int dest, int src);
extern void ia64_mul(drisc_ctx c, int signed, int imm, int dest, int src1, int src2);
extern void ia64_muli(drisc_ctx c, int signed, int i, int dest, int src, long imm);

extern void ia64_div_mod(drisc_ctx c, int sign, int div, int dest, int src1, int src2);
extern void ia64_div_modi(drisc_ctx c, int sign, int div, int dest, int src1, long imm);
extern void ia64_arith3i(drisc_ctx c, int op, int commut, int dest, int src1, long src2);
extern void ia64_shift(drisc_ctx c, int op, int junk, int dest, int src1, int src2);
extern void ia64_shifti(drisc_ctx c, int op, int junk, int dest, int src, long imm);
extern void ia64_set(drisc_ctx c, int r, long imm);
extern void ia64_proc_start(drisc_ctx c, char *subr_name, int arg_count, 
			   arg_info_list args, dr_reg_t *arglist);
extern void ia64_end(drisc_ctx c);
extern void *ia64_clone_code(drisc_ctx c, void *base, int size);
extern void ia64_ret(drisc_ctx c, int data1, int data2, int src);
extern void ia64_reti(drisc_ctx c, int data1, int data2, long imm);
extern void ia64_retf(drisc_ctx c, int data1, int data2, double imm);
extern int ia64_getreg(drisc_ctx c, dr_reg_t *reg_p, int type, int class);
extern int ia64_putreg(drisc_ctx c, dr_reg_t reg, int type);
extern void
ia64_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
ia64_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
ia64_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
ia64_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
ia64_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
ia64_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
ia64_modi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
ia64_mod(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
ia64_divi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
ia64_div(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
ia64_converti(drisc_ctx c, int from_type, int to_type, int dest, long src);
extern void
ia64_convert(drisc_ctx c, int from_type, int to_type, int dest, int src);
extern void
ia64_mov(drisc_ctx c, int type, int junk, int dest, int src);
extern void
ia64_pset(drisc_ctx c, int type, int junk, int dest, long imm);
extern void
ia64_setf(drisc_ctx c, int type, int junk, int dest, double imm);
extern void
ia64_branch(drisc_ctx c, int op, int type, int src1, int src2, int label);
extern void
ia64_branchi(drisc_ctx c, int op, int type, int src, long imm, int label);
extern void 
ia64_lea(drisc_ctx c, int junk, int junk1, int dest, int src, long imm);
extern void ia64_jump_to_label(drisc_ctx c, unsigned long label);
extern void ia64_jump_to_reg(drisc_ctx c, unsigned long reg);
extern void ia64_jump_to_imm(drisc_ctx c, unsigned long imm);
extern void ia64_jal(drisc_ctx c, int return_addr_reg, int target);
extern int ia64_calli(drisc_ctx c, int type, void *xfer_address);
extern int ia64_callr(drisc_ctx c, int type, int src);
extern void ia64_push(drisc_ctx c, int type, int reg);
extern void ia64_pushi(drisc_ctx c, int type, long value);
extern void ia64_pushfi(drisc_ctx c, int type, double value);
extern int ia64_local_op(drisc_ctx c, int flag, int val);
extern int ia64_local(drisc_ctx c, int type);
extern int ia64_localb(drisc_ctx c, int size);
extern void ia64_save_restore_op(drisc_ctx c, int save_restore, int type,
				 int reg);
extern int ia64_init_disassembly_info(drisc_ctx c, void * ptr);
extern int ia64_print_insn(drisc_ctx c, void *info_ptr, void *insn);
extern int ia64_count_insn(drisc_ctx c, int start, int end);
extern void ia64_print_reg(drisc_ctx c, int typ, int reg);
#endif
