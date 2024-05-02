/*-------------------------------------------------------------------------*/
/**   
 *   @file rpc_clnt.h
 * 
 *   @brief Prototypes for the client-side methods for RPC. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   $Revision: 1354 $
 *   $Date: 2007-04-24 11:20:13 -0600 (Tue, 24 Apr 2007) $
 */

#ifndef _LWFS_RPC_CLNT_H_
#define _LWFS_RPC_CLNT_H_

#include "common/types/types.h"
#include "common/rpc_common/rpc_debug.h"
#include "common/rpc_common/rpc_common.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/config_parser/config_parser.h"

/**
 * @defgroup rpc_client_api  Communicating with Remote Services
 * 
 * An LWFS client communicates with remote services using an 
 * asynchronous, remote procedure call (RPC) interface. 
 * As described in @latexonly Section~\ref{sec:Data-movement},@endlatexonly 
 * this interface uses the Portals message passing API to
 * take advantage of special features of the network such 
 * as RDMA and OS bypass.  
 *
 * The typical usage scenerio includes the following steps: 
 *
 * -# Obtain the service description by calling the 
 *   \ref lwfs_lookup_service "\c lwfs_lookup_service()" or 
 *   \ref lwfs_get_service "\c lwfs_get_service()" functions.  The service
 *   description, represented by the <tt>lwfs_service</tt> data structure,
 *   contains details about how to communicate with the 
 *   server, including the remote memory address reserved for incoming
 *   requests and the encoding scheme to use for control messages. 
 * -# Call the asynchronous \ref lwfs_call_rpc "\c lwfs_call_rpc()" function
 *   specifying the operation and providing buffer space for 
 *   arguments, bulk data, and results.  The buffers must remain
 *   unmodified by the client until the remote operation completes. 
 * -# To check for completion, call the functions \ref lwfs_wait "\c lwfs_wait()",
 *   \ref lwfs_timedwait "\c lwfs_timedwait()", or \ref lwfs_test "\c lwfs_test"
 *   with the <tt>\ref lwfs_request</tt> data structure that was created
 *   by the \ref lwfs_call_rpc "\c lwfs_call_rpc()" function.
 *   
 * This remainder of this section contains descriptions of the 
 * data structures, special data types, and functions that support 
 * client/service communication. 
 *
 *  
 *  @latexonly \input{generated/structlwfs__remote__pid} @endlatexonly
 *  @latexonly \input{generated/structlwfs__request} @endlatexonly
 *  @latexonly \input{generated/structlwfs__rma} @endlatexonly
 *  @latexonly \input{generated/structlwfs__service} @endlatexonly
 */


