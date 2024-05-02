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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif
#ifdef HAVE_DRISC_H
#include "drisc.h"
#define dr_reg_type int
#define dr_label_type int
#else
#ifdef HAVE_ICODE_H
#include "drisc_icode.h"
#include "icode.h"
#define dr_reg_type i_local_t
#define dr_label_type i_label_t
#else
/*stuff*/
#define dr_reg_type int
#define dr_label_type int
#define drisc_ctx void *
enum {
    DR_C,    /* char */
    DR_UC,   /* unsigned char */
    DR_S,    /* short */
    DR_US,   /* unsigned short */
    DR_I,    /* int */
    DR_U,    /* unsigned */
    DR_L,    /* long */
    DR_UL,   /* unsigned long */
    DR_P,    /* pointer */
    DR_F,    /* floating */
    DR_D,    /* double */
    DR_V,    /* void */
    DR_B,    /* block structure */
    DR_ERR   /* no type */
};
#define dr_type_size(c, t)  0
#endif
#endif
#include "ecl.h"
#include "ecl_internal.h"
#include "structs.h"
#include "assert.h"
#ifndef LINUX_KERNEL_MODULE
#include "string.h"
#endif
#include "ecl.tab.h"
#ifndef FALSE
#define FALSE 0
#endif

#ifdef LINUX_KERNEL_MODULE
#include <linux/string.h>
#include "kecl.h"
#define fprintf(fmt, args...) printk(args)
#define printf printk
#define malloc (void *)DAllocMM
#define realloc(a,b) (void *)DReallocMM((addrs_t)a,b)
#define free(a) DFreeMM((addrs_t)a)

extern char *getenv(const char *name);
#endif

typedef struct _oprnd {
    dr_reg_type reg;
    int is_addr;
    int in_kernel;
    int offset;
} operand;

#if defined(HAVE_ICODE_H) || defined(HAVE_DRISC_H)
static void cg_compound_statement(drisc_ctx c, sm_ref stmt, ecl_code descr);
static void cg_decls(drisc_ctx c, sm_list decl_list, ecl_code descr);
static void cg_statements(drisc_ctx c, sm_list stmt_list, ecl_code descr);
static void cg_decl(drisc_ctx c, sm_ref decl, ecl_code descr);
static void cg_statement(drisc_ctx c, sm_ref stmt, ecl_code descr);
static operand cg_expr(drisc_ctx c, sm_ref expr, int left);
static void cg_selection_statement(drisc_ctx c, sm_ref stmt, ecl_code descr);
static void cg_iteration_statement(drisc_ctx c, sm_ref stmt, ecl_code descr);
static void cg_return_statement(drisc_ctx c, sm_ref stmt);
static void gen_mov(drisc_ctx c, operand left, dr_reg_type right, int type);
static void gen_load(drisc_ctx c, dr_reg_type left, operand right, int type);
static dr_reg_type coerce_type(drisc_ctx c, dr_reg_type local, int target_type, int item_type);
static int is_comparison_operator(sm_ref expr);
static void cg_branch_if_false(drisc_ctx c, sm_ref pred, dr_label_type label);
static int is_complex_type(sm_ref expr);

extern int ecl_sm_get_type(sm_ref node);
#endif

#ifdef HAVE_ICODE_H
/* 
 * this extern and it's use in this_never_gets_called_ecl() are solely so 
 * that the linker pulls xlate.o from the ecl library into the link.
 */
extern void
this_never_gets_called_ecl()
{
  typedef i_puint32 (*xf_t)(i_puint32);
  extern xf_t i_op2xlate[];
  i_op2xlate[0] = 0;
}
#endif

static int inst_count_guess = 0;
static void
init_operand(operand *op) 
{
    op->reg = 0;
    op->is_addr = 0;
    op->in_kernel = 0;
    op->offset = 0;
}

static void
cg_preprocess(sm_ref node, void *data) {
    ecl_code code_descriptor = (ecl_code) data;

    switch(node->node_type) {
    case ecl_struct_type_decl: {
	break;
    }
    case ecl_operator: {
	inst_count_guess += 6;	/* 5 probable worst case */
	break;
    }
    case ecl_cast: {
	inst_count_guess += 2;
	break;
    }
    case ecl_reference_type_decl: {
	break;
    }
    case ecl_type_specifier: {
	break;
    }
    case ecl_subroutine_call: {
	sm_list args = node->node.subroutine_call.arguments;
	while (args != NULL) {
	    inst_count_guess++;
	    args = args->next;
	}
	inst_count_guess+= 3;		/* call plus a few extra */
	break;
    }
    case ecl_selection_statement: {
	inst_count_guess += 8;
	break;
    }
    case ecl_constant: {
	inst_count_guess += 3;	/* probably 2 (strings??) */
	break;
    }
    case ecl_compound_statement: {
	break;
    }
    case ecl_assignment_expression: {
	inst_count_guess += 3;	/* coerce plus move */
	break;
    }
    case ecl_declaration: {
	sm_ref decl = node;
	inst_count_guess += 4;
	if (decl->node.declaration.static_var) {
	    assert(decl->node.declaration.param_num == -1); /* not a param */
	    decl->node.declaration.cg_address = 
		(void*)(long)code_descriptor->static_size_required;
	    if (decl->node.declaration.cg_type != DR_B) {
		/* atomic element */ /* maintain alignment */
		code_descriptor->static_size_required += 8; 
	    } else {
		sm_ref struct_type = decl->node.declaration.type_spec->node;
		code_descriptor->static_size_required += 
		    struct_type->node.struct_type_decl.cg_size;
	    }
	}
	break;
    }
    case ecl_element_ref: {
	inst_count_guess += 6;
	break;
    }
    case ecl_return_statement: {
	inst_count_guess += 2;
	break;
    }
    case ecl_expression_statement: {
	break;
    }
    case ecl_field: {
	break;
    }
    case ecl_iteration_statement: {
	inst_count_guess += 6;
	break;
    }
    case ecl_identifier: {
	break;
    }
    case ecl_field_ref: {
	inst_count_guess += 6;
	break;
    }
    case ecl_array_type_decl:
	break;
    default:
	assert(FALSE);
    }
}
    
static char *arg_type_str[] = { "c", "uc", "s", "us", "i", "u", "l", "ul", "p", "f", "d", "v", "p", "EC"};

char *
generate_arg_str(sm_ref net)
{
    sm_list decls = NULL;
    char *arg_str = malloc(1);
    signed char *arg_types = malloc(1);
    int arg_count = 0;
    int i;
    if (net->node_type == ecl_compound_statement) {
	decls = net->node.compound_statement.decls;
    }
    arg_str[0] = 0;
    while (decls != NULL) {
	sm_ref decl = decls->node;
	
	if (decl->node_type == ecl_declaration) {
	    if (decl->node.declaration.param_num != -1) {
		/* parameter */
		int param_num = decl->node.declaration.param_num;
		if (param_num >= arg_count) {
		    arg_types = realloc(arg_types, param_num + 1);
		    memset(arg_types + arg_count, -1, 
			   param_num - arg_count);
		    arg_count = param_num + 1;
		}
		arg_types[param_num] = decl->node.declaration.cg_type;
	    }
	} else if (decl->node_type == ecl_array_type_decl) {
	    decl = decl->node.array_type_decl.element_ref;
	    if (decl->node.declaration.param_num != -1) {
		int param_num = decl->node.declaration.param_num;
		if (param_num >= arg_count) {
		    arg_types = realloc(arg_types, param_num + 1);
		    memset(arg_types + arg_count, -1, 
			   param_num - arg_count);
		    arg_count = param_num + 1;
		}
		arg_types[param_num] = DR_P;
	    }
	}
	    
	decls = decls->next;
    }
    for (i = 0; i < arg_count ; i++) {
	if (arg_types[i] == -1) {
	    printf("Arg %d not declared\n", i);
	    return arg_str;
	}
	arg_str = realloc(arg_str, strlen(arg_str) + 4);
	strcat(arg_str, "%");
	strcat(arg_str, arg_type_str[arg_types[i]]);
    }
    free(arg_types);
    return arg_str;
}

static int
cg_get_size(drisc_ctx c, sm_ref node) {

    sm_ref ref = node;
    
    switch(ref->node_type) {
    case ecl_identifier:
	if(ref->node.identifier.sm_declaration) {
	    return cg_get_size(c, ref->node.identifier.sm_declaration);
	}
	return dr_type_size(c, ref->node.identifier.cg_type);

    case ecl_declaration:
	if(ref->node.declaration.sm_complex_type) {
	    return cg_get_size(c, ref->node.declaration.sm_complex_type);
	}
	return dr_type_size(c, ref->node.declaration.cg_type);
	
    case ecl_reference_type_decl:
	return dr_type_size(c, DR_P);

    case ecl_field:
	return ref->node.field.cg_size;
		
    case ecl_struct_type_decl:
	return ref->node.struct_type_decl.cg_size;
    case ecl_cast:
	if (ref->node.cast.sm_complex_type != NULL) {
	    return cg_get_size(c, ref->node.cast.sm_complex_type);
	} else {
	    return dr_type_size(c, ref->node.cast.cg_type);
	};
    case ecl_array_type_decl: {
	/* handle dynamic here */
	return ref->node.array_type_decl.cg_static_size;
    }
    case ecl_assignment_expression:
    case ecl_field_ref:
    case ecl_element_ref:
    case ecl_subroutine_call:
    case ecl_operator: {
	sm_ref typ = get_complex_type(NULL, ref);
	if (typ != NULL) {
	    return cg_get_size(c, typ);
	}
	return dr_type_size(c, ecl_sm_get_type(ref));
    }
    case ecl_constant:
    case ecl_iteration_statement:
    case ecl_expression_statement:
    case ecl_return_statement:
    case ecl_compound_statement:
    case ecl_type_specifier:
    case ecl_selection_statement:
    default:
	/* shouldn't happen */
	assert(FALSE);
    }
    return 0;
}

extern void *
ecl_cg_net(net, ret_type, offset_p, code_descriptor)
sm_ref net;
int ret_type;
unsigned int *offset_p;
ecl_code code_descriptor;
{
    void *init_func;
    static int debug_cg = -1;
    drisc_ctx c = NULL;
#ifndef HAVE_ICODE_H
    char *arg_str;
#endif

    if (debug_cg == -1) {
	debug_cg = (int)(long)(getenv("ECL_DEBUG"));
    }
    inst_count_guess = 0;
    code_descriptor->static_size_required = 0;
    ecl_apply(net, cg_preprocess, NULL, NULL, code_descriptor);

    if (code_descriptor->static_size_required > 0) {
	code_descriptor->data = 
	    malloc(code_descriptor->static_size_required);
    } else {
	code_descriptor->data = NULL;
    }

#ifdef HAVE_ICODE_H
    i_init(inst_count_guess);
    cg_compound_statement(c, net, code_descriptor);
    i_end();
    if (debug_cg) i_debug_on();
    init_func = (void*) i_emit(offset_p, NULL).i;
    if (debug_cg) v_dump((void*)(long)init_func);
    i_reset();
    code_descriptor->drisc_context = NULL;
    return (void*)(long)init_func;
#else
#if defined(HAVE_DRISC_H)
    arg_str = generate_arg_str(net);
    c = dr_vinit();
    code_descriptor->drisc_context = c;
    dr_proc_params(c, "no name", ret_type, arg_str);
    free(arg_str);
    cg_compound_statement(c, net, code_descriptor);
    if (debug_cg) {
	printf("Virtual insn dump\n");
	dr_dump(c);
	printf("\n");
    }
    init_func = dr_end(c);
    if (debug_cg) {
	printf("Native insn dump\n");
	dr_dump(c);
	printf("\n");
    }
    code_descriptor->code_memory_block = NULL;
    return init_func;
#else
    printf("No dcg available\n");
    return NULL;
#endif
#endif
}

#if defined(HAVE_ICODE_H) || defined(HAVE_DRISC_H)
static void
cg_compound_statement(drisc_ctx c, sm_ref stmt, ecl_code descr)
{
    cg_decls(c, stmt->node.compound_statement.decls, descr);
    cg_statements(c, stmt->node.compound_statement.statements, descr);
}

static void
cg_decls(drisc_ctx c, sm_list decl_list, ecl_code descr)
{
    while(decl_list != NULL) {
	cg_decl(c, decl_list->node, descr);
	decl_list = decl_list->next;
    }
}

static void
cg_statements(drisc_ctx c, sm_list stmt_list, ecl_code descr)
{
    while(stmt_list != NULL) {
	cg_statement(c, stmt_list->node, descr);
	stmt_list = stmt_list->next;
    }
}

