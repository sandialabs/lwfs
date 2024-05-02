/**  @file authr_client.c
 *   
 *   @brief A test client for the authorization service.
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1271 $.
 *   $Date: 2007-03-02 16:37:32 -0700 (Fri, 02 Mar 2007) $.
 */

#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include "cmdline.h"
#include "support/logger/logger.h"
#include "support/logger/logger_opts.h"
#include "client/authr_client/authr_client.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/authr_client/authr_client_opts.h"



/* ------------- Global variables ------------------ */

enum testid {
	EXISTS=1,
	CREATE_CID=2,
	REMOVE_CID=3,
	GET_ACL=4,
	MOD_ACL=5,
	GET_CAP=6,
	VERIFY=7
};


/* -------------- private methods -------------------*/

static int print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
    lwfs_remote_pid myid; 

    lwfs_get_id(&myid);

    fprintf(fp, "%s ------------ Test Authr Client ----\n", prefix);
    fprintf(fp, "%s nid = %llu, pid=%llu\n", prefix, 
	    (unsigned long long)myid.nid, 
	    (unsigned long long)myid.pid);

    fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
    fprintf(fp, "%s \ttestid = %d\n", prefix, args_info->testid_arg);
    fprintf(fp, "%s \tcid = %d\n", prefix, args_info->cid_arg); 


    print_authr_client_opts(fp, args_info, prefix); 
    print_logger_opts(fp, args_info, prefix); 

    fprintf(fp, "%s -----------------------------------\n", prefix);
    return 0;
}



static int run_tests(
	const lwfs_service *authr_svc, 
	const int testid, 
	const lwfs_cid cid) 
{
    int rc; 

    lwfs_txn *txn = NULL; 
    lwfs_cred cred1, cred2, cred3, cred4, cred5;


    /* initialize credentials */
    memset(&cred1, 0, sizeof(lwfs_cred));
    memset(&cred2, 0, sizeof(lwfs_cred));
    memset(&cred3, 0, sizeof(lwfs_cred));
    memset(&cred4, 0, sizeof(lwfs_cred));
    memset(&cred5, 0, sizeof(lwfs_cred));

    /* TODO: get a credential from the authentication server */
    cred1.data.uid[0] = 1; 
    cred2.data.uid[0] = 2; 
    cred3.data.uid[0] = 3; 
    cred4.data.uid[0] = 4; 
    cred5.data.uid[0] = 5; 

    switch(testid) {

	/* create a container */
	case CREATE_CID:
	    {
		lwfs_cap cap, modacl_cap; 

		/* get a cap that allows cred1 to create a container */
		rc = lwfs_get_cap_sync(authr_svc, cid, 
			LWFS_CONTAINER_CREATE, &cred1, &cap); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, 
			    "unable to create cap to create a container", cid);
		    break;
		}

		/* create a container */
		rc = lwfs_create_container_sync(
			authr_svc, txn, cid, 
			&cap, &modacl_cap);
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, 
			    "unable to create container cid=%d", cid);

		}
	    }
	    break;

	case EXISTS:
	    rc = LWFS_ERR_NOTSUPP; 
	    break;


	    /* This operation adds user2 and user5 to the list of 
	     * users allowed to write to the container. 
	     */
	case MOD_ACL:
	    {
		lwfs_cap cap; 
		lwfs_container_op container_op = LWFS_CONTAINER_MODACL; 
		lwfs_uid_array set, unset; 
		lwfs_uid set_uids[5]; 
		lwfs_uid unset_uids[5]; 

		/* get the capability to get an ACL */
		rc = lwfs_get_cap_sync(authr_svc, cid, 
			container_op, &cred1, &cap); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "unable to getcap for modacl op: %s",
			    lwfs_err_str(rc));
		    return rc; 
		}

		/* add cred 1, 2, 3, 5 to the set list */
		memset(set_uids, 0, sizeof(set_uids));
		memcpy(&set_uids[0], &cred1.data.uid, sizeof(lwfs_uid)); 
		memcpy(&set_uids[1], &cred2.data.uid, sizeof(lwfs_uid)); 
		memcpy(&set_uids[2], &cred3.data.uid, sizeof(lwfs_uid)); 
		memcpy(&set_uids[3], &cred5.data.uid, sizeof(lwfs_uid)); 
		set.lwfs_uid_array_val = set_uids; 
		set.lwfs_uid_array_len = 4; 

		/* add cred 3, 4 to the unset list */
		memset(unset_uids, 0, sizeof(unset_uids));
		memcpy(&unset_uids[0], &cred3.data.uid, sizeof(lwfs_uid)); 
		memcpy(&unset_uids[1], &cred4.data.uid, sizeof(lwfs_uid)); 
		unset.lwfs_uid_array_val = unset_uids; 
		unset.lwfs_uid_array_len = 2; 

		log_debug(authr_debug_level, "modacl: printing unset acl");
		if (logging_debug(authr_debug_level)) {
		    fprint_lwfs_uid_array(logger_get_file(), "set", "", &set);
		    fprint_lwfs_uid_array(logger_get_file(), "unset", "", &unset);
		}

		rc = lwfs_mod_acl_sync(
			authr_svc, txn, cid, LWFS_CONTAINER_WRITE, 
			&set, &unset, &cap); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "unable to modify the acl: %s",
			    lwfs_err_str(rc));
		    return rc; 
		}

		/* verify the acl */
	    }
	    break;



	    /* Get a capbility that allows user1 to write to the container. */
	case GET_CAP:
	case VERIFY:
	    {
		lwfs_cap cap; 

		/* get a cap that allows cred1 to write (this should pass) */
		rc = lwfs_get_cap_sync(authr_svc, cid, 
			LWFS_CONTAINER_WRITE, &cred1, &cap); 

		/* verify the capability */
		rc = lwfs_verify_caps_sync(authr_svc, &cap, 1); 
		break;
	    }



	    /* Fetch the ACL for the LWFS_CONTAINER_WRITE operation. */
	case GET_ACL:
	    {
		lwfs_uid_array acl; 
		lwfs_cap cap; 
		lwfs_container_op container_op = LWFS_CONTAINER_READ; 
		memset(&acl, 0, sizeof(lwfs_uid_array));

		/* get the capability to get an ACL */
		rc = lwfs_get_cap_sync(
			authr_svc, cid, container_op,
			&cred1, &cap); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "unable to getcap for getacl op: %s",
			    lwfs_err_str(rc));
		    return rc; 
		}

		/* get the acl for LWFS_CONTAINER_WRITE */
		rc = lwfs_get_acl_sync(
			authr_svc, cid, LWFS_CONTAINER_WRITE, 
			&cap, &acl);

		/* verify that the acl has uid1 and uid2 in it */

		/* free the space allocated for the acl */
		lwfs_acl_free(&acl); 
	    }

	    break;


	case REMOVE_CID:
	    {
		lwfs_cap cap; 
		lwfs_container_op container_op = LWFS_CONTAINER_REMOVE; 

		/* get cap that allows the operation */
		rc = lwfs_get_cap_sync(
			authr_svc, cid, container_op,
			&cred1, &cap); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "unable to getcap for remove op: %s",
			    lwfs_err_str(rc));
		    return rc; 
		}

		/* remove the container */
		rc = lwfs_remove_container_sync(
			authr_svc, txn, cid, &cap); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "unable to remove container %d: %s",
			    cid, lwfs_err_str(rc));
		    return rc; 
		}
	    }
	    break; 


	default:
	    log_error(authr_debug_level, "unrecognized testID %d", testid);
    }

    return rc; 
}