#ifdef __cplusplus
extern "C" {
#endif

	/** 
	 * @ingroup rpc_client_api
	 *
	 * @brief States for a pending RPC request. 
	 *
	 *
	 * The <tt>\ref lwfs_request_status </tt> enumerator provides 
	 * integer values that identify the state of a pending LWFS request. 
	 */
	enum lwfs_request_status {
		/** @brief The request is complete with an error. */
		LWFS_REQUEST_ERROR = -1,      

		/** @brief The client is sending the request to server. */
		LWFS_SENDING_REQUEST = 1,     

		/** @brief The remote server is processing the request. */
		LWFS_PROCESSING_REQUEST, 

		/** @brief The client is processing the result. */
		LWFS_PROCESSING_RESULT,   

		/** @brief The request is complete. */
		LWFS_REQUEST_COMPLETE,
	};

	typedef enum lwfs_request_status lwfs_request_status;


	/* 
	 * @ingroup rpc_client_api_test
	 *
	 * @brief The Request Structure.
	 *
	 * The <tt>\ref lwfs_request</tt> structure represents a pending LWFS 
	 * request.  It contains a unique identifier and pointers to all
	 * data structures and buffers needed to send the request to the 
	 * remote server and to process the result when the server completes. 
	 * 
	 * \todo We need to abstract out the implementation-specific portions 
	 * of this data structure.  These include the fields to encode/decode
	 * arguments and the all of the Portals data structures. 
	 */
	typedef struct lwfs_request {
		/** @brief An ID for this request. */
		unsigned long id; 

		/** @brief The opcode of the remote function. */
		lwfs_opcode opcode;

		/** @brief Points to the memory reserved for the result. */
		void *result;        

		/** @brief Points to the memory reserved for the bulk data transfers (NULL if not used). */
		void *data; 

		/** @brief The error code of request. This value will be \ref LWFS_OK unless the 
		 *          request status=\ref LWFS_REQUEST_ERROR . */
		int error_code;   

		/** @brief Status of the pending request */
		lwfs_request_status status;   


		/* IMPLEMENTATION-SPECIFIC PORTION */

		/** @brief Points to the XDR function used to encode arguments. This
		  field is implementation specific. */
		xdrproc_t xdr_encode_args;    

		/** @brief Points to the XDR function used to encode data. This
		  field is implementation specific. */
		xdrproc_t xdr_encode_data;    

		/** @brief Points to the XDR function used to decode the result. This 
		  field is implementation specific.*/
		xdrproc_t xdr_decode_result;  

		/** @brief Handle for the Portals event queue for the long arguments. 
		  This field is implementation specific. */
		ptl_handle_eq_t args_eq_h; 

		/** @brief Handle for the Portals event queue for bulk data.  
		  This field is implementation specific. */
		ptl_handle_eq_t data_eq_h;

		/** @brief Handle handle for the Portals memory descriptor 
		  for bulk data.  This field is implementation specific. */
		ptl_handle_md_t data_md_h;

		/** @brief Handle to a Portals event queue for short 
		  results. This field is implementation specific.*/
		ptl_handle_eq_t short_res_eq_h;

	} lwfs_request;

	/** 
	 * @brief The core of the LWFS includes service
	 *        descriptions for authorization, 
	 *        authentication, and storage.
	 */
	struct lwfs_core_services {
	    lwfs_service authr_svc; 
	    lwfs_service naming_svc; 
	    int ss_num_servers; 
	    lwfs_service *storage_svc; 
	};



#if defined(__STDC__) || defined(__cplusplus)


	/* 
	 * @ingroup rpc_client_api
	 *
	 * @brief Lookup an RPC service. 
	 *
	 * The <em>\ref lwfs_lookup_service</em> function 
	 * finds a registered service by name and returns 
	 * the description of the remote service. 
	 *
	 * @param server_id @input_type The process ID of the registry server. 
	 * @param name   @input_type  The registered name of the service. 
	 * @param result @output_type If successful, points to a structure that holds a description
	 *                       of the remote service. Undefined otherwise. 
	 * @param req    @output_type Points to a data structure that holds information about
	 *                              the pending request. 
	 */
	extern int lwfs_lookup_service(
			lwfs_remote_pid server_id,
			const char *name, 
			lwfs_service *result, 
			lwfs_request *req); 

	/* 
	 * @brief Initialize an RPC client.
	 *
	 * The <em>\ref lwfs_rpc_clnt_init</em> function is a blocking call
	 * that initializes all internal data structures needed by an 
	 * LWFS RPC client. This method is called at least once for each 
	 * remote server that the client expects to use, and the implementation
	 * of this function may require a communication with that server. 
	 *
	 * @param server_id @input_type The process ID of the remote server. 
	 * @param result @output_type Points to a data structure that holds information
	 *                       about how to send RPC requests to the remote server. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that the client timed out
	 *                                       trying to communicate with the server. 
	 *
	 */
	extern int lwfs_rpc_clnt_init(
			const lwfs_remote_pid server_id, 
			lwfs_service *result);

	/** 
	 * @ingroup rpc_client_api
	 *
	 * @brief Get the service descripton from a known host. 
	 *
	 * The <tt>\ref lwfs_get_service</tt> function contacts a 
	 * remote process to fetch the service description that
	 * describes how to send and receive RPC requests to the 
	 * service. 
	 *
	 * @param server_id @input_type Identifies the host and process ID of the 
	 *                              remote service. 
	 * @param result @output_type   Points to the service description of the 
	 *                              remote service. 
	 *
	 * @return <b>\ref LWFS_OK</b>           Indicates success.
	 * @return <b>\ref LWFS_ERR_RPC</b>      Indicates a failure in the communication 
	 *                                library.
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that the client timed out 
	 *                                trying to communicate with the server. 
	 */
	extern int lwfs_get_service(
			const lwfs_remote_pid server_id,
			lwfs_service *result); 


	/** 
	 * @ingroup rpc_client_api
	 *
	 * @brief Get an array of service descriptons. 
	 *
	 * The <tt>\ref lwfs_get_services</tt> function contacts 
	 * remote processes to fetch service descriptions. 
	 *
	 * @param server_id @input_type Identifies the host and process ID of the 
	 *                              remote service. 
	 * @param result @output_type   Points to the service description of the 
	 *                              remote service. 
	 *
	 * @return <b>\ref LWFS_OK</b>           Indicates success.
	 * @return <b>\ref LWFS_ERR_RPC</b>      Indicates a failure in the communication 
	 *                                library.
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that the client timed out 
	 *                                trying to communicate with the server. 
	 */
	extern int lwfs_get_services(
			const lwfs_remote_pid *server_id,
			const int count,
			lwfs_service *result); 

	/** 
	 * @ingroup rpc_client_api
	 *
	 * @brief Load the core LWFS services
	 *
	 * The <tt>\ref lwfs_load_core_services</tt> function gets service
	 * descriptions for the authorization, naming, and storage services
	 * given host nid/pid pairs in the \ref struct lwfs_config structure.
	 *
	 * @param server_id @input_type Identifies the host and process ID of the 
	 *                              remote service. 
	 * @param result @output_type   Points to the service description of the 
	 *                              remote service. 
	 *
	 * @return <b>\ref LWFS_OK</b>           Indicates success.
	 * @return <b>\ref LWFS_ERR_RPC</b>      Indicates a failure in the communication 
	 *                                library.
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that the client timed out 
	 *                                trying to communicate with the server. 
	 */
	extern int lwfs_load_core_services(
			const struct lwfs_config *cfg,
			struct lwfs_core_services *core_svc);

	/** 
	  * @brief rpc_client_api
	  *
	  * Release data structures allocated for the core services structure. 
	  */
	extern void lwfs_core_services_free(
			struct lwfs_core_services *core_svc);



	extern int lwfs_trace_reset(
		const lwfs_service *svc,
		const int trace_enable, 
		const char *trace_fname, 
		const int trace_ftype);

	extern int lwfs_kill(
			const lwfs_service *svc);


	/**  
	 * @brief Call a remote procedure. 
	 *
	 * @ingroup rpc_client_api
	 *  
	 * The <tt>\ref lwfs_call_rpc</tt> function encodes and 
	 * transfers an RPC request to an LWFS server.  It returns
	 * an integer value that corresponds to a return code 
	 * defined in the enumerator <tt>\ref lwfs_return_code</tt>; 
	 * however, since the function does not wait for completion
	 * of the remote method, the return value only indicates 
	 * success or failure of marshaling and tranferring 
	 * the request to the server, not the success of the 
	 * remote method.  
	 *
	 * The arguments include the function-specific set of arguments 
	 * required by the remote function, optional data and result 
	 * arguments, and the <tt>\ref lwfs_request</tt> data 
	 * structure that holds all required information about the 
	 * pending request. 
	 *
	 * The ``data'' argument is reserved for functions that perform 
	 * bulk data transfers between the client and the server.  
	 * This argument  points to client-side memory buffer 
	 * that the server pulls from (e.g., for reads) or puts 
	 * into (e.g., for writes).  This buffer must remain 
	 * unmodified by the client until the remote function completes.  
	 *
	 * The ``result'' argument is reserved for functions that 
	 * expect a non-integer ``control'' response from the server.  
	 * For example, the <tt>\ref lwfs_get_cap "lwfs_get_cap()"</tt> 
	 * function defined for the \ref lwfs_authr "authorization service" 
	 * returns a list of requested capabilities.  The type of 
	 * the result argument is function specific and points to the 
	 * client-side buffer reserved for that data. At first glance, 
	 * this may seem similar to the data argument; however, there 
	 * are several important distinctions:
	 * 
	 *    -# Results are always directed from the server to the client, 
	 *       but data could flow in either direction (e.g., data for the 
	 *       <tt>\ref lwfs_write "lwfs_write()"</tt> function flows from 
	 *       client to storage server).
	 *    -# Data is typically large in comparison to the ``control'' structures 
	 *       (arguments and results). A clever implementation may transfer 
	 *       args and results over a network channel optimized for small 
	 *       messages, but transfer data over a network channel optimized 
	 *       for bulk transfers. 
	 *    -# Results are encoded in a portable binary format before transfer, 
	 *       data is transferred as ``raw'' binary data.
	 *
	 * The final argument is a pointer to the <tt>\ref lwfs_request</tt> structure that 
	 * contains all required information about the pending request, including 
	 * the \ref lwfs_request_status "status" of the pending
	 * request and the return code of the remote method (if complete). 
	 * The client calls the functions <tt>\ref lwfs_get_status "lwfs_get_status()"</tt>,
	 * <tt>\ref lwfs_test "lwfs_test()"</tt>, or <tt>\ref lwfs_wait "lwfs_wait()"</tt>
	 * to asynchronously get the status of a pending request, test for
	 * completion, or wait for completion of a request.  
	 *
	 * @param svc           @input_type Points to the data structure that describes how
	 *                             to communicate with the remote service. 
	 * @param opcode        @input_type Identifies the remote operation to execute. 
	 * @param args          @input_type Points to the unencoded arguments for the remote request. 
	 * @param data          @input_type In not null, points to memory reserved for bulk data 
	 *                             this buffer must remain available while the request 
	 *                             is pending. 
	 * @param data_len      @input_type The length of the data buffer (if not null). 
	 * @param result        @input_type Points to a memory reserved for result. This buffer must 
	 *                             remain available until the request completes. 
	 * @param req           @output_type Points to a data structure that holds information about
	 *                              the pending request. 
	 *
	* @return <b>\ref LWFS_OK</b> Indicates success of encoding and transferring the request.
		* @return <b>\ref LWFS_ERR_ENCODE</b> Indicates an error encoding the request. 
		* @return <b>\ref LWFS_ERR_RPC</b> Indicates an error in the underlying transport library
		*                                  while transferring the request. 
		* @return <b>\ref LWFS_ERR_NOTSUPP</b> Indicates an unsupported transport mechanism.
		*/
		extern int lwfs_call_rpc(
				const lwfs_service *svc, 
				const lwfs_opcode opcode, 
				void *args, 
				void *data, 
				uint32_t data_len, 
				void *result,
				lwfs_request *req); 


	/**  
	 * @brief Test for completion of an RPC request. 
	 * 
	 * @ingroup rpc_client_api
	 * 
	 * The <tt>\ref lwfs_test</tt> function checks the status of the specified
	 * request and returns <b>TRUE</b> if the status is equal to \ref LWFS_REQUEST_COMPLETE
	 * or \ref LWFS_REQUEST_ERROR . 
	 *
	 * @param req  @input_type  Points to the data structure that holds information 
	 *                     about the request. 
	 * @param rc   @output_type If the request is complete, points to the return code 
	 *                     of the completed request.  Undefined otherwise. 
	 *
	 * @return <b>TRUE</b> Indicates that the request is complete.
	 * @return <b>FALSE</b> Indicates that the request is not complete. 
	 */
	extern lwfs_bool lwfs_test(
			lwfs_request *req, 
			int *rc);

	/** 
	 * @brief Wait for a fixed amount of time for an RPC request to complete. 
	 *
	 * @ingroup rpc_client_api
	 *
	 * The <tt>\ref lwfs_timedwait</tt> function blocks for no more than 
	 * \em timeout milliseconds waiting for the remote procedure to complete.  
	 * 
	 * @param req  @input_type Points to the request structure associated with the 
	 *                    remote function. 
	 * @param timeout @input_type The maximum amount of time (milliseconds) that the 
	 *                       function will block waiting for the remote function
	 *                       to complete. 
	 * @param remote_rc   @output_type If the remote function completes, this parameter is 
	 *                     the return code of the remote method. Undefined otherwise. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates that the remote function completed 
	 *                             (possibly with an error). 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates failure in the low-level transport mechanism. 
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that the remote procedure failed to complete
	 *                                within the alloted time. 
	 */
	extern int lwfs_timedwait(
			lwfs_request *req, 
			int timeout, 
			int *remote_rc); 

	/** 
	 * @brief Wait for an RPC request to complete. 
	 *
	 * @ingroup rpc_client_api
	 *
	 * The <tt>\ref lwfs_wait</tt> function blocks until the remote procedure 
	 * associated with an RPC request completes. 
	 * 
	 * @param req  @input_type Points to the request structure associated with the 
	 *                    remote function. 
	 * @param rc   @output_type If the remote function completes, this parameter is 
	 *                     the return code of the remote method. Undefined otherwise. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates that the remote function is complete  
	 *                             (possibly with an error). 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates failure in the low-level transport mechanism. 
	 */
	extern int lwfs_wait(
			lwfs_request *req,
			int *rc); 


	/**
	 * @brief Wait for all requests to complete. 
	 *
	 * A request is not complete unless we receive the short
	 * result. 
	 *
	 * @param req_array  @input_type  The array of pending requests.
	 * @param size       @input_type  The number of pending requests.
	 * @param timeout    @input_type  The time to wait for any one request.
	 * 
	 */
	extern int lwfs_waitall(
			lwfs_request *req_array, 
			lwfs_size size, 
			int timeout);

	/** 
	 * @brief Wait for any request to complete. 
	 *
	 * @ingroup rpc_client_api
	 *
	 * The <tt>\ref lwfs_waitany</tt> function blocks for no more than 
	 * \em timeout milliseconds waiting for any one of an array of requests
	 * to complete. 
	 * 
	 * @param req_array  @input_type Points to an array of requests.
	 * @param size       @input_type The size of the request array. 
	 * @param timeout @input_type The maximum amount of time (milliseconds) that the 
	 *                       function will block waiting for a request to complete. 
	 * @param which       @output_type The index of the complete request. 
	 * @param remote_rc   @output_type The return code of the completed request. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates that a request completed
	 *                             (possibly with an error). 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates failure in the low-level transport mechanism. 
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that no request completed
	 *                                within the alloted time. 
	 */
	extern int lwfs_waitany(
			lwfs_request *req_array, 
			lwfs_size size, 
			int timeout, 
			int *which,
			int *remote_rc);

	/** 
	 * @brief Return the status of an RPC request. 
	 *
	 * @ingroup rpc_client_api
	 *
	 * The <tt>\ref lwfs_get_status</tt> function returns the status (if known)
	 * of a remote request.  This function is primarily used for 
	 * debugging. 
	 *
	 * @param req  @input_type Points to the request structure associated with the 
	 *                    remote function. 
	 * @param status @output_type The state of the pending request. 
	 * @param rc     @output_type If the status of the remote function is complete, 
	 *                     this parameter holds the return code of the remote 
	 *                     method. Undefined otherwise. 
	 *
	 * @return \ref LWFS_OK Indicates success.
	 * @return \ref LWFS_ERR_RPC Indicates failure to get the status. 
	 */
	extern int lwfs_get_status(
			lwfs_request *req,
			lwfs_request_status *status,
			int *rc); 

#else /* K&R C */
#endif



#ifdef __cplusplus
}
#endif

#endif 

