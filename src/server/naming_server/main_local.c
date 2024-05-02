/**  @file main.c
 *   
 *   @brief Driver for the LWFS name server. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 793 $.
 *   $Date: 2006-06-30 13:54:01 -0600 (Fri, 30 Jun 2006) $.
 */

#include <sys/types.h>
#include <db.h>
#include <getopt.h>
#include <unistd.h>
#include <argp.h>

#include "naming_server.h"

#include "common/rpc_common/lwfs_ptls.h"


/* -- prototypes --- */

int fill_op_list(lwfs_svc_op_list **op_list); 


/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 793 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "LWFS Name Server";
static char args_doc[] = ""; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief Debug level to use. */
	log_level debug_level; 

	/** @brief The number of experiments to run. */
	int num_reqs; 

	/** @brief Options for naming service. */
	struct naming_options naming_opts; 

	/** @brief Options for authorization. */
	struct authr_options authr_opts; 
	
	/** @brief Options for storage. */
	struct ss_options ss_opts; 

	/** @brief Options to control the thread pool behavior */
	struct thread_pool_options tp_opts; 
}; 

static int print_args(FILE *fp, struct arguments *args, const char *prefix) 
{
	int rc = 0; 

	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \tverbose = %d\n", prefix, args->debug_level);
	fprintf(fp, "%s \tnum_reqs = %d\n", prefix, args->num_reqs);

	print_authr_opts(fp, &args->authr_opts, prefix); 
	print_ss_opts(fp, &args->ss_opts, prefix); 
	print_thread_pool_opts(fp, &args->tp_opts, prefix); 
	print_naming_opts(fp, &args->naming_opts, prefix); 

	fprintf(fp, "%s -----------------------------------\n", prefix);

	return rc; 
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,1=fatal,2=error,3=warn,4=info,5=debug,6=all>", 0, 
		"Produce verbose output"},
	{"num_reqs",      2, "<val>", 0, 
		"Number of requests to process"},

	SS_OPTIONS,
	AUTHR_OPTIONS,
	THREAD_POOL_OPTIONS,
	NAMING_OPTIONS,

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

	/* check for storage server options */
	if (parse_ss_opt(key, arg, &arguments->ss_opts) == 0) {
		return 0; 
	}

	/* check for authorization options */
	if (parse_authr_opt(key, arg, &arguments->authr_opts) == 0) {
		return 0; 
	}

	/* check for thread pool options */
	if (parse_thread_pool_opt(key, arg, &arguments->tp_opts) == 0) {
		return 0; 
	}

	/* check for naming options */
	if (parse_naming_opt(key, arg, &arguments->naming_opts) == 0) {
		return 0; 
	}

	/* check for local options */
	switch (key) {

		case 1: /* verbose */
			arguments->debug_level= atoi(arg);
			break;

		case 2: /* num_reqs */
			arguments->num_reqs= atoi(arg);
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
 * @brief The LWFS storage server.
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK;
	lwfs_service authr_svc; 
	lwfs_service naming_svc; 
	lwfs_service *storage_svc; 

	int max_ops = 100; 
	int num_ops = 0; 

	/* command-line arguments */
	struct arguments args; 

	/* service descriptors (only need one) */
	lwfs_service service;  

	/* default arguments */
	args.debug_level = LOG_ALL;
	args.num_reqs = -1;  /* run forever */

	args.authr_opts.local = FALSE; 
	args.authr_opts.id.pid = LWFS_AUTHR_PID; 
	args.authr_opts.id.nid = 0;
	args.authr_opts.db_path = "acls.db";
	args.authr_opts.db_clear = TRUE;
	args.authr_opts.db_recover = FALSE;
	args.authr_opts.verify_creds = FALSE;
	args.authr_opts.verify_caps = TRUE;
	args.authr_opts.cache_caps = FALSE;

	args.ss_opts.local = FALSE;
	args.ss_opts.id.pid = LWFS_SS_PID;
	args.ss_opts.id.nid = 0; 
	args.ss_opts.num_servers = 1; 
	args.ss_opts.server_file = NULL;
	args.ss_opts.root_dir = "ss-root-dir";
	args.ss_opts.iolib = SS_SYSIO;

	args.naming_opts.local = FALSE;   // true disables RPC
	args.naming_opts.id.pid = LWFS_NAMING_PID;
	args.naming_opts.id.nid = 0;
	args.naming_opts.db_path = "naming.db";
	args.naming_opts.db_clear = TRUE;
	args.naming_opts.db_recover = FALSE; 

	args.tp_opts.initial_thread_count=1;
	args.tp_opts.min_thread_count=1;
	args.tp_opts.max_thread_count=1;
	args.tp_opts.low_watermark=1;
	args.tp_opts.high_watermark=1000;

	/* Parse command line options */
	argp_parse(&argp, argc, argv, 0, 0, &args); 

	/* initialize RPC */
	lwfs_ptl_init(LWFS_NAMING_PID);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR); 
	lwfs_get_id(&args.naming_opts.id); 

	/* if the user didn't specify a nid for the authr, assume one */
	if (args.authr_opts.id.nid == 0) {
		args.authr_opts.id.nid = args.naming_opts.id.nid; 
		log_warn(args.debug_level, "nid for authr not set: assuming nid=%u",
			args.authr_opts.id.nid);
	}

	/* if the user didn't specify a nid for the ss, assume one */
	if (args.ss_opts.id.nid == 0) {
		args.ss_opts.id.nid = args.naming_opts.id.nid; 
		log_warn(args.debug_level, "nid for ss not set: assuming nid=%u",
			args.ss_opts.id.nid);
	}

	/* initialize the logger */
	logger_set_file(stdout); 
	if (args.debug_level == 0)   {
		logger_set_default_level(LOG_OFF); 
	} else if (args.debug_level > 5)   {
		logger_set_default_level(LOG_ALL); 
	} else   {
		logger_set_default_level(args.debug_level - LOG_OFF); 
	}


	/* initialize the operation array */
	memset(op_array, 0, max_ops*sizeof(lwfs_svc_op));


	/* initialize the authr client */
	rc = lwfs_authr_clnt_init(&args.authr_opts, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to initialize auth clnt: %s",
				lwfs_err_str(rc));
		return rc; 
	}


