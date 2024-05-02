/**  
 *   @file authr_clnt.h
 * 
 *   @brief Client side functions for accessing the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#include "client/authr_client/authr_client.h"

#ifndef _AUTHR_CLIENT_SYNC_H_
#define _AUTHR_CLIENT_SYNC_H_


#ifdef __cplusplus
extern "C" {
#endif


#if defined(__STDC__) || defined(__cplusplus)

	/**
	 * @brief Create a new container on the authorization server.
	 *
	 * @ingroup authr_api
	 *
	 * The <tt>\ref lwfs_create_container</tt> function generates a 
     * unique container ID on the authorization server and returns 
	 * the capability that allows the caller to modify that container (e.g.,
	 * to add access permissions). Initially, only one ACL exists 
	 * for the container. The client must call the \ref lwfs_create_acl
	 * or \ref lwfs_mod_acl functions to add new access-control lists 
	 * to the container. 
	 *
	 * @param txn_id @input_type  If not null, points to a structure that 
     *                       holds information about the transaction. 
	 * @param cid @input_type  The container ID to use for the new 
	 *                         container. Use LWFS_CID_ANY to let the 
	 *                         server choose a container ID. 
	 * @param cap    @input_type  Points to the capability structure that 
     *                       allows the client to create a new container.
	 * @param result @output_type If successful, points to the capability structure
     *                       that enables the holder to modify the ACLs on 
     *                       the new container. The container ID is 
     *                       encoded in the capability structure. 
	 * @param req    @output_type Points to the request structure. 
     *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */
	extern int lwfs_create_container_sync(
			const lwfs_service *authr_svc, 
			const lwfs_txn *txn_id,
			const lwfs_cid cid, 
			const lwfs_cap *cap,
			lwfs_cap *result);

	/** 
	 * @brief Create an ACL for a container/op pair. 
	 * 
	 * @ingroup authr_api
	 *
	 * The <tt>\ref lwfs_create_acl</tt> function creates a new access-control
	 * list associated with a particular operation. 
     *
	 * @param txn_id @input_type  If not null, points to a structure that 
     *                       holds information about the transaction. 
	 * @param cid    @input_type  The ID of the container. 
	 * @param container_op @input_type  The ID of the operation to allow
	 * @param set    @input_type  Points to the list of user IDs that represents
     *                       the access-control list. 
	 * @param cap    @input_type  Points to the capability structure that 
     *                       allows the client to create the ACL.
	 * @param req    @output_type Points to the request structure. 
     *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */
	extern int lwfs_create_acl_sync(
			const lwfs_service *authr_svc, 
			const lwfs_txn *txn_id,
			const lwfs_cid cid,
			const lwfs_container_op container_op,
			const lwfs_uid_array *set,
			const lwfs_cap *cap);


	/**
	 * @brief Get the access-control list for an 
	 *        container/op pair.
	 *
	 * @ingroup authr_api
	 *
	 * The <tt>\ref lwfs_get_acl</tt> function returns the 
	 * list of users that have authorization to perform a
	 * specified operation on a container. 
	 *
	 * @param cid    @input_type  The ID of the container. 
	 * @param container_op @input_type  The ID of the operation. 
	 * @param cap    @input_type  Points to the capability structure that 
     *                       allows the client to create the ACL.
	 * @param result @output_type If successful, points to the list of 
     *                       user IDs that represents the access-control 
     *                       list for the specified container/op pair. 
	 * @param req    @output_type Points to the request structure. 
	 *
	 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
	 *         id for this function because getting the access-control list 
	 *         does not change the state of the system. 
     *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */
	extern int lwfs_get_acl_sync(
			const lwfs_service *authr_svc,
			const lwfs_cid cid,
			const lwfs_container_op container_op,
			const lwfs_cap *cap,
			lwfs_uid_array *result);


	/**
	 *  @brief Get a capability associated with a container. 
	 *
	 *  @ingroup authr_api
	 *
	 *  The <tt>lwfs_get_cap</tt> function grants a capability 
	 *  that allows the holder to perform the requested 
	 *  operations on objects in a container.  Clients requesting 
	 *  the capability must have valid credentails and 
	 *  appropriate permissions to perform the operations 
	 *  (i.e., the user ID is in the appropriate ACL). 
	 *
	 * @param cid     @input_type  The ID of the container. 
	 * @param container_op @input_type  The requested operations (or'd together).
	 * @param cred @input_type    Points to a structure that represents
     *                       the credential of the user. 
	 * @param result  @output_type  If successful, points to the 
     *                       capability structure granted by the 
     *                       authorization service. 
	 * @param req    @output_type Points to the request structure. 
     *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 *
	 *  @note This function does not return partial results. If the 
	 *  authorization service fails to authorize all requested 
	 *  operations, <tt>\ref lwfs_get_cap "lwfs_get_cap()"</tt> 
	 *  returns an error and the \em result field is invalid. 
	 *
	 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
	 *         id for this function because getting the access-control list 
	 *         does not change the state of the system. 
	 */
	extern int lwfs_get_cap_sync(
			const lwfs_service *authr_svc, 
			const lwfs_cid cid,
			const lwfs_container_op container_op,
			const lwfs_cred *cred,
			lwfs_cap *result);

	/**
	 * @brief Modify the access-control list for an container/op pair.
	 *
	 * @ingroup authr_api
	 *
	 * This function changes the access-control list for a particular
	 * operation on an container of objects.  The arguments 
	 * include a list of users that require access (set) and 
	 * a list of users that no longer require access (unset).  
	 * To resolve conflicts that occur when a user ID appears in both
	 * lists, the implementation first grants access to the users 
	 * in the set list, then removes access from users in the unset list. 
	 *
	 * @param txn_id @input_type  If not null, points to a structure that 
     *                       holds information about the transaction. 
	 * @param cid    @input_type  The ID of the container. 
	 * @param container_op @input_type  The ID of the operation. 
	 * @param set @input_type     Points to the list of 
     *                       user IDs to add to the access-control 
     *                       list. 
	 * @param unset @input_type   Points to the list of 
     *                       user IDs to remove from the access-control 
     *                       list. 
	 * @param cap    @input_type  Points to the capability structure that 
     *                       allows the client to modify the ACL.
	 * @param req    @output_type Points to the request structure. 
     *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 */ 
	extern int lwfs_mod_acl_sync(
			const lwfs_service *authr_svc,
			const lwfs_txn *txn_id,
			const lwfs_cid cid,
			const lwfs_container_op container_op,
			const lwfs_uid_array *set,
			const lwfs_uid_array *unset,
			const lwfs_cap *cap);


	/**
	 * @brief Remove a container.
	 *
	 * @ingroup authr_api
	 *
	 * The <em>\ref lwfs_remove_container</em> function removes an 
	 * existing container ID from the authorization server. 
	 *
	 * @note This operation does not check for "orphan" entities (i.e., 
	 * existing objects, namespace entries, and so forth) before removing
	 * the container ID. We leave this responsibility to the client. 
     *
	 * @param txn_id @input_type  If not null, points to a structure that 
     *                       holds information about the transaction. 
	 * @param cid    @input_type  The ID of the container to remove.
	 * @param cap    @input_type  Points to the capability structure that 
     *                       allows the client to remove the container.
	 * @param req    @output_type Points to the request structure. 
     *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */
	extern int lwfs_remove_container_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const lwfs_cid cid,
			const lwfs_cap *cap);

	/**
	 *  @brief Verify a list of capabilities. 
	 *
	 *  @ingroup authr_api
	 *
	 *  This function verifies that the provided capabilities were 
	 *  generated by this authorization server and that they have
	 *  not been modified. If all of the capabilities are valid, 
	 *  this function also registers the requesting process 
	 *  (parameter \em pid) as a user of the caps.  The authorization server
	 *  uses the information in the registration table to decide
	 *  who to contact when revoking access to capabilities. 
	 * 
	 * @param caps   @input_type  Points to the array of capability structures
     *                       that the caller wants to verify.
	 * @param num_caps @input_type  The number of capabilities in the array. 
	 * @param req    @output_type Points to the request structure. 
	 *
     * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */
	extern int lwfs_verify_caps_sync(
			const lwfs_service *authr_svc,
			const lwfs_cap *caps,
			const int num_caps);


#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

