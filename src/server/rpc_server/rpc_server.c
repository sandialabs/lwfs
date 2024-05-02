/*-------------------------------------------------------------------------*/
/**  @file rpc_server.c
 *   
 *   @brief Implementation of the \ref rpc_server_api "RPC server API". 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1545 $
 *   $Date: 2007-09-11 18:05:36 -0600 (Tue, 11 Sep 2007) $
 *
 */
 
/**
 * Thoughts on threaded event processing (14 Oct 2005):
 * 
 * thread pooling
 * get a thread in lwfs_service_start event loop
 * make this optional
 *   - some services may wish to be single threaded
 * thread.run() is process_request()
 *   - is process_request() reentrant?
 * synchronize on oid (_oid_el)
 *   - serialize events per object?  
 *   - somebody else's problem?
 *   - how to handle read/write after remove?
 *   - add locking to the API
 *   - last write wins?
 *   - pthread_mutex?
 * lwfs_service_start:cleanup should pthread_join()
 *   - wait for event processing to complete
 *   - timer to avoid hang
 * keep thread stats for tuning
 *   - events processed per thread
 *   - min/max/avg time per event
 *   - min/max/avg time per event by event type
 *   - total time per thread
 *   - num events by event type
 *   - min/max/avg bytes read/written per event
 *   - active/inactive thread counts
 * 
 */

#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "support/threadpool/thread_pool.h"
#include "support/threadpool/thread_pool_debug.h"
#include "support/logger/logger.h"
#include "support/timer/timer.h"
#include "support/signal/lwfs_signal.h"
#include "support/trace/trace.h"
#include "support/sysmon/sysmon.h"

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/rpc_common/rpc_debug.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/rpc_common/ptl_wrap.h"
#include "common/rpc_common/rpc_opcodes.h"
#include "common/rpc_common/rpc_trace.h"
#include "common/rpc_common/service_args.h"


#include "rpc_server.h"

int rpc_get_service(
	const lwfs_remote_pid *caller, 
	const void *args, 
	const lwfs_rma *data_addr,
	lwfs_service *result);

int rpc_kill_service(
	const lwfs_remote_pid *caller, 
	const void *args, 
	const lwfs_rma *data_addr,
	void *result);

int rpc_trace_reset(
	const lwfs_remote_pid *caller, 
	const lwfs_trace_reset_args *args, 
	const lwfs_rma *data_addr,
	void *result);

#ifndef HAVE_PTHREAD
#define pthread_self() 0
#define lwfs_thread_pool_getrank() 0
#endif

/* to synchronize access to portals calls */
/*
 *  On BSD (Darwin/MacOS), pthread_mutex_t is not recursive by default, and so the static
 *  initializer here doesn't work.  Instead, we have to play a little compiler trick, 
 *  getting gcc to generate a file-scope constructor for it.  I'm going to
 *  leave the bail-out #error if we're not being compiled with GCC, since porting
 *  to another compiler should make for a lot of work in other places, not just here.
 */
#if (defined(__APPLE__) && defined(__GNUC__))

static pthread_mutex_t meminfo_mutex;

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
static pthread_mutex_t meminfo_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif          

/**
  * @brief Array of operations supported by all services.
  *
  */
static const lwfs_svc_op svc_op_array[] = {

    /* get service */
    {
	LWFS_OP_GET_SERVICE,
	(lwfs_rpc_proc)&rpc_get_service, 
	sizeof(void), 
	(xdrproc_t)&xdr_void, 
	sizeof(lwfs_service),
	(xdrproc_t)&xdr_lwfs_service 
    },
    {
	LWFS_OP_KILL_SERVICE,
	(lwfs_rpc_proc)&rpc_kill_service, 
	sizeof(void), 
	(xdrproc_t)&xdr_void, 
	sizeof(void),
	(xdrproc_t)&xdr_void 
    },
    {
	LWFS_OP_TRACE_RESET,
	(lwfs_rpc_proc)&rpc_trace_reset, 
	sizeof(lwfs_trace_reset_args), 
	(xdrproc_t)&xdr_lwfs_trace_reset_args, 
	sizeof(void),
	(xdrproc_t)&xdr_void 
    },
    {LWFS_OP_NULL}
};

static lwfs_service local_service; 

#undef USE_THREADED_SERVERS


typedef struct {
    lwfs_service *svc;
    lwfs_thread_pool_args *pool_args;
} thread_args; 

typedef struct {
    lwfs_service *svc;
    lwfs_remote_pid caller;
    char *req_buf;
    lwfs_size short_req_len;
} thr_request;



static lwfs_svc_op *supported_ops = NULL;
static int num_supported_ops = 0;

unsigned long max_mem_allowed=0;

/* ----------- Implementation of core services ----------- */

/**
  * @brief Return the service description of this service.
  * 
  * @param caller @input_type the client's PID
  * @param args @input_type arguments needed to verify the container
  * @param data_addr @input_type address at which the bulk data can be found
  * @param result @output_type no result
  *
  * @returns The service descriptor for this service.
 */
int rpc_get_service(
	const lwfs_remote_pid *caller, 
	const void *args, 
	const lwfs_rma *data_addr,
	lwfs_service *result)
{
    int rc = LWFS_OK;

    /* copy the service description into the result */
    log_debug(rpc_debug_level, "entered get service");

    memcpy(result, &local_service, sizeof(lwfs_service));

    return rc; 
}


/**
  * @brief Schedule this service to be killed. 
  * 
  * @param caller @input_type the client's PID
  * @param args @input_type arguments needed to verify the container
  * @param data_addr @input_type address at which the bulk data can be found
  * @param result @output_type no result
  *
  * @returns The service descriptor for this service.
 */
int rpc_kill_service(
	const lwfs_remote_pid *caller, 
	const void *args, 
	const lwfs_rma *data_addr,
	void *result)
{
    int rc = LWFS_OK;

    /* copy the service description into the result */
    log_debug(rpc_debug_level, "killing service");

    /* set the "exit_now" flag. */
    lwfs_abort();

    return rc; 
}


