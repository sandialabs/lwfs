/*-------------------------------------------------------------------------*/
/**  @file rpc_xdr.c
 *   
 *   @brief  XDR-encoding support for the 
 *           \ref rpc_client_api "RPC Client API". 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 481 $
 *   $Date: 2005-11-03 16:21:36 -0700 (Thu, 03 Nov 2005) $
 *
 */

#include <stdio.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <signal.h>

#if defined(__APPLE__)
#include <stdlib.h>  /* find malloc()/free() */
#include <string.h> /* find memset() */
#endif

#include "common/types/types.h"
#include "support/logger/logger.h"
#include "support/hashtable/hashtable.h"
#include "support/hashtable/hash_funcs.h"

#include "rpc_debug.h"
#include "rpc_xdr.h"
#include "rpc_opcodes.h"
#include "service_args.h"

/** 
 * @brief XDR encodings for LWFS remote operations. 
 *
 * The <tt>\ref xdr_encodings</tt> structure contains the 
 * xdr-encoding function pointers required for a remote 
 * LWFS function. 
 */
struct xdr_encodings {
	/** @brief A function pointer to encode/decode the arguments. */
	xdrproc_t encode_args;
	/** @brief A function pointer to encode/decode the data. */
	xdrproc_t encode_data;
	/** @brief A function pointer to encode/decode the result. */
	xdrproc_t encode_result;
};

/**
 * @brief A hashtable for XDR encodings of LWFS remote functions.
 *
 * The <tt>\ref encodings_ht</tt> hashtable is a thread-safe global 
 * data structure that holds encodings of registered LWFS functions. 
 * It is used by both clients and servers. 
 */
static struct hashtable encodings_ht; 

/* --------------------- Private functions ------------------- */

static unsigned int encoding_hash_func(void *data)
{
	return (RSHash((char *)data, sizeof(lwfs_opcode)));
}

static int compare_opcodes(void *a, void *b)
{
	lwfs_opcode opcode_a = *((lwfs_opcode *)a); 
	lwfs_opcode opcode_b = *((lwfs_opcode *)b); 

	return opcode_a == opcode_b; 
}

/* --------------------- Public functions ------------------- */

/**
 * @brief Initialize the XDR encoding mechanism.
 */
int lwfs_xdr_init() 
{
	int rc; 
	static lwfs_bool initialized = FALSE; 

	if (initialized) return LWFS_OK;

	/* check to see if logging is enabled */
	if (logger_not_initialized()) {
		logger_set_file(stderr);
	}
	
	/* initialize the encodings hash table */
	if (!create_hashtable(20, encoding_hash_func, compare_opcodes, &encodings_ht)) {
		log_error(rpc_debug_level, "failed to create a new hashtable");
		rc = LWFS_ERR; 
	}

	initialized = TRUE;

	return LWFS_OK; 
}

int register_service_encodings(void)
{
    int rc = LWFS_OK;

    /* get service */
    lwfs_register_xdr_encoding(LWFS_OP_GET_SERVICE,
	    (xdrproc_t)NULL,
	    (xdrproc_t)NULL,
	    (xdrproc_t)&xdr_lwfs_service);

    /* kill service */
    lwfs_register_xdr_encoding(LWFS_OP_KILL_SERVICE,
	    (xdrproc_t)NULL,
	    (xdrproc_t)NULL,
	    (xdrproc_t)xdr_void);

    /* trace reset */
    lwfs_register_xdr_encoding(LWFS_OP_TRACE_RESET,
	    (xdrproc_t)&xdr_lwfs_trace_reset_args,
	    (xdrproc_t)NULL,
	    (xdrproc_t)xdr_void);

    return rc;
}

/**
 * @brief Finalize the XDR encoding mechanism.
 */
int lwfs_xdr_fini() 
{
	int rc = LWFS_OK;

	lwfs_xdr_init();

	/* destroy the hashtable */
	hashtable_destroy(&encodings_ht, free); 

	return rc; 
}


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
int lwfs_register_xdr_encoding(lwfs_opcode opcode, 
	xdrproc_t encode_args,
	xdrproc_t encode_data,
	xdrproc_t encode_result)
{
	int rc = LWFS_OK;
	struct xdr_encodings *encodings = NULL;
	lwfs_opcode *key = (lwfs_opcode *)malloc(sizeof(lwfs_opcode));
	*key = opcode;

	lwfs_xdr_init();

	log_debug(rpc_debug_level,"REGISTERING OPCODE=%d",opcode);
	
	encodings = (struct xdr_encodings *)malloc(sizeof(struct xdr_encodings)); 
	if (encodings == NULL) {
		log_error(rpc_debug_level, "could not allocate encodings structure");
		return LWFS_ERR_NOSPACE;
	}

	/* initialize the memory */
	memset(encodings, 0, sizeof(struct xdr_encodings));
	encodings->encode_args = encode_args;
	encodings->encode_data = encode_data;
	encodings->encode_result = encode_result;

	/* insert the structure into the hashtable */
	if (!hashtable_insert(&encodings_ht, key, encodings)) {
		log_error(rpc_debug_level, 
				"could not insert encodings structure "
				"into hashtable for opcode %d", opcode);
		rc = LWFS_ERR; 
	}

	return rc; 
}


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
	xdrproc_t *encode_result)
{
	int rc = LWFS_OK;
	struct xdr_encodings *encodings = NULL; 

	lwfs_xdr_init();

	encodings = hashtable_search(&encodings_ht, &opcode);

	if (encodings == NULL) {
		log_warn(rpc_debug_level, "could not find encodings for opcode=%d",opcode);
		return LWFS_ERR_NOENT;
	}

	*encode_args = encodings->encode_args;
	*encode_data = encodings->encode_data;
	*encode_result = encodings->encode_result;

	return rc; 
}

