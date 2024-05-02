/*-------------------------------------------------------------------------*/
/**  @file service_args.x
 *   
 *   @brief XDR definitions for the argument structures for 
 *   \ref generic service operations. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 969 $.
 *   $Date: 2006-08-28 15:40:44 -0600 (Mon, 28 Aug 2006) $.
 *
 */

#ifdef RPC_HDR
%#include "common/types/types.h"
#endif


/**
 * @brief Arguments for the \ref lwfs_create_container method that 
 * have to be passed to the authorization server. 
 */
struct lwfs_get_service_args {

    /** @brief The ID of the service (unused). */
    int id; 
};

/**
 * @brief Arguments for the \ref lwfs_create_container method that 
 * have to be passed to the authorization server. 
 */
struct lwfs_kill_service_args {

    /** @brief The ID of the service (unused). */
    int id; 
};


/**
 * @brief Arguments for the \ref lwfs_reset_tracing method. 
 */
struct lwfs_trace_reset_args {

    /** @brief The file type for the new tracing file. */
    int ftype; 

    /** @brief The name of the new tracing file. */
    string fname<256>; 

    /** @brief Flag to disable(0), enable(1), or no-change (-1) */
    int enable_flag; 
};

