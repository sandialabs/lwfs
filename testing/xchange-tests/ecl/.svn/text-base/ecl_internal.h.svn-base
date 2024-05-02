typedef struct {
    int line;
    int character;
} srcpos;

typedef struct {
    srcpos lx_srcpos;
    char *string;
} lx_info;

typedef struct {
    int static_size;
    sm_ref control_field;
} dimen_s;

typedef struct {
    int dimen_count;
    dimen_s dimens[1];
} *dimen_p;

typedef enum {op_modulus, op_plus, op_minus, op_leq, op_lt, op_geq, op_gt, op_eq, op_neq, op_log_neg, op_log_or, op_log_and, op_arith_and, op_arith_or, op_arith_xor, op_left_shift, op_right_shift, op_mult, op_div, op_deref, op_inc, op_dec, op_address, op_sizeof} operator_t;

extern void ecl_print_operator_t(operator_t o);

extern void ecl_print_srcpos(srcpos pos);

extern void ecl_print_dimen_p(dimen_p dimen);

extern void *ecl_cg_net(sm_ref net, int ret_type, unsigned int *offset_p,
			ecl_code code_descriptor);
extern int ecl_sm_get_type(sm_ref node);
extern int ecl_expr_is_string(sm_ref expr);
extern int is_control_value(sm_ref expr, sm_ref strct);
extern sm_ref get_complex_type(ecl_parse_context context, sm_ref node);
extern int are_compatible_ptrs(sm_ref left, sm_ref right);
extern void ecl_add_standard_elements(ecl_parse_context context);
extern void ecl_add_defined_type(char *id, ecl_parse_context context);
extern void ecl_add_decl_to_scope(char *id, sm_ref node, ecl_parse_context context);
extern int ecl_semanticize_added_decls(ecl_parse_context context);
extern void ecl_swap_decls_to_standard(ecl_parse_context context);
