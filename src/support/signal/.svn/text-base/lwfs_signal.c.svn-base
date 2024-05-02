/*-------------------------------------------------------------------------*/
/**  @file lwfs_signal.c
 *   
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 560 $
 *   $Date: 2006-02-28 14:02:02 -0700 (Tue, 28 Feb 2006) $
 *
 */

#include <stdlib.h>
#include <signal.h>

#include "support/logger/logger.h"
#include "lwfs_signal.h"


/* local variables */
static int volatile _exit_now = 0;

/* --------------------- Private methods ------------------- */

static void sighandler(int sig)
{
    log_warn(LOG_UNDEFINED, "Caught signal %d, setting exit_now flag", sig);
    _exit_now = 1; 
}


/**
 * @brief Install signal handlers.
 */
int lwfs_install_sighandler() 
{
	struct sigaction new_action, old_action;

	new_action.sa_handler = sighandler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction (SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN) {
		sigaction (SIGINT, &new_action, NULL);
	}
	sigaction (SIGHUP, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN) {
		sigaction (SIGHUP, &new_action, NULL);
	}
	sigaction (SIGTERM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN) {
		sigaction (SIGTERM, &new_action, NULL);
	}
	sigaction (SIGABRT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN) {
		sigaction (SIGABRT, &new_action, NULL);
	}
	return 0;
}


/**
 * @brief Return the exit_now variable.  If set, 
 * it is time to exit the service. 
 */
int lwfs_exit_now() {
	return _exit_now; 
}


/**
 * @brief Cleanly abort the running service.
 *
 * The abort function kills a running service by sending a 
 * SIGINT signal to the running process.  If the service
 * has already started,  the signal is caught by the 
 * signal handler.
 */
void lwfs_abort() 
{
    /* kill(0,SIGINT) */
    log_warn(LOG_UNDEFINED, "Received abort()... setting exit_now flag");
    _exit_now = 1; 
}


