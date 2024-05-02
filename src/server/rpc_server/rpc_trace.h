/*-------------------------------------------------------------------------*/
/**  
 *   @file rpc_trace.h
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 231 $
 *   $Date: 2005-01-13 00:28:37 -0700 (Thu, 13 Jan 2005) $
 *
 */

#ifndef _RPC_TRACING_H_
#define _RPC_TRACING_H_

#include "logger/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern int rpc_tracing_level;

	enum RPC_TRACING_IDS {
		TRACE_RPC_START_SERVICE = 1000,
		TRACE_RPC_FETCH_ARGS,
		TRACE_RPC_PROCESS_REQ,
		TRACE_RPC_GET_DATA,
		TRACE_RPC_PUT_DATA,
		TRACE_RPC_SEND_RES,
		TRACE_RPC_FETCH_RES,
		TRACE_RPC_REQ_PROCESSING,
		TRACE_RPC_REQ_PENDING,
		TRACE_RPC_REQ_TOTAL,
		TRACE_PTL_POLL_COUNT,
	};

#if defined(__STDC__) || defined(__cplusplus)

#else /* K&R C */

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif 
