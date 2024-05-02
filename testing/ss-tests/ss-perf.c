/**  @file perf-test.c
 *   
 *   @brief Synchronous implementations of each of the storage server oprerations.
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 1073 $
 *   @date $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mpi.h>
#include <time.h>
#include <math.h>
#include <argp.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "perms.h"
#include "client/storage_client/storage_client.h"
#include "client/authr_client/authr_client.h"

/* --- private methods --- */

/**
 * @brief returns the difference *tv1 - *tv0 in microseconds.
 */
double tv_diff_usec(const struct timespec *tv0, const struct timespec *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec) * 1e6
		+ (tv1->tv_nsec - tv0->tv_nsec)*1e3;
}

/**
 * @brief returns the difference *tv1 - *tv0 in seconds.
 */
double tv_diff_sec(const struct timespec *tv0, const struct timespec *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec)
            + (tv1->tv_nsec - tv0->tv_nsec)/1e9;
}


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

	if (len > 2) {

		for (i=0; i<len; i++) {
			if ((vals[i] != minv) && (vals[i] != maxv)) 
				sum += vals[i]; 
		}


		return sum/(len-2); 
	}
	else {
		for (i=0; i<len; i++) {
			sum += vals[i]; 
		}

		return sum/len; 
	}
}

double variance(const double mean, const double *vals, const int len)
{
	int i; 
	double sumsq = 0; 
	double minv = min(vals, len);
	double maxv = max(vals, len);

	if (len > 2) {

		for (i=0; i<len; i++) {
			if ((vals[i] != minv) && (vals[i] != maxv)) 
				sumsq += (vals[i]-mean)*(vals[i]*mean); 
		}

		return sumsq/(len-2); 
	}

	else {

		for (i=0; i<len; i++) {
			sumsq += (vals[i]-mean)*(vals[i]*mean); 
		}

		return sumsq/len; 
	}
}

double stddev(const double mean, const double *vals, const int len)
{
	return sqrt(variance(mean, vals, len)); 
}

void output_stats(
		FILE *result_fp, 
		int num_srvrs,
		int num_trials, 
		int ops_per_trial, 
		int bytes_per_op,
		double *time,
		double *mb_per_sec)
{
	double tmean = mean(time, num_trials); 
	double mbps_mean = mean(mb_per_sec, num_trials); 
	int num_clients; 

	MPI_Comm_size(MPI_COMM_WORLD, &num_clients); 

	fprintf(result_fp, "%s ----------------------------------------------------", "%");
	fprintf(result_fp, "-------------------------------------------------------\n");
	fprintf(result_fp, "%s clients    ", "%"); 
	fprintf(result_fp, " srvrs    "); 
	fprintf(result_fp, " num-trials "); 
	fprintf(result_fp, " ops/trial  "); 
	fprintf(result_fp, " bytes/op     "); 
	fprintf(result_fp, " mean(s)      "); 
	fprintf(result_fp, " min(s)       "); 
	fprintf(result_fp, " max(s)       "); 
	fprintf(result_fp, " stddev(s)    "); 
	fprintf(result_fp, " mean (MB/s)  "); 
	fprintf(result_fp, " min (MB/s)    "); 
	fprintf(result_fp, " max (MB/s)    "); 
	fprintf(result_fp, " stdev (MB/s)\n"); 

	/* print the row */
	fprintf(result_fp, "%09d   ", num_clients);    
	fprintf(result_fp, "%09d   ", num_srvrs);    
	fprintf(result_fp, "%09d   ", num_trials);    
	fprintf(result_fp, "%09d   ", ops_per_trial); 
	fprintf(result_fp, "%09d   ", bytes_per_op*num_clients);    
	fprintf(result_fp, "%1.6e  ", tmean); 
	fprintf(result_fp, "%1.6e  ", min(time, num_trials)); 
	fprintf(result_fp, "%1.6e  ", max(time, num_trials)); 
	fprintf(result_fp, "%1.6e  ", stddev(tmean, time, num_trials)); 
	fprintf(result_fp, "%1.6e  ", mbps_mean); 
	fprintf(result_fp, "%1.6e  ", min(mb_per_sec, num_trials)); 
	fprintf(result_fp, "%1.6e  ", max(mb_per_sec, num_trials)); 
	fprintf(result_fp, "%1.6e \n", stddev(mbps_mean, mb_per_sec, num_trials)); 

	fprintf(result_fp, "%s ----------------------------------------------------", "%");
	fprintf(result_fp, "-------------------------------------------------------\n");
}



