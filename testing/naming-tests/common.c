/**  @file test_ops.c
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

#include <argp.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include "types/types.h"
#include "types/fprint_types.h"
#include "logger/logger.h"
#include "naming/naming_clnt.h"
#include "naming/naming_debug.h"
#include "naming/naming_options.h"
#include "storage/ss_clnt.h"
#include "storage/ss_options.h"
#include "authr/authr_clnt.h"
#include "authr/authr_srvr.h"
#include "authr/authr_options.h"
#include "threadpool/thread_pool_options.h"
#include "common.h"



void usage(char *pname);

/* --- private methods --- */


/**
 * @brief returns the difference *tv1 - *tv0 in microseconds.
 */
double tv_diff_usec(const struct timeval *tv0, const struct timeval *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec) * 1e6
		+ (tv1->tv_usec - tv0->tv_usec);
}

/**
 * @brief returns the difference *tv1 - *tv0 in seconds.
 */
double tv_diff_sec(const struct timeval *tv0, const struct timeval *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec)
            + (tv1->tv_usec - tv0->tv_usec)/1e6;
}

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
int get_perms(
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

