%{
#include "config.h"
#ifdef SEGMENTED_POINTERS
int ecl_segmented_pointers = 1;
#else
int ecl_segmented_pointers = 0;
#endif
#ifdef KPLUGINS_INTEGRATION
int ecl_kplugins_integration = 1;
#else
int ecl_kplugins_integration = 0;
#endif
#ifndef LINUX_KERNEL_MODULE
#include "stdio.h"
#endif
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
#ifndef LINUX_KERNEL_MODULE
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif
#include "ecl.h"
#include "ecl_internal.h"
#include "assert.h"
#ifndef LINUX_KERNEL_MODULE
#include <ctype.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#else
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/mm.h>
#endif
#include "structs.h"
#ifdef HAVE_DRISC_H
#include "drisc.h"
#else
#ifdef HAVE_ICODE_H
#include "drisc_icode.h"
#include "icode.h"
#else
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
    DR_EC,   /* exec context */
    DR_ERR   /* no type */
};
#endif
#endif
#ifndef LINUX_KERNEL_MODULE
#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#else
#include "kecl.h"
#define malloc (void *)DAllocMM
#define free(a) DFreeMM((addrs_t)a)
#define realloc(a,b) DReallocMM((addrs_t)a,b)
#define fprintf(fmt, args...) printk(args);
#define printf printk
char *strdup(const char *s)
{
	char *p;

	p = (char *)kmalloc(strlen(s)+1, GFP_KERNEL);
	if (p != NULL)
		strcpy(p,s);
	return p;
}
#endif
#define yyparse ecl_yyparse
#define yylex ecl_yylex
#define yyrestart ecl_yyrestart
#define yywrap ecl_yywrap
#define yyerror ecl_yyerror
#define yy_create_buffer ecl_yy_create_buffer
#define yy_delete_buffer ecl_yy_delete_buffer
#define yy_flush_buffer ecl_yy_flush_buffer
#define yy_init_buffer ecl_yy_init_buffer
#define yy_load_buffer_state ecl_yy_load_buffer_state
#define yy_scan_buffer ecl_yy_scan_buffer
#define yy_scan_bytes ecl_yy_scan_bytes
#define yy_scan_string ecl_yy_scan_string
#define yy_switch_to_buffer ecl_yy_switch_to_buffer
#define yychar ecl_yychar
#define yyin ecl_yyin
#define yyleng ecl_yyleng
#define yylval ecl_yylval
#define yynerrs ecl_yynerrs
#define yyout ecl_yyout
#define yytext ecl_yytext

static char *create_string_from_yytext();
extern int yylex();
extern int yyparse();
static sm_ref yyparse_value;
static int yyerror_count = 1;
extern void yyerror(char *str);
static int parsing_type = 0;
static int parsing_param_spec = 0;
static ecl_parse_context yycontext;
static sm_ref ecl_build_parsed_type_node(ecl_parse_context c, char *name, sm_list l);
static void free_field_list(list)
IOFieldList list;
{
    int i = 0;
    while (list[i].field_name != NULL) {
	free((char*)list[i].field_name);
	free((char*)list[i].field_type);
	i++;
    }
    free(list);
}
static sm_list
ecl_dup_list(sm_list list)
{
    sm_list ret_list, new_list = NULL;
    sm_list *last_p = &ret_list;
    while (list != NULL) {
	*last_p = new_list = malloc(sizeof(struct list_struct));
	last_p = &(new_list->next);
	new_list->node = malloc(sizeof(*(list->node)));
	memcpy(new_list->node, list->node, sizeof(*(list->node)));
	list = list->next;
    }
    *last_p = NULL;
    return ret_list;
}
%}

%union {
    lx_info info;
    sm_ref reference;
    sm_list list;
    char *string;
};

%token <info> ARROW
%token <info> LPAREN
%token <info> RPAREN
%token <info> LCURLY
%token <info> RCURLY
%token <info> LBRACKET
%token <info> RBRACKET
%token <info> DOT
%token <info> STAR
%token <info> AT
%token <info> SLASH
%token <info> MODULUS
%token <info> PLUS
%token <info> MINUS
%token <info> LEQ
%token <info> LT
%token <info> GEQ
%token <info> GT
%token <info> EQ
%token <info> NEQ
%token <info> LEFT_SHIFT
%token <info> RIGHT_SHIFT
%token <info> ASSIGN
%token <info> LOG_OR
%token <info> LOG_AND
%token <info> ARITH_OR
%token <info> ARITH_AND
%token <info> ARITH_XOR
%token <info> INC_OP
%token <info> DEC_OP
%token <info> BANG
%token <info> SEMI
%token <info> IF
%token <info> ELSE
%token <info> FOR
%token <info> WHILE
%token <info> CHAR
%token <info> SHORT
%token <info> INT
%token <info> LONG
%token <info> UNSIGNED
%token <info> SIGNED
%token <info> FLOAT
%token <info> DOUBLE
%token <info> VOID
%token <info> STRING
%token <info> STATIC
%token <info> STRUCT
%token <info> UNION
%token <info> CONST
%token <info> SIZEOF
%token <info> TYPEDEF
%token <info> RETURN_TOKEN
%token <info> PRINT
%token <info> COMMA
%token <info> DOTDOTDOT
%token <info> integer_constant
%token <info> string_constant
%token <info> floating_constant
%token <info> identifier_ref
%token <info> type_id

%type <info> struct_or_union;
%type <reference> compound_statement;
%type <list> declaration_list statement_list;
%type <reference> declaration;
%type <reference> statement;
%type <reference> init_declarator;
%type <reference> declarator;
%type <reference> direct_declarator;
%type <list> declaration_specifiers;
%type <list> type_qualifier_list;
%type <list> specifier_qualifier_list;
%type <list> struct_declaration_list;
%type <list> struct_declaration;
%type <list> struct_declarator_list;
%type <reference> struct_declarator;
%type <list> pointer;
%type <list> abstract_declarator;
%type <list> type_name;
%type <list> parameter_list;
%type <list> parameter_type_list;
%type <list> argument_expression_list;
%type <reference> struct_or_union_specifier
%type <reference> parameter_declaration;
%type <reference> type_specifier;
%type <reference> type_qualifier;
%type <reference> storage_class_specifier;
%type <reference> initializer;
%type <reference> assignment_expression;
%type <reference> expression_statement;
%type <reference> selection_statement;
%type <reference> iteration_statement;
%type <reference> jump_statement;
%type <reference> conditional_expression;
%type <reference> constant_expression;
%type <reference> unary_expression;
%type <reference> cast_expression;
%type <reference> postfix_expression;
%type <reference> primary_expression;
%type <reference> expression;
%type <reference> expression_opt;
%type <reference> constant;
%type <reference> multiplicative_expression;
%type <reference> additive_expression;
%type <reference> shift_expression;
%type <reference> relational_expression;
%type <reference> equality_expression;
%type <reference> logical_or_expression;
%type <reference> logical_and_expression;
%type <reference> inclusive_or_expression;
%type <reference> exclusive_or_expression;
%type <reference> and_expression;
%type <reference> start
%%

start :
	declaration_list {
	    yyparse_value = (sm_ref)$1;
	}
	|
	compound_statement {
	    yyparse_value = $1;
	}
	;

primary_expression:
	identifier_ref {
	    $$ = ecl_new_identifier();
	    $$->node.identifier.id = $1.string;
	    $$->node.identifier.lx_srcpos = $1.lx_srcpos;
	}
	|
	constant
	|
	LPAREN expression RPAREN
	{ $$ = $2; }
	;

/* missing ->, ++, -- */
postfix_expression:
	primary_expression
	|
	postfix_expression LBRACKET expression RBRACKET {
	    $$ = ecl_new_element_ref();
	    $$->node.element_ref.lx_srcpos = $2.lx_srcpos;
	    $$->node.element_ref.expression = $3;
	    $$->node.element_ref.array_ref = $1;
	}
	|
	postfix_expression DOT identifier_ref {
	    $$ = ecl_new_field_ref();
	    $$->node.field_ref.lx_srcpos = $2.lx_srcpos;
	    $$->node.field_ref.lx_field = $3.string;
	    $$->node.field_ref.struct_ref = $1;
	}
	|
	postfix_expression LPAREN argument_expression_list RPAREN {
	    $$ = ecl_new_subroutine_call();
	    $$->node.subroutine_call.lx_srcpos = $2.lx_srcpos;
	    $$->node.subroutine_call.arguments = $3;
	    $$->node.subroutine_call.sm_func_ref = $1;
	}
	|
	postfix_expression LPAREN  RPAREN {
	    $$ = ecl_new_subroutine_call();
	    $$->node.subroutine_call.lx_srcpos = $2.lx_srcpos;
	    $$->node.subroutine_call.arguments = NULL;
	    $$->node.subroutine_call.sm_func_ref = $1;
	}
	| postfix_expression ARROW identifier_ref {
	    assert(0);
	}
	| postfix_expression INC_OP {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_inc;
	    $$->node.operator.right = NULL;
	    $$->node.operator.left = $1;
	}
        | postfix_expression DEC_OP {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_dec;
	    $$->node.operator.right = NULL;
	    $$->node.operator.left = $1;
	}
	;

argument_expression_list:
	assignment_expression {
		$$ = malloc(sizeof(struct list_struct));
		$$->node = $1;
		$$->next = NULL;
	}
	|
	argument_expression_list COMMA assignment_expression {
	    sm_list tmp = $1;
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = malloc(sizeof(struct list_struct));
	    tmp->next->node = $3;
	    tmp->next->next = NULL;
	    $$ = $1;
	};


/* missing ~, */
unary_expression:
	postfix_expression
        | INC_OP unary_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_inc;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
        | DEC_OP unary_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_dec;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| STAR cast_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_deref;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| BANG cast_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_log_neg;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| ARITH_AND cast_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_address;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| PLUS cast_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_plus;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| MINUS cast_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_minus;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| SIZEOF unary_expression {
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_sizeof;
	    $$->node.operator.right = $2;
	    $$->node.operator.left = NULL;
	}
	| SIZEOF LPAREN type_name RPAREN {
	    /* dummy up a cast to hold the sm_list of the type */
	    sm_ref cast = ecl_new_cast();
	    cast->node.cast.lx_srcpos = $1.lx_srcpos;
	    cast->node.cast.type_spec = $3;
	    cast->node.cast.expression = NULL;

	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $1.lx_srcpos;
	    $$->node.operator.op = op_sizeof;
	    $$->node.operator.right = cast;
	    $$->node.operator.left = NULL;
	};

cast_expression:
	unary_expression
	| LPAREN type_name RPAREN cast_expression {
	    $$ = ecl_new_cast();
	    $$->node.cast.lx_srcpos = $1.lx_srcpos;
	    $$->node.cast.type_spec = $2;
	    $$->node.cast.expression = $4;
	}
	;

multiplicative_expression:
	cast_expression
	|
	multiplicative_expression STAR cast_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_mult;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	multiplicative_expression SLASH cast_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_div;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	multiplicative_expression MODULUS cast_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_modulus;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

additive_expression:
	multiplicative_expression
	|
	additive_expression PLUS multiplicative_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.op = op_plus;
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	additive_expression MINUS multiplicative_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_minus;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

shift_expression:
	additive_expression
	|
	shift_expression LEFT_SHIFT additive_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_left_shift;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	shift_expression RIGHT_SHIFT additive_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_right_shift;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

relational_expression:
	shift_expression
	|
	relational_expression LT shift_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_lt;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	relational_expression GT shift_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_gt;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	relational_expression LEQ shift_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_leq;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	relational_expression GEQ shift_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_geq;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

equality_expression:
	relational_expression
	|
	equality_expression EQ relational_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_eq;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	|
	equality_expression NEQ relational_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_neq;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

and_expression:
	equality_expression
	|
	and_expression ARITH_AND equality_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_arith_and;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

exclusive_or_expression:
	and_expression
	|
	exclusive_or_expression ARITH_XOR and_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_arith_xor;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

inclusive_or_expression:
	exclusive_or_expression
	|
	inclusive_or_expression ARITH_OR exclusive_or_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_arith_or;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

logical_and_expression:
	inclusive_or_expression
	|
	logical_and_expression LOG_AND inclusive_or_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_log_and;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

logical_or_expression:
	logical_and_expression
	|
	logical_or_expression LOG_OR logical_and_expression
	{
	    $$ = ecl_new_operator();
	    $$->node.operator.lx_srcpos = $2.lx_srcpos;
	    $$->node.operator.op = op_log_or;
	    $$->node.operator.right = $3;
	    $$->node.operator.left = $1;
	}
	;

/* missing ?: construction */
conditional_expression:
	logical_or_expression
	;

/* missing other assignment operators */
assignment_expression:
	conditional_expression
	{ $$ = $1;} 
	|
	unary_expression ASSIGN assignment_expression
	{
	    $$ = ecl_new_assignment_expression();
	    $$->node.assignment_expression.lx_srcpos = $2.lx_srcpos;
	    $$->node.assignment_expression.left = $1;
	    $$->node.assignment_expression.right = $3;
	}
	;

/* missing expression, assignment_expression production */
expression:
	assignment_expression
	    {$$ = $1;}
	;

constant_expression
        : conditional_expression
        ;

declaration:
	declaration_specifiers 
	     { 
		 if (parsing_type) {
		     yyparse_value = (sm_ref) $1;
		     YYACCEPT;
		 }
	     }
	init_declarator 
	    {  /* stop here if we're just doing a proc decl */
		if (parsing_param_spec) {
		    $$ = $3;
		    if ($$->node_type == ecl_declaration) {
			if  ($$->node.declaration.type_spec == NULL) {
			    $$->node.declaration.type_spec = $1;
			} else {
			    /* 
			     * the pointer type list (with the declarator)
			     * goes at the end 
			     */
			    sm_list tmp = $1;
			    while (tmp->next != NULL) {
				tmp = tmp->next;
			    }
			    tmp->next = $$->node.declaration.type_spec;
			    $$->node.declaration.type_spec = $1;
			}
		    } else {
		        printf("unexpected node in init_declarator\n");
			ecl_print($$);
		    }
		    yyparse_value = $3;
		    YYACCEPT;
		}
	    }
	SEMI
	    {
		sm_list type_spec = $1;
		$$ = $3;
		if ($$->node_type == ecl_declaration) {
		    if  ($$->node.declaration.type_spec == NULL) {
			$$->node.declaration.type_spec = $1;
		    } else {
			/* 
			 * the pointer type list (with the declarator)
			 * goes at the end 
			 */
			sm_list tmp = $1;
			while (tmp->next != NULL) {
			    tmp = tmp->next;
			}
			tmp->next = $$->node.declaration.type_spec;
			$$->node.declaration.type_spec = $1;
		    }
		} else if ($$->node_type == ecl_array_type_decl) {
		    if  ($$->node.array_type_decl.type_spec == NULL) {
			$$->node.array_type_decl.type_spec = $1;
		    } else {
			/* 
			 * the pointer type list (with the declarator)
			 * goes at the end 
			 */
			sm_list tmp = $1;
			while (tmp->next != NULL) {
			    tmp = tmp->next;
			}
			tmp->next = $$->node.array_type_decl.type_spec;
			$$->node.array_type_decl.type_spec = $1;
		    }
		} else {
		    printf("Unknown decl entry\n");
		    ecl_print($$);
		}
		while (type_spec != NULL) {
		    if (type_spec->node->node.type_specifier.token == TYPEDEF) {
			ecl_add_defined_type($$->node.declaration.id, yycontext);
		    }
		    type_spec = type_spec->next;
		}

	    }
        | error SEMI {
	    $$ = NULL;
	}
	;

