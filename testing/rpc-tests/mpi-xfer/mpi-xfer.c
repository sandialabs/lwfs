/**
 * @file mpi-xfer.c
 *
 * @brief Implementation of two-process code that uses MPI
 * one-sided communication to send the \ref data_t buffer 
 * across the network.
 *
 * @author Ron A. Oldfield (raoldfi\@sandia.gov).
 * $Revision: 1073 $. 
 * $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $.
 */

#include <math.h>
#include <assert.h>
#include <mpi.h>
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
#include <arpa/inet.h>

typedef struct  {
	int int_val;
	float float_val;
	double double_val;
} data_t; 

enum {
	MPI_BLOCKING=0,
	MPI_ASYNC,
	MPI_ONE_SIDED
};


/* ----- STATS FUNCTIONS -------- */

double max(const double *vals, const int len)
{
	int i; 
	double max = vals[0]; 

	for (i=0; i<len; i++) {
		max = (vals[i] > max)? vals[i]: max; 
	}

	return max;
}

double min(const double *vals, const int len)
{
	int i; 
	double min = vals[0]; 

	for (i=0; i<len; i++) {
		min = (vals[i] < min)? vals[i]: min; 
	}

	return min;
}

double mean(const double *vals, const int len)
{
	int i; 
	double sum = 0; 
	double minv = min(vals, len);
	double maxv = max(vals, len);

	assert(len > 2); 

	for (i=0; i<len; i++) {
		if ((vals[i] != minv) && (vals[i] != maxv)) 
			sum += vals[i]; 
	}

	return sum/(len-2); 
}

double variance(const double mean, const double *vals, const int len)
{
	int i; 
	double sumsq = 0; 
	double minv = min(vals, len);
	double maxv = max(vals, len);

	assert(len > 2); 

	for (i=0; i<len; i++) {
		if ((vals[i] != minv) && (vals[i] != maxv)) 
			sumsq += (vals[i]-mean)*(vals[i]*mean); 
	}

	return sumsq/len; 
}

double stddev(const double mean, const double *vals, const int len)
{
	return sqrt(variance(mean, vals, len)); 
}


void output_stats(
		FILE *result_fp, 
		int count, 
		int len, 
		double *time,
		double *bw)
{
	double tmean = mean(time, count); 
	double bw_mean = mean(bw, count); 
	ssize_t nbytes = sizeof(int) + (len+1)*sizeof(data_t); 


	fprintf(result_fp, "%s ----------------------------------------------------", "%");
	fprintf(result_fp, "-------------------------------------------------------\n");
	fprintf(result_fp, "%s len       ", "%"); 
	fprintf(result_fp, "  count     "); 
	fprintf(result_fp, "  nbytes    "); 
	fprintf(result_fp, "   mean(s)    "); 
	fprintf(result_fp, "   min(s)     "); 
	fprintf(result_fp, "   max(s)     "); 
	fprintf(result_fp, "  stddev(s)  "); 
	fprintf(result_fp, " mean (MB/s)   "); 
	fprintf(result_fp, " min (MB/s)  "); 
	fprintf(result_fp, " max (MB/s)  "); 
	fprintf(result_fp, " stdev (MB/s)\n"); 

	/* print the row */
	fprintf(result_fp, "%09d   ", len);    
	fprintf(result_fp, "%09d   ", count); 
	fprintf(result_fp, "%09d   ", nbytes); 
	fprintf(result_fp, "%1.6e  ", tmean); 
	fprintf(result_fp, "%1.6e  ", min(time, count)); 
	fprintf(result_fp, "%1.6e  ", max(time, count)); 
	fprintf(result_fp, "%1.6e  ", stddev(tmean, time, count)); 
	fprintf(result_fp, "%1.6e  ", bw_mean = mean(bw, count)); 
	fprintf(result_fp, "%1.6e  ", min(bw, count)); 
	fprintf(result_fp, "%1.6e  ", max(bw, count)); 
	fprintf(result_fp, "%1.6e \n", stddev(bw_mean, bw, count)); 

	fprintf(result_fp, "%s ----------------------------------------------------", "%");
	fprintf(result_fp, "-------------------------------------------------------\n");
}


