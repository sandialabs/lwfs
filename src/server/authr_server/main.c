/**  @file main.c
 *   
 *   @brief Driver for the authorization server. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1562 $.
 *   $Date: 2007-09-11 18:30:26 -0600 (Tue, 11 Sep 2007) $.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <db.h>
#include <getopt.h>
#include <unistd.h>

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#if HAVE_GENGETOPT
#include "cmdline.h"
#else
#include "cmdline_default.h"
#endif

#include "support/logger/logger.h"
#include "support/logger/logger_opts.h"
#include "support/threadpool/thread_pool.h"
#include "support/threadpool/threadpool_opts.h"
#include "support/trace/trace.h"

#include "support/sysmon/sysmon_opts.h"

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/rpc_common/rpc_common.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/rpc_common/ptl_wrap.h"
#include "common/authr_common/authr_debug.h"

#include "server/rpc_server/rpc_server.h"

#include "cap.h"
#include "authr_server.h"
#include "authr_db.h"


/* -- globals -- */
/*static char *DEFAULT_DB_FNAME = "acls.db";*/

extern unsigned long max_mem_allowed;


static void print_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix) 
{
	lwfs_remote_pid myid; 

	lwfs_get_id(&myid);

    fprintf(fp, "%s ------------ AUTHR SERVER ---------\n", prefix);
    fprintf(fp, "%s nid = %llu, pid=%llu\n", prefix, 
			(unsigned long long)myid.nid, 
			(unsigned long long)myid.pid);

    fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
    fprintf(fp, "%s \tnum-reqs = %d\n", prefix, args_info->num_reqs_arg);
    fprintf(fp, "%s \tuse-threads = %s\n", prefix, (args_info->use_threads_flag)?"true":"false");
    fprintf(fp, "%s \tdaemon = %s\n", prefix, (args_info->daemon_flag)?"true":"false");


    fprintf(fp, "%s \tauthr-verify-caps = %s\n", prefix, 
			(args_info->authr_verify_caps_flag)?"true":"false");
    fprintf(fp, "%s \tauthr-db-path = %s\n", prefix, args_info->authr_db_path_arg);
    fprintf(fp, "%s \tauthr-db-clear = %s\n", prefix, 
			(args_info->authr_db_clear_flag)?"true":"false");
    fprintf(fp, "%s \tauthr-db-recover = %s\n", prefix, 
			(args_info->authr_db_recover_flag)?"true":"false");

	print_logger_opts(fp, args_info, prefix); 

    if (args_info->use_threads_flag) {
        print_threadpool_opts(fp, args_info, prefix); 
    }

	print_sysmon_opts(fp, args_info, prefix);

    fprintf(fp, "%s -----------------------------------\n", prefix);
}



/**
 * @brief The LWFS authorization server.
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK; 
	struct gengetopt_args_info args_info; 
	int daemon_pid = 0;

	/* service descriptors (only need one) */
	lwfs_service service;  


	/* Parse command line options to override defaults */
	if (cmdline_parser(argc, argv, &args_info) != 0) {
		exit(1);
	}

	/* Initialize the logger */
	authr_debug_level = args_info.verbose_arg;
//	rpc_debug_level=LOG_OFF;
	logger_init(args_info.verbose_arg, args_info.logfile_arg);

	/* initialize and enable tracing */
	if (args_info.authr_trace_flag) {
	    fprintf(stderr," main: initializing tracing\n");
	    trace_enable();
	    trace_init(args_info.authr_tracefile_arg, args_info.authr_traceftype_arg);
	}

	if (args_info.daemon_flag) {
		daemon_pid = fork(); 

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

		umask(0);

		//i = open("/dev/null", O_RDWR);  /* open stdin */
		//dup(i); /* stdout */
		//dup(i); /* stderr */
	}


	lwfs_ptl_init(PTL_IFACE_SERVER, args_info.authr_pid_arg);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

	/* print the arguments to standard out */
	print_opts(logger_get_file(), &args_info, ""); 

	/* initialize the auth server */
	lwfs_authr_srvr_init(
			args_info.authr_verify_caps_flag, 
			args_info.authr_db_path_arg, 
			args_info.authr_db_clear_flag, 
			args_info.authr_db_recover_flag, 
			&service); 

	/* start the server  */
	log_debug(authr_debug_level, "starting server");
	service.max_reqs = args_info.num_reqs_arg; 

	/* set the rpc_server max memory option */
	if (args_info.max_mem_allowed_arg > 0) {
		max_mem_allowed = args_info.max_mem_allowed_arg;
	}

	if (logging_debug(authr_debug_level)) {
		log_debug(authr_debug_level, "printing authr_svc");
		fprint_lwfs_service(logger_get_file(), "authr_svc", "", &service); 
	}

	if (args_info.use_threads_flag) {
		lwfs_thread_pool_args tp_opts; 
		tp_opts.initial_thread_count = args_info.tp_init_thread_count_arg; 
		tp_opts.min_thread_count = args_info.tp_min_thread_count_arg; 
		tp_opts.max_thread_count = args_info.tp_max_thread_count_arg; 
		tp_opts.low_watermark = args_info.tp_low_watermark_arg; 
		tp_opts.high_watermark = args_info.tp_high_watermark_arg; 
		rc = lwfs_service_start(&service, &tp_opts); 
	}
	else {
		/* send NULL tp_opts to signal single-threaded */
		rc = lwfs_service_start(&service, NULL); 
	}
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "error executing service driver");
		return rc;
	}

	/* finish the authr server */
	log_debug(authr_debug_level, "stopping server");
	rc = lwfs_authr_srvr_fini(&service);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "error stopping service driver");
		return rc;
	}

	/* release memory allocated for command-line arguments */
	cmdline_parser_free(&args_info);

	return rc; 
}

