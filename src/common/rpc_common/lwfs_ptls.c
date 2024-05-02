/**  @file lwfs_ptls.c
 *   
 *   @brief Portals implementation of the LWFS RPC. 
 *
 *   This file includes methods used by both clients
 *   and servers. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1536 $
 *   $Date: 2007-09-11 17:27:21 -0600 (Tue, 11 Sep 2007) $
 *
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include "config.h"

#include PORTALS_HEADER
#include PORTALS_NAL_HEADER

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "support/logger/logger.h"
#include "support/signal/lwfs_signal.h"

#include "lwfs_ptls.h"
#include "rpc_debug.h"
#include "ptl_wrap.h"

#ifndef HAVE_PTHREAD
#define pthread_self() 0
#define lwfs_thread_pool_getrank() 0
#endif


static const int MIN_TIMEOUT = 100;  /* in milliseconds */

/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 



/* locally global portals variables */
static ptl_handle_ni_t ni_h;        /* handle for the network interface */

/* to synchronize access to portals calls */
/*
 *  On BSD (Darwin/MacOS), pthread_mutex_t is not recursive by default, and so the static
 *  initializer here doesn't work.  Instead, we have to play a little compiler trick, 
 *  getting gcc to generate a file-scope constructor for it.  I'm going to
 *  leave the bail-out #error if we're not being compiled with GCC, since porting
 *  to another compiler should make for a lot of work in other places, not just here.
 */
#if (defined(__APPLE__) && defined(__GNUC__))

static pthread_mutex_t portals_mutex;

static void __attribute__ ((constructor)) thunk(void)
{
#if 0
  pthread_mutexattr_t attr;
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
  pthread_mutex_init( &portals_mutex, &attr );
#endif
}

#else
static pthread_mutex_t portals_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif          


int lwfs_ptl_get_ni(ptl_handle_ni_t *ni_handle) {
    *ni_handle = ni_h; 
    return LWFS_OK;
}


/**
 * @brief initialize portals.
 */
int lwfs_ptl_init(ptl_interface_t iface, lwfs_pid my_pid)
{
    int rc; /* return code */
    static lwfs_bool initialized=FALSE; 
    int max_interfaces; 
    ptl_ni_limits_t actual; 

    if (!initialized) {

	log_debug(rpc_debug_level, "initializing portals");

	/* The UTCP NAL requires that the application defines where the Portals
	 * API and library should send any output. We'll send the output to 
	 * the same file as logger. 
	 */
	utcp_api_out = logger_get_file(); 
	utcp_lib_out = logger_get_file(); 

	/* initialize the portals library */
	log_debug(rpc_debug_level, "initializing portals library.");
	rc = lwfs_PtlInit(&max_interfaces); 
	if (rc) {
	    log_fatal(rpc_debug_level,"PtlInit() failed, %s", ptl_err_str[rc]);
	    abort();
	}

	/*
	   ptl_ni_limits_t desired, actual; 

	   desired.max_match_entries   = LWFS_PTL_MAX_MES;
	   desired.max_mem_descriptors = LWFS_PTL_MAX_MDS;
	   desired.max_event_queues    = LWFS_PTL_MAX_EQS;
	   desired.max_atable_index    = LWFS_PTL_MAX_ACI;
	   desired.max_ptable_index    = LWFS_PTL_MAX_PTI;
	 */

	/* initialize the portals interface */
	log_debug(rpc_debug_level, "initializing portals network interface"
		" pid=%d", (int)my_pid);

	rc = lwfs_PtlNIInit(iface, my_pid, NULL, &actual, &ni_h);
	if ((rc != PTL_OK) && (rc != PTL_IFACE_DUP)) {
	    log_fatal(rpc_debug_level, "lwfs_PtlNIInit() failed, %s", ptl_err_str[rc]);
	    abort();
	}

	lwfs_remote_pid myid; 
	lwfs_ptl_get_id(&myid); 

	if (logging_info(rpc_debug_level)) {
	    fprintf(logger_get_file(), "Portals Initialized: nid=%llu, pid=%llu\n",
		    (unsigned long long)myid.nid, 
		    (unsigned long long)myid.pid);
	}
    }

    initialized = TRUE; 

    return 0; 
}