static void 
cg_decl(drisc_ctx c, sm_ref decl, ecl_code descr)
{
    dr_reg_type lvar;
    sm_ref ctype = decl->node.declaration.sm_complex_type;
    switch(decl->node_type) {
    case ecl_declaration:
	if (decl->node.declaration.is_typedef) {
	    cg_decl(c, decl->node.declaration.sm_complex_type, descr);
	}
	if ((ctype != NULL) && (ctype->node_type == ecl_struct_type_decl)
	    && (ctype->node.struct_type_decl.fields->node->node.field.cg_offset == -1)) {
	    /* we need to do structure layout */
	    int last_offset = 0;
	    int off_by;
	    sm_list fields = ctype->node.struct_type_decl.fields;
	    decl->node.declaration.cg_type = DR_B;
	    while(fields != NULL) {
		sm_ref field = fields->node;
		off_by = last_offset % dr_type_align(c, field->node.field.cg_type);
		if (off_by != 0) {
		    last_offset += dr_type_align(c, field->node.field.cg_type) - off_by;
		}
		field->node.field.cg_offset = last_offset;
		last_offset += dr_type_size(c, field->node.field.cg_type);
		fields = fields->next;
	    }
	    last_offset += (dr_type_align(c, DR_D) - (last_offset % dr_type_align(c, DR_D))) % dr_type_align(c, DR_D);
	    ctype->node.struct_type_decl.cg_size = last_offset;
	}
	if (decl->node.declaration.static_var) {
	    /* do statics */
	    decl->node.declaration.cg_address = 
		((long)decl->node.declaration.cg_address) + 
		(char *) descr->data;
	    if (decl->node.declaration.init_value == NULL) {
		/* init to zero's */
		memset(decl->node.declaration.cg_address, 0, cg_get_size(c, decl));
	    } else {
		sm_ref const_val = decl->node.declaration.init_value;
		if (const_val->node.constant.token == string_constant) {
		    assert(FALSE);
		} else if (const_val->node.constant.token == floating_constant) {
		    double f;
		    
		    sscanf(const_val->node.constant.const_val, "%lf", &f);
		    switch (decl->node.declaration.cg_type) {
		    case DR_C:
			*(char *)(decl->node.declaration.cg_address) = (int)f;
			break;
		    case DR_UC:
			*(unsigned char *)(decl->node.declaration.cg_address) = (int)f;
			break;
		    case DR_S:
			*(short *)(decl->node.declaration.cg_address) = (int)f;
			break;
		    case DR_US:
			*(unsigned short *)(decl->node.declaration.cg_address) = (int)f;
			break;
		    case DR_I:
			*(int *)(decl->node.declaration.cg_address) = (int)f;
			break;
		    case DR_U:
			*(unsigned int *)(decl->node.declaration.cg_address) = (int)f;
			break;
		    case DR_L:
			*(long *)(decl->node.declaration.cg_address) = (long)f;
			break;
		    case DR_UL:
			*(unsigned long *)(decl->node.declaration.cg_address) = (long)f;
			break;
		    case DR_F:
			*(float*)(decl->node.declaration.cg_address) = (float)f;
			break;
		    case DR_D:
			*(double*)(decl->node.declaration.cg_address) = (double)f;
			break;
		    default:
			assert(FALSE);
		    }
		} else {
		    long i;
		    char *val = const_val->node.constant.const_val;
		    if (val[0] == '0') {
			/* hex or octal */
			if (val[1] == 'x') {
			    /* hex */
			    if (sscanf(val+2, "%lx", &i) != 1) 
				printf("sscanf failed\n");
			} else {
			    if (sscanf(val, "%lo", &i) != 1) 
				printf("sscanf failed\n");
			}
		    } else {
			if (sscanf(val, "%ld", &i) != 1) 
			    printf("sscanf failed\n");
		    }
		    switch (decl->node.declaration.cg_type) {
		    case DR_C:
			*(char *)(decl->node.declaration.cg_address) = (char)i;
			break;
		    case DR_UC:
			*(unsigned char *)(decl->node.declaration.cg_address) = (unsigned char)i;
			break;
		    case DR_S:
			*(short *)(decl->node.declaration.cg_address) = (short)i;
			break;
		    case DR_US:
			*(unsigned short *)(decl->node.declaration.cg_address) = (unsigned short)i;
			break;
		    case DR_I:
			*(int *)(decl->node.declaration.cg_address) = i;
			break;
		    case DR_U:
			*(unsigned int *)(decl->node.declaration.cg_address) = i;
			break;
		    case DR_L:
			*(long *)(decl->node.declaration.cg_address) = i;
			break;
		    case DR_UL:
			*(unsigned long *)(decl->node.declaration.cg_address) = i;
			break;
		    case DR_F:
			*(float*)(decl->node.declaration.cg_address) = (float)i;
			break;
		    case DR_D:
			*(double*)(decl->node.declaration.cg_address) = (double)i;
			break;
		    default:
			assert(FALSE);
		    }
		}
	    }
	    return;
	}
	if (decl->node.declaration.param_num != -1) {
#ifdef HAVE_ICODE_H
	    lvar = i_paramn(decl->node.declaration.cg_type, decl->node.declaration.param_num);
#else
	    lvar = dr_param(c, decl->node.declaration.param_num);
#endif
	    if ((ctype != NULL) && (ctype->node_type == ecl_array_type_decl)){
		/* array param */
		long i;
		char *val;
		sm_ref const_val = ctype->node.array_type_decl.size_expr;
		ctype->node.array_type_decl.cg_element_size = 
		    dr_type_align(c, ctype->node.array_type_decl.cg_element_type);
		if (const_val != NULL) {
		    val = const_val->node.constant.const_val;
		    if (val[0] == '0') {
			/* hex or octal */
			if (val[1] == 'x') {
			    /* hex */
			    if (sscanf(val+2, "%lx", &i) != 1) 
				printf("sscanf failed\n");
			} else {
			    if (sscanf(val, "%lo", &i) != 1) 
				printf("sscanf failed\n");
			}
		    } else {
			if (sscanf(val, "%ld", &i) != 1) 
			    printf("sscanf failed\n");
		    }
		    ctype->node.array_type_decl.cg_static_size = i;
		}
	    }
	} else {
	    if (decl->node.declaration.is_subroutine) {
		lvar = 0;
	    } else {
		if ((ctype != NULL) && (ctype->node_type == ecl_array_type_decl)){
		    /* array */
		    long i;
		    char *val;
		    sm_ref const_val = ctype->node.array_type_decl.size_expr;
		    ctype->node.array_type_decl.cg_element_size = 
			dr_type_align(c, ctype->node.array_type_decl.cg_element_type);
		    val = const_val->node.constant.const_val;
		    if (val[0] == '0') {
			/* hex or octal */
			if (val[1] == 'x') {
			    /* hex */
			    if (sscanf(val+2, "%lx", &i) != 1) 
				printf("sscanf failed\n");
			} else {
			    if (sscanf(val, "%lo", &i) != 1) 
				printf("sscanf failed\n");
			}
		    } else {
			if (sscanf(val, "%ld", &i) != 1) 
			    printf("sscanf failed\n");
		    }
		    ctype->node.array_type_decl.cg_static_size = i;
		    lvar = 
			dr_getvblock(c, i * ctype->node.array_type_decl.cg_element_size);
		} else if (decl->node.declaration.cg_type != DR_B) {
		    if(decl->node.declaration.addr_taken) {
		        /* make sure it's in memory if its address is taken */
		        lvar = dr_getvblock(c, 8);
		    } else {
		        lvar = dr_getvreg(c, decl->node.declaration.cg_type);
		    }
		} else {
		    sm_ref struct_type = decl->node.declaration.sm_complex_type;
		    lvar = 
			dr_getvblock(c, struct_type->node.struct_type_decl.cg_size);
		}
	    }
	}
	if (!decl->node.declaration.is_typedef) {
	    decl->node.declaration.cg_oprnd = lvar;
	}
	if (decl->node.declaration.init_value != NULL) {
	    operand left, right;
	    int assign_type = ecl_sm_get_type(decl);
	    init_operand(&left);
	    right = cg_expr(c, decl->node.declaration.init_value, 0);
	    assert(right.is_addr == 0);
	    right.reg = coerce_type(c, right.reg, assign_type, 
				    ecl_sm_get_type(decl->node.declaration.init_value));
	    if (decl->node.declaration.addr_taken) {
		dr_reg_type addr_reg = dr_getvreg(c, DR_P);
		dr_virtual_lea(c, addr_reg, lvar);	/* op_i_leai */
		init_operand(&left);
		left.reg = addr_reg;
		left.is_addr = 1;
		left.in_kernel = 0; /* we have no access to kernel structures */
		left.offset = 0;

	    } else {
		left.reg = lvar;
		left.is_addr = 0;
		left.in_kernel = 0; /* we have no access to kernel structures */
		left.offset = 0;
	    }
	    gen_mov(c, left, right.reg, assign_type);
	} else if ((decl->node.declaration.cg_type == DR_B) &&
		   (decl->node.declaration.param_num == -1)) {
	    /* init structure to zero's */
	    dr_reg_type addr_reg = dr_getvreg(c, DR_P);
	    dr_virtual_lea(c, addr_reg, lvar);	/* op_i_leai */
	    (void) dr_scallv(c, (void*)memset, "%p%I%I", addr_reg, 0, ctype->node.struct_type_decl.cg_size);
	}
	break;
    case ecl_struct_type_decl: {
	    sm_list fields = decl->node.struct_type_decl.fields;
	    int do_cg = 0, last_offset = 0;
	    int off_by;
	    while (fields != NULL) {
		if (fields->node->node.field.cg_size <= 0) {
		    do_cg = 1;
		}
		fields = fields->next;
	    }
	    if (do_cg == 0) return;
	    fields = decl->node.struct_type_decl.fields;
	    decl->node.declaration.cg_type = DR_B;
	    while(fields != NULL) {
		sm_ref field = fields->node;
		off_by = last_offset % dr_type_align(c, field->node.field.cg_type);
		if (off_by != 0) {
		    last_offset += dr_type_align(c, field->node.field.cg_type) - off_by;
		}
		field->node.field.cg_offset = last_offset;
		last_offset += dr_type_size(c, field->node.field.cg_type);
		fields = fields->next;
	    }
	    last_offset += (dr_type_align(c, DR_D) - (last_offset % dr_type_align(c, DR_D))) % dr_type_align(c, DR_D);
	    decl->node.struct_type_decl.cg_size = last_offset;
	
        }
	break;
    case ecl_reference_type_decl:
	printf("got a reference type decl\n");
	break;
    case ecl_array_type_decl:
	cg_decl(c, decl->node.array_type_decl.element_ref, descr);
	break;
    default:
	printf("Unexpected case in cg_decl\n");
	ecl_print(decl);
    }
}

static void
cg_statement(drisc_ctx c, sm_ref stmt, ecl_code descr)
{
    switch(stmt->node_type) {
    case ecl_selection_statement:
	cg_selection_statement(c, stmt, descr);
	break;
    case ecl_iteration_statement:
	cg_iteration_statement(c, stmt, descr);
	break;
    case ecl_expression_statement: 
	(void) cg_expr(c, stmt->node.expression_statement.expression, 0);
	break;
    case ecl_compound_statement:
	cg_compound_statement(c, stmt, descr);
	break;
    case ecl_return_statement:
	cg_return_statement(c, stmt);
	break;
    default:
	printf("unhandled case in cg_statement\n");
    }
}

static int
ecl_streq(char *str1, char *str2)
{
    int ret;
    if ((str1 == NULL) && (str2 == NULL)) return 0;
    if ((str1 == NULL) || (str2 == NULL)) return 1;
    ret = (strcmp(str1, str2) != 0);
/*    printf("Comparing %s and %s, returning %d\n", str1, str2, ret);*/
    return ret;
}

static char*
ecl_strdup(char *str)
{
    if (str == NULL) return NULL;
/*    printf("Duplicating str %s\n", str);*/
    return strdup(str);
}

