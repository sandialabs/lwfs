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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "types/types.h"
#include "types/fprint_types.h"
#include "logger/logger.h"
#include "naming/naming_clnt.h"
#include "naming/naming_debug.h"
#include "storage/ss_clnt.h"
#include "authr/authr_clnt.h"
#include "authr/authr_srvr.h"
#include "perf/perf.h"

enum op {
	CREATE_OBJ = 1,
	REMOVE_OBJ, 
	STAT_OBJ, 

	CREATE_FILE, 
	REMOVE_FILE, 

	CREATE_DIR, 
	REMOVE_DIR,

	LOOKUP,
	GET_DIR,
	STAT_DIR,
	STAT_FILE,

	PING, 

	UFS_CREATE,
	UFS_REMOVE,
	UFS_MKDIR,
	UFS_RMDIR,
	UFS_STAT
};

static const char *type_to_str(enum op type) {

	switch (type) {
		case CREATE_OBJ:
			return "CREATE_OBJ";

		case REMOVE_OBJ:
			return "REMOVE_OBJ";

		case STAT_OBJ:
			return "STAT_OBJ";

		case CREATE_FILE:
			return "CREATE_FILE";

		case REMOVE_FILE:
			return "REMOVE_FILE";

		case CREATE_DIR:
			return "CREATE_DIR";

		case REMOVE_DIR:
			return "REMOVE_DIR";

		case LOOKUP:
			return "LOOKUP";

		case GET_DIR:
			return "GET_DIR";

		case STAT_DIR:
			return "STAT_DIR";

		case STAT_FILE:
			return "STAT_FILE";

		case PING:
			return "PING";

		case UFS_CREATE:
			return "UFS_CREATE";

		case UFS_REMOVE:
			return "UFS_REMOVE";

		case UFS_MKDIR:
			return "UFS_MKDIR";

		case UFS_RMDIR:
			return "UFS_RMDIR";

		case UFS_STAT:
			return "UFS_STAT";

		default:
			return "UNDEFINED_TYPE"; 
	}
}

static int type_to_int(const char *str) 
{
	int i=CREATE_OBJ; 

	while (strcasecmp(type_to_str(i), "UNDEFINED_TYPE") != 0) {
		if (strcasecmp(str, type_to_str(i)) == 0) {
			return i; 
		}
		i++; 
	}

	fprintf(stderr, "Undefined type \"%s\". Valid types are: \n", str);
	i = CREATE_OBJ;

	while (strcasecmp(type_to_str(i), "UNDEFINED_TYPE") != 0) {
		fprintf(stderr, "\t%s\n", type_to_str(i++));
	}

	return -1; 
}

/* --- private methods --- */


static void output_stats(
		FILE *result_fp, 
		int num_ops, 
		int size, 
		int count, 
		double *time)
{
	int i; 
	int num_clients; 
	int total_ops; 
	int rank; 

	MPI_Comm_size(MPI_COMM_WORLD, &num_clients); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 

	/* gather the total number of ops */
	MPI_Reduce(&num_ops, &total_ops, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); 

	if (rank == 0) {

		fprintf(result_fp, "%s -----------------------------------\n", "%");
		fprintf(result_fp, "%s clients      ", "%"); 
		fprintf(result_fp, "num_ops    "); 
		fprintf(result_fp, "size       "); 
		fprintf(result_fp, "data[0](sec) data[1](sec) ...\n"); 

		/* print the row */
		fprintf(result_fp, "%09d   ", num_clients);    
		fprintf(result_fp, "%09d   ", total_ops);    
		fprintf(result_fp, "%09d   ", size);    

		/* print the sample timings */
		for (i=0; i<count; i++) {
			fprintf(result_fp, "%1.6e  ", time[i]); 
		}
		fprintf(result_fp, "\n"); 
		fprintf(result_fp, "%s -----------------------------------\n", "%");
	}
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

	/** @brief the size of the input/output buffer to use for tests. */
	int bufsize; 

	/** @brief The number of experiments to run. */
	int count; 

	/** @brief The type of experiment. */
	enum op type; 

	/** @brief The number of ops/experiment. */
	int num_ops; 

	/** @brief The size of the target directory. */
	int size; 

	/** @brief Where to output results. */
	char *result_file; 

	/** @brief Mode to use when opening file. */
	char *result_file_mode; 

	/** @brief The number of ops to run to get the system to a steady state. */
	int precount; 




	/** @brief Flag to run the storage service locally. */
	lwfs_bool ss_local; 

	/** @brief Process ID of the storage service. */
	lwfs_process_id ss; 


	/** @brief Flag to run the naming service locally. */
	lwfs_bool naming_local; 

	/** @brief Process ID of the naming service */
	lwfs_process_id naming; 

	/** @brief Clear the database. */
	lwfs_bool naming_db_clear; 

	/** @brief Recover from a crash. */
	lwfs_bool naming_db_recover; 

	/** @brief Path to the database file. */
	char *naming_db_path; 



	/** @brief Flag to run the auth service locally. */
	lwfs_bool authr_local; 

	/** @brief Process ID of the authorization service. */
	lwfs_process_id authr; 
	
	/** @brief Clear the database. */
	lwfs_bool authr_db_clear; 

	/** @brief Recover from a crash. */
	lwfs_bool authr_db_recover; 

	/** @brief Path to the database file. */
	char *authr_db_path; 

	/** @brief Verify credentials. */
	lwfs_bool authr_verify_creds; 

	/** @brief Verify caps. */
	lwfs_bool authr_verify_caps; 

	/** @brief Cache caps. */
	lwfs_bool authr_cache_caps; 


	/** @brief Path to scratch directory */
	char *scratch; 

	lwfs_bool ufs_test; 
}; 


