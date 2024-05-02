/**  
 *   @file authr_clnt.c
 * 
 *   @brief Implementation of the client-side methods for 
 *   accessing the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1458 $
 *   $Date: 2007-06-06 11:43:56 -0600 (Wed, 06 Jun 2007) $
 */
#include "config.h"

#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

/*#include "portals3.h"*/

#include "support/hashtable/hash_funcs.h"
#include "support/hashtable/hashtable.h"

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/authr_common/authr_xdr.h"
#include "common/authr_common/authr_args.h"
#include "common/authr_common/authr_debug.h"

#include "client/rpc_client/rpc_client.h"

#include "authr_client.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/* ----------- Private variables ---------------*/

static const lwfs_pid DEFAULT_PID = 125;


/* flags for overhead testing */
static volatile lwfs_bool _verify_caps = TRUE; 
static volatile lwfs_bool cache_caps = FALSE; 
static int cache_size = 1024; 
static struct hashtable cache_ht;


/* make the hashtable functions a little more type-safe */
DEFINE_HASHTABLE_INSERT(insert_some, lwfs_mac, lwfs_cap);
DEFINE_HASHTABLE_SEARCH(search_some, lwfs_mac, lwfs_cap);
DEFINE_HASHTABLE_REMOVE(remove_some, lwfs_mac, lwfs_cap);

static unsigned int 
hashfromkey(void *key)
{
	//lwfs_mac *mac = (lwfs_mac *)key;

	/* hash an LWFS mac using a standard char * hash func */
	return RSHash(key, sizeof(lwfs_mac)); 
}

static int
equalkeys(void *k1, void *k2)
{
	lwfs_mac *mac1 = (lwfs_mac *)k1;
	lwfs_mac *mac2 = (lwfs_mac *)k2; 

	/* macs are only equal if they are byte-wise identical */
	return (0 == memcmp(mac1, mac2, sizeof(lwfs_mac)));
}


/* ----------- Private methods ---------------*/


/* ----------- Auth client API ---------------*/


static int client_init( )
{
	static lwfs_bool init_flag = FALSE;
	int rc = LWFS_OK; 

	if (init_flag) {
		return rc; 
	}

	/* initialize the RPC library */
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

	/* register the message encoding scheme for the authr service */
	register_authr_encodings();


	/* client is authorization is initialized */
	init_flag = TRUE; 

	return rc; 
}

int set_verify_caps_flag(const lwfs_bool flag)
{
	_verify_caps = flag; 
	return LWFS_OK;
}

lwfs_bool get_verify_caps_flag()
{
	return _verify_caps; 
}



/**
 * @brief Cache caps on the client. 
 */
int lwfs_cache_caps_init() 
{
	int rc = LWFS_OK; 

	client_init();

	if (!cache_caps) {

		cache_caps = TRUE;

		if (cache_caps) {
			rc = create_hashtable(cache_size, hashfromkey, equalkeys, &cache_ht); 
			log_debug(authr_debug_level, "cache_ht == %p", &cache_ht);
			if (rc != 1) {
				log_error(authr_debug_level, "could not create cache_ht");
				rc = LWFS_ERR_NOSPACE; 
			}
			else {
				rc = LWFS_OK;
			}
		}
	}

	return rc;
}

int lwfs_cache_caps_fini() 
{
	int rc = LWFS_OK;

	/* if we are caching caps, we need to remove the hashtable */
	if (cache_caps) {
		log_debug(authr_debug_level, "destroying cap cache");
		hashtable_destroy(&cache_ht, free);
		cache_caps = FALSE;
	}

	return rc; 
}

/**
 * @brief Create a new container on the authorization server.
 *
 * The \ref lwfs_create_container method generates a unique container
 * ID on the authorization server and returns a capability that enables
 * the caller to modify the ACLs associated with the container.  
 *
 * @param txn_id @input transaction id.
 * @param cid         @input  the container ID to use.
 * @param cap         @input  capability that allows the client to create a cid.
 * @param result      @output a new container ID.
 * @param req         @output request handle (used to test for completion).
 */
