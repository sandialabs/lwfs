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
#include "naming_xdr.h"
#include "naming_args.h"
#include "naming_opcodes.h"


/**
 * @brief Register XDR encoding functions for the authorization service. 
 */
int register_naming_encodings(void)
{
	int rc = LWFS_OK; 

	lwfs_register_xdr_encoding(LWFS_OP_CREATE_DIR,
			(xdrproc_t)&xdr_lwfs_create_dir_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry);

	lwfs_register_xdr_encoding(LWFS_OP_REMOVE_DIR,
			(xdrproc_t)&xdr_lwfs_remove_dir_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry);

	lwfs_register_xdr_encoding(LWFS_OP_CREATE_FILE,
			(xdrproc_t)&xdr_lwfs_create_file_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry);

	lwfs_register_xdr_encoding(LWFS_OP_CREATE_LINK,
			(xdrproc_t)&xdr_lwfs_create_link_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry);

	lwfs_register_xdr_encoding(LWFS_OP_UNLINK,
			(xdrproc_t)&xdr_lwfs_unlink_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry);

	lwfs_register_xdr_encoding(LWFS_OP_LOOKUP,
			(xdrproc_t)&xdr_lwfs_lookup_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry);

	lwfs_register_xdr_encoding(LWFS_OP_LIST_DIR,
			(xdrproc_t)&xdr_lwfs_list_dir_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_ns_entry_array);

	lwfs_register_xdr_encoding(LWFS_OP_NAME_STAT,
			(xdrproc_t)&xdr_lwfs_name_stat_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_stat_data);

	lwfs_register_xdr_encoding(LWFS_OP_CREATE_NAMESPACE,
			(xdrproc_t)&xdr_lwfs_create_namespace_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_namespace);

	lwfs_register_xdr_encoding(LWFS_OP_REMOVE_NAMESPACE,
			(xdrproc_t)&xdr_lwfs_remove_namespace_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_namespace);

	lwfs_register_xdr_encoding(LWFS_OP_GET_NAMESPACE,
			(xdrproc_t)&xdr_lwfs_get_namespace_args,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_namespace);

	lwfs_register_xdr_encoding(LWFS_OP_LIST_NAMESPACES,
			(xdrproc_t)NULL,
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_namespace_array);

	return rc;
}

