/**  
 *   @file ss_xdr.c
 * 
 *   @brief Register XDR encoding functions for the storage service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 540 $
 *   $Date: 2006-01-11 16:57:56 -0700 (Wed, 11 Jan 2006) $
 */

#include "common/types/types.h"
#include "common/rpc_common/rpc_xdr.h"
#include "ss_opcodes.h"
#include "ss_xdr.h"
#include "ss_args.h"

/** 
 * @brief Register xdr encodings for storage server operations. 
 */
int register_ss_encodings()
{
	int rc = LWFS_OK; 

	/* create object */
	lwfs_register_xdr_encoding(LWFS_OP_CREATE, 
			(xdrproc_t)&xdr_ss_create_obj_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_obj);

	/* remove object */
	lwfs_register_xdr_encoding(LWFS_OP_REMOVE, 
			(xdrproc_t)&xdr_ss_remove_obj_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);


	/* read */
	lwfs_register_xdr_encoding(LWFS_OP_READ, 
			(xdrproc_t)&xdr_ss_read_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_size);

	/* write */
	lwfs_register_xdr_encoding(LWFS_OP_WRITE, 
			(xdrproc_t)&xdr_ss_write_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* fsync */
	lwfs_register_xdr_encoding(LWFS_OP_FSYNC, 
			(xdrproc_t)&xdr_ss_fsync_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* stat */
	lwfs_register_xdr_encoding(LWFS_OP_STAT, 
			(xdrproc_t)&xdr_ss_stat_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_stat_data);

	/* listattrs */
	lwfs_register_xdr_encoding(LWFS_OP_LISTATTRS, 
			(xdrproc_t)&xdr_ss_listattrs_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_name_array);

	/* getattrs */
	lwfs_register_xdr_encoding(LWFS_OP_GETATTRS, 
			(xdrproc_t)&xdr_ss_getattrs_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_attr_array);

	/* setattrs */
	lwfs_register_xdr_encoding(LWFS_OP_SETATTRS, 
			(xdrproc_t)&xdr_ss_setattrs_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* rmattrs */
	lwfs_register_xdr_encoding(LWFS_OP_RMATTRS, 
			(xdrproc_t)&xdr_ss_rmattrs_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* getattr */
	lwfs_register_xdr_encoding(LWFS_OP_GETATTR, 
			(xdrproc_t)&xdr_ss_getattr_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_attr);

	/* setattr */
	lwfs_register_xdr_encoding(LWFS_OP_SETATTR, 
			(xdrproc_t)&xdr_ss_setattr_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* rmattr */
	lwfs_register_xdr_encoding(LWFS_OP_RMATTR, 
			(xdrproc_t)&xdr_ss_rmattr_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_void);

	/* truncate */
	lwfs_register_xdr_encoding(LWFS_OP_TRUNCATE, 
			(xdrproc_t)&xdr_ss_truncate_args, 
			(xdrproc_t)NULL,
			(xdrproc_t)&xdr_lwfs_stat_data);

	return rc; 
}
