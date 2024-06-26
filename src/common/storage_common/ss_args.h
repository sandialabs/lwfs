/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SS_ARGS_H_RPCGEN
#define _SS_ARGS_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "common/types/types.h"

struct ss_create_obj_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_cap *cap;
};
typedef struct ss_create_obj_args ss_create_obj_args;

struct ss_remove_obj_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_cap *cap;
};
typedef struct ss_remove_obj_args ss_remove_obj_args;

struct ss_read_args {
	lwfs_txn *txn_id;
	lwfs_obj *src_obj;
	lwfs_size src_offset;
	lwfs_size len;
	lwfs_cap *cap;
};
typedef struct ss_read_args ss_read_args;

struct ss_fsync_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_cap *cap;
};
typedef struct ss_fsync_args ss_fsync_args;

struct ss_write_args {
	lwfs_txn *txn_id;
	lwfs_obj *dest_obj;
	lwfs_size dest_offset;
	lwfs_size len;
	lwfs_cap *cap;
};
typedef struct ss_write_args ss_write_args;

struct ss_stat_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_cap *cap;
};
typedef struct ss_stat_args ss_stat_args;

struct ss_listattrs_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_cap *cap;
};
typedef struct ss_listattrs_args ss_listattrs_args;

struct ss_getattrs_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_name_array *names;
	lwfs_cap *cap;
};
typedef struct ss_getattrs_args ss_getattrs_args;

struct ss_setattrs_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_attr_array *attrs;
	lwfs_cap *cap;
};
typedef struct ss_setattrs_args ss_setattrs_args;

struct ss_rmattrs_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_name_array *names;
	lwfs_cap *cap;
};
typedef struct ss_rmattrs_args ss_rmattrs_args;

struct ss_getattr_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_name name;
	lwfs_cap *cap;
};
typedef struct ss_getattr_args ss_getattr_args;

struct ss_setattr_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_attr *attr;
	lwfs_cap *cap;
};
typedef struct ss_setattr_args ss_setattr_args;

struct ss_rmattr_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_name name;
	lwfs_cap *cap;
};
typedef struct ss_rmattr_args ss_rmattr_args;

struct ss_truncate_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj;
	lwfs_ssize size;
	lwfs_cap *cap;
};
typedef struct ss_truncate_args ss_truncate_args;

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_ss_create_obj_args (XDR *, ss_create_obj_args*);
extern  bool_t xdr_ss_remove_obj_args (XDR *, ss_remove_obj_args*);
extern  bool_t xdr_ss_read_args (XDR *, ss_read_args*);
extern  bool_t xdr_ss_fsync_args (XDR *, ss_fsync_args*);
extern  bool_t xdr_ss_write_args (XDR *, ss_write_args*);
extern  bool_t xdr_ss_stat_args (XDR *, ss_stat_args*);
extern  bool_t xdr_ss_listattrs_args (XDR *, ss_listattrs_args*);
extern  bool_t xdr_ss_getattrs_args (XDR *, ss_getattrs_args*);
extern  bool_t xdr_ss_setattrs_args (XDR *, ss_setattrs_args*);
extern  bool_t xdr_ss_rmattrs_args (XDR *, ss_rmattrs_args*);
extern  bool_t xdr_ss_getattr_args (XDR *, ss_getattr_args*);
extern  bool_t xdr_ss_setattr_args (XDR *, ss_setattr_args*);
extern  bool_t xdr_ss_rmattr_args (XDR *, ss_rmattr_args*);
extern  bool_t xdr_ss_truncate_args (XDR *, ss_truncate_args*);

#else /* K&R C */
extern bool_t xdr_ss_create_obj_args ();
extern bool_t xdr_ss_remove_obj_args ();
extern bool_t xdr_ss_read_args ();
extern bool_t xdr_ss_fsync_args ();
extern bool_t xdr_ss_write_args ();
extern bool_t xdr_ss_stat_args ();
extern bool_t xdr_ss_listattrs_args ();
extern bool_t xdr_ss_getattrs_args ();
extern bool_t xdr_ss_setattrs_args ();
extern bool_t xdr_ss_rmattrs_args ();
extern bool_t xdr_ss_getattr_args ();
extern bool_t xdr_ss_setattr_args ();
extern bool_t xdr_ss_rmattr_args ();
extern bool_t xdr_ss_truncate_args ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_SS_ARGS_H_RPCGEN */
