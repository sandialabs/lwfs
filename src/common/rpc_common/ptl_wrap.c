/*-------------------------------------------------------------------------*/
/**  @file ptl_wrap.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include PORTALS_HEADER
#include <pthread.h>

#include "common/types/types.h"
#include "common/rpc_common/rpc_common.h"
#include "common/rpc_common/rpc_debug.h"
#include "support/logger/logger.h"

#include "ptl_wrap.h"

//#undef LOCK_PORTALS_API

#ifdef LOCK_PORTALS_API
/* mutex initialization options are:
 *   - PTHREAD_MUTEX_INITIALIZER
 *   - PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
 *   - PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
 */
static pthread_mutex_t ptl_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; 
#endif

#ifndef HAVE_PTHREAD
#define pthread_self() 0
#define lwfs_thread_pool_getrank() 0
#endif


static int ptl_lock() {
    int rc = LWFS_OK; 
#ifdef LOCK_PORTALS_API
    rc =  pthread_mutex_lock(&ptl_mutex);
#endif 
    return rc;
}

static int ptl_unlock() {
    int rc = LWFS_OK;
#ifdef LOCK_PORTALS_API
    rc =  pthread_mutex_unlock(&ptl_mutex);
#endif 
    return rc;
}

int lwfs_PtlInit(int *max_interfaces)
{
    int result; 

    ptl_lock();
    result = PtlInit(max_interfaces); 
    ptl_unlock();

    return result; 
}

void lwfs_PtlFini(void) 
{
    //ptl_lock();
    PtlFini();
    //ptl_unlock();
}

/*
 * P3.3 API spec: section 3.5
 */
int lwfs_PtlNIInit(ptl_interface_t interface, ptl_pid_t pid,
	      ptl_ni_limits_t *desired, ptl_ni_limits_t *actual,
	      ptl_handle_ni_t *ni_handle)
{
    int result; 

    ptl_lock();
    result = PtlNIInit(interface, pid, desired, actual, ni_handle);
    ptl_unlock();

    return result; 
}

int lwfs_PtlNIFini(ptl_handle_ni_t ni_handle)
{
    int result; 

    ptl_lock();
    result = PtlNIFini(ni_handle);
    ptl_unlock();

    return result; 
}

int lwfs_PtlNIStatus(ptl_handle_ni_t ni_handle, ptl_sr_index_t register_index,
		ptl_sr_value_t *status)
{
    int result; 

    ptl_lock();
    result = PtlNIStatus(ni_handle, register_index, status);
    ptl_unlock();

    return result;
}

int lwfs_PtlNIDist(ptl_handle_ni_t ni_handle, ptl_process_id_t process,
	      unsigned long *distance)
{
    int result; 

    ptl_lock();
    result = PtlNIDist(ni_handle, process, distance);
    ptl_unlock();

    return result;
}

int lwfs_PtlNIHandle(ptl_handle_any_t handle, ptl_handle_ni_t *ni_handle)
{
    int result; 

    ptl_lock();
    result = PtlNIHandle(handle, ni_handle);
    ptl_unlock();

    return result;
}

/*
 * P3.3 API spec: section 3.6
 */
int lwfs_PtlGetUid(ptl_handle_ni_t ni_handle, ptl_uid_t *uid)
{
    int result; 

    ptl_lock();
    result = PtlGetUid(ni_handle, uid);
    ptl_unlock();

    return result;
}

/*
 * P3.3 API spec: section 3.7
 */
int lwfs_PtlGetId(ptl_handle_ni_t ni_handle, ptl_process_id_t *id)
{
    int result; 

    ptl_lock();
    result = PtlGetId(ni_handle, id); 
    ptl_unlock();

    return result;
}

int PtlGetJid();

/*
 * P3.3 API spec: section 3.8
 */
int lwfs_PtlGetJid(ptl_handle_ni_t ni_handle, ptl_jid_t *jid)
{
    int result; 

#ifdef HAVE_PTLGETJID
    ptl_lock();
    result = PtlGetJid(ni_handle, jid); 
    ptl_unlock();
#else
    fprintf(stderr, "ERROR: PtlGetJid not implemented\n");
    result = -1;
#endif

    return result;
}

/*
 * P3.3 API spec: section 3.9
 */