int lwfs_ptl_fini() {
    log_debug(rpc_debug_level, "shutting down PTL Network interface");
    lwfs_PtlNIFini(ni_h);
    log_debug(rpc_debug_level, "shutting down PTL library");
    lwfs_PtlFini();
    return LWFS_OK;
}	



/**
 * @brief Send a buffer to a destination memory descriptor.
 *
 */
int lwfs_ptl_send(
        void *src_buf, 
        int len, 
        lwfs_rma *dest_addr) 
{
    int rc; 
    rc = lwfs_ptl_put(src_buf, len, dest_addr);
    return rc; 
}


int lwfs_ptl_eq_wait(
        ptl_handle_eq_t eq_handle, 
        ptl_event_t *event) 
{
	int which; 
	return lwfs_ptl_eq_poll(&eq_handle, 1, -1, event, &which); 
}


int lwfs_ptl_eq_poll(
		ptl_handle_eq_t *eq_handle, 
		int size, 
		int timeout, 
		ptl_event_t *event,
		int *which)
{
	int rc = PTL_EQ_EMPTY; 
	int elapsed_time = 0; 
	int timeout_per_call;
	int i;
	
	if (timeout < 0) 
		timeout_per_call = MIN_TIMEOUT;
	else
		timeout_per_call = (timeout < MIN_TIMEOUT)? timeout: MIN_TIMEOUT;

	while (!lwfs_exit_now())   {
//		log_debug(rpc_debug_level, "waiting for events on any of: ");
//		for (i=0;i<size;i++) {
//			log_debug(rpc_debug_level, "                              eq_h==%u", eq_handle[i]);
//		}
		lwfs_ptl_lock();
		//rc = lwfs_PtlEQGet(eq_handle, event); 
		rc = lwfs_PtlEQPoll(eq_handle, size, timeout_per_call, event, which); 
		lwfs_ptl_unlock();
//		log_debug(rpc_debug_level, "thread_id(%d): polling status is %s", lwfs_thread_pool_getrank(), ptl_err_str[rc]);

		/* case 1: success */
		if (rc == PTL_OK) {
			rc = LWFS_OK;
			break;
		}

		/* case 2: success, but some events were dropped */
		else if (rc == PTL_EQ_DROPPED) {
			log_warn(rpc_debug_level, "lwfs_PtlEQPoll dropped some events");
	        	log_warn(rpc_debug_level, "lwfs_PtlEQPoll succeeded, but at least one event was dropped");
			rc = LWFS_OK;
			break;
		}

		/* case 3: timed out */
		else if (rc == PTL_EQ_EMPTY) {
			elapsed_time += timeout_per_call; 

			/* if the caller asked for a legitimate timeout, we need to exit */
			if (((timeout > 0) && (elapsed_time >= timeout)) || lwfs_exit_now()) {
				log_warn(rpc_debug_level, "lwfs_PtlEQPoll timed out: %s",
						ptl_err_str[rc]);
				rc = LWFS_ERR_TIMEDOUT;
				break;
			}

			/* continue if the timeout has not expired */
			/* log_debug(rpc_debug_level, "timedout... continuing"); */
		}

		/* case 4: failure */
		else {
			log_error(rpc_debug_level, "thread_id(%d): lwfs_PtlEQPoll failed: %s",
					lwfs_thread_pool_getrank(), ptl_err_str[rc]);
			rc = LWFS_ERR_RPC;
			break;
		}
	}

//        log_debug(rpc_debug_level, "Poll Event= {");
//        log_debug(rpc_debug_level, "\ttype = %d", event->type);
//        log_debug(rpc_debug_level, "\tinitiator = (%llu, %llu)", 
//                        (unsigned long long)event->initiator.nid, 
//                        (unsigned long long)event->initiator.pid);
//        log_debug(rpc_debug_level, "\tuid = %d", event->uid);
//        log_debug(rpc_debug_level, "\tjid = %d", event->jid);
//        log_debug(rpc_debug_level, "\tpt_index = %d", event->pt_index);
//        log_debug(rpc_debug_level, "\trlength = %llu", (unsigned long long)event->rlength);
//        log_debug(rpc_debug_level, "\tmlength = %llu", (unsigned long long)event->mlength);
//        log_debug(rpc_debug_level, "\toffset = %llu", (unsigned long long)event->offset);
//        log_debug(rpc_debug_level, "\tmd_handle = %d", event->md_handle);
//        log_debug(rpc_debug_level, "\tmd.threshold = %d", event->md.threshold);

    if (event->ni_fail_type != PTL_NI_OK) {
        ptl_handle_ni_t ni_h; 
        lwfs_ptl_get_ni(&ni_h); 
        log_error(rpc_debug_level, "NI reported error: ni_fail_type=%s",
                PtlNIFailStr(ni_h, event->ni_fail_type)); 
        rc = LWFS_ERR_RPC; 
    }

	return rc; 
}


