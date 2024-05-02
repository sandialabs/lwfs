/**  
 *   @file ptl_wrap.h
 *   
 *   @brief  Wrappers around the Portals interface.  
 *
 *   Since we're not sure how thread-safe the portals code is, we provide
 *   wrappers that protect portals functions using pthread mutexes. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1.23 $
 *   $Date: 2005/11/09 20:15:51 $
 *
 */

#ifndef _PTL_WRAP_H_
#define _PTL_WRAP_H_

#include "config.h"
#include "common/types/lwfs_int.h"
#include PORTALS_HEADER

#ifndef PTL_IFACE_SERVER
#define PTL_IFACE_SERVER PTL_IFACE_DEFAULT
#endif

#ifndef PTL_IFACE_CLIENT
#define PTL_IFACE_CLIENT PTL_IFACE_DEFAULT
#endif

/* Cray extensions */
#ifndef PTL_IFACE_DUP
#define PTL_IFACE_DUP PTL_OK
#endif

#ifndef PTL_MD_EVENT_AUTO_UNLINK_ENABLE
#define PTL_MD_EVENT_AUTO_UNLINK_ENABLE 0
#endif

#ifndef PTL_MD_EVENT_MANUAL_UNLINK_ENABLE
#define PTL_MD_EVENT_MANUAL_UNLINK_ENABLE 0
#endif

#ifndef HAVE_PTLERRORSTR
#define PtlErrorStr(a) ""
#endif

#ifndef HAVE_PTLNIFAILSTR
#define PtlNIFailStr(a,b) ""
#endif

#ifndef HAVE_PTLEVENTKINDSTR
#define PtlEventKindStr(a) ""
#endif

#ifndef HAVE_PTL_TIME_T
typedef uint32_t ptl_time_t;
#endif

#ifndef HAVE_PTL_EQ_HANDLER_T
typedef void (*ptl_eq_handler_t)(ptl_event_t *event);
#endif 

#ifndef PTL_EQ_HANDLER_NONE
#define PTL_EQ_HANDLER_NONE (ptl_eq_handler_t)NULL
#endif


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

    extern int lwfs_PtlInit(int *max_interfaces);

    extern void lwfs_PtlFini(void);

    /*
     * P3.3 API spec: section 3.5
     */
    extern int lwfs_PtlNIInit(ptl_interface_t interface, ptl_pid_t pid,
	    ptl_ni_limits_t *desired, ptl_ni_limits_t *actual,
	    ptl_handle_ni_t *ni_handle);

    extern int lwfs_PtlNIFini(ptl_handle_ni_t ni_handle);

    extern int lwfs_PtlNIStatus(ptl_handle_ni_t ni_handle, ptl_sr_index_t register_index,
	    ptl_sr_value_t *status);

    extern int lwfs_PtlNIDist(ptl_handle_ni_t ni_handle, ptl_process_id_t process,
	    unsigned long *distance);

    extern int lwfs_PtlNIHandle(ptl_handle_any_t handle, ptl_handle_ni_t *ni_handle);

    /*
     * P3.3 API spec: section 3.6
     */
    extern int lwfs_PtlGetUid(ptl_handle_ni_t ni_handle, ptl_uid_t *uid);

    /*
     * P3.3 API spec: section 3.7
     */
    extern int lwfs_PtlGetId(ptl_handle_ni_t ni_handle, ptl_process_id_t *id);

    /*
     * P3.3 API spec: section 3.8
     */
    extern int lwfs_PtlGetJid(ptl_handle_ni_t ni_handle, ptl_jid_t *jid);

    /*
     * P3.3 API spec: section 3.9
     */
    extern int lwfs_PtlMEAttach(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt_index,
	    ptl_process_id_t match_id, ptl_match_bits_t match_bits,
	    ptl_match_bits_t ignore_bits, ptl_unlink_t unlink,
	    ptl_ins_pos_t postion, ptl_handle_me_t *me_handle);

    extern int lwfs_PtlMEAttachAny(ptl_handle_ni_t ni_handle, ptl_pt_index_t *pt_index,
	    ptl_process_id_t match_id, ptl_match_bits_t match_bits,
	    ptl_match_bits_t ignore_bits, ptl_unlink_t unlink,
	    ptl_handle_me_t *me_handle);

    extern int lwfs_PtlMEInsert(ptl_handle_me_t base, ptl_process_id_t match_id,
	    ptl_match_bits_t match_bits, ptl_match_bits_t ignore_bits,
	    ptl_unlink_t unlink, ptl_ins_pos_t position,
	    ptl_handle_me_t *me_handle);

    extern int lwfs_PtlMEUnlink(ptl_handle_me_t me_handle);

    /*
     * P3.3 API spec: section 3.10
     */
    extern int lwfs_PtlMDAttach(ptl_handle_me_t me_handle, ptl_md_t md, ptl_unlink_t unlink_op,
	    ptl_handle_md_t *md_handle);

    extern int lwfs_PtlMDBind(ptl_handle_ni_t ni_handle, ptl_md_t md, ptl_unlink_t unlink_op,
	    ptl_handle_md_t *md_handle);

    extern int lwfs_PtlMDUnlink(ptl_handle_md_t md_handle);

    extern int lwfs_PtlMDUpdate(ptl_handle_md_t md_handle, ptl_md_t *old_md, ptl_md_t *new_md,
	    ptl_handle_eq_t eq_handle);

    /*
     * P3.3 API spec: section 3.11
     */
    extern int lwfs_PtlEQAlloc(
	    ptl_handle_ni_t ni_handle, 
	    ptl_size_t count,
	    void (* eq_handler)(ptl_event_t *event), 
	    ptl_handle_eq_t *eq_handle);

    extern int lwfs_PtlEQFree(ptl_handle_eq_t eq_handle);

    extern int lwfs_PtlEQGet(ptl_handle_eq_t eq_handle, ptl_event_t *event);

    extern int lwfs_PtlEQWait(ptl_handle_eq_t eq_handle, ptl_event_t *event);

    extern int lwfs_PtlEQWait_timeout(ptl_handle_eq_t eq_handle, ptl_event_t *event_out);

    extern int lwfs_PtlEQPoll(
	    ptl_handle_eq_t *eq_handles, 
	    int size, 
	    ptl_time_t timeout,
	    ptl_event_t *event, 
	    int *which_eq);

    /*
     * P3.3 API spec: section 3.12
     */