static void
operator_prep(drisc_ctx c, sm_ref expr, dr_reg_type *rp, dr_reg_type *lp)
{
    operand right_op, left_op;
    dr_reg_type right = 0, left = 0;
    int op_type = expr->node.operator.operation_type;
    int string_op = FALSE;
    if (expr->node.operator.op == op_address) {
	right_op = cg_expr(c, expr->node.operator.right, 1);
	assert(right_op.is_addr == 1);
	if (right_op.offset != 0) {
	    dr_reg_type result = dr_getvreg(c, DR_P);
	    dr_addpi(c, result, right_op.reg, right_op.offset);
	    right_op.reg = result;
	}
	*rp = right_op.reg;
	return;
    }
	
    if (expr->node.operator.right != NULL) {
	int right_cg_type = ecl_sm_get_type(expr->node.operator.right);
	string_op = ecl_expr_is_string(expr->node.operator.right);

	right_op = cg_expr(c, expr->node.operator.right, 0);
	assert(right_op.is_addr == 0);

	switch(right_cg_type) {
	case DR_C: case DR_UC: case DR_S: case DR_US:
	    /* do integer promotion */
	    right = coerce_type(c, right_op.reg, DR_I, right_cg_type);
	    right_cg_type = DR_I;
	    right_op.reg = right;
	}

	right = right_op.reg;
	if (!string_op && op_type != DR_P) 
	    right = coerce_type(c, right_op.reg, op_type, right_cg_type);
    }
    if (expr->node.operator.left != NULL) {
	int left_cg_type = ecl_sm_get_type(expr->node.operator.left);
	left_op = cg_expr(c, expr->node.operator.left, 0);
	assert(left_op.is_addr == 0);

	switch(left_cg_type) {
	case DR_C: case DR_UC: case DR_S: case DR_US:
	    /* do integer promotion */
	    left = coerce_type(c, left_op.reg, DR_I, left_cg_type);
	    left_cg_type = DR_I;
	    left_op.reg = left;
	}
	left = left_op.reg;
	if (!string_op && op_type != DR_P)
	    left = coerce_type(c, left_op.reg, op_type, left_cg_type);
    }
    *rp = right;
    *lp = left;
}

#ifndef HAVE_DRISC_H
static void
dr_pmov(drisc_ctx c, int cg_type, dr_reg_type dest, dr_reg_type src)
{
    switch(cg_type) {
    case DR_C:
    case DR_UC:
    case DR_S:
    case DR_US:
    case DR_I:
	dr_movi(c, dest, src);	/* op_i_movi */
	break;
    case DR_U:
	dr_movu(c, dest, src);	/* op_i_movu */
	break;
    case DR_L:
	dr_movl(c, dest, src);	/* op_i_movl */
	break;
    case DR_UL:
	dr_movul(c, dest, src);	/* op_i_movul */
	break;
    case DR_P:
	dr_movp(c, dest, src);	/* op_i_movp */
	break;
    case DR_D:
	dr_movd(c, dest, src);	/* op_i_movd */
	break;
    case DR_F:
	dr_movf(c, dest, src);	/* op_i_movf */
	break;
    default:
	fprintf(stderr, "unhandled case in dr_pmov\n");
	break;
    }
}
#endif

static void
poly_addi(drisc_ctx c, int op_type, dr_reg_type result, dr_reg_type left,
	  long imm)
{
    switch (op_type) {
    case DR_I:
	dr_addii(c, result, left, imm);	/* op_i_addii */
	break;
    case DR_U:
	dr_addui(c, result, left, imm);	/* op_i_addui */
	break;
    case DR_L:
	dr_addli(c, result, left, imm);	/* op_i_addli */
	break;
    case DR_UL:
	dr_adduli(c, result, left, imm);	/* op_i_adduli */
	break;
    case DR_P:
	dr_addpi(c, result, left, imm);	/* op_i_addpi */
	break;
    default:
	assert(0);
    }
}

static void
poly_subi(drisc_ctx c, int op_type, dr_reg_type result, dr_reg_type left,
	  long imm)
{
    switch (op_type) {
    case DR_I:
	dr_subii(c, result, left, imm);	/* op_i_subii */
	break;
    case DR_U:
	dr_subui(c, result, left, imm);	/* op_i_subui */
	break;
    case DR_L:
	dr_subli(c, result, left, imm);	/* op_i_subli */
	break;
    case DR_UL:
	dr_subuli(c, result, left, imm);	/* op_i_subuli */
	break;
    case DR_P:
	dr_subpi(c, result, left, imm);	/* op_i_subpi */
	break;
    default:
	assert(0);
    }
}

static operand
handle_incdec(drisc_ctx c, sm_ref expr)
{
    dr_reg_type right = 0, opnd, left = 0;
    operand ret_op, store_op;
    operand right_op, left_op;
    int op_type = expr->node.operator.operation_type;
    dr_reg_type result = dr_getvreg(c, expr->node.operator.result_type);
    operator_t op = expr->node.operator.op;

    sm_ref ref = expr->node.operator.right;
    sm_ref typ;
    int size = 1;
    if (ref == NULL) {
	ref = expr->node.operator.left;
    }
    
    if (expr->node.operator.right != NULL) {
	right_op = cg_expr(c, expr->node.operator.right, 1);
	if (right_op.is_addr == 1) {
	    right = dr_getvreg(c, op_type);
	    gen_load(c, right, right_op, op_type);
	} else {
	    right = right_op.reg;
	}
    }
    if (expr->node.operator.left != NULL) {
	left_op = cg_expr(c, expr->node.operator.left, 1);
	if (left_op.is_addr == 1) {
	    left = dr_getvreg(c, op_type);
	    gen_load(c, left, left_op, op_type);
	} else {
	    left = left_op.reg;
	}
    }

    typ = get_complex_type(NULL, ref);
    
    if(typ && typ->node_type == ecl_reference_type_decl) {
	if(typ->node.reference_type_decl.sm_complex_referenced_type) {
	    size = cg_get_size(c, typ->node.reference_type_decl.sm_complex_referenced_type);
	} else {
	    size = dr_type_size(c, typ->node.reference_type_decl.cg_referenced_type);
	}
    }
    
    opnd = right;
    store_op = right_op;

    if (expr->node.operator.right == NULL) {
	/* post-increment/decrement, grab the result first */
	dr_pmov(c, op_type, result, left);
	opnd = left;
	store_op = left_op;
    }
    if (op == op_inc) {
	poly_addi(c, op_type, opnd, opnd, size);
    } else {
	poly_subi(c, op_type, opnd, opnd, size);
    }
    if (expr->node.operator.left == NULL) {
	/* pre-increment/decrement, grab the result last */
	dr_pmov(c, op_type, result, right);	/* op_i_movp */
    }
    if (store_op.is_addr == 1) {
	/* store the result home either way if we didn't have an assignable */
	gen_mov(c, store_op, opnd, op_type);
    }
    init_operand(&ret_op);
    ret_op.is_addr = 0;
    ret_op.reg = result;
    return ret_op;
}	