int lwfs_ptl_eq_timedwait(
        ptl_handle_eq_t eq_handle, 
		int timeout, 
        ptl_event_t *event) 
{
	int which; 
	return lwfs_ptl_eq_poll(&eq_handle, 1, timeout, event, &which); 
}


int lwfs_ptl_eq_alloc(
	ptl_handle_ni_t ni_handle, 
	ptl_size_t count,
	void (* eq_handler)(ptl_event_t *event), 
	ptl_handle_eq_t *eq_handle) 
{
    int rc; /* return code */

    lwfs_ptl_lock();
    log_debug(rpc_debug_level, "enter lwfs_ptl_eq_alloc()");
    rc = lwfs_PtlEQAlloc(ni_handle, count, eq_handler, eq_handle);
    lwfs_ptl_unlock();
    if (rc != PTL_OK) {
        fprintf(stderr, "lwfs_PtlEQAlloc() failed: %s (%d)\n",
                PtlErrorStr(rc), rc); 
        rc = LWFS_ERR_RPC;
    }

    return (rc);
}


/** 
 * @brief Put a buffer into the destination memory descriptor. 
 * 
 * @param src_buf a pointer to the buffer.
 * @param len size of the buffer in bytes.
 * @param dest_addr the remote memory address.
 * @return PTL_OK if success, 
 */