/**
  * @brief Reset the tracing library.
  *
  * If the service is currently using the tracing
  * library, this operation will reset the tracing
  * library... forcing a flush of the previous file
  * and creating new file for the trace data. All 
  * counts and timers are reset to 0.
  * 
  * @param caller @input_type the client's PID
  * @param args @input_type arguments needed to verify the container
  * @param data_addr @input_type address at which the bulk data can be found
  * @param result @output_type no result
  *
  * @returns The service descriptor for this service.
 */
int rpc_trace_reset(
	const lwfs_remote_pid *caller, 
	const lwfs_trace_reset_args *args, 
	const lwfs_rma *data_addr,
	void *result)
{
    int rc = LWFS_OK;

    const char *fname = args->fname;
    const int ftype = args->ftype; 
    const int enable_flag = args->enable_flag; 

    /* copy the service description into the result */
    log_debug(rpc_debug_level, "reset tracing(%s, %d)",
	    fname, ftype);

    /* set the "exit_now" flag. */
    trace_reset(enable_flag, fname, ftype);

    return rc; 
}


/* ----------- Implementation of the LWFS messaging -------- */

/**
 * @brief  Fetch or extract the operation arguments. 
 *
 * If the args were small enough to fit into the short
 * request buffer, we extract the args directly from
 * the buffer.  Otherwise, we GET the arguments from 
 * a remote memory descriptor on the client. 
 *
 * @param encoded_buf  The encoded short request buffer.
 * @param header  The request header.
 * @param decode_args  The XDR function that decodes the arguments.
 * @param args  Where to place the decoded arguments.
 */
static int fetch_args(
        lwfs_request_header *header, 
        xdrproc_t xdr_decode_args,
        void *args)
{
	int rc;   /* return code from non-LWFS methods */
	XDR xdrs; 

	/* pointer to the decoded buffer for arguments */
	char *encoded_args_buf = NULL; 
	lwfs_size encoded_args_size = header->args_addr.len;

	/* allocate the decoded buffer */
	encoded_args_buf = (char *)malloc(encoded_args_size);

	assert(header->fetch_args); 

	log_debug(rpc_debug_level,
			"thread_id(%d): get args from client ", 
			lwfs_thread_pool_getrank());

	/* fetch the buffer from the client */
	rc = lwfs_ptl_get(encoded_args_buf, 
			encoded_args_size, 
			&header->args_addr);
	if (rc != LWFS_OK) {
		log_fatal(rpc_debug_level,
				"could not get args from client");
		goto cleanup;
	}


	/* decode the arguments */
	log_debug(rpc_debug_level,"thread_id(%d): decoding args, size=%d",
			lwfs_thread_pool_getrank(),
			encoded_args_size);

	/* create an xdr memory stream for the decoded args */
	xdrmem_create(&xdrs, encoded_args_buf,
			encoded_args_size, 
			XDR_DECODE);

	/* decode -- will allocate memory if necessary */
	if (! xdr_decode_args(&xdrs, args)) {
		log_fatal(rpc_debug_level,"could not decode args");
		return LWFS_ERR_DECODE; 
	}

cleanup:

	/* if we had to fetch the args, we need to free the buffer */
	free(encoded_args_buf); 

	return LWFS_OK;
}

/**
 * @brief Send the result back to the client.
 *
 * If the result is small enough to fit inside a small result
 * buffer, the results are sent in one message transfer. If the 
 * results are too large, we tell the client to fetch the result
 * (by setting the fetch_result flag of the result header to true)
 * send the result header, then wait for the client to fetch the
 * result. 
 *
 * @param dest              @input Where to send the encoded result. 
 * @param xdr_encode_result @input function used to encode result.
 * @param return_code       @input The return code of the function. 
 * @param result            @input the result of the function. 
 */
