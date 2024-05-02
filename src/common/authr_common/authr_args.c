/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "../../../../../src/common/authr_common/authr_args.h"

bool_t
xdr_lwfs_create_container_args (XDR *xdrs, lwfs_create_container_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_lwfs_cid (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_lwfs_remove_container_args (XDR *xdrs, lwfs_remove_container_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_lwfs_cid (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_lwfs_create_acl_args (XDR *xdrs, lwfs_create_acl_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_lwfs_cid (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_lwfs_container_op (xdrs, &objp->container_op))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->set, sizeof (lwfs_uid_array), (xdrproc_t) xdr_lwfs_uid_array))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_lwfs_get_acl_args (XDR *xdrs, lwfs_get_acl_args *objp)
{
	register int32_t *buf;

	 if (!xdr_lwfs_cid (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_lwfs_container_op (xdrs, &objp->container_op))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_lwfs_mod_acl_args (XDR *xdrs, lwfs_mod_acl_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->txn_id, sizeof (lwfs_txn), (xdrproc_t) xdr_lwfs_txn))
		 return FALSE;
	 if (!xdr_lwfs_cid (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_lwfs_container_op (xdrs, &objp->container_op))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->set, sizeof (lwfs_uid_array), (xdrproc_t) xdr_lwfs_uid_array))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->unset, sizeof (lwfs_uid_array), (xdrproc_t) xdr_lwfs_uid_array))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cap, sizeof (lwfs_cap), (xdrproc_t) xdr_lwfs_cap))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_lwfs_get_cap_args (XDR *xdrs, lwfs_get_cap_args *objp)
{
	register int32_t *buf;

	 if (!xdr_lwfs_cid (xdrs, &objp->cid))
		 return FALSE;
	 if (!xdr_lwfs_container_op (xdrs, &objp->container_op))
		 return FALSE;
	 if (!xdr_pointer (xdrs, (char **)&objp->cred, sizeof (lwfs_cred), (xdrproc_t) xdr_lwfs_cred))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_lwfs_verify_caps_args (XDR *xdrs, lwfs_verify_caps_args *objp)
{
	register int32_t *buf;

	 if (!xdr_pointer (xdrs, (char **)&objp->cap_array, sizeof (lwfs_cap_array), (xdrproc_t) xdr_lwfs_cap_array))
		 return FALSE;
	return TRUE;
}