int lwfs_ptl_put(
        const void *src_buf, 
        const int len, 
        const lwfs_rma *dest_addr) 
{
    int rc = LWFS_OK; 
    int rc2; 

    int num_eq_slots = 5; 

    ptl_handle_eq_t eq_h;         /* event queue  */
    ptl_event_t event;            /* event */
    ptl_md_t md;                  /* memory descriptor */
    ptl_handle_md_t md_h;         /* memory descriptor handle */
    ptl_process_id_t snd_target;  /* process id of the target */

    lwfs_bool got_send_start = FALSE;
    lwfs_bool got_send_end = FALSE;
    lwfs_bool got_ack = FALSE; 
    lwfs_bool got_unlink = FALSE; 
    lwfs_bool done = FALSE;
    
    int iter, max_iters;
    
    /* initialize the send target */
    snd_target.nid = dest_addr->match_id.nid; 
    snd_target.pid = dest_addr->match_id.pid; 

    lwfs_ptl_lock();

    log_debug(rpc_debug_level, "enter lwfs_ptl_put");

    /* allocate an event queue for the send */
    rc = lwfs_PtlEQAlloc(ni_h, num_eq_slots, PTL_EQ_HANDLER_NONE, &eq_h); 
    if (rc) {
        log_fatal(rpc_debug_level,"lwfs_PtlEQAlloc() failed, %s", 
                ptl_err_str[rc]);
        rc = LWFS_ERR_RPC;
        goto unlock;
    }
    
    log_debug(rpc_debug_level, "thread_id(%d): eq_h == %d", lwfs_thread_pool_getrank(), eq_h);

    /* find out if we are trying to send something larger than the remote buffer */
    if (len > dest_addr->len) {
        log_error(rpc_debug_level, 
                "source buffer (size %d) bigger than dest buffer (size %d)", len, dest_addr->len);
        rc = LWFS_ERR_RPC; 
        goto cleanup;
    }

    /* initialize the memory descriptor */
    memset(&md, 0, sizeof(ptl_md_t));
    md.start = (void *)src_buf; 
    md.length = len; 
    md.threshold = 2;   // expect put + ack
    md.options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
    md.eq_handle = eq_h; 
    md.user_ptr = NULL;  /* unused */

    /* bind the md */
    rc = lwfs_PtlMDBind(ni_h, md, PTL_UNLINK, &md_h); 
    if (rc) {
        log_fatal(rpc_debug_level,"lwfs_PtlMDBind() failed, %s", 
                ptl_err_str[rc]);
        rc = LWFS_ERR_RPC;
        goto cleanup;
    }


    /* send the buffer to the remote address */
    log_debug(rpc_debug_level,"sending buffer (len=%d) to (nid=%llu, pid=%llu)", 
            len,
            (unsigned long long)snd_target.nid,
            (unsigned long long)snd_target.pid);
    if (logging_debug(rpc_debug_level)) {
        fprint_lwfs_rma(logger_get_file(), "dest_addr", 
                "DEBUG ", dest_addr);
    }

    rc = lwfs_PtlPut(md_h, PTL_ACK_REQ, snd_target, dest_addr->buffer_id, 0, 
            dest_addr->match_bits, dest_addr->offset, 0);
    if (rc) {
        log_fatal(rpc_debug_level,"lwfs_PtlPut() failed, %s", ptl_err_str[rc]);
        rc = LWFS_ERR_RPC;
        goto cleanup;
    }

    done = FALSE;
    iter = 0;
    max_iters = 3;
    log_debug(rpc_debug_level, "waiting for send to finish");
    while (!done) {
    	memset(&event, 0, sizeof(ptl_event_t));
//        rc = lwfs_ptl_eq_wait(eq_h, &event);
	int which=0;
	int timeout=MIN_TIMEOUT;
        rc = lwfs_ptl_eq_poll(&eq_h, 1, timeout, &event, &which);
        if (which != 0) {
        	log_error(rpc_debug_level, "funny lwfs_ptl_eq_poll result for which (%d)...should always be 0", which);
        }
        if (rc == LWFS_ERR_TIMEDOUT) {
        	log_debug(rpc_debug_level, "lwfs_ptl_eq_poll timed out with empty EQ");
        	if (iter >= max_iters) {
	        	log_debug(rpc_debug_level, "retrying after timeout");
        		continue;
        	}
        	log_error(rpc_debug_level, "lwfs_ptl_eq_poll failed after %d attempts timed out", max_iters);
        	goto failed;
        }
        if (rc != LWFS_OK) {
//            log_debug(rpc_debug_level, "lwfs_ptl_eq_wait failed. rc = %d", rc);
            log_error(rpc_debug_level, "lwfs_ptl_eq_poll failed. rc = %d", rc);
            rc = LWFS_ERR_RPC;
            goto failed; 
        }

        log_debug(rpc_debug_level, "Put Event= {");
        log_debug(rpc_debug_level, "\ttype = %d", event.type);
        log_debug(rpc_debug_level, "\tinitiator = (%llu, %llu)", 
                        (unsigned long long)event.initiator.nid, 
                        (unsigned long long)event.initiator.pid);
        log_debug(rpc_debug_level, "\tuid = %d", event.uid);
        log_debug(rpc_debug_level, "\tjid = %d", event.jid);
        log_debug(rpc_debug_level, "\tpt_index = %d", event.pt_index);
        log_debug(rpc_debug_level, "\trlength = %llu", (unsigned long long)event.rlength);
        log_debug(rpc_debug_level, "\tmlength = %llu", (unsigned long long)event.mlength);
        log_debug(rpc_debug_level, "\toffset = %llu", (unsigned long long)event.offset);
        log_debug(rpc_debug_level, "\tmd_handle = %d", event.md_handle);
        log_debug(rpc_debug_level, "\tmd.threshold = %d", event.md.threshold);
	
        switch (event.type) {
            case PTL_EVENT_SEND_START:
                log_debug(rpc_debug_level, "thread_id(%d): got send_start", lwfs_thread_pool_getrank());
                got_send_start = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on send start: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            case PTL_EVENT_SEND_END:
                log_debug(rpc_debug_level, "thread_id(%d): got send_end", lwfs_thread_pool_getrank());
                got_send_end = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on send end: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            case PTL_EVENT_ACK:
                log_debug(rpc_debug_level, "thread_id(%d): got ack", lwfs_thread_pool_getrank());
                got_ack = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on ack: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            case PTL_EVENT_UNLINK:
                log_debug(rpc_debug_level, "thread_id(%d): got unlink", lwfs_thread_pool_getrank());
                got_unlink = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on unlink: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            default:
                log_error(rpc_debug_level, "unexpected event=%d", event.type);
	        log_debug(rpc_debug_level, "Unexpected Event= {");
	        log_debug(rpc_debug_level, "\ttype = %d", event.type);
	        log_debug(rpc_debug_level, "\tinitiator = (%llu, %llu)", 
	                        (unsigned long long)event.initiator.nid, 
	                        (unsigned long long)event.initiator.pid);
	        log_debug(rpc_debug_level, "\tuid = %d", event.uid);
	        log_debug(rpc_debug_level, "\tjid = %d", event.jid);
	        log_debug(rpc_debug_level, "\tpt_index = %d", event.pt_index);
	        log_debug(rpc_debug_level, "\trlength = %llu", (unsigned long long)event.rlength);
	        log_debug(rpc_debug_level, "\tmlength = %llu", (unsigned long long)event.mlength);
	        log_debug(rpc_debug_level, "\toffset = %llu", (unsigned long long)event.offset);
	        log_debug(rpc_debug_level, "\tmd_handle = %d", event.md_handle);

                rc = LWFS_ERR_RPC; 
                goto cleanup; 
        }

        if (got_send_start && got_send_end && got_ack && got_unlink) { 
            done = TRUE;
        }

	log_debug(rpc_debug_level, "thread_id(%d): s:%d,e:%d,a:%d,u:%d,d:%d", 
		lwfs_thread_pool_getrank(), got_send_start, got_send_end, got_ack, got_unlink, done);
    }

    log_debug(rpc_debug_level, "message sent");
    
    goto cleanup;

failed:
cleanup:
    /* unlink the memory descriptor */
    /*
    rc2 = lwfs_PtlMDUnlink(md_h); 
    if (rc2 != PTL_OK) {
        log_error(rpc_debug_level,"failed to unlink md (%s)",
        ptl_err_str[rc]);
        return LWFS_ERR_RPC;
    }
    */

    /* free the event queue */
    rc2 = lwfs_PtlEQFree(eq_h); 
    if (rc2 != PTL_OK) {
        log_error(rpc_debug_level,"thread_id(%d); failed to free event queue (%s)",
                lwfs_thread_pool_getrank(), ptl_err_str[rc2]);
        rc = LWFS_ERR_RPC;
    }

unlock:
    lwfs_ptl_unlock();

    return rc; 
}

