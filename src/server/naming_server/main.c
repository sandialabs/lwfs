/**  @file main.c
 *   
 *   @brief Driver for the LWFS name server. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1560 $.
 *   $Date: 2007-09-11 18:29:51 -0600 (Tue, 11 Sep 2007) $.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <db.h>
#include <getopt.h>
#include <unistd.h>

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#ifdef HAVE_GENGETOPT
#include "cmdline.h"
#else
#include "cmdline_default.h"
#endif 

#include "naming_server.h"
#include "naming_server_opts.h"

#include "client/authr_client/authr_client_opts.h"
#include "support/logger/logger_opts.h"
#include "support/threadpool/threadpool_opts.h"
#include "support/sysmon/sysmon_opts.h"


#include "common/rpc_common/lwfs_ptls.h"
#include "common/rpc_common/ptl_wrap.h"

/* -- prototypes --- */

int fill_op_list(lwfs_svc_op_list **op_list); 

extern unsigned long max_mem_allowed;


/* ----------------- COMMAND-LINE OPTIONS --------------- */

static int print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix) 
{
	int rc = 0; 

	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \tnum_reqs = %d\n", prefix, args_info->num_reqs_arg);
	fprintf(fp, "%s \tdaemon = %s\n", prefix, 
			(args_info->daemon_flag)?"true":"false");

	print_logger_opts(fp, args_info, prefix); 
	print_authr_client_opts(fp, args_info, prefix); 
	print_threadpool_opts(fp, args_info, prefix); 
	print_naming_server_opts(fp, args_info, prefix); 

	print_sysmon_opts(fp, args_info, prefix);

	fprintf(fp, "%s -----------------------------------\n", prefix);

	return rc; 
}



/**
 * @brief The LWFS storage server.
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK;
	lwfs_remote_pid authr_id; 
	lwfs_service authr_svc; 
	lwfs_service naming_svc; 

	/* command-line arguments */
	struct gengetopt_args_info args_info; 

	/* Parse command line options */
	if (cmdline_parser(argc, argv, &args_info) != 0) {
		exit(1);
	}


	/* initialize the logger */
	naming_debug_level = args_info.verbose_arg; 
	//rpc_debug_level=LOG_OFF;
	logger_init(args_info.verbose_arg, args_info.logfile_arg);

	if (args_info.daemon_flag) {
		int daemon_pid = fork(); 

		if (daemon_pid < 0) {  /* fork error */
			log_error(authr_debug_level, "could not fork process");
			return -1;
		}

		if (daemon_pid > 0) {  /* parent exits */
			exit(0);
		}

		/* child (daemon) continues */

		/* obtain a new process groupd for the daemon */
		setsid(); 

		//i = open("/dev/null", O_RDWR);  /* open stdin */
		//dup(i); /* stdout */
		//dup(i); /* stderr */
	}


	/* initialize RPC for the child */
	lwfs_ptl_init(PTL_IFACE_SERVER, args_info.naming_pid_arg);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR); 

	/* set the ID of this authorization service */
	authr_id.nid = args_info.authr_nid_arg; 
	authr_id.pid = args_info.authr_pid_arg; 

	/* get the authorization service */
	lwfs_get_service(authr_id, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to get authorization service");
		return rc; 
	}

	/* initialize the naming service */
	rc = naming_server_init(
			args_info.naming_db_path_arg, 
			args_info.naming_db_clear_flag, 
			args_info.naming_db_recover_flag,
			&authr_svc, 
			&naming_svc); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to initialize naming_srvr: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* print the arguments */
	print_args(logger_get_file(), &args_info, "");

	/* start processing requests */
	naming_svc.max_reqs = args_info.num_reqs_arg; 

	/* set the rpc_server max memory option */
	if (args_info.max_mem_allowed_arg > 0) {
		max_mem_allowed = args_info.max_mem_allowed_arg;
	}

	if (args_info.use_threads_flag) {
		lwfs_thread_pool_args tp_opts; 
		tp_opts.initial_thread_count = args_info.tp_init_thread_count_arg; 
		tp_opts.min_thread_count = args_info.tp_min_thread_count_arg; 
		tp_opts.max_thread_count = args_info.tp_max_thread_count_arg; 
		tp_opts.low_watermark = args_info.tp_low_watermark_arg; 
		tp_opts.high_watermark = args_info.tp_high_watermark_arg;
		rc = lwfs_service_start(&naming_svc, &tp_opts);
	}
	else {
		rc = lwfs_service_start(&naming_svc, NULL); 
		if (rc != LWFS_OK) {
			log_info(naming_debug_level, "exited service: %s",
					lwfs_err_str(rc));
		}
	}

cleanup:
	/* shutdown the naming service */
	naming_server_fini(&naming_svc);

	return rc; 
}


