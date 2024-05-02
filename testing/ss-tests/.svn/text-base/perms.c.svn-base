/**  @file test_ops.c
 *   
 *   @brief Synchronous implementations of each of the storage server oprerations.
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 534 $
 *   @date $Date: 2005-11-23 16:49:52 -0700 (Wed, 23 Nov 2005) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <argp.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "support/logger/logger.h"

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/storage_client/storage_client.h"
#include "perms.h"

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
		const lwfs_service *authr_svc,
		const lwfs_cred *cred,
		lwfs_cid cid, 
		const lwfs_opcode opcodes,
		lwfs_cap *cap)
{
	int rc = LWFS_OK;  /* return code */


	/* try to get the caps from an existing container */
	rc = lwfs_get_cap_sync(authr_svc, cid, opcodes, cred, cap);
	switch (rc) {

		case LWFS_OK: 
			/* we're done */
			break;

		case LWFS_ERR_NOENT:
			/* if the container does not exist, create the container */
			{
				lwfs_cap create_cid_cap;  
				lwfs_cap modacl_cap;
				lwfs_uid_array uid_array; 
				lwfs_txn *txn_id = NULL;  /* not used */

				/* get a cap that allows cred to create a container */
				rc = lwfs_get_cap_sync(authr_svc, cid, 
						LWFS_CONTAINER_CREATE, cred, &create_cid_cap); 
				if (rc != LWFS_OK) {
					log_error(authr_debug_level, 
							"unable to create cap to create a container", cid);
					break;
				}

				/* create the container */
				rc = lwfs_create_container_sync(authr_svc, 
						txn_id, cid, 
						&create_cid_cap, &modacl_cap);
				if (rc != LWFS_OK) {
					log_error(ss_debug_level, "unable to create a container: %s",
							lwfs_err_str(rc));
					break;
				}

				/* initialize the uid array */
				uid_array.lwfs_uid_array_len = 1; 
				uid_array.lwfs_uid_array_val = (lwfs_uid *)&cred->data.uid;

				/* create the acls */
				rc = lwfs_create_acl_sync(authr_svc, txn_id, 
						cid, opcodes, &uid_array, &modacl_cap);
				if (rc != LWFS_OK) {
					log_error(ss_debug_level, "unable to create write acl: %s",
							lwfs_err_str(rc));
					break;
				}

				/* try again to get the caps */
				rc = lwfs_get_cap_sync(authr_svc, cid, opcodes, cred, cap);
				if (rc != LWFS_OK) {
					log_error(ss_debug_level, "unable to getcaps: %s",
							lwfs_err_str(rc));
					break;
				}
			}
			break;

		default:  
			log_error(ss_debug_level, "unable to getcaps: %s",
					lwfs_err_str(rc));
			break;
	}

	return rc; 
}


