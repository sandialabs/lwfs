#ifndef _X86__64_H
#define _X86_64_H

#define BYTE_OUT1(c, insn1) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+1

#define BYTE_OUT2(c, insn1, insn2) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+2

#define BYTE_OUT3(c, insn1, insn2, insn3) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(((unsigned char*)c->p->cur_ip) + 2)= (unsigned char)insn3;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+3

#define BYTE_OUT4(c, insn1, insn2, insn3, insn4) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(((unsigned char*)c->p->cur_ip) + 2)= (unsigned char)insn3;\
*(((unsigned char*)c->p->cur_ip) + 3)= (unsigned char)insn4;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+4

#define BYTE_OUT5(c, insn1, insn2, insn3, insn4, insn5) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(((unsigned char*)c->p->cur_ip) + 2)= (unsigned char)insn3;\
*(((unsigned char*)c->p->cur_ip) + 3)= (unsigned char)insn4;\
*(((unsigned char*)c->p->cur_ip) + 4)= (unsigned char)insn5;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+5

#define BYTE_OUT6(c, insn1, insn2, insn3, insn4, insn5, insn6) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(((unsigned char*)c->p->cur_ip) + 2)= (unsigned char)insn3;\
*(((unsigned char*)c->p->cur_ip) + 3)= (unsigned char)insn4;\
*(((unsigned char*)c->p->cur_ip) + 4)= (unsigned char)insn5;\
*(((unsigned char*)c->p->cur_ip) + 5)= (unsigned char)insn6;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+6

#define BYTE_OUT3I(c, insn1, insn2, insn3, imm32) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(((unsigned char*)c->p->cur_ip) + 2)= (unsigned char)insn3;\
*(unsigned int *)(((unsigned char*)c->p->cur_ip) + 3)= (unsigned int)imm32;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+7

#define BYTE_OUT3S(c, insn1, insn2, insn3, imm16) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(((unsigned char*)c->p->cur_ip) + 2)= (unsigned char)insn3;\
*(unsigned short *)(((unsigned char*)c->p->cur_ip) + 3)= (unsigned short)imm16;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+5

#define BYTE_OUT1I(c, insn1, imm32) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(unsigned int *)(((unsigned char*)c->p->cur_ip) + 1)= (unsigned int)imm32;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+5

#define BYTE_OUT2I(c, insn1, insn2,imm32) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(unsigned int *)(((unsigned char*)c->p->cur_ip) + 2)= (unsigned int)imm32;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+6

#define BYTE_OUT2II(c, insn1, insn2,imm32, imm32_2) \
if (c->p->cur_ip >= c->p->code_limit) {\
   expand_drisc_buffer(c);\
}\
*(unsigned char*)c->p->cur_ip = (unsigned char)insn1;\
*(((unsigned char*)c->p->cur_ip) + 1)= (unsigned char)insn2;\
*(unsigned int *)(((unsigned char*)c->p->cur_ip) + 2)= (unsigned int)imm32;\
*(unsigned int *)(((unsigned char*)c->p->cur_ip) + 6)= (unsigned int)imm32_2;\
if (c->drisc_debug) dump_cur_drisc_insn(c);\
c->p->cur_ip = ((char*)c->p->cur_ip)+10

#ifdef LINUX_KERNEL_MODULE
#undef EAX
#undef ECX
#undef EDX
#undef EBX
#undef ESP
#undef EBP
#undef ESI
#undef EDI
#endif

enum {
    AL = 0, CL, DL, BL, AH, CH, DH, BH,   /* r8 */
    AX = 0, CX, DX, BX, SP, BP, SI, DI,   /* r16 */
    EAX = 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI,   /* r32 */
    RAX = 0, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15,   /* r64 */
    XMM0 = 0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15
};

#define REX_W 0x8
#define REX_R 0x4
#define REX_X 0x2
#define REX_B 0x1

typedef struct x86_64_mach_info {
    int act_rec_size;
    int stack_align;
    int stack_constant_offset;
    int conversion_word;
    int fcu_word;
    int save_base;
    int cur_arg_offset;
    int int_arg_count;
    int float_arg_offset;
    int float_arg_count;
    int backpatch_offset;
    int call_backpatch_offset;
    int call_stack_space;
    char pending_prefix;
} *x86_64_mach_info;

