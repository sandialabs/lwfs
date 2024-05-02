/**  
 *   @file authr_client_sync.c
 * 
 *   @brief Implementation of the client-side methods for 
 *   accessing the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */


#include "authr_client.h"
#include "authr_client_sync.h"

/* ----------- Auth client API ---------------*/


/**
 * @brief Create a new container on the authorization server.
 *
 * The \ref lwfs_create_container method generates a unique container
 * ID on the authorization server and returns a capability that enables
 * the caller to modify the ACLs associated with the container.  
 *
 * @param txn_id @input transaction id.
 * @param cid         @input  the container ID to use.
 * @param cap         @input  capability that allows the client to create a cid.
 * @param result      @output a new container ID.
 * @param req         @output request handle (used to test for completion).
 */
int lwfs_create_container_sync(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid, 
		const lwfs_cap *cap,
		lwfs_cap *result)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 

    /* create a container (returns a cap to modify acls) */
    rc = lwfs_create_container(authr_svc, txn_id, cid, cap, result, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call create_container: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for response from server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}

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
 * @param txn_id @input transaction id.
 * @param cid  @input container ID to remove.
 * @param cap  @input capability that allows the client to remove a cid.
 * @param req  @output request handle (used to test for completion).
 */
int lwfs_remove_container_sync(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_cap *cap)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 

    /* remove a container (returns a cap to modify acls) */
    rc = lwfs_remove_container(authr_svc, txn_id, cid, cap, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call remove_container: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for a response from the server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


/** 
 * @brief Create an ACL for a container/op pair. 
 * 
 * The \b lwfs_create_acl method creates a new access-control
 * list associated with a particular operation. 
 *
 * @param txn_id @input transaction id.
 * @param cid     @input the container id.
 * @param op      @input the operation (e.g., read, write, ...).
 * @param set     @input the list of users that want access.
 * @param result  @input capability that allows the client to create an acl.
 * @param req     @output request handle (used to test for completion). 
 */
int lwfs_create_acl_sync(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_uid_array *set,
		const lwfs_cap *cap)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 


    log_debug(authr_debug_level, "beginning lwfs_bcreate_acl(cid=%Lu, container_op=%u)",
	    cid, container_op);

    /* remove a container (returns a cap to modify acls) */
    rc = lwfs_create_acl(authr_svc, txn_id, cid, container_op, set, cap, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call create_acl: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for response from server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


/**
 * @brief Get the access-control list for an 
 *        container/op pair.
 *
 * The \b lwfs_get_acl method returns a list of users that have 
 * a particular type of access to a container. 
 *
 * @param cid  @input the container id.
 * @param op   @input  the operation (i.e., LWFS_OP_READ, LWFS_OP_WRITE, ...)
 * @param cap  @input  the capability that allows us to get the ACL.
 * @param result  @output the list of users with access. 
 * @param req  @output the request handle (used to test for completion). 
 *
 * @returns an ACL in the \em result field. 
 *
 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
 *         id for this method because getting the access-control list 
 *         does not change the state of the system. 
 */
int lwfs_get_acl_sync(
		const lwfs_service *authr_svc,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_cap *cap,
		lwfs_uid_array *result)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 

    log_debug(authr_debug_level, "beginning lwfs_bget_acl(cid=%Lu, container_op=%u)",
	    cid, container_op);

    /* remove a container (returns a cap to modify acls) */
    rc = lwfs_get_acl(authr_svc, cid, container_op, cap, result, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call create_acl: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for response from server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}



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
 * @param txn_id @input transaction id.
 * @param cid   @input the container id.
 * @param op    @input the operation (e.g., read, write, ...).
 * @param set   @input the list of users that want access.
 * @param unset @input the list of users that do not get access.
 * @param cap   @input the capability that allows the user to change the acl.
 * @param req   @output the request handle (used to test for completion). 
 *
 */ 
int lwfs_mod_acl_sync(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_uid_array *set,
		const lwfs_uid_array *unset,
		const lwfs_cap *cap)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 

    /* remove a container (returns a cap to modify acls) */
    rc = lwfs_mod_acl(authr_svc, txn_id, cid, container_op, set, unset, cap, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call lwfs_mod_acl: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for response from server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


/**
 *  @brief Get a capability for a container. 
 *
 *  This method allocates a list of capabilities for users 
 *  that have valid credentails and have appropriate permissions 
 *  (i.e., their user ID is in the appropriate ACL) to access 
 *  the container.  
 * 
 * @param cid     @input  the container id.
 * @param container_op @input  the container_op to request (e.g., LWFS_CID_READ | LWFS_CID_WRITE)
 * @param cred   @input  credential of the user. 
 * @param result @output the resulting list of caps.
 * @param req    @output the request handle (used to test for completion). 
 *
 * @returns the list of caps in the \em result field. 
 * 
 *  @note This method does not return partial results. If the 
 *  authorization service fails to grant all requested capabilities,
 *  \b lwfs_getcaps returns NULL for the resulting list of caps and 
 *  reports the error in the return code embedded in the request handle. 
 *
 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
 *         id for this method because getting the access-control list 
 *         does not change the state of the system. 
 */
int lwfs_get_cap_sync(
		const lwfs_service *authr_svc,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_cred *cred,
		lwfs_cap *result)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 

    log_debug(authr_debug_level, "beginning lwfs_bget_cap(cid=%Lu, container_op=%u)",
	    cid, container_op);

    /* remove a container (returns a cap to modify acls) */
    rc = lwfs_get_cap(authr_svc, cid, container_op, cred, result, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call lwfs_get_cap: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for response from server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


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
 * @param caps    @input  the array of capabilities to verify
 * @param n      @input  the number of caps in the array. 
 * @param req    @output the request handle (used to test for completion). 
 *
 * @returns The remote method returns \ref LWFS_OK if all the 
 * caps are valid. 
 */
int lwfs_verify_caps_sync(
		const lwfs_service *authr_svc,
		const lwfs_cap *caps,
		const int num_caps)
{
    int rc = LWFS_OK;
    int rc2 = LWFS_OK;
    lwfs_request req; 

    /* remove a container (returns a cap to modify acls) */
    rc = lwfs_verify_caps(authr_svc, caps, num_caps, &req); 
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "could not call lwfs_get_cap: %s",
		lwfs_err_str(rc));
	return rc;
    }

    /* wait for response from server */
    rc2 = lwfs_wait(&req, &rc); 

    if (rc2 != LWFS_OK) {
	log_error(authr_debug_level, "error waiting on request: %s",
		lwfs_err_str(rc2));
	return rc2; 
    }

    if (rc != LWFS_OK) {
	log_warn(authr_debug_level, "error in remote method: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}

