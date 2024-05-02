#include "config.h"
#include "ecl.h"
#include "assert.h"
#include <stdio.h>
char code_string[] = "{return sizeof(int);}";
char code_string2[] = "{return sizeof(int*);}";
char code_string3[] = "{   int j;      return sizeof j;}";

int
main()
{
    char *code_blocks[] = {code_string, code_string2, code_string3, NULL};
    int results[] = {sizeof(int), sizeof(int*), sizeof(int)};
    int test = 0;
	
    while (code_blocks[test] != NULL) {
	int ret;
	ecl_parse_context context = new_ecl_parse_context();

	ecl_code gen_code;
	int (*func)();

	gen_code = ecl_code_gen(code_blocks[test], context);
	func = (int(*)())gen_code->func;
	ret = (func)();
	if (ret != results[test]) {
	    printf("bad test %d, ret was %d\n", test, ret);
	}
	ecl_code_free(gen_code);
	ecl_free_parse_context(context);
	test++;
    }
    return 0;
}
