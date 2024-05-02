
#include <stdio.h>
#include <rpc/types.h>

int main(int argc, char *argv[]) {

	printf("sizeof(void *) = %d\n", sizeof(void *));
	printf("sizeof(u_char)=%d\n", sizeof(u_char));
	printf("sizeof(u_short)=%d\n", sizeof(u_short));
	printf("sizeof(u_int)=%d\n", sizeof(u_int));
	printf("sizeof(u_long)=%d\n", sizeof(u_long));
	printf("sizeof(quad_t)=%d\n", sizeof(quad_t));
	printf("sizeof(u_quad_t)=%d\n", sizeof(u_quad_t));
	printf("sizeof(fsid_t)=%d\n", sizeof(fsid_t));
	printf("sizeof(daddr_t)=%d\n", sizeof(daddr_t));
	printf("sizeof(caddr_t)=%d\n", sizeof(caddr_t));
}