static int send_result(
		int thread_id, 
		unsigned long id, 
		lwfs_rma *dest_addr,
		xdrproc_t xdr_encode_result, 
		int return_code, 
		void *result) 
{
	static uint32_t res_counter = 1;  

	/* portals structs */
	ptl_handle_ni_t ni_h; 
	ptl_handle_eq_t eq_h; 
	ptl_process_id_t match_id; 
	ptl_match_bits_t match_bits  = 0; 
	ptl_match_bits_t ignore_bits = 0; 
	ptl_md_t md; 
	ptl_handle_md_t md_h; 
	ptl_handle_me_t me_h; 

	int rc;  /* return code for non-LWFS methods */

	uint32_t hdr_size; 
	uint32_t res_size; 
	uint32_t res_buf_size; 
	uint32_t remaining; 
	uint32_t valid_bytes; 
	char *short_res_buf; 
	char *long_res_buf = NULL; 
	lwfs_result_header header; 

	/* get the network interface */
	lwfs_ptl_get_ni(&ni_h);

	/* xdrs for the header and the result. */
	XDR hdr_xdrs, res_xdrs;

	/* initialize the result header */
	memset(&header, 0, sizeof(lwfs_result_header));




	/* --- HANDLE ERROR CASE --- */

	/* If a function returns an error, we return the error code, but 
	 * no result.  */
	if (return_code != LWFS_OK) {

		/* treat as if there is no result to return */
		xdr_encode_result = (xdrproc_t)&xdr_void;
	}

	

	/* --- CALCULATE SIZES --- */

	/* Calculate size of the encoded header */
	hdr_size = xdr_sizeof((xdrproc_t)&xdr_lwfs_result_header, &header);

	/* Calculate size of the encoded result */
	res_size = xdr_sizeof(xdr_encode_result, result);

	/* Extract the size of the client-side buffer for the result */
	res_buf_size = dest_addr->len; 

	/* Calculate space left in the short result buffer */
	remaining = dest_addr->len - hdr_size; 





	/* allocated an xdr memory stream for the short result buffer */ 
	assert(dest_addr->len > 0);
	short_res_buf = calloc(dest_addr->len, 1);
	xdrmem_create(&hdr_xdrs, short_res_buf, 
			dest_addr->len, XDR_ENCODE); 



	/* If the result fits in the short result buffer, send it with the header */
	if (res_size < remaining) {

		log_debug(rpc_debug_level,"thread_id(%d): sending short_result %lu, "
				"available space = %d, result_size = %d", 
				thread_id, id, remaining, res_size); 

		/* client needs this information from the header */
		header.fetch_result = FALSE; 
		header.id = id; 
		header.rc = return_code; 
		header.result_addr.len = res_size; 

		/* encode the header  */
		log_debug(rpc_debug_level,"thread_id(%d): encode result header", thread_id);
		if (! xdr_lwfs_result_header(&hdr_xdrs, &header)) {
			log_fatal(rpc_debug_level,
					"failed to encode the result header");
			return LWFS_ERR_ENCODE;
		}

		/* encode the result in the header */
		log_debug(rpc_debug_level,"thread_id(%d), encode result data", thread_id);
		if (! xdr_encode_result(&hdr_xdrs, result)) {
			log_fatal(rpc_debug_level,
					"failed to encode the result");
			return LWFS_ERR_ENCODE;
		}
	}


	/* if result does not fit, client has to fetch result */
	else { 

		match_bits = res_counter++;

		log_debug(rpc_debug_level,"thread_id(%d): sending long result %lu, "
				"available space = %d, result_size = %d", 
				thread_id, id, remaining, res_size); 


		/* allocate memory for the result
		 * structure keeps track of the buffer so it can free 
		 * the memory later. */
		long_res_buf = (char *)malloc(res_size);

		log_debug(rpc_debug_level,"thread_id(%d): storing long result at "
				"ptl_index=%d, match_bits=%d", 
				thread_id, LWFS_LONG_RES_PT_INDEX, match_bits); 

		/* post the memory descriptor for the long result */

		/* create an event queue */
		/* TODO: should we share an event queue? */
		rc = lwfs_ptl_eq_alloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h); 
		if (rc != LWFS_OK) {
			log_error(rpc_debug_level, "failed to allocate eventq");
			rc = LWFS_ERR_RPC;
			goto cleanup;
		}

		/* We expect the data reqs to come from "dest" */
		match_id.nid = dest_addr->match_id.nid;
		match_id.pid = dest_addr->match_id.pid; 

		/* create a match entry (unlink with MD) */
		rc = lwfs_ptl_me_attach(ni_h, LWFS_LONG_RES_PT_INDEX, 
				match_id, match_bits, ignore_bits,
				PTL_UNLINK, PTL_INS_AFTER, &me_h); 
		if (rc != LWFS_OK) {
			log_error(rpc_debug_level, "failed to allocate eventq");
			rc = LWFS_ERR_RPC;
			goto cleanup;
		}

		/* initialize the md */
		memset(&md, 0, sizeof(ptl_md_t)); 
		md.start = long_res_buf; 
		md.length = res_size;
		md.threshold = 1;  /* expect only one get */
		md.options = PTL_MD_OP_GET; 
		md.user_ptr = NULL; 
		md.eq_handle = eq_h; 

		/* attach the memory descriptor (manually unlink) */
		rc = lwfs_ptl_md_attach(me_h, md, PTL_UNLINK, &md_h); 
		if (rc != LWFS_OK) {
			log_error(rpc_debug_level, "failed to attach md");
			rc = LWFS_ERR_RPC; 
			goto cleanup;
		}


		/* we want the client to fetch the result */
		/* client needs this information from the header */
		header.fetch_result = TRUE;
		header.id = id; 
		header.rc = return_code; 
		lwfs_get_id(&header.result_addr.match_id);
		header.result_addr.buffer_id = LWFS_LONG_RES_PT_INDEX;
		header.result_addr.offset = 0;
		header.result_addr.match_bits = match_bits;
		header.result_addr.len = res_size;


		/* create a xdr memory stream for the encoded args buffer */
		xdrmem_create(&res_xdrs, long_res_buf, 
				res_size, XDR_ENCODE); 

		/* encode the header  */
		log_debug(rpc_debug_level,"thread_id(%d): encode result %lu header", thread_id, id);
		if (! xdr_lwfs_result_header(&hdr_xdrs, &header)) {
			log_fatal(rpc_debug_level,
					"failed to encode the result header");
			rc = LWFS_ERR_ENCODE;
			goto cleanup;
		}

		/* encode the result  */
		log_debug(rpc_debug_level,"thread_id(%d): encode result %lu data", thread_id, id);
		if (! xdr_encode_result(&res_xdrs, result)) {
			log_fatal(rpc_debug_level,
					"failed to encode the result");
			rc = LWFS_ERR_ENCODE;
			goto cleanup;
		}

	}

	/* send the short result to the client */
	valid_bytes = hdr_size;

	if (!header.fetch_result)
		valid_bytes += res_size; 

	log_debug(rpc_debug_level,"thread_id(%d): send short result %lu "
			"(in xdr bytes:  len=%d bytes: header=%d byte, res=%d bytes)",
			thread_id, id, valid_bytes, hdr_size, res_size);

	if (logging_debug(rpc_debug_level)) {
		fprint_lwfs_rma(logger_get_file(), "short_result_addr", 
				"DEBUG", dest_addr);
	}

	log_debug(rpc_debug_level,"thread_id(%d): sizeof(ptl_event_t) = %d", 
			thread_id, sizeof(ptl_event_t));

	rc = lwfs_ptl_put(short_res_buf, valid_bytes, dest_addr); 
	if (rc != LWFS_OK) {
		log_fatal(rpc_debug_level, "failed to put result %lu", id); 
		goto cleanup;
	}

	/* if the client has to fetch the results, we need to wait for 
	 * the GET to complete */
	if (header.fetch_result) {
		ptl_handle_ni_t ni_h; 
		ptl_event_t event; 
		lwfs_bool got_get_start = FALSE;
		lwfs_bool got_get_end = FALSE; 
		lwfs_bool got_unlink = FALSE; 
		lwfs_bool done = FALSE; 

		lwfs_ptl_get_ni(&ni_h);

		log_debug(rpc_debug_level, "thread_id(%d): waiting for client to "
				"fetch result %lu", thread_id, id);
		while (!done){ 

			/* wait for next event */
			rc = lwfs_ptl_eq_wait(eq_h, &event); 
			if (rc != LWFS_OK) {
				log_error(rpc_debug_level, "failed to get event");
				goto cleanup;
			}

			switch (event.type) {

				case PTL_EVENT_GET_START:
					got_get_start = TRUE;
					if (event.ni_fail_type != PTL_NI_OK) {
						log_error(rpc_debug_level, 
								"failed on get start: ni_fail_type=%d\n",
								event.ni_fail_type);
						rc = LWFS_ERR_RPC; 
						goto cleanup;
					}
					break;
				case PTL_EVENT_GET_END:
					got_get_end = TRUE;
					if (event.ni_fail_type != PTL_NI_OK) {
						log_error(rpc_debug_level, "failed on get end: ni_fail_type=%d\n",
								event.ni_fail_type);
						rc = LWFS_ERR_RPC; 
						goto cleanup;
					}
					break;
				case PTL_EVENT_UNLINK:
					got_unlink = TRUE;
					if (event.ni_fail_type != PTL_NI_OK) {
						log_error(rpc_debug_level, "failed on unlink: ni_fail_type=%d\n",
								event.ni_fail_type);
						rc = LWFS_ERR_RPC; 
						goto cleanup;
					}
					break;
				default:
					log_error(rpc_debug_level, "unexpected event (%d)",
							event.type);
					done = TRUE;
					rc = LWFS_ERR_RPC;
					goto cleanup;
			}

			if (got_get_start && got_get_end && got_unlink) {
				done = TRUE;
			}
		}
	}


