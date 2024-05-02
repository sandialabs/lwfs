/**  @file main.c
 *   
 *   @brief An LWFS storage server. 
 * 
 *   The LWFS storage server executes the code to service 
 *   remote requests for the LWFS storage service.  The 
 *   storage server also requires access to an authorization
 *   service that can verify the authenticity of capabilities
 *   sent from the client.  This implementation allows the 
 *   user to install either install an authorization service
 *   on the same process as the storage server, or select a 
 *   remote authorization service by specifying the Portals 
 *   nid, pid, and index to send authorization service requests. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 */

#include <sys/types.h>
#include <db.h>
#include <getopt.h>
#include <unistd.h>
#include <argp.h>

#include "storage_server.h"

#include "common/rpc_common/lwfs_ptls.h"

#include "support/threadpool/thread_pool.h"
#include "support/threadpool/thread_pool_options.h"


/* -- prototypes --- */


/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 791 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "LWFS Storage Server";
static char args_doc[] = ""; 

struct arguments {
	/** @brief Debug level to use */
	log_level debug_level; 

	/** @brief Number of requests to service */
	int num_reqs; 

	/** @brief Use a threaded server. */
	lwfs_bool use_threads; 


	/** @brief Authorization options */
	struct authr_options authr_opts;

	/** @brief Storage options. */
	struct ss_options ss_opts; 

	/** @brief Thread pool options */
	struct thread_pool_options tp_opts; 
}; 


static int print_args(FILE *fp, struct arguments *args, const char *prefix) 
{
	int rc = 0; 

	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \tverbose = %d\n", prefix, args->debug_level);
	fprintf(fp, "%s \tnum_reqs = %d\n", prefix, args->num_reqs);
	fprintf(fp, "%s \tuse_threads = %d\n", prefix, args->use_threads);

	print_ss_opts(fp, &args->ss_opts, prefix); 
	print_authr_opts(fp, &args->authr_opts, prefix); 

    if (args->use_threads) {
        print_thread_pool_opts(fp, &args->tp_opts, prefix); 
    }

	fprintf(fp, "-----------------------------------\n");

	return rc; 
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,...,1=all>", 0, "Produce verbose output"},
	{"num-reqs",   2, "<val>", 0, "Number of requests to service"},
	{"use-threads",   3, 0, 0, "Use a threaded server."},

	SS_OPTIONS, 
	AUTHR_OPTIONS,
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

	/* check local options */
	switch (key) {

		case 1: /* verbose */
			arguments->debug_level= atoi(arg);
			break;

		case 2: /* num_reqs */
			arguments->num_reqs= atoi(arg);
			break;

		case 3: /* use_threads */
			arguments->use_threads=TRUE;
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


/* -- private methods -- */

static const char DEFAULT_ROOT[] = "./ss-root";


static int copy_ops(
	lwfs_svc_op *dest, 
	const lwfs_svc_op *src, 
	const int len)
{
	int i;
	int count = 0;

	lwfs_svc_op *to; 
	lwfs_svc_op *from; 

	for (i=0; i<len; i++) {
		from = (lwfs_svc_op *)&src[i]; 
		to = &dest[i]; 

		if (from->opcode == LWFS_OP_NULL) {
			return count; 
		}

		else {
			memcpy(to, from, sizeof(lwfs_svc_op));
		}
		count++;
	}

	return count; 
}


/**
 * @brief The LWFS storage server.
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK;
	lwfs_service authr_svc; 
	lwfs_svc_op op_array[100]; 
	int max_ops = 100; 
	int num_ops = 0;

	struct arguments args; 

	/* default process ID and portal index to use for incoming requests */

	/* service descriptors (only need one) */
	lwfs_service service;  

	memset(&args, 0, sizeof(struct arguments));

	/* default options */
	args.debug_level = LOG_ALL;
	args.num_reqs = -1; 
	args.use_threads = FALSE; 

	args.authr_opts.verify_creds = FALSE;
	args.authr_opts.verify_caps = TRUE;
	args.authr_opts.cache_caps = TRUE;
	args.authr_opts.local = FALSE; 
	args.authr_opts.id.pid = LWFS_AUTHR_PID; 
	args.authr_opts.id.nid = 0;
	args.authr_opts.db_path = "acl.db";
	args.authr_opts.db_clear = TRUE;
	args.authr_opts.db_recover = FALSE;

	args.ss_opts.local = FALSE;  // local=TRUE disables PORTALS
	args.ss_opts.id.nid = 0;  // not used 
	args.ss_opts.id.pid = LWFS_SS_PID; 
	args.ss_opts.server_file = NULL;
	args.ss_opts.num_servers = 1;
	args.ss_opts.root_dir = (char *)DEFAULT_ROOT;
    args.ss_opts.iolib = SS_SYSIO;

	args.tp_opts.initial_thread_count=1;
	args.tp_opts.min_thread_count=1;
	args.tp_opts.max_thread_count=1;
	args.tp_opts.low_watermark=1;
	args.tp_opts.high_watermark=1000;

	/* initialize the array of supported operations */
	memset(op_array, 0, max_ops*sizeof(lwfs_svc_op)); 

	/* Parse command line options */
	argp_parse(&argp, argc, argv, 0, 0, &args); 


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
	lwfs_ptl_init(LWFS_SS_PID);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

	/* sets the id.nid */
	lwfs_get_id(&args.ss_opts.id);


	/* get the service description for authorization */
	rc = lwfs_get_service(args.authr_opts.id, &authr_svc); 
	if (rc != LWFS_OK) {
		log_error(args.debug_level, 
				"unable to get authr service: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* cache caps */
	if (args.authr_opts.cache_caps) {
		lwfs_cache_caps_init();
	}

	/* if local, copy authr ops into our list of supported operations */
	if (args.authr_opts.local) {
		num_ops += copy_ops(&op_array[num_ops], lwfs_authr_op_array(), 
				max_ops-num_ops); 
	}

	/* initialize the storage server */
	log_debug(args.debug_level, "initializing storage server");
	rc = lwfs_ss_srvr_init(&args.ss_opts, &args.authr_opts, &authr_svc, &service);
	if (rc != LWFS_OK) {
		log_fatal(args.debug_level, "Could not start storage server");
		goto exit; 
	}

	/* print the arguments */
	print_args(stdout, &args, "");
	fflush(stdout);

	/* copy storage server ops into our list of supported operations */
	num_ops += copy_ops(&op_array[num_ops], lwfs_ss_op_array(), 
			max_ops-num_ops); 

	/* start the storage server */
	log_debug(args.debug_level, "starting server");
	service.max_reqs = args.num_reqs; 

    if (args.use_threads) {
        rc = lwfs_service_start(&service, &args.tp_opts, op_array);
    }
    else {
        /* send null tp_opts to signal single-threaded server */
        rc = lwfs_service_start(&service, NULL, op_array);
    }
	if (rc != LWFS_OK) {
		log_fatal(args.debug_level, "Could not start storage server");
		goto exit; 
	}

exit:

	/* shutdown the lwfs storage server  */
	log_debug(ss_debug_level, "shutting down RPC library");
	lwfs_ss_srvr_fini(&service); 
	
	/* close the authr client */
	rc = lwfs_authr_clnt_fini(&args.authr_opts, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(args.debug_level, 
				"unable to close authr clnt: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	lwfs_rpc_fini(); 

	return rc; 
}