int lwfs_create_container(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid, 
		const lwfs_cap *cap,
		lwfs_cap *result,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_create_container_args args; 

	/* initialize client */
	client_init();

	/* copy arguments to the args structure */
	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.cap = (lwfs_cap *)cap; 

	log_debug(authr_debug_level, "calling rpc");


	/* call the remote procedure */
	rc = lwfs_call_rpc(authr_svc, LWFS_OP_CREATE_CONTAINER, 
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to call remote method");
	}

	return rc; 
}


/**
 * @brief Remove a container.
 *
 * The \b lwfs_remove_container method removes an 
 * existing container ID from the authorization server. 
 *
 * @note This operation does not check for "orphan" entities (i.e., 
 * existing objects, namespace entries, and so forth) before removing
 * the container ID. We leave this responsibility to the client. 
 *
 * @param txn_id @input transaction id.
 * @param cid  @input container ID to remove.
 * @param cap  @input capability that allows the client to remove a cid.
 * @param req  @output request handle (used to test for completion).
 */
int lwfs_remove_container(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_cap *cap,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_remove_container_args args; 

	/* initialize client */
	client_init();

	/* copy arguments to the args structure */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.cap = (lwfs_cap *)cap; 

	/* call the remote procedure */
	rc = lwfs_call_rpc(authr_svc, LWFS_OP_REMOVE_CONTAINER, 
			&args, NULL, 0, NULL, req);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to call remote method");
	}

	return rc; 
}


/** 
 * @brief Create an ACL for a container/op pair. 
 * 
 * The \b lwfs_create_acl method creates a new access-control
 * list associated with a particular operation. 
 *
 * @param txn_id @input transaction id.
 * @param cid     @input the container id.
 * @param op      @input the operation (e.g., read, write, ...).
 * @param set     @input the list of users that want access.
 * @param result  @input capability that allows the client to create an acl.
 * @param req     @output request handle (used to test for completion). 
 */
int lwfs_create_acl(
		const lwfs_service *authr_svc, 
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_uid_array *set,
		const lwfs_cap *cap,
		lwfs_request *req)
{
	int rc = LWFS_OK; 

	/* initialize client */
	client_init();

	log_debug(authr_debug_level, "beginning lwfs_create_acl(cid=%Lu, container_op=%u)",
			cid, container_op);

	rc = lwfs_mod_acl(authr_svc, txn_id, cid, 
			container_op, set, NULL, cap, req);

	return rc; 
}



/**
 * @brief Get the access-control list for an 
 *        container/op pair.
 *
 * The \b lwfs_get_acl method returns a list of users that have 
 * a particular type of access to a container. 
 *
 * @param cid  @input the container id.
 * @param op   @input  the operation (i.e., LWFS_OP_READ, LWFS_OP_WRITE, ...)
 * @param cap  @input  the capability that allows us to get the ACL.
 * @param result  @output the list of users with access. 
 * @param req  @output the request handle (used to test for completion). 
 *
 * @returns an ACL in the \em result field. 
 *
 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
 *         id for this method because getting the access-control list 
 *         does not change the state of the system. 
 */
int lwfs_get_acl(
		const lwfs_service *authr_svc,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_cap *cap,
		lwfs_uid_array *result,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_get_acl_args args; 

	/* initialize client */
	client_init();

	log_debug(authr_debug_level, "beginning lwfs_get_acl(cid=%Lu, container_op=%u)",
			cid, container_op);

	/* copy arguments to the args structure */
	args.cid = cid; 
	args.container_op = container_op; 
	args.cap = (lwfs_cap *)cap;

	/* call the remote procedure */
	rc = lwfs_call_rpc(authr_svc, LWFS_OP_GET_ACL, 
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to call remote method");
	}

	return rc; 
}




