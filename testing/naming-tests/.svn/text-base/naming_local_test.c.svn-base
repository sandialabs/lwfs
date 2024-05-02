/**  @file test_ss_create.c
 *   
 *   @brief Insert short description of file here. 
 *   
 *   Insert more detailed description of file here (or remove this line). 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 791 $
 *   @date $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <argp.h>
#include <sys/time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "client/naming_client/naming_client.h"

/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 791 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS storage service";
static char args_doc[] = ""; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief Debug level to use. */
	log_level debug_level; 

	/** @brief the size of the input/output buffer to use for tests. */
	int bufsize; 

	/** @brief The number of experiments to run. */
	int count; 


	struct authr_options authr_opts; 
	struct naming_options naming_opts; 
}; 

static int print_args(FILE *fp, struct arguments *args) 
{
	int rc = 0;

	fprintf(fp, "------------  ARGUMENTS -----------\n");
	fprintf(fp, "\tverbose = %d\n", args->debug_level);
	fprintf(fp, "\tbufsize = %d\n", args->bufsize);
	fprintf(fp, "\tcount = %d\n", args->count);

	print_naming_opts(fp, &args->naming_opts, "");
	print_authr_opts(fp, &args->authr_opts, "");
	print_ss_opts(fp, &args->ss_opts, "");

	fprintf(fp, "-----------------------------------\n");

	return rc; 
}

static struct argp_option options[] = {
	{"verbose",    1, "<0=none,...,1=all>", 0, "Produce verbose output"},
	{"bufsize",    2, "<val>", 0, "Size of buffer to use in read/write tests"},
	{"count",      3, "<val>", 0, "Number of tests to run"},

	{"ss-local",SS_LOCAL_OPT, 0, 0, 
		"Run the storage svc locally"},
	{"ss-nid",  SS_NID_OPT, "<val>", 0, 
		"NID of the storage server (if remote authr)."},
	{"ss-pid", SS_PID_OPT, "<val>", 0, 
		"PID of the storage server"},
	{"ss-root-dir",   SS_ROOT_DIR_OPT, "<DIR>", 0, 
		"Where to store obj files"},
	{"ss-iolib",   SS_IOLIB_OPT, "<val>", 0, 
		"Which I/O library to use for the storage server"},


	{"authr-verify-caps",AUTHR_VERIFY_CAPS_OPT, 0, 0, 
		"Verify caps"},
	{"authr-cache-caps",AUTHR_CACHE_CAPS_OPT, 0, 0, 
		"Cache caps"},
	{"authr-local",AUTHR_LOCAL_OPT, 0, 0, 
		"Run the authr svc locally"},
	{"authr-nid",  AUTHR_NID_OPT, "<val>", 0, 
		"Portals NID of the authr server (if remote authr)."},
	{"authr-pid",  AUTHR_PID_OPT, "<val>", 0, 
		"Portals PID of the authr server (if remote authr)." },
	{"authr-db-path",  AUTHR_DB_PATH_OPT, "<FILE>", 0, 
		"Path to db file." },
	{"authr-db-clear",  AUTHR_DB_CLEAR_OPT, 0, 0, 
		"Flag to clear DB." },
	{"authr-db-recover",  AUTHR_DB_RECOVER_OPT, 0, 0, 
		"Flag to recover DB after a crash." },


	{"tp-init-thread-count",  TP_INITIAL_COUNT_OPT, "<val>", 0, 
		"Initial number of threads in the pool." },
	{"tp-min-thread-count",  TP_MIN_COUNT_OPT, "<val>", 0, 
		"Minimum number of threads in the pool." },
	{"tp-max-thread-count",  TP_MAX_COUNT_OPT, "<val>", 0, 
		"Maximum number of threads in the pool." },
	{"tp-low-watermark",  TP_LOW_WATERMARK_OPT, "<val>", 0, 
		"Request queue size at which threads are removed from the pool." },
	{"tp-high-watermark",  TP_HIGH_WATERMARK_OPT, "<val>", 0, 
		"Request queue size at which threads are added to the pool." },


	{"naming-pid",  NAMING_PID_OPT, "<val>", 0, 
		"Portals PID of the naming server." },
	{"naming-db-path", NAMING_DB_PATH_OPT, "<FILE>", 0, 
		"path to the naming service database" },
	{"naming-db-clear",    NAMING_DB_CLEAR_OPT, 0, 0, 
		"clear the database before use"},
	{"naming-db-recover",  NAMING_DB_RECOVER_OPT, 0, 0, 
		"recover database after a crash"},

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