cleanup:

	if (header.fetch_result) {
		int rc2; 

		/* free the event queue */
		rc2 = lwfs_ptl_eq_free(eq_h); 
		if (rc2 != LWFS_OK) {
			log_error(rpc_debug_level, "unable to free EQ");
			rc2 = LWFS_ERR_RPC; 
		}

		/* free the result buffer */
		free(long_res_buf);
	}

	/* free the short result buffer */
	free(short_res_buf);

	log_debug(rpc_debug_level, "thread_id(%d): result %lu sent", thread_id, id);

	return rc; 
}


/** 
 * @brief Code executed by the POSIX thread that processes requests.
 */
static void *thread_start(void *args)
{
    int rc = LWFS_OK; 

    lwfs_service *svc                = ((thread_args *)args)->svc; 
    lwfs_thread_pool_args *pool_args = ((thread_args *)args)->pool_args; 

    rc = lwfs_service_start(svc, pool_args);
    if (rc != LWFS_OK) {
        log_error(rpc_debug_level, "error running service: %s",
                lwfs_err_str(rc)); 
    }

    pthread_exit(&rc); 
}



/** 
 * @brief Process a received requests.
 * 
 * Each incoming request arrives as a chunk of xdr data. This 
 * method decodes the mds_request_header, decodes the arguments, 
 * and calls the appropriate method to process the request. 
 *
 * @param caller         @input The process ID of the caller.
 * @param req_buf        @input The encoded short-request buffer. 
 * @param short_req_len  @input The number of bytes in a short request. 
 */ 