static int print_args(FILE *fp, struct arguments *args) 
{
	int rc = 0; 
	const char *prefix = "%"; 

	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	fprintf(fp, "%s \ttype = %s\n", prefix, type_to_str(args->type));
	fprintf(fp, "%s \tverbose = %d\n", prefix, args->debug_level);
	fprintf(fp, "%s \tbufsize = %d\n", prefix, args->bufsize);
	fprintf(fp, "%s \tcount = %d\n", prefix, args->count);
	fprintf(fp, "%s \tnum-ops = %d\n", prefix, args->num_ops);
	fprintf(fp, "%s \tsize = %d\n", prefix, args->size);
	fprintf(fp, "%s \tprecount = %d\n", prefix, args->precount);
	fprintf(fp, "%s \tscratch = %s\n", prefix, args->scratch);
	fprintf(fp, "%s \tresult-file = %s\n", prefix, args->result_file);
	fprintf(fp, "%s \tresult-file-mode = %s\n", prefix, args->result_file_mode);

	if (args->ufs_test) {
		fprintf(fp, "%s \tufs-test\n", prefix);
	}
	else {
		fprintf(fp, "%s \tlwfs-test...\n", prefix);
		fprintf(fp, "%s \tnaming-local = %s\n", prefix, ((args->naming_local)? "true" : "false"));
		if (!args->naming_local) {
			fprintf(fp, "%s \tnaming-nid = %u\n", prefix, args->naming.nid);
			fprintf(fp, "%s \tnaming-pid = %u\n", prefix, args->naming.pid);
		}
		else {
			fprintf(fp, "%s \tnaming-db-path = %s\n", prefix, args->naming_db_path);
			fprintf(fp, "%s \tnaming-db-clear = %s\n", prefix, 
					((args->naming_db_clear)? "true" : "false"));
			fprintf(fp, "%s \tnaming-db-recover = %s\n", prefix, 
					((args->naming_db_recover)? "true" : "false"));
		}
		fprintf(fp, "%s \tauthr-verify-creds = %s\n", prefix, ((args->authr_verify_creds)? "true" : "false"));
		fprintf(fp, "%s \tauthr-verify-caps = %s\n", prefix, ((args->authr_verify_caps)? "true" : "false"));
		fprintf(fp, "%s \tauthr-cache-caps = %s\n", prefix, ((args->authr_cache_caps)? "true" : "false"));
		fprintf(fp, "%s \tauthr-local = %s\n", prefix, ((args->authr_local)? "true" : "false"));
		if (!args->authr_local) {
			fprintf(fp, "%s \tauthr-nid = %u\n", prefix, args->authr.nid);
			fprintf(fp, "%s \tauthr-pid = %u\n", prefix, args->authr.pid);
		}
		else {
			fprintf(fp, "%s \tauthr-db-path = %s\n", prefix, args->authr_db_path);
			fprintf(fp, "%s \tauthr-db-clear = %s\n", prefix, 
					((args->authr_db_clear)? "true" : "false"));
			fprintf(fp, "%s \tauthr-db-recover = %s\n", prefix, 
					((args->authr_db_recover)? "true" : "false"));
		}
		fprintf(fp, "%s \tss-local = %s\n", prefix, ((args->ss_local)? "true" : "false"));
		if (!args->ss_local) {
			fprintf(fp, "%s \tss-nid = %u\n", prefix, args->ss.nid);
			fprintf(fp, "%s \tss-pid = %u\n", prefix, args->ss.pid);
		}
	}
	fprintf(fp, "%s -----------------------------------\n", prefix);

	return rc; 
}


static struct argp_option options[] = {
	{"verbose",    1, "<0=none,...,1=all>", 0, "Produce verbose output"},
	{"bufsize",    2, "<val>", 0, "Size of buffer to use in read/write tests"},
	{"count",      3, "<val>", 0, "Number of experiments to run"},
	{"num-ops",    4, "<val>", 0, "Number of ops per experiment"},
	{"result-file",5, "<FILE>", 0, "Results file"},
	{"result-file-mode",6, "<val>", 0, "Results file mode"},
	{"type",7, "<OP NAME>", 0, "type of experiment (e.g., CREATE_OBJ, CREATE_DIR, ..."},
	{"size",    8, "<val>", 0, "Size of the target"},
	{"precount",    9, "<val>", 0, "Number of ops to run to get system to steady state."},

	{"naming-local",     10, 0, 0, "Run the naming svc locally [yes,no]"},
	{"naming-nid",       11, "<val>", 0, "Portals NID of the naming server"},
	{"naming-pid",       12, "<val>", 0, "Portals PID of the naming server"},
	{"naming-db-clear",  13, 0, 0, "clear the naming database"},
	{"naming-db-recover",14, 0, 0, "recover the naming database"},
	{"naming-db-path",   15, "<FILE>", 0, "path to the naming database"},

