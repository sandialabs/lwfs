/**  @file fuse-lwfs.c
 *   
 *   @brief Driver for FUSE-LWFS. 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 1073 $
 *   @date $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fuse.h>
#include <argp.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include "types/types.h"
#include "types/fprint_types.h"
#include "logger/logger.h"
#include "naming/naming_clnt.h"
#include "authr/authr_clnt.h"
#include "storage/ss_clnt.h"
#include "fuse-callbacks.h"
#include "fuse-debug.h"

void usage(char *pname);

/* --- private methods --- */
extern int fuse_callbacks_init(
	const lwfs_service *authr_svc, 
	const lwfs_service *naming_svc, 
	const lwfs_service *ss_svc); 


/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1073 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS";
static char args_doc[] = ""; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief Debug level to use. */
	log_level debug_level; 

	/** @brief Arguments to pass to fuse. */
	char *fuse_args; 

	/* --- Args for the naming service ------ */

	/** @brief Flag to run the storage service locally. */
	lwfs_bool naming_local; 

	/** @brief Process ID for the naming service. */
	lwfs_process_id naming; 

	/** @brief Path to the naming database file (if local). */
	char *naming_db_path; 

	/** @brief Flag to clear naming database  (if local).*/
	lwfs_bool naming_db_clear; 

	/** @brief Flag to recover naming db from crash (if local). */
	lwfs_bool naming_db_recover;


	/* --- Args for the authorization service --- */

	/** @brief Flag to run the auth service locally. */
	lwfs_bool authr_local; 

	/** @brief Process ID for the authorization service. */
	lwfs_process_id authr; 

	/** @brief Path to the authorization database file (if local). */
	char *authr_db_path; 

	/** @brief Flag to clear naming database  (if local). */
	lwfs_bool authr_db_clear; 

	/** @brief Flag to recover naming db from crash (if local). */
	lwfs_bool authr_db_recover;

	/** @brief Flag to fully verify capabilities (if local). */
	lwfs_bool authr_verify_caps;

	/** @brief Cache caps */
	lwfs_bool authr_cache_caps;

	/** @brief Flag to fully verify credentials (if local). */
	lwfs_bool authr_verify_creds;


	/* --- Args for the storage service ---- */

	/** @brief Flag to run the storage service locally. */
	lwfs_bool ss_local; 

	/** @brief Process ID for the storage service. */
	lwfs_process_id ss; 

	/** @brief Path to root directory used for the storage service (if local). */
	char *ss_root_dir; 
}; 