int process_request(
        struct lwfs_thread_pool_request *a_request, 
        const int thread_id)
{
	XDR xdrs; 
	int rc; 
	int index = 0;

	lwfs_request_header header; 
	static volatile long req_count = 0;
	int interval_id; 

	/* increment the request count */
	req_count++; 
	interval_id = req_count; 

	thr_request *data = (thr_request *)a_request->client_data;
	/*lwfs_service *svc = data->svc;*/
	lwfs_remote_pid caller = data->caller;
	char *req_buf = data->req_buf;
	lwfs_size short_req_len = data->short_req_len;

	
	log_debug(rpc_debug_level, "i'm doing the work.  my rank is %d", lwfs_thread_pool_getrank());

	/* memory check - log memory statistics.  if memory in use 
	 * is greater than maximum memory allowed, then exit.
	 */
        rc = pthread_mutex_lock(&meminfo_mutex);
	log_meminfo(rpc_debug_level);
	unsigned long main_memory_in_use = main_memory_used();
        rc = pthread_mutex_unlock(&meminfo_mutex);
	log_debug(rpc_debug_level, 
	    	"max memory check (allowed=%lukB, in_use=%lukB, %%free=%f)", 
	    	max_mem_allowed, main_memory_in_use, 100.0-(((float)main_memory_in_use/(float)max_mem_allowed)*100.0));
	if ((max_mem_allowed > 0) && 
	    (main_memory_in_use > max_mem_allowed)) {
	    rc = LWFS_OK;
	    log_error(rpc_debug_level, 
	    	"max memory allowed exceeded (allowed=%lukB, in_use=%lukB), exiting", 
	    	max_mem_allowed, main_memory_in_use);
	    goto cleanup;
	}

	/* initialize the request header */
	memset(&header, 0, sizeof(lwfs_request_header));

	/* create an xdr memory stream from the request buffer */
	xdrmem_create(&xdrs, 
			req_buf, 
			short_req_len, 
			XDR_DECODE);

	/* decode the request header */
	log_debug(rpc_debug_level, "thread_id(%d): decoding header...", thread_id); 
	rc = xdr_lwfs_request_header(&xdrs, &header); 
	if (!rc) {
		log_fatal(rpc_debug_level, "thread_id(%d): failed to decode header", thread_id);
		abort();
	}

	log_debug(thread_debug_level, "thread %d: begin processing request (%lu) with opcode (%lu)\n", 
			thread_id, header.id, header.opcode);



	/* linear search to find out if the operation is in our list of ops */
	index = 0; 
	for (index=0; index<num_supported_ops; index++) {

		const lwfs_svc_op *op = &supported_ops[index]; 

		/* if there is a match, extract the args and execute the function */
		if (op->opcode == header.opcode) {

			/* allocate space for args and result (these are passed in with the header) */
			void *args = malloc(header.args_addr.len); 
			void *res  = malloc(header.res_addr.len); 

			/* initialize args and res */
			memset(args, 0, header.args_addr.len); 
			memset(res, 0, header.res_addr.len); 
			log_debug(rpc_debug_level, "thread_id(%d): header.res_addr.len==%d\n", 
					thread_id, header.res_addr.len);

			/* If the args fit in the header, extract them from the 
			 * header buffer.  Otherwise, get them from the client 
			 */
			if (!header.fetch_args) {
				if (! op->decode_args(&xdrs, args)) {
					log_fatal(rpc_debug_level,"could not decode args");
					rc = LWFS_ERR_DECODE; 
					goto cleanup; 
				}
			}
			else {
				/* fetch the operation arguments */
				log_debug(rpc_debug_level, "thread_id(%d): fetching args", thread_id);
				// is reentrant??
				rc = fetch_args(
						&header, 
						op->decode_args, 
						args);
				if (rc != LWFS_OK) {
					log_fatal(rpc_debug_level, 
							"thread_id(%d): unable to fetch args", thread_id);
					goto cleanup; 
				}
			}

			/*
			 ** Process the request (print warning if method fails), but
			 ** don't return error, because some operations are meant to fail
			 */
			log_debug(rpc_debug_level, "thread_id(%d): calling the server function", thread_id);

			trace_start_interval(interval_id, thread_id);

			// is reentrant??
			rc = op->func(&caller, args, &header.data_addr, res); 
			if (rc != LWFS_OK) {
				log_warn(rpc_debug_level, "thread_id(%d): User op failed: rc=%d", thread_id, rc);
			}
			trace_end_interval(interval_id, TRACE_RPC_PROC, thread_id, "operation timer");

			/* send result back to client */
			log_debug(rpc_debug_level, "thread_id(%d): sending result %lu back to client", 
					thread_id, header.id);
			// is reentrant??


			/* measure time for the send result portion */
			trace_start_interval(interval_id, thread_id);

			lwfs_ptl_lock();
			rc = send_result(thread_id, header.id, &header.res_addr, 
					op->encode_res, rc, res);
			lwfs_ptl_unlock();

			trace_end_interval(interval_id, TRACE_RPC_SENDRES, thread_id, "sendres timer");

			if (rc != LWFS_OK) {
				log_fatal(rpc_debug_level, "thread_id(%d): unable to send result %lu",
						thread_id, header.id);
				goto cleanup; 
			}

			/* free data structures created for the args and result */
			log_debug(rpc_debug_level, "thread_id(%d): xdr_freeing args", thread_id);
			xdr_free((xdrproc_t)op->decode_args, (char *)args); 
			log_debug(rpc_debug_level, "thread_id(%d): xdr_freeing result", thread_id);
			xdr_free((xdrproc_t)op->encode_res, (char *)res); 

			log_debug(rpc_debug_level, "thread_id(%d): freeing args", thread_id);
			free(args);
			log_debug(rpc_debug_level, "thread_id(%d): freeing result", thread_id);
			free(res);

			log_debug(rpc_debug_level, "thread_id(%d): result freed", thread_id);

			rc = LWFS_OK; 
			goto cleanup; 
		}

		/* else if no match, check next item */
	}

	/* if we get here, there is not match */
	log_warn(rpc_debug_level, "thread_id(%d): unrecognized request: opcode=%d", thread_id,
			header.opcode);
	rc = LWFS_ERR_RPC; 

cleanup:

	log_debug(thread_debug_level, "thread %d: finished processing request %lu\n", thread_id, header.id);

	/* free the client data */
	free(data);
	return rc; 
}


/**
 * @brief An abstract method to get data from a remote memory descriptor.
 *
 * The server stub uses this function to get or put data to a 
 * client memory descriptor. 
 *
 * @param buf    @input   the buffer for the data.
 * @param len    @input   the maximum length of the buffer.
 * @param src_md @input   the remote memory descriptor.
 */
int lwfs_get_data(
        void *buf,
        const int len,
        const lwfs_rma *data_addr)
{
	int rc = LWFS_OK;

	if (len == 0)
		return rc;

	rc = lwfs_ptl_get(buf, len, data_addr);
	if (rc != LWFS_OK) {
		log_error(rpc_debug_level, "failed getting data: %s",
				lwfs_err_str(rc));
	}


	return rc; 
}

/**
 * @brief An abstract method to put data into a remote memory descriptor.
 *
 * The server stub uses this function to put data to a 
 * client memory descriptor. 
 *
 * @param buf    @input the buffer for the data.
 * @param len    @input   the amount of data to send.
 * @param dest_md   @input the remote memory descriptor.
 */
extern int lwfs_put_data(
        const void *buf,
        const int len,
        const lwfs_rma *data_addr)
{
	int rc = LWFS_OK;

	if (len == 0)
		return rc;

	rc = lwfs_ptl_put(buf, len, data_addr);
	if (rc != LWFS_OK) {
		log_error(rpc_debug_level, "failed putting data: %s",
				lwfs_err_str(rc));
	}

	return rc; 
}


/**
 * @brief Register an RPC service. 
 * 
 * This method creates a named RPC service on the specified
 * registry server.  Along with the name, the server has to 
 * specify where (in the form of an \ref lwfs_md) the 
 * client should "put" requests.  
 *
 * @todo For now, the registry is on the same host as the service 
 *       (registrcy_id is ignored). At some point, we need to separate
 *       the registry service. 
 *
 * @param registry_id @input Process ID of the registry server. 
 * @param name        @input Name of the service. 
 * @param remote_sd   @input The remote service description to associate with the name. 
 * @param req         @output The request handle (used to test for completion).
 */
 /*
int lwfs_register_service(
        const lwfs_remote_pid registry_id,
        const char *name, 
        const lwfs_service *svc,
        lwfs_request *req)
{
    return LWFS_ERR_NOTSUPP;
}
*/

