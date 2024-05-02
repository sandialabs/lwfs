/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "../../../../../src/common/storage_common/ss_args.h"

bool_t
xdr_ss_create_obj_args (XDR *xdrs, ss_create_obj_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_remove_obj_args (XDR *xdrs, ss_remove_obj_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_read_args (XDR *xdrs, ss_read_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->src_obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_lwfs_size (xdrs, &objp->src_offset))
		 return FALSE;
	 if (!xdr_lwfs_size (xdrs, &objp->len))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_fsync_args (XDR *xdrs, ss_fsync_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_write_args (XDR *xdrs, ss_write_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->dest_obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_lwfs_size (xdrs, &objp->dest_offset))
		 return FALSE;
	 if (!xdr_lwfs_size (xdrs, &objp->len))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_stat_args (XDR *xdrs, ss_stat_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_listattrs_args (XDR *xdrs, ss_listattrs_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_getattrs_args (XDR *xdrs, ss_getattrs_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->names, sizeof (lwfs_name_array), (xdrproc_t) xdr_lwfs_name_array))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_setattrs_args (XDR *xdrs, ss_setattrs_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->attrs, sizeof (lwfs_attr_array), (xdrproc_t) xdr_lwfs_attr_array))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_rmattrs_args (XDR *xdrs, ss_rmattrs_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->names, sizeof (lwfs_name_array), (xdrproc_t) xdr_lwfs_name_array))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_getattr_args (XDR *xdrs, ss_getattr_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_lwfs_name (xdrs, &objp->name))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_setattr_args (XDR *xdrs, ss_setattr_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->attr, sizeof (lwfs_attr), (xdrproc_t) xdr_lwfs_attr))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_rmattr_args (XDR *xdrs, ss_rmattr_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_lwfs_name (xdrs, &objp->name))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_ss_truncate_args (XDR *xdrs, ss_truncate_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->obj, sizeof (lwfs_obj), (xdrproc_t) xdr_lwfs_obj))
		 return FALSE;
	 if (!xdr_lwfs_ssize (xdrs, &objp->size))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}