		case 2: /* bufsize */
			arguments->bufsize= atoi(arg);
			break;

		case 3: /* count */
			arguments->count= atoi(arg);
			break;


		/* naming options */
		case NAMING_PID_OPT: 
			arguments->naming_opts.id.pid = (lwfs_ptl_pid)atoll(arg);
			break;

        case NAMING_DB_CLEAR_OPT: 
            arguments->naming_opts.db_clear = TRUE;
            break;

        case NAMING_DB_RECOVER_OPT: /* naming_db_recover */
            arguments->naming_opts.db_recover = TRUE;
            break;

        case NAMING_DB_PATH_OPT: /* naming_db_recover */
            arguments->naming_opts.db_path = arg;
            break;



		/* storage options */
		case SS_LOCAL_OPT: 
			arguments->ss_opts.local = TRUE;
			break;

		case SS_PID_OPT: 
			arguments->ss_opts.id.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case SS_NID_OPT: 
			arguments->ss_opts.id.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case SS_ROOT_DIR_OPT: 
			arguments->ss_opts.root_dir = arg; 
			break;

		case SS_IOLIB_OPT: 
            if (strcasecmp(arg, "sysio") == 0) {
                arguments->ss_opts.iolib = SS_SYSIO;
            }
            else if (strcasecmp(arg, "aio") == 0) {
                arguments->ss_opts.iolib = SS_AIO;
            }
            else if (strcasecmp(arg, "simio") == 0) {
                arguments->ss_opts.iolib = SS_SIMIO;
            }
            else {
                log_error(naming_debug_level, "invalid library type");
                return ARGP_ERR_UNKNOWN; 
            }
			break;


		/* authorization service options */
		case AUTHR_LOCAL_OPT: 
			arguments->authr_opts.local = TRUE;
			break;

		case AUTHR_PID_OPT: 
			arguments->authr_opts.id.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case AUTHR_NID_OPT: 
			arguments->authr_opts.id.nid = (lwfs_ptl_nid)atoll(arg);
			break;

        case AUTHR_DB_CLEAR_OPT: 
            arguments->authr_opts.db_clear = TRUE;
            break;

        case AUTHR_DB_RECOVER_OPT: /* authr_db_recover */
            arguments->authr_opts.db_recover = TRUE;
            break;

        case AUTHR_DB_PATH_OPT: /* authr_db_recover */
            arguments->authr_opts.db_path = arg;
            break;

        case AUTHR_VERIFY_CREDS_OPT: /* authr_verify_creds */
            arguments->authr_opts.verify_creds = TRUE;
            break;

        case AUTHR_VERIFY_CAPS_OPT: /* authr_verify_caps */
            arguments->authr_opts.verify_caps = TRUE;
            break;

        case AUTHR_CACHE_CAPS_OPT: /* authr_cache_caps */
            arguments->authr_opts.cache_caps = TRUE;
            break;


		/* thread pool options */
		case TP_INITIAL_COUNT_OPT:
			arguments->tp_opts.initial_thread_count = atoi(arg);
			break;

		case TP_MIN_COUNT_OPT:
			arguments->tp_opts.min_thread_count = atoi(arg);
			break;

		case TP_MAX_COUNT_OPT:
			arguments->tp_opts.max_thread_count = atoi(arg);
			break;

		case TP_LOW_WATERMARK_OPT:
			arguments->tp_opts.low_watermark = atoi(arg);
			break;

