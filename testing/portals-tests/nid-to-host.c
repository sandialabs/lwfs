
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
	char hostname[256]; 
	char str[INET6_ADDRSTRLEN];
	unsigned long long nid = (unsigned long long)atoll(argv[1]); 

	struct in_addr addr;

	addr.s_addr = htonl(nid); 

	hptr = gethostbyaddr(&addr, sizeof(addr), AF_INET); 

	if (hptr == NULL) {
		fprintf(stderr, "could not get host address for \"%llu\"\n", nid); 
		return -1; 
	}

	printf("official hostname = %s\n", hptr->h_name);

	switch (hptr->h_addrtype) {

		case AF_INET:
		case AF_INET6:
			pptr = hptr->h_addr_list;
			for ( ; *pptr != NULL; pptr++) {
				struct in_addr *host_addr; 

				printf("\taddress: %s\n",
					inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));

				host_addr = (struct in_addr *)(*pptr); 

				printf("\tnid: %llu\n",htonl(host_addr->s_addr)); 
			}
			break;
		default:
			fprintf(stderr, "unknown address type\n");
			break;
	}
}
