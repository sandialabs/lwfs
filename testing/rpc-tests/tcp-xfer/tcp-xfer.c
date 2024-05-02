/**
 * @file tcp-xfer.c
 *
 * @brief Implementation of client/server code that uses TCP sockets 
 * to receive buffers of data. 
 *
 * @author Ron A. Oldfield (raoldfi\@sandia.gov).
 * $Revision: 1073 $. 
 * $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $.
 */

#include <assert.h>
#include <math.h>
#include <time.h>
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "support/timer/timer.h"

typedef struct  {
	int int_val;
	float float_val;
	double double_val;
} data_t; 


int exit_now = 0; 


/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1073 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS storage service";
static char args_doc[] = "HOSTNAME"; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief A flag that identifies a server. */
	int server; 

	/** @brief the size of the input/output buffer to use for tests. */
	int len; 

	/** @brief The number of experiments to run. */
	int count; 

	/** @brief The number of experiments to run. */
	int num_reqs; 

	/** @brief Where to put results */
	char *result_file; 

	/** @brief Where to put results */
	char *result_file_mode; 

	/** @brief Hostname of the remote server. */
	char *host; 
}; 

static int print_args(FILE *fp, const char *prefix, struct arguments *args) 
{
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tMeasure Raw TCP/IP throughput\n", prefix);
	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	if (!args->server) {
		fprintf(fp, "%s \t--host = \"%s\"\n", prefix, args->host);
		fprintf(fp, "%s \t--len = %d\n", prefix, args->len);
		fprintf(fp, "%s \t--count = %d\n", prefix, args->count);
		fprintf(fp, "%s \t--num-reqs = %d\n", prefix, args->num_reqs);
		fprintf(fp, "%s \t--result-file = \"%s\"\n", prefix, args->result_file);
		fprintf(fp, "%s \t--result-file-mode = \"%s\"\n", prefix, args->result_file_mode);
	}
	else {
		fprintf(fp, "%s \t--server = %s\n", prefix, (args->server)? "TRUE" : "FALSE");
		fprintf(fp, "%s \t--count = %d\n", prefix, args->count);
	}
	fprintf(fp, "%s -----------------------------------\n", prefix);

	return 0;
}


static struct argp_option options[] = {
	{"num-reqs",    1000, "<val>", 0, "number of requests/trial"},
	{"len",    2000, "<val>", 0, "number of array elements to send for each exp"},
	{"count",      3000, "<val>", 0, "Number of experiments to run"},
	{"server",      4000, 0, 0, "Run as a server"},
	{"result-file",      5000, "<FILE>", 0, "Where to put results"},
	{"result-file-mode",      6000, "<val>", 0, "Result file mode"},
	{ 0 }
};

/** 
 * @brief Parse a command-line argument. 
 * 
 * This function is called by the argp library. 
 */
static error_t parse_opt(
		int key, 
		char *arg, 
		struct argp_state *state)
{
	/* get the input arguments from argp_parse, which points to 
	 * our arguments structure */
	struct arguments *arguments = state->input; 

	switch (key) {
		case 1000: /* num_reqs */
			arguments->num_reqs = atoi(arg);
			break;
			

		case 2000: /* len */
			arguments->len = atoi(arg);
			break;

		case 3000: /* count */
			arguments->count = atoi(arg);
			break;

		case 4000: /* server flag */
			arguments->server = 1;
			break;

		case 5000: /* server flag */
			arguments->result_file = arg;
			break;

		case 6000: /* mode for result file */
			arguments->result_file_mode = arg;
			break;


		case ARGP_KEY_ARG:
			/* we expect only one argument */
			if (state->arg_num >= 1) {
				argp_usage(state);
			}
			//arguments->args[state->arg_num] = arg; 
			arguments->host = arg; 
			break; 

		case ARGP_KEY_END:
			if (state->arg_num < 0)
				argp_usage(state);
			break;

		default:
			return ARGP_ERR_UNKNOWN; 
	}

	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc}; 


/*----------------------------------------------------------*/



static int write_len(int connfd, int len)
{
	int nbytes; 

	/* get the number of elements the client will send */
	nbytes = write(connfd, &len, sizeof(int));
	if (nbytes != sizeof(int)) {
		perror("error writing the array size");
		return -1;
	}

	return 0; 
}

static int read_len(int connfd, int *len)
{
	int nbytes; 

	/* get the number of elements the client will send */
	nbytes = read(connfd, len, sizeof(int));
	if (nbytes != sizeof(int)) {
		perror("error reading the array size");
		return -1;
	}

	return 0; 
}