#if 0
	if (args.authr_opts.local) {
		/* copy the authorization server ops into our list of supported ops */
		num_ops += copy_ops(&op_array[num_ops], lwfs_authr_op_array(), 
				max_ops-num_ops);
	}
#endif


	/* initialize a storage client (local calls to storage svc) */
	rc = lwfs_ss_clnt_init(&args.ss_opts, &args.authr_opts, &authr_svc, &storage_svc);
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to initialize storage svc: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* initialize the storage service client */
#if 0
	if (args.ss_opts.local) {

		/* copy the storage server ops into our list of supported ops */
		num_ops += copy_ops(&op_array[num_ops], lwfs_ss_op_array(), 
				max_ops-num_ops);
	}
#endif


	/* initialize the naming service */
	rc = lwfs_naming_srvr_init(&args.naming_opts, &args.authr_opts, 
			&authr_svc, &naming_svc); 
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to initialize naming_srvr: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* copy the naming server ops into our list of supported ops */
	num_ops += copy_ops(&op_array[num_ops], lwfs_naming_op_array(), 
			max_ops-num_ops);

	/* print the arguments */
	print_args(stdout, &args, "");


	/* start processing requests */
	naming_svc.max_reqs = args.num_reqs; 
	rc = lwfs_service_start(&naming_svc, &args.tp_opts, op_array); 
	if (rc != LWFS_OK) {
		log_info(naming_debug_level, "exited service: %s",
			lwfs_err_str(rc));
	}

	/* shutdown the naming service */
	lwfs_naming_srvr_fini();


	/* shutdown the service */
	log_debug(naming_debug_level, "shutting down service library");
	lwfs_service_fini(&service); 

	/* finish the storage server client  */
	log_debug(naming_debug_level, "finishing ss client");
	lwfs_ss_clnt_fini(&args.ss_opts, &storage_svc); 

	log_debug(naming_debug_level, "finishing authr client");
	lwfs_authr_clnt_fini(&args.authr_opts, &authr_svc);

	return rc; 
}


