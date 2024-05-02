/**  
 *   @file authr_srvr.h
 * 
 *   @brief Prototypes for the server-side methods for accessing 
 *   the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 975 $
 *   $Date: 2006-08-28 15:48:31 -0600 (Mon, 28 Aug 2006) $
 */

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/authr_common/authr_args.h"
#include "common/authr_common/authr_debug.h"
#include "common/authr_common/authr_trace.h"
#include "common/authr_common/authr_xdr.h"

#include "server/rpc_server/rpc_server.h"

#ifndef _LWFS_AUTH_SRVR_H_
#define _LWFS_AUTH_SRVR_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)
	
	extern const lwfs_svc_op *lwfs_authr_op_array(); 

	/**
	 * @brief Initialize an authorization server. 
	 */
	extern int lwfs_authr_srvr_init(
			const lwfs_bool verify_caps, 
			const char *db_path,
			const lwfs_bool db_clear,
			const lwfs_bool db_recover,
			lwfs_service *svc); 

	/** 
	 * @brief Start an authorization server. 
	 */
	extern int lwfs_authr_srvr_fini(const lwfs_service *svc);

	/**
	 * @brief Create a new container on the authorization server.
	 *
	 * The \b lwfs_create_container method generates a unique container
	 * ID on the authorization server.  Initially, only one ACL exists
	 * for the container, an ACL that allows the owner to add more ACLs. 
	 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to create the container
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type a capability for the new container
	 */
	extern int create_container(
			const lwfs_remote_pid *caller,
			const lwfs_create_container_args *args,
			const lwfs_rma *data_addr,
			lwfs_cap *result);

	/**
	 * @brief Remove a container.
	 *
	 * The \b lwfs_remove_container method removes an 
	 * existing container ID from the authorization server. 
	 *
	 * @note This operation does not check for "orphan" entities (i.e., 
	 * existing objects, namespace entries, and so forth) before removing
	 * the container ID. We leave this responsibility to the client. 
	 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to remove the container
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type a capability for the new container
	 */
	extern int remove_container(
			const lwfs_remote_pid *caller,
			const lwfs_remove_container_args *args,
			const lwfs_rma *data_addr,
			void *result);

	/**
	 * @brief Get the access-control list for an 
	 *        container/op pair.
	 *
	 * The \b lwfs_get_acl method returns a list of users that have 
	 * a particular type of access to a container. 
	 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to get an ACL
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type the list of users with access to the specified container
	 *
	 * @returns the ACL in the \em result field. 
	 */
	extern int get_acl(
			const lwfs_remote_pid *caller,
			const lwfs_get_acl_args *args, 
			const lwfs_rma *data_addr,
			lwfs_uid_array *result);

	/**
	 * @brief Modify the access-control list for an container/op pair.
	 *
	 * This method changes the access-control list for a particular
	 * operation on an container of objects.  The arguments 
	 * include a list of users that require access (set) and 
	 * a list of users that no longer require access (unset).  
	 * To resolve conflicts that occur when a user ID appears in both
	 * lists, the implementation first grants access to the users 
	 * in the set list, then removes access from users in the unset list. 
	 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to modify the ACL
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type the list of users with access to the specified container
	 *
	 * @returns the new ACL in the \em result field. 
	 */ 
	extern int mod_acl(
			const lwfs_remote_pid *caller,
			const lwfs_mod_acl_args *args,
			const lwfs_rma *data_addr,
			void *result);

	/**
	 *  @brief Get a list of capabilities associated with a container. 
	 *
	 *  This method grants capabilities to users 
	 *  that have valid credentails and have appropriate permissions 
	 *  (i.e., their user ID is in the appropriate ACL) to access 
	 *  the container.  
	 * 
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to get a capability
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type the capability for access to container with op
	 *
	 * @returns the list of caps in the \em result field. 
	 * 
	 *  @note This method does not return partial results. If the 
	 *  authorization service fails to grant all requested capabilities,
	 *  \b lwfs_getcaps returns NULL for the resulting list of caps and 
	 *  reports the error in the return code embedded in the request handle. 
	 */
	extern int get_cap(
			const lwfs_remote_pid *caller,
			const lwfs_get_cap_args *args, 
			const lwfs_rma *data_addr,
			lwfs_cap *result);


	/**
	 *  @brief Verify a list of capabilities. 
	 *
	 *  This method verifies that the provided capabilities were 
	 *  generated by this authorization server and that they have
	 *  not been modified. If all of the caps are valid, this method 
	 *  also registers the requesting process (parameter \em pid) 
	 *  as a user of the caps.  The authorization server
	 *  uses the information in registration table to decide
	 *  who to contact when revoking access to capabilities. 
	 * 
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to verify the container
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type no result
	 *
	 * @returns The remote method returns \ref LWFS_OK if all the 
	 * caps are valid. 
	 */
	extern int verify_caps(
			const lwfs_remote_pid *caller,
			const lwfs_verify_caps_args *args, 
			const lwfs_rma *data_addr,
			void *result);


#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