		case TP_HIGH_WATERMARK_OPT:
			arguments->tp_opts.high_watermark = atoi(arg);
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


/* ----------- PRIVATE FUNCTIONS ------------ */


static int list(
		const char *prefix, 
		const char *name, 
		const lwfs_ns_entry *dir,
		const lwfs_cap *cap)
{
	int rc = LWFS_OK;
	char ostr[33];
	
	lwfs_ns_entry_array listing; 
	char subprefix[100]; 
	int i; 
	int len; 

	/* get the contents of the root directory */
	memset(&listing, 0, sizeof(lwfs_ns_entry_array));
	rc = lwfs_blist_dir(dir, cap, &listing);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not list dir: %s", 
			lwfs_err_str(rc));
		return rc; 
	}

	len = listing.lwfs_ns_entry_array_len; 

	if (len > 0) {
		fprintf(stdout, "%s %s, oid=0x%s, cid=%d, type=dir \n", 
				prefix, name, lwfs_oid_to_string(&dir->entry_obj.oid, ostr), (int)dir->entry_obj.cid);
		fprintf(stdout, "%s {\n", prefix);

		for (i=0; i<len; i++) {

			lwfs_ns_entry *ent = &listing.lwfs_ns_entry_array_val[i]; 

			switch (ent->entry_obj.type) {
				case LWFS_DIR_ENTRY:
					/* recursively list the directory contents */
					sprintf(subprefix, "%s    ", prefix); 
					list(subprefix, ent->name, ent, cap);
					break; 

				case LWFS_FILE_ENTRY:
					fprintf(stdout, "%s    %s, oid=%s, cid=%d, type=file\n",
							prefix, ent->name, 
							lwfs_oid_to_string(&ent->entry_obj.oid, ostr), 
							(int)ent->entry_obj.cid);
					break; 

				case LWFS_LINK_ENTRY:
					fprintf(stdout, "%s    %s, oid=%s, cid=%d, type=link\n",
							prefix, ent->name, 
							lwfs_oid_to_string(&ent->entry_obj.oid, ostr), 
							(int)ent->entry_obj.cid);
					break; 

				case LWFS_GENERIC_OBJ:
					fprintf(stdout, "%s    %s, oid=%s, cid=%d, type=generic\n",
							prefix, ent->name, 
							lwfs_oid_to_string(&ent->entry_obj.oid, ostr), 
							(int)ent->entry_obj.cid);
					break; 

				default:
					fprintf(stdout, "%s    %s, oid=%s, cid=%d, type=undefined\n",
							prefix, ent->name, 
							lwfs_oid_to_string(&ent->entry_obj.oid, ostr, 
							(int)ent->entry_obj.cid));
					break; 
			}
		}

		fprintf(stdout, "%s }\n", prefix); 
	}
	else {
		fprintf(stdout, "%s %s = { }\n", prefix, name);
	}

	free(listing.lwfs_ns_entry_array_val);
	return LWFS_OK;
}

int run_tests(
		const lwfs_service *authr_svc, 
		const lwfs_service *storage_svc,
		const lwfs_service *naming_svc)
{
	int rc = LWFS_OK;

	/* directories and file objects */
	lwfs_ns_entry bill, rob, leland, david, neil, ron, rolf;
	lwfs_ns_entry tmp_ent; 
	lwfs_obj obj; 

	lwfs_obj_attr attr; 

	/* credential of the user */
	lwfs_cred cred; 

	/* caps and opcode variables */
	lwfs_opcode opcodes;
	lwfs_cap cap; 

	/* variables needed for the naming tests */
	lwfs_txn *txn_id = NULL;
	lwfs_cid cid; 
	/* initialize the opcodes */
	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	/* initialize the credential */
	memset(&cred, 0, sizeof(lwfs_cred));

	/* Create a container and allocate the capabilities necessary 
	 * to perform operations. For simplicity, all objs belong to 
	 * one container.  */
	rc = get_perms(&cred, &cid, opcodes, &cap);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to create a container and get caps: %s",
			lwfs_err_str(rc));
		return rc; 
	}


	if (logging_debug(naming_debug_level)) {
		fprint_lwfs_cap(stdout, "cap", "DEBUG", &cap);
	}

	/* create a directory named "bill" from the root */
	rc = lwfs_bcreate_dir(txn_id, LWFS_NAMING_ROOT, "bill", cid, 
			&cap, &bill);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create dir: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* create a directory named "rob" under "bill" */
	rc = lwfs_bcreate_dir(txn_id, &bill, "rob", cid, 
			&cap, &rob);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create dir: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* create a directory named "david" under "bill" */
	rc = lwfs_bcreate_dir(txn_id, &bill, "david", cid, 
			&cap, &david);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create dir: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* create a directory named "neil" under "rob" */
	rc = lwfs_bcreate_dir(txn_id, &rob, "neil", cid, 
			&cap, &neil);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create dir: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* create a file named "ron" under "neil" */
	rc = lwfs_bcreate_obj(storage_svc, NULL, cid, &cap, &obj); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not create obj: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc = lwfs_bcreate_file(txn_id, &neil, "ron", &obj, &cap, &ron);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create dir: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* create a file named "rolf" under "neil" */
	rc = lwfs_bcreate_obj(storage_svc, NULL, cid, &cap, &obj); 
	rc = lwfs_bcreate_file(txn_id, &neil, "rolf", &obj, &cap, &rolf);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create dir: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* create a link under root called leland linked to rob under bill */
	rc = lwfs_bcreate_link(txn_id, LWFS_NAMING_ROOT, "leland (link)", &cap,
			&bill, "rob", &cap, &leland);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create link: %s", 
				lwfs_err_str(rc));
		return rc; 
	}
	rc = lwfs_bget_attr(txn_id, &leland.entry_obj, &cap, &attr); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to get attributes: %s",
				lwfs_err_str(rc));
		return rc; 
	}
	fprint_lwfs_obj_attr(stdout, "leland (link)", "", &attr);

	/* list the contents of the root directory */
	list("", "9000", LWFS_NAMING_ROOT, &cap);

	fprintf(stdout, "\n --- Remove file \"ron\" --- \n\n"); 

	/* remove the file "ron" */
	rc = lwfs_bunlink(txn_id, &neil, "ron", &cap, &tmp_ent);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not unlink: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* remove the object associated with ron */
	rc = lwfs_bremove_obj(txn_id, tmp_ent.file_obj, &cap);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not remove obj: %s", 
				lwfs_err_str(rc));
		return rc; 
	}


	/* list the contents of the bill directory */
	list("", "9000", LWFS_NAMING_ROOT, &cap);

	fprintf(stdout, "\n --- Remove file \"rolf\" --- \n\n"); 

	/* remove the file "rolf" */
	rc = lwfs_bunlink(txn_id, &neil, "rolf", &cap, &tmp_ent);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create link: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	rc = lwfs_bremove_obj(txn_id, tmp_ent.file_obj, &cap);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not remove obj: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* list the contents of the bill directory */
	list("", "9000", LWFS_NAMING_ROOT, &cap);


	fprintf(stdout, "\n --- Remove link \"leland (link)\" --- \n\n"); 

	/* remove the link "leland" */
	rc = lwfs_bunlink(txn_id, LWFS_NAMING_ROOT, "leland (link)", &cap, &tmp_ent);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not create link: %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* list the contents of the bill directory */
	list("", "9000", LWFS_NAMING_ROOT, &cap);


	fprintf(stdout, "\n --- Remove dir \"rob\" --- \n\n"); 


	/* remove the dir "rob" */
	rc = lwfs_bunlink(txn_id, &bill, "rob", &cap, &tmp_ent);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOTSUPP)) {
		log_error(naming_debug_level, "could not remove \"rob\": %s", 
				lwfs_err_str(rc));
		return rc; 
	}

	/* list the contents of the bill directory */
	list("", "9000", LWFS_NAMING_ROOT, &cap);

	rc = lwfs_bget_attr(txn_id, &bill.entry_obj, &cap, &attr); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to get attributes: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	fprint_lwfs_obj_attr(stdout, "bill_atttr", "", &attr);



	fprintf(stdout, "SUCCESS!\n");


	return LWFS_OK;
}





