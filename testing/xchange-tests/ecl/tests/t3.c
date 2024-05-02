#include "config.h"
#include "ecl.h"
#include "assert.h"
#include <stdio.h>

static double testd(){return 1.0;}

int
main()
{
    {
	/* test the basics */
	char code_string[] = "\
{\n\
    static int j = 4;\n\
    static long k = 10;\n\
    static short l = 3;\n\
    static int m = 0;\n\
\n\
    j = j + 1;\n\
    k = k + 2;\n\
    l = l + 3;\n\
\n\
    return j + k + l + m;\n\
}";

	ecl_parse_context context = new_ecl_parse_context();
	ecl_code gen_code;
	long (*func)();

	gen_code = ecl_code_gen(code_string, context);
	func = (long(*)()) (long) gen_code->func;
	assert(func() == 23);
	assert(func() == 29);
	assert(func() == 35);
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }

    {

	/* test the ability to have a parameter */
	char code_string[] = "\
{\n\
    static int j = 4;\n\
    static long k = 10;\n\
    static short l = 3;\n\
\n\
    return l * (j + k + i);\n\
}";

	ecl_parse_context context = new_ecl_parse_context();
	ecl_code gen_code;
    	long (*func)(int i);

	ecl_subroutine_declaration("int proc(int i)", context);
	gen_code = ecl_code_gen(code_string, context);
	func = (long(*)(int)) (long) gen_code->func;
        assert(func(15) == 87);
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }

    {
	/* structured types */
	char code_string[] = "\
{\n\
    return input.l * (input.j + input.k + input.i);\n\
}";

	typedef struct test {
	    int i;
	    int j;
	    long k;
	    short l;
	} test_struct, *test_struct_p;

	IOField struct_fields[] = {
	    {"i", "integer", sizeof(int), IOOffset(test_struct_p, i)},
	    {"j", "integer", sizeof(int), IOOffset(test_struct_p, j)},
	    {"k", "integer", sizeof(long), IOOffset(test_struct_p, k)},
	    {"l", "integer", sizeof(short), IOOffset(test_struct_p, l)},
	    {(void*)0, (void*)0, 0, 0}};

	ecl_parse_context context = new_ecl_parse_context();
	test_struct str;
	ecl_code gen_code;
	long (*func)(test_struct_p s);

	ecl_add_struct_type("struct_type", struct_fields, context);
	ecl_subroutine_declaration("int proc(struct_type *input)", context);

	gen_code = ecl_code_gen(code_string, context);
	func = (long(*)(test_struct_p)) (long) gen_code->func;

	str.i = 15;
	str.j = 4;
	str.k = 10;
	str.l = 3;
	assert(func(&str) == 87);
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    {
	static char extern_string[] = "int printf(string format, ...);";
	static ecl_extern_entry externs[] = 
	{
	    {"printf", (void*)(long)printf},
	    {(void*)0, (void*)0}
	};
	static char code[] = "{\
		    int i;\
		    int j;\
		    double sum = 0.0;\
		    double average = 0.0;\
		    for(i = 0; i<37; i= i+1) {\
		        for(j = 0; j<253; j=j+1) {\
			sum = sum + input.levels[j][i];\
		        }\
		    }\
		    average = sum / (37 * 253);\
		    return average;\
		}";

	static IOField input_field_list[] =
	{
	    {"levels", "float[253][37]", sizeof(double), 0},
	    {(void*)0, (void*)0, 0, 0}
	};

	ecl_parse_context context = new_ecl_parse_context();
	int i, j;
	double levels[253][37], result;
	ecl_code gen_code;
	double (*func)(double*);

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	ecl_add_struct_type("input_type", input_field_list, context);
	ecl_subroutine_declaration("double proc(input_type *input)", context);

	for(i=0; i< 253; i++) {
	    for (j=0; j< 37; j++) {
	        levels[i][j] = i + 1000*j;
	    }
	}

	gen_code = ecl_code_gen(code, context);
	func = (double (*)(double*))(long) gen_code->func;
	result = (func)(&levels[0][0]);
	if (result != 18126.00) {
	    printf("Expected %g, got %g\n", 18126.0, result);
	}
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }
    {
	static char extern_string[] = "int printf(string format, ...);\
					double testd();";
	static ecl_extern_entry externs[] = 
	{
	    {"testd", (void*)(long)testd},
	    {"printf", (void*)(long)printf},
	    {(void*)0, (void*)0}
	};
	static char code[] = "{\
				   double b = testd();\n\
				   return b;\
		}";

	ecl_parse_context context = new_ecl_parse_context();
	int i, j;
	double levels[253][37], result;
	ecl_code gen_code;
	double (*func)();

	ecl_assoc_externs(context, externs);
	ecl_parse_for_context(extern_string, context);

	ecl_subroutine_declaration("double proc()", context);

	gen_code = ecl_code_gen(code, context);
	func = (double (*)())(long) gen_code->func;
	result = (func)();
	if (result != 1.0) {
	    printf("Expected %g, got %g\n", 1.0, result);
	}
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
    }

    return 0;
}