/** 
 * @brief Get a buffer from the src memory descriptor. 
 * 
 * @param dest_buf a pointer to the buffer.
 * @param len size of the buffer in bytes.
 * @param src_addr the source remote memory descriptor.
 */
int lwfs_ptl_get(
        void *rcv_buf, 
        const int len, 
        const lwfs_rma *src_addr) 
{
    int rc = LWFS_OK;  /* return code */

    ptl_process_id_t src_target;  /* process id of the source */

    ptl_md_t md;              /* the memory descriptor */
    ptl_handle_md_t md_h;     /* handle to the memory descriptor */
    ptl_handle_md_t eq_h;     /* handle to the event queue */
    ptl_event_t event; 

    lwfs_bool got_send_start = FALSE;
    lwfs_bool got_send_end = FALSE;
    lwfs_bool got_reply_start = FALSE;
    lwfs_bool got_reply_end = FALSE;
    lwfs_bool got_unlink = FALSE; 
    lwfs_bool done = FALSE;

    int num_eq_slots = 10;  /* no more than 10 entries in the event queue */

    int iter, max_iters;

    lwfs_ptl_lock();
    
    int old_debug_level = rpc_debug_level;
//    rpc_debug_level=rpc_debug_level;

    log_debug(rpc_debug_level, "%d: enter lwfs_ptl_get", pthread_self());

    /* create the event queue */
    log_debug(rpc_debug_level,"%d: allocating event queue for \"get\"", pthread_self());
    rc = lwfs_PtlEQAlloc(ni_h, num_eq_slots, NULL, &eq_h); 
    if (rc) {
        log_fatal(rpc_debug_level,"lwfs_PtlEQAlloc() failed, %s", 
                ptl_err_str[rc]);
        rc = LWFS_ERR_RPC;
        goto unlock;
    }

    /* create the memory descriptor */
    md.start     = rcv_buf;   /* where to put the data */
    md.length    = len;       /* max size of the data */
    md.threshold = 1;         /* expect a reply_{start,end} pair */
#if defined(HAVE_CRAY_PORTALS)
    md.threshold += 1;        /* also expect a send_{start,end} pair */
#endif
    md.options   = 0;         /* unimportant */
    md.eq_handle = eq_h;      /* attach the event Q */

    /* bind the memory descriptor */
    log_debug(rpc_debug_level,"binding memory descriptor for \"get\"");
    rc = lwfs_PtlMDBind(ni_h, md, PTL_UNLINK, &md_h); 
    if (rc) {
        log_fatal(rpc_debug_level,"lwfs_PtlMDBind() failed, %s",rc);
        rc = LWFS_ERR_RPC;
        goto cleanup;
    }

    /* set the target */
    src_target.nid = src_addr->match_id.nid; 
    src_target.pid = src_addr->match_id.pid; 

    log_debug(rpc_debug_level,"getting buffer from "
            "(nid=%llu, pid=%llu), index=%d, match-bits=%d", 
            (unsigned long long) src_target.nid,
            (unsigned long long) src_target.pid,
            src_addr->buffer_id,
            src_addr->match_bits); 


    /* get the data */
    rc = lwfs_PtlGet(md_h, src_target, src_addr->buffer_id, 0, 
            src_addr->match_bits, src_addr->offset);
    if (rc) {
        log_fatal(rpc_debug_level,"lwfs_PtlGet() failed, %s", ptl_err_str[rc]);
        rc = LWFS_ERR_RPC;
        goto cleanup; 
    }

    /* wait for events that signal completion */
    iter=0;
    max_iters=3;
    done = FALSE;
    while (!done) {

/*
        rc = lwfs_ptl_eq_wait(eq_h, &event);
        if (rc != LWFS_OK) {
            log_error(rpc_debug_level,"failed waiting for event");
            rc = LWFS_ERR_RPC;
            goto cleanup; 
        }
*/

	int which=0;
	int timeout=MIN_TIMEOUT;
        rc = lwfs_ptl_eq_poll(&eq_h, 1, timeout, &event, &which);
        if (which != 0) {
        	log_error(rpc_debug_level, "funny lwfs_ptl_eq_poll result for which (%d)...should always be 0", which);
        }
        if (rc == LWFS_ERR_TIMEDOUT) {
        	log_debug(rpc_debug_level, "lwfs_ptl_eq_poll timed out with empty EQ");
        	if (iter >= max_iters) {
	        	log_debug(rpc_debug_level, "retrying after timeout");
        		continue;
        	}
        	log_error(rpc_debug_level, "lwfs_ptl_eq_poll failed after %d iterations", max_iters);
        	goto failed;
        }
        if (rc != LWFS_OK) {
//            log_debug(rpc_debug_level, "lwfs_ptl_eq_wait failed. rc = %d", rc);
            log_debug(rpc_debug_level, "lwfs_ptl_eq_poll failed. rc = %d", rc);
            rc = LWFS_ERR_RPC;
            goto failed; 
        }

        log_debug(rpc_debug_level, "Get Event= {");
        log_debug(rpc_debug_level, "\ttype = %d", event.type);
        log_debug(rpc_debug_level, "\tinitiator = (%llu, %llu)", 
                        (unsigned long long)event.initiator.nid, 
                        (unsigned long long)event.initiator.pid);
        log_debug(rpc_debug_level, "\tuid = %d", event.uid);
        log_debug(rpc_debug_level, "\tjid = %d", event.jid);
        log_debug(rpc_debug_level, "\tpt_index = %d", event.pt_index);
        log_debug(rpc_debug_level, "\trlength = %llu", (unsigned long long)event.rlength);
        log_debug(rpc_debug_level, "\tmlength = %llu", (unsigned long long)event.mlength);
        log_debug(rpc_debug_level, "\toffset = %llu", (unsigned long long)event.offset);
        log_debug(rpc_debug_level, "\tmd_handle = %d", event.md_handle);
        log_debug(rpc_debug_level, "\tmd.threshold = %d", event.md.threshold);
	
        switch (event.type) {

            /* 
             * the send_{start,end} events only occur on Cray XT3,
             * but they are safe to put here because they should 
             * never occur on Schutt Portals. 
             */
            case PTL_EVENT_SEND_START:
                log_debug(rpc_debug_level, "got send_start");
                got_send_start = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on send start: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            case PTL_EVENT_SEND_END:
                log_debug(rpc_debug_level, "got send_end");
                got_send_end = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on send end: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            case PTL_EVENT_REPLY_START:
                log_debug(rpc_debug_level,"got reply_start");
                got_reply_start = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on reply start: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break; 

            case PTL_EVENT_REPLY_END:
                log_debug(rpc_debug_level,"got reply_end");
                got_reply_end = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on reply end: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            case PTL_EVENT_UNLINK:
                log_debug(rpc_debug_level,"got unlink");
                got_unlink = TRUE;
                if (event.ni_fail_type != PTL_NI_OK) {
                    log_error(rpc_debug_level, "failed on unlink: ni_fail_type=%d",
                            event.ni_fail_type);
                    rc = LWFS_ERR_RPC; 
                    goto cleanup;
                }
                break;

            default:
                log_error(rpc_debug_level, "unexpected event=%d", event.type);
	        log_debug(rpc_debug_level, "Unexpected Event= {");
	        log_debug(rpc_debug_level, "\ttype = %d", event.type);
	        log_debug(rpc_debug_level, "\tinitiator = (%llu, %llu)", 
	                        (unsigned long long)event.initiator.nid, 
	                        (unsigned long long)event.initiator.pid);
	        log_debug(rpc_debug_level, "\tuid = %d", event.uid);
	        log_debug(rpc_debug_level, "\tjid = %d", event.jid);
	        log_debug(rpc_debug_level, "\tpt_index = %d", event.pt_index);
	        log_debug(rpc_debug_level, "\trlength = %llu", (unsigned long long)event.rlength);
	        log_debug(rpc_debug_level, "\tmlength = %llu", (unsigned long long)event.mlength);
	        log_debug(rpc_debug_level, "\toffset = %llu", (unsigned long long)event.offset);
	        log_debug(rpc_debug_level, "\tmd_handle = %d", event.md_handle);

                rc = LWFS_ERR_RPC; 
                goto cleanup;
        }

        if ((got_reply_start && got_reply_end && got_unlink) || /* Schutt portals */
            (got_send_start && got_send_end && got_reply_start && got_reply_end && got_unlink)) /* Cray XT3 portals */ { 
        
            done = TRUE;
        }
    }

    log_debug(rpc_debug_level,"success"); 

failed:
cleanup:   
    log_debug(rpc_debug_level,"freeing eq_h..."); 
    int rc2 = lwfs_PtlEQFree(eq_h); 
    if (rc2 != PTL_OK) {
        log_error(rpc_debug_level,"unable to free eq");
        rc = LWFS_ERR_RPC;
    }

unlock:
    lwfs_ptl_unlock();

    rpc_debug_level = old_debug_level;
    return rc;
}


