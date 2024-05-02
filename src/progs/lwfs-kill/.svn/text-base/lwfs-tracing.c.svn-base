/**
*/
#include "config.h"

#include <time.h>

#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

/*#include "portals3.h"*/

#include "trace_opts.h"
#include "support/logger/logger.h"
#include "support/logger/logger_opts.h"
#include "client/rpc_client/rpc_client.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/rpc_common/rpc_debug.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/rpc_common/rpc_common.h"

/* ----------------- COMMAND-LINE OPTIONS --------------- */


static int print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
	int rc= 0;
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tReset tracing on an LWFS service", prefix);

	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));


	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix); 
	fprintf(fp, "%s \t--server-nid = %u\n", prefix, args_info->server_nid_arg);
	fprintf(fp, "%s \t--server-pid = %u\n", prefix, args_info->server_pid_arg);
	fprintf(fp, "%s \t--trace-fname = %s\n", prefix, args_info->trace_fname_arg);
	fprintf(fp, "%s \t--trace-ftype = %d\n", prefix, args_info->trace_ftype_arg);
	fprintf(fp, "%s -----------------------------------\n", prefix);

	print_logger_opts(fp, args_info, prefix);
	

	fflush(fp);

	return rc; 
}



int
main (int argc, char *argv[])
{
	int rc; 
	struct gengetopt_args_info args_info; 
	int trace_enable_flag = -1; 

	lwfs_remote_pid svc_id; 
	lwfs_service svc; 

	if (cmdline_parser(argc, argv, &args_info) != 0) {
		exit(1);
	}

	/* initialize the logger */
	logger_init(args_info.verbose_arg, args_info.logfile_arg);

	/* initialize LWFS RPC */
	lwfs_ptl_init(PTL_IFACE_SERVER, PTL_PID_ANY);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

	if (logging_debug(rpc_debug_level)) {
	    print_args(logger_get_file(), &args_info, ""); 
	}

	/* get the remote service descriptor */
	svc_id.nid = args_info.server_nid_arg; 
	svc_id.pid = args_info.server_pid_arg; 
	rc = lwfs_get_service(svc_id, &svc);
	if (rc != LWFS_OK) {
		log_error(rpc_debug_level, "could not get svc description: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* reset the tracing on the remote service */

	if (args_info.trace_enable_flag) 
	    trace_enable_flag = 1; 
	else if (args_info.trace_disable_flag)
	    trace_enable_flag = 0;
	else
	    trace_enable_flag = -1;

	rc = lwfs_trace_reset(&svc, 
		trace_enable_flag,
		args_info.trace_fname_arg, 
		args_info.trace_ftype_arg);
	if (rc != LWFS_OK) {
		log_error(rpc_debug_level, "could not reset tracing: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	cmdline_parser_free(&args_info);
	lwfs_rpc_fini();

	return 0; 
}