/**
 * @brief Modify the access-control list for an container/op pair.
 *
 * This method changes the access-control list for a particular
 * operation on an container of objects.  The arguments 
 * include a list of users that require access (set) and 
 * a list of users that no longer require access (unset).  
 * To resolve conflicts that occur when a user ID appears in both
 * lists, the implementation first grants access to the users 
 * in the set list, then removes access from users in the unset list. 
 *
 * @param txn_id @input transaction id.
 * @param cid   @input the container id.
 * @param op    @input the operation (e.g., read, write, ...).
 * @param set   @input the list of users that want access.
 * @param unset @input the list of users that do not get access.
 * @param cap   @input the capability that allows the user to change the acl.
 * @param req   @output the request handle (used to test for completion). 
 *
 * @returns the new ACL in the \em result field. 
 */ 
int lwfs_mod_acl(
		const lwfs_service *authr_svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_uid_array *set,
		const lwfs_uid_array *unset,
		const lwfs_cap *cap,
		lwfs_request *req) 
{
	int rc = LWFS_OK; 
	lwfs_mod_acl_args args; 

	/* initialize client */
	client_init();

	/* copy arguments to the args structure */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.container_op = container_op; 
	args.set = (lwfs_uid_array *)set; 
	args.unset = (lwfs_uid_array *)unset; 
	args.cap = (lwfs_cap *)cap; 

	/* call the remote procedure */
	rc = lwfs_call_rpc(authr_svc, LWFS_OP_MOD_ACL, 
			&args, NULL, 0, NULL, req);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to call remote method");
	}

	return rc; 
}



/**
 *  @brief Get a capability for a container. 
 *
 *  This method allocates a list of capabilities for users 
 *  that have valid credentails and have appropriate permissions 
 *  (i.e., their user ID is in the appropriate ACL) to access 
 *  the container.  
 * 
 * @param cid     @input  the container id.
 * @param container_op @input  the container_op to request 
 *        (e.g., LWFS_CONTAINER_READ | LWFS_CONTAINER_WRITE)
 * @param cred   @input  credential of the user. 
 * @param result @output the resulting list of caps.
 * @param req    @output the request handle (used to test for completion). 
 *
 * @returns the list of caps in the \em result field. 
 * 
 *  @note This method does not return partial results. If the 
 *  authorization service fails to grant all requested capabilities,
 *  \b lwfs_getcaps returns NULL for the resulting list of caps and 
 *  reports the error in the return code embedded in the request handle. 
 *
 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
 *         id for this method because getting the access-control list 
 *         does not change the state of the system. 
 */
int lwfs_get_cap(
		const lwfs_service *authr_svc,
		const lwfs_cid cid,
		const lwfs_container_op container_op,
		const lwfs_cred *cred,
		lwfs_cap *result,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_get_cap_args args; 

	/* initialize client */
	client_init();

	log_debug(authr_debug_level, "beginning lwfs_get_cap(cid=%Lu, container_op=%u)",
			cid, container_op);

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_cap_array));

	/* copy arguments to the args structure */
	args.cid = cid; 
	args.container_op = container_op; 
	args.cred = (lwfs_cred *)cred; 

	/* call the remote procedure */
	rc = lwfs_call_rpc(authr_svc, LWFS_OP_GET_CAP, 
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to call remote method");
	}

	return rc; 
}



/**
 *  @brief Verify a list of capabilities. 
 *
 *  This method verifies that the provided capabilities were 
 *  generated by this authorization server and that they have
 *  not been modified. If all of the caps are valid, this method 
 *  also registers the requesting process (parameter \em pid) 
 *  as a user of the caps.  The authorization server
 *  uses the information in registration table to decide
 *  who to contact when revoking access to capabilities. 
 * 
 * @param caps    @input  the array of capabilities to verify
 * @param n      @input  the number of caps in the array. 
 * @param req    @output the request handle (used to test for completion). 
 *
 * @returns The remote method returns \ref LWFS_OK if all the 
 * caps are valid. 
 */