int lwfs_PtlMEAttach(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt_index,
		ptl_process_id_t match_id, ptl_match_bits_t match_bits,
		ptl_match_bits_t ignore_bits, ptl_unlink_t unlink,
		ptl_ins_pos_t postion, ptl_handle_me_t *me_handle)
{
    int result; 

    ptl_lock();
    result = PtlMEAttach(ni_handle, pt_index, match_id, 
            match_bits, ignore_bits, unlink, postion, me_handle); 
    ptl_unlock();

    return result;
}

int lwfs_PtlMEAttachAny(ptl_handle_ni_t ni_handle, ptl_pt_index_t *pt_index,
		   ptl_process_id_t match_id, ptl_match_bits_t match_bits,
		   ptl_match_bits_t ignore_bits, ptl_unlink_t unlink,
		   ptl_handle_me_t *me_handle)
{
    int result; 

    ptl_lock();
    result = PtlMEAttachAny(ni_handle, pt_index,
		   match_id, match_bits, ignore_bits, unlink,
		   me_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlMEInsert(ptl_handle_me_t base, ptl_process_id_t match_id,
		ptl_match_bits_t match_bits, ptl_match_bits_t ignore_bits,
		ptl_unlink_t unlink, ptl_ins_pos_t position,
		ptl_handle_me_t *me_handle)
{
    int result; 

    ptl_lock();
    result = PtlMEInsert(base, match_id, match_bits, ignore_bits,
            unlink, position, me_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlMEUnlink(ptl_handle_me_t me_handle)
{
    int result; 

    ptl_lock();
    result = PtlMEUnlink(me_handle);
    ptl_unlock();

    return result;
}

/*
 * P3.3 API spec: section 3.10
 */
int lwfs_PtlMDAttach(ptl_handle_me_t me_handle, ptl_md_t md, ptl_unlink_t unlink_op,
		ptl_handle_md_t *md_handle)
{
    int result; 

    ptl_lock();

    /* Cray required addition to the md options */
    if (unlink_op == PTL_UNLINK) {
	md.options |= PTL_MD_EVENT_AUTO_UNLINK_ENABLE;
    }
    else {
	md.options |= PTL_MD_EVENT_MANUAL_UNLINK_ENABLE;
    }

    result = PtlMDAttach(me_handle, md, unlink_op, md_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlMDBind(ptl_handle_ni_t ni_handle, ptl_md_t md, ptl_unlink_t unlink_op,
	      ptl_handle_md_t *md_handle)
{
    int result; 

    ptl_lock();

    /* Cray required addition to the md options */
    if (unlink_op == PTL_UNLINK) {
	md.options |= PTL_MD_EVENT_AUTO_UNLINK_ENABLE;
    }
    else {
	md.options |= PTL_MD_EVENT_MANUAL_UNLINK_ENABLE;
    }

    result = PtlMDBind(ni_handle, md, unlink_op, md_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlMDUnlink(ptl_handle_md_t md_handle)
{
    int result; 

    ptl_lock();
    result = PtlMDUnlink(md_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlMDUpdate(ptl_handle_md_t md_handle, ptl_md_t *old_md, ptl_md_t *new_md,
		ptl_handle_eq_t eq_handle)
{
    int result; 

    ptl_lock();
    result = PtlMDUpdate(md_handle, old_md, new_md, eq_handle);
    ptl_unlock();

    return result;
}

/*
 * P3.3 API spec: section 3.11
 */
int lwfs_PtlEQAlloc(ptl_handle_ni_t ni_handle, ptl_size_t count,
	       ptl_eq_handler_t eq_handler, ptl_handle_eq_t *eq_handle)
{
    int result; 
    lwfs_remote_pid myid;  

    ptl_lock();
    result = PtlEQAlloc(ni_handle, count, eq_handler, eq_handle);
    lwfs_get_id(&myid); 
    log_debug(rpc_debug_level, "allocating EQ (nid==%u; hndl==%u)", myid.nid, *eq_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlEQFree(ptl_handle_eq_t eq_handle)
{
    int result; 
    lwfs_remote_pid myid;  

    ptl_lock();
    lwfs_get_id(&myid); 
    log_debug(rpc_debug_level, "freeing EQ (nid==%u; hndl==%u)", myid.nid, eq_handle); 
    result = PtlEQFree(eq_handle);
    ptl_unlock();

    return result;
}

int lwfs_PtlEQGet(ptl_handle_eq_t eq_handle, ptl_event_t *event)
{
    int result; 

    ptl_lock();
    result = PtlEQGet(eq_handle, event);
    ptl_unlock();

    return result;
}

int lwfs_PtlEQWait(ptl_handle_eq_t eq_handle, ptl_event_t *event)
{
    int result; 

    ptl_lock();
    result = PtlEQWait(eq_handle, event);
    ptl_unlock();

    return result;
}

int lwfs_PtlEQWait_timeout(ptl_handle_eq_t eq_handle, ptl_event_t *event_out)
{
    int result = 0; 

    ptl_lock();
    //result = PtlEQWait_timeout(eq_handle, event_out);
    ptl_unlock();

    return result;
}

int lwfs_PtlEQPoll(ptl_handle_eq_t *eq_handles, int size, ptl_time_t timeout,
	      ptl_event_t *event, int *which_eq)
{
    int result; 

    ptl_lock();
//    log_debug(rpc_debug_level, "thread_id(%d): polling EQ (eq_handle[0]==%d)", lwfs_thread_pool_getrank(), eq_handles[0]);
    result = PtlEQPoll(eq_handles, size, timeout, event, which_eq);
    ptl_unlock();

    return result;
}

/*
 * P3.3 API spec: section 3.12
 */
#if 0
int lwfs_PtlACEntry(
	ptl_handle_ni_t ni_handle, 
	ptl_ac_index_t ac_index, 
	ptl_process_id_t match_id, 
	ptl_uid_t user_id, 
	ptl_jid_t job_id, 
	ptl_pt_index_t pt_index)
{
    int result; 

    ptl_lock();
    result = PtlACEntry(ni_handle, 
	    ac_index, match_id, user_id, 
	    job_id, pt_index);
    ptl_unlock();

    return result;
}
#endif

/*
 * P3.3 API spec: section 3.13
 */
int lwfs_PtlPut(ptl_handle_md_t md_handle, ptl_ack_req_t ack_req,
	   ptl_process_id_t target_id, ptl_pt_index_t pt_index,
	   ptl_ac_index_t ac_index, ptl_match_bits_t match_bits,
	   ptl_size_t remote_offset, ptl_hdr_data_t hdr_data)
{
    int result; 

    ptl_lock();
    result = PtlPut(md_handle, ack_req, target_id, pt_index,
            ac_index, match_bits, remote_offset, hdr_data);
    ptl_unlock();

    return result;
}

int lwfs_PtlPutRegion(ptl_handle_md_t md_handle, ptl_size_t local_offset,
		 ptl_size_t length, ptl_ack_req_t ack_req,
		 ptl_process_id_t target_id, ptl_pt_index_t pt_index,
		 ptl_ac_index_t ac_index, ptl_match_bits_t match_bits,
		 ptl_size_t remote_offset, ptl_hdr_data_t hdr_data)
{
    int result; 

    ptl_lock();
    result = PtlPutRegion(md_handle, local_offset, length, ack_req,
            target_id, pt_index, ac_index, match_bits,
            remote_offset, hdr_data);
    ptl_unlock();

    return result;
}

int lwfs_PtlGet(ptl_handle_md_t md_handle, ptl_process_id_t target_id,
	   ptl_pt_index_t pt_index, ptl_ac_index_t ac_index,
	   ptl_match_bits_t match_bits, ptl_size_t remote_offset)
{
    int result; 

    ptl_lock();
    result = PtlGet(md_handle, target_id, pt_index, ac_index,
            match_bits, remote_offset);
    ptl_unlock();

    return result;
}

int lwfs_PtlGetRegion(ptl_handle_md_t md_handle, ptl_size_t local_offset,
		 ptl_size_t length, ptl_process_id_t target_id,
		 ptl_pt_index_t pt_index, ptl_ac_index_t ac_index,
		 ptl_match_bits_t match_bits, ptl_size_t remote_offset)
{
    int result; 

    ptl_lock();
    result = PtlGetRegion(md_handle, local_offset, length, target_id,
            pt_index, ac_index, match_bits, remote_offset);
    ptl_unlock();

    return result;
}

int lwfs_PtlGetPut(ptl_handle_md_t get_md_handle, ptl_handle_md_t put_md_handle,
	      ptl_process_id_t target_id, ptl_pt_index_t pt_index,
	      ptl_ac_index_t ac_index, ptl_match_bits_t match_bits,
	      ptl_size_t remote_offset, ptl_hdr_data_t hdr_data)
{
    int result; 

    ptl_lock();
    result = PtlGetPut(get_md_handle, put_md_handle, target_id, pt_index,
            ac_index, match_bits, remote_offset, hdr_data);
    ptl_unlock();

    return result;
}