static operand
cg_operator(drisc_ctx c, sm_ref expr, int need_assignable)
{
    dr_reg_type right, left = 0;
    operand ret_op;
    int op_type = expr->node.operator.operation_type;
    dr_reg_type result = dr_getvreg(c, expr->node.operator.result_type);
    operator_t op = expr->node.operator.op;
    int string_op = FALSE;
    if (expr->node.operator.right) {
	string_op = ecl_expr_is_string(expr->node.operator.right);
    }
    init_operand(&ret_op);
    ret_op.is_addr = 0;
    ret_op.offset = 0;

    if (is_comparison_operator(expr)) {
	dr_label_type false_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	cg_branch_if_false(c, expr, false_label);
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, false_label);
	ret_op.reg = result;
	return ret_op;
    }

    if ((op == op_inc) || (op == op_dec)) {
	assert(!need_assignable);  /* if we do, this is wrong */
	return handle_incdec(c, expr);
    }
    if (op == op_sizeof) {
	int size = cg_get_size(c, expr->node.operator.right);
	dr_seti(c, result, size);
	ret_op.reg = result;
	return ret_op;
    }

    operator_prep(c, expr, &right, &left);

    if (op == op_log_neg) {
	op = op_neq;
	left = dr_getvreg(c, op_type);
	switch (op_type) {
#ifdef HAVE_ICODE_H
	case DR_I: i_seti(left, 0); break;	/* op_i_seti */
	case DR_U: i_setu(left, 0); break;	/* op_i_setu */
	case DR_L: i_setl(left, 0); break;	/* op_i_setl */
	case DR_UL: i_setul(left, 0); break;	/* op_i_setul */
#else
	case DR_I: case DR_U: case DR_L: case DR_UL:
	    dr_piset(c, op_type, left, 0); break;
#endif
	case DR_F: dr_setf(c, left, 0.0); break;	/* op_i_setf */
	case DR_D: dr_setd(c, left, 0.0); break;	/* op_i_setd */
	}
    }
    switch(op) {
    case  op_modulus:
	switch (op_type) {
	case DR_I:
	    dr_modi(c, result, left, right);	/* op_i_modi */
	    break;
	case DR_U:
	    dr_modu(c, result, left, right);	/* op_i_modu */
	    break;
	case DR_L:
	    dr_modl(c, result, left, right);	/* op_i_modl */
	    break;
	case DR_UL:
	    dr_modul(c, result, left, right);	/* op_i_modul */
	    break;
	case DR_P:
	    /* Modulus not applicable to pointers */
	    assert(FALSE);
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case  op_plus:
	if (expr->node.operator.left == NULL) {
	    ret_op.reg = right;
	    return ret_op;
	}
	switch (op_type) {
	case DR_I:
	    dr_addi(c, result, left, right);	/* op_i_addi */
	    break;
	case DR_U:
	    dr_addu(c, result, left, right);	/* op_i_addu */
	    break;
	case DR_L:
	    dr_addl(c, result, left, right);	/* op_i_addl */
	    break;
	case DR_UL:
	    dr_addul(c, result, left, right);	/* op_i_addul */
	    break;
	case DR_F:
	    dr_addf(c, result, left, right);	/* op_i_addf */
	    break;
	case DR_D:
	    dr_addd(c, result, left, right);	/* op_i_addd */
	    break;
	case DR_P:
	{
	    
	    sm_ref ptr, arg;
	    sm_ref typ;
	    int size;
	    dr_reg_type opPtr, opArg;
	    dr_reg_type offset = dr_getvreg(c, DR_UL);	    

	    /* Figure out which arg is ptr and which is integral */
	    if(typ = get_complex_type(NULL, expr->node.operator.left)) {
		if(typ->node_type == ecl_reference_type_decl) {
		    ptr = expr->node.operator.left;  opPtr = left;
		    arg = expr->node.operator.right; opArg = right;
		} else {
		    arg = expr->node.operator.left;  opArg = left;
		    ptr = expr->node.operator.right; opPtr = right;
		}
	    } else {
		if(ecl_sm_get_type(expr->node.operator.left) == DR_P) {
		    ptr = expr->node.operator.left;  opPtr = left; 
		    arg = expr->node.operator.right; opArg = right;
		} else {
		    arg = expr->node.operator.left;  opArg = left; 
		    ptr = expr->node.operator.right; opPtr = right;
		}
	    }

	    /* Get the size of the referenced type */
	    typ = get_complex_type(NULL, ptr);
	    if(typ->node.reference_type_decl.sm_complex_referenced_type) {
		size = cg_get_size(c, typ->node.reference_type_decl.sm_complex_referenced_type);
	    } else {
		size = dr_type_size(c, typ->node.reference_type_decl.cg_referenced_type);
	    }
	    if (size != 1) {
		switch(ecl_sm_get_type(arg)) {
		case DR_I:
		    dr_mulii(c, offset, opArg, size);  /* op_i_mulii */
		    dr_cvi2l(c, offset, offset);       /* op_i_cvi2l */
		    break;
		case DR_U:
		    dr_mului(c, offset, opArg, size);  /* op_i_mului */
		    dr_cvu2l(c, offset, offset);       /* op_i_cvi2l */
		    break;
		case DR_L:
		    dr_mulli(c, offset, opArg, size);  /* op_i_mulli */
		    break;
		case DR_UL:
		    dr_mululi(c, offset, opArg, size); /* op_i_mululi */
		    break;
		}
	    } else {
		offset = opArg;
	    }
	    dr_addp(c, result, opPtr, offset);           /* op_i_addp */ 
	}
	break;
	}
	break;
    case  op_minus:
	if (expr->node.operator.left == NULL) {
	    left = dr_getvreg(c, op_type);
	    switch (op_type) {
	    case DR_I: dr_seti(c, left, 0); break;	/* op_i_seti */
	    case DR_U: dr_setu(c, left, 0); break;	/* op_i_setu */
	    case DR_L: dr_setl(c, left, 0); break;	/* op_i_setl */
	    case DR_UL: dr_setul(c, left, 0); break;	/* op_i_setul */
	    case DR_F: dr_setf(c, left, 0.0); break;	/* op_i_setf */
	    case DR_D: dr_setd(c, left, 0.0); break;	/* op_i_setd */
            case DR_P:
                /* Unary minus not applicable to pointers */
                assert(FALSE);
                break;
	    }
	}
	switch (op_type) {
	case DR_I:
	    dr_subi(c, result, left, right);	/* op_i_subi */
	    break;
	case DR_U:
	    dr_subu(c, result, left, right);	/* op_i_subu */
	    break;
	case DR_L:
	    dr_subl(c, result, left, right);	/* op_i_subl */
	    break;
	case DR_UL:
	    dr_subul(c, result, left, right);	/* op_i_subul */
	    break;
	case DR_F:
	    dr_subf(c, result, left, right);	/* op_i_subf */
	    break;
	case DR_D:
	    dr_subd(c, result, left, right);	/* op_i_subd */
	    break;
	case DR_P:
	{
	    sm_ref ltyp, rtyp;
	    char   lIsPtr, rIsPtr;
	    int size;
	    
	    /* Figure out if left arg is a pointer or integral */
	    if(ltyp = get_complex_type(NULL, expr->node.operator.left)) {
		if(ltyp->node_type == ecl_reference_type_decl) {
		    lIsPtr = 1;
		} else {
		    lIsPtr = 0;
		}
	    } else {
		if(ecl_sm_get_type(expr->node.operator.left) == DR_P) {
		    lIsPtr = 1;
		} else {
		    lIsPtr = 0;
		}
	    }

	    /* Figure out if right arg is a pointer or integral */
	    if(rtyp = get_complex_type(NULL, expr->node.operator.right)) {
		if(rtyp->node_type == ecl_reference_type_decl) {
		    rIsPtr = 1;
		} else {
		    rIsPtr = 0;
		}
	    } else {
		if(ecl_sm_get_type(expr->node.operator.right) == DR_P) {
		    rIsPtr = 1;
		} else {
		    rIsPtr = 0;
		}
	    }
	    
	    /*
	     * The semanticizer would only allow op_minus w/ a pointer arg if:
	     * (1) it is between two pointers referencing compatible types
	     * (2) it is between a pointer and an integral type
	     */
	    if(lIsPtr && rIsPtr) {
		/* Case #1: binary minus between two compatible pointers */

		/* Figure out referenced type size */
		if(ltyp->node.reference_type_decl.sm_complex_referenced_type) {
		    size = cg_get_size(c, ltyp->node.reference_type_decl.sm_complex_referenced_type);
		} else {
		    size = dr_type_size(c, ltyp->node.reference_type_decl.cg_referenced_type);
		}
		
		/* Perform subtraction */
		dr_subp(c, result, left, right);           /* op_i_subp   */

		if(size != 1) {
		    dr_divli(c, result, result, size);     /* op_i_divli */
		}
	    } else {
		/* Case #2: binary minus between a pointer and an integral */
		sm_ref ptr, arg;
		sm_ref typ;
		dr_reg_type opPtr, opArg;
		dr_reg_type offset = dr_getvreg(c, DR_UL);	    

		if(lIsPtr) {
		    /* Left arg is pointer, right arg is integral */ 
		    ptr = expr->node.operator.left;  opPtr = left;
		    arg = expr->node.operator.right; opArg = right;
		} else {
		    /* Left arg is integral, right arg is pointer */ 
		    arg = expr->node.operator.left;  opArg = left;
		    ptr = expr->node.operator.right; opPtr = right;
		}

		/* Get the size of the referenced type */
		typ = get_complex_type(NULL, ptr);
		if(typ->node.reference_type_decl.sm_complex_referenced_type) {
		    size = cg_get_size(c, typ->node.reference_type_decl.sm_complex_referenced_type);
		} else {
		    size = dr_type_size(c, typ->node.reference_type_decl.cg_referenced_type);
		}
		if (size != 1) {
		    switch(ecl_sm_get_type(arg)) {
		    case DR_I:
			dr_mulii(c, offset, opArg, size);  /* op_i_mulii */
			dr_cvi2l(c, offset, offset);       /* op_i_cvi2l */
			break;
		    case DR_U:
			dr_mului(c, offset, opArg, size);  /* op_i_mului */
			dr_cvu2l(c, offset, offset);       /* op_i_cvi2l */
			break;
		    case DR_L:
			dr_mulli(c, offset, opArg, size);  /* op_i_mulli */
			break;
		    case DR_UL:
			dr_mululi(c, offset, opArg, size); /* op_i_mululi */
			break;
		    }
		} else {
		    offset = opArg;
		}
		dr_subp(c, result, opPtr, offset);           /* op_i_subp */ 
	    }
	}
	break;
	}
	ret_op.reg = result;
	return ret_op;
    case  op_leq:{
	dr_label_type true_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	switch (op_type) {
	case DR_I:
	    dr_blei(c, left, right, true_label);	/* op_i_blei */
	    break;
	case DR_U:
	    dr_bleu(c, left, right, true_label);	/* op_i_bleu */
	    break;
	case DR_L:
	    dr_blel(c, left, right, true_label);	/* op_i_blel */
	    break;
	case DR_UL:
	    dr_bleul(c, left, right, true_label);	/* op_i_bleul */
	    break;
	case DR_F:
	    dr_blef(c, left, right, true_label);	/* op_i_blef */
	    break;
	case DR_D:
	    dr_bled(c, left, right, true_label);	/* op_i_bled */
	    break;
	default:
	    assert(FALSE);
	}
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, true_label);
	break;
    }
    case  op_lt:{
	dr_label_type true_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	switch (op_type) {
	case DR_I:
	    dr_blti(c, left, right, true_label);	/* op_i_blti */
	    break;
	case DR_U:
	    dr_bltu(c, left, right, true_label);	/* op_i_bltu */
	    break;
	case DR_L:
	    dr_bltl(c, left, right, true_label);	/* op_i_bltl */
	    break;
	case DR_UL:
	    dr_bltul(c, left, right, true_label);	/* op_i_bltul */
	    break;
	case DR_F:
	    dr_bltf(c, left, right, true_label);	/* op_i_bltf */
	    break;
	case DR_D:
	    dr_bltd(c, left, right, true_label);	/* op_i_bltd */
	    break;
	default:
	    assert(FALSE);
	}
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, true_label);
	break;
    }
    case  op_geq:{
	dr_label_type true_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	switch (op_type) {
	case DR_I:
	    dr_bgei(c, left, right, true_label);	/* op_i_bgei */
	    break;
	case DR_U:
	    dr_bgeu(c, left, right, true_label);	/* op_i_bgeu */
	    break;
	case DR_L:
	    dr_bgel(c, left, right, true_label);	/* op_i_bgel */
	    break;
	case DR_UL:
	    dr_bgeul(c, left, right, true_label);	/* op_i_bgeul */
	    break;
	case DR_F:
	    dr_bgef(c, left, right, true_label);	/* op_i_bgef */
	    break;
	case DR_D:
	    dr_bged(c, left, right, true_label);	/* op_i_bged */
	    break;
	default:
	    assert(FALSE);
	}
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, true_label);
	break;
    }
    case  op_gt:{
	dr_label_type true_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	switch (op_type) {
	case DR_I:
	    dr_bgti(c, left, right, true_label);	/* op_i_bgti */
	    break;
	case DR_U:
	    dr_bgtu(c, left, right, true_label);	/* op_i_bgtu */
	    break;
	case DR_L:
	    dr_bgtl(c, left, right, true_label);	/* op_i_bgtl */
	    break;
	case DR_UL:
	    dr_bgtul(c, left, right, true_label);	/* op_i_bgtul */
	    break;
	case DR_F:
	    dr_bgtf(c, left, right, true_label);	/* op_i_bgtf */
	    break;
	case DR_D:
	    dr_bgtd(c, left, right, true_label);	/* op_i_bgtd */
							/* op_i_bgtp */
							/* op_i_bnep */
							/* op_i_beqp */
							/* op_i_blep */
							/* op_i_bgep */
							/* op_i_bltp */
	    break;
	default:
	    assert(FALSE);
	}
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, true_label);
	break;
    }
    case  op_eq:{
	dr_label_type true_label;
	if (string_op) {
	    /* this has to be strcmp */
	    dr_reg_type rv;
	    dr_push_init(c);
	    if (dr_do_reverse_vararg_push(c)) {
		dr_push_argp(c, right);
		dr_push_argp(c, left);
	    } else {
		dr_push_argp(c, left);
		dr_push_argp(c, right);
	    }
#ifdef i_callii
	    rv = dr_getvreg(c, DR_I);
	    i_callii(rv, (void *)ecl_streq);
#else
	    rv = dr_calli(c, (void *)ecl_streq);
#endif
	    dr_movi(c, result, rv);
	    break;
	}
	true_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	switch (op_type) {
	case DR_I:
	    dr_beqi(c, left, right, true_label);	/* op_i_beqi */
	    break;
	case DR_U:
	    dr_bequ(c, left, right, true_label);	/* op_i_bequ */
	    break;
	case DR_L:
	    dr_beql(c, left, right, true_label);	/* op_i_beql */
	    break;
	case DR_UL:
	    dr_bequl(c, left, right, true_label);	/* op_i_bequl */
	    break;
	case DR_F:
	    dr_beqf(c, left, right, true_label);	/* op_i_beqf */
	    break;
	case DR_D:
	    dr_beqd(c, left, right, true_label);	/* op_i_beqd */
	    break;
	default:
	    assert(FALSE);
	}
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, true_label);
	break;
    }
    case  op_neq: {
	dr_label_type true_label = dr_genlabel(c);
	dr_seti(c, result, 0);	/* op_i_seti */
	switch (op_type) {
	case DR_I:
	    dr_bnei(c, left, right, true_label);	/* op_i_bnei */
	    break;
	case DR_U:
	    dr_bneu(c, left, right, true_label);	/* op_i_bneu */
	    break;
	case DR_L:
	    dr_bnel(c, left, right, true_label);	/* op_i_bnel */
	    break;
	case DR_UL:
	    dr_bneul(c, left, right, true_label);	/* op_i_bneul */
	    break;
	case DR_F:
	    dr_bnef(c, left, right, true_label);	/* op_i_bnef */
	    break;
	case DR_D:
	    dr_bned(c, left, right, true_label);	/* op_i_bned */
	    break;
	default:
	    assert(FALSE);
	}
	dr_seti(c, result, 1);	/* op_i_seti */
	dr_label(c, true_label);
	break;
    }
    case  op_log_or:
    case  op_arith_or:
	switch (op_type) {
	case DR_I:
	    dr_ori(c, result, left, right);	/* op_i_ori */
	    break;
	case DR_U:
	    dr_oru(c, result, left, right);	/* op_i_oru */
	    break;
	case DR_L:
	    dr_orl(c, result, left, right);	/* op_i_orl */
	    break;
	case DR_UL:
	    dr_orul(c, result, left, right);	/* op_i_orul */
	    break;
	case DR_P:
	    if(op == op_arith_or) {
		/* Arithmetic OR not allowed on pointers. */
		assert(FALSE);
	    } else {
		dr_cvul2p(c, left, left);	/* op_i_cvul2p */
		dr_cvul2p(c, right, right);	/* op_i_cvul2p */
		dr_orul(c, result, left, right);
	    }
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case  op_arith_xor:
	switch (op_type) {
	case DR_I:
	    dr_xori(c, result, left, right);	/* op_i_xori */
	    break;
	case DR_U:
	    dr_xoru(c, result, left, right);	/* op_i_xoru */
	    break;
	case DR_L:
	    dr_xorl(c, result, left, right);	/* op_i_xorl */
	    break;
	case DR_UL:
	    dr_xorul(c, result, left, right);	/* op_i_xorul */
	    break;
	case DR_P:
	    /* Fall-through. Arithmetic AND not allowed on pointers. */
	default:
	    assert(FALSE);
	}
	break;
    case  op_log_and:
    case  op_arith_and:
	switch (op_type) {
	case DR_I:
	    dr_andi(c, result, left, right);	/* op_i_andi */
	    break;
	case DR_U:
	    dr_andu(c, result, left, right);	/* op_i_andu */
	    break;
	case DR_L:
	    dr_andl(c, result, left, right);	/* op_i_andl */
	    break;
	case DR_UL:
	    dr_andul(c, result, left, right);	/* op_i_andul */
	    break;
	case DR_P:
	    if(op == op_arith_and) {
		/* Arithmetic AND not allowed on pointers. */
		assert(FALSE);
	    } else {
		dr_cvul2p(c, left, left);	/* op_i_cvul2p */
		dr_cvul2p(c, right, right);	/* op_i_cvul2p */
		dr_andul(c, result, left, right);
	    }
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case op_right_shift:
	switch (op_type) {
	case DR_I:
	    dr_rshi(c, result, left, right);	/* op_i_rshi */
	    break;
	case DR_U:
	    dr_rshu(c, result, left, right);	/* op_i_rshu */
	    break;
	case DR_L:
	    dr_rshl(c, result, left, right);	/* op_i_rshl */
	    break;
	case DR_UL:
	    dr_rshul(c, result, left, right);	/* op_i_rshul */
	    break;
	case DR_P:
	    /* Fall-through. No shifts allowed for pointers. */
	default:
	    assert(FALSE);
	}
	break;
    case op_left_shift:
	switch (op_type) {
	case DR_I:
	    dr_lshi(c, result, left, right);	/* op_i_lshi */
	    break;
	case DR_U:
	    dr_lshu(c, result, left, right);	/* op_i_lshu */
	    break;
	case DR_L:
	    dr_lshl(c, result, left, right);	/* op_i_lshl */
	    break;
	case DR_UL:
	    dr_lshul(c, result, left, right);	/* op_i_lshul */
	    break;
	case DR_P:
	    /* Fall-through. No shifts allowed for pointers. */
	default:
	    assert(FALSE);
	}
	break;
    case  op_mult:
	switch (op_type) {
	case DR_I:
	    dr_muli(c, result, left, right);	/* op_i_muli */
	    break;
	case DR_U:
	    dr_mulu(c, result, left, right);	/* op_i_mulu */
	    break;
	case DR_L:
	    dr_mull(c, result, left, right);	/* op_i_mull */
	    break;
	case DR_UL:
	    dr_mulul(c, result, left, right);	/* op_i_mulul */
	    break;
	case DR_F:
	    dr_mulf(c, result, left, right);	/* op_i_mulf */
	    break;
	case DR_D:
	    dr_muld(c, result, left, right);	/* op_i_muld */
	    break;
	case DR_P:
	    /* Multiplication not applicable to pointers */
	    assert(FALSE);
	    break;
	}
	break;
    case  op_div:
	switch (op_type) {
	case DR_I:
	    dr_divi(c, result, left, right);	/* op_i_divi */
	    break;
	case DR_U:
	    dr_divu(c, result, left, right);	/* op_i_divu */
	    break;
	case DR_L:
	    dr_divl(c, result, left, right);	/* op_i_divl */
	    break;
	case DR_UL:
	    dr_divul(c, result, left, right);	/* op_i_divul */
	    break;
	case DR_F:
	    dr_divf(c, result, left, right);	/* op_i_divf */
	    break;
	case DR_D:
	    dr_divd(c, result, left, right);	/* op_i_divd */
	    break;
	case DR_P:
	    /* Division not applicable to pointers */
	    assert(FALSE);
	    break;
	}
	break;
    case  op_deref:
    {
	sm_ref typ = get_complex_type(NULL, expr->node.operator.right);
	operand right_op;
	init_operand(&right_op);
	right_op.reg = right;
	right_op.is_addr = 1;
	right_op.in_kernel = 0;
	right_op.offset = 0;

	if(typ && typ->node_type == ecl_reference_type_decl) {
	    if(typ->node.reference_type_decl.kernel_ref)
		right_op.in_kernel = 1;
	    else
		right_op.in_kernel = 0;
	}

	if (!need_assignable) {
	    if(right_op.in_kernel) {
		dr_special(c, DR_SEGMENTED_FOLLOWS, DR_X86_GS_PREFIX);
	    }
	    gen_load(c, result, right_op, expr->node.operator.result_type);
	    right_op.reg = result;
	    right_op.is_addr = 0;
	    right_op.in_kernel = 0;
	}
	return right_op;
	break;
    }
    case  op_address:
	ret_op.reg = right;
	return ret_op;
	break;
    case op_log_neg:
	/* handled by if at top */
	assert(FALSE);
	break;
    case op_sizeof:
    case op_inc:
    case op_dec: 
	/* handled at top */
	assert(FALSE);
	break;
    }
    ret_op.reg = result;
    return ret_op;
}

