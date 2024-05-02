#include "config.h"
#include "ecl.h"
#include "malloc.h"
#include "assert.h"
#include <string.h>
#include <stdio.h>
#ifndef HAVE_ATL_H
int
main()
{return 0;}
#else
#include "atl.h"

static int verbose = 0;

int
main(int argc, char **argv)
{
    char *test_only = NULL;
    int i = 1;
    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-v") == 0) {
	    verbose++;
	} else if (strcmp(argv[i], "-test_only") == 0){
	    test_only = argv[++i];
	} else {
	    printf("Unknown argument %s\n", argv[i]);
	}
    }
    if (!test_only || (strcmp(test_only, "paramattr") == 0)) {
	static char extern_string[] = "int printf(string format, ...);";

#ifndef PRINTF_DEFINED
	extern int printf();
#endif
	static ecl_extern_entry externs[] = 
	{
	    {"printf", (void*)(long)printf},
	    {(void*)0, (void*)0}
	};
	/* test external call */
	static char code[] = "{\
			if (attr_set(l, \"test_value\")) {\n\
				return attr_ivalue(l, \"test_value\");\n\
			}\n\
			return 0;\n\
		}";

	ecl_parse_context context = new_ecl_parse_context();
	ecl_parse_context context2;

	ecl_code gen_code;
	int (*func)(attr_list l);
	attr_list l;

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	context2 = ecl_copy_context(context);

	set_attr_atom_and_string("test_value", 
				 ATL_CHAR_CONS('A','T','t','v'));
	set_attr_atom_and_string("test_value", 
				 ATL_CHAR_CONS('A','T','t','v'));
	l = create_attr_list();
	set_attr(l, ATL_CHAR_CONS('A','T','t','v'), Attr_Int4, (attr_value)15);

	ecl_subroutine_declaration("int proc(attr_list l)", context);
	gen_code = ecl_code_gen(code, context);
	func = (int (*)(attr_list))(long)gen_code->func;

	if ((func)(l) != 15) {
	    printf("Function didn't return 15\n");
	}
	
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);

	ecl_subroutine_declaration("int proc(attr_list l)", context2);
	gen_code = ecl_code_gen(code, context2);
	func = (int (*)(attr_list))(long)gen_code->func;

	if ((func)(l) != 15) {
	    printf("Function didn't return 15\n");
	}
	free_attr_list(l);
	ecl_code_free(gen_code);
	ecl_free_parse_context(context2);
    }
    if (!test_only || (strcmp(test_only, "globalattr") == 0)) {
	static char extern_string[] = "int printf(string format, ...);\n\
				       attr_list l;";

#ifndef PRINTF_DEFINED
	extern int printf();
#endif
	static ecl_extern_entry externs[] = 
	{
	    {"printf", (void*)(long)printf},
	    {"l", (void*) 0},
	    {(void*)0, (void*)0}
	};
	/* test external call */
	static char code[] = "{\
			if (attr_set(l, \"test_value\")) {\n\
				return attr_ivalue(l, \"test_value\");\n\
			}\n\
			return 0;\n\
		}";

	ecl_parse_context context = new_ecl_parse_context();
	ecl_parse_context context2;

	ecl_code gen_code;
	int (*func)(attr_list l);
	attr_list l = create_attr_list();
	
	externs[1].extern_value = &l;

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	context2 = ecl_copy_context(context);

	set_attr_atom_and_string("test_value", 
				 ATL_CHAR_CONS('A','T','t','v'));
	set_attr_atom_and_string("test_value", 
				 ATL_CHAR_CONS('A','T','t','v'));
	set_attr(l, ATL_CHAR_CONS('A','T','t','v'), Attr_Int4, (attr_value)15);

	ecl_subroutine_declaration("int proc()", context);
	gen_code = ecl_code_gen(code, context);
	func = (int (*)(attr_list))(long)gen_code->func;

	if ((func)(l) != 15) {
	    printf("Function didn't return 15\n");
	}
	
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);

    }
    if (!test_only || (strcmp(test_only, "attrarray") == 0)) {
	static char extern_string[] = "int printf(string format, ...);";

#ifndef PRINTF_DEFINED
	extern int printf();
#endif
	static ecl_extern_entry externs[] = 
	{
	    {"printf", (void*)(long)printf},
	    {(void*)0, (void*)0}
	};
	/* test external call */
	static char code[] = "{\
			if (attr_set(l[i], \"test_value\")) {\n\
				return attr_ivalue(l[i], \"test_value\");\n\
			}\n\
			return 0;\n\
		}";

	ecl_parse_context context = new_ecl_parse_context();

	ecl_code gen_code;
	int (*func)(attr_list *l, int i);
	attr_list l[2];

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	ecl_subroutine_declaration("int proc(attr_list l[], int i)", context);
	gen_code = ecl_code_gen(code, context);
	func = (int (*)(attr_list*, int))(long)gen_code->func;

	set_attr_atom_and_string("test_value", 
				 ATL_CHAR_CONS('A','T','t','v'));
	l[0] = create_attr_list();
	set_attr(l[0], ATL_CHAR_CONS('A','T','t','v'), Attr_Int4, (attr_value)15);
	l[1] = create_attr_list();
	set_attr(l[1], ATL_CHAR_CONS('A','T','t','v'), Attr_Int4, (attr_value)25);

	if ((func)(&l[0], 0) != 15) {
	    printf("Function didn't return 15\n");
	}
	
	if ((func)(&l[0], 1) != 25) {
	    printf("Function didn't return 25\n");
	}
	
	free_attr_list(l[0]);
	free_attr_list(l[1]);
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    if (!test_only || (strcmp(test_only, "timer") == 0)) {
	static char extern_string[] = "int printf(string format, ...);";

#ifndef PRINTF_DEFINED
	extern int printf();
#endif
	static ecl_extern_entry externs[] = 
	{
	    {"printf", (void*)(long)printf},
	    {(void*)0, (void*)0}
	};
	/* test external call */
	static char code[] = "{\
			chr_time timer;\n\
			int i;\n\
			int tmp = 0;\n\
			chr_timer_start(&timer);\n\
			for(i=0; i < limit; i++) {\n\
				tmp++;\n\
			}\n\
			chr_timer_stop(&timer);\n\
			return chr_time_to_millisecs(&timer);\n\
		}";

	ecl_parse_context context = new_ecl_parse_context();

	ecl_code gen_code;
	double (*func)(int);
	double big, little;

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	ecl_subroutine_declaration("double proc(int limit)", context);
	gen_code = ecl_code_gen(code, context);
	func = (double (*)(int))(long)gen_code->func;

	big = (func)(200000);
	little = (func)(2000);

	if (verbose) {
	    printf("200K autoincrements took %g milliseconds\n", big);
	    printf("2000 autoincrements took %g milliseconds\n", little);
	}

	if (((big / 100.0) < (.5 *little)) || 
	    ((big / 100.0) > (2.0 *little))) {
	    printf("chr_timer_test values inconsistent:  big, little out of range, %g, %g\n",
		   big, little);
	}
	
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    return 0;
}
#endif