declaration_specifiers
	: storage_class_specifier {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	| storage_class_specifier declaration_specifiers {
	    sm_list tmp = malloc(sizeof(struct list_struct));
	    tmp->node = $1;
	    tmp->next = $2;
	    $$ = tmp;
	}
	| type_specifier {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	| type_specifier declaration_specifiers {
	    sm_list tmp = malloc(sizeof(struct list_struct));
	    tmp->node = $1;
	    tmp->next = $2;
	    $$ = tmp;
	}
	| type_qualifier {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	| type_qualifier declaration_specifiers {
	    sm_list tmp = malloc(sizeof(struct list_struct));
	    tmp->node = $1;
	    tmp->next = $2;
	    $$ = tmp;
	};

init_declarator:
	declarator
	|
	declarator ASSIGN initializer
	    {
		assert($1->node_type == ecl_declaration);
		$1->node.declaration.init_value = $3;
	    }
	;

storage_class_specifier
	: TYPEDEF {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = TYPEDEF;
	}
	| STATIC {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = STATIC;
	}
	;

type_specifier:
	CHAR {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = CHAR;
	}
	| SHORT {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = SHORT;
	}
	| INT {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = INT;
	}
	| LONG {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = LONG;
	}
	| FLOAT {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = FLOAT;
	}
	| DOUBLE {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = DOUBLE;
	}
	| VOID {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = VOID;
	}
	| SIGNED {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = SIGNED;
	}
	| UNSIGNED {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = UNSIGNED;
	}
	| STRING {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = STRING;
	}
	| type_id {
	    $$ = ecl_new_identifier();
	    $$->node.identifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.identifier.id = $1.string;
	}
	| struct_or_union_specifier {
	    $$ = $1;
	}
	;

struct_or_union_specifier
	: struct_or_union identifier_ref LCURLY struct_declaration_list RCURLY {
	    $$ = ecl_build_parsed_type_node(yycontext, $1.string, $4);
	}
	| struct_or_union LCURLY struct_declaration_list RCURLY {
	    $$ = ecl_build_parsed_type_node(yycontext, "anon", $3);
	}
	| struct_or_union identifier_ref {
	    $$ = ecl_build_parsed_type_node(yycontext, $1.string, NULL);
	}
	;

struct_or_union
	: STRUCT
	| UNION {
            yyerror("UNIONs not supported!");
	}
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration {
	    sm_list tmp = $1;
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next =$2;
	    $$ = $1;
	}
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list SEMI {
	    sm_list type_spec = $1;
	    sm_list decl_list = $2;
 	    $$ = $2;
/******** GSE This isn't right.  Reusing potentially modified type spec */
	    while (decl_list != NULL) {
		sm_ref decl = decl_list->node;
		if (decl->node_type == ecl_declaration) {
		    if  (decl->node.declaration.type_spec == NULL) {
			decl->node.declaration.type_spec = type_spec;
		    } else {
			/* 
			 * the pointer type list (with the declarator)
			 * goes at the end 
			 */
			sm_list tmp = $1;
			while (tmp->next != NULL) {
			    tmp = tmp->next;
			}
			tmp->next = decl->node.declaration.type_spec;
			decl->node.declaration.type_spec = type_spec;
		    }
		} else if (decl->node_type == ecl_array_type_decl) {
		    if  (decl->node.array_type_decl.type_spec == NULL) {
			decl->node.array_type_decl.type_spec = type_spec;
		    } else {
			/* 
			 * the pointer type list (with the declarator)
			 * goes at the end 
			 */
			sm_list tmp = type_spec;
			while (tmp->next != NULL) {
			    tmp = tmp->next;
			}
			tmp->next = decl->node.array_type_decl.type_spec;
			decl->node.array_type_decl.type_spec = type_spec;
		    }
		} else {
		    printf("Unknown decl entry\n");
		    ecl_print(decl);
		}
		decl_list = decl_list->next;
		if (decl_list != NULL) {
		    type_spec = ecl_dup_list(type_spec);
		}
	    }
	}
	;


struct_declarator_list
 	: struct_declarator {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	| struct_declarator_list COMMA struct_declarator {
	    sm_list tmp = $1;
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = malloc(sizeof(struct list_struct));
	    tmp->next->node = $3;
	    tmp->next->next = NULL;
	    $$ = $1;
	}
	;

struct_declarator : declarator ;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list {
	    sm_list tmp = malloc(sizeof(struct list_struct));
	    tmp->node = $1;
	    tmp->next = $2;
	    $$ = tmp;
	}
	| type_specifier {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	| type_qualifier specifier_qualifier_list {
	    sm_list tmp = malloc(sizeof(struct list_struct));
	    tmp->node = $1;
	    tmp->next = $2;
	    $$ = tmp;
	}
	| type_qualifier {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	;

type_qualifier
	: CONST {
	    $$ = ecl_new_type_specifier();
	    $$->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    $$->node.type_specifier.token = CONST;
	}
	;

declarator:
	direct_declarator
	|
	pointer direct_declarator {
	    $$ = $2;
	    if ($$->node_type == ecl_declaration) {
		$$->node.declaration.type_spec = $1;
	    } else if ($$->node_type == ecl_array_type_decl) {
		$$->node.array_type_decl.type_spec = $1;
	    } else {
		printf("Unknown direct_declarator entry\n");
		ecl_print($$);
	    }
	}
	;

direct_declarator:
	identifier_ref	    {
		$$ = ecl_new_declaration();
		$$->node.declaration.param_num = -1;
		$$->node.declaration.id = $1.string;
		$$->node.declaration.init_value = NULL;
		$$->node.declaration.lx_srcpos = $1.lx_srcpos;
		$$->node.declaration.is_subroutine = 0;
		$$->node.declaration.params = NULL;
	    }
	| LPAREN declarator RPAREN {
	    $$ = $2;
	}	
	| identifier_ref LPAREN parameter_type_list RPAREN {
		$$ = ecl_new_declaration();
		$$->node.declaration.param_num = -1;
		$$->node.declaration.id = $1.string;
		$$->node.declaration.init_value = NULL;
		$$->node.declaration.lx_srcpos = $1.lx_srcpos;
		$$->node.declaration.is_subroutine = 1;
		$$->node.declaration.params = $3;
	}
	| identifier_ref LPAREN RPAREN {
		$$ = ecl_new_declaration();
		$$->node.declaration.param_num = -1;
		$$->node.declaration.id = $1.string;
		$$->node.declaration.init_value = NULL;
		$$->node.declaration.lx_srcpos = $1.lx_srcpos;
		$$->node.declaration.is_subroutine = 1;
		$$->node.declaration.params = NULL;
	}
	| direct_declarator LBRACKET constant_expression RBRACKET {
		$$ = ecl_new_array_type_decl();
		$$->node.array_type_decl.lx_srcpos = $2.lx_srcpos;
		$$->node.array_type_decl.size_expr = $3;
		$$->node.array_type_decl.element_ref = $1;
		$$->node.array_type_decl.sm_dynamic_size = NULL;
	}
	| direct_declarator LBRACKET RBRACKET {
		$$ = ecl_new_array_type_decl();
		$$->node.array_type_decl.lx_srcpos = $2.lx_srcpos;
		$$->node.array_type_decl.size_expr = NULL;
		$$->node.array_type_decl.element_ref = $1;
		$$->node.array_type_decl.sm_dynamic_size = NULL;
	}
	;

pointer :
	STAR {
	    sm_ref star = ecl_new_type_specifier();
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = STAR;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = NULL;
	}
	| STAR type_qualifier_list {
	    sm_ref star = ecl_new_type_specifier();
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = STAR;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = $2;
	}
	| STAR pointer {
	    sm_ref star = ecl_new_type_specifier();
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = STAR;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = $2;
	}
	| STAR type_qualifier_list pointer {
	    sm_list tmp = $2;
	    sm_ref star = ecl_new_type_specifier();
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = STAR;

	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = $3;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = $2;
	}
	| AT {
	    sm_ref star = ecl_new_type_specifier();
	    if(!ecl_segmented_pointers) { 
                yyerror("Segmented pointers disabled!");
	    }
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = AT;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = NULL;
	}
	| AT type_qualifier_list {
	    sm_ref star = ecl_new_type_specifier();
	    if(!ecl_segmented_pointers) {
                yyerror("Segmented pointers disabled!");
	    }
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = AT;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = $2;
	}
	| AT pointer {
	    sm_ref star = ecl_new_type_specifier();
	    if(!ecl_segmented_pointers) {
                yyerror("Segmented pointers disabled!");
	    }
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = AT;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = $2;
	}
	| AT type_qualifier_list pointer {
	    sm_list tmp = $2;
	    sm_ref star = ecl_new_type_specifier();
	    if(!ecl_segmented_pointers) {
                yyerror("Segmented pointers disabled!");
	    }
	    star->node.type_specifier.lx_srcpos = $1.lx_srcpos;
	    star->node.type_specifier.token = AT;

	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = $3;
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = star;
	    $$->next = $2;
	}
	;

type_qualifier_list
	: type_qualifier {
	    $$ = malloc(sizeof(struct list_struct));
	    $$->node = $1;
	    $$->next = NULL;
	}
	| type_qualifier_list type_qualifier {
	    sm_list tmp = $1;
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = malloc(sizeof(struct list_struct));
	    tmp->next->node = $2;
	    tmp->next->next = NULL;
	    $$ = $1;
	}
	;

parameter_type_list:
	parameter_list |
	parameter_list COMMA DOTDOTDOT {
	    sm_list tmp = $1;
	    sm_ref id = ecl_new_declaration();
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = malloc(sizeof(struct list_struct));
	    tmp->next->node = id;
	    tmp->next->next = NULL;
	    id->node.declaration.id = strdup("...");
	    $$ = $1;
	}
	;

parameter_list:
	parameter_declaration {
		$$ = malloc(sizeof(struct list_struct));
		$$->node = $1;
		$$->next = NULL;
	}
	|
	parameter_list COMMA parameter_declaration {
	    sm_list tmp = $1;
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = malloc(sizeof(struct list_struct));
	    tmp->next->node = $3;
	    tmp->next->next = NULL;
	    $$ = $1;
	};
	
/* missing
 	| declaration_specifiers abstract_declarator
	| declaration_specifiers
*/
parameter_declaration:
	declaration_specifiers declarator {
		$$ = $2;
		if ($$->node_type == ecl_declaration) {
		    $$->node.declaration.static_var = 0;
		    if  ($$->node.declaration.type_spec == NULL) {
		        $$->node.declaration.type_spec = $1;
		    } else {
		        /* 
			 * the pointer type list (with the declarator)
			 * goes at the end 
			 */
		      sm_list tmp = $1;
		      while (tmp->next != NULL) {
			  tmp = tmp->next;
		      }
		      tmp->next = $$->node.declaration.type_spec;
		      $$->node.declaration.type_spec = $1;
		    }
		} else if ($$->node_type == ecl_array_type_decl) {
		    if  ($$->node.array_type_decl.type_spec == NULL) {
		        $$->node.array_type_decl.type_spec = $1;
		    } else {
		        /* 
			 * the pointer type list (with the declarator)
			 * goes at the end 
			 */
		      sm_list tmp = $1;
		      while (tmp->next != NULL) {
			  tmp = tmp->next;
		      }
		      tmp->next = $$->node.array_type_decl.type_spec;
		      $$->node.array_type_decl.type_spec = $1;
		    }
		} else {
		    printf("unexpected node in parameter_declaration");
		}
	};

type_name: 
	specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator {
	    sm_list tmp = $1;
	    while (tmp->next != NULL) {
		tmp = tmp->next;
	    }
	    tmp->next = $2;
	    $$ = $1;
	}
	;

abstract_declarator:
	pointer
	;

initializer:
	assignment_expression { $$ = $1;}
	;

statement_list:
	/* NULL */ {$$ = NULL;}
	| statement_list statement {
	    if ($2 == NULL) {
		$$ = $1;
	    } else {
		if ($1 == NULL) {
		    $$ = malloc(sizeof(struct list_struct));
		    $$->node = $2;
		    $$->next = NULL;
		} else {
		    sm_list tmp = $1;
		    while (tmp->next != NULL) {
			tmp = tmp->next;
		    }
		    tmp->next = malloc(sizeof(struct list_struct));
		    tmp->next->node = $2;
		    tmp->next->next = NULL;
		    $$ = $1;
		}
	    }
	};

/* missing labeled statement */
statement:
	  compound_statement
	| expression_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	| error SEMI {
	      $$ = NULL;
	  }
	;

compound_statement:
	LCURLY declaration_list statement_list RCURLY {
	    $$ = ecl_new_compound_statement();
	    $$->node.compound_statement.decls = $2;
	    $$->node.compound_statement.statements = $3;
	};

declaration_list:
	/* empty */ { $$ = NULL; }
	|
	declaration_list declaration {
	    if ($2 == NULL) {
		$$ = $1;
	    } else {
		if ($1 == NULL) {
		    $$ = malloc(sizeof(struct list_struct));
		    $$->node = $2;
		    $$->next = NULL;
		} else {
		    sm_list tmp = $1;
		    while (tmp->next != NULL) {
			tmp = tmp->next;
		    }
		    tmp->next = malloc(sizeof(struct list_struct));
		    tmp->next->node = $2;
		    tmp->next->next = NULL;
		    $$ = $1;
		}
	    }
	};

/* Missing goto, continue, break */
jump_statement:
	RETURN_TOKEN expression_opt SEMI {
	    $$ = ecl_new_return_statement();
	    $$->node.return_statement.expression = $2;
	    $$->node.return_statement.lx_srcpos = $1.lx_srcpos;
	}
	;

expression_statement:
	expression SEMI	{ 
	    $$ = ecl_new_expression_statement();
	    $$->node.expression_statement.expression = $1;
	}
	;

/* missing switch */
selection_statement:
	IF LPAREN expression RPAREN statement
	{ 
	    $$ = ecl_new_selection_statement();
	    $$->node.selection_statement.lx_srcpos = $1.lx_srcpos;
	    $$->node.selection_statement.conditional = $3;
	    $$->node.selection_statement.then_part = $5;
	    $$->node.selection_statement.else_part = NULL;
	}
	| 
	IF LPAREN expression RPAREN statement ELSE statement
	{ 
	    $$ = ecl_new_selection_statement();
	    $$->node.selection_statement.lx_srcpos = $1.lx_srcpos;
	    $$->node.selection_statement.conditional = $3;
	    $$->node.selection_statement.then_part = $5;
	    $$->node.selection_statement.else_part = $7;
	}
	;

/* missing while, do while */
iteration_statement:
	FOR LPAREN expression_opt SEMI expression_opt SEMI expression_opt RPAREN statement
	{ 
	    $$ = ecl_new_iteration_statement();
	    $$->node.selection_statement.lx_srcpos = $1.lx_srcpos;
	    $$->node.iteration_statement.init_expr = $3;
	    $$->node.iteration_statement.test_expr = $5;
	    $$->node.iteration_statement.iter_expr = $7;
	    $$->node.iteration_statement.statement = $9;
	} 
	|
	WHILE LPAREN expression RPAREN statement
	{ 
	    $$ = ecl_new_iteration_statement();
	    $$->node.selection_statement.lx_srcpos = $1.lx_srcpos;
	    $$->node.iteration_statement.init_expr = NULL;
	    $$->node.iteration_statement.test_expr = $3;
	    $$->node.iteration_statement.iter_expr = NULL;
	    $$->node.iteration_statement.statement = $5;
	} 

	;

expression_opt:
	{ $$ = NULL; }
	/* null */
	| expression;

constant :
	integer_constant {
	    $$ = ecl_new_constant();
	    $$->node.constant.token = integer_constant;
	    $$->node.constant.const_val = $1.string;
	    $$->node.constant.lx_srcpos = $1.lx_srcpos;
	}
	|
	floating_constant {
	    $$ = ecl_new_constant();
	    $$->node.constant.token = floating_constant;
	    $$->node.constant.const_val = $1.string;
	    $$->node.constant.lx_srcpos = $1.lx_srcpos;
	}
	|
	string_constant {
	    $$ = ecl_new_constant();
	    $$->node.constant.token = string_constant;
	    $$->node.constant.const_val = $1.string;
	    $$->node.constant.lx_srcpos = $1.lx_srcpos;
	}
	;

%%
#include "lex.yy.c"

typedef struct scope *scope_ptr;

struct parse_struct {
    sm_list decls;
    sm_list standard_decls;
    scope_ptr scope;
    int defined_type_count;
    char **defined_types;
    err_out_func_t error_func;
    void *client_data;
    sm_list return_type_list;
    int return_cg_type;
    int has_exec_context;
};

static int
semanticize_compound_statement(ecl_parse_context context, sm_ref compound, 
			       scope_ptr containing_scope);
static int semanticize_decls_list(ecl_parse_context context, sm_list decls, 
				  scope_ptr scope);
static int semanticize_array_type_node(ecl_parse_context context,
				       sm_ref array, scope_ptr scope);
static int semanticize_reference_type_node(ecl_parse_context context,
					   sm_ref decl, scope_ptr scope);
static void add_decl(char *id, sm_ref node, scope_ptr scope);
static sm_ref find_complex_type(sm_ref node, scope_ptr scope);
static const char *ecl_code_string;
static int is_string(sm_ref expr);


int
ecl_semanticize_added_decls(ecl_parse_context context)
{
    return semanticize_decls_list(context, context->decls, context->scope);
}

extern void
ecl_swap_decls_to_standard(ecl_parse_context context)
{
    context->standard_decls = context->decls;
    context->decls = NULL;
}

int
ecl_parse_for_context(code, context)
char *code;
ecl_parse_context context;
{
    sm_list decls;
    int ret;
#if defined(YYDEBUG) && (YYDEBUG != 0)
    extern int yydebug;
    yydebug = 1;
#endif
    if (code != NULL) {
	setup_for_string_parse(code, context->defined_type_count, context->defined_types);
	ecl_code_string = code;
    }
    yyerror_count = 0;
    yycontext = context;
    yyparse();
    terminate_string_parse();

    if ((yyparse_value == NULL) || (yyerror_count != 0)) {
	return 0;
    }

    decls = (sm_list) yyparse_value;
    if (context->decls) {
	sm_list last = context->decls;
	while (last->next != NULL)
	    last = last->next;
	last->next = decls;
    } else {
	context->decls = decls;
    }
    ret = semanticize_decls_list(context, decls, context->scope);
    if (ret == 0) {
	ecl_rfree_list(decls, NULL);
	context->decls = NULL;
    }
    return ret;
}

ecl_code
ecl_code_gen(code, context)
char *code;
ecl_parse_context context;
{
    sm_ref tmp, tmp2;
    ecl_code ret_code;
    unsigned int offset;
    void *func;

    if (code != NULL) {
	setup_for_string_parse(code, context->defined_type_count, context->defined_types);
	ecl_code_string = code;
    }

    yyerror_count = 0;
    yycontext = context;
    yyparse();
    terminate_string_parse();

    if ((yyparse_value == NULL) || (yyerror_count != 0)) {
	return 0;
    }
    tmp = ecl_new_compound_statement();
    tmp->node.compound_statement.decls = context->decls;
    tmp->node.compound_statement.statements =
	malloc(sizeof(struct list_struct));
    tmp->node.compound_statement.statements->next = NULL;
    tmp->node.compound_statement.statements->node = yyparse_value;
    tmp2 = ecl_new_compound_statement();
    tmp2->node.compound_statement.decls = context->standard_decls;
    tmp2->node.compound_statement.statements =
	malloc(sizeof(struct list_struct));
    tmp2->node.compound_statement.statements->next = NULL;
    tmp2->node.compound_statement.statements->node = tmp;
    if (!semanticize_compound_statement(context, tmp, context->scope)) {
	tmp->node.compound_statement.decls = NULL;
	tmp2->node.compound_statement.decls = NULL;
	ecl_rfree(tmp2);
	return NULL;
    }
    ret_code = malloc(sizeof(struct _ecl_code_struct));
    ret_code->code_memory_block = NULL;
    func = ecl_cg_net(tmp, context->return_cg_type, &offset, ret_code);
    tmp->node.compound_statement.decls = NULL;
    tmp2->node.compound_statement.decls = NULL;
    ecl_rfree(tmp2);
    ret_code->func = (void(*)())(long)func;
    ret_code->data = NULL;
    return ret_code;
}

void 
ecl_dump(ecl_code code)
{
    printf("ECL CODE structure %lx - \n", (long) code);
    printf("  function pointer %lx, code memory block %lx, data %lx, static size %d\n",
	   (long) code->func, (long) code->code_memory_block,
	   (long) code->data, code->static_size_required);
#ifndef HAVE_ICODE_H
#ifdef HAVE_DRISC_H
    dr_dump((drisc_ctx) code->drisc_context);
#endif
#else
    v_dump((void*)(long)code->func);
#endif
}
    

int
ecl_code_verify(code, context)
char *code;
ecl_parse_context context;
{
    sm_ref tmp;

    if (code != NULL) {
	setup_for_string_parse(code, context->defined_type_count, context->defined_types);
	ecl_code_string = code;
    }

    yyerror_count = 0;
    yycontext = context;
    yyparse();
    terminate_string_parse();

    if ((yyparse_value == NULL) || (yyerror_count != 0)) {
	return 0;
    }

    tmp = ecl_new_compound_statement();
    tmp->node.compound_statement.decls = context->decls;
    tmp->node.compound_statement.statements =
	malloc(sizeof(struct list_struct));
    tmp->node.compound_statement.statements->next = NULL;
    tmp->node.compound_statement.statements->node = yyparse_value;
    if (semanticize_compound_statement(context, tmp, context->scope) == 0) {
	tmp->node.compound_statement.decls = NULL;
	ecl_rfree(tmp);
	return 0;
    }
    tmp->node.compound_statement.decls = NULL;
    ecl_rfree(tmp);
    return 1;
}

extern void 
ecl_code_free(code)
ecl_code code;
{
    if (code->code_memory_block) free(code->code_memory_block);
    if (code->data) free(code->data);
#if !defined(HAVE_ICODE_H) && defined(HAVE_DRISC_H)
    if (code->drisc_context) {
	dr_free_context((drisc_ctx) code->drisc_context);
    }
#endif
    free(code);
}

static char *
copy_line(const char *line_begin)
{
    const char *line_end;
    if ((line_end = strchr(line_begin, 10)) == NULL) {
	/* no CR */
	return strdup(line_begin);
    } else {
	char *tmp = malloc(line_end - line_begin + 1);
	strncpy(tmp, line_begin, line_end - line_begin);
	tmp[line_end - line_begin] = 0;
	return tmp;
    }
}

static void
default_error_out(void *client_data, char *string)
{
    fprintf(stderr, "%s", string);
}

static void
print_context(ecl_parse_context context, int line, int character)
{
    const char *tmp = ecl_code_string;
    const char *line_begin = ecl_code_string;
    char *line_copy = NULL;
    int i, line_len, offset = 0;

    while (line > 1) {
	switch(*tmp) {
	case 10:
	    line_begin = tmp + 1;
	    line--;
	    break;
	case 0:
	    line = 1;   /* end of src */
	    break;
	}
	tmp++;
    }
    if (character > 40) {
	offset = character - 40;
    }
    line_copy = copy_line(line_begin + offset);
    line_len = strlen(line_copy);
    if (line_len > 60) {
	line_copy[60] = 0;
    }
    context->error_func(context->client_data, line_copy);
    context->error_func(context->client_data, "\n");
    free(line_copy);
    for(i=offset + 1; i< character; i++) {
	if (line_begin[i-1] == '\t') {
	    context->error_func(context->client_data, "\t");
	} else {
	    context->error_func(context->client_data, " ");
	}
    }
    context->error_func(context->client_data, "^\n");
}

void yyerror(str)
char *str;
{
    char tmp_str[100];
    yycontext->error_func(yycontext->client_data, "## Error ");
    yycontext->error_func(yycontext->client_data, str);
    yycontext->error_func(yycontext->client_data, "\n");
    yycontext->error_func(yycontext->client_data, "## While parsing near ");
    yycontext->error_func(yycontext->client_data, yytext);
    sprintf(tmp_str, ", offset = %d, line = %d ####\n",lex_offset,line_count);
    yycontext->error_func(yycontext->client_data, tmp_str);
    print_context(yycontext, line_count, lex_offset);
    yyerror_count++;
}

#ifdef STDC_HEADERS
static void
ecl_src_error(ecl_parse_context context, sm_ref expr, char *format, ...)
#else
static void
ecl_src_error(context, expr, format, va_alist)
ecl_parse_context context;
sm_ref expr;
char *format;
va_dcl
#endif
{

    va_list ap;
    char *tmp = malloc(10240); /* arbitrarily large */
    srcpos lx_srcpos = ecl_get_srcpos(expr);
#ifdef STDC_HEADERS
    va_start(ap, format);
#else
    va_start(ap);
#endif
    context->error_func(context->client_data, "## Ecode Error:  ");
    vsprintf(tmp, format, ap);
    context->error_func(context->client_data, tmp);
    sprintf(tmp, " at line %d, char %d\n", lx_srcpos.line, lx_srcpos.character);
    context->error_func(context->client_data, tmp);
    free(tmp);
    print_context(context, lx_srcpos.line, lx_srcpos.character);
}

extern void
ecl_print_dimen_p(dimen_p d)
{
    int i;
    for (i=0; i < d->dimen_count; i++) {
	if (d->dimens[i].static_size != -1) {
	    printf("[%d]", d->dimens[i].static_size);
	} else {
	    sm_ref field = d->dimens[i].control_field;
	    printf("[%s]", field->node.field.name);
	}
    }
    printf("\n");
}

extern void
ecl_print_operator_t(operator_t o)
{
    switch (o) {
    case  op_modulus:
	printf("MODULUS");
	break;
    case  op_plus:
	printf("PLUS");
	break;
    case  op_minus:
	printf("MINUS");
	break;
    case  op_leq:
	printf("LEQ");
	break;
    case  op_lt:
	printf("LESS THAN");
	break;
    case  op_geq:
	printf("GEQ");
	break;
    case  op_gt:
	printf("GREATER THAN");
	break;
    case  op_eq:
	printf("EQUAL");
	break;
    case  op_neq:
	printf("NOT EQUAL");
	break;
    case  op_log_or:
	printf("LOGICAL OR");
	break;
    case  op_log_and:
	printf("LOGICAL AND");
	break;
    case op_log_neg:
	printf("LOGICAL NEGATION");
	break;
    case op_arith_and:
	printf("ARITH AND");
	break;
    case op_arith_or:
	printf("ARITH OR");
	break;
    case op_arith_xor:
	printf("ARITH XOR");
	break;
    case op_left_shift:
	printf("LEFT SHIFT");
	break;
    case op_right_shift:
	printf("RIGHT SHIFT");
	break;
    case  op_mult:
	printf("MULTIPLY");
	break;
    case  op_div:
	printf("DIVISION");
	break;
    case  op_deref:
	printf("DEREFERENCE");
	break;
    case  op_inc:
	printf("INCREMENT");
	break;
    case  op_dec:
	printf("DECREMENT");
	break;
    case  op_address:
	printf("ADDRESS");
	break;
    }
}

extern void
ecl_print_srcpos(srcpos pos)
{
    printf("line %d, char %d", pos.line, pos.character);
}

typedef struct st_entry {
    char *id;
    sm_ref node;
    struct st_entry *next;
} *st_entry;

struct scope {
    ecl_extern_list externs;
    struct st_entry *entry_list;
    struct scope *containing_scope;
};


extern ecl_parse_context
ecl_copy_context(context)
ecl_parse_context context;
{
    int i;
    ecl_parse_context new_context = new_ecl_parse_context();
    new_context->has_exec_context = context->has_exec_context;
    new_context->decls = ecl_copy_list(context->decls);
    new_context->scope->externs = context->scope->externs;
    new_context->error_func = context->error_func;
    new_context->client_data = context->client_data;
    semanticize_decls_list(new_context, new_context->decls, 
			   new_context->scope);
    new_context->defined_types = malloc(sizeof(char*) *
					context->defined_type_count);
    for (i=0; i< context->defined_type_count; i++) {
	new_context->defined_types[i] = context->defined_types[i];
    }
    return new_context;
}

static void *
resolve_extern(char *id, scope_ptr scope)
{
    if (scope == NULL) return NULL;
    if (scope->externs != NULL) {
	ecl_extern_list externs = scope->externs;
	while(externs->extern_name != NULL) {
	    if (strcmp(id, externs->extern_name) == 0) {
		return externs->extern_value;
	    }
	    externs++;
	}
    }
    return resolve_extern(id, scope->containing_scope);
}

static scope_ptr
push_scope(scope_ptr containing_scope)
{
    scope_ptr new_scope = malloc(sizeof(*new_scope));
    new_scope->externs = NULL;
    new_scope->entry_list = NULL;
    new_scope->containing_scope = containing_scope;
    return new_scope;
}

static void
pop_scope(scope_ptr scope)
{
    st_entry list = scope->entry_list;
    while (list != NULL) {
	st_entry tmp = list->next;
	free(list);
	list = tmp;
    }
    free(scope);
}

extern void
dump_scope(scope_ptr scope)
{
    printf("Containing_scope is %lx\n", scope->containing_scope);
    printf("Extern list:");
    if (scope->externs != NULL) {
	int i = 0;
	while (scope->externs[i].extern_name != NULL) {
	    printf("\t\"%s\" -> 0x%lx\n", scope->externs[i].extern_name,
		   scope->externs[i].extern_value);
	    i++;
	}
    }
    printf("Symbol list:");
    if (scope->entry_list != NULL) {
	st_entry e = scope->entry_list;
	while (e != NULL) {
	    printf("\t\"%s\" -> 0x%lx\n", e->id, e->node);
	    ecl_print(e->node);
	    e = e->next;
	}
    }
}	
static void
add_decl(char *id, sm_ref node, scope_ptr scope)
{
    st_entry entry = malloc(sizeof(*entry));
    entry->node = node;
    entry->id = id;
    entry->next = scope->entry_list;
    scope->entry_list = entry;
}

extern void
ecl_add_decl_to_scope(char *id, sm_ref node, ecl_parse_context context)
{
    add_decl(id, node, context->scope);
}

static sm_ref
resolve_local(char *id, scope_ptr scope)
{
    st_entry list = scope->entry_list;
    while(list != NULL) {
	if (strcmp(list->id, id) == 0) {
	    return list->node;
	}
	list = list->next;
    }
    return NULL;
}

static sm_ref
resolve(char *id, scope_ptr scope)
{
    sm_ref tmp;
    if (scope == NULL) return NULL;
    if ((tmp = resolve_local(id, scope)) != NULL) {
	return tmp;
    }
    return resolve(id, scope->containing_scope);
}

static char*
gen_anon()
{
    static int anon_count = 0;
    char *ret = malloc(strlen("Anonymous-xxxxx"));
    sprintf(ret, "Anonymous-%d", anon_count++);
    return ret;
}

static int
determine_unary_type(ecl_parse_context context, sm_ref expr, sm_ref right)
{
    int right_type = ecl_sm_get_type(right);
    operator_t op = expr->node.operator.op;
    switch(right_type) {
    case DR_C: case DR_UC: case DR_S: case DR_US:
	right_type = DR_I;   /* integer promotion */
    }
    if (op == op_minus) {
	switch(right_type) {
	case DR_U:
	    return DR_I;
	case DR_UL:
	    return DR_L;
	}
    }
    return right_type;
}

static int 
determine_op_type(ecl_parse_context context, sm_ref expr, 
		  sm_ref left, sm_ref right)
{
    int unsigned_used = 0;
    int left_type  = ecl_sm_get_type(left);
    int right_type = ecl_sm_get_type(right);
    operator_t op = expr->node.operator.op;

    if (left_type == DR_P) {
	if (is_string(expr->node.operator.left) &&
	    is_string(expr->node.operator.right) &&
	    (op == op_eq)) {
	    return DR_I;
	}

	if(get_complex_type(context, left)->node_type == ecl_struct_type_decl) {
	    ecl_src_error(context, expr, 
			  "Illegal arithmetic. Left side is a structure.");
	    return DR_ERR;
	}

	switch(right_type) {
	case DR_P:
	case DR_C:
	case DR_UC:
	case DR_S:
	case DR_US:
	case DR_I:
	case DR_U:
	case DR_L:
	case DR_UL:
	    return DR_P;
	    
	default:
	    ecl_src_error(context, expr, 
			  "Illegal pointer arithmetic. Right side is of incompatible type.");
	    return DR_ERR;
	}
    }
    if (right_type == DR_P) {

	if(get_complex_type(context, right)->node_type == ecl_struct_type_decl) {
	    ecl_src_error(context, expr, 
			  "Illegal arithmetic. Right side is a structure.");
	    return DR_ERR;
	}

	switch(left_type) {
	case DR_P:
	case DR_C:
	case DR_UC:
	case DR_S:
	case DR_US:
	case DR_I:
	case DR_U:
	case DR_L:
	case DR_UL:
	    return DR_P;
	    
	default:
	    ecl_src_error(context, expr, 
			  "Illegal pointer arithmetic. Left side is of incompatible type.");
	    return DR_ERR;
	}	
	
    }
    if (left_type == DR_B) {
	ecl_src_error(context, expr, 
		      "Illegal arithmetic.  Left side is a structured type");
	return DR_ERR;
    }
    if (right_type == DR_B) {
	ecl_src_error(context, expr, 
		      "Illegal arithmetic.  Right side is a structured type");
	return DR_ERR;
    }
    if ((left_type == DR_D) || (right_type == DR_D)) {
	if ((op == op_modulus) || (op == op_log_or) || (op == op_log_and)) {
	    ecl_src_error(context, expr, "Operands must be integral.");
	    return DR_ERR;
	} else {
	    return DR_D;
	}
    }
    if ((left_type == DR_F) || (right_type == DR_F)) {
	if ((op == op_modulus) || (op == op_log_or) || (op == op_log_and)) {
	    ecl_src_error(context, expr, "Operands must be integral.");
	    return DR_ERR;

	} else {
	    return DR_F;
	}
    }
    switch(left_type) {
    case DR_C: case DR_UC: case DR_S: case DR_US:
	left_type = DR_I;   /* integer promotion */
    }
    switch(right_type) {
    case DR_C: case DR_UC: case DR_S: case DR_US:
	right_type = DR_I;   /* integer promotion */
    }
    switch(left_type) {
    case DR_UC: case DR_US: case DR_U: case DR_UL:
	unsigned_used++;
    }
    switch(right_type) {
    case DR_UC: case DR_US: case DR_U: case DR_UL:
	unsigned_used++;
    }
    if ((op == op_left_shift) || (op == op_right_shift)) return left_type;
    if ((left_type == DR_UL) || (right_type == DR_UL)) return DR_UL;
    if ((left_type == DR_L) || (right_type == DR_L)) {
	/* GSE -bug  This test should be for *generated* target, not host */
	if (sizeof(long) > sizeof(unsigned int)) {
	    /* Long can represent all values of unsigned int */
	    return DR_L;
	} else {
	    return unsigned_used? DR_UL : DR_L;
	}
    }
    if ((left_type == DR_U) || (right_type == DR_U)) return DR_U;
    return unsigned_used? DR_U: DR_I;
}

static sm_ref reduce_type_list(ecl_parse_context context, sm_list type_list, 
			       int *cg_type, scope_ptr scope, int *is_typedef);
static int 
assignment_types_match(ecl_parse_context context, sm_ref left, sm_ref right);

static int
is_n_dimen_array(int dimen, sm_ref expr)
{
    if ((dimen == 0) && (expr == NULL)) return 1;
    if ((dimen > 0) && (expr == NULL)) return 0;
    if (dimen == 0) {
	if (expr->node_type == ecl_array_type_decl) {
	    return 0;
	} else {
	    return 1;
	}
    }
    if (expr->node_type == ecl_field_ref) {
	return is_n_dimen_array(dimen, expr->node.field_ref.sm_field_ref);
    }
    if (expr->node_type == ecl_element_ref) {
	return is_n_dimen_array(dimen + 1, expr);
    }
    if (expr->node_type != ecl_field) return 0;
    if (expr->node_type == ecl_field) {
	return is_n_dimen_array(dimen, expr->node.field.sm_complex_type);
    }
    /* ought to recurse or handle above */
    assert(0);
    return 0;
}

static int 
is_string(sm_ref expr)
{
    if (expr->node_type == ecl_field) {
	return (strcmp(expr->node.field.string_type, "string") == 0);
    } else if (expr->node_type == ecl_field_ref) {
	return is_string(expr->node.field_ref.sm_field_ref);
    } else if (expr->node_type == ecl_identifier) {
	return is_string(expr->node.identifier.sm_declaration);
    } else if (expr->node_type == ecl_declaration) {
	if (expr->node.declaration.cg_type != DR_P) return 0;
	if (expr->node.declaration.sm_complex_type != NULL) return 0;
	/* only strings have pointers without complex types */
	return 1;
    } else if (expr->node_type == ecl_constant) {
	return (expr->node.constant.token == string_constant);
    }
    return 0;
}

extern int 
ecl_expr_is_string(sm_ref expr)
{
    return is_string(expr);
}

extern int 
is_control_value(sm_ref expr, sm_ref strct)
{
    sm_list fields;
    if (expr->node_type == ecl_field_ref) {
	return is_control_value(expr->node.field_ref.sm_field_ref,
				expr->node.field_ref.struct_ref);
    }
    if (expr->node_type != ecl_field) return 0;
    assert(strct != NULL);
    strct = get_complex_type(0, strct);	
    if (strct->node_type == ecl_reference_type_decl) {
	strct = strct->node.reference_type_decl.sm_complex_referenced_type;
    }
    if (strct->node_type == ecl_declaration) {
	strct = strct->node.declaration.sm_complex_type;
    }
    assert(strct->node_type == ecl_struct_type_decl);
    fields =  strct->node.struct_type_decl.fields;
    while(fields != NULL) {
	sm_ref ctype = fields->node->node.field.sm_complex_type;
	if ((ctype != NULL) && (ctype->node_type == ecl_reference_type_decl))
	    ctype = ctype->node.reference_type_decl.sm_complex_referenced_type;
	while (ctype != NULL) {
	    if (ctype->node_type == ecl_array_type_decl) {
		if (ctype->node.array_type_decl.sm_dynamic_size == expr) {
		    return 1;
		}
		ctype = ctype->node.array_type_decl.sm_complex_element_type;
	    } else {
		ctype = NULL;
	    }
	}
	fields = fields->next;
    }
    return 0;
}


static char*
type_list_to_string(ecl_parse_context context, sm_list type_list, int *size)
{
    sm_list orig_list = type_list;
    int short_appeared = 0;
    int long_appeared = 0;
    int int_appeared = 0;
    int double_appeared = 0;
    int float_appeared = 0;
    int char_appeared = 0;
    int signed_appeared = 0;
    int unsigned_appeared = 0;
    int void_appeared = 0;
    int string_appeared = 0;
    int spec_count = 0;
    int prefix_end = 0;
    int type_found = 0;
    sm_ref complex_return_type = NULL;
    int cg_type;

    cg_type = DR_ERR;
    while ((type_list != NULL) && (prefix_end == 0)) {
	int typ = type_list->node->node.type_specifier.token;
	if ((type_list->node->node_type != ecl_type_specifier) ||
	    (typ == STAR) || (typ == AT)) {
	    prefix_end = 1;
	} else {
	    spec_count++;
	    switch (typ) {
	    case INT:
		int_appeared++;
		break;
	    case LONG:
		long_appeared++;
		break;
	    case SHORT:
		short_appeared++;
		break;
	    case DOUBLE:
		double_appeared++;
		break;
	    case STRING:
		string_appeared++;
		break;
	    case VOID:
		void_appeared++;
		break;
	    case FLOAT:
		float_appeared++;
		break;
	    case CHAR:
		char_appeared++;
		break;
	    case SIGNED:
		signed_appeared++;
		break;
	    case UNSIGNED:
		unsigned_appeared++;
		break;
	    case TYPEDEF:
		break;
	    default:
		printf("Unknown type\n");
	    }
	    type_list = type_list->next;
	}
    }
    if (spec_count == 0) {
	if (type_list == NULL) cg_type = DR_I;   /* default to int */
	goto finalize;
    }
    if (void_appeared && (spec_count > 1)) {
	ecl_src_error(context, orig_list->node, 
		      "Void type may not appear with other specifiers");
	cg_type = DR_ERR;
	return NULL;
    }
    if (string_appeared && (spec_count > 1)) {
	ecl_src_error(context, orig_list->node, 
		      "String type may not appear with other specifiers");
	cg_type = DR_ERR;
	return NULL;
    }
    if (void_appeared) {
	cg_type = DR_V;
	goto finalize;
    }
    if (string_appeared) {
	cg_type = DR_P;
	goto finalize;
    }
    if (short_appeared + long_appeared > 1) {
	ecl_src_error(context, orig_list->node, 
		      "Only one of long or short permitted");
	cg_type = DR_ERR;
	return NULL;
    }
    if (short_appeared && (double_appeared + float_appeared)) {
	ecl_src_error(context, orig_list->node, 
		      "Short may not be specified with double or float");
	cg_type = DR_ERR;
	return NULL;
    }
    if (double_appeared + float_appeared) {
	if (double_appeared + float_appeared + short_appeared + signed_appeared + unsigned_appeared + char_appeared + int_appeared > 1) {
	    ecl_src_error(context, orig_list->node, "Bad type spec");
	    cg_type = DR_ERR;
	    return NULL;
	} else {
	    /* not handling LONG plus one of these */
	    if (double_appeared) {
		cg_type = DR_D;
		goto finalize;
	    } else {
		cg_type = DR_F;
		goto finalize;
	    }
	}
    }

    /* neither float or double appeared */
    if (short_appeared + char_appeared + long_appeared >= 2) {
	ecl_src_error(context, orig_list->node, 
		      "Only one integer size spec may be specified");
	cg_type = DR_ERR;
	return NULL;
    }
    if (unsigned_appeared + signed_appeared > 1) {
	ecl_src_error(context, orig_list->node, "Bad type spec");
	cg_type = DR_ERR;
	return NULL;
    }
    if (unsigned_appeared) {
	if (char_appeared) {
	    cg_type = DR_UC;
	    goto finalize;
	} else if (short_appeared) {
	    cg_type = DR_US;
	    goto finalize;
	} else if (long_appeared) {
	    cg_type = DR_UL;
	    goto finalize;
	} else {
	    cg_type = DR_U;
	    goto finalize;
	}
    } else {
	if (char_appeared) {
	    cg_type = DR_C;
	    goto finalize;
	} else if (short_appeared) {
	    cg_type = DR_S;
	    goto finalize;
	} else if (long_appeared) {
	    cg_type = DR_L;
	    goto finalize;
	} else {
	    cg_type = DR_I;
	    goto finalize;
	}
    }
 finalize:
    if (cg_type != DR_ERR) {
	type_found++;
    }
    switch(cg_type) {
    case DR_I: 
	*size = sizeof(int);
	return strdup("integer");
    case DR_L:
	*size = sizeof(long);
	return strdup("integer");
    case DR_S:
	*size = sizeof(short);
	return strdup("integer");
    case DR_U: 
	*size = sizeof(int);
	return strdup("unsigned");
    case DR_UL:
	*size = sizeof(long);
	return strdup("unsigned");
    case DR_US:
	*size = sizeof(short);
	return strdup("unsigned");
    case DR_F:
	*size = sizeof(float);
	return strdup("float");
    case DR_D:
	*size = sizeof(double);
	return strdup("float");
    }
    return NULL;
}

static sm_ref
ecl_build_parsed_type_node(ecl_parse_context c, char *name, sm_list l)
{
    sm_ref decl = ecl_new_struct_type_decl();
    sm_list *end_ptr = &decl->node.struct_type_decl.fields;

    sm_list tmp = l;
    sm_list last_type = NULL;
    int field_count = 0;
    decl->node.struct_type_decl.id = strdup(name);
    
     while(tmp != NULL) {
	sm_ref node = tmp->node;
	sm_list typ;
	int cg_type = DR_ERR;
	sm_list new_elem;
	new_elem = malloc(sizeof(*new_elem));
	new_elem->next = NULL;
	new_elem->node = ecl_new_field();
	if (node->node_type == ecl_declaration) {
	    typ = node->node.declaration.type_spec;
	    new_elem->node->node.field.name = node->node.declaration.id;
	    new_elem->node->node.field.string_type  = 
		type_list_to_string(c, typ, &new_elem->node->node.field.cg_size);
	} else if (node->node_type == ecl_array_type_decl) {
	    sm_ref base_decl = node->node.array_type_decl.element_ref;
	    sm_ref size = node->node.array_type_decl.size_expr;
	    char *base_string_type = NULL;
	    typ = node->node.array_type_decl.type_spec;
	    char *size_str = NULL, *final_type;
	    if (base_decl->node_type != ecl_declaration) {
		printf("Array base type must be a simple type\n");
		return NULL;
	    }
	    new_elem->node->node.field.name = base_decl->node.declaration.id;
	    base_string_type  = 
		type_list_to_string(c, typ, &new_elem->node->node.field.cg_size);
	    if (size->node_type == ecl_constant) {
		if (size->node.constant.token != integer_constant) {
		    printf("Array size constant is non-integer\n");
		    return NULL;
		} else {
		    size_str = size->node.constant.const_val;
		}
	    } else if (size->node_type == ecl_identifier) {
		size_str = size->node.identifier.id;
	    } else {
		printf("Unexpected value for array size\n");
		return NULL;
	    }
	    final_type = malloc(strlen(base_string_type) + 
				      strlen(size_str) + 3);
	    sprintf(final_type, "%s[%s]", base_string_type, size_str);
	    new_elem->node->node.field.string_type = final_type;
	    free(base_string_type);
		
	}
	new_elem->node->node.field.cg_offset = -1;
	new_elem->node->node.field.cg_type = DR_ERR;
	new_elem->node->node.field.type_spec = typ;
	free(node);
	field_count++;
	last_type = tmp;
	tmp = tmp->next;
	free(last_type);
	*end_ptr = new_elem;
	end_ptr = &new_elem->next;
    }
    return decl;
}

static int 
is_array(sm_ref expr)
{
    sm_ref typ;
    if (expr->node_type == ecl_field_ref) {
	return is_array(expr->node.field_ref.sm_field_ref);
    }
    if (expr->node_type == ecl_identifier) {
	return is_array(expr->node.identifier.sm_declaration);
    }
    if (expr->node_type == ecl_declaration) {
	sm_ref ctype = expr->node.declaration.sm_complex_type;
	if ((ctype != NULL) && (ctype->node_type == ecl_array_type_decl)) {
	    return 1;
	}
    }
    typ = get_complex_type(NULL, expr);
    if (typ == NULL) return 0;

    if (typ->node_type == ecl_array_type_decl) {
	return 1;
    }

    if (typ->node_type == ecl_reference_type_decl) {
	sm_ref ctype = 
	    typ->node.reference_type_decl.sm_complex_referenced_type;
	if (ctype == NULL) return 0;
	if (ctype->node_type == ecl_array_type_decl) {
	    return 1;
	}
    }
    return 0;
}
    
static sm_ref
get_containing_structure(sm_ref expr)
{
    switch(expr->node_type) {
    case ecl_element_ref:
	return get_containing_structure(expr->node.element_ref.array_ref);
    case ecl_field_ref:
	return expr->node.field_ref.struct_ref;
    default:
	return NULL;
    }
}

    
static void
add_field_list(int *format_count_p, IOFormatList *format_list_p, sm_ref typ)
{
    sm_list fields =  typ->node.struct_type_decl.fields;
    IOFieldList field_list = malloc(sizeof(field_list[0]) * 2);
    int field_count = 0;
    int my_format_num = (*format_count_p)++;
    *format_list_p = realloc(*format_list_p, sizeof(*format_list_p[0]) * (*format_count_p + 1));
    while(fields != NULL) {
	field_list = realloc(field_list, (sizeof(field_list[0]) * (field_count +2)));
	field_list[field_count].field_name = strdup(fields->node->node.field.name);
	field_list[field_count].field_type = strdup(fields->node->node.field.string_type);
	field_list[field_count].field_size = fields->node->node.field.cg_size;
	field_list[field_count].field_offset = fields->node->node.field.cg_offset;
	if ((fields->node->node.field.sm_complex_type != NULL) &&
	    (fields->node->node.field.sm_complex_type->node_type == ecl_struct_type_decl)) {
	    add_field_list(format_count_p, format_list_p,
			   fields->node->node.field.sm_complex_type);
	}
	field_count++;
	fields = fields->next;
    }
    field_list[field_count].field_name = field_list[field_count].field_type = NULL;
    field_list[field_count].field_size = field_list[field_count].field_offset = 0;
    (*format_list_p)[my_format_num].format_name = strdup(typ->node.struct_type_decl.id);
    (*format_list_p)[my_format_num].field_list = field_list;
}

static IOFormatList
build_format_list(ecl_parse_context context, sm_ref expr)
{
    sm_ref typ = get_complex_type(context, expr);
    IOFormatList formats = malloc(sizeof(formats[0]) * 2);
    int format_count = 0;
    sm_list fields;
    if (typ == NULL) {
	ecl_src_error(context, expr->node.field_ref.struct_ref, 
		      "Reference must be structured type", 
		      expr->node.field_ref.lx_field);
	return 0;
    }
    if (typ->node_type == ecl_reference_type_decl) {
	typ = typ->node.reference_type_decl.sm_complex_referenced_type;
    }
    if (typ->node_type == ecl_declaration) {
	typ = typ->node.declaration.sm_complex_type;
    }
    add_field_list(&format_count, &formats, typ);
    formats[format_count].format_name = NULL;
    formats[format_count].field_list = NULL;
    return formats;
}

static int semanticize_expr(ecl_parse_context context, sm_ref expr, 
			    scope_ptr scope) 
{
    switch(expr->node_type) {
    case ecl_identifier: {
	sm_ref tmp = resolve(expr->node.identifier.id, scope);
	if (tmp != NULL) {
	    expr->node.identifier.sm_declaration = tmp;
	    return 1;
	} else {
	    ecl_src_error(context, expr,
			  "Undefined Symbol \"%s\"", 
			  expr->node.identifier.id);
	    return 0;
	}
    }
    case ecl_cast: {
	int cg_type;
	sm_ref typ;
	if (expr->node.cast.expression &&
	    !semanticize_expr(context, expr->node.cast.expression, scope)) {
	    return 0;
	}

	typ = reduce_type_list(context, expr->node.cast.type_spec, &cg_type, 
			       scope, NULL);
	if ((cg_type == DR_ERR) && (typ == NULL)) {
	    ecl_src_error(context, expr, "Illegal cast");
	    return 0;
	}
	expr->node.cast.cg_type = cg_type;
	expr->node.cast.sm_complex_type = typ;
	return 1;
    }
    case ecl_operator: {
	int ret = 1;
	if (expr->node.operator.left != NULL) {
	    if (!semanticize_expr(context, expr->node.operator.left, scope)) {
		ret = 0;
	    }
	}
	if (expr->node.operator.right != NULL) {
	    if (!semanticize_expr(context, expr->node.operator.right, scope)) {
		ret = 0;
	    }
	}
	if (ret == 0) return 0;
	if ((expr->node.operator.left != NULL) && 
	    (expr->node.operator.right != NULL)) {
	    expr->node.operator.operation_type = 
		determine_op_type(context, expr, 
				  expr->node.operator.left,
				  expr->node.operator.right);
	    if (expr->node.operator.operation_type == DR_ERR) {
		return 0;
	    }
	} else if (expr->node.operator.right != NULL) {
	    expr->node.operator.operation_type = 
		determine_unary_type(context, expr, expr->node.operator.right);
	} else if (expr->node.operator.left != NULL) {
	    expr->node.operator.operation_type = 
		determine_unary_type(context, expr, expr->node.operator.left);
	}
	switch (expr->node.operator.op) {
	case op_leq: case op_lt: case op_geq: case op_gt: case op_neq: 
	case op_eq:  case op_log_neg: case op_log_or: case op_log_and:
	case op_sizeof:
	    expr->node.operator.result_type = DR_I;
	    break;
	case op_address:
	    expr->node.operator.result_type = DR_P;
	    if (expr->node.operator.right->node_type == ecl_identifier) {
		sm_ref decl = expr->node.operator.right->node.identifier.sm_declaration;
		if (decl->node_type == ecl_declaration) {
		    if (decl->node.declaration.param_num != -1) {
			ecl_src_error(context, expr, "Cannot take address of a parameter");
			return 0;
		    }
		    decl->node.declaration.addr_taken = 1;
		}
	    } else {
		/* Can only take the address of something we can name... */
		ecl_src_error(context, expr, "Invalid operand to address operator");
		return 0;
	    }
	    break;
	case op_deref: {
	    sm_ref typ = get_complex_type(context, expr->node.operator.right);
	    if (!typ || (typ->node_type != ecl_reference_type_decl)) {
		ecl_src_error(context, expr, "Cannot dereference a non-reference type");
		return 0;
	    } else {
		expr->node.operator.result_type =
		    typ->node.reference_type_decl.cg_referenced_type;
	    }
	    break;
	}
	default:
	    /* Operator applied to pointer types? Check compatibility... */
	    if(expr->node.operator.operation_type == DR_P) {
		
		switch(expr->node.operator.op) {
		case op_inc:
		case op_dec:
		    break;
		    
		case op_plus:
		{
		    sm_ref left  = expr->node.operator.left;
		    sm_ref right = expr->node.operator.right;
		    
		    sm_ref lcplx = NULL;
		    sm_ref rcplx = NULL;

		    if(!left) {
			ecl_src_error(context, expr,
				      "Invalid operand to unary plus\n");
			return 0;
		    }
		    
		    /* Extract complex types, if any */
		    lcplx = get_complex_type(context, left);
		    rcplx = get_complex_type(context, right);
		 
		    /* Pointers do not add with complex types */
		    if(lcplx && rcplx) {
			ecl_src_error(context, expr,
				      "Invalid operands to binary plus");
			return 0;
		    }

		    /*
		     * We're ok if we reach this, since that implies we have subtraction 
		     * between a pointer and an integral type. The suitability of the
		     * integral type has been checked in determine_op_type() already.
		     */
		}
		break;

		case op_minus:
		{
		    sm_ref left  = expr->node.operator.left;
		    sm_ref right = expr->node.operator.right;
		    
		    sm_ref lcplx = NULL;
		    sm_ref rcplx = NULL;

		    if(!left) {
			ecl_src_error(context, expr,
				      "Invalid operand to unary minus\n");
			return 0;
		    }

		    /* Extract complex types, if any */
		    lcplx = get_complex_type(context, left);
		    rcplx = get_complex_type(context, right);

		    
		    /* If both are complex types... */
		    if(lcplx && rcplx) {

			/* If both are pointers... */
			if(lcplx->node_type == ecl_reference_type_decl &&
			   rcplx->node_type == ecl_reference_type_decl) {
			    /* Check if the argument pointers are compatible */
			    if(!are_compatible_ptrs(lcplx, rcplx)) {
				ecl_src_error(context, expr,
					      "Incompatible pointer arguments to binary minus");
				return 0;
			    } else {
				/*
				 * Binary minus between two compatible pointers is allowed,
				 * but it produces an integral type, so we fix that here.
				 */
				expr->node.operator.result_type=DR_L;
				
				/*
				 * NOTE how we return success directly from here and do not
				 * break from the switch through the line below setting the
				 * result_type from the operation_type... In this case this
				 * would cause problems. We want operation_type to stay DR_P
				 * but the final result_type to be a DR_L.
				 */
				return 1;
			    }
			} else {
			    /*
			     * Pointers and other complex types do not subtract.
			     * Arithmetic canno be done on non-pointer complex types.
			     */
			    ecl_src_error(context, expr,
					  "Incompatible arguments to binary minus");
			    return 0;
			}
		    }

		    /*
		     * We're ok if we reach this, since that implies we have subtraction 
		     * between a pointer and an integral type. The suitability of the
		     * integral type has been checked in determine_op_type() already.
		     */
		}
		break;
		
		default:
		    ecl_src_error(context, expr,
				  "Operator cannot be applied to pointer types!\n");
		    return 0;
		}
	    }
	    /*
	     * NOTE: If anything here changes, one (potentially) has to
	     * update the code above which deals with binary minus
	     * between two compatible pointers, changes the result to
	     * an integral type, and returns directly without going
	     * through this code (as all other cases do).
	     */
	    expr->node.operator.result_type=expr->node.operator.operation_type;
	}
	return ret;
    }
    case ecl_constant:
	return 1;
    case ecl_assignment_expression: {
	int ret = 1;
	if (!semanticize_expr(context, expr->node.assignment_expression.left, scope)) {
	    ret = 0;
	} else {
	    expr->node.assignment_expression.cg_type = 
		ecl_sm_get_type(expr->node.assignment_expression.left);
	}
	if ((expr->node.assignment_expression.cg_type == DR_P) ||
	    (expr->node.assignment_expression.cg_type == DR_ERR)) {
	    sm_ref typ = 
		get_complex_type(context, 
				 expr->node.assignment_expression.left);
	    if (typ == NULL) {
		if (!is_string(expr->node.assignment_expression.left)) {
		    ecl_src_error(context, expr->node.assignment_expression.left, "Invalid assignment, left side must be simple, non-pointer type");
		    ret = 0;
		}
	    } else {
		if (typ->node_type != ecl_reference_type_decl) {
		    ecl_src_error(context, expr->node.assignment_expression.left, "Invalid assignment, left side must be simple, non-pointer type");
		    ret = 0;
		}
	    }
	}
	if (!semanticize_expr(context, expr->node.assignment_expression.right, scope)){
	    ret = 0;
	} else {
	    int right_type = 
		ecl_sm_get_type(expr->node.assignment_expression.right);
	    if ((right_type == DR_P) && 
		(is_string(expr->node.assignment_expression.right))) {
		if (expr->node.assignment_expression.cg_type != DR_P) {
		    ecl_src_error(context, expr, "assignment mixes string and non-string types");
		    ret = 0;
		}
	    } else if ((right_type == DR_B) || (right_type == DR_ERR)) {
		ecl_src_error(context, expr->node.assignment_expression.right, "Invalid assignment, right side must be simple type");
		ret = 0;
	    }
	}
	if (ret == 1) {
	    ret = assignment_types_match(context,
					 expr->node.assignment_expression.left,
					 expr->node.assignment_expression.right);
	}
	return ret;
    }
    case ecl_field_ref: {
	sm_ref typ;
	sm_list fields;
	if (!semanticize_expr(context, expr->node.field_ref.struct_ref, scope)) {
	    return 0;
	}
	typ = get_complex_type(context, expr->node.field_ref.struct_ref);
	if (typ == NULL) {
	    ecl_src_error(context, expr->node.field_ref.struct_ref, 
			  "Reference must be structured type", 
			  expr->node.field_ref.lx_field);
	    return 0;
	}
	if (typ->node_type == ecl_reference_type_decl) {
	    typ = typ->node.reference_type_decl.sm_complex_referenced_type;
	}
	if (typ->node_type == ecl_declaration) {
	    typ = typ->node.declaration.sm_complex_type;
	}
	fields =  typ->node.struct_type_decl.fields;
	while(fields != NULL) {
	    if (strcmp(expr->node.field_ref.lx_field,
		       fields->node->node.field.name) == 0) {
		break;
	    }
	    fields = fields->next;
	}
	if (fields == NULL) {
	    ecl_src_error(context, expr, 
			  "Unknown field reference, \"%s\".",
			  expr->node.field_ref.lx_field);
	    return 0;
	}
	expr->node.field_ref.sm_field_ref = fields->node;
	return 1;
    }
    case ecl_element_ref: {
	if (semanticize_expr(context, expr->node.element_ref.array_ref, scope)) {
	    int cg_type;
	    sm_ref arr;
	    if (!is_array(expr->node.element_ref.array_ref)) {
		ecl_src_error(context, expr, "Indexed element must be array");
		return 0;
	    }
	    arr = get_complex_type(NULL, expr->node.element_ref.array_ref);
	    if (arr->node_type == ecl_reference_type_decl) {
		arr = arr->node.reference_type_decl.sm_complex_referenced_type;
	    }
	    expr->node.element_ref.sm_complex_element_type = 
		arr->node.array_type_decl.sm_complex_element_type;
	    expr->node.element_ref.cg_element_type = 
		arr->node.array_type_decl.cg_element_type;
	    expr->node.element_ref.sm_containing_structure_ref =
		get_containing_structure(expr->node.element_ref.array_ref);
	    if (!semanticize_expr(context, expr->node.element_ref.expression, scope)) {
		return 0;
	    }
	    
	    cg_type = ecl_sm_get_type(expr->node.element_ref.expression);
	    switch(cg_type) {
	    case DR_C:
	    case DR_UC:
	    case DR_S:
	    case DR_US:
	    case DR_I: 
	    case DR_U: 
	    case DR_L:
	    case DR_UL:
		return 1;
		break;
	    }
	    ecl_src_error(context, expr, 
			  "Index for element reference must be integer type");
	    return 0;
	}
	return 0;
    }
    case ecl_subroutine_call: {
	sm_ref func_ref = expr->node.subroutine_call.sm_func_ref;
	sm_ref tmp = resolve(func_ref->node.identifier.id, scope);
	sm_list args = expr->node.subroutine_call.arguments;
	sm_list formals;
	int ret = 1;
	if (tmp != NULL) {
	    if ((tmp->node_type != ecl_declaration) ||
		!tmp->node.declaration.is_subroutine) {
		ecl_src_error(context, expr, 
			      "Identifier is not subroutine \"%s\".", 
			func_ref->node.identifier.id);
		return 0;
	    }
	    free(func_ref->node.identifier.id);
	    free(func_ref);
	    expr->node.subroutine_call.sm_func_ref = func_ref = tmp;
	    formals = func_ref->node.declaration.params;
	} else {
	    ecl_src_error(context, func_ref, "Undefined Subroutine \"%s\".", 
			  func_ref->node.identifier.id);

	    return 0;
	}
	while (args != NULL) {
	    sm_ref arg = args->node;
	    sm_ref formal = NULL;
	    if (formals != NULL) {
		formal = formals->node;
	    }
	    if (formal && (formal->node.declaration.sm_complex_type != NULL)) {
		sm_ref ct = formal->node.declaration.sm_complex_type;
		if ((ct->node_type == ecl_reference_type_decl) &&
		    (ct->node.reference_type_decl.name != NULL) &&
		    (strcmp(ct->node.reference_type_decl.name, "ecl_exec_context")
		     == 0)) {
		    if (context->has_exec_context == 0) {
			ecl_src_error(context, arg, "Calling subroutine has no ecl_exec_context");
			return 0;
		    }
		    /* swallow next formal, we'll fill that in ourselves */
		    formals = formals->next;
		    formal = formals->node;
		}
	    }
	    if (!semanticize_expr(context, arg, scope) ) {
		args = args->next;
		continue;
	    }
	    if (formal == NULL) {
		ecl_src_error(context, arg, "To many arguments to subroutine");
		ret = 0;
		return ret;
	    }
	    if (strcmp(formal->node.declaration.id, "...") != 0) {
		/* we've got a real formal to check against */
		/* do some checking... */
		int mismatch = 0;
		switch (ecl_sm_get_type(arg)) {
		case DR_D: case DR_F:
		    if ((formal->node.declaration.cg_type != DR_D) &&
			(formal->node.declaration.cg_type != DR_F)) {
			mismatch++;
		    }
		    break;
		case DR_I: case DR_U:
		case DR_L: case DR_UL:
		    if ((formal->node.declaration.cg_type == DR_D) ||
			(formal->node.declaration.cg_type == DR_F) ||
			(formal->node.declaration.cg_type == DR_P)) {
			sm_ref ct = formal->node.declaration.sm_complex_type;
			if (!ct || 
			    (ct->node_type != ecl_reference_type_decl) ||
			    (strcmp(ct->node.reference_type_decl.name, "ecl_type_spec") != 0)) {
			    mismatch++;
			}
		    }
		    break;
		case DR_P:
		    if ((formal->node.declaration.cg_type != DR_P) &&
			(formal->node.declaration.sm_complex_type->node_type !=
			 ecl_reference_type_decl)) {
			mismatch++;
		    }
		    break;
		}

		if (mismatch) {
		    ecl_src_error(context, arg, 
				  "Type mismatch, parameter \"%s\".",
			    formal->node.declaration.id);
		    ret = 0;
		}
	    }
	    if ((formals != NULL) &&
		(strcmp(formal->node.declaration.id, "...") != 0)) {
		formals = formals->next;
		formal = NULL;
		if (formals != NULL) formal = formals->node;
	    }
	    /* look ahead to next formal and insert an arg if it's ecl_type_spec */
	    if (formal &&
		formal->node.declaration.sm_complex_type != NULL) {
		sm_ref ct = formal->node.declaration.sm_complex_type;
		if ((ct->node_type == ecl_reference_type_decl) &&
		    (strcmp(ct->node.reference_type_decl.name, "ecl_type_spec")
		     == 0)) {
		    /* swallow next formal, we'll fill that in ourselves */
		    sm_list tmp_args = malloc(sizeof(struct list_struct));
		    IOFormatList list = build_format_list(context,  arg);
		    char tmp[30];
		    sprintf(&tmp[0], "%ld", list);
		    tmp_args->node = ecl_new_constant();
		    tmp_args->node->node.constant.token = integer_constant;
		    tmp_args->node->node.constant.const_val = strdup(tmp);
		    tmp_args->next = args->next;
		    args->next = tmp_args;
		}
	    }
	    args = args->next;
	    if ((args == NULL) && (formals != NULL)) {
		if (strcmp(formal->node.declaration.id, "...") != 0) {
		    ecl_src_error(context, arg, "Too few arguments to function");
		    ret = 0;
		}
	    }
	}
	return ret;
    }
    default:
	fprintf(stderr, "Unknown case in semanticize_expression\n");
	ecl_print(expr);
    }
    return 0;
}

extern int
ecl_sm_get_type(sm_ref node)
{
    switch(node->node_type) {
    case ecl_identifier:
	if (node->node.identifier.sm_declaration != NULL) {
	    return ecl_sm_get_type(node->node.identifier.sm_declaration);
	}
	return node->node.identifier.cg_type;
    case ecl_operator:
	return node->node.operator.result_type;
    case ecl_cast:
	return node->node.cast.cg_type;
    case ecl_assignment_expression:
	return node->node.assignment_expression.cg_type;
    case ecl_declaration:
	return node->node.declaration.cg_type;
    case ecl_constant:
	/* need to handle bigger constants */
	if (node->node.constant.token == string_constant) {
	    return DR_P;
	} else if (node->node.constant.token == floating_constant) {
	    return DR_F;
	} else {
	    return DR_I;
	}
    case ecl_field_ref:
	return ecl_sm_get_type(node->node.field_ref.sm_field_ref);
    case ecl_element_ref:
	return node->node.element_ref.cg_element_type;
    case ecl_field:
	if (is_array(node)) {
	    return DR_P;
	} else {
	    return node->node.field.cg_type;
	}
    case ecl_subroutine_call:
	return ecl_sm_get_type(node->node.subroutine_call.sm_func_ref);
    default:
	fprintf(stderr, "Unknown case in ecl_sm_get_type()\n");
	ecl_print(node);
    }
    return DR_ERR;
}

extern int
are_compatible_ptrs(sm_ref left, sm_ref right) {
    sm_ref lTyp, rTyp;

    /* Sanity check */
    if(left->node_type != ecl_reference_type_decl ||
       right->node_type != ecl_reference_type_decl) return 0;
    
    lTyp = left->node.reference_type_decl.sm_complex_referenced_type;
    rTyp = right->node.reference_type_decl.sm_complex_referenced_type;

    if(lTyp && rTyp) {
	/* Two complex referenced types */
	if(lTyp->node_type == ecl_reference_type_decl &&
	   rTyp->node_type == ecl_reference_type_decl) {
	    /* Recurse if both are pointers */
	    return are_compatible_ptrs(lTyp, rTyp);
	}
	return (lTyp == rTyp)?1:0;
    }

    if(!lTyp && !rTyp) {
	/* Two integral referenced types */
	return (left->node.reference_type_decl.cg_referenced_type ==
		right->node.reference_type_decl.cg_referenced_type)?1:0;
    }

    /* Mix of a pointer to a complex type and a pointer to an integral type */
    return 0;
}

extern sm_ref
get_complex_type(ecl_parse_context context, sm_ref node)
{
    switch(node->node_type) {
    case ecl_subroutine_call:
	return NULL;
    case ecl_identifier:
	return get_complex_type(context, 
				node->node.identifier.sm_declaration);
    case ecl_element_ref:
	return node->node.element_ref.sm_complex_element_type;
    case ecl_field:
	return node->node.field.sm_complex_type;
    case ecl_declaration:
	return node->node.declaration.sm_complex_type;
    case ecl_field_ref:{
	sm_ref typ;
	sm_list fields;
	typ = get_complex_type(context, node->node.field_ref.struct_ref);
	if (typ->node_type == ecl_reference_type_decl) {
	    typ = typ->node.reference_type_decl.sm_complex_referenced_type;
	}
	if (typ->node_type == ecl_declaration) {
	    typ = typ->node.declaration.sm_complex_type;
	}
	fields =  typ->node.struct_type_decl.fields;
	while ((fields != NULL) && 
	       (strcmp(node->node.field_ref.lx_field,
		       fields->node->node.field.name) != 0)) {
	    fields = fields->next;
	}
	if (fields == NULL) {
	    ecl_src_error(context, node, "Unknown field reference \"%s\".",
		    node->node.field_ref.lx_field);
	    return NULL;
	}
	return fields->node->node.field.sm_complex_type;
    }
    case ecl_constant:
	return NULL;
    case ecl_operator:
	switch (node->node.operator.op) {
	case op_deref: {
	    sm_ref right = get_complex_type(NULL, node->node.operator.right);
	    if ((right != NULL) && 
		(right->node_type == ecl_reference_type_decl)) {
		return right->node.reference_type_decl.sm_complex_referenced_type;
	    }
	    return NULL;
	}
	case op_plus: case op_minus: case op_inc: case op_dec: {
	    sm_ref right = NULL;
	    sm_ref left  = NULL;
	    if (node->node.operator.right)
		right = get_complex_type(NULL, node->node.operator.right);
	    if (node->node.operator.left)
		left = get_complex_type(NULL, node->node.operator.left);
	    if (right && (left == NULL)) return right;
	    if (left && (right == NULL)) return left;
	    if ((left == NULL) && (right == NULL)) return NULL;
	    /*
	     * GANEV: op_minus can be applied to two pointers,
	     * i.e. two complex types => both left _and_ right can be
	     * non-NULL, hence this code. This shouldn't happen in
	     * other cases... (I think).
	     */
	    if(node->node.operator.op == op_minus && right && left) {
		if(left->node_type == ecl_reference_type_decl &&
		   right->node_type == ecl_reference_type_decl) {
		    /* Ok, so it's op_minus between two pointers, then check compatibility */
		    if(are_compatible_ptrs(left, right)) {
			return left;
		    } else {
			ecl_src_error(context, node, 
				      "Incompatible pointer args to binary minus");
			return NULL;
		    }
		}
	    }
	    fprintf(stderr, "Unknown case in get_complex_type() int ops\n");
	    ecl_print(node);
	    return NULL;
	}
	default:
	    return NULL;
	}

    case ecl_cast:
	/* GANEV: can we have casts to complex types? */
	return NULL;
	break;

    default:
	fprintf(stderr, "Unknown case in get_complex_type()\n");
	ecl_print(node);
    }
    return NULL;
}

static sm_ref
reduce_type_list(ecl_parse_context context, sm_list type_list, int *cg_type,
			    scope_ptr scope, int*is_typedef) 
{
    sm_list orig_list = type_list;
    int short_appeared = 0;
    int long_appeared = 0;
    int int_appeared = 0;
    int double_appeared = 0;
    int float_appeared = 0;
    int char_appeared = 0;
    int signed_appeared = 0;
    int unsigned_appeared = 0;
    int void_appeared = 0;
    int string_appeared = 0;
    int spec_count = 0;
    int prefix_end = 0;
    int type_found = 0;
    sm_ref complex_return_type = NULL;;

    *cg_type = DR_ERR;
    while ((type_list != NULL) && (prefix_end == 0)) {
	int typ = type_list->node->node.type_specifier.token;
	if ((type_list->node->node_type != ecl_type_specifier) ||
	    (typ == STAR) || (typ == AT)) {
	    prefix_end = 1;
	} else {
	    spec_count++;
	    switch (typ) {
	    case INT:
		int_appeared++;
		break;
	    case LONG:
		long_appeared++;
		break;
	    case SHORT:
		short_appeared++;
		break;
	    case DOUBLE:
		double_appeared++;
		break;
	    case STRING:
		string_appeared++;
		break;
	    case VOID:
		void_appeared++;
		break;
	    case FLOAT:
		float_appeared++;
		break;
	    case CHAR:
		char_appeared++;
		break;
	    case SIGNED:
		signed_appeared++;
		break;
	    case UNSIGNED:
		unsigned_appeared++;
		break;
	    case TYPEDEF:
		if (is_typedef) (*is_typedef)++;
		break;
	    default:
		printf("Unknown type\n");
	    }
	    type_list = type_list->next;
	}
    }
    if (spec_count == 0) {
	if (type_list == NULL) *cg_type = DR_I;   /* default to int */
	goto finalize;
    }
    if (void_appeared && (spec_count > 1)) {
	ecl_src_error(context, orig_list->node, 
		      "Void type may not appear with other specifiers");
	*cg_type = DR_ERR;
	return NULL;
    }
    if (string_appeared && (spec_count > 1)) {
	ecl_src_error(context, orig_list->node, 
		      "String type may not appear with other specifiers");
	*cg_type = DR_ERR;
	return NULL;
    }
    if (void_appeared) {
	*cg_type = DR_V;
	goto finalize;
    }
    if (string_appeared) {
	*cg_type = DR_P;
	goto finalize;
    }
    if (short_appeared + long_appeared > 1) {
	ecl_src_error(context, orig_list->node, 
		      "Only one of long or short permitted");
	*cg_type = DR_ERR;
	return NULL;
    }
    if (short_appeared && (double_appeared + float_appeared)) {
	ecl_src_error(context, orig_list->node, 
		      "Short may not be specified with double or float");
	*cg_type = DR_ERR;
	return NULL;
    }
    if (double_appeared + float_appeared) {
	if (double_appeared + float_appeared + short_appeared + signed_appeared + unsigned_appeared + char_appeared + int_appeared > 1) {
	    ecl_src_error(context, orig_list->node, "Bad type spec");
	    *cg_type = DR_ERR;
	    return NULL;
	} else {
	    /* not handling LONG plus one of these */
	    if (double_appeared) {
		*cg_type = DR_D;
		goto finalize;
	    } else {
		*cg_type = DR_F;
		goto finalize;
	    }
	}
    }

    /* neither float or double appeared */
    if (short_appeared + char_appeared + long_appeared >= 2) {
	ecl_src_error(context, orig_list->node, 
		      "Only one integer size spec may be specified");
	*cg_type = DR_ERR;
	return NULL;
    }
    if (unsigned_appeared + signed_appeared > 1) {
	ecl_src_error(context, orig_list->node, "Bad type spec");
	*cg_type = DR_ERR;
	return NULL;
    }
    if (unsigned_appeared) {
	if (char_appeared) {
	    *cg_type = DR_UC;
	    goto finalize;
	} else if (short_appeared) {
	    *cg_type = DR_US;
	    goto finalize;
	} else if (long_appeared) {
	    *cg_type = DR_UL;
	    goto finalize;
	} else {
	    *cg_type = DR_U;
	    goto finalize;
	}
    } else {
	if (char_appeared) {
	    *cg_type = DR_C;
	    goto finalize;
	} else if (short_appeared) {
	    *cg_type = DR_S;
	    goto finalize;
	} else if (long_appeared) {
	    *cg_type = DR_L;
	    goto finalize;
	} else {
	    *cg_type = DR_I;
	    goto finalize;
	}
    }
 finalize:
    if (type_list == NULL) {
	/* no error and no more to process */
	return NULL;  
    }
    if (*cg_type != DR_ERR) {
	type_found++;
    }
    while (type_list != NULL) {
	sm_ref node = type_list->node;
	switch (node->node_type) {
	case ecl_identifier: 
	{
	    if (type_found != 0) {
		ecl_src_error(context, node, 
			      "Type identifier cannot follow prior identifiers");
		*cg_type = DR_ERR;
		return NULL;
	    }
	    complex_return_type = find_complex_type(node, scope);
	    if ((complex_return_type != NULL)&&
		(complex_return_type->node_type == ecl_declaration)) {
		complex_return_type = complex_return_type->node.declaration.sm_complex_type;
	    }
	    if ((complex_return_type != NULL)&&
		(complex_return_type->node_type == ecl_reference_type_decl)) {
		*cg_type = DR_P;
	    }
	    if ((complex_return_type != NULL)&&
		(complex_return_type->node_type == ecl_struct_type_decl)) {
		*cg_type = DR_B;
	    }
	    if ((complex_return_type == NULL) && node->node.identifier.id &&
		((strcmp(node->node.identifier.id, "ecl_type_spec") == 0) ||
		 (strcmp(node->node.identifier.id, "ecl_exec_context") == 0))) {
		/* special ECL type information for prior arg */
		sm_ref typ = ecl_new_reference_type_decl();
		typ->node.reference_type_decl.name = node->node.identifier.id;
		if (strcmp(node->node.identifier.id, "ecl_type_spec") == 0) {
		    *cg_type = DR_P;
		} else {
		    context->has_exec_context = 1;
		    *cg_type = DR_EC;
		}
		typ->node.reference_type_decl.cg_referenced_type = *cg_type;
		typ->node.reference_type_decl.sm_complex_referenced_type =
		    complex_return_type;
		typ->node.reference_type_decl.kernel_ref = 0;
		complex_return_type = typ;
	    }
	    assert(complex_return_type != NULL);
	    type_found++;
	}
	break;
	case ecl_type_specifier:
	    switch (node->node.type_specifier.token) {
	    case STAR:
	      {
		  if (node->node.type_specifier.created_type_decl == NULL) {
		      /* GSE create anon-type */
		      sm_ref typ = ecl_new_reference_type_decl();
		      typ->node.reference_type_decl.name = gen_anon();
		      typ->node.reference_type_decl.cg_referenced_type = *cg_type;
		      *cg_type = DR_P;
		      typ->node.reference_type_decl.sm_complex_referenced_type =
			  complex_return_type;
		      typ->node.reference_type_decl.kernel_ref = 0;
		      complex_return_type = typ;
		      node->node.type_specifier.created_type_decl = typ;
		  } else {
		      complex_return_type = node->node.type_specifier.created_type_decl;
		      *cg_type = DR_P;
		  }
	      }
	      break;
	    case AT:
	      {
		  /* GSE create anon-type */
		  sm_ref typ = ecl_new_reference_type_decl();
		  typ->node.reference_type_decl.name = gen_anon();
		  typ->node.reference_type_decl.cg_referenced_type = *cg_type;
		  *cg_type = DR_P;
		  typ->node.reference_type_decl.sm_complex_referenced_type =
		      complex_return_type;
		  typ->node.reference_type_decl.kernel_ref = 1;
		  complex_return_type = typ;
	      }
	      break;
	    default:
		if (type_found != 0) {
		    ecl_src_error(context, node, 
				  "Only '*', '@', and CONST can follow valid type");
		    *cg_type = DR_ERR;
		    return NULL;
		}
	    }
	    break;
	case ecl_struct_type_decl: {
	    complex_return_type = node;
	    break;
	}
	default:
	    printf("Unexpected node in reduce_type_list\n");
	    return NULL;
	}
	type_list = type_list->next;
    }
    return complex_return_type;
}

static int 
assignment_types_match(ecl_parse_context context, sm_ref left, sm_ref right)
{
    sm_ref left_smt, right_smt;
    int left_cgt, right_cgt;
    left_smt = get_complex_type(context, left);
    right_smt = get_complex_type(context, right);
    left_cgt = ecl_sm_get_type(left);
    right_cgt = ecl_sm_get_type(right);
    if ((left_smt == NULL) && (right_smt == NULL)) {
	/* just check cgts */
	/* don't mix DR_P, DR_B and anything else */
	switch (left_cgt) {
	case DR_P: 
	    switch (right_cgt) {
	    case DR_P:
	    case DR_L:
	    case DR_UL:
		return 1;
		break;
	    default:
		ecl_src_error(context, left, "Trying to assign a pointer variable with a non-pointer value.");
		return 0;
	    }
	default:
	    switch (right_cgt) {
	    case DR_P:
		ecl_src_error(context, left, "Trying to assign pointer to an incompatible variable.");
		return 0;
	    default:
		return 1;
		break;
	    }
	}
    }
    if ((left_smt != NULL) && 
	(left_smt->node_type != ecl_reference_type_decl)) {
	ecl_src_error(context, left, "Only pointer complex types allowed as LHS in assignment");
	return 0;
    }
    if ((right_smt != NULL) && 
	(right_smt->node_type != ecl_reference_type_decl)) {
	ecl_src_error(context, right, "Only pointer complex types allowed as RHS in assignment");
	return 0;
    }
    if (left_smt && (right_smt == NULL)) {

	switch(right_cgt) {
	case DR_P:
	case DR_L:
	case DR_UL:
	    return 1;

	default:
	    ecl_src_error(context, right, "Right hand side must be pointer type");
	    return 0;
	}
    }
    if (right_smt && (left_smt == NULL)) {
	switch(left_cgt) {
	case DR_C:
	case DR_UC:
	case DR_S:
	case DR_US:
	case DR_I:
	case DR_U:
	case DR_L:
	case DR_UL:
	case DR_P:
	    /* GANEV: should we have a warning here? */
	    return 1;

	default:
	    ecl_src_error(context, right, "Pointer converted without explicit cast");
	    return 0;
	}
    }
    return 1;	
}

static int semanticize_type_node(ecl_parse_context context, sm_ref decl, 
				 scope_ptr scope);

static int semanticize_decl(ecl_parse_context context, sm_ref decl, 
			    scope_ptr scope)
{
    switch(decl->node_type) {
    case ecl_declaration: 
	if (resolve_local(decl->node.declaration.id, scope) != NULL) {
	    if (resolve_local(decl->node.declaration.id, scope) != decl) {
		ecl_src_error(context, decl, "Duplicate Symbol \"%s\"", 
			      decl->node.declaration.id);
		return 0;
	    } else {
		/* been here, done that */
		return 1;
	    }
	} else {
	    add_decl(decl->node.declaration.id, decl, scope);
	}
	if (scope->containing_scope == NULL) {
	    /* must be external variable */
	    void *extern_value = 
		resolve_extern(decl->node.declaration.id, scope);
	    if (extern_value == NULL) {
		ecl_src_error(context, decl, 
			      "External symbol lacking address \"%s\"", 
			decl->node.declaration.id);
		return  0;
	    }
	    decl->node.declaration.cg_address = extern_value;
	    decl->node.declaration.is_extern = 1;
	}
	if (decl->node.declaration.type_spec != NULL) {
	    sm_list l = decl->node.declaration.type_spec;
	    if ((l->node->node_type == ecl_type_specifier) && 
		(l->node->node.type_specifier.token == STATIC)) {
		decl->node.declaration.type_spec = l->next;
		decl->node.declaration.static_var = 1;
		free(l);
	    }
	}
	if (decl->node.declaration.static_var) {
	    if (decl->node.declaration.init_value != NULL) {
		sm_ref const_val = decl->node.declaration.init_value;
		if (const_val->node_type != ecl_constant) {
		    ecl_src_error(context, const_val, 
				  "Only simple constants allowed as static initializers.  Variable \"%s\"",
			decl->node.declaration.id);
		    return 0;
		}
	    }
	}
	if ((decl->node.declaration.sm_complex_type != NULL) &&
	    (decl->node.declaration.param_num != -1)) {
	    /* complex type + param, must be pass by reference */
	    sm_ref type = decl->node.declaration.sm_complex_type;
	    decl->node.declaration.cg_type = DR_P;
	    if (type->node_type == ecl_array_type_decl) {
		int ret = semanticize_array_type_node(context, type, 
						      scope);
		if (ret == 0) return ret;
	    }
	}
	/* some array decls have sm_complex_type set already */
	if (decl->node.declaration.sm_complex_type == NULL) {
	    sm_ref typ = NULL;
	    int cg_type = DR_I;
	    if (decl->node.declaration.type_spec != NULL) {
		int type_def = 0;
		typ = reduce_type_list(context, decl->node.declaration.type_spec,
				       &cg_type, scope, &type_def);
		if (type_def) {
		    decl->node.declaration.is_typedef = 1;
		}
		if (type_def != 0) {
		    semanticize_decl(context, typ, scope);
		}
	    } else {
		sm_ref arr = decl->node.declaration.sm_complex_type;
		if ((arr != NULL) && 
		    (arr->node_type == ecl_array_type_decl)) {
		    typ == reduce_type_list(context, 
					    arr->node.array_type_decl.type_spec, 
					    &cg_type, scope, NULL);
		}
	    }
	    if ((typ == NULL) && (cg_type == DR_ERR)) return 0;
	    decl->node.declaration.cg_type = cg_type;
	    decl->node.declaration.sm_complex_type = typ;
	}
	if (decl->node.declaration.init_value != NULL) {
	    int ret;
	    ret = semanticize_expr(context, decl->node.declaration.init_value, 
				   scope);
	    if (ret == 0) return ret;
	    ret = assignment_types_match(context, decl, 
					 decl->node.declaration.init_value);
	    return ret;
	}
	if (decl->node.declaration.is_subroutine) {
	    int ret;
	    scope_ptr sub_scope = push_scope(scope);
	    ret = semanticize_decls_list(context,
					 decl->node.declaration.params, 
					 sub_scope);
	    pop_scope(sub_scope);
	    return ret;
	}
	return 1;
	break;
    case ecl_struct_type_decl:
	return semanticize_type_node(context, decl, scope);
	break;
    case ecl_array_type_decl:
	return semanticize_array_type_node(context, decl, scope);
	break;
    case ecl_reference_type_decl:
	return semanticize_reference_type_node(context, decl, scope);
	break;
    default:
	printf("Unhandled case in semanticize decls_list\n");
	ecl_print(decl);
    }
    return 0;
}

static int
semanticize_decls_list(ecl_parse_context context, sm_list decls, 
		       scope_ptr scope)
{
    int ret = 1;
    while (decls != NULL) {
	if (!semanticize_decl(context, decls->node, scope)) {
	    ret = 0;
	}
	decls = decls->next;
    }
    return ret;
}

static int semanticize_statement(ecl_parse_context context, sm_ref stmt, 
				 scope_ptr scope);

static int
semanticize_selection_statement(ecl_parse_context context, sm_ref selection,
				scope_ptr scope)
{
    int ret = 1;
    if (!semanticize_expr(context, 
			  selection->node.selection_statement.conditional,
			  scope)) {
	ret = 0;
    }
    if (!semanticize_statement(context,
			       selection->node.selection_statement.then_part,
			       scope)) {
	ret = 0;
    }
    if (selection->node.selection_statement.else_part) {
	if (!semanticize_statement(context,
				   selection->node.selection_statement.else_part,
				   scope)) {
	    ret = 0;
	}
    }
    return ret;
}

static int
semanticize_iteration_statement(ecl_parse_context context, sm_ref iteration,
				scope_ptr scope)
{
    int ret = 1;
    if (iteration->node.iteration_statement.init_expr != NULL) {
	if (!semanticize_expr(context, 
			      iteration->node.iteration_statement.init_expr,
			      scope)) {
	    ret = 0;
	}
    }

    if (iteration->node.iteration_statement.test_expr != NULL) {
	if (!semanticize_expr(context, 
			      iteration->node.iteration_statement.test_expr,
			      scope)) {
	    ret = 0;
	}
    }

    if (iteration->node.iteration_statement.iter_expr != NULL) {
	if (!semanticize_expr(context, 
			      iteration->node.iteration_statement.iter_expr,
			      scope)) {
	    ret = 0;
	}
    }

    if (iteration->node.iteration_statement.statement != NULL) {
	if (!semanticize_statement(context,
				   iteration->node.iteration_statement.statement,
				   scope)) {
	    ret = 0;
	}
    }
    return ret;
}

static int 
semanticize_statement(ecl_parse_context context, sm_ref stmt, 
		      scope_ptr scope)
{
    switch (stmt->node_type) {
    case ecl_selection_statement:
	return semanticize_selection_statement(context, stmt, scope);
    case ecl_iteration_statement:
	return semanticize_iteration_statement(context, stmt, scope);
    case ecl_expression_statement: {
	return semanticize_expr(context, 
				stmt->node.expression_statement.expression,
				scope);
    }	
    case ecl_compound_statement:
	return semanticize_compound_statement(context, stmt, scope);
    case ecl_return_statement:{
	stmt->node.return_statement.cg_func_type = context->return_cg_type;
	return semanticize_expr(context, 
				stmt->node.return_statement.expression,
				scope);
    }	
    default:
	printf("unhandled case in semanticize statement\n");
	return 1;
    }
}

static int
semanticize_statements(ecl_parse_context context, sm_list stmts, 
		       scope_ptr scope)
{
    int ret = 1;
    while (stmts != NULL) {
	if (!semanticize_statement(context, stmts->node, scope)) {
	    ret = 0;
	}
	stmts = stmts->next;
    }
    return ret;
}

extern int
semanticize_compound_statement(ecl_parse_context context, sm_ref compound, 
			       scope_ptr containing_scope)
{
    int ret = 1;
    scope_ptr current_scope = push_scope(containing_scope);
    ret &= semanticize_decls_list(context,
				  compound->node.compound_statement.decls,
				  current_scope);
    ret &= semanticize_statements(context,
				  compound->node.compound_statement.statements,
				  current_scope);
    pop_scope(current_scope);
    return ret;
}

extern sm_ref
ecl_build_type_node(const char *name, IOFieldList field_list)
{
    sm_ref decl = ecl_new_struct_type_decl();
    sm_list *end_ptr = &decl->node.struct_type_decl.fields;

    decl->node.struct_type_decl.id = strdup(name);
    while ((field_list != NULL) && (field_list->field_name != NULL)) {
	sm_list new_elem;
	new_elem = malloc(sizeof(*new_elem));
	new_elem->next = NULL;
	new_elem->node = ecl_new_field();
	new_elem->node->node.field.name = strdup(field_list->field_name);
	new_elem->node->node.field.string_type = strdup(field_list->field_type);
	new_elem->node->node.field.cg_size = field_list->field_size;
	new_elem->node->node.field.cg_offset = field_list->field_offset;
	new_elem->node->node.field.cg_type = DR_ERR;
	*end_ptr = new_elem;
	end_ptr = &new_elem->next;
	field_list++;
    }
    return decl;
}


void
ecl_add_defined_type(id, context)
char *id;
ecl_parse_context context;
{
    int count = context->defined_type_count;
    if (count == 0) {
	context->defined_types = malloc(sizeof(char*));
    } else {
	context->defined_types = realloc(context->defined_types,
					 (count+1)*sizeof(char*));
    }
    context->defined_types[count] = id;
    context->defined_type_count++;
    reset_types_table(context->defined_type_count, context->defined_types);
}

extern void
ecl_add_struct_type(const char *name, IOFieldList field_list, 
		    ecl_parse_context context)
{
    sm_ref node = ecl_build_type_node(name, field_list);
    ecl_add_decl_to_parse_context(name, node, context);
    ecl_add_decl_to_scope((char*)name, node, context);
}

static int
str_to_data_type(str, size)
char *str;
int size;
{
    char *tmp = malloc(strlen(str) + 1);
    strcpy(tmp, str);
    str = tmp;			/* make a copy of str parameter */

    while (isspace((int)*str)) {	/* skip preceeding space */
	str++;
    }
    tmp = str + strlen(str) - 1;
    while (isspace((int)*tmp)) {	/* kill trailing space */
	*tmp = 0;
	tmp--;
    }
    tmp = str;
    while (*tmp) {		/* map to lower case */
	*tmp = tolower(*tmp);
	tmp++;
    }
    if ((strcmp(str, "integer") == 0) || (strcmp(str, "enumeration") == 0)) {
	free(str);
	if (size == sizeof(long)) {
	    return DR_L;
	} else if (size == sizeof(int)) {
	    return DR_I;
	} else if (size == sizeof(short)) {
	    return DR_S;
	} else if (size == sizeof(char)) {
	    return DR_C;
	} else {
	    fprintf(stderr, "unsupported integer size %d\n", size);
	    return DR_I;
	}
    } else if (strcmp(str, "unsigned integer") == 0) {
	free(str);
	if (size == sizeof(long)) {
	    return DR_UL;
	} else if (size == sizeof(int)) {
	    return DR_U;
	} else if (size == sizeof(short)) {
	    return DR_US;
	} else if (size == sizeof(char)) {
	    return DR_UC;
	} else {
	    fprintf(stderr, "unsupported unsigned integer size %d\n", size);
	    return DR_I;
	}
    } else if ((strcmp(str, "float") == 0) || (strcmp(str, "double") == 0)) {
	free(str);
	if (size == sizeof(double)) {
	    return DR_D;
	} else if (size == sizeof(float)) {
	    return DR_F;
	} else {
	    fprintf(stderr, "unsupported float size %d\n", size);
	    return DR_D;
	}
    } else if (strcmp(str, "char") == 0) {
	free(str);
	assert(size == 1);
	return DR_C;
    } else if (strcmp(str, "string") == 0) {
	free(str);
	assert(size == sizeof(char*));
	return DR_P;
    } else {
	free(str);
	return DR_ERR;
    }
}

static int
array_str_to_data_type(str, size)
char *str;
int size;
{
    int ret_type;
    char field_type[1024];
    char *left_paren;
    if ((left_paren = strchr(str, '[')) == NULL) {
	ret_type = str_to_data_type(str, size);
    } else {
	char *tmp = str;
	int i = 0;
	for( ; tmp < left_paren; tmp++) {
	    field_type[i++] = *tmp;
	}
	field_type[i] = 0;
	ret_type = str_to_data_type(field_type, size);
    }
    return ret_type;
}

static int
IOget_array_size_dimen(str, fields, dimen, control_field)
const char *str;
IOFieldList fields;
int dimen;
int *control_field;
{
    char *left_paren, *end;
    long static_size;

    *control_field = -1;
    if ((left_paren = strchr(str, '[')) == NULL) {
	return 0;
    }	
    while (dimen != 0) {
	left_paren = strchr(left_paren + 1, '[');
	if (left_paren == NULL) return 0;
	dimen--;
    }
    static_size = strtol(left_paren + 1, &end, 0);
    if (left_paren + 1 == end) {
	/* dynamic element */
	char field_name[1024];
	int count = 0;
	int i = 0;
	while (((left_paren+1)[count] != ']') &&
	       ((left_paren+1)[count] != 0)) {
	    field_name[count] = (left_paren+1)[count];
	    count++;
	}
	field_name[count] = 0;
	while (fields[i].field_name != NULL) {
	    if (strcmp(field_name, fields[i].field_name) == 0) {
		*control_field = i;
		return -1;
	    }
	    i++;
	}
	fprintf(stderr, "Array dimension \"%s\" in type spec \"%s\" not recognized.\n",
		field_name, str);
	fprintf(stderr, "Dimension must be a field name (for dynamic arrays) or a positive integer.\n");
	fprintf(stderr, "To use a #define'd value for the dimension, use the IOArrayDecl() macro.\n");
	return -1;
    }
    if (*end != ']') {
	fprintf(stderr, "Malformed array dimension, unexpected character '%c' in type spec \"%s\"\n",
		*end, str);
	fprintf(stderr, "Dimension must be a field name (for dynamic arrays) or a positive integer.\n");
	fprintf(stderr, "To use a #define'd value for the dimension, use the IOArrayDecl() macro.\n");
	return -1;
    }
    if (static_size <= 0) {
	fprintf(stderr, "Non-positive array dimension %ld in type spec \"%s\"\n",
		static_size, str);
	fprintf(stderr, "Dimension must be a field name (for dynamic arrays) or a positive integer.\n");
	fprintf(stderr, "To use a #define'd value for the dimension, use the IOArrayDecl() macro.\n");
	return -1;
    }
    return static_size;
}

static sm_ref
build_array_decl_list(context, str, fields, cg_type, cg_size, complex_type, err)
ecl_parse_context context;
char *str;
IOFieldList fields;
int cg_type;
int cg_size;
sm_ref complex_type;
int *err;
{
    sm_ref ret = ecl_new_array_type_decl();
    int static_size;
    int control_field;
    char *rparen;
    static_size = IOget_array_size_dimen(str, fields, 0, &control_field);
    if (static_size == 0) {
	if (*str == 0) {
	    return NULL;
	} else {
	    ecl_src_error(context, NULL, 
			  "Failed to parse or find array dimension in \"%s\" not found.", str);
	    *err = 1;
	    return NULL;
	}
    }
    ret->node.array_type_decl.cg_static_size = static_size;
    if (static_size != -1) {
	ret->node.array_type_decl.sm_dynamic_size = NULL;
    } else {	
	sm_ref cf = (sm_ref)(fields[control_field].field_type);
	switch (str_to_data_type(cf->node.field.string_type, 
				 (int)sizeof(int))) {
	case DR_C: case DR_UC: case DR_S: case DR_US: 
	case DR_I: case DR_U: case DR_L: case DR_UL:
	    break;
	default:
	    ecl_src_error(context, NULL, 
			  "Variable length control field \"%s\"not of integer type.", str);
	    *err = 1;
	    return NULL;
	    break;
	}
	ret->node.array_type_decl.sm_dynamic_size = 
	    (sm_ref)fields[control_field].field_type;
    }
    rparen = strchr(str, ']');
    rparen++;
    if (*rparen == 0) {
	/* we're at the end, fill in other fields */
	ret->node.array_type_decl.cg_element_type = cg_type;
	ret->node.array_type_decl.sm_complex_element_type = complex_type;
	ret->node.array_type_decl.cg_element_size = cg_size;
    } else {
	sm_ref sub_type = build_array_decl_list(context, rparen, fields, 
						cg_type, cg_size,
						complex_type, err);
	int sub_size;
	if (*err == 1) {
	    free(ret);
	    return NULL;
	}
	ret->node.array_type_decl.cg_element_type = DR_B;
	ret->node.array_type_decl.sm_complex_element_type = sub_type;
	sub_size = sub_type->node.array_type_decl.cg_static_size;
	if (sub_size == -1) {
	    /* element of *this* array has varying elements */
	    ret->node.array_type_decl.cg_element_size = -1;
	} else {
	    ret->node.array_type_decl.cg_element_size = 
		sub_size * sub_type->node.array_type_decl.cg_element_size;;
	}
    }
    return ret;
}



static int
is_var_array(context, decl, f, fields, cg_size, cg_type, complex_type, err)
ecl_parse_context context;
sm_ref decl;
sm_ref f;
sm_list fields;
int cg_size;
int cg_type;
sm_ref complex_type;
int *err;
{
    char *str = f->node.field.string_type;
    char *lparen;
    *err = 0;

    if ((lparen = strchr(str, '[')) == NULL) {
	return 0;
    } else {
	int cg_type;
	int field_count = 0;
	sm_ref arr;
	IOFieldList list = malloc(2*sizeof(list[0]));
	while (fields != NULL) {
	    list = realloc(list, (field_count + 2) * sizeof(list[0]));
	    list[field_count].field_name = fields->node->node.field.name;
	    list[field_count].field_type = (char*) fields->node; /* cheap trick */
	    field_count++;
	    fields = fields->next;
	}
	list[field_count].field_name = list[field_count].field_type = NULL;
	cg_type = array_str_to_data_type(str, cg_size);
	if (complex_type && 
	    (complex_type->node_type ==ecl_reference_type_decl)) {
	    cg_type = DR_P;
	    cg_size = sizeof(char*);
	}
	arr = build_array_decl_list(context, lparen, list, cg_type, cg_size, 
				    complex_type, err);
	free(list);
	if (*err == 1) {
	    return 0;
	}
	if ((arr->node.array_type_decl.cg_static_size == -1) ||
	    (arr->node.array_type_decl.cg_element_size == -1)) {
	    /* this array is varying */
	    /* GSE create anon-type */
	    sm_ref ref = ecl_new_reference_type_decl();
	    ref->node.reference_type_decl.name = gen_anon();
	    ref->node.reference_type_decl.cg_referenced_type = DR_ERR;
	    ref->node.reference_type_decl.sm_complex_referenced_type = arr;
	    ref->node.reference_type_decl.cg_referenced_size = -1;
	    f->node.field.sm_complex_type = ref;
	    return 1;
	}
	f->node.field.sm_complex_type = arr;
    }
    return 0;
}

static int
semanticize_array_element_node(context, array, super_type, base_type_spec, 
			       scope)
ecl_parse_context context;
sm_ref array;
sm_ref super_type;
sm_list base_type_spec;
scope_ptr scope;
{
    if (array->node.array_type_decl.size_expr != NULL) {
	if (semanticize_expr(context,
			     array->node.array_type_decl.size_expr, scope) == 0) {
	    return 0;
	}
    } else {
	sm_ref element_ref = array->node.array_type_decl.element_ref;
	/* allow NULL array type sizes */
	if (element_ref->node_type != ecl_declaration) {
	    ecl_src_error(context, element_ref, 
			  "Null sizes only allowed in parameter contexts");
	    return 0;
	}
	    
    }
    if (array->node.array_type_decl.element_ref->node_type 
	== ecl_declaration) {
	/* we're the last in line */
	sm_ref typ = NULL;
	int cg_type = DR_ERR;
	int ret;
	sm_ref decl = array->node.array_type_decl.element_ref;
	decl->node.declaration.sm_complex_type = super_type;
	decl->node.declaration.cg_type = DR_B;
	ret = semanticize_decl(context, decl, scope);
	if (ret == 0) return 0;

	if (decl->node.declaration.type_spec != NULL) {
	    typ = reduce_type_list(context, decl->node.declaration.type_spec,
				   &cg_type, scope, NULL);
	} else {
	    sm_ref arr = decl->node.declaration.sm_complex_type;
	    if ((arr != NULL) && 
		(arr->node_type == ecl_array_type_decl)) {
		typ == reduce_type_list(context, 
					arr->node.array_type_decl.type_spec, 
					&cg_type, scope, NULL);
	    }
	}
	if ((typ == NULL) && (cg_type == DR_ERR)) return 0;
	array->node.array_type_decl.cg_element_type = cg_type;
	array->node.array_type_decl.sm_complex_element_type = typ;
    } else {
	assert(array->node.array_type_decl.element_ref->node_type == ecl_array_type_decl);
	array->node.array_type_decl.sm_complex_element_type = array->node.array_type_decl.element_ref;
	return semanticize_array_element_node(context, 
					      array->node.array_type_decl.element_ref,
					      array,  
					      base_type_spec, scope);
    }
    return 1;
}	

static int
semanticize_array_type_node(context, array, scope)
ecl_parse_context context;
sm_ref array;
scope_ptr scope;
{
    return semanticize_array_element_node(context, array, array,  
					  array->node.array_type_decl.type_spec,
					  scope);
}

#define Max(i,j) ((i<j) ? j : i)

static int
semanticize_type_node(ecl_parse_context context, sm_ref decl, 
		      scope_ptr scope)
{
    int ret = 1;
    int struct_size = 0;
    sm_list fields = decl->node.struct_type_decl.fields;
    add_decl(decl->node.struct_type_decl.id, decl, scope);
    while(fields != NULL) {
	field *f = &fields->node->node.field;
	int err;
	int cg_type;
	sm_ref complex_type = NULL;
	int field_size = f->cg_size;

	if (f->string_type != NULL) {
	    /* PBIO-compatible field type */
	    cg_type = array_str_to_data_type(f->string_type, f->cg_size);
	    if (cg_type == DR_ERR) {
		char *tmp_str = strdup(f->string_type);
		char *rbracket = strchr(tmp_str, '[');
		if (rbracket != NULL) *rbracket = 0;
		complex_type = resolve(tmp_str, scope);
		if (complex_type == NULL) {
		    ecl_src_error(context, decl, 
				  "Field \"%s\" has unknown type \"%s\".",
				  f->name, tmp_str);
		    ret = 0;
		}
		if (rbracket != NULL) *rbracket = '[';
		free(tmp_str);	    
	    }
	    if (is_var_array(context, decl, fields->node,
			     decl->node.struct_type_decl.fields, 
			     f->cg_size, cg_type, complex_type, &err)) {
		field_size = sizeof(char*);
	    } else {
		if ((f->sm_complex_type != NULL) &&
		    (f->sm_complex_type->node_type == ecl_array_type_decl)) {
		    array_type_decl *arr =
			&f->sm_complex_type->node.array_type_decl;
		    field_size = arr->cg_element_size * arr->cg_static_size;
		} else {
		    /* not a var array, or a simple array */
		    f->sm_complex_type = complex_type;
		    f->cg_type = cg_type;
		}
	    }
	} else {
	    /* not able to get a PBIO-compatible form */
	    int type_def = 0;
	    int cg_type;
	    sm_ref typ;
	    typ = reduce_type_list(context, f->type_spec, &cg_type, scope, 
				   &type_def);
	    f->sm_complex_type = typ;
	    f->cg_type = cg_type;
	    field_size = -1;
	}
	if (err == 1) ret = 0;
	struct_size = Max(struct_size,
			  (f->cg_offset + field_size));
	fields = fields->next;
	    
    }
    decl->node.struct_type_decl.cg_size = struct_size;
    return ret;
}

static int
semanticize_reference_type_node(ecl_parse_context context, sm_ref decl, 
				scope_ptr scope)
{
    int ret = 1;
    int struct_size = 0;
    if (decl->node.reference_type_decl.name != NULL) {
	add_decl(decl->node.reference_type_decl.name, decl, scope);
    }
    decl->node.struct_type_decl.cg_size = struct_size;
    return ret;
}

extern sm_ref
ecl_build_param_node(const char *id, sm_ref typ, int param_num)
{
    sm_ref node = ecl_new_declaration();
    sm_ref ident;
    node->node.declaration.param_num = param_num;
    node->node.declaration.id = strdup(id);
    node->node.declaration.sm_complex_type = typ;
    if (typ != NULL) {
	ident = ecl_new_identifier();
	node->node.declaration.type_spec = malloc(sizeof(struct list_struct));
	node->node.declaration.type_spec->next = NULL;
	node->node.declaration.type_spec->node = ident;
	ident->node.identifier.id = strdup(typ->node.struct_type_decl.id);
    }
    return node;
}

extern void
ecl_add_param(const char *id, const char *typ, int param_num, 
	      ecl_parse_context context)
{
    sm_list type_list;
    sm_ref node;
    setup_for_string_parse(typ, context->defined_type_count, context->defined_types);
    ecl_code_string = typ;
    parsing_type = 1;
    yyerror_count = 0;
    yycontext = context;
    yyparse();
    parsing_type = 0;
    terminate_string_parse();

    if ((yyparse_value == NULL) || (yyerror_count != 0)) {
	return;
    }
    type_list = (sm_list) yyparse_value;

    node = ecl_build_param_node(id, NULL, param_num);
    node->node.declaration.type_spec = type_list;
    ecl_add_decl_to_parse_context(id, node, context);
}

extern void
ecl_subroutine_declaration(const char *decl, ecl_parse_context context)
{
    sm_list type_list, params;
    sm_ref complex_type, declaration;
    int cg_type, param_num;

    setup_for_string_parse(decl, context->defined_type_count, 
			   context->defined_types);
    ecl_code_string = decl;
    parsing_param_spec = 1;
    yyerror_count = 0;
    yycontext = context;
    yyparse();
    parsing_param_spec = 0;
    terminate_string_parse();

    if ((yyparse_value == NULL) || (yyerror_count != 0)) {
	return;
    }
    declaration = yyparse_value;
    type_list = declaration->node.declaration.type_spec;

    /* handle return type */
    complex_type = reduce_type_list(context, type_list, &cg_type, context->scope, NULL);
    context->return_type_list = type_list;
    if (complex_type != NULL) {
	cg_type = DR_P;
    }
    context->return_cg_type = cg_type;


    /* handle params */
    params = declaration->node.declaration.params;
    param_num = 0;
    while (params != NULL) {
	sm_ref param = NULL;
	switch (params->node->node_type) {
	case ecl_declaration:
	    param = params->node;
	    break;
	case ecl_array_type_decl:
	    param = params->node->node.array_type_decl.element_ref;
	    param->node.declaration.sm_complex_type = params->node;
	    break;
	default:
	    printf("unhandled case in ecl_subroutine_declaration\n");
	}
	param->node.declaration.param_num = param_num;
	ecl_add_decl_to_parse_context(param->node.declaration.id,
				      params->node, context);
	param_num++;
	params = params->next;
    }
}

extern void
ecl_set_return_type(char *typ, ecl_parse_context context)
{
    sm_list type_list;
    int cg_type;
    sm_ref complex_type;
    setup_for_string_parse(typ, context->defined_type_count, context->defined_types);
    ecl_code_string = typ;
    parsing_type = 1;
    yyerror_count = 0;
    yycontext = context;
    yyparse();
    parsing_type = 0;
    terminate_string_parse();

    if ((yyparse_value == NULL) || (yyerror_count != 0)) {
	return;
    }
    type_list = (sm_list) yyparse_value;

    complex_type = reduce_type_list(context, type_list, &cg_type, context->scope, NULL);
    context->return_type_list = type_list;
    if (complex_type != NULL) {
	cg_type = DR_P;
    }
    context->return_cg_type = cg_type;
}

static sm_ref
find_complex_type(sm_ref node, scope_ptr scope)
{
    assert(node->node_type == ecl_identifier);
    return resolve(node->node.identifier.id, scope);
}

extern ecl_parse_context
new_ecl_parse_context()
{
    ecl_parse_context context = malloc(sizeof(struct parse_struct));
    context->decls = NULL;
    context->defined_type_count = 0;
    context->defined_types = NULL;
    context->error_func = default_error_out;
    context->client_data = NULL;
    context->return_type_list = NULL;
    context->return_cg_type = DR_I;
    context->scope = push_scope(NULL);
    context->has_exec_context = 0;
    ecl_add_standard_elements(context);
    return context;
}

extern void
ecl_free_parse_context(ecl_parse_context parse_context)
{
    pop_scope(parse_context->scope);
    if (parse_context->defined_types) {
	free(parse_context->defined_types);
    }
    if (parse_context->decls) {
	ecl_rfree_list(parse_context->decls, NULL);
    }
    if (parse_context->standard_decls) {
	ecl_rfree_list(parse_context->standard_decls, NULL);
    }
    free(parse_context);
}

extern void
ecl_assoc_externs(ecl_parse_context context, ecl_extern_list externs)
{
    context->scope->externs = externs;
}

extern void
ecl_add_decl_to_parse_context(const char *name, sm_ref item, ecl_parse_context context)
{
    sm_list *last_ptr = &context->decls;
    sm_list list = context->decls;
    while (list != NULL) {
	last_ptr = &list->next;
	list = list->next;
    }
    *last_ptr = malloc(sizeof(*list));
    (*last_ptr)->next = NULL;
    (*last_ptr)->node = item;
    if (item->node_type == ecl_struct_type_decl) {
	ecl_add_defined_type((char *)name, context);
    }
}

static void
space_to_underscore(char *str){
    while(*str != '\0'){
	if(isspace(*str))
	    *str = '_';
	    str++;
    }
}

static void
purify_name(IOFormatList list){
    int i,j;
    for(i=0; list[i].format_name; i++){
	IOFieldList fl = list[i].field_list;
	space_to_underscore(list[i].format_name);
	for(j=0; fl[j].field_name; j++){
	    space_to_underscore(fl[j].field_name);
	    space_to_underscore(fl[j].field_type);
	}
    }
}

static void
uniqueify_names(IOFormatList list, char *prefix)
{
    int i = 0;
    int prefix_len = strlen(prefix);
    while (list[i].format_name != NULL) {
	int j = 0;
	IOFieldList fl = list[i].field_list;
	char *new_name =
	    malloc(strlen(list[i].format_name) + prefix_len + 1);
	strcpy(new_name, prefix);
	strcpy(new_name + prefix_len, list[i].format_name);
	free(list[i].format_name);
	list[i].format_name = new_name;
	while (fl[j].field_name != 0) {
	    int field_type_len = strlen(fl[j].field_type);
	    char *bracket = strchr(fl[j].field_type, '[');
	    int k;
	    if (bracket != NULL) {
		field_type_len = (long) bracket - (long) fl[j].field_type;
	    }
	    for (k = 0; k < i; k++) {
		char *new_type;
		if (strncmp
		    (fl[j].field_type, list[k].format_name + prefix_len,
		     field_type_len) != 0) {
		    /* don't match in N chars */
		    continue;
		}
		if ((list[k].format_name + prefix_len)[field_type_len] !=
		    0) {
		    /* list name is longer */
		    continue;
		}
		new_type =
		    malloc(strlen(fl[j].field_type) + prefix_len + 1);
		strcpy(new_type, prefix);
		strcpy(new_type + prefix_len, fl[j].field_type);
		free((void *) fl[j].field_type);
		fl[j].field_type = new_type;
		break;
	    }
	    j++;
	}
	i++;
    }
    purify_name(list);
}

/* Returns the ecode function which will do message format conversion */
extern ecl_code
gen_rollback_code(IOFormatList format1, IOFormatList format2, char *xform_code)
{
    /* setup ECL generation */
    /* 
     *  NOTE:  We have to make the type names (structure names)
     *  in format1 and format2 to be unique. 
     *  Because of the nature of the problem, they are likely to be 
     *  identical and this may cause namespace collisions in ECL.
     */
    ecl_code code;
    ecl_parse_context parse_context = new_ecl_parse_context();

    int i = 0;
    uniqueify_names(format1, "f0_");
    while (format1[i].format_name != NULL) {
	ecl_add_struct_type(format1[i].format_name,
			    format1[i].field_list, parse_context);
	i++;
    }
    ecl_add_param("new", format1[i - 1].format_name, 0, parse_context);

    i = 0;
    uniqueify_names(format2, "f1_");
    while (format2[i].format_name != NULL) {
	ecl_add_struct_type(format2[i].format_name,
			    format2[i].field_list, parse_context);
	i++;
    }
    ecl_add_param("old", format2[i - 1].format_name, 1, parse_context);

    code = ecl_code_gen(xform_code, parse_context);
    ecl_free_parse_context(parse_context);

    /* 
     * the "code" structure is the only output from this block,
     * all else is free'd.
     */
    return code;
}