static int write_buf(int connfd, data_t *buf, int len)
{
	size_t nleft;
	ssize_t nwritten; 
	char *ptr; 

	int n = len*sizeof(data_t); 
	nleft = n; 
	ptr = (char *)buf; 

	while (nleft > 0) {
		if ((nwritten = write(connfd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nwritten = 0; /* call write() again */
			else 
				return -1;
		} else if (nwritten == 0)
			break;             /* EOF */

		nleft -= nwritten; 
		ptr += nwritten; 
	}

	return 0; 
}


static int read_buf(int connfd, data_t *buf, int len)
{
	size_t nleft;
	ssize_t nread; 
	char *ptr; 

	int n = len*sizeof(data_t); 
	nleft = n; 
	ptr = (char *)buf; 

	while (nleft > 0) {
		if ((nread = read(connfd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0; /* call read again */
			else 
				return -1;
		} else if (nread == 0)
			break;             /* EOF */

		nleft -= nread; 
		ptr += nread; 
	}

	return 0; 
}


static int xfer_srvr(int server_port, int count)
{
	int numreqs = 0;
	int on=1;
	int sbsz = 65536; 
	int listenq = 1024; 
	int listenfd=0, connfd=0; 
	int rc; 
	socklen_t client_addr_len; 
	int len; 
	data_t *buf;
	struct sockaddr_in client_addr, server_addr; 

	static struct linger linger = {0,0}; 

	//struct sockaddr client_addr, server_addr; 

	/* create a socket for the listener */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		perror("error creating socket");
		return -1; 
	}

	/* initialize the server address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);
	
	setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &sbsz, sizeof(sbsz));
	setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &sbsz, sizeof(sbsz));
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); 
	setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)); 


	/* bind the server address to the listener socket */
	rc = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rc == -1) {
		perror("error binding socket");
		goto cleanup;
	}

	numreqs=0; 

	while (1) {
		/* listen for a connection */
		rc = listen(listenfd, listenq);
		if (rc == -1) {
			perror("error listening for connection");
			goto cleanup;
		}

		if (exit_now) {
			break;
		}

		/* accept a connection from the client  */
		client_addr_len = sizeof(client_addr); 
		connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (connfd == -1) {
			perror("error accepting connection");
			goto cleanup;
		}

		setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
		setsockopt(connfd, IPPROTO_TCP, SO_LINGER, &linger, sizeof(linger));
		
		
		/* get the length of the incoming buffer */
		rc = read_len(connfd, &len); 
		if (rc == -1) {
			perror("could not read length");
			goto cleanup;
		}

		//fprintf(stderr, "allocating space for %d elements\n",
		//	len);

		/* allocate space for the incoming buffer */
		buf = (data_t *)malloc(len*sizeof(data_t));
		if (buf == NULL) {
			perror("out of memory");
			goto cleanup;
		}

		/* read the buffer */
		rc = read_buf(connfd, buf, len); 
		if (rc == -1) {
			perror("could not read length");
			goto cleanup;
		}

		/* write a single structure back to the client */
		rc = write_buf(connfd, &buf[len-1], 1);
		if (rc == -1) {
			perror("could not send result");
			goto cleanup;
		}

		/* free the buffer */
		free(buf);


		/* increment the number of requests */
		numreqs++; 
		//fprintf(stderr, "srvr finished req=%d\n", numreqs);

		if ((count >0) && (numreqs >= count)) {
			break;
		}


		/* close the connection from the client */
		shutdown(connfd, 2);

	}

cleanup:
	shutdown(connfd, 2);
	shutdown(listenfd, 2);
	fprintf(stderr, "srvr exiting!\n");
    return 0;
}

static int xfer_clnt(const char *server, int server_port, data_t *buf, int len)
{
	int sbsz = 65536; 

	int rc = 0; 
	int sockfd; 
	struct sockaddr_in server_addr; 
	data_t result; 
	struct hostent *server_ent; 

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("error creating socket");
		return -1; 
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sbsz, sizeof(sbsz)) ||
	    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sbsz, sizeof(sbsz))) {
		fprintf(stderr, "setsockopt: %s\n", strerror(errno));
		shutdown(sockfd, 2); 
		return rc; 
	}


	/* get the host entry for the server */
	server_ent = gethostbyname(server); 

	/* initialize the server address */
	memset(&server_addr, 0, sizeof(server_addr)); 
	/*
	   server_addr.sin_family = AF_INET;
	   server_addr.sin_port = htons(server_port);
	 */

	server_addr.sin_family = server_ent->h_addrtype;
	server_addr.sin_port = htons(server_port);

	memcpy(&server_addr.sin_addr, server_ent->h_addr_list[0],
			sizeof(server_addr.sin_addr));

	/* convert the server name to a binary address */
	/*
	   rc = inet_pton(AF_INET, server, &server_addr.sin_addr);
	   if (rc == -1) {
	   perror("error converting server address");
	   return -1;
	   }
	 */



	/* connect to the server */
	rc = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rc == -1) {
		perror("failed to connect to server on ");
		return -1; 
	}

	/* write the size of the buffer */
	rc = write_len(sockfd, len); 
	if (rc == -1) {
		perror("failed to write array length");
		return -1;
	}

	/* write the buffer */
	rc = write_buf(sockfd, buf, len);
	if (rc == -1) {
		perror("failed to write buffer");
		return -1;
	}


	/* read the result */
	rc = read_buf(sockfd, &result, 1); 
	if (rc == -1) {
		perror("failed to read result");
		return -1;
	}

	shutdown(sockfd, 2); 
	return rc; 
}