	{"authr-local",     20, 0, 0, "Run the authr svc locally [yes,no]"},
	{"authr-nid",       21, "<val>", 0, "Portals NID of the authr server"},
	{"authr-pid",       22, "<val>", 0, "Portals PID of the authr server" },
	{"authr-db-clear",  23, 0, 0, "clear the authr db (local only)"},
	{"authr-db-recover",24, 0, 0, "recover db from crash (local only)"},
	{"authr-db-path",   25, "<FILE>", 0, "Path to authr database (local only)" },
	{"authr-verify-creds",26, 0, 0, "Verify credentials"},
	{"authr-verify-caps",27, 0, 0, "Verify capabilities"},
	{"authr-cache-caps",28, 0, 0, "Cache capabilities"},

	{"ss-local",   30, 0, 0, "Run the storage svc locally [yes,no]"},
	{"ss-nid",     31, "<val>", 0, "Portals NID of the storage server"},
	{"ss-pid",     32, "<val>", 0, "Portals PID of the storage server"},

	{"scratch",     40, "<PATH>", 0, "Path to scratch directory"},
	{"ufs-test",    41, 0, 0, "boolean to signal a UFS test"},

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

		case 4: /* num_ops */
			arguments->num_ops= atoi(arg);
			break;

		case 5: /* result_file */
			arguments->result_file = arg;
			break;

		case 6: /* result_file_mode */
			arguments->result_file_mode = arg;
			break;

		case 7: /* type */
			{
				int i = type_to_int(arg); 
				if (i == -1) {
					argp_usage(state);
				}
				else 
					arguments->type = i; 
			}
			break;

		case 8: /* size */
			arguments->size= atoi(arg);
			break;

		case 9: /* precount */
			arguments->precount= atoi(arg);
			break;







		case 10: /* naming_local */
			arguments->naming_local = TRUE;
			break;

		case 11: /* naming.nid */
			arguments->naming.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case 12: /* naming.pid */
			arguments->naming.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case 13: /* naming_db_clear */
			arguments->naming_db_clear = TRUE;
			break;

		case 14: /* naming_db_recover */
			arguments->naming_db_recover = TRUE;
			break;

		case 15: /* naming_db_recover */
			arguments->naming_db_path = arg;
			break;





		case 20: /* authr_local */
			arguments->authr_local = TRUE;
			break;

		case 21: /* authr.nid */
			arguments->authr.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case 22: /* authr.pid */
			arguments->authr.pid = (lwfs_ptl_pid)atoll(arg);
			break;

		case 23: /* authr_db_clear */
			arguments->authr_db_clear = TRUE;
			break;

		case 24: /* authr_db_recover */
			arguments->authr_db_recover = TRUE;
			break;

		case 25: /* authr_db_recover */
			arguments->authr_db_path = arg;
			break;

		case 26: /* authr_verify_creds */
			arguments->authr_verify_creds = TRUE;
			break;

		case 27: /* authr_verify_caps */
			arguments->authr_verify_caps = TRUE;
			break;

		case 28: /* authr_cache_caps */
			arguments->authr_cache_caps = TRUE;
			break;




		case 30: /* ss_local */
			arguments->ss_local = TRUE;
			break;

		case 31: /* ss.nid */
			arguments->ss.nid = (lwfs_ptl_nid)atoll(arg);
			break;

		case 32: /* ss-pid */
			arguments->ss.pid = (lwfs_ptl_pid)atoll(arg);
			break;



		case 40: /* scratch */
			arguments->scratch = arg; 
			break;

