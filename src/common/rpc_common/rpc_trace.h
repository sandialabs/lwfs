/*-------------------------------------------------------------------------*/
/**  
 *   @file rpc_trace.h
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 231 $
 *   $Date: 2005-01-13 00:28:37 -0700 (Thu, 13 Jan 2005) $
 *
 */

#ifndef _RPC_TRACE_H_
#define _RPC_TRACE_H_

#include "support/logger/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern int ss_trace_level;

	enum RPC_TRACE_IDS {
	    TRACE_RPC_IDLE = 1,
	    TRACE_RPC_PROC,
	    TRACE_RPC_SENDRES
	};

#if defined(__STDC__) || defined(__cplusplus)

#else /* K&R C */

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif 
