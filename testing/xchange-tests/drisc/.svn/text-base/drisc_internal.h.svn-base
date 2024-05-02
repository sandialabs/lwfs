extern void expand_drisc_buffer(drisc_ctx c);
extern void dump_cur_drisc_insn(drisc_ctx c);
extern jmp_table alloc_drisc_jump_table(void);

#define INIT_CODE_SIZE 64

typedef struct arg_info { 
    char type;
    char is_register;		/* true if parameter is in register */
    char is_immediate;		/* true if actual is an immediate */
    unsigned in_reg;		/* callee register it's in */
    unsigned out_reg;		/* caller register it's in */
    int offset;			/* otherwise at this offset from v_pp */
} *arg_info_list;

typedef struct reg_set {
    long init_avail[1];
    long members[1];
    long avail[1];
    long used[1];
    long mustsave[1];
} reg_set;

struct branch_location {
    int label;
    int loc;
};

struct data_mark {
    long *addr;
    int label;
};

struct branch_table {
    int next_label;
    int max_alloc;
    int *label_locs;
    int branch_count;
    int branch_alloc;
    struct branch_location *branch_locs;
    int data_mark_count;
    struct data_mark *data_marks;
};

typedef struct branch_table *branch_t;


struct call_location {
    int loc;
    void *xfer_addr;
    char *xfer_name;
    void *mach_info;
};

typedef struct call_table {
    int call_alloc;
    int call_count;
    struct call_location *call_locs;
} call_t;

typedef struct ret_table {
    int ret_alloc;
    int ret_count;
    int *ret_locs;
} ret_t;

typedef void (*mach_reset_func)(drisc_ctx c);

typedef struct vreg_info {
    int typ;
    int preg;
    int offset;
    int need_assign;
    struct {
	short use_count;
	short def_count;
    } use_info;

    int assign_loc;
    int in_reg;
    int last_use;
    int use_metric;
    int update_in_reg;
    int value_in_mem;
} vreg_info;

typedef struct preg_info {
    int holds;
} preg_info;

typedef struct saved {
    jmp_table mach_jump;
    mach_reset_func mach_reset;
    void *mach_info;
    void *code_base;
    void *cur_ip;
    void *code_limit;
} saved_insn_info;

struct drisc_private_ctx {
    void *code_base;
    void *cur_ip;
    void *code_limit;
    void *fp;
    void *fpc;
    int ret_type;
    struct branch_table branch_table;
    struct call_table call_table;
    struct ret_table ret_table;
    mach_reset_func mach_reset;
    mach_reset_func native_mach_reset;
    saved_insn_info native;
    saved_insn_info virtual;
    void *mach_info;
    int machine_strr_tmp_reg;
    reg_set var_i;
    reg_set tmp_i;
    reg_set var_f;
    reg_set tmp_f;
    int c_param_count;
    int save_param_count;
    dr_reg_t **c_param_regs;
    arg_info_list c_param_args;
    drisc_parameter_type **c_param_structs;
    int doing_reverse_push;
    int unavail_called;

    int vreg_count;
    vreg_info *vregs;
    int v_tmps[DR_B][3];
};

extern int drisc_mustsave(reg_set *regs, int reg);
extern int drisc_wasused(reg_set *regs, int reg);
extern void dr_mark_branch_location(drisc_ctx c, int label);
extern void dr_mark_call_location(drisc_ctx c, char *xfer_name,
				 void *xfer_address);
extern void dr_mark_ret_location(drisc_ctx c);
extern int dr_do_reverse_vararg_push(drisc_ctx c);
extern void dr_end_vararg_push(drisc_ctx c);
extern void dr_dump_reg(drisc_ctx c, int typ, int reg);
extern void setup_VM_proc(drisc_ctx c);