		case 41: /* ufs-test */
			arguments->ufs_test = 1; 
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
 * @brief Create a container and acls for testing the storage service.
 *
 * To properly test the storage service, we need a container for the 
 * objects we wish to create, write to, and read from.  So, we need to
 * create a container, modify the acl to allow the user to perform the 
 * operations.  In this case, we really only need capabilities to write 
 * to the container. 
 *
 * @param cred @input the user's credential. 
 * @param cid @output the generated container ID. 
 * @param modacl_cap @output the cap that allows the user to modify acls for the container.
 * @param write_cap @output the cap that allows the user to 
 */
static int get_perms(
		const lwfs_cred *cred,
		lwfs_cid *cid, 
		const lwfs_opcode opcodes,
		lwfs_cap *cap)
{
	int rc = LWFS_OK;  /* return code */

	lwfs_txn *txn_id = NULL;  /* not used */
	lwfs_cap create_cid_cap;  
	lwfs_cap modacl_cap;
	lwfs_uid_array uid_array; 


	/* get the capability to create the container (for now anyone can create cids) */
	memset(&create_cid_cap, 0, sizeof(lwfs_cap)); 
	
	log_debug(naming_debug_level, "create container");
	fprint_lwfs_cap(stderr, "create_cid_cap", "", &create_cid_cap);

	/* create the container */
	rc = lwfs_bcreate_container(txn_id, &create_cid_cap, &modacl_cap);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to create a container: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* initialize the cid */
	*cid = modacl_cap.data.cid; 

	/* initialize the uid array */
	uid_array.lwfs_uid_array_len = 1; 
	uid_array.lwfs_uid_array_val = (lwfs_uid *)&cred->data.uid;

	log_debug(naming_debug_level, "create acl");
	fprint_lwfs_uid_array(stderr, "uids", "", &uid_array);

	/* create the acls */
	rc = lwfs_bcreate_acl(txn_id, *cid, opcodes, &uid_array, &modacl_cap);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to create write acl: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* get the cap */
	rc = lwfs_bget_cap(*cid, opcodes, cred, cap);
        if (rc != LWFS_OK) {
                log_error(naming_debug_level, "unable to call getcaps: %s",
                                lwfs_err_str(rc));
                return rc;
        }

	return rc; 
}


/* ---------------------- UFS OPERATIONS ------------- */

struct lookup_args {
	int *indices;
	int size; 
	lwfs_ns_entry *dir;
	char *prefix;
	lwfs_cap *cap;
}; 

static int ufs_stat(const int i, struct lookup_args *args)
{
	static char name[LWFS_NAME_LEN];
	static struct stat attr; 
	int rc=0; 


	/* construct the filename */
	sprintf(name, "%s_%d", args->prefix, args->indices[i%args->size]);

	/* stat the file */
	rc = stat(name, &attr); 
	if (rc != 0) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	return rc; 
}

struct ufs_args {
	char *prefix; 
	int flags; 
	mode_t mode; 
}; 

static int ufs_open(const int i, struct ufs_args *args)
{
	static char name[LWFS_NAME_LEN];
	int rc=0; 
	int fd; 

	/* construct the filename */
	sprintf(name, "%s_%d", args->prefix, i);


	/* open the file */
	fd = open(name, args->flags, args->mode); 
	if (fd == -1) {
		log_error(naming_debug_level, "could not open %s: %s\n", name, strerror(errno)); 
		rc = LWFS_ERR; 
		return rc; 
	}

	close(fd); 

	return rc; 
}

static int ufs_unlink(const int i, struct ufs_args *args)
{
	static char name[LWFS_NAME_LEN];
	int rc=0; 


	/* construct the filename */
	sprintf(name, "%s_%d", args->prefix, i);

	/* stat the file */
	rc = unlink(name);
	if (rc != 0) {
		log_error(naming_debug_level, "could not create %s: %s\n", name, strerror(errno)); 
	}

	return rc; 
}

static int ufs_mkdir(const int i, struct ufs_args *args)
{
	static char name[LWFS_NAME_LEN];
	int rc=0; 


	/* construct the filename */
	sprintf(name, "%s_%d", args->prefix, i);

	/* make the directory */
	rc = mkdir(name, args->mode);
	if (rc != 0) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	return rc; 
}

static int ufs_rmdir(const int i, struct ufs_args *args)
{
	static char name[LWFS_NAME_LEN];
	int rc=0; 


	/* construct the filename */
	sprintf(name, "%s_%d", args->prefix, i);

	/* make the directory */
	rc = rmdir(name);
	if (rc != 0) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	return rc; 
}



/* ---------------------- LWFS OPERATIONS ------------- */

struct obj_args {
	int size; 
	lwfs_service *ss; 
	lwfs_cap *cap;
	lwfs_cid cid; 
	int index; 
	lwfs_obj *obj; 
};


static int create_obj_op(const int i, struct obj_args *args)
{
	int rc=0; 
	int index = args->index; 

	/* create the object */
	rc = lwfs_bcreate_obj(args->ss, NULL, args->cid, args->cap, &(args->obj[index])); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create obj: %s",
				lwfs_err_str(rc));
		return rc; 
	}
	args->index++; 

	return rc; 
}

static int remove_obj_op(const int i, struct obj_args *args)
{
	int rc=0; 
	int index = args->index; 

	/* create the object */
	rc = lwfs_bremove_obj(NULL, &(args->obj[index]), args->cap); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create obj: %s",
				lwfs_err_str(rc));
		return rc; 
	}
	args->index++; 

	return rc; 
}

static int stat_obj_op(const int i, struct obj_args *args)
{
	int rc=0; 
	int index = args->index; 
	lwfs_obj_attr attr; 

	/* create the object */
	rc = lwfs_bget_attr(NULL, &(args->obj[index]), args->cap, &attr); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create obj: %s",
				lwfs_err_str(rc));
		return rc; 
	}
	args->index++; 

	return rc; 
}


struct file_args {
	lwfs_service *ss; 
	lwfs_ns_entry *dir;
	char *prefix;
	lwfs_cap *cap;
	lwfs_cid cid; 
	int count; 
};