/**
 * @brief Add an operation to an operation list.
 *
 * This method adds an operation to an operation list used by 
 * a registered RPC service. Operations in this list must have 
 * the following prototype:
 *
 * \code
 *    int svc_fun(svc_args *args, svc_result *res);
 * \endcode
 *
 * where \b svc_args is a data structure that contains all required 
 * arguments for the method, and \b svc_result is a structure 
 * that contains the result. 
 * 
 * @param svc_op      @input The \ref lwfs_svc_op operation description. 
 * @param op_list     @output The operation list (modified by this function).
 * 
 * @returns \ref LWFS_OK if the operation was successfully added. 
 */
int lwfs_add_svc_op(
        const lwfs_svc_op *svc_op, 
        lwfs_svc_op_list **op_list)
{
    lwfs_svc_op_list *new_list = NULL; 

    /* create a new entry */
    new_list = (lwfs_svc_op_list *)malloc(1*sizeof(lwfs_svc_op_list)); 

    /* initialize the entry */
    new_list->svc_op.opcode = svc_op->opcode; 
    new_list->svc_op.func = svc_op->func; 
    new_list->svc_op.sizeof_args = svc_op->sizeof_args; 
    new_list->svc_op.decode_args = svc_op->decode_args; 
    new_list->svc_op.sizeof_res  = svc_op->sizeof_res; 
    new_list->svc_op.encode_res  = svc_op->encode_res; 

    /* push entry onto the front of the list */
    new_list->next = *op_list; 
    *op_list = new_list; 

    return LWFS_OK;
}


/**
 * @brief Initialize an RPC server.
 *
 * @ingroup rpc_server_api
 *
 * This method initializes the portals library and allocates space
 * for incoming requests.
 *
 * @param portal_index @input the portals index to use for incoming reqs.
 * @param service      @output the local service descriptor (used by the server).
 */
int lwfs_service_init(
        const lwfs_match_bits match_bits,
        const int short_req_len, 
        lwfs_service *service)
{
    int rc = LWFS_OK;
    lwfs_rma *remote_addr; 

    /* initialize the service descriptors */
    memset(service, 0, sizeof(lwfs_service));

    /* use XDR to encode control messages */
    service->rpc_encode = LWFS_RPC_XDR;

    /* initialize the remote address where others "put" requests. */
    remote_addr = &(service->req_addr); 
    memset(remote_addr, 0, sizeof(lwfs_rma));
    lwfs_get_id(&(remote_addr->match_id));
    remote_addr->buffer_id = LWFS_REQ_PT_INDEX;
    remote_addr->offset = 0;
    remote_addr->match_bits = match_bits; 
    remote_addr->len = short_req_len; 

    /* copy the service description to the local service description */
    memcpy(&local_service, service, sizeof(lwfs_service));

    /* add the default services */
    rc = lwfs_service_add_ops(service, svc_op_array, 3);
    if (rc != LWFS_OK) {
	log_error(rpc_debug_level, "unable to add default svc ops: %s",
		lwfs_err_str(rc));
    }
    return LWFS_OK;
}

/**
  * @brief Add operations to service. 
  *
  * @param svc  @input_type  The service descriptor.
  * @param ops  @input_type  The array operations to add to the service.
  * @param len  @input_type  The number of operations to add.
  */
int lwfs_service_add_ops(
	const lwfs_service *svc,
	const lwfs_svc_op *ops, 
	const int len)
{
	int rc = LWFS_OK;
	int i;
	int total = num_supported_ops + len;
	int count = 0; 
		
	supported_ops = (lwfs_svc_op *)realloc(supported_ops, total*sizeof(lwfs_svc_op));
	if (supported_ops == NULL) {
		return LWFS_ERR_NOSPACE;
	}
	
	count = 0;
	for (i=num_supported_ops; i<total; i++) {
		memcpy(&supported_ops[i], &ops[count++], sizeof(lwfs_svc_op));
	}

	/* update the number of suported ops */
	num_supported_ops = total; 

	return rc; 
}



/**
 * @brief Close down an active service.
 *
 * @ingroup rpc_server_api
 *
 * Shuts down the service and releases any associated resources.
 *
 * @param local_sd @input The local service descriptor.
 */
int lwfs_service_fini(const lwfs_service *service)
{
	if (supported_ops != NULL) free(supported_ops);
    return LWFS_OK;
}


#define NUM_QUEUES 2

/**
 * @brief Start the RPC server.
 *
 * This method never returns.  It waits for an RPC request, and
 * calls the appropriate function from the list of services. 
 *
 * This implementation allocates two buffers that can hold a fixed
 * number of incoming requests each and alternates between 
 * processing requests from the two buffers.
 *
 * @param service  @input The service descriptor. 
 * @param count @input The maximum number of requests to process.
 * @param svc_op_list    @input The list of available operations. 
 */
