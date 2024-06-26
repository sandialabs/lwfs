/**  @file main.c
 *   
 *   @brief Driver for the LWFS name server. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1264 $.
 *   $Date: 2007-02-27 15:30:26 -0700 (Tue, 27 Feb 2007) $.
 */

#include <time.h>
#include <sys/types.h>
#include <db.h>
#include <getopt.h>
#include <unistd.h>
#include <argp.h>

#include "support/logger/logger.h"
#include "support/threadpool/thread_pool_debug.h"

#include "common/rpc_common/lwfs_ptls.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"

#include "server/rpc_server/rpc_server.h"

/*
#include "rpc/rpc_srvr.h"
#include "rpc/rpc_common.h"
*/
#include "xfer.h"



log_level xfer_debug_level = LOG_UNDEFINED; 

/**
 * @brief Transfer an array of \ref data_t structures through 
 *        the process arguments. 
 */
int xfer_1_srvr(
	const lwfs_remote_pid *caller, 
	const xfer_1_args *args,
	const lwfs_rma *data_addr,
	data_t *result)
{
	const int len = args->array.data_array_t_len; 
	const data_t *array = args->array.data_array_t_val; 

	/* copy the last entry of the array to the result */
	memcpy(result, &array[len-1], sizeof(data_t));

	return LWFS_OK;
}

/**
 * @brief Transfer an array of \ref data_t structures through 
 *        the data portal. 
 */
int xfer_2_srvr(
	const lwfs_remote_pid *caller, 
	const xfer_2_args *args,
	const lwfs_rma *data_addr,
	data_t *result)
{
	int rc = LWFS_OK;

	const int len = args->len; 
	int nbytes = len*sizeof(data_t); 

	/* allocate space for the incoming buffer */
	data_t *buf = (data_t *)malloc(nbytes); 

	/* now we need to fetch the data from the client */
	rc = lwfs_get_data(buf, nbytes, data_addr); 
	if (rc != LWFS_OK) {
		log_warn(rpc_debug_level, "could not fetch data from client");
		return LWFS_ERR_RPC; 
	}

	/* copy the last entry into the result */
	memcpy(result, &buf[len-1], sizeof(data_t));

    //sleep(1); 

	free(buf);

	return LWFS_OK; 
}



static const lwfs_svc_op xfer_op_array[] = {
	{
		1,                                /* opcode */
		(lwfs_rpc_proc)&xfer_1_srvr,        /* func */
		sizeof(xfer_1_args),             /* sizeof args */
		(xdrproc_t)&xdr_xfer_1_args,     /* decode args */
		sizeof(data_t),                   /* sizeof res */
		(xdrproc_t)&xdr_data_t            /* encode res */
	},
	{
		2,                                /* opcode */
		(lwfs_rpc_proc)&xfer_2_srvr,        /* func */
		sizeof(xfer_2_args),             /* sizeof args */
		(xdrproc_t)&xdr_xfer_2_args,     /* decode args */
		sizeof(data_t),                   /* sizeof res */
		(xdrproc_t)&xdr_data_t            /* encode res */
	},
	{LWFS_OP_NULL}
};

/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1264 $"; 
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

	/** @brief The process ID of the server. */
	lwfs_remote_pid id; 

	/** @brief Index where the server expects requests. */
	int req_len; 

	/** @brief Use a threaded server. */
	lwfs_bool use_threads;
	
	/** @brief Options to control the thread pool behavior */
	struct thread_pool_options tp_opts;
}; 

static int print_args(FILE *fp, const char *prefix, struct arguments *args) 
{
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tLWFS RPC throughput server\n", prefix);
	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s \tnid = %llu\n", prefix, (unsigned long long)args->id.nid);
	fprintf(fp, "%s \tpid = %llu\n", prefix, (unsigned long long)args->id.pid);
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \t--verbose = %d\n", prefix, args->debug_level);
	fprintf(fp, "%s \t--count = %d\n", prefix, args->count);
	fprintf(fp, "%s \t--req-len = %d\n", prefix, args->req_len);
	fprintf(fp, "%s \t--use-threads = %s\n", prefix, (args->use_threads)?"yes":"no");

    if (args->use_threads) {
	    fprintf(fp, "%s \t--thread-debug-level = %d\n", prefix, args->thread_debug_level);
        print_thread_pool_opts(fp, &args->tp_opts, prefix);
    }
	return 0;
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,1=fatal,2=error,3=warn,4=info,5=debug,6=all>", 0, "Produce verbose output"},
	{"thread-debug-level",    2, "<0=none,1=fatal,2=error,3=warn,4=info,5=debug,6=all>", 0, "Produce verbose output"},
	{"count",      3, "<val>", 0, "Number of tests to run"},
	{"pid",      4, "<val>", 0, "PID used by the server"},
	{"index",      5, "<val>", 0, "Index of request portal"},
	{"req-len",      6, "<val>", 0, "Length of a short request"},
    {"use-threads",   7, 0, 0, "Use a threaded server."},

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

		case 4: /* pid */
			arguments->id.pid= atoll(arg);
			break;

		case 6: /* req-len */
			arguments->req_len= atoi(arg);
			break;

		case 7: /* use-threads */
			arguments->use_threads= TRUE;
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



/**
 * @brief The LWFS xfer-server.
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK;

	lwfs_service xfer_svc; 

	struct arguments args; 

	/* default arguments */
	memset(&args, 0, sizeof(struct arguments));
	args.debug_level = LOG_ALL;
	args.thread_debug_level = LOG_UNDEFINED;
	args.count = -1;  /* run forever */
	args.id.nid = 0; 
	args.id.pid = 122; 
	args.req_len = LWFS_SHORT_REQUEST_SIZE; 
	args.use_threads = FALSE;

	/* defaults for thread pool */
	load_default_thread_pool_opts(&args.tp_opts);

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

    xfer_debug_level = args.debug_level; 

	// global variable
    thread_debug_level = args.thread_debug_level; 

	/* initialize RPC */
	lwfs_ptl_init(PTL_IFACE_DEFAULT, args.id.pid); 
	rc = lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);
	if (rc != LWFS_OK) {
		log_error(xfer_debug_level, "could not init RPC: %s",
			lwfs_err_str(rc));
		return -1;
	}

	/* set the nid of the arguments */
	lwfs_get_id(&args.id); 


	/* initialize the lwfs service */
	rc = lwfs_service_init(0, args.req_len, &xfer_svc); 
	if (rc != LWFS_OK) {
		log_error(xfer_debug_level, "could not init xfer_svc: %s",
			lwfs_err_str(rc));
		return -1;
	}

	/* add the xfer operations to the list of supported ops */
	rc = lwfs_service_add_ops(&xfer_svc, xfer_op_array, 2);

	/* print the arguments */
	print_args(stdout, "", &args);


	/* start processing requests */
	xfer_svc.max_reqs = args.count; 
    if (args.use_threads) {
        rc = lwfs_service_start(&xfer_svc, &args.tp_opts); 
    }
    else {
        /* send null tp_opts to signal single-threaded */
        rc = lwfs_service_start(&xfer_svc, NULL); 
    }
	if (rc != LWFS_OK) {
		log_info(xfer_debug_level, "exited xfer_svc: %s",
			lwfs_err_str(rc));
	}

	/* shutdown the xfer_svc */
	log_debug(xfer_debug_level, "shutting down service library");
	lwfs_service_fini(&xfer_svc); 

	lwfs_rpc_fini(); 

	/* shutdown the lwfs storage server  */
	/*
	log_debug(ss_debug_level, "shutting down RPC library");
	lwfs_ss_srvr_fini(); 
	*/

	return rc; 
}


