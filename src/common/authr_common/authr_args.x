/*-------------------------------------------------------------------------*/
/**  @file authr_args.x
 *   
 *   @brief XDR definitions for the argument structures for the 
 *   \ref authr_api "authorization API" for the LWFS. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 969 $.
 *   $Date: 2006-08-28 15:40:44 -0600 (Mon, 28 Aug 2006) $.
 *
 */

/* include files for security_xdr.h */
#ifdef RPC_HDR
%#include "common/types/types.h"
#endif


/**
 * @brief Arguments for the \ref lwfs_create_container method that 
 * have to be passed to the authorization server. 
 */
struct lwfs_create_container_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The Container ID to use for the new container. */
	lwfs_cid cid; 

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/**
 * @brief Arguments for the \ref lwfs_remove_container method that 
 * have to be passed to the authorization server. 
 */
struct lwfs_remove_container_args {
	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The container ID to be removed. */
	lwfs_cid cid;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/**
 * @brief Arguments for the \ref lwfs_create_acl method that have 
 * to be passed to the server. 
 */
struct lwfs_create_acl_args {
	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The container ID to be removed. */
	lwfs_cid cid;

	/** @brief The operation to modify. */
	lwfs_container_op container_op; 

	/** @brief the list of user IDs to add to the ACL (may be NULL). */
	lwfs_uid_array *set; 

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/**
 * @brief Arguments for the \ref lwfs_get_acl method that have to be 
 * passed to the authorization server. 
 */
struct lwfs_get_acl_args {

	/** @brief The container ID to be removed. */
	lwfs_cid cid;

	/** @brief The operation. */
	lwfs_container_op container_op; 

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/** 
 * @brief Arguments to the \ref lwfs_mod_acl method that have to be sent
 * to the authorization server.
 */
struct lwfs_mod_acl_args {
	/** @brief The transaction ID. */
	lwfs_txn *txn_id;

	/** @brief The container ID to be removed. */
	lwfs_cid cid;

	/** @brief The operation to modify. */
	lwfs_container_op container_op; 

	/** @brief the list of user IDs to add to the ACL (may be NULL). */
	lwfs_uid_array *set; 

	/** @brief the list of user IDs to remove from the ACL (may be NULL). */
	lwfs_uid_array *unset; 

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/** 
 * @brief Arguments to the \ref lwfs_get_cap method that have to be sent
 * to the authorization server.
 */
struct lwfs_get_cap_args {
	/** @brief The container ID to be removed. */
	lwfs_cid cid;

	/** @brief The container operations for the capability. */
	lwfs_container_op container_op;

	/** @brief The credential of the user requesting cap. */
	lwfs_cred *cred; 
};

/** 
 * @brief Arguments to the \ref lwfs_verify_caps method that have 
 * to be sent to the authorization server.
 */
struct lwfs_verify_caps_args {
	/** @brief The list of caps to verify. */
	lwfs_cap_array *cap_array;
};