static int
next_formal_is_drisc_exec_ctx(sm_list formals)
{
    sm_ref formal, ct;
    if (formals == NULL) return 0;
    formal = formals->node;
    ct = formal->node.declaration.sm_complex_type;
    if (ct == NULL) return 0;
    if ((ct->node_type == ecl_reference_type_decl) &&
	(ct->node.reference_type_decl.name != NULL) &&
	(strcmp(ct->node.reference_type_decl.name, "ecl_exec_context")
	 == 0)) {
	return 1;
    }
    return 0;
}
    

static int
next_formal_is_ecl_type_spec(sm_list formals)
{
    sm_ref formal, ct;
    if (formals == NULL) return 0;
    formal = formals->node;
    ct = formal->node.declaration.sm_complex_type;
    if (ct == NULL) return 0;
    if ((ct->node_type == ecl_reference_type_decl) &&
	(strcmp(ct->node.reference_type_decl.name, "ecl_type_spec")
	 == 0)) {
	return 1;
    }
    return 0;
}
    

#define MAX_ARG 128
static operand
cg_subroutine_call(drisc_ctx c, sm_ref expr)
{
    sm_ref func_ref = expr->node.subroutine_call.sm_func_ref;
    sm_list formals = func_ref->node.declaration.params;
    sm_list args = expr->node.subroutine_call.arguments;
    sm_ref arg, formal;
    sm_ref last_arg = NULL;
#ifndef HAVE_ICODE_H
    dr_reg_type args_array[MAX_ARG];  /* should be large enough */
    int types_array[MAX_ARG];  /* should be large enough */
    int start, i, direction;
#endif
    int arg_count = 0;

    operand ret;

    dr_push_init(c);

    init_operand(&ret);
    ret.is_addr = 0;
    ret.offset = 0;
    if (next_formal_is_drisc_exec_ctx(formals)) {
	if (arg_count < MAX_ARG) {
	    args_array[arg_count] = 0; /* context must be first param */
	    types_array[arg_count] = DR_P;
	}
	arg_count++;
	formals = formals->next;
    }
    while (args != NULL) {
	operand param;
	int formal_type;
	arg = args->node;
	if (formals != NULL) {
	    formal = formals->node;
	    if (strcmp(formal->node.declaration.id, "...") == 0) {
		formal = NULL;
	    }
	} else {
	    formal = NULL;
	}
	if (formal != NULL) {
	    int actual_type = ecl_sm_get_type(arg);
	    formal_type = formal->node.declaration.cg_type;
	    if (actual_type == DR_B) {
		/* structure param */
		param = cg_expr(c, arg, 1);
		if (is_complex_type(arg) && (param.is_addr == 0)) {
		    param.is_addr = 1;
		}
	    } else {
		param = cg_expr(c, arg, 0);
		param.reg = coerce_type(c, param.reg, formal_type,
					actual_type);
	    }
	} else {
	    param = cg_expr(c, arg, 0);
	    formal_type = ecl_sm_get_type(arg);
	}
#ifdef HAVE_ICODE_H	
	switch (formal_type) {
	case DR_C: case DR_S: case DR_I:
	    i_argi(param.reg);	/* op_i_argi */
	    break;
	case DR_UC: case DR_US: case DR_U:
	    i_argu(param.reg);	/* op_i_argu */
	    break;
	case DR_L:
	    i_argl(param.reg);	/* op_i_argl */
	    break;
	case DR_UL:
	    i_argul(param.reg);	/* op_i_argul */
	    break;
	case DR_F:
	    i_argf(param.reg);	/* op_i_argf */
	    break;
	case DR_D:
	    i_argd(param.reg);	/* op_i_argd */
	    break;
	case DR_P:
	case DR_EC:
	    i_argp(param.reg);  /* op_i_argp */
	    break;
	default:
	    printf("unhandled case in subroutine call\n");
	    assert(0);
	}
#else
	if (arg_count < MAX_ARG) {
	    args_array[arg_count] = param.reg;
	    types_array[arg_count] = formal_type;
	}
#endif
	arg_count++;
	args = args->next;
	last_arg = arg;
	if (formals) formals = formals->next;
	if (next_formal_is_drisc_exec_ctx(formals)) {
	    if (arg_count < MAX_ARG) {
		args_array[arg_count] = 0; /* context must be first param */
		types_array[arg_count] = DR_P;
	    }
	    arg_count++;
	    formals = formals->next;
	}
    }	

#ifdef HAVE_ICODE_H    
    switch (ecl_sm_get_type(expr)) {
	case DR_C: case DR_S: case DR_I:
	    ret.reg = dr_getvreg(c, DR_I);
	    i_callii(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_callii */
	    break;
        case DR_UC: case DR_US: case DR_U:
	    ret.reg = dr_getvreg(c, DR_U);
	    i_callui(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_callui */
	    break;
	case DR_L:
	    ret.reg = dr_getvreg(c, DR_L);
	    i_callli(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_callli */
	    break;
	case DR_UL:
	    ret.reg = dr_getvreg(c, DR_UL);
	    i_calluli(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_calluli */
	    break;
	case DR_F:
	    ret.reg = dr_getvreg(c, DR_F);
	    i_callfi(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_callfi */
	    break;
	case DR_D:
	    ret.reg = dr_getvreg(c, DR_D);
	    i_calldi(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_calldi */
	    break;
	case DR_P:
	    ret.reg = dr_getvreg(c, DR_P);
	    i_callli(ret.reg, func_ref->node.declaration.cg_address);  /* op_i_callli */
	    break;
	default:
	    printf("unhandled case in subroutine call\n");
	    assert(0);
	}
#else
    if (dr_do_reverse_vararg_push(c)) {
	start = arg_count - 1;
	direction = -1;
    } else {
	start = 0;
	direction = 1;
    }
    for (i=0; i< arg_count; i++) {
	int index = start + i * direction;
	dr_push_arg(c, types_array[index], args_array[index]);
    }
    ret.reg = dr_pcall(c, ecl_sm_get_type(expr),
		       func_ref->node.declaration.cg_address);
#endif
    return ret;
}

static void
ecl_expand_dyn_array(void *base_addr, int new_size, int old_size, int struct_size)
{

    static int debug_cg = -1;

    if (debug_cg == -1) {
	debug_cg = (int)(long)(getenv("ECL_DEBUG"));
    }
    if (debug_cg) {
	printf("ecl_expand_dyn_array, base_addr %lx, old_base %lx, new_size %d, old_size %d, struct_size %d\n",
	       (long)base_addr, *(long*)base_addr, new_size, old_size, 
	       struct_size);
    }
    if (*(void**)base_addr == NULL) {
	*(void**)base_addr = malloc(new_size * struct_size);
	memset(*(void**)base_addr, 0, new_size *struct_size);
    } else {
	if (new_size > old_size) {
	    *(void**)base_addr = realloc(*(void**)base_addr, 
					 new_size * struct_size);
	    memset(*(char **)base_addr + old_size *struct_size, 0, 
		   (new_size - old_size) * struct_size);
	}
    }	
    if (debug_cg) {
	printf("\tnew base %lx\n", *(long*)base_addr);
    }
}

static void do_var_array_adjust(drisc_ctx c, sm_ref expr, operand old_val, 
				operand new_val)
{
    sm_ref strct;
    sm_ref struct_decl;
    sm_list fields;
    assert(expr->node_type == ecl_field_ref);

    strct = expr->node.field_ref.struct_ref;
    expr = expr->node.field_ref.sm_field_ref;
    struct_decl = get_complex_type(NULL, strct);
    if (struct_decl->node_type == ecl_reference_type_decl) {
	struct_decl = struct_decl->node.reference_type_decl.sm_complex_referenced_type;
    }
    fields = struct_decl->node.struct_type_decl.fields;
    while(fields != NULL) {
	sm_ref tmp, darray_field;
	int size_reg, old_size;
	operand array_base;
	int dimen = -1, i = 0;
	sm_ref typ = fields->node->node.field.sm_complex_type;
	if ((typ == NULL) || (typ->node_type != ecl_reference_type_decl)) {
	    fields = fields->next;
	    continue;
	}
	typ = typ->node.reference_type_decl.sm_complex_referenced_type;
	while (typ != NULL) {
	    if (typ->node_type != ecl_array_type_decl) {
		typ = NULL;
		continue;
	    }
	    if (typ->node.array_type_decl.sm_dynamic_size == expr) {
		dimen = i;
	    }
	    i++;
	    typ = typ->node.array_type_decl.sm_complex_element_type;
	}
	if (dimen == -1) {
	    fields = fields->next;
	    continue;
	}
	darray_field = fields->node;
	tmp = malloc(sizeof(*tmp));
	tmp->node_type = ecl_field_ref;
	tmp->node.field_ref.struct_ref = strct;
	tmp->node.field_ref.lx_field = NULL;
	tmp->node.field_ref.sm_field_ref = darray_field;
	array_base = cg_expr(c, tmp, 1);
	free(tmp);
	fields = fields->next;
	if (array_base.offset != 0) {
	    int new_base = dr_getvreg(c, DR_P);
	    dr_addpi(c, new_base, array_base.reg, array_base.offset); /* op_i_addpi */
	    array_base.reg = new_base;
	    array_base.offset = 0;
	}
	size_reg = dr_getvreg(c, DR_I);
	old_size = dr_getvreg(c, ecl_sm_get_type(expr));
	gen_load(c, old_size, old_val, DR_I);
	dr_seti(c, size_reg, darray_field->node.field.cg_size);

	dr_push_init(c);
	if (dr_do_reverse_vararg_push(c)) {
	    dr_push_argi(c, size_reg);
	    dr_push_argi(c, old_size);
	    dr_push_argi(c, new_val.reg);
	    dr_push_argp(c, array_base.reg);
	} else {	    
	    dr_push_argp(c, array_base.reg);
	    dr_push_argi(c, new_val.reg);
	    dr_push_argi(c, old_size);
	    dr_push_argi(c, size_reg);
	}
	dr_callv(c, (void*)&ecl_expand_dyn_array);  /* op_i_callvi */
    }
}

static int 
is_var_array(sm_ref expr)
{
    sm_ref typ;
    if (expr->node_type == ecl_field_ref) {
	return is_var_array(expr->node.field_ref.sm_field_ref);
    }
    if (expr->node_type == ecl_field) {
	sm_ref ref = expr->node.field.sm_complex_type;
	if (ref == NULL) return 0;
	if (ref->node_type != ecl_reference_type_decl) return 0;
	if (ref->node.reference_type_decl.sm_complex_referenced_type == NULL)
	    return 0;
	return (ref->node.reference_type_decl.sm_complex_referenced_type->node_type == ecl_array_type_decl);
    }
    typ = get_complex_type(NULL, expr);
    if ((typ == NULL) || (typ->node_type != ecl_array_type_decl)) return 0;
    if (typ->node.array_type_decl.cg_static_size == -1) {
	/* this array is varying */
	return 1;
    }
    if (typ->node.array_type_decl.cg_element_size == -1) {
	/* subarray is varying */
	return 1;
    }

    return 0;
}

static dr_reg_type
gen_dynamic_element_size(drisc_ctx c, sm_ref arr, sm_ref containing)
{
    sm_ref tmp;
    tmp = malloc(sizeof(*tmp));
    tmp->node_type = ecl_field_ref;
    tmp->node.field_ref.struct_ref = containing;
    tmp->node.field_ref.lx_field = NULL;
    tmp->node.field_ref.sm_field_ref = NULL;

    if (arr->node.array_type_decl.cg_element_size == -1) {
	/* subarray is dynamic */
	sm_ref subtyp =	arr->node.array_type_decl.sm_complex_element_type;
	dr_reg_type subsize = gen_dynamic_element_size(c, subtyp, containing);
	if (arr->node.array_type_decl.cg_static_size != -1) {
	    dr_mulii(c, subsize, subsize, 
		     arr->node.array_type_decl.cg_static_size);
	    free(tmp);
	    return subsize;
	} else {
	    operand dsize;
	    tmp->node.field_ref.sm_field_ref =
		arr->node.array_type_decl.sm_dynamic_size;
	    dsize = cg_expr(c, tmp, 0);
	    dr_muli(c, subsize, subsize, dsize.reg);
	    free(tmp);
	    return subsize;
	}
    } else {
	/* subarray is static, we must be dynamic */
	operand dsize;
	tmp->node.field_ref.sm_field_ref =
	    arr->node.array_type_decl.sm_dynamic_size;
	dsize = cg_expr(c, tmp, 0);
	if (arr->node.array_type_decl.cg_element_size != 1)
	    dr_mulii(c, dsize.reg, dsize.reg, 
		     arr->node.array_type_decl.cg_element_size);
	free(tmp);
	return dsize.reg;
    }
}		

static int  /* return true if this is a structured type or a reference type */
is_complex_type(sm_ref expr)
{
    switch(expr->node_type) {
    case ecl_identifier:
	return is_complex_type(expr->node.identifier.sm_declaration);
    case ecl_declaration:
	if (expr->node.declaration.sm_complex_type == NULL) return 0;
	return is_complex_type(expr->node.declaration.sm_complex_type);
    case ecl_struct_type_decl:
	return 1;
    case ecl_reference_type_decl:
	return 1;
    case ecl_array_type_decl:
	return 1;
    case ecl_field_ref:
	return is_complex_type(expr->node.field_ref.sm_field_ref);
    case ecl_element_ref:
	return is_complex_type(expr->node.element_ref.sm_complex_element_type);
    case ecl_field:
	return is_complex_type(expr->node.field.sm_complex_type);
    default:
	printf("unhandled case in is_complex_type() cg.c\n");
	assert(0);
    }
}

static operand
cg_expr(drisc_ctx c, sm_ref expr, int need_assignable)
{
    operand oprnd;

    init_operand(&oprnd);
    switch(expr->node_type) {
    case ecl_identifier:
	return cg_expr(c, expr->node.identifier.sm_declaration, need_assignable);
    case ecl_declaration:
	if (expr->node.declaration.static_var || 
	    expr->node.declaration.is_extern) {
	    dr_reg_type lvar = dr_getvreg(c, DR_P);
	    dr_setp(c, lvar, (long)expr->node.declaration.cg_address);  /* op_i_setp */
	    oprnd.reg = lvar;
	    oprnd.offset = 0;
	    oprnd.is_addr = 1;
	    if (need_assignable == 1) {
		return oprnd;
	    } else {
		dr_reg_type ret = dr_getvreg(c, expr->node.declaration.cg_type);
		gen_load(c, ret, oprnd, expr->node.declaration.cg_type);
		oprnd.reg = ret;
		oprnd.is_addr = 0;
		oprnd.offset = 0;
		return oprnd;
	    }
	}
	if (need_assignable) {
	    if (expr->node.declaration.param_num != -1) {
		/* it's a parameter */
		int is_reference_type = 0;
		if (expr->node.declaration.sm_complex_type &&
		    (expr->node.declaration.sm_complex_type->node_type ==
		    ecl_reference_type_decl)) {
		    is_reference_type++;
		}
		if ((expr->node.declaration.cg_type != DR_P) ||
		    (is_reference_type)) {
		    /* passed by value */
		    oprnd.reg = expr->node.declaration.cg_oprnd;
		    oprnd.is_addr = 0;
		    oprnd.offset = 0;
		} else {
		    /* passed by reference */
		    oprnd.reg = expr->node.declaration.cg_oprnd;
		    oprnd.is_addr = 1;
		    oprnd.offset = 0;
		}
	    } else {
		/* it's a local */
		if ((expr->node.declaration.cg_type != DR_B)  &&
		    ! expr->node.declaration.addr_taken) {
		    /* value is assignable */
		    oprnd.reg = expr->node.declaration.cg_oprnd;
		    oprnd.is_addr = 0;
		    oprnd.offset = 0;
		} else {
		    dr_reg_type addr_reg = dr_getvreg(c, DR_P);
		    dr_virtual_lea(c, addr_reg, expr->node.declaration.cg_oprnd);	/* op_i_leai */
		    oprnd.reg = addr_reg;
		    oprnd.is_addr = 1;
		    oprnd.offset = 0;
		}
	    }
	} else {
	    if (expr->node.declaration.addr_taken) {
		dr_reg_type addr_reg = dr_getvreg(c, DR_P);
		dr_reg_type ret = dr_getvreg(c, expr->node.declaration.cg_type);
		dr_virtual_lea(c, addr_reg, expr->node.declaration.cg_oprnd);	/* op_i_leai */
		oprnd.reg = addr_reg;
		oprnd.is_addr = 1;
		oprnd.offset = 0;
		gen_load(c, ret, oprnd, expr->node.declaration.cg_type);
		oprnd.reg = ret;
		oprnd.is_addr = 0;
		oprnd.offset = 0;
	    } else {
		oprnd.reg = expr->node.declaration.cg_oprnd;
		oprnd.is_addr = 0;
		oprnd.offset = 0;
	    }
	}
	return oprnd;
    case ecl_operator: {
	oprnd = cg_operator(c, expr, need_assignable);
	return oprnd;
    }
    case ecl_cast: {
	operand right;
	int expr_type = ecl_sm_get_type(expr->node.cast.expression);
	right = cg_expr(c, expr->node.cast.expression, 0);
	assert(right.is_addr == 0);
	right.reg = coerce_type(c, right.reg, expr->node.cast.cg_type, 
				expr_type);
	return right;
    }
    case ecl_constant: {
	dr_reg_type lvar = -1;
	if (expr->node.constant.token == string_constant) {
	    lvar = dr_getvreg(c, DR_P);
	    /* memory leak of constant value here.... */
	    dr_setp(c, lvar, (long)strdup(expr->node.constant.const_val));  /* op_i_setp */
	} else if (expr->node.constant.token == floating_constant) {
	    float f;

	    lvar = dr_getvreg(c, DR_F);
	    sscanf(expr->node.constant.const_val, "%f", &f);
	    dr_setf(c, lvar, f);	/* op_i_setf */
	} else {
	    long i;
	    char *val = expr->node.constant.const_val;
	    lvar = dr_getvreg(c, DR_I);
	    if (val[0] == '0') {
		/* hex or octal */
		if (val[1] == 'x') {
		    /* hex */
		    if (sscanf(val+2, "%lx", &i) != 1) 
			printf("hex sscanf failed, %s\n", val);
		} else {
		    if (sscanf(val, "%lo", &i) != 1) 
			printf("octal sscanf failed %s\n", val);
		}
	    } else {
		if (sscanf(val, "%ld", &i) != 1) 
		    printf("decimal sscanf failed %s\n", val);
	    }
	    dr_seti(c, lvar, i);	/* op_i_seti */
	}
	oprnd.reg = lvar;
	oprnd.is_addr = 0;
	oprnd.offset = 0;
	return oprnd;
    }
    case ecl_assignment_expression: {
	operand right;
	operand left;
	int assign_type = ecl_sm_get_type(expr);
	right = cg_expr(c, expr->node.assignment_expression.right, 0);
	left = cg_expr(c, expr->node.assignment_expression.left, 1);
	assert(right.is_addr == 0);
	right.reg = coerce_type(c, right.reg, assign_type, 
				ecl_sm_get_type(expr->node.assignment_expression.right));
	if (is_control_value(expr->node.assignment_expression.left, NULL)) {
	    do_var_array_adjust(c, expr->node.assignment_expression.left, left,
				right);
	}
	if (ecl_expr_is_string(expr->node.assignment_expression.right)) {
	    dr_push_init(c);
	    dr_push_argp(c, right.reg);
#ifdef i_callii
	    right.reg = dr_getvreg(c, DR_P);
	    i_callli(rv, (void *)ecl_strdup);
#else
	    right.reg = dr_calll(c, (void *)ecl_strdup);
#endif
	}

	if(left.is_addr && left.in_kernel) {
	    dr_special(c, DR_SEGMENTED_FOLLOWS, DR_X86_GS_PREFIX);
	}
	gen_mov(c, left, right.reg, assign_type);
	return left;
    }
    case ecl_field_ref: {
	operand base = cg_expr(c, expr->node.field_ref.struct_ref, 1);
	sm_ref field = expr->node.field_ref.sm_field_ref;
	base.offset += field->node.field.cg_offset;
	if (is_var_array(field) && (need_assignable == 0)) {
	    /* variable array */
	    dr_reg_type ret = dr_getvreg(c, DR_P);
	    gen_load(c, ret, base, DR_P);
	    oprnd.reg = ret;
	    oprnd.is_addr = 1;
	    oprnd.offset = 0;
	    return oprnd;
	}
	{
	    sm_ref base_expr = expr->node.field_ref.struct_ref;
	    if (is_complex_type(base_expr) && (base.is_addr == 0)) {
		base.is_addr = 1;
	    }
	}

	if (need_assignable == 1) {
	    return base;
	} else {
	    dr_reg_type ret = dr_getvreg(c, field->node.field.cg_type);
	    gen_load(c, ret, base, field->node.field.cg_type);
	    oprnd.reg = ret;
	    oprnd.is_addr = 0;
	    oprnd.offset = 0;
	    return oprnd;
	}
    }
    case ecl_element_ref: {
	operand base = cg_expr(c, expr->node.element_ref.array_ref, 1);
	operand index = cg_expr(c, expr->node.element_ref.expression, 0);
	dr_reg_type new_base = dr_getvreg(c, DR_P);
	int cg_type;
	sm_ref arr;
	int size;
	arr = get_complex_type(NULL, expr->node.element_ref.array_ref);
	if (arr->node_type == ecl_reference_type_decl) {
	    /* we didn't load the address */
	    dr_reg_type ret = dr_getvreg(c, DR_P);
	    gen_load(c, ret, base, DR_P);
	    base.reg = ret;
	    base.is_addr = 1;
	    base.offset = 0;
	    arr = arr->node.reference_type_decl.sm_complex_referenced_type;
	}
	size = arr->node.array_type_decl.cg_element_size;
	cg_type = arr->node.array_type_decl.cg_element_type;
	if (size == -1) {
	    /* subarray is dynamic */
	    sm_ref subtyp = arr->node.array_type_decl.sm_complex_element_type;
	    sm_ref containing = expr->node.element_ref.sm_containing_structure_ref;
	    dr_reg_type dsize = gen_dynamic_element_size(c, subtyp, containing);
	    dr_muli(c, new_base, index.reg, dsize);
	    dr_addp(c, new_base, new_base, base.reg);
	} else if (size != 1) {
	    dr_mulii(c, new_base, index.reg, size);	/* op_i_mulii */
	    dr_addp(c, new_base, new_base, base.reg);	/* op_i_addp */
	} else {
	    dr_addp(c, new_base, index.reg, base.reg);
	}
	base.reg = new_base;
	if (need_assignable == 1) {
	    return base;
	} else {
	    dr_reg_type ret = dr_getvreg(c, cg_type);
	    gen_load(c, ret, base, cg_type);
	    oprnd.reg = ret;
	    oprnd.is_addr = 0;
	    oprnd.offset = 0;
	    return oprnd;
	}
    }
    case ecl_subroutine_call:
	return cg_subroutine_call(c, expr);
    default:
	fprintf(stderr, "Unknown case in cg_expression\n");
	ecl_print(expr);
    }
    oprnd.reg = -1;
    oprnd.is_addr = -1;
    oprnd.offset = -1;
    return oprnd;
}

static dr_reg_type
coerce_type(drisc_ctx c, dr_reg_type obj, int target_type, int obj_type)
{
    dr_reg_type ret;

    if (target_type == obj_type) return obj;

    ret = dr_getvreg(c, target_type);
    switch(target_type) {
    case DR_C:
    case DR_UC:
	switch (obj_type) {
	case DR_UC:
	    dr_orii(c, ret, obj, 0xff);
	    break;
	case DR_C:
	    dr_cvuc2i(c, ret, obj);
	    break;
	case DR_I:
	    dr_cvi2c(c, ret, obj);	/* op_i_cvi2c */
	    break;
	case DR_U:
	    dr_cvu2c(c, ret, obj);	/* op_i_cvu2c */
	    break;
	case DR_L:
	    dr_cvl2c(c, ret, obj);	/* op_i_cvl2c */
	    break;
	case DR_UL:
	    dr_cvul2c(c, ret, obj);	/* op_i_cvul2c */
	    break;
	case DR_F:
	    if (target_type == DR_UC) {
	        dr_cvf2u(c, ret, obj);	/* op_i_cvf2u */
		dr_cvu2c(c, ret, ret);	/* op_i_cvu2c */
	    } else {
	        dr_cvf2i(c, ret, obj);	/* op_i_cvf2i */
		dr_cvi2c(c, ret, ret);	/* op_i_cvi2c */
	    }	    
	    break;
	case DR_D:
	    if (target_type == DR_UC) {
	        dr_cvd2u(c, ret, obj);	/* op_i_cvd2u */
		dr_cvu2c(c, ret, ret);	/* op_i_cvu2c */
	    } else {
	        dr_cvd2i(c, ret, obj);	/* op_i_cvd2i */
		dr_cvi2c(c, ret, ret);	/* op_i_cvi2c */
	    }
	    break;
	case DR_P:
	    dr_cvp2ul(c, ret, obj);	/* op_i_cvp2ul */
	    dr_cvul2c(c, ret, ret);	/* op_i_cvul2c */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_S:
    case DR_US:
	switch (obj_type) {
	case DR_I:
	    dr_cvi2s(c, ret, obj);	/* op_i_cvi2s */
	    break;
	case DR_U:
	    dr_cvu2s(c, ret, obj);	/* op_i_cvu2s */
	    break;
	case DR_L:
	    dr_cvl2s(c, ret, obj);	/* op_i_cvl2s */
	    break;
	case DR_UL:
	    dr_cvul2s(c, ret, obj);	/* op_i_cvul2s */
	    break;
	case DR_F:
	    if (target_type == DR_US) {
	        dr_cvf2u(c, ret, obj);	/* op_i_cvf2u */
		dr_cvu2s(c, ret, ret);	/* op_i_cvu2s */
	    } else {
	        dr_cvf2i(c, ret, obj);	/* op_i_cvf2i */
		dr_cvi2s(c, ret, ret);	/* op_i_cvi2s */
	    }
	    break;
	case DR_D:
	    if (target_type == DR_US) {
	        dr_cvd2u(c, ret, obj);	/* op_i_cvd2u */
		dr_cvu2s(c, ret, ret);	/* op_i_cvu2s */
	    } else {
	        dr_cvd2i(c, ret, obj);	/* op_i_cvd2i */
		dr_cvi2s(c, ret, ret);	/* op_i_cvi2s */
	    }
	    break;
	case DR_P:
	    dr_cvp2ul(c, ret, obj);	/* op_i_cvp2ul */
	    dr_cvul2s(c, ret, ret);	/* op_i_cvul2s */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_I:
	switch (obj_type) {
	case DR_C:
	    dr_cvc2i(c, ret, obj);	/* op_i_cvc2i */
	    break;
	case DR_UC:
	    dr_andii(c, ret, obj, 0xff); /* op_i_andii */
	    break;
	case DR_S:
	    dr_cvs2i(c, ret, obj);	/* op_i_cvs2i */
	    break;
	case DR_US:
	    dr_cvus2i(c, ret, obj);	/* op_i_cvus2i */
	    break;
	case DR_I:
	    assert(FALSE);
	    break;
	case DR_U:
	    dr_cvu2i(c, ret, obj);	/* op_i_cvu2i */
	    break;
	case DR_L:
	    dr_cvl2i(c, ret, obj);	/* op_i_cvl2i */
	    break;
	case DR_UL:
	    dr_cvul2i(c, ret, obj);	/* op_i_cvul2i */
	    break;
	case DR_F:
	    dr_cvf2i(c, ret, obj);	/* op_i_cvf2i */
	    break;
	case DR_D:
	    dr_cvd2i(c, ret, obj);	/* op_i_cvd2i */
	    break;
	case DR_P:
	    dr_cvp2ul(c, ret, obj);	/* op_i_cvp2ul */
	    dr_cvul2i(c, ret, ret);	/* op_i_cvul2i */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_U:
	switch (obj_type) {
	case DR_C:
	    dr_cvc2u(c, ret, obj);	/* op_i_cvc2u */
	    break;
	case DR_UC:
	    dr_andii(c, ret, obj, 0xff); /* op_i_andii */
	    break;
	case DR_S:
	    dr_cvs2u(c, ret, obj);	/* op_i_cvs2u */
	    break;
	case DR_US:
	    dr_cvus2u(c, ret, obj);	/* op_i_cvus2u */
	    break;
	case DR_I:
	    dr_cvi2u(c, ret, obj);	/* op_i_cvi2u */
	    break;
	case DR_U:
	    assert(FALSE);
	    break;
	case DR_L:
	    dr_cvl2u(c, ret, obj);	/* op_i_cvl2u */
	    break;
	case DR_UL:
	    dr_cvul2u(c, ret, obj);	/* op_i_cvul2u */
	    break;
	case DR_F:
	    dr_cvf2u(c, ret, obj);	/* op_i_cvf2u */
	    break;
	case DR_D:
	    dr_cvd2u(c, ret, obj);	/* op_i_cvd2u */
	    break;
	case DR_P:
	    dr_cvp2ul(c, ret, obj);	/* op_i_cvp2ul */
	    dr_cvul2u(c, ret, ret);	/* op_i_cvul2u */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_L:
	switch (obj_type) {
	case DR_C:
	    dr_cvc2l(c, ret, obj);	/* op_i_cvc2l */
	    break;
	case DR_UC:
	    dr_andii(c, ret, obj, 0xff); /* op_i_andii */
	    break;
	case DR_S:
	    dr_cvs2l(c, ret, obj);	/* op_i_cvs2l */
	    break;
	case DR_US:
	    dr_cvus2l(c, ret, obj);	/* op_i_cvus2l */
	    break;
	case DR_I:
	    dr_cvi2l(c, ret, obj);	/* op_i_cvi2l */
	    break;
	case DR_U:
	    dr_cvu2l(c, ret, obj);	/* op_i_cvu2l */
	    break;
	case DR_L:
	    assert(FALSE);
	    break;
	case DR_UL:
	    dr_cvul2l(c, ret, obj);	/* op_i_cvul2l */
	    break;
	case DR_F:
	    dr_cvf2l(c, ret, obj);	/* op_i_cvf2l */
	    break;
	case DR_D:
	    dr_cvd2l(c, ret, obj);	/* op_i_cvd2l */
	    break;
	case DR_P:
	    dr_cvp2ul(c, ret, obj);	/* op_i_cvp2ul */
	    dr_cvul2l(c, ret, ret);	/* op_i_cvul2l */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_UL:
	switch (obj_type) {
	case DR_C:
	    dr_cvc2ul(c, ret, obj);	/* op_i_cvc2ul */
	    break;
	case DR_UC:
	    dr_andii(c, ret, obj, 0xff); /* op_i_andii */
	    break;
	case DR_S:
	    dr_cvs2ul(c, ret, obj);	/* op_i_cvs2ul */
	    break;
	case DR_US:
	    dr_cvus2ul(c, ret, obj);	/* op_i_cvus2ul */
	    break;
	case DR_I:
	    dr_cvi2ul(c, ret, obj);	/* op_i_cvi2ul */
	    break;
	case DR_U:
	    dr_cvu2ul(c, ret, obj);	/* op_i_cvu2ul */
	    break;
	case DR_L:
	    dr_cvl2ul(c, ret, obj);	/* op_i_cvl2ul */
	    break;
	case DR_UL:
	    assert(FALSE);
	    break;
	case DR_F:
	    dr_cvf2ul(c, ret, obj);	/* op_i_cvf2ul */
	    break;
	case DR_D:
	    dr_cvd2ul(c, ret, obj);	/* op_i_cvd2ul */
	    break;
	case DR_P:
	    dr_cvp2ul(c, ret, obj);	/* op_i_cvp2ul */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_F:
	switch (obj_type) {
	case DR_S:
	case DR_US:
	case DR_UC:
	case DR_C:
	    dr_cvi2f(c, ret, coerce_type(c, obj, DR_I, obj_type));	/* op_i_cvi2f */
	    break;
	case DR_I:
	    dr_cvi2f(c, ret, obj);	/* op_i_cvi2f */
	    break;
	case DR_U:
	    dr_cvu2f(c, ret, obj);	/* op_i_cvu2f */
	    break;
	case DR_L:
	    dr_cvl2f(c, ret, obj);	/* op_i_cvl2f */
	    break;
	case DR_UL:
	    dr_cvul2f(c, ret, obj);	/* op_i_cvul2f */
	    break;
	case DR_F:
	    assert(FALSE);
	    break;
	case DR_D:
	    dr_cvd2f(c, ret, obj);	/* op_i_cvd2f */
	    break;
	default:
	    assert(FALSE);
	}
	break;
    case DR_D:
	switch (obj_type) {
	case DR_S:
	case DR_US:
	case DR_UC:
	case DR_C:
	    dr_cvi2d(c, ret, coerce_type(c, obj, DR_I, obj_type));	/* op_i_cvi2d */
	    break;
	case DR_I:
	    dr_cvi2d(c, ret, obj);	/* op_i_cvi2d */
	    break;
	case DR_U:
	    dr_cvu2d(c, ret, obj);	/* op_i_cvu2d */
	    break;
	case DR_L:
	    dr_cvl2d(c, ret, obj);	/* op_i_cvl2d */
	    break;
	case DR_UL:
	    dr_cvul2d(c, ret, obj);	/* op_i_cvul2d */
	    break;
	case DR_F:
	    dr_cvf2d(c, ret, obj);	/* op_i_cvf2d */
	    break;
	case DR_D:
	    assert(FALSE);
	    break;
	default:
	    assert(FALSE);
	}
	break;

    case DR_P:
	switch (obj_type) {
	case DR_L:
	    dr_cvl2ul(c, ret, obj);	/* op_i_cvl2ul */
	    dr_cvul2p(c, ret, ret);	/* op_i_cvul2p */
	    break;
	case DR_UL:
	    dr_cvul2p(c, ret, obj);	/* op_i_cvul2p */
	    break;
	case DR_P:
	    assert(FALSE);
	    break;
	case DR_I:
	    dr_cvi2ul(c, ret, obj);	/* op_i_cvul2p */
	    dr_cvul2p(c, ret, ret);	/* op_i_cvul2p */
	    break;
	default:
	    assert(FALSE);
	}
	break;

    default:
	assert(FALSE);
    }
    return ret;
}

static void
gen_load(drisc_ctx c, dr_reg_type left, operand right, int type)
{
    switch(type) {
    case DR_C:
	dr_ldci(c, left, right.reg, right.offset);	/* op_i_ldci */
	break;
    case DR_UC:
	dr_lduci(c, left, right.reg, right.offset);	/* op_i_lduci */
	break;
    case DR_S:
	dr_ldsi(c, left, right.reg, right.offset);	/* op_i_ldsi */
	break;
    case DR_US:
	dr_ldusi(c, left, right.reg, right.offset);	/* op_i_ldusi */
	break;
    case DR_I:
	dr_ldii(c, left, right.reg, right.offset);	/* op_i_ldii */
	break;
    case DR_U:
	dr_ldui(c, left, right.reg, right.offset);	/* op_i_ldui */
	break;
    case DR_L:
	dr_ldli(c, left, right.reg, right.offset);	/* op_i_ldli */
	break;
    case DR_UL:
	dr_lduli(c, left, right.reg, right.offset);	/* op_i_lduli */
	break;
    case DR_F:
	dr_ldfi(c, left, right.reg, right.offset);	/* op_i_ldfi */
	break;
    case DR_D:
	dr_lddi(c, left, right.reg, right.offset);	/* op_i_lddi */
	break;
    case DR_P:
	dr_ldpi(c, left, right.reg, right.offset);  /* op_i_ldpi */
	break;
    default:
	fprintf(stderr, "unhandled case in gen_load\n");
    }
}

static void
gen_mov(drisc_ctx c, operand left, dr_reg_type right, int type)
{
    if (left.is_addr == 0) {
	switch(type) {
	case DR_C:
	    dr_movc(c, left.reg, right);	/* op_i_movc */
	    break;
	case DR_UC:
	    dr_movuc(c, left.reg, right);	/* op_i_movuc */
	    break;
	case DR_S:
	    dr_movs(c, left.reg, right);	/* op_i_movs */
	    break;
	case DR_US:
	    dr_movus(c, left.reg, right);	/* op_i_movus */
	    break;
	case DR_I:
	    dr_movi(c, left.reg, right);	/* op_i_movi */
	    break;
	case DR_U:
	    dr_movu(c, left.reg, right);	/* op_i_movu */
	    break;
	case DR_L:
	    dr_movl(c, left.reg, right);	/* op_i_movl */
	    break;
	case DR_UL:
	    dr_movul(c, left.reg, right);	/* op_i_movul */
	    break;
	case DR_F:
	    dr_movf(c, left.reg, right);	/* op_i_movf */
	    break;
	case DR_D:
	    dr_movd(c, left.reg, right);	/* op_i_movd */
	    break;
	case DR_P:
	    dr_movp(c, left.reg, right);	/* op_i_movp */
	    break;
	default:
	    fprintf(stderr, "unhandled case in gen_mov, mov side\n");
	}
    } else {
	/* left is addr */
	switch(type) {
	case DR_C:
	    dr_stci(c, right, left.reg, left.offset);	/* op_i_stci */
	    break;
	case DR_UC:
	    dr_stuci(c, right, left.reg, left.offset);	/* op_i_stuci */
	    break;
	case DR_S:
	    dr_stsi(c, right, left.reg, left.offset);	/* op_i_stsi */
	    break;
	case DR_US:
	    dr_stusi(c, right, left.reg, left.offset);	/* op_i_stusi */
	    break;
	case DR_I:
	    dr_stii(c, right, left.reg, left.offset);	/* op_i_stii */
	    break;
	case DR_U:
	    dr_stui(c, right, left.reg, left.offset);	/* op_i_stui */
	    break;
	case DR_L:
	    dr_stli(c, right, left.reg, left.offset);	/* op_i_stli */
	    break;
	case DR_UL:
	    dr_stuli(c, right, left.reg, left.offset);	/* op_i_stuli */
	    break;
	case DR_F:
	    dr_stfi(c, right, left.reg, left.offset);	/* op_i_stfi */
	    break;
	case DR_D:
	    dr_stdi(c, right, left.reg, left.offset);	/* op_i_stdi */
	    break;
	case DR_P:
	    dr_stpi(c, right, left.reg, left.offset);	/* op_i_stpi */
	    break;
	default:
	    fprintf(stderr, "unhandled case in gen_mov st side\n");
	}
    }	
}

static void
gen_bnz(drisc_ctx c, int conditional, int target_label, int op_type)
{
    switch(op_type) {
    case DR_I:
	dr_bneii(c, conditional, 0, target_label);	/* op_i_bneii */
	break;
    case DR_U:
	dr_bneui(c, conditional, 0, target_label);	/* op_i_bneui */
	break;
    case DR_L:
	dr_bneli(c, conditional, 0, target_label);	/* op_i_bneli */
	break;
    case DR_UL:
	dr_bneuli(c, conditional, 0, target_label);	/* op_i_bneuli */
	break;
    case DR_F: {
	dr_reg_type tmp = dr_getvreg(c, DR_F);
	dr_setf(c, tmp, 0.0);	/* op_i_setf */
	dr_bnef(c, conditional, tmp, target_label);
	break;
    }
    case DR_D: {
	dr_reg_type tmp = dr_getvreg(c, DR_D);
	dr_setd(c, tmp, 0.0);
	dr_bned(c, conditional, tmp, target_label);	/* op_i_bned */
	break;
    }
    default:
	fprintf(stderr, "unhandled case in gen_bnz\n");
    }
}

static void
gen_bz(drisc_ctx c, int conditional, int target_label, int op_type)
{
    switch(op_type) {
    case DR_I:
	dr_beqii(c, conditional, 0, target_label);	/* op_i_beqii */
	break;
    case DR_U:
	dr_bequi(c, conditional, 0, target_label);	/* op_i_bequi */
	break;
    case DR_L:
    case DR_P:
	dr_beqli(c, conditional, 0, target_label);	/* op_i_beqli */
	break;
    case DR_UL:
	dr_bequli(c, conditional, 0, target_label);	/* op_i_bequli */
	break;
    case DR_F: {
	dr_reg_type tmp = dr_getvreg(c, DR_F);
	dr_setf(c, tmp, 0.0);	/* op_i_setf */
	dr_beqf(c, conditional, tmp, target_label);	/* op_i_beqf */
	break;
    }
    case DR_D: {
	dr_reg_type tmp = dr_getvreg(c, DR_D);
	dr_setd(c, tmp, 0.0);	/* op_i_setd */
	dr_beqd(c, conditional, tmp, target_label);	/* op_i_beqd */
	break;
    }
    default:
	fprintf(stderr, "unhandled case in gen_bz\n");
    }
}

static void cg_return_statement(drisc_ctx c, sm_ref stmt)
{
    operand ret_val;
    int expr_cg_type = ecl_sm_get_type(stmt->node.return_statement.expression);
    int func_cg_type = stmt->node.return_statement.cg_func_type;
    ret_val = cg_expr(c, stmt->node.return_statement.expression, 0);
    assert(ret_val.is_addr == 0);
    if (func_cg_type == DR_V) {
	dr_retii(c, 0);
	return;
    }
    ret_val.reg = coerce_type(c, ret_val.reg, func_cg_type, expr_cg_type);
#ifdef HAVE_DRISC_H
    dr_pret(c, func_cg_type, ret_val.reg);
#else
    switch(func_cg_type) {
    case DR_C:
    case DR_UC:
    case DR_S:
    case DR_US:
    case DR_I:
	dr_reti(c, ret_val.reg);	/* op_i_reti */
	break;
    case DR_U:
	dr_retu(c, ret_val.reg);	/* op_i_retu */
	break;
    case DR_L:
	dr_retl(c, ret_val.reg);	/* op_i_retl */
	break;
    case DR_UL:
	dr_retul(c, ret_val.reg);	/* op_i_retul */
	break;
    case DR_D:
	dr_retd(c, ret_val.reg);	/* op_i_retd */
	break;
    case DR_F:
	dr_retf(c, ret_val.reg);	/* op_i_retf */
	break;
    default:
	fprintf(stderr, "unhandled case in return\n");
	break;
    }
#endif
}

static int
is_comparison_operator(sm_ref expr)
{
    if (expr->node_type != ecl_operator) return 0;
    switch (expr->node.operator.op) {
    case op_neq: case op_eq: case op_gt: case op_geq: case op_lt: case op_leq:
	return 1;
    default:
	return 0;
    }
}

extern ecl_exec_context
ecl_create_exec_context(ecl_code code)
{
    drisc_exec_ctx ec;
    ec = dr_get_exec_context(code->drisc_context);
    return (ecl_exec_context) ec;
}


extern void
ecl_exec_context_free(ecl_exec_context ec)
{
    dr_free_exec_context((drisc_exec_ctx) ec);
}

extern void
ecl_assoc_client_data(ecl_exec_context ec, int key, long value)
{
    dr_assoc_client_data((drisc_exec_ctx) ec, key, value);
}

extern long
ecl_get_client_data(ecl_exec_context ec, int key)
{
    return dr_get_client_data((drisc_exec_ctx) ec, key);
}

#ifdef i_callii
#define dr_blt_code 0
#define dr_ble_code 1
#define dr_bgt_code 2
#define dr_bge_code 3
#define dr_beq_code 4
#define dr_bne_code 5

extern void
dr_pbr(drisc_ctx c, int op_type, int data_type, dr_reg_type src1,
       dr_reg_type src2, int label)
{
    int typ, index;
    if ((op_type < dr_blt_code ) || (op_type > dr_bne_code)) {
	printf("Bad op type in dr_pbr\n");
    }
    switch (data_type) {
    case DR_I: typ = 0; break;
    case DR_U: typ = 1; break;
    case DR_L: typ = 2; break;
    case DR_UL: typ = 3; break;
    case DR_P: typ = 4; break;
    case DR_D: typ = 6; break;
    case DR_F: typ = 5; break;
    default:
	printf("BAD  data type in dr_pbr\n");
	typ = 0;
    }
    index = 170 + typ + 7 * op_type;
    f_br(index,src1,src2,label);
}
#endif
static int
reverse_operator(int ecl_op)
{
    switch(ecl_op) {
    case op_neq:   return dr_beq_code;
    case op_eq:   return dr_bne_code;
    case op_gt:   return dr_ble_code;
    case op_geq:   return dr_blt_code;
    case op_lt:   return dr_bge_code;
    case op_leq:   return dr_bgt_code;
    default:
	assert(0);
    }
    return -1;
}

static void
cg_branch_if_false(drisc_ctx c, sm_ref pred, dr_label_type label)
{
    operator_t op;
    int string_op;
    int branch_operator;
    dr_reg_type right = 0, left = 0;

    if (!is_comparison_operator(pred)) {
	operand conditional;
	conditional = cg_expr(c, pred, 0);
	assert(conditional.is_addr == 0);
	gen_bz(c, conditional.reg, label, ecl_sm_get_type(pred));
	return;
    }

    op = pred->node.operator.op;
    string_op = ecl_expr_is_string(pred->node.operator.right);

    operator_prep(c, pred, &right, &left);

    if (string_op) {
	/* this has to be strcmp */
	dr_reg_type rv;
	dr_push_init(c);
	if (dr_do_reverse_vararg_push(c)) {
	    dr_push_argp(c, right);
	    dr_push_argp(c, left);
	} else {
	    dr_push_argp(c, left);
		dr_push_argp(c, right);
	}
#ifdef i_callii
	rv = dr_getvreg(c, DR_I);
	i_callii(rv, (void *)ecl_streq);
#else
	rv = dr_calli(c, (void *)ecl_streq);
#endif
	gen_bnz(c, rv, label, DR_I);
	return;
    }

    branch_operator = reverse_operator(op);
    dr_pbr(c, branch_operator, pred->node.operator.operation_type, 
	   left, right, label);
}
    
static void cg_selection_statement(drisc_ctx c, sm_ref stmt, ecl_code descr)
{
    dr_label_type else_label = dr_genlabel(c);
    
    cg_branch_if_false(c, stmt->node.selection_statement.conditional, 
		       else_label);
    cg_statement(c, stmt->node.selection_statement.then_part, descr);
    if (stmt->node.selection_statement.else_part != NULL) {
	dr_label_type end_label = dr_genlabel(c);
	dr_jpi(c, end_label);	/* op_i_jpi */
	dr_label(c, else_label);
	cg_statement(c, stmt->node.selection_statement.else_part, descr);
	dr_label(c, end_label);
    } else {
	dr_label(c, else_label);
    }
}

static void cg_iteration_statement(drisc_ctx c, sm_ref stmt, ecl_code descr)
{
    dr_label_type begin_label = dr_genlabel(c), end_label = dr_genlabel(c);
    if (stmt->node.iteration_statement.init_expr != NULL) {
	(void) cg_expr(c, stmt->node.iteration_statement.init_expr, 0);
    }
    dr_label(c, begin_label);
    if (stmt->node.iteration_statement.test_expr != NULL) {
	cg_branch_if_false(c, stmt->node.iteration_statement.test_expr, 
			   end_label);
    }
    cg_statement(c, stmt->node.iteration_statement.statement, descr);
    if (stmt->node.iteration_statement.iter_expr != NULL) {
	(void) cg_expr(c, stmt->node.iteration_statement.iter_expr, 0);
    }
    dr_jv(c, begin_label);	/* op_i_jpi */
    dr_label(c, end_label);
    
}
#endif
