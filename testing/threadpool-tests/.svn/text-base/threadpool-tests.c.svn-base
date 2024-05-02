/**  @file thread-pool-tests.c
 *   
 *   @brief Test the thread pool. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 643 $.
 *   $Date: 2006-05-11 14:07:09 -0600 (Thu, 11 May 2006) $.
 */

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <getopt.h>
#include <unistd.h>
#include <argp.h>
#include <string.h>

#include "support/threadpool/thread_pool.h"
#include "support/threadpool/thread_pool_options.h"
#include "support/threadpool/thread_pool_debug.h"


log_level xfer_debug_level = LOG_UNDEFINED; 

/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 643 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "LWFS Name Server";
static char args_doc[] = ""; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {
	/** @brief Debug level to use. */
	log_level debug_level; 

	/** @brief Debug level to use. */
	log_level thread_debug_level; 

	/** @brief The number of experiments to run. */
	int count; 

	/** @brief Options to control the thread pool behavior */
	struct thread_pool_options tp_opts;
}; 

static int print_args(FILE *fp, const char *prefix, struct arguments *args) 
{
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tLWFS threadpool tests\n", prefix);
	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \t--verbose = %d\n", prefix, args->debug_level);
	fprintf(fp, "%s \t--count = %d\n", prefix, args->count);
	fprintf(fp, "%s \t--thread-debug-level = %d\n", prefix, args->thread_debug_level);
	print_thread_pool_opts(fp, &args->tp_opts, prefix);
	return 0;
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,1=fatal,2=error,3=warn,4=info,5=debug,6=all>", 0, "Produce verbose output"},
	{"count",      3, "<val>", 0, "Number of tests to run"},
	{"thread-debug-level",    2, "<0=none,1=fatal,2=error,3=warn,4=info,5=debug,6=all>", 0, "Produce verbose output"},
	THREAD_POOL_OPTIONS,

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

	/* check for thread pool options */
	if (parse_thread_pool_opt(key, arg, &arguments->tp_opts) == 0) {
		return 0;
	}

	switch (key) {

		case 1: /* verbose */
			arguments->debug_level= atoi(arg);
			break;

		case 2: /* verbose */
			arguments->thread_debug_level= atoi(arg);
			break;

		case 3: /* count */
			arguments->count= atoi(arg);
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


/* --------------------- private methods ----------------------- */


struct thread_args {
	int req_id;
	int sleep_time; 
};


static int process_request(
	struct lwfs_thread_pool_request *a_request,
	const int thread_id)
{
	struct thread_args *thread_args = (struct thread_args *)(a_request->client_data);

	fprintf(stderr, "%d: ", thread_id);
	fprintf(stderr, "processing request %d, ", thread_args->req_id);
	fprintf(stderr, "sleep %d secs\n", thread_args->sleep_time);

	sleep(thread_args->sleep_time);
	return 0;
}


/**
 * @brief The LWFS xfer-server.
 */
int main(int argc, char **argv)
{
	int rc = 0;
	int i;
	lwfs_thread_pool pool; 

	struct arguments args; 
	struct thread_args *thread_args; 

	/* default arguments */
	memset(&args, 0, sizeof(args));
	args.debug_level = 0;
	args.count = 10;  /* run forever */
	args.thread_debug_level = 2; 


	/* defaults for thread pool */
	load_default_thread_pool_opts(&args.tp_opts);
	args.tp_opts.min_thread_count = 5;
	args.tp_opts.max_thread_count = 5;
	args.tp_opts.initial_thread_count = 5;

	/* Parse command line options */
	argp_parse(&argp, argc, argv, 0, 0, &args); 

    thread_debug_level = args.thread_debug_level; 

	/* print the arguments */
	print_args(stdout, "", &args);


	/* allocate a thread pool */
	lwfs_thread_pool_init(&pool, &args.tp_opts);

	/* allocate arguments for the threads */
	thread_args = (struct thread_args *)calloc(args.count, sizeof(struct thread_args));

	/* submit requests to the thread pool */
	for (i=0; i<args.count; i++) {
		thread_args[i].req_id = i; 
		thread_args[i].sleep_time = rand() % args.count; 
		lwfs_thread_pool_add_request(&pool, &thread_args[i], process_request);
	}

	/* free the thread pool */
	lwfs_thread_pool_fini(&pool);

	/* free the thread arguments */
	free(thread_args);

	return rc; 
}


