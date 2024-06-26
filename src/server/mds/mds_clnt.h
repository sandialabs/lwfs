/*-------------------------------------------------------------------------*/
/**  
 *   @file mds_clnt.h
 *   
 *   @brief Data structures and method prototypes 
 *   for the metadata service API for clients.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#ifndef _MDS_API_CLIENT_H_
#define _MDS_API_CLIENT_H_

#include "lwfs_xdr.h"
#include "mds/mds_xdr.h"
#include "comm/comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

extern lwfs_return_code_t mds_init_client(void); 

extern lwfs_return_code_t mds_fini_client(void);

extern lwfs_return_code_t mds_create(
	lwfs_obj_ref_t *, 
	mds_name_t *, 
	lwfs_cap_t *, 
	mds_res_N_rc_t *, 
	lwfs_request_t *); 

extern lwfs_return_code_t mds_remove(
	lwfs_obj_ref_t *, 
	lwfs_cap_t *, 
	mds_res_note_rc_t *, 
	lwfs_request_t *); 

extern lwfs_return_code_t mds_lookup(
	lwfs_obj_ref_t *, 
	mds_name_t *, 
	lwfs_cap_t *, 
	mds_res_N_rc_t *, 
	lwfs_request_t *); 

extern lwfs_return_code_t mds_lookup2(
	lwfs_obj_ref_t *node, 
	lwfs_cap_t *cap, 
	mds_res_note_rc_t *result,
	lwfs_request_t *req);

extern lwfs_return_code_t mds_mkdir(
	lwfs_obj_ref_t *, 
	mds_name_t *, 
	lwfs_cap_t *, 
	mds_res_N_rc_t *, 
	lwfs_request_t *); 

extern lwfs_return_code_t mds_rmdir(
	lwfs_obj_ref_t *, 
	lwfs_cap_t *, 
	mds_res_note_rc_t *, 
	lwfs_request_t *); 

extern lwfs_return_code_t mds_link(
	lwfs_obj_ref_t *src,
	lwfs_obj_ref_t *destdir, 
	mds_name_t *destname, 
	lwfs_cap_t *destcap,
	mds_res_N_rc_t *result,
	lwfs_request_t *req);

extern lwfs_return_code_t mds_rename(
	lwfs_obj_ref_t *objID,
	lwfs_cap_t *srccap,
	lwfs_obj_ref_t *destdir, 
	mds_name_t *destname, 
	lwfs_cap_t *destcap,
	mds_res_N_rc_t *result,
	lwfs_request_t *req);

extern lwfs_return_code_t mds_getattr(
	lwfs_obj_ref_t *obj, 
	lwfs_cap_t *cap,
	mds_res_note_rc_t *result,
	lwfs_request_t *req);

extern lwfs_return_code_t mds_setattr(
	lwfs_obj_ref_t *obj, 
	lwfs_cap_t *cap,
	mds_opaque_data_t *newattr, 
	mds_res_note_rc_t *result,
	lwfs_request_t *req);

extern lwfs_return_code_t mds_readdir(
		lwfs_obj_ref_t *dir,
		lwfs_cap_t *cap,
		mds_readdir_res_t *result,
		lwfs_request_t *req);

#else /* K&R C */

extern void mds_client_init(); 

extern lwfs_return_code_t mds_create();

extern lwfs_return_code_t mds_remove();

extern lwfs_return_code_t mds_lookup();

extern lwfs_return_code_t mds_lookup2();

extern lwfs_return_code_t mds_mkdir();

extern lwfs_return_code_t mds_rmdir();

extern lwfs_return_code_t mds_link();

extern lwfs_return_code_t mds_rename();

extern lwfs_return_code_t mds_getattr();

extern lwfs_return_code_t mds_setattr();

extern lwfs_return_code_t mds_readdir();

#endif /* K&R C */


#ifdef __cplusplus
}
#endif

#endif /* !_MDS_API_CLNT_H_ */