static int xfer_srvr_blk(int client, int count, MPI_Datatype mpi_data_t)
{
	int i; 
	int len; 
	int rc = 0; 
	data_t *buf = NULL; 
	MPI_Status stat; 

	for (i=0; i<count; i++) {

		/* synchronize with client */
		MPI_Barrier(MPI_COMM_WORLD);

		/* get the length of the incoming buffer */
		MPI_Recv(&len, 1, MPI_INT, client, 0, MPI_COMM_WORLD, &stat); 

		/* allocate space for buffer */
		fprintf(stderr, "%d: allocating %d structs\n",
			(count+1)%2, len);
		buf = (data_t *)malloc(len*sizeof(data_t)); 
		if (buf == NULL) {
			fprintf(stderr, "out of space");
			MPI_Abort(MPI_COMM_WORLD, -1); 
		}

		/* get the buffer from the client */
		MPI_Recv(buf, len, mpi_data_t, client, 0, MPI_COMM_WORLD, &stat); 

		/* send the last value of the buffer back to the client */
		MPI_Send(&buf[len-1], 1, mpi_data_t, client, 0, MPI_COMM_WORLD); 

		/* sync back up with client */
		MPI_Barrier(MPI_COMM_WORLD); 
	}

	/* free the buffer */
	free(buf);

	return rc; 
}

static int xfer_clnt_blk(
		int server, 
		int count, 
		int len, 
		data_t *buf, 
		MPI_Datatype mpi_data_t,
		double *time,
		double *bw)
{
	int rc = 0;
	int i;
	data_t result; 
	ssize_t nbytes; 
	double start, end; 
	MPI_Status stat; 


	for (i=0; i<count; i++) {
		MPI_Barrier(MPI_COMM_WORLD); 
		start = MPI_Wtime(); 

		/* write the length of the buffer */
		MPI_Send(&len, 1, MPI_INT, server, 0, MPI_COMM_WORLD);

		/* write the buffer */
		MPI_Send(buf, len, mpi_data_t, server, 0, MPI_COMM_WORLD);

		/* receive the result */
		MPI_Recv(&result, 1, mpi_data_t, server, 0, MPI_COMM_WORLD, &stat);

		end = MPI_Wtime(); 
		time[i] = end-start; 
		MPI_Barrier(MPI_COMM_WORLD);

		/* calculate MB/sec throughput */
		nbytes = sizeof(int) + (len+1)*sizeof(data_t); 
		bw[i] = nbytes/(1024*1024*time[i]); 
	}

	return rc; 
}

/**
 * @brief returns the difference *tv1 - *tv0 in microseconds.
 */
double tv_diff_usec(const struct timespec *tv0, const struct timespec *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec) * 1e6
		+ (tv1->tv_nsec - tv0->tv_nsec) * 1e3;
}

/**
 * @brief returns the difference *tv1 - *tv0 in seconds.
 */
double tv_diff_sec(const struct timespec *tv0, const struct timespec *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec)
            + (tv1->tv_nsec - tv0->tv_nsec)/1e9;
}

/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1073 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS storage service";
static char args_doc[] = "HOSTNAME"; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief the size of the input/output buffer to use for tests. */
	int len; 

	/** @brief The number of experiments to run. */
	int count; 

	/** @brief Where to put results */
	char *result_file; 

	/** @brief Where to put results */
	char *result_file_mode; 

	/** @brief type of experiment. */
	int type; 
}; 

