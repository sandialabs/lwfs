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

struct async_req {
	lwfs_request req;
	char input[256];
	lwfs_size bytes;
};

/**
 * Tests a single SS client. 
 */
int main(int argc, char *argv[])
{
	int rc = LWFS_OK;
	int i;
	char ostr[33];
	
	lwfs_remote_pid authr_id; 
	lwfs_remote_pid ss_id; 

	/* arguments */
	struct gengetopt_args_info args_info; 

	/* variables used in the test */
	lwfs_service storage_svc;   /* service descriptor for the storage service */
	lwfs_service authr_svc;  /* service descriptor for the authorization service */

	lwfs_txn *txn = NULL;
	lwfs_cred cred; 
	lwfs_opcode opcodes; 
	lwfs_cap cap;
	
	lwfs_oid cli_oid; /* the oid provided on the command-line */
	
	rpc_debug_level = LOG_ALL;

	/* Parse the command-line arguments */
	if (cmdline_parser(argc, argv, &args_info) != 0)
		return (1); 


	/* initialize RPC before we do anything */
#ifdef PTL_IFACE_CLIENT
	lwfs_ptl_init(PTL_IFACE_CLIENT, args_info.test_pid_arg);
#else
	lwfs_ptl_init(PTL_IFACE_DEFAULT, args_info.test_pid_arg);
#endif
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);


	/* initialize the logger */
	logger_init(args_info.verbose_arg, args_info.logfile_arg); 

	/* initialize credentials */
	memset(&cred, 0, sizeof(lwfs_cred));

	/* TODO: get a credential from the authentication server */
	cred.data.uid[0] = 1; 

	/* get the descriptor for the authorization service */
	authr_id.nid = args_info.authr_nid_arg; 
	authr_id.pid = args_info.authr_pid_arg; 
	rc = lwfs_get_service(authr_id, &authr_svc);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to get authr service descriptor: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* get the storage service descriptor */
	ss_id.nid = args_info.ss_nid_arg; 
	ss_id.pid = args_info.ss_pid_arg; 
	rc = lwfs_get_service(ss_id, &storage_svc);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to get storage service descriptor: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	str_to_oid(args_info.oid_arg, cli_oid);
	log_debug(ss_debug_level, "oid=0x%s", lwfs_oid_to_string(cli_oid, ostr));
	
	/* print the arguments */
	print_args(logger_get_file(), &args_info, ""); 

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

	lwfs_obj *obj_array = (lwfs_obj *)malloc(args_info.num_objs_arg*sizeof(lwfs_obj));

	/* Run the experiment for n objects */
        uint64_t *id=(uint64_t *)cli_oid;
        struct async_req *async_reqs=calloc(args_info.num_objs_arg, sizeof(struct async_req));
        int async_op = FALSE;
	for (i=0; i<args_info.num_objs_arg; i++) {
	    /* assign the object for this operation */
	    lwfs_obj *obj = &obj_array[i]; 

	    /* initialize the object data structure */
	    id[1]++;
	    log_debug(ss_debug_level, "oid[%d]=0x%s", i, lwfs_oid_to_string(cli_oid, ostr));
	    lwfs_init_obj(&storage_svc, 0, 
		    args_info.cid_arg, 
		    cli_oid, 
		    obj); 

	    /*---- run the test ----*/

	    /* EXISTS */
	    if (strcmp("exists", args_info.test_arg) == 0) {
	    }

	    /* CREATE */
	    else if (strcmp("create", args_info.test_arg) == 0) {

		fprintf(stdout, "calling lwfs_create_obj(%d)\n",i);
		log_debug(ss_debug_level, "calling lwfs_create_obj(oid=0x%s)", 
			lwfs_oid_to_string(cli_oid, ostr));

		/* try to create the object */
		rc = lwfs_create_obj_sync(txn, obj, &cap);
		if (rc != LWFS_OK) {
		    fprintf(stderr,"failed to create obj %d", i);
		    rc = FAILED; 
		    break;
		}
		else 
		    rc = PASSED;
	    }

	    /* REMOVE */
	    else if (strcmp("remove", args_info.test_arg) == 0) {
		log_debug(ss_debug_level, "calling lwfs_remove_obj(%d)",i);
		rc = lwfs_remove_obj_sync(txn, obj, &cap);
		if (rc != LWFS_OK) {
		    rc = FAILED; 
		    break;
		}
		else 
		    rc = PASSED;
	    }

	    /* READ */
	    else if (strcmp("read", args_info.test_arg) == 0) {
		lwfs_size bytes=0; 
		char input[256]; 
		memset(input, 0, sizeof(input));
		log_debug(ss_debug_level, "calling lwfs_read_sync(oid=0x%s,len=%d)", 
			lwfs_oid_to_string(cli_oid, ostr), (int)sizeof(input));

		rc = lwfs_read_sync(txn, obj, 0, input, sizeof(input), &cap, &bytes);
		if (rc != LWFS_OK) {
		    rc = FAILED; 
		    goto cleanup;
		}

		if (bytes != strlen(args_info.data_arg)) {
		    log_error(ss_debug_level, "read %d bytes, should have read %d\n",
			    bytes, strlen(args_info.data_arg));
		    rc = FAILED; 
		    goto cleanup; 
		}

		if (strcmp(args_info.data_arg, input) != 0) {
		    log_error(ss_debug_level, "expected \"%s\", read \"%s\"",
			    args_info.data_arg, input);
		    rc = FAILED; 
		}
	    }

	    /* async READ */
	    else if (strcmp("aread", args_info.test_arg) == 0) {
		async_op = TRUE; 
		memset(async_reqs[i].input, 0, sizeof(async_reqs[i].input));
		log_debug(ss_debug_level, "calling lwfs_read(oid=0x%s,len=%d)", 
			lwfs_oid_to_string(cli_oid, ostr), (int)sizeof(async_reqs[i].input));

		rc = lwfs_read(txn, obj, 0, async_reqs[i].input, sizeof(async_reqs[i].input), &cap, &async_reqs[i].bytes, &async_reqs[i].req);
		if (rc != LWFS_OK) {
		    rc = FAILED; 
		    goto cleanup;
		}
	    }

	    /* WRITE */
	    else if (strcmp("write", args_info.test_arg) == 0) {
		log_debug(ss_debug_level, 
			"calling lwfs_write_sync(oid=0x%s,len=%d,data=%s)", 
			lwfs_oid_to_string(cli_oid, ostr), (int)strlen(args_info.data_arg),
			args_info.data_arg);

		rc = lwfs_write_sync(txn, obj, 0, args_info.data_arg, 
			strlen(args_info.data_arg), &cap);
		if (rc != LWFS_OK)
		    rc = FAILED; 
		else 
		    rc = PASSED;
	    }

	    /* async WRITE */
	    else if (strcmp("awrite", args_info.test_arg) == 0) {
		async_op = TRUE; 
		log_debug(ss_debug_level, 
			"calling lwfs_write(oid=0x%s,len=%d,data=%s)", 
			lwfs_oid_to_string(cli_oid, ostr), (int)strlen(args_info.data_arg),
			args_info.data_arg);

		rc = lwfs_write(txn, obj, 0, args_info.data_arg, 
			strlen(args_info.data_arg), &cap, &async_reqs[i].req);
		if (rc != LWFS_OK)
		    rc = FAILED; 
		else 
		    rc = PASSED;
	    }

	    /* TRUNC */
	    else if (strcmp("trunc", args_info.test_arg) == 0) {
		log_debug(ss_debug_level, "calling lwfs_trunc(oid=0x%s,size=%d)", 
			lwfs_oid_to_string(cli_oid, ostr), args_info.trunc_size_arg);
		rc = FAILED;
	    }


	    /* SETATTR */
	    else if (strcmp("setattr", args_info.test_arg) == 0) {
		lwfs_attr_array attrs;

		attrs.lwfs_attr_array_len = 1;
		attrs.lwfs_attr_array_val = calloc(1, sizeof(lwfs_attr));
		attrs.lwfs_attr_array_val[0].name = calloc(1, sizeof(char)*LWFS_NAME_LEN);
		strncpy(attrs.lwfs_attr_array_val[0].name, args_info.attr_name_arg, LWFS_NAME_LEN);
		attrs.lwfs_attr_array_val[0].value.lwfs_attr_data_len = strlen(args_info.attr_val_arg)+1;
		attrs.lwfs_attr_array_val[0].value.lwfs_attr_data_val = (char *)args_info.attr_val_arg;

		log_debug(ss_debug_level, "calling "
			"lwfs_setattrs(oid=0x%s,attr_name=%s,attr_val=%s)", 
			lwfs_oid_to_string(cli_oid, ostr), args_info.attr_name_arg, 
			args_info.attr_val_arg);
		rc = lwfs_setattrs_sync(txn, obj, &attrs, &cap);
		if (rc != LWFS_OK) 
		    rc = FAILED; 
		else 
		    rc = PASSED;
	    }

	    /* GETATTR */
	    else if (strcmp("getattr", args_info.test_arg) == 0) {
		lwfs_attr_array result;
		lwfs_name_array names;

		names.lwfs_name_array_len=1;
		names.lwfs_name_array_val=calloc(1, sizeof(lwfs_name *));
		names.lwfs_name_array_val[0]=calloc(1, LWFS_NAME_LEN);
		strncpy(names.lwfs_name_array_val[0], args_info.attr_name_arg, LWFS_NAME_LEN);

		log_debug(ss_debug_level, "calling "
			"lwfs_getattrs(oid=0x%s,attr_name=%s)", 
			lwfs_oid_to_string(cli_oid, ostr), args_info.attr_name_arg);
		rc = lwfs_getattrs_sync(txn, obj, &names, &cap, &result);
		if (rc != LWFS_OK) {
		    log_error(ss_debug_level, "could not get attribute \"%s\"",
			    args_info.attr_name_arg); 
		    rc = FAILED; 
		    goto cleanup; 
		}

		/* check to make sure the value of the attribute is correct */
		if (strcmp(result.lwfs_attr_array_val[0].value.lwfs_attr_data_val, args_info.attr_val_arg) != 0) {
		    log_error(ss_debug_level, "attr.value=%s does not match expected=%s",
			    result.lwfs_attr_array_val[0].value.lwfs_attr_data_val, 
			    args_info.attr_val_arg); 
		    rc = FAILED; 
		}
		else 
		    rc = PASSED;
	    }

	    /* RMATTR */
	    else if (strcmp("rmattr", args_info.test_arg) == 0) {
		lwfs_name_array names;

		names.lwfs_name_array_len=1;
		names.lwfs_name_array_val=calloc(1, sizeof(lwfs_name *));
		names.lwfs_name_array_val[0]=calloc(1, LWFS_NAME_LEN);
		strncpy(names.lwfs_name_array_val[0], args_info.attr_name_arg, LWFS_NAME_LEN);

		log_debug(ss_debug_level, "calling lwfs_rmattrs(%d,%s)", 
			cli_oid, args_info.attr_name_arg);
		rc = lwfs_rmattrs_sync(txn, obj, &names, &cap); 
		if (rc != LWFS_OK) {
		    log_error(ss_debug_level, "could not remove attribute \"%s\"",
			    args_info.attr_name_arg); 
		    rc = FAILED; 
		    goto cleanup; 
		}
	    }

	    /* LISTATTR */
	    else if (strcmp("listattr", args_info.test_arg) == 0) {
		int i;
		lwfs_name_array result;

		memset(&result, 0, sizeof(lwfs_name_array));

		log_debug(ss_debug_level, "calling "
			"lwfs_listattrs(oid=0x%s)", 
			lwfs_oid_to_string(cli_oid, ostr));
		rc = lwfs_listattrs_sync(txn, obj, &cap, &result);
		if (rc != LWFS_OK) {
		    log_error(ss_debug_level, "could not list attribute"); 
		    rc = FAILED; 
		    goto cleanup; 
		}

		/* check to make sure the value of the attribute is correct */
		if (strcmp(result.lwfs_name_array_val[0], args_info.attr_name_arg) != 0) {
		    log_error(ss_debug_level, "name=%s does not match expected=%s",
			    result.lwfs_name_array_val[0], 
			    args_info.attr_name_arg); 
		    rc = FAILED; 
		}
		else 
		    rc = PASSED;
		for (i=0; i<result.lwfs_name_array_len; i++) {
		    log_debug(ss_debug_level, "lwfs_listattrs(oid=0x%s) found: %s", 
			    lwfs_oid_to_string(cli_oid, ostr), result.lwfs_name_array_val[i]);
		}
	    }

	    /* STAT */
	    else if (strcmp("stat", args_info.test_arg) == 0) {
		lwfs_stat_data stat_data; 
		log_debug(ss_debug_level, "calling obj_stat(%d)", 
			cli_oid);
		rc = lwfs_stat_sync(txn, obj, &cap, &stat_data); 
		if (rc != LWFS_OK) {
		    log_error(ss_debug_level, "could not stat object \"%s\"",
			    args_info.attr_name_arg); 
		    rc = FAILED; 
		    goto cleanup; 
		}
	    }

	    else {
		log_error(ss_debug_level, "unrecognized test=%s", 
			args_info.test_arg);
		rc = FAILED;
	    }
	}
	if (async_op == TRUE) {
		/* this is an a async test.  cleanup the outstanding requests. */
		int wait_rc=LWFS_OK;
		int remote_rc=LWFS_OK;
		for (i=0; i<args_info.num_objs_arg; i++) {
			wait_rc = lwfs_wait(&async_reqs[i].req, &remote_rc);
			if (wait_rc != LWFS_OK) {
				log_error(ss_debug_level, "wait failed: %s",
					  lwfs_err_str(wait_rc));
				rc = 0; /* failure */
				goto cleanup; 
			}
			if (remote_rc != LWFS_OK) {
				log_error(ss_debug_level, "remote operation failed: %s",
					  lwfs_err_str(remote_rc));
				rc = 0; /* failure */
				goto cleanup; 
			}
			if (strcmp("aread", args_info.test_arg) == 0) {
				if (async_reqs[i].bytes != strlen(args_info.data_arg)) {
				    log_error(ss_debug_level, "read %d bytes, should have read %d\n",
					    async_reqs[i].bytes, strlen(args_info.data_arg));
				    rc = FAILED; 
				    goto cleanup; 
				}
				if (strcmp(args_info.data_arg, async_reqs[i].input) != 0) {
				    log_error(ss_debug_level, "expected \"%s\", read \"%s\"",
					    args_info.data_arg, async_reqs[i].input);
				    rc = FAILED; 
				}
			}
		}
	}
	free(async_reqs);

cleanup:
	free(obj_array);
	cmdline_parser_free (&args_info);

	if (rc == PASSED) {
		fprintf(stdout, "PASSED\n");
	}
	else {
		fprintf(stdout, "FAILED\n");
	}
	return rc;
}