#if 0
    extern int lwfs_PtlACEntry(ptl_handle_ni_t ni_handle, 
	    ptl_ac_index_t ac_index, 
	    ptl_process_id_t match_id, 
	    ptl_uid_t user_id, 
	    ptl_jid_t job_id,
	    ptl_pt_index_t pt_index);
#endif
    /*
     * P3.3 API spec: section 3.13
     */
    extern int lwfs_PtlPut(ptl_handle_md_t md_handle, ptl_ack_req_t ack_req,
	    ptl_process_id_t target_id, ptl_pt_index_t pt_index,
	    ptl_ac_index_t ac_index, ptl_match_bits_t match_bits,
	    ptl_size_t remote_offset, ptl_hdr_data_t hdr_data);

    extern int lwfs_PtlPutRegion(ptl_handle_md_t md_handle, ptl_size_t local_offset,
	    ptl_size_t length, ptl_ack_req_t ack_req,
	    ptl_process_id_t target_id, ptl_pt_index_t pt_index,
	    ptl_ac_index_t ac_index, ptl_match_bits_t match_bits,
	    ptl_size_t remote_offset, ptl_hdr_data_t hdr_data);

    extern int lwfs_PtlGet(ptl_handle_md_t md_handle, ptl_process_id_t target_id,
	    ptl_pt_index_t pt_index, ptl_ac_index_t ac_index,
	    ptl_match_bits_t match_bits, ptl_size_t remote_offset);

    extern int lwfs_PtlGetRegion(ptl_handle_md_t md_handle, ptl_size_t local_offset,
	    ptl_size_t length, ptl_process_id_t target_id,
	    ptl_pt_index_t pt_index, ptl_ac_index_t ac_index,
	    ptl_match_bits_t match_bits, ptl_size_t remote_offset);

    extern int lwfs_PtlGetPut(ptl_handle_md_t get_md_handle, ptl_handle_md_t put_md_handle,
	    ptl_process_id_t target_id, ptl_pt_index_t pt_index,
	    ptl_ac_index_t ac_index, ptl_match_bits_t match_bits,
	    ptl_size_t remote_offset, ptl_hdr_data_t hdr_data);

#endif

#ifdef __cplusplus
}
#endif

#endif
