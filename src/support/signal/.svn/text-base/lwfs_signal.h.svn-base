/*-------------------------------------------------------------------------*/
/**  
 *   @file lwfs_signal.h
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 756 $
 *   $Date: 2006-06-27 14:12:27 -0600 (Tue, 27 Jun 2006) $
 *
 */

#ifndef _RPC_SIGNAL_H_
#define _RPC_SIGNAL_H_

#include "support/logger/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	/**
	 * @brief Install signal handlers.
	 */
	extern int lwfs_install_sighandler();


	/**
	 * @brief Return the exit_now variable.  If set, 
	 * it is time to exit the service. 
	 */
	extern int lwfs_exit_now(); 


	/**
	 * @brief Cleanly abort the running service.
	 *
	 * The abort function kills a running service by sending a 
	 * SIGINT signal to the running process.  If the service
	 * has already started,  the signal is caught by the 
	 * signal handler.
	 */
	extern void lwfs_abort();



#else /* K&R C */

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif 
