/**  
 *   @file authr_xdr.c
 * 
 *   @brief XDR encoding functions for the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 540 $
 *   $Date: 2006-01-11 16:57:56 -0700 (Wed, 11 Jan 2006) $
 */

#include "common/rpc_common/rpc_xdr.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "authr_xdr.h"
#include "authr_args.h"
#include "authr_opcodes.h"


/**
 * @brief Register XDR encoding functions for the authorization service. 
 */
int register_authr_encodings(void)
{
	int rc = LWFS_OK; 

	/* create container */
	lwfs_register_xdr_encoding(LWFS_OP_CREATE_CONTAINER,
			(xdrproc_t)&xdr_lwfs_create_container_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_cap);

	/* remove container */
	lwfs_register_xdr_encoding(LWFS_OP_REMOVE_CONTAINER,
			(xdrproc_t)&xdr_lwfs_remove_container_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* create acl */
	lwfs_register_xdr_encoding(LWFS_OP_CREATE_ACL,
			(xdrproc_t)&xdr_lwfs_create_acl_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* get acl */
	lwfs_register_xdr_encoding(LWFS_OP_GET_ACL,
			(xdrproc_t)&xdr_lwfs_get_acl_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_uid_array);

	/* mod acl */
	lwfs_register_xdr_encoding(LWFS_OP_MOD_ACL,
			(xdrproc_t)&xdr_lwfs_mod_acl_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_uid_array);

	/* get caps */
	lwfs_register_xdr_encoding(LWFS_OP_GET_CAP,
			(xdrproc_t)&xdr_lwfs_get_cap_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_cap);

	/* verify caps */
	lwfs_register_xdr_encoding(LWFS_OP_VERIFY_CAPS,
			(xdrproc_t)&xdr_lwfs_verify_caps_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	return rc;
}