static int print_args(FILE *fp, struct arguments *args) 
{
	fprintf(fp, "------------  ARGUMENTS -----------\n");
	fprintf(fp, "\tverbose = %d\n", args->debug_level);
	fprintf(fp, "\tfuse-args = \"%s\"\n", args->fuse_args);

	/* naming svc */
	if (args->naming_local) {
		fprintf(fp, "\tUsing local naming service\n");
		fprintf(fp, "\t    naming-db-path = \"%s\"\n", args->naming_db_path);
		fprintf(fp, "\t    naming-db-clear = %s\n", 
				((args->naming_db_clear)? "true" : "false"));
		fprintf(fp, "\t    naming-db-recover = %s\n", 
				((args->naming_db_recover)? "true" : "false"));
	}
	else {
		fprintf(fp, "\tUsing remote naming service\n");
		fprintf(fp, "\t    naming-nid = %u\n", args->naming.nid);
		fprintf(fp, "\t    naming-pid = %u\n", args->naming.pid);
	}

	/* authr svc */
	if (args->authr_local) {
		fprintf(fp, "\tUsing local authorization service\n");
		fprintf(fp, "\t    authr-db-path = \"%s\"\n", args->authr_db_path);
		fprintf(fp, "\t    authr-db-clear = %s\n", 
				((args->authr_db_clear)? "true" : "false"));
		fprintf(fp, "\t    authr-db-recover = %s\n", 
				((args->authr_db_recover)? "true" : "false"));
		fprintf(fp, "\t    authr-verify-caps = %s\n", 
				((args->authr_verify_caps)? "true" : "false"));
		fprintf(fp, "\t    authr-cache-caps = %s\n", 
				((args->authr_cache_caps)? "true" : "false"));
		fprintf(fp, "\t    authr-verify-creds = %s\n", 
				((args->authr_verify_creds)? "true" : "false"));
	}
	else {
		fprintf(fp, "\tUsing remote authorization service\n");
		fprintf(fp, "\t    authr-nid = %u\n", args->authr.nid);
		fprintf(fp, "\t    authr-pid = %u\n", args->authr.pid);
	}

	/* storage svc */
	if (args->ss_local) {
		fprintf(fp, "\tUsing local storage service\n");
		fprintf(fp, "\t    ss-root-dir = \"%s\"\n", args->ss_root_dir);
	}
	else {
		fprintf(fp, "\tUsing remote storage service\n");
		fprintf(fp, "\t    ss-nid = %u\n", args->ss.nid);
		fprintf(fp, "\t    ss-pid = %u\n", args->ss.pid);
	}

	fprintf(fp, "-----------------------------------\n");

	return LWFS_OK;
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,...,1=all>", 0, "Produce verbose output"},
	{"fuse-args",  2, "<ARGS>", 0, "Arguments to pass to fuse"},

	{"naming-local",   10, 0, 0, "Run the storage svc locally"},
	{"naming-nid",     11, "<val>", 0, "Portals NID of the storage server"},
	{"naming-pid",     12, "<val>", 0, "Portals PID of the storage server"},
	{"naming-db-path",    13, "<FILE>", 0, "Path to naming database (local only)" },
	{"naming-db-clear",    14, 0, 0, "clear the naming db (local only)"},
	{"naming-db-recover",  15, 0, 0, "recover db from crash (local only)"},

	{"authr-local",       20, 0, 0, "Run the authr svc locally"},
	{"authr-nid",         21, "<val>", 0, "Portals NID of the authr server"},
	{"authr-pid",         22, "<val>", 0, "Portals PID of the authr server" },
	{"authr-db-path",     23, "<FILE>", 0, "Path to authr database (local only)" },
	{"authr-db-clear",    24, 0, 0, "clear the authr db (local only)"},
	{"authr-db-recover",  25, 0, 0, "recover db from crash (local only)"},
	{"authr-verify-caps", 26, 0, 0, "verify capabilities (local only)"},
	{"authr-verify-creds",27, 0, 0, "verify credentials (local only)"},
	{"authr-cache-caps",28, 0, 0, "cache caps"},

	{"ss-local",30, 0, 0, "Run the storage svc locally"},
	{"ss-nid",  31, "<val>", 0, "Portals NID of the storage server"},
	{"ss-pid",  32, "<val>", 0, "Portals PID of the storage server" },
	{"ss-root-dir",  33, "<DIR>", 0, "Directory used for the local storage service." },
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

		case 1: /* verbose */
			arguments->debug_level= atoi(arg);
			break;

		case 2: /* fuse arguments */
			arguments->fuse_args = arg;
			break;

		/* --- naming svc --- */

		case 10: /* naming_local */
			arguments->naming_local = TRUE;
			break;

		case 11: /* naming.nid */
			arguments->naming.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case 12: /* naming.pid */
			arguments->naming.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case 13: /* naming_db_path */
			arguments->naming_db_path = arg; 
			break;


		case 14: /* naming_db_clear */
			arguments->naming_db_clear = TRUE;
			break;

		case 15: /* naming_db_recover */
			arguments->naming_db_recover = TRUE;
			break;


		/* --- authr svc --- */

		case 20: /* authr_local */
			arguments->authr_local = TRUE; 
			break;

		case 21: /* authr.nid */
			arguments->authr.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case 22: /* authr.pid */
			arguments->authr.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case 23: /* db_path */
			arguments->authr_db_path = arg; 
			break;


		case 24: /* authr_db_clear */
			arguments->authr_db_clear = TRUE;
			break;

		case 25: /* authr_db_recover */
			arguments->authr_db_recover = TRUE;
			break;

		case 26: /* authr_verify_caps */
			arguments->authr_verify_caps = TRUE;

		case 27: /* authr_verify_creds */
			arguments->authr_verify_creds = TRUE;
			break;

		case 28:
			arguments->authr_cache_caps = TRUE;
			break;


		/* --- authr svc --- */

		case 30: /* ss_local */
			arguments->ss_local = TRUE;
			break;

		case 31: /* ss.nid */
			arguments->ss.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case 32: /* ss.pid */
			arguments->ss.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case 33: /* ss_root_dir */
			arguments->ss_root_dir = arg; 
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


/* initialize the argp structure for the command-line args */
static struct argp argp = {options, parse_opt, args_doc, doc}; 

static int parse_fuse_args(
		char *args, 
		int *argc, 
		char **argv) 
{
	char *token;

	/* arg count starts at 1, 0 reserved for program name */
	*argc = 1; 

	/* extract the first string */
	token = strtok(args, " "); 
	if (token != NULL) {
		argv[(*argc)++] = token; 
	}
	else 
		return 0; 

	while ((token = strtok(NULL, " ")) != NULL) {
		argv[(*argc)++] = token;
	}

	return 0; 
}


/* initialize the fuse operation callbacks */
static struct fuse_operations lwfs_fuse_oper = {
    .getattr	= lwfs_fuse_getattr,
    .readlink	= lwfs_fuse_readlink,
    .getdir	= lwfs_fuse_getdir,
    .mknod	= lwfs_fuse_mknod,
    .mkdir	= lwfs_fuse_mkdir,
    .symlink	= lwfs_fuse_symlink,
    .unlink	= lwfs_fuse_unlink,
    .rmdir	= lwfs_fuse_rmdir,
    .rename	= lwfs_fuse_rename,
    .link	= lwfs_fuse_link,
    .chmod	= lwfs_fuse_chmod,
    .chown	= lwfs_fuse_chown,
    .truncate	= lwfs_fuse_truncate,
    .utime	= lwfs_fuse_utime,
    .open	= lwfs_fuse_open,
    .read	= lwfs_fuse_read,
    .write	= lwfs_fuse_write,
    .statfs	= lwfs_fuse_statfs,
    .release	= lwfs_fuse_release,
    .fsync	= lwfs_fuse_fsync,
#ifdef HAVE_SETXATTR
    .setxattr	= lwfs_fuse_setxattr,
    .getxattr	= lwfs_fuse_getxattr,
    .listxattr	= lwfs_fuse_listxattr,
    .removexattr= lwfs_fuse_removexattr,
#endif
};

/**
 * Driver for LWFS fuse. 
 */
int main(int argc, char *argv[])
{
	int rc = LWFS_OK;   /* return code */
	int i=0;

	struct arguments args; 

	/* max 50 args for fuse */
	int fuse_argc = 0; 
	char *fuse_argv[50]; 

	lwfs_service authr_svc;   /* service descriptor for the authr svc */
	lwfs_service naming_svc;  /* service descriptor for the naming svc */
	lwfs_service storage_svc; /* service descriptor for the storage svc */

	lwfs_bool verify_creds = FALSE;
	lwfs_bool verify_caps = TRUE;

	/* default arg values */
	args.debug_level = LOG_ALL;
	args.fuse_args = "";

	args.naming_local = FALSE; 
	args.naming.nid = 0; 
	args.naming.pid = LWFS_NAMING_PID;
	args.naming_db_path = "naming.db";
	args.naming_db_clear = TRUE; 
	args.naming_db_recover = FALSE; 

	args.authr_local = FALSE; 
	args.authr.nid = 0; 
	args.authr.pid = LWFS_AUTHR_PID;
	args.authr_db_path = "authr.db";
	args.authr_db_clear = TRUE; 
	args.authr_db_recover = FALSE; 
	args.authr_verify_caps = verify_caps; 
	args.authr_cache_caps = FALSE; 
	args.authr_verify_creds = FALSE; 

	args.ss_local = FALSE; 
	args.ss.nid = 0; 
	args.ss.pid = LWFS_SS_PID;
	args.ss_root_dir = "./ss_root_dir";


	/* Parse the command-line arguments */
	rc = argp_parse(&argp, argc, argv, 0, 0, &args); 
	if (rc != 0) {
		fprintf(stderr, "error parsing arguments\n");
		return rc; 
	}

	print_args(stdout, &args); 

	/* initialize the logger */
	logger_set_file(stdout); 
	if (args.debug_level == 0)   {
		logger_set_default_level(LOG_OFF); 
	} else if (args.debug_level > 5)   {
		logger_set_default_level(LOG_ALL); 
	} else   {
		logger_set_default_level(args.debug_level - LOG_OFF); 
	}

	/* set fuse_debug_level */
	//fuse_call_debug_level = LOG_DEBUG; 

	/* initialize RPC */
	lwfs_rpc_init(PTL_PID_ANY);


	/* initialize the authr client */
	if (args.authr_local) {
		/* initialize a local client (no remote calls) */
		rc = lwfs_authr_clnt_init_local(verify_caps, 
				verify_creds, &authr_svc);
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "unable to initialize auth clnt: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}
	else {
		/* if the user didn't specify a nid for the storage server, assume one */
		if (args.authr.nid == 0) {
			lwfs_process_id my_id; 
			log_warn(args.debug_level, "nid not set: assuming server is on local node");

			lwfs_get_my_pid(&my_id);
			args.authr.nid = my_id.nid; 
		}

		/* Initialize the authr_svc descriptor. ...should look this up in a registry */
		/*
		authr_svc.remote_md.match_id.nid = args.authr.nid; 
		authr_svc.remote_md.match_id.pid = args.authr.pid; 
		authr_svc.remote_md.index = LWFS_AUTHR_INDEX; 
		authr_svc.remote_md.match_bits = LWFS_AUTHR_MATCH_BITS; 
		authr_svc.remote_md.len = LWFS_SHORT_REQUEST_SIZE; 
		authr_svc.remote_md.offset = 0; 
		*/

		/* get the svc description from the process_id */
		rc = lwfs_rpc_ping(args.authr, &authr_svc);
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "unable to ping authr svc: %s",
					lwfs_err_str(rc));
			return rc; 
		}

		/* initialize a remote client */
		rc = lwfs_authr_clnt_init(&authr_svc, args.authr_cache_caps);
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "unable to initialize auth clnt: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}


	/* initialize the naming service client */
	if (args.naming_local) {

		/* initialize a storage svc local client (local calls to storage svc) */
		rc = lwfs_naming_clnt_init_local(PTL_PID_ANY, 
				&authr_svc, args.authr_verify_caps, 
				args.authr_cache_caps, args.naming_db_path, 
				args.naming_db_clear, args.naming_db_recover, &naming_svc); 
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "unable to initialize naming_srvr: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}
	else {
		/* if the user didn't specify a nid for the naming server, assume one */
		if (args.naming.nid == 0) {
			lwfs_process_id my_id; 
			log_warn(args.debug_level, "nid not set: assuming server is on local node");

			lwfs_get_my_pid(&my_id);
			args.naming.nid = my_id.nid; 
		}

		/* manually initialize the service (we should look this up in a registry) */
		/*
		memset(&naming_svc, 0, sizeof(lwfs_service));
		naming_svc.remote_md.match_id.nid = (lwfs_ptl_nid)args.naming.nid; 
		naming_svc.remote_md.match_id.pid = (lwfs_ptl_pid)args.naming.pid; 
		naming_svc.remote_md.index  = (lwfs_ptl_index)LWFS_NAMING_INDEX; 
		naming_svc.remote_md.match_bits  = (lwfs_ptl_match_bits)LWFS_NAMING_MATCH_BITS;  
		naming_svc.remote_md.len = LWFS_SHORT_REQUEST_SIZE; 
		naming_svc.remote_md.offset = 0; 
		*/

		rc = lwfs_rpc_ping(args.naming, &naming_svc); 
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "could not get naming svc: %s",
				lwfs_err_str(rc));
			return rc; 
		}

		/* initialize a storage svc remote client */
		rc = lwfs_naming_clnt_init(&naming_svc);
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "failed to initialize naming_client: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	/* initialize the storage service client */
	if (args.ss_local) {

		/* initialize a storage svc local client (local calls to storage svc) */
		rc = lwfs_ss_clnt_init_local(args.ss.nid, args.ss.pid,
				&authr_svc, args.authr_verify_caps, 
				args.authr_cache_caps, 
				args.ss_root_dir, &storage_svc); 
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "unable to initialize storage svc: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}
	else {
		/* if the user didn't specify a nid for the storage server, assume one */
		if (args.ss.nid == 0) {
			lwfs_process_id my_id; 
			log_warn(args.debug_level, "nid not set: assuming server is on local node");

			lwfs_get_my_pid(&my_id);
			args.ss.nid = my_id.nid; 
		}

		/* manually initialize the service (we should look this up in a registry) */
		/*
		memset(&storage_svc, 0, sizeof(lwfs_service));
		storage_svc.remote_md.match_id.nid = (lwfs_ptl_nid)args.ss.nid; 
		storage_svc.remote_md.match_id.pid = (lwfs_ptl_pid)args.ss.pid; 
		storage_svc.remote_md.index  = (lwfs_ptl_index)LWFS_SS_INDEX; 
		storage_svc.remote_md.match_bits  = (lwfs_ptl_match_bits)LWFS_SS_MATCH_BITS;  
		storage_svc.remote_md.len = LWFS_SHORT_REQUEST_SIZE; 
		storage_svc.remote_md.offset = 0; 
		*/

		/* get the svc description from the process_id */
		rc = lwfs_rpc_ping(args.ss, &storage_svc);
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "unable to ping storage svc: %s",
					lwfs_err_str(rc));
			return rc; 
		}

		/* initialize a storage svc remote client */
		rc = lwfs_ss_clnt_init(&storage_svc);
		if (rc != LWFS_OK) {
			log_error(args.debug_level, "failed to initialize storage service client: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}


	/* print the arguments */
	print_args(stdout, &args); 

	/* parse the fuse arguments */
	rc = parse_fuse_args(args.fuse_args, &fuse_argc, fuse_argv); 
	fuse_argv[0] = argv[0];  /* program name */

	for (i=0; i<fuse_argc; i++) {
		fprintf(stdout, "%s\n", fuse_argv[i]);
	}

	log_debug(fuse_debug_level, "starting fuse");

	/* initialize data structures for the callbacks */
	rc = fuse_callbacks_init(&authr_svc, &naming_svc, &storage_svc); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "unable to initialize fuse callbacks: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* call the fuse driver */
	rc = fuse_main(fuse_argc, fuse_argv, &lwfs_fuse_oper);

	/* cleanup */

	/* close the storage server client */
	/* close the authr server client */
	/* close the naming server client */
	return rc; 
}

