/*-------------------------------------------------------------------------*/
/**  
 *   @file rpc_common.h
 * 
 *   @brief Prototypes for the RPC methods used by both clients and servers.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   $Revision: 301 $
 *   $Date: 2005-03-16 00:21:00 -0700 (Wed, 16 Mar 2005) $
 */

#ifndef _LWFS_RPC_XDR_H_
#define _LWFS_RPC_XDR_H_

#include "common/types/types.h"
#include "rpc_debug.h"


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)


/**
 * @brief Initialize the XDR encoding mechanism.
 */
extern int lwfs_xdr_init(); 

/**
 * @brief Finalize the XDR encoding mechanism.
 */
extern int lwfs_xdr_fini();

/**
  * @brief Register encodings for standard service ops.
  */
extern int register_service_encodings(void);

/** 
 * @brief Register xdr encodings for an opcode.
 *
 * The <tt>\ref lwfs_register_xdr_encodings</tt> function registers
 * the xdr encodings functions associated with a particular remote
 * LWFS operation. 
 *
 * @param opcode        @input_type   The opcode to lookup.
 * @param encode_args   @input_type   The xdr function used to
 *                                    encode/decode the arguments. 
 * @param encode_data   @input_type   The xdr function used to
 *                                    encode/decode the data. 
 * @param encode_result @input_type   The xdr function used to
 *                                    encode/decode the result. 
 */
extern int lwfs_register_xdr_encoding(lwfs_opcode opcode, 
	xdrproc_t encode_args,
	xdrproc_t encode_data,
	xdrproc_t encode_result); 


/**
 * @brief Lookup the encodings for an opcode.
 *
 * The <tt>\ref lwfs_lookup_xdr_encodings</tt> function looks up
 * the registered xdr encodings for an opcode. 
 * 
 * @param opcode        @input_type   The opcode to lookup.
 * @param encode_args   @output_type  Points to the xdr function used to
 *                                    encode/decode the arguments. 
 * @param encode_data   @output_type  Points to the xdr function used to
 *                                    encode/decode the data. 
 * @param encode_result @output_type  Points to the xdr function used to
 *                                    encode/decode the result. 
 *
 * @return <b>\ref LWFS_OK</b> If successful. 
 * @return <b>\ref LWFS_ERR_NOENT</b> If the encodings for the specified 
 *                                    opcode do not exist. 
 */
int lwfs_lookup_xdr_encoding(lwfs_opcode opcode,
	xdrproc_t *encode_args,
	xdrproc_t *encode_data,
	xdrproc_t *encode_result); 


    
#else /* K&R C */
#endif



#ifdef __cplusplus
}
#endif

#endif 