/** 
 * @brief Returns the current process id.
 *
 * @param id  pointer to the id structure. 
 */
int lwfs_ptl_get_id(lwfs_remote_pid *id)
{
    int rc;  /* return code */
    ptl_process_id_t ptl_id; 

    rc = lwfs_PtlGetId(ni_h, &ptl_id); 
    if (rc != PTL_OK) {
        log_error(rpc_debug_level,"failed %s", ptl_err_str[rc]);
        return LWFS_ERR_RPC;
    }

    id->nid = ptl_id.nid; 
    id->pid = ptl_id.pid; 

    return rc; 
}



int lwfs_ptl_me_attach(
        ptl_handle_ni_t ni_handle, 
        ptl_pt_index_t pt_index,
	ptl_process_id_t match_id, 
	ptl_match_bits_t match_bits,
	ptl_match_bits_t ignore_bits, 
	ptl_unlink_t unlink,
	ptl_ins_pos_t postion, 
	ptl_handle_me_t *me_handle) 
{
        int rc; /* return code */

        lwfs_ptl_lock();

        log_debug(rpc_debug_level, "enter lwfs_ptl_me_attach");

        rc = lwfs_PtlMEAttach(ni_handle, pt_index, 
                match_id, match_bits, ignore_bits,
                unlink, postion, me_handle); 
        lwfs_ptl_unlock();
        if (rc != PTL_OK) {
            log_error(rpc_debug_level, "could not attach ME");
            rc = LWFS_ERR_RPC; 
        }

    return (rc);
}