static void output_stats(
		FILE *result_fp,
		struct arguments *args,
		double t_total)
{
	double t_total_max = t_total;
	double t_total_min = t_total;
	double t_total_sum = t_total;
	int total_reqs = args->num_reqs;
	int myrank=0; 
	int np=1;
	static int first=1; 

	if (myrank == 0) {
		double t_total_avg = t_total_sum/np;
		double nbytes = total_reqs*(sizeof(int) + (args->len+1)*sizeof(data_t));

		if (first) {
			time_t rawtime;
			time(&rawtime);

			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			fprintf(result_fp, "%s rpc-xfer\n", "%");
			fprintf(result_fp, "%s %s", "%", ctime(&rawtime));
			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			fprintf(result_fp, "%s column   description\n", "%");
			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			fprintf(result_fp, "%s   0     number of clients\n", "%");
			fprintf(result_fp, "%s   1     total ops\n","%");
			fprintf(result_fp, "%s   2     data structures/op\n", "%");
			fprintf(result_fp, "%s   3     aggregate bytes\n","%");
			fprintf(result_fp, "%s   4     min time (sec)\n","%");
			fprintf(result_fp, "%s   5     max time (sec)\n","%");
			fprintf(result_fp, "%s   6     avg time (sec)\n","%");
			fprintf(result_fp, "%s   7     min throughput (MB/sec)\n","%");
			fprintf(result_fp, "%s   8     max throughput (MB/sec)\n","%");
			fprintf(result_fp, "%s   9     avg throughput (MB/sec)\n","%");
			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			first = 0; 
		}

		/* print the row */
		fprintf(result_fp, "%04d   ", np);    
		fprintf(result_fp, "%09d   ", total_reqs); 
		fprintf(result_fp, "%09d   ", args->len);    
		fprintf(result_fp, "%1.6e  ", nbytes); 
		fprintf(result_fp, "%1.6e  ", t_total_min); 
		fprintf(result_fp, "%1.6e  ", t_total_max); 
		fprintf(result_fp, "%1.6e  ", t_total_avg); 
		fprintf(result_fp, "%1.6e  ", nbytes/(t_total_max*1024*1024)); 
		fprintf(result_fp, "%1.6e  ", nbytes/(t_total_min*1024*1024)); 
		fprintf(result_fp, "%1.6e\n", nbytes/(t_total_avg*1024*1024)); 
		fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
		fflush(result_fp);
	}
}






/**
*
*  The TCP server receives the size of the incoming array, allocates 
*  the required memory for the buffer, receives the array, and sends 
*  a single 16-byte data_t object back to the client. 
*/
int main(int argc, char **argv)
{
	struct arguments args; 
	int rc = 0; 
	int server_port = 9877;
	FILE *result_fp = stdout; 

	/* default values for the arguments */
	args.count = 1; 
	args.len = 1;
	args.num_reqs = 1; 
	args.host = "localhost";
	args.server = 0;  /* run as client by default */
	args.result_file = NULL;  /* run as client by default */
	args.result_file_mode = "w";  /* run as client by default */


	/* install the signal handler */
	//signal(SIGINT, sighandler); 

	argp_parse(&argp, argc, argv, 0, 0, &args); 

	if (args.result_file != NULL) {
		result_fp = fopen(args.result_file, args.result_file_mode);
		if (result_fp == NULL) {
			fprintf(stderr, "could not open %s: using stdout\n",
					args.result_file);
			result_fp = stdout;
		}
	}


	if (args.server) {
		print_args(stdout, "", &args); 
		rc = xfer_srvr(server_port, args.count);
	}
	else {
		int i; 
		data_t *buf; 

		print_args(result_fp, "%", &args); 

		/* allocate buffer */
		buf = (data_t *)malloc(args.len*sizeof(data_t));
		if (buf == NULL) {
			perror("could not allocate buffer");
			return -1; 
		}

		/* initialize the buffer */
		for (i=0; i<args.len; i++) {
			buf[i].int_val = (int)i;
			buf[i].float_val = (float)i;
			buf[i].double_val = (double)i;
		}

		/* warm caches */
		for (i=0; i<args.num_reqs; i++) {
			rc = xfer_clnt(args.host, server_port, buf, args.len);
			if (rc == -1) {
				fprintf(stderr, "client error\n");
				break;
			}
		}

		for (i=0; i<args.count; i++) {
			int j; 
			double time, start; 

			start = lwfs_get_time(); 

			for (j=0; j<args.num_reqs; j++) {
				rc = xfer_clnt(args.host, server_port, buf, args.len);
				if (rc == -1) {
					fprintf(stderr, "client error\n");
					break;
				}
			}

			time = lwfs_get_time() - start; 
			output_stats(result_fp, &args, time); 
		}


		/* free the buffers */
		free(buf);
	}

	return rc; 
}