/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1073 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS storage service";
static char args_doc[] = ""; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief Debug level to use. */
	log_level debug_level; 

	/** @brief The type of experiment. */
	int type; 

	/** @brief The number of trials per experiment. */
	int num_trials; 

	/** @brief The number of operations in a trial. */
	int ops_per_trial; 

	/** @brief the size of the input/output buffer to use for each op. */
	int bytes_per_op; 

	/** @brief Where to output results. */
	char *result_file; 

	/** @brief Mode to use when opening file. */
	char *result_file_mode; 



	/** @brief Options for storage. */
	struct ss_options ss_opts; 

	/** @brief Options for authorization. */
	struct authr_options authr_opts; 

}; 

static int print_args(FILE *fp, struct arguments *args) 
{
	fprintf(fp, "------------  ARGUMENTS -----------\n");
	fprintf(fp, "\ttype = %d\n", args->type);
	fprintf(fp, "\tverbose = %d\n", args->debug_level);
	fprintf(fp, "\tbytes-per-op = %d\n", args->bytes_per_op);
	fprintf(fp, "\tnum-trials = %d\n", args->num_trials);
	fprintf(fp, "\tops-per-trial = %d\n", args->ops_per_trial);
	fprintf(fp, "\tresult-file = %s\n", ((args->result_file)? args->result_file: "stdout"));
	fprintf(fp, "\tresult-file-mode = %s\n", args->result_file_mode);

	print_authr_opts(fp, &args->authr_opts, "");
	print_ss_opts(fp, &args->ss_opts, ""); 

	fprintf(fp, "-----------------------------------\n");

    return LWFS_OK;
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,...,1=all>", 0, "Produce verbose output"},
	{"num-trials", 3, "<val>", 0, "Number of trials to run"},
	{"ops-per-trial",    4, "<val>", 0, "Number of ops per trial"},
	{"bytes-per-op",    2, "<val>", 0, "Size of buffer to use per operation"},
	{"result-file",5, "<FILE>", 0, "Results file"},
	{"result-file-mode",6, "<val>", 0, "Results file mode"},
	{"type",7, "<val>", 0, "type of experiment"},

	SS_OPTIONS,
	AUTHR_OPTIONS,

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


	/* check for ss_options */
	if (parse_ss_opt(key, arg, &arguments->ss_opts) == 0) {
		return 0; 
	}

	/* check for authr_options */
	if (parse_authr_opt(key, arg, &arguments->authr_opts) == 0) {
		return 0; 
	}


	/* check for local options */
	switch (key) {

		case 1: /* verbose */
			arguments->debug_level= atoi(arg);
			break;

		case 2: /* bytes_per_op */
			arguments->bytes_per_op = atoi(arg);
			break;

		case 3: /* num_trials */
			arguments->num_trials= atoi(arg);
			break;

		case 4: /* ops_per_trial */
			arguments->ops_per_trial= atoi(arg);
			break;

		case 5: /* result_file */
			arguments->result_file = arg;
			break;

		case 6: /* result_file_mode */
			arguments->result_file_mode = arg;
			break;

		case 7: /* type */
			arguments->type = atoi(arg);
			break;

		case ARGP_KEY_ARG:
			/* we don't expect any arguments */
			if (state->arg_num >= 0) {
				argp_usage(state);
			}
			// arguments->args[state->arg_num] = arg; 
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
 * @brief Measure the time to write \em num_ops*bytes_per_op bytes to a single 
 *        remote object. 
 */
int io_test(
		const int num_ops, 
		const int bytes_per_op,
		const lwfs_service *storage_svc,
		const lwfs_cid cid,
		const lwfs_cap *cap,
		double *write_time,
		double *read_time)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	int i, myrank; 
	lwfs_obj obj; 
	double start = 0; 
	double time = 0; 
	char *outbuf = (char *)malloc(bytes_per_op); 
	char *inbuf  = (char *)malloc(bytes_per_op); 
	size_t offset = 0; 
	lwfs_size bytes_read; 

	MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 

	/* initialize the output buffer */
	for (i=0; i< bytes_per_op; i++) {
		outbuf[i] = myrank*bytes_per_op*num_ops + i; 
	}

	/* create a new object */
	rc = lwfs_bcreate_obj(storage_svc, NULL, cid, cap, &obj); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed to create obj: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* write the buffer to the object */
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime(); 
	for (i=0; i<num_ops; i++) {
		offset = i*bytes_per_op; 
		rc = lwfs_bwrite(NULL, &obj, offset, outbuf, bytes_per_op, cap); 
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "failed to write to file: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}
	}

	/* make sure all the writes complete before we start the reads*/
	lwfs_bfsync(NULL, &obj, cap);
	
	MPI_Barrier(MPI_COMM_WORLD); 
	time = MPI_Wtime() - start;   /* this measures the max time */

	if (write_time != NULL) {
		*write_time = time; 
	}

	/* read the input bufffer from the object */
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime(); 
	for (i=0; i<num_ops; i++) {
		offset = i*bytes_per_op; 
		rc = lwfs_bread(NULL, &obj, offset, inbuf, bytes_per_op, cap, &bytes_read); 
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "failed to read buffer %d : %s", i,
					lwfs_err_str(rc));
			goto cleanup;
		}
	}
	MPI_Barrier(MPI_COMM_WORLD); 
	time = MPI_Wtime() - start;   /* this measures the max time */

	/* store the read time */
	if (read_time != NULL) {
		*read_time = time; 
	}

	/* verify the input buffer */
	for (i=0; i<bytes_per_op; i++) {
		if (inbuf[i] != outbuf[i]) {
			log_error(ss_debug_level, "failed to verify inbuf[%d]",i);
			goto cleanup;
		}
	}