int lwfs_ptl_md_attach(
		ptl_handle_me_t me_handle, 
		ptl_md_t md, 
		ptl_unlink_t unlink_op,
		ptl_handle_md_t *md_handle)
{
        int rc; /* return code */

        lwfs_ptl_lock();

        log_debug(rpc_debug_level, "enter lwfs_ptl_md_attach");

        rc = lwfs_PtlMDAttach(me_handle, md, unlink_op, md_handle); 
        lwfs_ptl_unlock();
        if (rc != PTL_OK) {
            log_error(rpc_debug_level, "could not alloc eq: %s",
                    ptl_err_str[rc]);
            rc = LWFS_ERR_RPC; 
        }
        
        return (rc);
}

int lwfs_ptl_md_unlink(
		ptl_handle_md_t md_handle)
{
        int rc; /* return code */

        lwfs_ptl_lock();

        log_debug(rpc_debug_level, "enter lwfs_ptl_md_unlink");

	rc = lwfs_PtlMDUnlink(md_handle); 
        lwfs_ptl_unlock();
        if (rc != PTL_OK) {
            log_error(rpc_debug_level, "could not alloc eq: %s",
                    ptl_err_str[rc]);
            rc = LWFS_ERR_RPC; 
        }
        
        return (rc);
}

int lwfs_ptl_eq_free(
		ptl_handle_eq_t eq_handle)
{
        int rc; /* return code */

        lwfs_ptl_lock();

        log_debug(rpc_debug_level, "enter lwfs_ptl_eq_free");

	rc = lwfs_PtlEQFree(eq_handle); 
        lwfs_ptl_unlock();
        if (rc != PTL_OK) {
            log_error(rpc_debug_level, "could not alloc eq: %s",
                    ptl_err_str[rc]);
            rc = LWFS_ERR_RPC; 
        }
        
        return (rc);
}

static int use_locks=0;
void lwfs_ptl_use_locks(int should_lock)
{
	use_locks = should_lock;
}

int lwfs_ptl_lock()
{
	int rc=LWFS_OK;
	if (use_locks)
	{
            log_debug(rpc_debug_level, "thread_id(%d): attempting lock of portals_mutex=%p", lwfs_thread_pool_getrank(), &portals_mutex, rc);
            rc = pthread_mutex_lock(&portals_mutex);
            log_debug(rpc_debug_level, "thread_id(%d): locked using portals_mutex=%p, rc=%d", lwfs_thread_pool_getrank(), &portals_mutex, rc);
	}
        
        return(rc);
}
int lwfs_ptl_unlock()
{
	int rc=LWFS_OK;
	if (use_locks)
	{
            log_debug(rpc_debug_level, "thread_id(%d): attempting unlock of portals_mutex=%p", lwfs_thread_pool_getrank(), &portals_mutex, rc);
            rc = pthread_mutex_unlock(&portals_mutex);
            log_debug(rpc_debug_level, "thread_id(%d): unlocked using portals_mutex=%p, rc=%d", lwfs_thread_pool_getrank(), &portals_mutex, rc);
            pthread_yield();
	}        
        return(rc);
}