int lwfs_verify_caps(
		const lwfs_service *authr_svc,
		const lwfs_cap *caps,
		const int num_caps, 
		lwfs_request *req) 
{
    int rc = LWFS_OK; 
    int i;
    lwfs_verify_caps_args args; 
    lwfs_cap_array cap_array; 

    /* initialize client */
    client_init();

    log_debug(authr_debug_level, "entering lwfs_verify_caps... cache_caps=%d",
	    cache_caps);

    /* return if the client is not supposed to verify caps */
    if (!_verify_caps) {
	return rc;
    }

    cap_array.lwfs_cap_array_len = num_caps; 
    cap_array.lwfs_cap_array_val = (lwfs_cap *)caps; 

    /* copy arguments to the args structure */
    args.cap_array = &cap_array; 

    /* If we are caching caps, we need to check the local cache. 
     * If the local cache does not have the cap, we verify the cap
     * store it in the cache, and register ourself as a trusted 
     * verifier of the cap. */
    if (cache_caps) {
	lwfs_bool found=TRUE; 
	lwfs_cap *cap = NULL; 

	/* look in the cache for the capability */
	for (i=0; (i<num_caps) && found; i++) {
	    cap = search_some(&cache_ht, (lwfs_mac *)&caps[i].mac);

	    if (cap == NULL) {
		log_debug(authr_debug_level, "cap[%d] not found in cache\n", i);
	    	/* at least one cap not found in the cache, verify them all */
		found = FALSE;
		break;
	    }
	    /* verify that the two caps are the same? */
	    else {
		if (0!=memcmp(&caps[i].data, &cap->data, sizeof(lwfs_cap_data))) {
		    log_warn(authr_debug_level, "cap[%d] found in hash, but full compare failed", i);
		    /* cap may have been tampered with, reverify */
		    found = FALSE;
		    break;
		}
	    }

	    log_debug(authr_debug_level, "cap[%d] found in cache\n", i);

	}

	/* if all caps were not found, call the remote method */
	if (!found) {
	    int rc2 = LWFS_OK; 

	    log_debug(authr_debug_level, "at least one cap not found in cache\n");

	    /* call the remote method */
	    rc = lwfs_call_rpc(authr_svc, LWFS_OP_VERIFY_CAPS, 
		    &args, NULL, 0, NULL, req);
	    if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to call remote method");
	    }

	    /* wait for result */
	    rc2 = lwfs_wait(req, &rc); 
	    if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
			(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
	    }

	    /* since there are no errors, cache the caps */
	    for (i=0; i<num_caps; i++) {
		lwfs_cap *new_cap = malloc(sizeof(lwfs_cap));
		if (new_cap == NULL) {
		    return LWFS_ERR_VERIFYCAP;
		}						
		lwfs_mac *new_mac = malloc(sizeof(lwfs_mac));
		if (new_mac == NULL) {
		    return LWFS_ERR_VERIFYCAP;
		}						
		memcpy(new_cap, &caps[i], sizeof(lwfs_cap));
		memcpy(new_mac, (lwfs_mac *)&caps[i].mac, sizeof(lwfs_mac));
		if (!insert_some(&cache_ht, 
			    new_mac,
			    new_cap)) {
		    log_error(authr_debug_level, 
			    "could not insert into cache");
		    return LWFS_ERR_VERIFYCAP;
		}
	    }
	}

	/* if all caps were found in the cache, the request is complete */
	else {
	    req->status = LWFS_REQUEST_COMPLETE;
	    req->error_code = LWFS_OK;
	}
    }

    else {
	/* call the remote method */
	rc = lwfs_call_rpc(authr_svc, LWFS_OP_VERIFY_CAPS, 
		&args, NULL, 0, NULL, req);
	if (rc != LWFS_OK) {
	    log_error(authr_debug_level, "unable to call remote method");
	}
    }

    return rc; 
}