cleanup: 

	/* remove the object */
	rc2= lwfs_bremove_obj(NULL, &obj, cap); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed to remove obj: %s",
				lwfs_err_str(rc));
		rc = rc2;
		goto freebufs;
	}

freebufs:
	free(outbuf);
	free(inbuf);

	return rc;
}


/**
 * Tests for an SS client. 
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK;   /* return code */
	int i; 
	int nprocs, myproc;

	struct arguments args; 


	lwfs_opcode opcodes; 
	lwfs_cred cred; 
	lwfs_cap cap; 
	lwfs_cid cid;

	double *time; 
	double *mb_per_sec; 

	char fprefix[256]; 

	FILE *result_fp = stdout; 

	lwfs_service authr_svc;
	lwfs_service *storage_svc; 

	/* default arg values */
	args.type = 1;
	args.debug_level = LOG_ALL;
	args.ops_per_trial = 10; 
	args.bytes_per_op = 1024; 
	args.num_trials = 1;
	args.result_file = NULL; 
	args.result_file_mode = "w"; 

	args.ss_opts.local = FALSE; 
    args.ss_opts.num_servers = 1; 
    args.ss_opts.server_file = NULL;
    args.ss_opts.server_ids = NULL; 
	args.ss_opts.root_dir = "ss-root";
	args.ss_opts.id.pid = LWFS_SS_PID; 
	args.ss_opts.id.nid = 0;
	
    args.authr_opts.id.nid = 0;
    args.authr_opts.id.pid = LWFS_AUTHR_PID;
    args.authr_opts.local = FALSE;
    args.authr_opts.db_path = "naming.db";
    args.authr_opts.db_clear = FALSE; 
    args.authr_opts.db_recover = FALSE; 
    args.authr_opts.verify_creds = FALSE;
    args.authr_opts.verify_caps = TRUE;
    args.authr_opts.cache_caps = FALSE;


	/* initialize MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myproc);

	MPI_Barrier(MPI_COMM_WORLD);

	/* Parse the command-line arguments */
	argp_parse(&argp, argc, argv, 0, 0, &args); 


	/* allocate space for the time array */
	time = (double *)malloc(args.num_trials*sizeof(double));
	mb_per_sec = (double *)malloc(args.num_trials*sizeof(double));


	/* initialize the logger */
	logger_set_file(stdout); 
	if (args.debug_level == 0)   {
		logger_set_default_level(LOG_OFF); 
	} else if (args.debug_level > 5)   {
		logger_set_default_level(LOG_ALL); 
	} else   {
		logger_set_default_level(args.debug_level - LOG_OFF); 
	}


	/* initialize RPC */
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);


	/* get the descriptor for the authorization service */
	rc = lwfs_get_service(args.authr_opts.id, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to get authr service descriptor: %s",
				lwfs_err_str(rc));
		return rc; 
	}


	/* get the storage service descriptors */
	storage_svc = (lwfs_service *)
		calloc(args.ss_opts.num_servers, sizeof(lwfs_service));
	rc = lwfs_get_services(args.ss_opts.server_ids, 
			args.ss_opts.num_servers, storage_svc);
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to get storage server descriptors: %s",
				lwfs_err_str(rc));
		return rc; 
	}



	/* open the result file */
	if ((myproc == 0) && (args.result_file != NULL)) {
		result_fp = fopen(args.result_file, args.result_file_mode); 
		if (result_fp == NULL) {
			log_warn(ss_debug_level, "could not open result file: using stdout");
			result_fp = stdout; 
		}
	}


	/* print the arguments */
	if (myproc == 0) {
		print_args(stdout, &args); 
	}



	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ; 
	memset(&cred, 0, sizeof(cred)); 

	/* Create a container and allocate the capabilities necessary 
	 * to perform operations. For simplicity, all objs belong to 
	 * one container. 
	 */
	rc = get_perms(&authr_svc, &cred, &cid, opcodes, &cap); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to create a container and get caps: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	sprintf(fprefix, "test%d", myproc); 

	for (i=0; i<args.num_trials; i++) {

		switch (args.type) {
			case 1: 
				/* time writes */
				log_debug(ss_debug_level, "Starting write exp %d", i);
				rc = io_test(args.ops_per_trial, 
						args.bytes_per_op, 
						&storage_svc[myproc%args.ss_opts.num_servers], 
						cid, 
						&cap, 
						&time[i],
						NULL);
				if (rc != LWFS_OK) {
					log_error(ss_debug_level, "could not write to objs files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
				log_debug(ss_debug_level, "Finished write exp %d", i);

				break;

			case 2: 
				/* create the files */
				log_debug(ss_debug_level, "Starting read exp %d", i);
				rc = io_test(args.ops_per_trial, 
						args.bytes_per_op, 
						&storage_svc[myproc%args.ss_opts.num_servers], 
						cid, 
						&cap, 
						NULL,
						&time[i]);
				if (rc != LWFS_OK) {
					log_error(ss_debug_level, "could not read objs: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}

				log_debug(ss_debug_level, "Finished read exp %d", i);
				break;

			default:
				log_error(ss_debug_level, "unrecognized experiment type (%d)",
						args.type); 
				return LWFS_ERR; 
		}

		mb_per_sec[i] = (args.ops_per_trial*nprocs*args.bytes_per_op)/(time[i]*1024*1024); 
	}
	
	if (myproc == 0) {
		output_stats(result_fp, 
				args.ss_opts.num_servers, 
				args.num_trials, 
				args.ops_per_trial, 
				args.bytes_per_op, 
				time, 
				mb_per_sec); 
	}

cleanup:

	/* close ss */
	free(storage_svc);

	lwfs_rpc_fini();

	/* clean up */ 
	free(time); 
	free(mb_per_sec); 

	MPI_Finalize();


	return rc; 
}
