/*-------------------------------------------------------------------------*/
/**  
 *   @file mds_srvr.h
 *   
 *   @brief Data structures and method prototypes 
 *   for the server-side methods for the metadata 
 *   service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#ifndef _MDS_API_SRVR_H_
#define _MDS_API_SRVR_H_

#include <portals/p30.h>
#include "lwfs_xdr.h"        /* common types for the LWFS */
#include "mds/mds_xdr.h"         /* mds data types that need to be communicated */
#include "comm/comm.h"            /* types and method prototypes for messaging */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

extern void mds_srvr_init(); 

extern lwfs_return_code_t mds_create_srvr(
	mds_args_Nn_t *, 
	mds_res_N_rc_t *);

extern lwfs_return_code_t mds_remove_srvr(
	mds_node_args_t *,
	mds_res_note_rc_t *);

extern lwfs_return_code_t mds_lookup2_srvr(
	mds_args_N_t *, 
	mds_res_note_rc_t *);

extern lwfs_return_code_t mds_lookup_srvr(
	mds_args_Nn_t *, 
	mds_res_N_rc_t *);

extern lwfs_return_code_t mds_mkdir_srvr(
	mds_args_Nn_t *, 
	mds_res_N_rc_t *);

extern lwfs_return_code_t mds_rmdir_srvr(
	mds_args_N_t *, 
	mds_res_note_rc_t *);

extern lwfs_return_code_t mds_link_srvr(
	mds_args_NNn_t *, 
	mds_res_N_rc_t *);

extern lwfs_return_code_t mds_rename_srvr(
	mds_args_NNn_t *, 
	mds_res_N_rc_t *);

extern lwfs_return_code_t mds_getattr_srvr(
	mds_node_args_t *,
	mds_res_note_rc_t *);

extern lwfs_return_code_t mds_setattr_srvr(
	mds_node_args_t *,
	mds_res_note_rc_t *);

extern lwfs_return_code_t mds_readdir_srvr(
	mds_args_N_t *args,
	mds_readdir_res_t *result);

#else /* K&R C */

extern lwfs_return_code_t mds_create_srvr();

extern lwfs_return_code_t mds_remove_srvr();

extern lwfs_return_code_t mds_lookup2_srvr();

extern lwfs_return_code_t mds_lookup_srvr();

extern lwfs_return_code_t mds_mkdir_srvr();

extern lwfs_return_code_t mds_rmdir_srvr();

extern lwfs_return_code_t mds_link_srvr();

extern lwfs_return_code_t mds_rename_srvr();

extern lwfs_return_code_t mds_getattr_srvr();

extern lwfs_return_code_t mds_setattr_srvr();

extern lwfs_return_code_t mds_readdir_srvr();

#endif /* K&R C */


#ifdef __cplusplus
}
#endif

#endif /* !_MDS_API_SRVR_H_ */
