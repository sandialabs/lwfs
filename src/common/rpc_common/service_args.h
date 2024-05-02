/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SERVICE_ARGS_H_RPCGEN
#define _SERVICE_ARGS_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "common/types/types.h"

struct lwfs_get_service_args {
	int id;
};
typedef struct lwfs_get_service_args lwfs_get_service_args;

struct lwfs_kill_service_args {
	int id;
};
typedef struct lwfs_kill_service_args lwfs_kill_service_args;

struct lwfs_trace_reset_args {
	int ftype;
	char *fname;
	int enable_flag;
};
typedef struct lwfs_trace_reset_args lwfs_trace_reset_args;

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_lwfs_get_service_args (XDR *, lwfs_get_service_args*);
extern  bool_t xdr_lwfs_kill_service_args (XDR *, lwfs_kill_service_args*);
extern  bool_t xdr_lwfs_trace_reset_args (XDR *, lwfs_trace_reset_args*);

#else /* K&R C */
extern bool_t xdr_lwfs_get_service_args ();
extern bool_t xdr_lwfs_kill_service_args ();
extern bool_t xdr_lwfs_trace_reset_args ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_SERVICE_ARGS_H_RPCGEN */