int lwfs_service_start(
	lwfs_service *svc,
	const lwfs_thread_pool_args *pool_args)
{
    int rc = LWFS_OK, rc2;
    int req_count = 0;
    int index = 0; 
    int offset = 0; 
    lwfs_bool done = FALSE; 
    lwfs_bool got_put_start_event = FALSE; 
    lwfs_bool got_put_end_event   = FALSE; 
    double t1;
    double idle_time = 0; 
    double processing_time = 0; 
    lwfs_bool use_threads = (pool_args != NULL); 

    /* two incoming queues */
    char *req_queue[NUM_QUEUES]; 
    char *req_buf; 

    /* each message is no larger than req_size */
    int req_size = svc->req_addr.len;

    /* each md can recv reqs_per_queue messages */
    int reqs_per_queue = 10000;

    /* keep track of the queue index and count */
    int indices[NUM_QUEUES];
    int queue_count[NUM_QUEUES];

    /* portals structs */
    ptl_handle_ni_t ni_h; 
    ptl_handle_eq_t eq_h; 
    ptl_process_id_t match_id; 
    ptl_match_bits_t match_bits = 0;
    ptl_match_bits_t ignore_bits;
    ptl_event_t event; 

    /* for each queue, we need these structs */
    ptl_md_t md[NUM_QUEUES];
    ptl_handle_md_t md_h[NUM_QUEUES]; 
    ptl_handle_me_t  me_h[NUM_QUEUES];

    lwfs_remote_pid caller; 

    lwfs_thread_pool pool;
    thr_request *req=NULL;

    if (use_threads) {
	/* make our portals abstraction thread-safe */
	lwfs_ptl_use_locks(1);

	lwfs_thread_pool_init(&pool, pool_args);
    }
    else {
	/* remove locks from our portals stuff */
	lwfs_ptl_use_locks(0);
    }


    match_id.nid = PTL_NID_ANY;
    match_id.pid = PTL_PID_ANY;

    /* get the network interface */
    lwfs_ptl_get_ni(&ni_h); 

    /* create an event queue */
    rc = lwfs_ptl_eq_alloc(ni_h, reqs_per_queue, PTL_EQ_HANDLER_NONE, &eq_h);
    if (rc != LWFS_OK) {
	log_error(rpc_debug_level, "lwfs_ptl_eq_alloc() failed"); 
	return (rc);
    }

    /* make the req_thread point to pthread_self */
    //service->req_thread = pthread_self(); 


    /* Accept requests from anyone */
    match_id.nid = PTL_NID_ANY;
    match_id.pid = PTL_PID_ANY;
    match_bits = svc->req_addr.match_bits;
    ignore_bits = 0;


    for (index=0; index<NUM_QUEUES; index++) {

	/*void *addr = index; */

	/* initialize the indices stored in the MD user pointer */
	indices[index] = index; 
	queue_count[index] = 0; 

	/* allocate the buffer for the incoming MD */
	req_queue[index] = (char *)malloc(reqs_per_queue*req_size);
	if (req_queue[index] == NULL) {
	    log_error(rpc_debug_level, "out of memory");
	    return LWFS_ERR_NOSPACE; 
	}

	/* initialize the buffer */
	memset(req_queue[index], 0, reqs_per_queue*req_size);

	/* initialize the MD */
	memset(&md[index], 0, sizeof(ptl_md_t)); 
	md[index].start = req_queue[index];
	md[index].length = reqs_per_queue*req_size;
	md[index].threshold = reqs_per_queue;
	md[index].max_size = req_size;
	md[index].options = PTL_MD_OP_PUT | PTL_MD_MAX_SIZE; 
	md[index].user_ptr = &indices[index];  /* used to store the index */
	md[index].eq_handle = eq_h; 

	log_debug(rpc_debug_level, "attaching match entry to index=%d",
		svc->req_addr.buffer_id);

	/* Attach the match entry to the portal index */
	rc = lwfs_ptl_me_attach(ni_h, svc->req_addr.buffer_id, 
		match_id, match_bits, ignore_bits,
		PTL_RETAIN, PTL_INS_AFTER, &me_h[index]); 
	if (rc != LWFS_OK) {
	    log_error(rpc_debug_level, "could not attach ME");
	    return (rc); 
	}

	/* Attach the MD to the match entry */
	rc = lwfs_ptl_md_attach(me_h[index], md[index], PTL_RETAIN, &md_h[index]); 
	if (rc != LWFS_OK) {
	    log_error(rpc_debug_level, "could not alloc eq: %s",
		    ptl_err_str[rc]);
	    return (rc); 
	}
    }

    /* initialize indices and counters */
    req_count = 0; /* number of reqs processed */
    index = 0;     /* which queue to use */
    offset = 0;    /* offset in the queue */

    /* SIGINT (Ctrl-C) will get us out of this loop */
    while (!lwfs_exit_now()) {

	log_debug(rpc_debug_level, "a");

	/* exit if we've received our max number of reqs */
	if ((svc->max_reqs >= 0) && (req_count >= svc->max_reqs)) {
	    rc = LWFS_OK;
	    log_info(rpc_debug_level, "recved max_reqs=%d, exiting",svc->max_reqs);
	    goto cleanup;
	}
	
	/* memory check - log memory statistics.  if memory in use 
	 * is greater than maximum memory allowed, then exit.
	 */
//	log_meminfo(rpc_debug_level);
//	unsigned long main_memory_in_use = main_memory_used();
//	log_debug(rpc_debug_level, 
//	    	"max memory check (allowed=%lukB, in_use=%lukB, %%free=%f)", 
//	    	max_mem_allowed, main_memory_in_use, 100.0-(((float)main_memory_in_use/(float)max_mem_allowed)*100.0));
//	if ((max_mem_allowed > 0) && 
//	    (main_memory_in_use > max_mem_allowed)) {
//	    rc = LWFS_OK;
//	    log_error(rpc_debug_level, 
//	    	"max memory allowed exceeded (allowed=%lukB, in_use=%lukB), exiting", 
//	    	max_mem_allowed, main_memory_in_use);
//	    goto cleanup;
//	}

	done = FALSE; 
	got_put_start_event = FALSE; 
	got_put_end_event   = FALSE; 

	/* measure idle time */
	if (req_count > 0) {
	    t1 = lwfs_get_time();
	}

	/*trace_start_interval(req_count);*/

	/* wait for the next event pair (code breaks out of while loop) */ 
	while (TRUE) { 

	    /* get an event */
	    log_debug(rpc_debug_level, "waiting for request...");


	    rc = lwfs_ptl_eq_wait(eq_h, &event); 


	    if (rc != LWFS_OK) {
		if (!lwfs_exit_now()) {
		    log_error(rpc_debug_level, "failed to get event");
		}
		goto cleanup;
	    }

	    switch (event.type) {
		case PTL_EVENT_PUT_START:
		    log_debug(rpc_debug_level, 
			    "got PTL_EVENT_PUT_START");
		    got_put_start_event = TRUE;
		    if (event.ni_fail_type != PTL_NI_OK) {
			log_error(rpc_debug_level, "failed on put start: ni_fail_type=%d\n",
				event.ni_fail_type);
			return LWFS_ERR_RPC; 
		    }
		    break;

		case PTL_EVENT_PUT_END:
		    log_debug(rpc_debug_level, 
			    "got PTL_EVENT_PUT_END");
		    got_put_end_event = TRUE;
		    if (event.ni_fail_type != PTL_NI_OK) {
			log_error(rpc_debug_level, "failed on put end: ni_fail_type=%d\n",
				event.ni_fail_type);
			return LWFS_ERR_RPC; 
		    }
		    break;

		default:
		    log_error(rpc_debug_level, 
			    "unrecognized event type: %d",
			    event.type);
		    rc = LWFS_ERR_RPC; 
		    goto cleanup;
	    }


	    /* only care about end events */
	    if (got_put_end_event) {
		offset = event.offset; 
		req_buf = event.md.start + offset; 
		break; 
	    }
	}

	/* capture the idle time */
	idle_time += lwfs_get_time() - t1; 
	/*trace_end_interval(req_count, TRACE_RPC_IDLE, 0, 0, "idle time");*/

	/* get the index of the queue */
	index = *(int *)event.md.user_ptr;


	/* increment the number of requests */
	req_count++; 
	queue_count[index]++;

	/* Now we can process the request */
	log_info(rpc_debug_level, "received request %d from (%llu,%llu) "
		"on queue=%d, offset=%d, req_size=%d", 
		(int)req_count,
		(unsigned long long)event.initiator.nid,
		(unsigned long long)event.initiator.pid,
		*((int *)event.md.user_ptr), 
		(int)event.offset, 
		(int)event.mlength);

	caller.nid = event.initiator.nid; 
	caller.pid = event.initiator.pid; 

	req = (thr_request *)calloc(1,sizeof(thr_request));
	req->svc = svc;
	req->caller = caller;
	req->req_buf = req_buf;
	req->short_req_len = event.mlength;

	if (use_threads) {
	    /* add the request to the thread pool */
	    lwfs_thread_pool_add_request(&pool, req, &process_request);
	}
	else {
	    /* process the request directly */
	    struct lwfs_thread_pool_request a_request; 
	    /* measure the processing time */
	    t1 = lwfs_get_time();

	    a_request.client_data = req; 
	    rc = process_request(&a_request, 0); 
	    if (rc != LWFS_OK) {
		/* warn only... we do not exit */
		log_warn(rpc_debug_level, "main: unable to process request.");
	    }

	    /* measure the processing time */
	    processing_time += lwfs_get_time() - t1; 
	}


	/* if we've processed all we can on this queue, reset */
	if (queue_count[index] >= reqs_per_queue) {

	    log_debug(rpc_debug_level, "Resetting MD on queue[%d]", index);

	    /* Unlink the ME (also unlinks the MD) */
	    rc = lwfs_PtlMEUnlink(me_h[index]);
	    if (rc != PTL_OK) {
		log_error(rpc_debug_level, "Could not unlink ME: %s", ptl_err_str[rc]);
		goto cleanup; 
	    }

	    /* Re-attach the match-list entry */
	    rc = lwfs_PtlMEAttach(ni_h, svc->req_addr.buffer_id, match_id, 
		    match_bits, ignore_bits, PTL_RETAIN, PTL_INS_AFTER, 
		    &me_h[index]);
	    if (rc != PTL_OK) {
		log_error(rpc_debug_level, "Could not reset ME: %s", ptl_err_str[rc]);
		goto cleanup; 
	    }

	    /* Re-attach the MD */
	    rc = lwfs_PtlMDAttach(me_h[index], md[index], PTL_RETAIN, &md_h[index]);
	    if (rc != PTL_OK) {
		log_error(rpc_debug_level, "Could not reset MD: %s", ptl_err_str[rc]);
		goto cleanup; 
	    }
	    
	    queue_count[index] = 0;
	}

    }


cleanup:

    /* finish any tracing */
    trace_fini();

    rc = LWFS_OK;
    for (index=0; index<NUM_QUEUES; index++) {
	/* unlink the memory descriptor */
	rc2 = lwfs_ptl_md_unlink(md_h[index]); 
	if (rc2 != LWFS_OK) {
	    log_warn(rpc_debug_level, "unable to unlink memory descriptor for queue %d: %s",
		    index, ptl_err_str[rc2]);
	    rc = LWFS_ERR;
	}

	/* free the request queue buffers */
	free(req_queue[index]);
    }

    /* free the event queue */
    rc2 = lwfs_ptl_eq_free(eq_h); 
    if (rc2 != LWFS_OK) {
	log_fatal(rpc_debug_level, "unable to free event queue (%s)",
		ptl_err_str[rc2]);
	rc = LWFS_ERR;
    }

    if (use_threads) {
	log_debug(rpc_debug_level, "shutting down thread pool");
	lwfs_thread_pool_fini(&pool);
    }

    /* print out stats about the server */
    log_info(rpc_debug_level, "Exiting lwfs_service_start: %d "
	    "reqs processed, exit_now=%d", req_count,lwfs_exit_now());

    //if (logging_info(rpc_debug_level)) {
    FILE *fp = logger_get_file();
    fprintf(fp, "----- SERVER STATS ---------\n");
    fprintf(fp, "\tprocessed requests = %d\n", req_count);
    //fprintf(fp, "\tidle time       = %g (sec)\n", idle_time);
    fprintf(fp, "\tprocessing time = %g (sec)\n", processing_time);
    fprintf(fp, "----------------------------\n");
    //}

    return rc;
}

/**
 * @brief Start an RPC service as a thread. 
 *
 * @ingroup rpc_server_api
 *
 * @param service @input The service descriptor.
 * @param fn_ptr  @input pointer to the 
 *
 * Creates a POSIX thread to process incoming
 * requests for the specified service. 
 *
 */
int lwfs_service_start_thread(
        lwfs_service *service, 
        const lwfs_thread_pool_args *pool_args)
{
    int rc = LWFS_OK; 
    pthread_t thread; 
    thread_args *args = (thread_args *)malloc(1*sizeof(thread_args));

    args->svc = (lwfs_service *)service; 
    args->pool_args = (lwfs_thread_pool_args *)pool_args; 

    /* Create the thread. Do we want special attributes for this? */
    rc = pthread_create(&thread, NULL, thread_start, args);
    if (rc) {
        log_error(rpc_debug_level, "could not spawn thread");
        rc = LWFS_ERR_RPC; 
    }

    /* assign the request thread ID */
    service->req_thread = thread; 

    return rc; 
}
