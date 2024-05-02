#include "config.h"
#include "ecl.h"
#include "malloc.h"
#include "assert.h"
#include <stdio.h>

int
main()
{
    {
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
			printf(\"values are is %d, %g, %s\\n\", i, d, s);\
		}";

	ecl_parse_context context = new_ecl_parse_context();

	ecl_code gen_code;
	void (*func)(int, double, char*);

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	ecl_subroutine_declaration("void proc(int i, double d, string s)", 
				   context);
	gen_code = ecl_code_gen(code, context);
	func = (void (*)(int, double, char*))(long)gen_code->func;
	printf("Expect -> \"values are is %d, %g, %s\"\n", 5, 3.14159, "hello!");
	(func)(5, (double)3.14159, "hello!");
	
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    {
	typedef struct test {
	    int count;
	    double *vals;
	} test_struct, *test_struct_p;

	static char code[] = "{\
		    int i;\n\
		    double sum = 0.0;\n\
		    for(i = 0; i<input.count; i= i+1) {\n\
			sum = sum + input.vals[i];\n\
		    }\n\
/* comment */\n\
		    return sum;\n\
		}";

	static IOField input_field_list[] =
	{
	    {"count", "integer", sizeof(int), 
	     IOOffset(test_struct_p, count)},
	    {"vals", "float[count]", sizeof(double), 
	     IOOffset(test_struct_p, vals)},
	    {(void*)0, (void*)0, 0, 0}
	};

	ecl_parse_context context = new_ecl_parse_context();
	int i;
	test_struct tmp;
	ecl_code gen_code;
	double (*func)(test_struct_p);

	ecl_add_struct_type("input_type", input_field_list, context);
	ecl_subroutine_declaration("double proc(input_type *input)", context);
	tmp.count = 10;
	tmp.vals = (double*) malloc(tmp.count * sizeof(double));
	for(i=0; i< tmp.count; i++) {
	    tmp.vals[i] = i + 0.1;
	}

	gen_code = ecl_code_gen(code, context);
	func = (double (*)(test_struct_p))(long) gen_code->func;
	assert((func)(&tmp) == 46.00);
	free(tmp.vals);
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    return 0;
}