/* --------------------------------------------- */

int main(int argc, char **argv)
{
    int rc = LWFS_OK;
    struct gengetopt_args_info args_info; 
    lwfs_service authr_svc;
    lwfs_remote_pid authr_id; 

    /* Parse command line options to override defaults */
    if (cmdline_parser(argc, argv, &args_info) != 0) {
	exit(1);
    }

    /* Initialize the logger */
    authr_debug_level = args_info.verbose_arg;
    logger_init(args_info.verbose_arg, args_info.logfile_arg);

    /* Initialize RPC */
    lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

    /* get the service description for the authorization service */
    authr_id.nid = args_info.authr_nid_arg; 
    authr_id.pid = args_info.authr_pid_arg; 
    rc = lwfs_get_service(authr_id, &authr_svc); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not get service descriptor: %s",
		lwfs_err_str(rc));
	return rc; 
    }


    if (authr_debug_level > 2) {
	print_args(logger_get_file(), &args_info, ""); 
	fflush(logger_get_file());
    }

    rc = run_tests(&authr_svc, args_info.testid_arg, args_info.cid_arg); 

    if (rc != LWFS_OK) {
	log_info(authr_debug_level, "Test %d failed!  %s", 
		args_info.testid_arg, lwfs_err_str(rc));
	fprintf(stdout, "FAILED\n");
    }
    else {
	log_info(authr_debug_level, "Test %d passed!", args_info.testid_arg);
	fprintf(stdout, "PASSED\n");
    }

    cmdline_parser_free(&args_info);

    return rc;
}
