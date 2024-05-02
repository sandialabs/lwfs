/**  @file storage_tests.c
 *   
 *   @brief Test the storage service API.
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
#include "common/types/types.h"
#include "client/storage_client/storage_client.h"
#include "client/storage_client/storage_client_sync.h"
#include "client/storage_client/storage_client_opts.h"
#include "client/authr_client/authr_client_opts.h"
#include "support/timer/timer.h"
#include "support/logger/logger_opts.h"
#include "perms.h"


const int FAILED = -1; 
const int PASSED = 0; 
	
/* -------------- PRIVATE METHODS -------------- */

static void print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
	print_logger_opts(fp, args_info, prefix); 
	print_authr_client_opts(fp, args_info, prefix); 
	print_storage_client_opts(fp, args_info, prefix); 
}

static void str_to_oid(char *str, lwfs_oid oid)
{
	char *oid_char = (char*)oid;

	sscanf(str, "%08X%08X%08X%08X", 
		(uint32_t *)&(oid_char[0]), (uint32_t *)&(oid_char[4]), (uint32_t *)&(oid_char[8]), (uint32_t *)&(oid_char[12]));
}

/**
 * Tests a single SS client. 
 */
int main(int argc, char *argv[])
{
	int rc = LWFS_OK;
	char ostr[33];
	
	lwfs_remote_pid authr_id; 
	lwfs_remote_pid *storage_svc_ids; 

	/* arguments */
	struct gengetopt_args_info args_info; 

	/* variables used in the test */
	lwfs_service *storage_svc;   /* service descriptor for the storage service */
	lwfs_service authr_svc;  /* service descriptor for the authorization service */

	lwfs_txn *txn = NULL;
	lwfs_cred cred; 
	lwfs_opcode opcodes; 
	lwfs_cap cap;
	lwfs_obj obj; 
	lwfs_oid cli_oid; /* the oid provided on the command-line */

	uid_t unix_uid;


	/* initialize RPC before we do anything */
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);


	/* Parse the command-line arguments */
	if (cmdline_parser(argc, argv, &args_info) != 0)
		return (1); 


	/* initialize the logger */
	logger_init(args_info.verbose_arg, args_info.logfile_arg); 


	/* get the descriptor for the authorization service */
	authr_id.nid = args_info.authr_nid_arg; 
	authr_id.pid = args_info.authr_pid_arg; 
	rc = lwfs_get_service(authr_id, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to get authr service descriptor: %s",
				lwfs_err_str(rc));
		return rc; 
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
		return rc; 
	}

	str_to_oid(args_info.oid_arg, cli_oid);
	log_debug(ss_debug_level, "oid=0x%s", lwfs_oid_to_string(cli_oid, ostr));
	
	/* print the arguments */
	print_args(logger_get_file(), &args_info, "");
	
	/* ---------------------- Get necessary authentication -------- */

	/* initialize the user credential */
	memset(&cred, 0, sizeof(lwfs_cred));
	unix_uid = getuid(); 
	memcpy(&cred.data.uid, &unix_uid, sizeof(unix_uid));
	 

	/* ---------------------- Get necessary authorization -------- */

	/* initialize the opcodes */
	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	/* Create a container and allocate a capability that 
	 * allows the holder to read and write to the container.  
	 */
	rc = get_perms(&authr_svc, &cred, args_info.cid_arg, opcodes, &cap);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to get perms: %s",
				lwfs_err_str(rc));
		rc = FAILED; 
		goto cleanup;
	}

	if (logging_debug(ss_debug_level)) {
		fprint_lwfs_cap(stdout, "cap[0]", "DEBUG", &cap);
	}

	/* ---------------------- Run Tests --------------- */

	/* initialize the object data structure */
	lwfs_init_obj(storage_svc, 0, args_info.cid_arg, cli_oid, &obj); 
	log_debug(ss_debug_level, "initialized oid=0x%s",
		lwfs_oid_to_string(cli_oid, ostr));



	/* CREATE */
	{
		log_debug(ss_debug_level, "calling lwfs_create_obj()");
	
		/* try to create the object */
		rc = lwfs_create_obj_sync(txn, &obj, &cap);
		if (rc != LWFS_OK) {
			rc = FAILED;
			goto cleanup; 
		}
	}

	/* SETATTRS */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname[5];
		char *attrvalue[5];
		int attrsize[5];
		lwfs_attr_array attrs;
		
		for (i=0;i<attr_count;i++) {
			attrname[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
			sprintf(attrname[i], "test_attr_name_%d", i);
			attrvalue[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
			sprintf(attrvalue[i], "test_attr_value_%d", i);
			attrsize[i] = strlen(attrvalue[i])+1;
		}
		
		lwfs_create_attr_array(attrname, (const void **)attrvalue, attrsize, attr_count, &attrs);

		log_debug(ss_debug_level, "calling lwfs_setattrs(oid=0x%s)", 
				lwfs_oid_to_string(cli_oid, ostr));
		rc = lwfs_setattrs_sync(txn, &obj, &attrs, &cap);
		if (rc != LWFS_OK) {
			rc = FAILED;
			goto cleanup; 
		}
	}

	/* GETATTRS */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname[5];
		lwfs_attr_array result;
		lwfs_name_array names;

		for (i=0;i<attr_count;i++) {
			attrname[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
			sprintf(attrname[i], "test_attr_name_%d", i);
		}

		lwfs_create_name_array(attrname, attr_count, &names);

		log_debug(ss_debug_level, "calling lwfs_getattrs(oid=0x%s)", 
				lwfs_oid_to_string(cli_oid, ostr));
		rc = lwfs_getattrs_sync(txn, &obj, &names, &cap, &result);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not get attribute \"%s\"",
					args_info.attr_name_arg); 
			rc = FAILED; 
			goto cleanup; 
		}

		for (i=0;i<result.lwfs_attr_array_len;i++) {
			char attrvalue[LWFS_NAME_LEN];

			log_debug(ss_debug_level, "getattrs found: attr.name=%s; attr.value=%s",
				  result.lwfs_attr_array_val[i].name,
				  result.lwfs_attr_array_val[i].value.lwfs_attr_data_val); 

			if (strcmp(result.lwfs_attr_array_val[i].name, attrname[i]) != 0) {
				log_error(ss_debug_level, "attr.name=%s does not match expected=%s",
					  result.lwfs_attr_array_val[i].name, 
					  attrname[i]); 
				rc = FAILED;
				goto cleanup; 
			}

			sprintf(attrvalue, "test_attr_value_%d", i);
			if (strcmp(result.lwfs_attr_array_val[i].value.lwfs_attr_data_val, attrvalue) != 0) {
				log_error(ss_debug_level, "attr.value=%s does not match expected=%s",
					  result.lwfs_attr_array_val[i].value.lwfs_attr_data_val, 
					  attrvalue); 
				rc = FAILED;
				goto cleanup; 
			}
		}
	}

	/* SETATTR */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname;
		char attrvalue[LWFS_NAME_LEN];
		
		attrname = calloc(1, sizeof(char)*LWFS_NAME_LEN);

		for (i=0;i<attr_count;i++) {
			sprintf(attrname, "test_attr_name_0%d", i);
			sprintf(attrvalue, "test_attr_value_0%d", i);

			log_debug(ss_debug_level, "calling lwfs_setattr(oid=0x%s)", 
					lwfs_oid_to_string(cli_oid, ostr));
			log_debug(ss_debug_level, "setattr: attr.name=%s; attr.value=%s",
				  attrname,
				  attrvalue); 

			rc = lwfs_setattr_sync(txn, &obj, attrname, attrvalue, strlen(attrvalue), &cap);
			if (rc != LWFS_OK) {
				log_error(ss_debug_level, "could not set attribute \"%s\"",
						attrname); 
				rc = FAILED; 
				goto cleanup; 
			}
		}
	}

	/* GETATTR */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname;
		char attrvalue[LWFS_NAME_LEN];
		lwfs_attr result;
		
		attrname = calloc(1, sizeof(char)*LWFS_NAME_LEN);

		for (i=0;i<attr_count;i++) {
			sprintf(attrname, "test_attr_name_0%d", i);

			log_debug(ss_debug_level, "calling lwfs_getattr(oid=0x%s)", 
					lwfs_oid_to_string(cli_oid, ostr));
			memset(&result, 0, sizeof(lwfs_attr));
			rc = lwfs_getattr_sync(txn, &obj, attrname, &cap, &result);
			if (rc != LWFS_OK) {
				log_error(ss_debug_level, "could not get attribute \"%s\"",
						attrname); 
				rc = FAILED; 
				goto cleanup; 
			}
	
	
			log_debug(ss_debug_level, "getattr found: attr.name=%s; attr.value=%s",
				  result.name,
				  result.value.lwfs_attr_data_val); 

			if (strcmp(result.name, attrname) != 0) {
				log_error(ss_debug_level, "attr.name=%s does not match expected=%s",
					  result.name, 
					  attrname); 
				rc = FAILED;
				goto cleanup; 
			}

			sprintf(attrvalue, "test_attr_value_0%d", i);
			if (strcmp(result.value.lwfs_attr_data_val, attrvalue) != 0) {
				log_error(ss_debug_level, "attr.value=%s does not match expected=%s",
					  result.value.lwfs_attr_data_val, 
					  attrvalue); 
				rc = FAILED;
				goto cleanup; 
			}
		}
	}

	/* RMATTR */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname;
		
		attrname = calloc(1, sizeof(char)*LWFS_NAME_LEN);

		for (i=0;i<attr_count;i++) {
			sprintf(attrname, "test_attr_name_0%d", i);

			log_debug(ss_debug_level, "calling lwfs_rmattr(oid=0x%s)", 
					lwfs_oid_to_string(cli_oid, ostr));
			rc = lwfs_rmattr_sync(txn, &obj, attrname, &cap);
			if (rc != LWFS_OK) {
				log_error(ss_debug_level, "could not remove attribute \"%s\"",
						attrname); 
				rc = FAILED; 
				goto cleanup; 
			}
		}
	}

	/* LISTATTRS */
	{
		int i;
		lwfs_name_array result;
		 
		log_debug(ss_debug_level, "calling lwfs_listattrs(oid=0x%s)", 
				lwfs_oid_to_string(cli_oid, ostr));
		rc = lwfs_listattrs_sync(txn, &obj, &cap, &result);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not list attribute"); 
			rc = FAILED; 
			goto cleanup; 
		}

		for (i=0;i<result.lwfs_name_array_len;i++) {
			char attrname[LWFS_NAME_LEN];

			log_debug(ss_debug_level, "listattrs found: attr.name=%s",
				  result.lwfs_name_array_val[i]); 

			sprintf(attrname, "test_attr_name_%d", i);
			if (strcmp(result.lwfs_name_array_val[i], attrname) != 0) {
				log_error(ss_debug_level, "attr.name=%s does not match expected=%s",
					  result.lwfs_name_array_val[i], 
					  attrname); 
				rc = FAILED;
				goto cleanup; 
			}
		}
	}

	/* RMATTRS */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname[5];
		lwfs_name_array names;

		for (i=0;i<attr_count;i++) {
			attrname[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
			sprintf(attrname[i], "test_attr_name_%d", i);
		}

		lwfs_create_name_array(attrname, attr_count, &names);
		 
		log_debug(ss_debug_level, "calling lwfs_rmattrs(%d)", 
				cli_oid);
		rc = lwfs_rmattrs_sync(txn, &obj, &names, &cap); 
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not remove attribute \"%s\"",
					args_info.attr_name_arg); 
			rc = FAILED; 
			goto cleanup; 
		}
	}

	/* SETATTRS */
	{
		int i=0;
		int attr_count=5;
		lwfs_name attrname[5];
		char *attrvalue[5];
		int attrsize[5];
		lwfs_attr_array attrs;
		
		for (i=0;i<attr_count;i++) {
			attrname[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
			sprintf(attrname[i], "test_attr_name_%d", i);
			attrvalue[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
			sprintf(attrvalue[i], "test_attr_value_%d", i);
			attrsize[i] = strlen(attrvalue[i])+1;
		}
		
		lwfs_create_attr_array(attrname, (const void **)attrvalue, attrsize, attr_count, &attrs);

		log_debug(ss_debug_level, "calling lwfs_setattrs(oid=0x%s)", 
				lwfs_oid_to_string(cli_oid, ostr));
		rc = lwfs_setattrs_sync(txn, &obj, &attrs, &cap);
		if (rc != LWFS_OK) {
			rc = FAILED;
			goto cleanup; 
		}
	}

	/* REMOVE */
	{
		log_debug(ss_debug_level, "calling lwfs_remove_obj()");
		rc = lwfs_remove_obj_sync(txn, &obj, &cap);
		if (rc != LWFS_OK) {
			rc = FAILED;
			goto cleanup; 
		}
	}

cleanup:
	free(storage_svc);
	free(storage_svc_ids);
	cmdline_parser_free (&args_info);

	if (rc == FAILED) {
		fprintf(stdout, "FAILED\n");
	}
	else {
		fprintf(stdout, "PASSED\n");
	}
	return rc;
}

