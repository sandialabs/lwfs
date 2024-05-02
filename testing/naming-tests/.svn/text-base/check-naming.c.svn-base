/**  @file naming-tests.c
 *   
 *   @brief Test the naming service API.
 *
 *   These tests only make sure that the APIs work correctly.
 *   The goal is not to measure performance. 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 791 $
 *   @date $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "cmdline.h"
#include "client/authr_client/authr_client.h"
#include "client/authr_client/authr_client_opts.h"
#include "client/naming_client/naming_client.h"
#include "client/naming_client/naming_client_opts.h"
#include "client/naming_client/naming_client_sync.h"
#include "client/storage_client/storage_client.h"
#include "client/storage_client/storage_client_opts.h"
#include "client/storage_client/storage_client_sync.h"
#include "support/timer/timer.h"
#include "support/logger/logger.h"
#include "support/logger/logger_opts.h"
#include "perms.h"


lwfs_remote_pid authr_id; 
lwfs_remote_pid naming_id; 
lwfs_remote_pid *storage_svc_ids; 

/* variables used in the test */
lwfs_service *storage_svc;   /* service descriptor for the storage service */
lwfs_service authr_svc;      /* service descriptor for the storage service */
lwfs_service naming_svc;     /* service descriptor for the naming service */

lwfs_cred cred; 
lwfs_opcode opcodes; 
lwfs_cap cap;
lwfs_cid cid = LWFS_CID_ANY; 

struct gengetopt_args_info args_info; 

/* ----------------- COMMAND-LINE OPTIONS --------------- */

static void print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
	print_logger_opts(fp, args_info, prefix); 
	print_authr_client_opts(fp, args_info, prefix); 
	print_storage_client_opts(fp, args_info, prefix); 
	print_naming_client_opts(fp, args_info, prefix); 
}


/* -------------- PRIVATE METHODS -------------- */


int test_result(FILE *fp, const char* func_name, int rc, int expected) {
	fprintf(fp, "testing  %-24s expecting rc=%-15s ... ", func_name, 
			lwfs_err_str(expected));
	if (rc != expected) {
		fprintf(fp, "FAILED (rc=%s)\n", lwfs_err_str(rc));
	}
	else {
		fprintf(fp, "PASSED\n");
	}
	return rc==expected;
}


int test_equiv(FILE *fp, const char* name, void *val, void *expected, lwfs_size size) {
	fprintf(fp, "testing  %-24s ... ", name);
	if (memcmp(val, expected, size) != 0) {
		fprintf(fp, "FAILED\n");
		return FALSE; 
	}
	else {
		fprintf(fp, "PASSED\n");
		return TRUE;
	}
}

int test_int(FILE *fp, const char* name, int val, int expected) {
	fprintf(fp, "   expecting %s=%d ... ", name, expected);
	if (val != expected) {
		fprintf(fp, "FAILED (%s=%d)\n", name, val);
	}
	else {
		fprintf(fp, "PASSED\n");
	}
	return val==expected;
}


int test_str(FILE *fp, const char* name, const char* val, const char *expected) {
	fprintf(fp, "   expecting %s=\"%s\" ... ", name, expected);
	if (strcmp(val, expected) != 0) {
		fprintf(fp, "FAILED (%s=%s)\n", name, val);
		return 0;
	}
	else {
		fprintf(fp, "PASSED\n");
		return 1;
	}
}

