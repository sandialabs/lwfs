
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char **argv)
{
	char *ptr, **pptr; 
	struct hostent *hptr; 
	char *hostname = argv[1]; 
	char str[INET6_ADDRSTRLEN];

	hptr = gethostbyname(hostname); 
	if (hptr == NULL) {
		fprintf(stderr, "could not get host address for \"%s\"\n", hostname); 
		return -1; 
	}

	printf("official hostname = %s\n", hptr->h_name);

	switch (hptr->h_addrtype) {

		case AF_INET:
		case AF_INET6:
			pptr = hptr->h_addr_list;
			for ( ; *pptr != NULL; pptr++) {
				struct in_addr *addr; 

				printf("\taddress: %s\n",
					inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));

				addr = (struct in_addr *)(*pptr); 

				printf("\tnid: %llu\n",htonl(addr->s_addr)); 
			}
			break;
		default:
			fprintf(stderr, "unknown address type\n");
			break;
	}
}