static int print_args(FILE *fp, const char *prefix, struct arguments *args) 
{
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tMeasure MPI throughput ", prefix);
	switch(args->type) {
		case MPI_BLOCKING:
			fprintf(fp, "(blocking calls)\n");
			break;
		case MPI_ASYNC:
			fprintf(fp, "(blocking calls)\n");
			break;
		case MPI_ONE_SIDED:
			fprintf(fp, "(one-sided calls)\n");
			break;
		default:
			fprintf(fp, "(undefined type=%d)\n", args->type);
			break;
	}


	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \t--len = %d\n", prefix, args->len);
	fprintf(fp, "%s \t--count = %d\n", prefix, args->count);
	fprintf(fp, "%s \t--result-file = \"%s\"\n", prefix, args->result_file);
	fprintf(fp, "%s \t--result-file-mode = \"%s\"\n", prefix, args->result_file_mode);
	fprintf(fp, "%s -----------------------------------\n", prefix);

	return 0;
}


static struct argp_option options[] = {
	{"type",  1, "[0--3]", 0, "type of experiment"},
	{"len",    2, "<val>", 0, "number of array elements to send for each exp"},
	{"count",      3, "<val>", 0, "Number of experiments to run"},
	{"result-file",      5, "<FILE>", 0, "Where to put results"},
	{"result-file-mode",      6, "<val>", 0, "Result file mode"},
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

		case 1: /* type */
			arguments->type = atoi(arg);
			break;

		case 2: /* len */
			arguments->len = atoi(arg);
			break;

		case 3: /* count */
			arguments->count = atoi(arg);
			break;

		case 5: /* server flag */
			arguments->result_file = arg;
			break;

		case 6: /* mode for result file */
			arguments->result_file_mode = arg;
			break;


		case ARGP_KEY_ARG:
			/* we expect only one argument */
			if (state->arg_num >= 0) {
				argp_usage(state);
			}
			//arguments->args[state->arg_num] = arg; 
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

/**
*
*  The TCP server receives the size of the incoming array, allocates 
*  the required memory for the buffer, receives the array, and sends 
*  a single 16-byte data_t object back to the client. 
*/
int main(int argc, char **argv)
{
	char hostname[256];
	struct arguments args; 
	int rc = 0; 
	FILE *result_fp = stdout; 
	int myproc, nprocs, other_proc; 
	int blen[3]; 
	MPI_Aint indices[3]; 
	MPI_Datatype oldtypes[3]; 
	MPI_Datatype mpi_data_t; 

	/* default values for the arguments */
	args.type = MPI_BLOCKING;
	args.count = 1; 
	args.len = 1;
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

	/* initialize MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myproc);

	if (nprocs != 2) goto exit;
	other_proc = (myproc+1) % 2; 

	gethostname(hostname, 256); 

	fprintf(stderr, "Hello from %s: node %d of %d\n", 
			hostname, myproc, nprocs);

	MPI_Barrier(MPI_COMM_WORLD);

	/* define the derived datatype for the data_t structure */
	/* the int value */
	blen[0] = 1; 
	indices[0] = 0; 
	oldtypes[0] = MPI_INT; 

	/* the float value */
	blen[1] = 1; 
	indices[1] = sizeof(int); 
	oldtypes[1] = MPI_FLOAT; 

	/* the double value */
	blen[2] = 1; 
	indices[2] = sizeof(int) + sizeof(float);
	oldtypes[2] = MPI_DOUBLE; 

	MPI_Type_struct(3, blen, indices, oldtypes, &mpi_data_t); 
	MPI_Type_commit(&mpi_data_t);

	

	if (myproc == 0) {
		rc = xfer_srvr_blk(other_proc, args.count, mpi_data_t);
	}
	else {
		int i; 
		data_t *buf; 
		double *time = (double *)malloc(args.count*sizeof(double));
		double *bw = (double *)malloc(args.count*sizeof(double));

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

		rc = xfer_clnt_blk(other_proc, args.count, args.len, 
				buf, mpi_data_t, time, bw);
		if (rc == -1) {
			fprintf(stderr, "client error\n");
			return -1;
		}

		/* output stats */
		output_stats(result_fp, args.count, args.len, time, bw ); 
		

		/* free the buffer */
		free(buf);
		free(time); 
		free(bw); 
	}

exit:
	/* finish the MPI function */
	MPI_Finalize();

	return rc; 
}