/**
 * Tests for an SS client. 
 */
int
main(int argc, char *argv[])
{
	int rc = LWFS_OK;
	struct arguments args; 

	lwfs_service authr_svc; 
	lwfs_service naming_svc; 
	lwfs_service *storage_svc; 
	

	/* default arg values */
	args.debug_level = LOG_ALL;
	args.count = 1; 
	args.bufsize = 1024; 

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
	args.ss_opts.iolib = SS_SYSIO;
	args.ss_opts.root_dir = "ss-root-dir";

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

	/* Parse the command-line arguments */
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

	if (args.authr_opts.local && !args.naming_opts.local) {
		log_error(args.debug_level, 
				"if authr runs local, naming svc must also run local"); 
		return LWFS_ERR; 
	}

	/* initialize RPC */
	lwfs_rpc_init(PTL_PID_ANY);

	/* initialize the authr client */
	rc = lwfs_authr_clnt_init(&args.authr_opts, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to initialize auth clnt: %s",
				lwfs_err_str(rc));
		return rc; 
	}


	/* initialize the storage service client */
	rc = lwfs_ss_clnt_init(&args.ss_opts, &args.authr_opts,
		&authr_svc, &storage_svc); 
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to initialize storage svc: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* initialize the naming service client */
	rc = lwfs_naming_clnt_init(&args.naming_opts, &args.authr_opts,
		&authr_svc, &naming_svc); 
	if (rc != LWFS_OK) {
		log_error(args.debug_level, "unable to initialize storage svc: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* print the arguments */
	print_args(stdout, &args); 

	run_tests(&authr_svc, storage_svc, &naming_svc);

	lwfs_naming_clnt_fini(&args.naming_opts, &naming_svc);
	lwfs_ss_clnt_fini(&args.ss_opts, &storage_svc);
	lwfs_authr_clnt_fini(&args.authr_opts, &authr_svc);

	lwfs_rpc_fini();

	return rc; 
}