void run_tests()
{
	FILE *fp = stdout;
	const int num_servers = args_info.ss_num_servers_arg;

	int rc = LWFS_OK;
	lwfs_obj obj;
	const char *outbuf = "Hello world";
	char inbuf[256];
	lwfs_txn *txn = NULL;
	lwfs_size bytes_read = 0; 
	lwfs_lock_type lock_type = LWFS_LOCK_NULL;
	const char *dir1_str = "tmp";
	const char *dir2_str = "test";
	const char *file1_str = "file";
	const char *link_str = "link";
	char path[256];

	lwfs_ns_entry dir1, dir2;
	lwfs_ns_entry file1, file2; 
	lwfs_ns_entry link; 
	lwfs_ns_entry_array listing; 
	
	const lwfs_name ns_name = "naming-tests.ns";
	lwfs_namespace namespace;
	lwfs_ns_entry *namespace_root=NULL;

	memset(inbuf, 0, 256*sizeof(char));


	/* create the test namespace */
	rc = lwfs_create_namespace_sync(&naming_svc, txn, ns_name, cid, &namespace);
	sprintf(path, "lwfs_create_namespace(%s)", ns_name); 
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* get the test namespace */
	rc = lwfs_get_namespace_sync(&naming_svc, ns_name, &namespace);
	sprintf(path, "lwfs_get_namespace(%s)", ns_name); 
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	namespace_root = &namespace.ns_entry;

	/* create a directory */
	rc = lwfs_create_dir_sync(&naming_svc, txn, namespace_root, 
			dir1_str, cid, &cap, &dir1);
	sprintf(path, "lwfs_create_dir(/%s)", dir1_str); 
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* create a sub-dir directory */
	rc = lwfs_create_dir_sync(&naming_svc, txn, &dir1, 
			dir2_str, cid, &cap, &dir2);
	sprintf(path, "lwfs_create_dir(/%s/%s)", dir1_str, dir2_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* create file */

	/* initialize the object to associate with the file name */
	rc = lwfs_init_obj(storage_svc, LWFS_FILE_OBJ, cid, LWFS_OID_ANY, &obj);
	if (!test_result(fp, "lwfs_init_obj()", rc, LWFS_OK))
		return;

	rc = lwfs_create_obj_sync(txn, &obj, &cap); 
	if (!test_result(fp, "lwfs_create_obj()", rc, LWFS_OK))
		return;

	/* write outbuf to the object */
	rc = lwfs_write_sync(txn, &obj, 0, outbuf, strlen(outbuf), &cap);
	sprintf(path, "lwfs_write(%s)", outbuf); 
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* associate the object with the file */
	rc = lwfs_create_file_sync(&naming_svc, txn, &dir2, 
			file1_str, &obj, &cap, &file1);
	sprintf(path, "lwfs_create_file(/%s/%s/%s)", dir1_str, dir2_str, file1_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* lookup the file entry */
	rc = lwfs_lookup_sync(&naming_svc, txn, &dir2, 
			file1_str, lock_type, &cap, &file2);
	sprintf(path, "lwfs_lookup(/%s/%s/%s)",dir1_str, dir2_str, file1_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* make sure file_2 is the same as file1 */
	//if (!test_equiv(fp, "file object", file1.file_obj, file2.file_obj, sizeof(lwfs_obj)))
	//	return; 


	/* read the data in the file object */
	rc = lwfs_read_sync(txn, file2.file_obj, 0, inbuf, 256, &cap, &bytes_read);
	if (!test_result(fp, "lwfs_read(file.file_obj\")", 
				rc, LWFS_OK))
		return; 
	if (!test_int(fp, "bytes_read", bytes_read, strlen(outbuf)))
		return; 

	if (!test_str(fp, "file data", inbuf, outbuf))
		return; 

	/* create link */
	rc = lwfs_create_link_sync(&naming_svc, txn, 
			&dir2, link_str, &cap, 
			&dir2, file1_str, &cap, 
			&link);
	sprintf(path, "lwfs_create_link(%s,%s)",file1_str, link_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return; 

	/* list dir */
	rc = lwfs_list_dir_sync(&naming_svc, &dir2, &cap, &listing);
	sprintf(path, "lwfs_list_dir(/%s/%s)",dir1_str, dir2_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return; 
	{
		int dirlen = listing.lwfs_ns_entry_array_len;  
		if (!test_int(fp, "dirlen", dirlen, 2))
			return; 
		{
			lwfs_ns_entry *ent;
			
			ent = &listing.lwfs_ns_entry_array_val[0];
			sprintf(path, "entry[%d]", 0);
			if (!test_str(fp, path, ent->name, file1_str))
				return; 

			ent = &listing.lwfs_ns_entry_array_val[1];
			sprintf(path, "entry[%d]", 1);
			if (!test_str(fp, path, ent->name, link_str))
				return; 
		}
	}


	/* get attr */
	fprintf(fp, "STILL NEED TESTS FOR GETATTR...\n");

	/* unlink file1_str */
	rc = lwfs_unlink_sync(&naming_svc, 
			txn,
			&dir2, 
			file1_str,
			&cap, 
			&file1);
	sprintf(path, "lwfs_unlink(/%s/%s/%s)",dir1_str, dir2_str, file1_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return; 

	/* make sure unlink did the right thing */
	rc = lwfs_lookup_sync(&naming_svc, txn, &dir2, 
			file1_str, lock_type, &cap, &file2);
	sprintf(path, "lwfs_lookup(/%s/%s/%s)",dir1_str, dir2_str, file1_str);
	if (!test_result(fp, path, rc, LWFS_ERR_NOENT))
		return;

	rc = lwfs_lookup_sync(&naming_svc, txn, &dir2, 
			link_str, lock_type, &cap, &file2);
	sprintf(path, "lwfs_lookup(/%s/%s/%s)",dir1_str, dir2_str, link_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;


	/* unlink link_str */
	rc = lwfs_unlink_sync(&naming_svc, 
			txn,
			&dir2, 
			link_str,
			&cap, 
			&file1);
	sprintf(path, "lwfs_unlink(/%s/%s/%s)",dir1_str, dir2_str, link_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return; 

	/* make sure unlink did the right thing */
	rc = lwfs_lookup_sync(&naming_svc, txn, &dir2, 
			link_str, lock_type, &cap, &file2);
	sprintf(path, "lwfs_lookup(/%s/%s/%s)",dir1_str, dir2_str, link_str);
	if (!test_result(fp, path, rc, LWFS_ERR_NOENT))
		return;

	/* remove the object associated with the file */
	rc = lwfs_remove_obj_sync(txn, file1.file_obj, &cap);
	sprintf(path, "lwfs_remove_obj()");
	if (!test_result(fp, path, rc, LWFS_OK))
		return; 
	
	/* remove dir */
	rc = lwfs_remove_dir_sync(&naming_svc, txn, &dir1, 
			dir2_str, &cap, &dir2);
	sprintf(path, "lwfs_remove_dir(/%s/%s)",dir1_str, dir2_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* make sure rmdir did the right thing */
	rc = lwfs_lookup_sync(&naming_svc, txn, &dir1, 
			dir2_str, lock_type, &cap, &dir2);
	sprintf(path, "lwfs_lookup(/%s/%s)",dir1_str, dir2_str);
	if (!test_result(fp, path, rc, LWFS_ERR_NOENT))
		return;

	
	/* remove dir */
	rc = lwfs_remove_dir_sync(&naming_svc, txn, namespace_root, 
			dir1_str, &cap, &dir1);
	sprintf(path, "lwfs_remove_dir(/%s)",dir1_str);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* make sure rmdir did the right thing */
	rc = lwfs_lookup_sync(&naming_svc, txn, namespace_root, 
			dir1_str, lock_type, &cap, &dir1);
	sprintf(path, "lwfs_lookup(/%s)",dir1_str);
	if (!test_result(fp, path, rc, LWFS_ERR_NOENT))
		return;

	rc = lwfs_remove_namespace_sync(&naming_svc, txn, ns_name, &cap, &namespace);
	sprintf(path, "lwfs_remove_namespace(%s)",ns_name);
	if (!test_result(fp, path, rc, LWFS_OK))
		return;

	/* get the test namespace */
	rc = lwfs_get_namespace_sync(&naming_svc, ns_name, &namespace);
	sprintf(path, "lwfs_get_namespace(%s)", ns_name); 
	if (!test_result(fp, path, rc, LWFS_ERR_NOENT))
		return;

	fprintf(fp, "FINISHED!\n");
}

void setup(void)
{
	int rc = LWFS_OK;

	/* initialize the logger */
	logger_init(args_info.verbose_arg, args_info.logfile_arg); 

	/* initialize RPC before we do anything */
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);



	/* get the descriptor for the authorization service */
	authr_id.nid = args_info.authr_nid_arg; 
	authr_id.pid = args_info.authr_pid_arg; 
	rc = lwfs_get_service(authr_id, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(args_info.verbose_arg, "unable to get authr service descriptor: %s",
				lwfs_err_str(rc));
		return; 
	}

	/* get the storage service descriptors */
	storage_svc = (lwfs_service *)
		calloc(args_info.ss_num_servers_arg, sizeof(lwfs_service));
	storage_svc_ids = (lwfs_remote_pid *)
		calloc(args_info.ss_num_servers_arg, sizeof(lwfs_remote_pid));

	rc = read_ss_file(
			args_info.ss_server_file_arg, 
			args_info.ss_num_servers_arg,
			storage_svc_ids); 

	rc = lwfs_get_services(storage_svc_ids, 
			args_info.ss_num_servers_arg, 
			storage_svc);
	if (rc != LWFS_OK) {
		log_error(args_info.verbose_arg, "unable to get storage server descriptors: %s",
				lwfs_err_str(rc));
		return; 
	}

	/* get the descriptor for the naming service */
	naming_id.nid = args_info.naming_nid_arg; 
	naming_id.pid = args_info.naming_pid_arg; 
	rc = lwfs_get_service(naming_id, &naming_svc);
	if (rc != LWFS_OK) {
		log_error(args_info.verbose_arg, "unable to get authr service descriptor: %s",
				lwfs_err_str(rc));
		return; 
	}


	/* print the arguments */
	if (args_info.verbose_arg > 2) {
		print_args(logger_get_file(), &args_info, ""); 
	}

	/* ---------------------- START TESTING --------------- */

	/* initialize the opcodes */
	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	/* Create a container and allocate a capability that 
	 * allows the holder to read and write to the container.  
	 */
	rc = get_perms(&authr_svc, &cred, &cid, opcodes, &cap);
	if (rc != LWFS_OK) {
		log_error(args_info.verbose_arg, "unable to get perms: %s",
				lwfs_err_str(rc));
		return;
	}

	if (logging_debug(args_info.verbose_arg)) {
		fprint_lwfs_cap(stdout, "cap[0]", "DEBUG", &cap);
	}
}

void teardown(void)
{
	free(storage_svc);
	free(storage_svc_ids);
}

/**
 * Tests the naming service. 
 */
int main(int argc, char *argv[])
{
	/* Parse command line options */
	if (cmdline_parser(argc, argv, &args_info) != 0)
		exit(1);

	setup();

	run_tests();

	teardown();
	
	return LWFS_OK;
}

