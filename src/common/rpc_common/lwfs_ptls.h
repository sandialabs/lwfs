/*-------------------------------------------------------------------------*/
/**  
 *   @file lwfs_ptls.h
 *   
 *   @brief Method prototypes for the lwfs_portals API.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1367 $
 *   $Date: 2007-04-24 11:36:19 -0600 (Tue, 24 Apr 2007) $
 *
 */

#ifndef _LWFS_PTLS_H_
#define _LWFS_PTLS_H_

#include "config.h"
#include "common/rpc_common/ptl_wrap.h"

#include PORTALS_HEADER
#include PORTALS_NAL_HEADER
#include PORTALS_RT_HEADER

#include "common/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LWFS_PTL_MAX_MES 100   /* maximum number of match entries */
#define LWFS_PTL_MAX_MDS 100   /* max number of memory descriptors */
#define LWFS_PTL_MAX_EQS 100   /* max number of event queues */
#define LWFS_PTL_MAX_ACI  0   /* max atable index */
#define LWFS_PTL_MAX_PTI 10   /* max ptable index */

	/*------------- Data structures ---------------*/

	/*------------- Data structures ---------------*/

	/*------------- Method prototypes ---------------*/



#if defined(__STDC__) || defined(__cplusplus)

	extern int lwfs_ptl_init(ptl_interface_t iface, lwfs_nid my_pid);

	extern int lwfs_ptl_fini(void);

	extern int lwfs_ptl_get_ni(
			ptl_handle_ni_t *ni_handle);

	extern int lwfs_ptl_eq_poll(
		ptl_handle_eq_t *eq_handle, 
		int size, 
		int timeout, 
		ptl_event_t *event,
		int *which);

	extern int lwfs_ptl_eq_timedwait(
			ptl_handle_eq_t eq_handle, 
			int timeout, 
			ptl_event_t *event);

	extern int lwfs_ptl_eq_wait(
			ptl_handle_eq_t eq_handle, 
			ptl_event_t *event);

	extern int lwfs_ptl_eq_alloc(
			ptl_handle_ni_t ni_handle, 
			ptl_size_t count,
			void (* eq_handler)(ptl_event_t *event), 
			ptl_handle_eq_t *eq_handle);

	extern int lwfs_ptl_put(
			const void *src_buf, 
			const int len,
			const lwfs_rma *dest_addr);

	extern int lwfs_ptl_get(
			void *dest_buf, 
			const int len,
			const lwfs_rma *src_addr);

	extern int lwfs_ptl_get_id(lwfs_remote_pid *id);

	extern int lwfs_ptl_me_attach(
			ptl_handle_ni_t ni_handle, 
			ptl_pt_index_t pt_index,
			ptl_process_id_t match_id, 
			ptl_match_bits_t match_bits,
			ptl_match_bits_t ignore_bits, 
			ptl_unlink_t unlink,
			ptl_ins_pos_t postion, 
			ptl_handle_me_t *me_handle);

	extern int lwfs_ptl_md_attach(
			ptl_handle_me_t me_handle, 
			ptl_md_t md, 
			ptl_unlink_t unlink_op,
			ptl_handle_md_t *md_handle);

	extern int lwfs_ptl_md_unlink(
			ptl_handle_md_t md_handle);

	extern int lwfs_ptl_eq_free(
			ptl_handle_eq_t eq_handle);

	void lwfs_ptl_use_locks(int should_lock);

	int lwfs_ptl_lock();

	int lwfs_ptl_unlock();

#else /* K&R C */


#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_MESSAGING_H_ */