static int create_file_op(const int i, struct file_args *args)
{
	static char name[LWFS_NAME_LEN];
	static lwfs_ns_entry ent; 
	static lwfs_obj obj; 
	int rc=0; 
	int index = args->count++; 

	sprintf(name, "%s-%d", args->prefix, index);

	//fprintf(stderr, "creating %s\n", name);  

	/* create the object to associate with the file */
	rc = lwfs_bcreate_obj(args->ss, NULL, args->cid, args->cap, &obj); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create obj: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc = lwfs_bcreate_file(NULL, args->dir, name, &obj, args->cap, &ent); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create file: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

static int remove_file_op(const int i, struct file_args *args)
{
	static char name[LWFS_NAME_LEN];
	static lwfs_ns_entry ent; 
	int rc=0; 
	int index = --args->count; 

	/* lookup the entry */
	sprintf(name, "%s-%d", args->prefix, index);

	//fprintf(stderr, "removing %s\n", name);  

	/* remove the file */ 
	rc = lwfs_bunlink(NULL, args->dir, name, args->cap, &ent); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create file: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* remove the object to associate with the file */
	rc = lwfs_bremove_obj(NULL, ent.file_obj, args->cap); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create obj: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

/**
 * Assign "len" random index values between 0 and "max_val"-1 to the index array.
 */
static void randomize_array(
		int *indices, 
		int len, 
		int max_val) 
{
	int i; 

	for (i=0; i<len; i++) {
		indices[i] = random() % max_val; 
	}
}



static int lookup_op(const int i, struct lookup_args *args)
{
	static char name[LWFS_NAME_LEN];
	static lwfs_ns_entry ent; 
	static const lwfs_lock_type lock_type = 0; 
	int rc=0; 


	/* lookup the entry */
	sprintf(name, "%s-%d", args->prefix, args->indices[i%args->size]);
	rc = lwfs_blookup(NULL, args->dir, name, 
			lock_type, args->cap, &ent); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to lookup file \"%s\": %s",
				name,
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

static int stat_op(const int i, struct lookup_args *args)
{
	static char name[LWFS_NAME_LEN];
	static lwfs_ns_entry ent; 
	static lwfs_obj_attr attr; 
	static const lwfs_lock_type lock_type = 0; 
	int rc=0; 


	/* lookup the entry */
	sprintf(name, "%s-%d", args->prefix, args->indices[i%args->size]);
	rc = lwfs_blookup(NULL, args->dir, name, 
			lock_type, args->cap, &ent); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to lookup entry \"%s\": %s",
				name,
				lwfs_err_str(rc));
		return rc; 
	}

	/* get the attribute */
	rc = lwfs_bget_attr(NULL, &ent.entry_obj, args->cap, &attr); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to get attribute of entry \"%s\": %s",
				name,
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

struct dir_args {
	lwfs_ns_entry *dir;
	char *prefix;
	lwfs_cap *cap;
	lwfs_cid cid; 
	int count; 
};


static int mkdir_op(const int i, struct dir_args *args)
{
	static char name[LWFS_NAME_LEN];
	static lwfs_ns_entry ent; 
	int rc=0; 
	int index = args->count++; 

	/* lookup the entry */
	sprintf(name, "%s-%d", args->prefix, index);

	//fprintf(stderr, "creating %s\n", name);  

	rc = lwfs_bcreate_dir(NULL, args->dir, name, 
			args->cid, args->cap, &ent); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to create dir \"%s\": %s",
				name,
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}


static int rmdir_op(const int i, struct dir_args *args)
{
	static char name[LWFS_NAME_LEN];
	static lwfs_ns_entry ent; 
	int rc=0; 
	int index = --args->count;

	/* remove the directory */
	sprintf(name, "%s-%d", args->prefix, index);

	//fprintf(stderr, "removing %s\n", name);  

	rc = lwfs_bremove_dir(NULL, args->dir, name, args->cap, &ent); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to remove dir \"%s\": %s",
				name,
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

static int ping_op(const int i, lwfs_process_id *id)
{
	int rc=0; 
	lwfs_service svc; 

	rc = lwfs_rpc_ping(*id, &svc); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to ping server: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}



/**
 * Tests for an SS client. 
 */
int main(int argc, char **argv)
{
	int rc = LWFS_OK;   /* return code */
	int i=0; 
	int nprocs, myproc;

	struct arguments args; 


	lwfs_opcode opcodes; 
	lwfs_cred cred; 
	lwfs_cap cap; 
	lwfs_cid cid=0;

	double *time; 

	char fprefix[256]; 

	FILE *result_fp = stdout; 

	lwfs_service naming_svc;
	lwfs_service authr_svc;
	lwfs_service storage_svc; 

	/* default arg values */
	args.type = 1;
	args.debug_level = LOG_ALL;
	args.count = 1; 
	args.bufsize = 1024; 
	args.num_ops = 1;
	args.size = 1;
	args.result_file = NULL; 
	args.precount = 100; 
	args.ufs_test = FALSE; 

	args.naming_local = FALSE; 
	args.naming.nid = 0; 
	args.naming.pid = LWFS_NAMING_PID;
	args.naming_db_path = "naming.db";
	args.naming_db_clear = FALSE; 
	args.naming_db_recover = FALSE; 

	args.ss_local = FALSE; 
	args.ss.nid = 0; 
	args.ss.pid = LWFS_SS_PID;

	args.authr.nid = 0;
	args.authr.pid = LWFS_AUTHR_PID;
	args.authr_local = FALSE;
	args.authr_db_path = "naming.db";
	args.authr_db_clear = FALSE; 
	args.authr_db_recover = FALSE; 
	args.authr_verify_creds = FALSE;
	args.authr_verify_caps = TRUE;
	args.authr_cache_caps = FALSE;

	args.scratch = NULL; 

	/* initialize MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myproc);

	fprintf(stderr, "I am %d/%d\n", myproc, nprocs); 
	MPI_Barrier(MPI_COMM_WORLD); 

	/* Parse the command-line arguments */
	argp_parse(&argp, argc, argv, 0, 0, &args); 

	if (args.size < (2*args.precount + args.num_ops)) {
		args.size = 2*args.precount + args.num_ops;
	}

	/* allocate space for the time array */
	time = (double *)malloc(args.count*sizeof(double));


	/* initialize the logger */
	logger_set_file(stdout); 
	if (args.debug_level == 0)   {
		logger_set_default_level(LOG_OFF); 
	} else if (args.debug_level > 5)   {
		logger_set_default_level(LOG_ALL); 
	} else   {
		logger_set_default_level(args.debug_level - LOG_OFF); 
	}

	if (args.authr_local && !args.naming_local) {
		log_error(args.debug_level, 
				"if authr runs local, naming svc must also run local"); 
		return LWFS_ERR; 
	}


	/* setup the LWFS services */
	if (!args.ufs_test) {

		/* initialize RPC */
		lwfs_rpc_init(128-myproc);


		/* initialize the authr client */
		if (args.authr_local) {
			/* initialize a local client (no remote calls) */
			rc = lwfs_authr_clnt_init_local(args.authr_verify_caps, 
					args.authr_verify_creds, &authr_svc);
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

			/* Ping the authr process ID to get the authr svc */
			rc = lwfs_rpc_ping(args.authr, &authr_svc); 
			if (rc != LWFS_OK) {
				log_error(args.debug_level, "unable to get authr svc");
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
			rc = lwfs_naming_clnt_init_local(PTL_PID_ANY, &authr_svc, 
					args.authr_verify_caps, 
					args.authr_cache_caps,
					args.naming_db_path, args.naming_db_clear, 
					args.naming_db_recover, &naming_svc); 
			if (rc != LWFS_OK) {
				log_error(args.debug_level, "unable to initialize naming_srvr: %s",
						lwfs_err_str(rc));
				return rc; 
			}
		}
		else {
			/* if the user didn't specify a nid for the storage server, assume one */
			if (args.naming.nid == 0) {
				lwfs_process_id my_id; 
				log_warn(args.debug_level, "nid not set: assuming server is on local node");

				lwfs_get_my_pid(&my_id);
				args.naming.nid = my_id.nid; 
			}

			/* Ping the naming process ID to get the naming svc */
			rc = lwfs_rpc_ping(args.naming, &naming_svc); 
			if (rc != LWFS_OK) {
				log_error(args.debug_level, "unable to get authr svc");
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
			lwfs_process_id my_id; 
			lwfs_get_my_pid(&my_id); 

			/* set the arguments  */
			args.ss.nid = my_id.nid; 
			args.ss.pid = my_id.pid; 

			/* initialize a storage svc local client (local calls to storage svc) */
			rc = lwfs_ss_clnt_init_local(args.ss.nid, args.ss.pid, 
					&authr_svc, args.authr_verify_caps, 
					args.authr_cache_caps, 
					"ss_root_dir", &storage_svc);
			if (rc != LWFS_OK) {
				log_error(args.debug_level, "unable to initialize storage server: %s",
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

			/* Ping the naming process ID to get the naming svc */
			rc = lwfs_rpc_ping(args.ss, &storage_svc); 
			if (rc != LWFS_OK) {
				log_error(args.debug_level, "unable to get authr svc");
				return rc; 
			}

			/* initialize a storage svc remote client */
			rc = lwfs_ss_clnt_init();
			if (rc != LWFS_OK) {
				log_error(args.debug_level, "failed to initialize storage client: %s",
						lwfs_err_str(rc));
				return rc; 
			}
		}

		opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ; 
		if (myproc == 0) {
			/* Create a container and allocate the capabilities necessary 
			 * to perform operations. For simplicity, all objs belong to 
			 * one container. 
			 */
			rc = get_perms(&cred, &cid, opcodes, &cap); 
			if (rc != LWFS_OK) {
				log_error(naming_debug_level, "unable to create a container and get caps: %s",
						lwfs_err_str(rc));
				return rc; 
			}
		}
		/* broadcast the capability to everyone */
		MPI_Bcast(&cid, sizeof(lwfs_cid), MPI_BYTE, 0, MPI_COMM_WORLD); 
		MPI_Bcast(&cap, sizeof(lwfs_cap), MPI_BYTE, 0, MPI_COMM_WORLD); 
	}


	if (myproc == 0) {
		/* print the arguments */
		print_args(result_fp, &args); 

		/* open the result file */
		if (args.result_file != NULL) {
			result_fp = fopen(args.result_file, args.result_file_mode); 
			if (result_fp == NULL) {
				log_warn(naming_debug_level, "could not open result file: using stdout");
				result_fp = stdout; 
			}
		}
	}


	sprintf(fprefix, "%s_test%d", (args.scratch == NULL)?"":args.scratch, myproc); 

	switch (args.type) {

		/* create directories in advance for some of the tests */
		case LOOKUP:
		case GET_DIR:
		case STAT_DIR:
			sprintf(fprefix, "%s_test0", (args.scratch == NULL)?"":args.scratch); 
			if (myproc == 0) {
				struct dir_args dir_args; 

				memset(&dir_args, 0, sizeof(dir_args));
				dir_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
				dir_args.prefix = fprefix; 
				dir_args.cap = &cap;
				dir_args.cid = cid; 
				dir_args.count = 0;

				fprintf(stderr, "args.size=%d\n", args.size);
				rc = perf_run(args.size, (perf_fptr)&mkdir_op, 
						&dir_args, 0, NULL);

				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not create files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
			break;

			/* create files in advance for some of the tests */
		case STAT_FILE:
			sprintf(fprefix, "%s_test0", (args.scratch == NULL)?"":args.scratch); 
			if (myproc == 0) {
				struct file_args file_args; 

				memset(&file_args, 0, sizeof(file_args));
				file_args.ss = &storage_svc;
				file_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
				file_args.prefix = fprefix; 
				file_args.cap = &cap;
				file_args.cid = cid; 
				file_args.count = 0; 

				/* create the files */
				fprintf(stderr, "args.size=%d\n", args.size);
				rc = perf_run(args.size, (perf_fptr)&create_file_op, 
						&file_args, 0, NULL);

				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not create files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
			break;

		case UFS_STAT:
			/* create files */
			{
				struct ufs_args ufs_args; 

				ufs_args.prefix = fprefix;
				ufs_args.flags  = O_CREAT|O_WRONLY|O_TRUNC;
				ufs_args.mode   = 0666; 

				/* create the objects */
				rc = perf_run(args.num_ops, (perf_fptr)&ufs_open, 
						&ufs_args, args.precount, NULL);
				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not create files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
			break; 

		default:
			break;
	}


	for (i=0; i<args.count; i++) {

		/* run the experiments */
		switch (args.type) {
			case UFS_CREATE:
				{
					struct ufs_args ufs_args; 

					ufs_args.prefix = fprefix;
					ufs_args.flags  = O_CREAT|O_WRONLY|O_TRUNC;
					ufs_args.mode   = 0666; 

					//fprintf(stderr, "%d: creating files at \"%s\"\n", (int)myproc, fprefix); 

					/* create the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_open, 
							&ufs_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_unlink, 
							&ufs_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not remove files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}

			case UFS_REMOVE:
				{
					struct ufs_args ufs_args; 

					ufs_args.prefix = fprefix;
					ufs_args.flags  = O_CREAT|O_WRONLY|O_TRUNC;
					ufs_args.mode   = 0666; 

					/* create the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_open, 
							&ufs_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_unlink, 
							&ufs_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not remove files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

			case UFS_MKDIR:
				{
					struct ufs_args ufs_args; 

					ufs_args.prefix = fprefix;
					ufs_args.flags  = O_CREAT|O_WRONLY|O_TRUNC;
					ufs_args.mode   = 0666; 

					/* create the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_mkdir, 
							&ufs_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_rmdir, 
							&ufs_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not remove files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

			case UFS_RMDIR:
				{
					struct ufs_args ufs_args; 

					ufs_args.prefix = fprefix;
					ufs_args.flags  = O_CREAT|O_WRONLY|O_TRUNC;
					ufs_args.mode   = 0666; 

					/* create the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_mkdir, 
							&ufs_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create dirs: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					rc = perf_run(args.num_ops, (perf_fptr)&ufs_rmdir, 
							&ufs_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not remove dirs: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

			case UFS_STAT: 
				{
					struct lookup_args lookup_args; 

					/* generate the random indices */
					int *indices = (int *)malloc(args.size*sizeof(int));
					memset(indices, 0, args.size*sizeof(int));
					randomize_array(indices, args.num_ops, args.size); 

					lookup_args.indices = indices; 
					lookup_args.size = args.size; 
					lookup_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					lookup_args.prefix = fprefix; 
					lookup_args.cap = &cap;

					rc = perf_run(args.num_ops, (perf_fptr)&ufs_stat, 
							&lookup_args, args.precount, &(time[i]));

					free(indices); 

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;


			case CREATE_OBJ:
				{
					struct obj_args obj_args; 

					/* allocate space for the objects */
					obj_args.obj = (lwfs_obj *)malloc(args.size*sizeof(lwfs_obj));
					memset(obj_args.obj, 0, args.size*sizeof(lwfs_obj));

					obj_args.ss = &storage_svc; 
					obj_args.size = args.size; 
					obj_args.cap = &cap;
					obj_args.cid = cid; 

					/* create the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&create_obj_op, 
							&obj_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&remove_obj_op, 
							&obj_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					free(obj_args.obj); 
				}
				break;

			case REMOVE_OBJ:
				{
					struct obj_args obj_args; 

					/* allocate space for the objects */
					obj_args.obj = (lwfs_obj *)malloc(args.size*sizeof(lwfs_obj));
					memset(obj_args.obj, 0, args.size*sizeof(lwfs_obj));

					obj_args.ss = &storage_svc; 
					obj_args.size = args.size; 
					obj_args.cap = &cap;
					obj_args.cid = cid; 

					/* create the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&create_obj_op, 
							&obj_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&remove_obj_op, 
							&obj_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					free(obj_args.obj); 
				}
				break;

			case STAT_OBJ:
				{
					struct obj_args obj_args; 

					/* allocate space for the objects */
					obj_args.obj = (lwfs_obj *)malloc(args.size*sizeof(lwfs_obj));
					memset(obj_args.obj, 0, args.size*sizeof(lwfs_obj));

					obj_args.ss = &storage_svc; 
					obj_args.size = args.size; 
					obj_args.cap = &cap;
					obj_args.cid = cid; 

					/* create the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&create_obj_op, 
							&obj_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* stat the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&stat_obj_op, 
							&obj_args, args.precount, &(time[i]));
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the objects */
					obj_args.index = 0; 
					rc = perf_run(args.num_ops, (perf_fptr)&remove_obj_op, 
							&obj_args, args.precount, NULL);
					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					free(obj_args.obj); 
				}
				break;

			case CREATE_FILE: 
				{
					struct file_args file_args; 

					memset(&file_args, 0, sizeof(file_args));
					file_args.ss = &storage_svc;
					file_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					file_args.prefix = fprefix; 
					file_args.cap = &cap;
					file_args.cid = cid; 
					file_args.count=0;

					/* create the files */
					rc = perf_run(args.num_ops, (perf_fptr)&create_file_op, 
							&file_args, args.precount, &(time[i]));

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the files */
					rc = perf_run(args.num_ops, (perf_fptr)&remove_file_op, 
							&file_args, args.precount, NULL);

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not remove files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

			case REMOVE_FILE: 
				{
					struct file_args file_args; 

					file_args.ss = &storage_svc;
					file_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					file_args.prefix = fprefix; 
					file_args.cap = &cap;
					file_args.cid = cid; 

					/* create the files */
					rc = perf_run(args.num_ops, (perf_fptr)&create_file_op, 
							&file_args, args.precount, NULL);

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the files */
					rc = perf_run(args.num_ops, (perf_fptr)&remove_file_op, 
							&file_args, args.precount, &(time[i]));

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not remove files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

			case CREATE_DIR: 
				{
					struct dir_args dir_args; 

					dir_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					dir_args.prefix = fprefix; 
					dir_args.cap = &cap;
					dir_args.cid = cid; 

					/* time creating the dirs */
					rc = perf_run(args.num_ops, (perf_fptr)&mkdir_op, 
							&dir_args, args.precount, &(time[i]));

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the directories */
					rc = perf_run(args.num_ops, (perf_fptr)&rmdir_op, 
							&dir_args, args.precount, NULL);

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

			case REMOVE_DIR: 
				{
					struct dir_args dir_args; 

					dir_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					dir_args.prefix = fprefix; 
					dir_args.cap = &cap;
					dir_args.cid = cid; 

					/* create the dirs */
					rc = perf_run(args.num_ops, (perf_fptr)&mkdir_op, 
							&dir_args, args.precount, NULL);

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}

					/* remove the directories */
					rc = perf_run(args.num_ops, (perf_fptr)&rmdir_op, 
							&dir_args, args.precount, &(time[i]));

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not create files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;

				/* random directory entry lookups */
			case LOOKUP: 
				{
					struct lookup_args lookup_args; 

					/* generate the random indices */
					int *indices = (int *)malloc(args.size*sizeof(int));
					memset(indices, 0, args.size*sizeof(int));
					randomize_array(indices, args.num_ops, args.size); 

					lookup_args.indices = indices; 
					lookup_args.size = args.size; 
					lookup_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					lookup_args.prefix = fprefix; 
					lookup_args.cap = &cap;

					rc = perf_run(args.num_ops, (perf_fptr)&lookup_op, 
							&lookup_args, args.precount, &(time[i]));

					free(indices); 

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;


			case GET_DIR: 
				/* time to get a directory listing */
				/*
				   rc = getdir(args.num_ops, LWFS_NAMING_ROOT,
				   &cap, &(time[i])); 
				   if (rc != LWFS_OK) {
				   log_error(naming_debug_level, "could not lookup files: %s",
				   lwfs_err_str(rc));
				   goto cleanup;
				   }
				 */
				break; 

				/* random stats on entries */
			case STAT_FILE: 
			case STAT_DIR: 
				{
					struct lookup_args lookup_args; 

					/* generate the random indices */
					int *indices = (int *)malloc(args.size*sizeof(int));
					memset(indices, 0, args.size*sizeof(int));
					randomize_array(indices, args.num_ops, args.size); 

					lookup_args.indices = indices; 
					lookup_args.size = args.size; 
					lookup_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
					lookup_args.prefix = fprefix; 
					lookup_args.cap = &cap;

					rc = perf_run(args.num_ops, (perf_fptr)&stat_op, 
							&lookup_args, args.precount, &(time[i]));

					free(indices); 

					if (rc != LWFS_OK) {
						log_error(naming_debug_level, "could not lookup files: %s",
								lwfs_err_str(rc));
						goto cleanup;
					}
				}
				break;


				/* ping the naming server */
			case PING: 
				/* ping the naming server */
				rc = perf_run(args.num_ops, (perf_fptr)&ping_op,
						&args.naming, args.precount, &(time[i]));	
				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not ping server: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
				break; 

			default:
				log_error(naming_debug_level, "unrecognized experiment type (%d)",
						args.type); 
				return LWFS_ERR; 
		}

		if (myproc == 0) {
			fprintf(stderr, "time[%d]=%g\n", i, time[i]); 
		}
	}

	/* cleanup after the tests */
	switch (args.type) {
		case LOOKUP:
		case GET_DIR:
		case STAT_DIR:
			if (myproc == 0) {
				struct dir_args dir_args; 

				memset(&dir_args, 0, sizeof(dir_args));
				dir_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
				dir_args.prefix = fprefix; 
				dir_args.cap = &cap;
				dir_args.count = args.size; 

				rc = perf_run(args.size, (perf_fptr)&rmdir_op, 
						&dir_args, 0, NULL);

				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not create files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
			break;

		case STAT_FILE:
			/* remove the files */
			if (myproc == 0) {
				struct file_args file_args; 

				memset(&file_args, 0, sizeof(file_args));
				file_args.ss = &storage_svc;
				file_args.dir = (lwfs_ns_entry *)LWFS_NAMING_ROOT; 
				file_args.prefix = fprefix; 
				file_args.cap = &cap;
				file_args.cid = cid; 
				file_args.count = args.size; 

				/* remove the files */
				rc = perf_run(args.size, (perf_fptr)&remove_file_op, 
						&file_args, 0, NULL);

				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not remove files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
			break; 

		case UFS_STAT:
			/* remove the files */
			{
				struct ufs_args ufs_args; 

				ufs_args.prefix = fprefix;

				/* remove the objects */
				rc = perf_run(args.num_ops, (perf_fptr)&ufs_unlink, 
						&ufs_args, args.precount, NULL);
				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "could not remove files: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
			break; 

		default:
			break;
	}

	output_stats(result_fp, args.num_ops, args.size, args.count, time); 

cleanup:

	/* clean up */ 
	rc = lwfs_naming_clnt_fini();
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed to shutdown naming_client: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	free(time); 

	MPI_Finalize();

	return rc; 
}