extern int x86_64_type_align[];
extern int x86_64_type_size[];
extern void *gen_x86_64_mach_info();
extern void x86_64_arith3(drisc_ctx c, int op, int commut, int dest, int src1, int src2);
extern void x86_64_arith2(drisc_ctx c, int op, int subop, int dest, int src);
extern void x86_64_mul(drisc_ctx c, int signed, int imm, int dest, int src1, long src2);

extern void x86_64_div_mod(drisc_ctx c, int sign, int div, int dest, int src1, int src2);
extern void x86_64_div_modi(drisc_ctx c, int sign, int div, int dest, int src1, long imm);
extern void x86_64_arith3i(drisc_ctx c, int op, int commut, int dest, int src1, long src2);
extern void x86_64_shift(drisc_ctx c, int op, int junk, int dest, int src1, int src2);
extern void x86_64_shifti(drisc_ctx c, int op, int junk, int dest, int src, long imm);
extern void x86_64_special(drisc_ctx c, special_operations type, long param);
extern void x86_64_set(drisc_ctx c, int r, long imm);
extern void x86_64_proc_start(drisc_ctx c, char *subr_name, int arg_count, 
			   arg_info_list args, dr_reg_t *arglist);
extern void x86_64_end(drisc_ctx c);
extern void *x86_64_clone_code(drisc_ctx c, void *base, int size);
extern void x86_64_ret(drisc_ctx c, int data1, int data2, int src);
extern void x86_64_reti(drisc_ctx c, int data1, int data2, long imm);
extern void x86_64_retf(drisc_ctx c, int data1, int data2, double imm);
extern int x86_64_getreg(drisc_ctx c, dr_reg_t *reg_p, int type, int class);
extern int x86_64_putreg(drisc_ctx c, dr_reg_t reg, int type);
extern void
x86_64_ploadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
x86_64_pload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
x86_64_pbsloadi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
x86_64_pbsload(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
x86_64_pstorei(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
x86_64_pstore(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
x86_64_modi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
x86_64_mod(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
x86_64_divi(drisc_ctx c, int type, int junk, int dest, int src, long offset);
extern void
x86_64_div(drisc_ctx c, int type, int junk, int dest, int src1, int src2);
extern void
x86_64_converti(drisc_ctx c, int from_type, int to_type, int dest, long src);
extern void
x86_64_convert(drisc_ctx c, int from_type, int to_type, int dest, int src);
extern void
x86_64_mov(drisc_ctx c, int type, int junk, int dest, int src);
extern void
x86_64_pset(drisc_ctx c, int type, int junk, int dest, long imm);
extern void
x86_64_setf(drisc_ctx c, int type, int junk, int dest, double imm);
extern void
x86_64_branch(drisc_ctx c, int op, int type, int src1, int src2, int label);
extern void
x86_64_branchi(drisc_ctx c, int op, int type, int src, long imm, int label);
extern void 
x86_64_lea(drisc_ctx c, int junk, int junk1, int dest, int src, long imm);
extern void 
x86_64_farith(drisc_ctx c, int op, int typ, int dest, int src1, int src2);
extern void 
x86_64_farith2(drisc_ctx c, int op, int typ, int dest, int src);
extern void x86_64_jump_to_label(drisc_ctx c, unsigned long label);
extern void x86_64_jump_to_reg(drisc_ctx c, unsigned long reg);
extern void x86_64_jump_to_imm(drisc_ctx c, unsigned long imm);
extern void x86_64_jal(drisc_ctx c, int return_addr_reg, int target);
extern int x86_64_calli(drisc_ctx c, int type, void *xfer_address);
extern int x86_64_callr(drisc_ctx c, int type, int src);
extern void x86_64_push(drisc_ctx c, int type, int reg);
extern void x86_64_pushi(drisc_ctx c, int type, long value);
extern void x86_64_pushfi(drisc_ctx c, int type, double value);
extern int x86_64_local_op(drisc_ctx c, int flag, int val);
extern int x86_64_local(drisc_ctx c, int type);
extern int x86_64_localb(drisc_ctx c, int size);
extern void x86_64_save_restore_op(drisc_ctx c, int save_restore, int type,
				 int reg);
extern int x86_64_init_disassembly_info(drisc_ctx c, void * ptr);
extern int x86_64_print_insn(drisc_ctx c, void *info_ptr, void *insn);
extern int x86_64_count_insn(drisc_ctx c, int start, int end);
extern void x86_64_print_reg(drisc_ctx c, int typ, int reg);
#endif
