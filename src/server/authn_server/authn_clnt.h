/**  
 *   @file authn_clnt.h
 * 
 *   @brief Client side functions for accessing the authentication service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 476 $
 *   $Date: 2005-11-03 16:09:52 -0700 (Thu, 03 Nov 2005) $
 */

#include "types/types.h"
#include "rpc/rpc_clnt.h"

#ifndef _LWFS_AUTHENTICATION_H_
#define _LWFS_AUTHENTICATION_H_


#ifdef __cplusplus
extern "C" {
#endif


#if defined(__STDC__) || defined(__cplusplus)

	/**
	 *   @addtogroup authn_api
	 *
	 *   The functions in the LWFS authentication specification 
	 *   provide the mechanisms that allow a client 
	 *   to acquire a transferrable credential from an 
	 *   authentication service. 
     *
	 *   @remark We need to add a paragraph that illustrates how a client 
	 *           will use this API. 
	 *
	 */   

	 /**
	 * @addtogroup authn_api
	 *
	 *  @latexonly \input{generated/structlwfs__cred} @endlatexonly
	 */


	/**
	 * @brief Initialize the authentication client.
	 *
	 *
	 * The <em>\ref lwfs_authn_clnt_init</em> function is a blocking call 
	 * that performs initialization steps required by clients of the 
	 * authentication service. This function must be called before
	 * any of the functions in the \ref authn_api "authorization API".
	 *
	 * @param opts  @input_type Points to an implementation-specific structure 
	 *                     that holds information about how to initialize a 
	 *                     client of an authorization service. 
	 * @param svc   @output_type If not null, points to a structure that 
	 *                     describes how to connect to the authorization
	 *                     service. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 * @return <b>\ref LWFS_ERR_TIMEDOUT</b> Indicates that the client timed out
	 *                                       trying to communicate with the server.
	 */ 
	extern int lwfs_authn_clnt_init(
			const struct authn_options *opts,
			lwfs_service *svc); 

	/**
	 * @brief Finish the authentication client.
	 *
	 *
	 * The <tt>\ref lwfs_authn_clnt_fini </tt> function 
	 * cleans up internal data structures created for a 
	 * client of the authentication service.
	 *
	 * @param svc @input_type Points to a structure that describes how to
	 *                        communicate with the authentication service. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */ 
	extern int lwfs_authn_clnt_fini(const lwfs_service *svc); 

	/**
	 *  @brief Get the callers credential.
	 *
	 *  @ingroup authn_api
	 *
	 *  The <tt>\ref lwfs_get_cred</tt> function returns a transferrable 
	 *  credential that represents the identity of the caller. 
	 *
	 * @param svc  @input_type  Points to the structure that describes how
	 *                          to communicate with the authentication svc. 
	 * @param cred  @output_type  If successful, points to the 
	 *                       credential structure provided by the 
	 *                       authentication service. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR</b> Indicates failure. 
	 */
	extern int lwfs_get_cred(
			const lwfs_service *svc, 
			lwfs_cred *result);


#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

