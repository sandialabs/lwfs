/**  
 *   @file authr_clnt.c
 * 
 *   @brief Implementation of the client-side methods for accessing 
 *   the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 763 $
 *   $Date: 2006-06-27 15:32:59 -0600 (Tue, 27 Jun 2006) $
 */

#include "client/rpc_client/rpc_client.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/authr_common/authr_xdr.h"
#include "authr_clnt.h"
#include "authr_args.h"
#include "authr_debug.h"
#include "hashtable/hash_funcs.h"
#include "hashtable/mt_hashtable.h"

#include "authr_srvr.h"

/* ----------- Private variables ---------------*/

/* global variable for the remote authorization service */
static lwfs_bool local_calls = FALSE; 
static lwfs_service authr_svc; 
static const lwfs_pid DEFAULT_PID = 125;

/* rpc descriptors for the API methods */
#if 0
static lwfs_rpc rpc_create_container; 
static lwfs_rpc rpc_remove_container; 
static lwfs_rpc rpc_create_acl; 
static lwfs_rpc rpc_get_acl; 
static lwfs_rpc rpc_mod_acl; 
static lwfs_rpc rpc_get_cap; 
static lwfs_rpc rpc_verify_caps;
#endif

static lwfs_bool cache_caps = FALSE; 
static int cache_size = 1024; 
static mt_hashtable cache_ht;


/* make the hashtable functions a little more type-safe */
DEFINE_MT_HASHTABLE_INSERT(insert_some, lwfs_mac, lwfs_cap);
DEFINE_MT_HASHTABLE_SEARCH(search_some, lwfs_mac, lwfs_cap);
DEFINE_MT_HASHTABLE_REMOVE(remove_some, lwfs_mac, lwfs_cap);

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

/**
 * @brief Is the opcode valid?  
 *
 * For now we return true if only one bit is set in the 
 * opcode. 
 */
 #if 0
static int count_ops(const lwfs_opcode opcode) 
{
	int nbits = sizeof(lwfs_opcode)*sizeof(opcode);
	int count; 
	int i; 
	lwfs_opcode op=(lwfs_opcode)1; 
	
	count = 0; 

	for (i=0; i<nbits; i++) {
		if (opcode & op) {
			/* the bit is set */
			count++; 
		}

		op = op << 1; 
	}

	return count; 
}
#endif


/* ----------- Auth client API ---------------*/

/**
 * @brief Bootstrap a client of the authorization service. 
 *
 * This method initializes an authorization client to use 
 * the LWFS \ref rpc_api "RPC API" to send requests. 
 *
 * @param opts @input The authorization options.
 * @param svc @input The service descriptor for the remote service.
 */
int lwfs_authr_clnt_init(
	struct authr_options *opts,
	lwfs_service *svc)
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


	if (opts->local) {

		local_calls = TRUE; 

		/* no need to cache caps */
		cache_caps = opts->cache_caps;


		/* get the process ID assigned to me */
		rc = lwfs_get_id(&opts->id); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to get process ID: %s",
					lwfs_err_str(rc));
			return rc; 
		}

		/* initialize this process as an auth srvr (also initializes svc )*/
		rc = lwfs_authr_srvr_init(opts, svc);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to initialize local svc: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}
	else {
		/* if the nid is not specified, assume this host */
		if (opts->id.nid == 0) {
			lwfs_remote_pid my_id; 

			rc = lwfs_get_id(&my_id); 
			if (rc != LWFS_OK) {
				log_error(authr_debug_level, "unable to get process ID: %s",
						lwfs_err_str(rc));
				return rc; 
			}

			opts->id.nid = my_id.nid; 
		}

		rc = lwfs_get_service(opts->id, svc); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "could not ping authr server");
			return rc; 
		}
	}

	/* are we to cache the caps */
	cache_caps = opts->cache_caps; 

	if (cache_caps) {
		rc = create_mt_hashtable(cache_size, hashfromkey, equalkeys, &cache_ht); 
		if (logging_debug(authr_debug_level)) {
			fprintf(stdout, "cache_ht == %p\n", &cache_ht);
		}
		if (rc != 1) {
			log_error(authr_debug_level, "could not create cache_ht");
			return LWFS_ERR_NOSPACE; 
		}
		rc = LWFS_OK;
	}


	if (svc != NULL) {
		/* store the authr_svc variable */
		memcpy(&authr_svc, svc, sizeof(lwfs_service));
	}

	/* client is authorization is initialized */
	init_flag = TRUE; 

	return rc; 
}


/** 
 * @brief Clean up the authorization client. 
 */
int lwfs_authr_clnt_fini() 
{
	/* if we are caching caps, we need to remove the hashtable */
	if (cache_caps) {
		log_debug(authr_debug_level, "destroying cap cache");
		mt_hashtable_destroy(&cache_ht, free);
	}
	return lwfs_rpc_fini();
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
		const lwfs_txn *txn_id,
		const lwfs_cid cid, 
		const lwfs_cap *cap,
		lwfs_cap *result,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_create_container_args args; 

	/* copy arguments to the args structure */
	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.cap = (lwfs_cap *)cap; 

	log_debug(authr_debug_level, "calling rpc");

	if (local_calls) {
		rc = create_container(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {

		/* call the remote procedure */
		rc = lwfs_call_rpc(&authr_svc, LWFS_OP_CREATE_CONTAINER, 
				&args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to call remote method");
		}
	}

	return rc; 
}

int lwfs_bcreate_container(
	const lwfs_txn *txn_id, 
	const lwfs_cid cid, 
	const lwfs_cap *cap, 
	lwfs_cap *result)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 

	/* create a container (returns a cap to modify acls) */
	rc = lwfs_create_container(txn_id, cid, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call create_container: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
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
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_cap *cap,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_remove_container_args args; 

	/* copy arguments to the args structure */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.cap = (lwfs_cap *)cap; 

	if (local_calls) {
		rc = remove_container(NULL, &args, NULL, NULL); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {
		/* call the remote procedure */
		rc = lwfs_call_rpc(&authr_svc, LWFS_OP_REMOVE_CONTAINER, 
				&args, NULL, 0, NULL, req);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to call remote method");
		}
	}

	return rc; 
}

int lwfs_bremove_container(
	const lwfs_txn *txn_id,
	const lwfs_cid cid,
	const lwfs_cap *cap)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 

	/* remove a container (returns a cap to modify acls) */
	rc = lwfs_remove_container(txn_id, cid, cap, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call remove_container: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
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
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_opcode opcode,
		const lwfs_uid_array *set,
		const lwfs_cap *cap,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_create_acl_args args; 

	log_debug(authr_debug_level, "beginning lwfs_create_acl(cid=%Lu, opcode=%u)",
			cid, opcode);

	/* copy arguments to the args structure */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.opcode = opcode;
	args.set = (lwfs_uid_array *)set; 
	args.cap = (lwfs_cap *)cap;

	if (local_calls) {
		rc = create_acl(NULL, &args, NULL, NULL); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {
		/* call the remote procedure */
		rc = lwfs_call_rpc(&authr_svc, LWFS_OP_CREATE_ACL, 
				&args, NULL, 0, NULL, req);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to call remote method");
		}
	}

	return rc; 
}


int lwfs_bcreate_acl(
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_opcode opcode,
		const lwfs_uid_array *set,
		const lwfs_cap *cap)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 
	

	log_debug(authr_debug_level, "beginning lwfs_bcreate_acl(cid=%Lu, opcode=%u)",
			cid, opcode);

	/* remove a container (returns a cap to modify acls) */
	rc = lwfs_create_acl(txn_id, cid, opcode, set, cap, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call create_acl: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
	}

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
		const lwfs_cid cid,
		const lwfs_opcode opcode,
		const lwfs_cap *cap,
		lwfs_uid_array *result,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_get_acl_args args; 

	log_debug(authr_debug_level, "beginning lwfs_get_acl(cid=%Lu, opcode=%u)",
			cid, opcode);

	/* copy arguments to the args structure */
	args.cid = cid; 
	args.opcode = opcode; 
	args.cap = (lwfs_cap *)cap;

	/* call the remote procedure */
	if (local_calls) {
		rc = get_acl(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {
		rc = lwfs_call_rpc(&authr_svc, LWFS_OP_GET_ACL, 
				&args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to call remote method");
		}
	}

	return rc; 
}

int lwfs_bget_acl(
		const lwfs_cid cid,
		const lwfs_opcode opcode,
		const lwfs_cap *cap,
		lwfs_uid_array *result)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 

	log_debug(authr_debug_level, "beginning lwfs_bget_acl(cid=%Lu, opcode=%u)",
			cid, opcode);

	/* remove a container (returns a cap to modify acls) */
	rc = lwfs_get_acl(cid, opcode, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call create_acl: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
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
 * @param result   @output the resulting set of users with access.
 * @param req   @output the request handle (used to test for completion). 
 *
 * @returns the new ACL in the \em result field. 
 */ 
int lwfs_mod_acl(
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_opcode opcode,
		const lwfs_uid_array *set,
		const lwfs_uid_array *unset,
		const lwfs_cap *cap,
		lwfs_uid_array *result,
		lwfs_request *req) 
{
	int rc = LWFS_OK; 
	lwfs_mod_acl_args args; 

	/* copy arguments to the args structure */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid = cid; 
	args.opcode = opcode; 
	args.set = (lwfs_uid_array *)set; 
	args.unset = (lwfs_uid_array *)unset; 
	args.cap = (lwfs_cap *)cap; 

	if (local_calls) {
		rc = mod_acl(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {
		/* call the remote procedure */
		rc = lwfs_call_rpc(&authr_svc, LWFS_OP_MOD_ACL, 
				&args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to call remote method");
		}
	}

	return rc; 
}


int lwfs_bmod_acl(
		const lwfs_txn *txn_id,
		const lwfs_cid cid,
		const lwfs_opcode opcode,
		const lwfs_uid_array *set,
		const lwfs_uid_array *unset,
		const lwfs_cap *cap,
		lwfs_uid_array *result)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 

	/* remove a container (returns a cap to modify acls) */
	rc = lwfs_mod_acl(txn_id, cid, opcode, set, unset, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call lwfs_mod_acl: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
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
 * @param opcodes @input  the opcodes to request (e.g., LWFS_CID_READ | LWFS_CID_WRITE)
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
		const lwfs_cid cid,
		const lwfs_opcode opcodes,
		const lwfs_cred *cred,
		lwfs_cap *result,
		lwfs_request *req)
{
	int rc = LWFS_OK; 
	lwfs_get_cap_args args; 

	log_debug(authr_debug_level, "beginning lwfs_get_cap(cid=%Lu, opcode=%u)",
			cid, opcodes);

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_cap_array));

	/* copy arguments to the args structure */
	args.cid = cid; 
	args.opcodes = opcodes; 
	args.cred = (lwfs_cred *)cred; 

	/* call the remote procedure */
	if (local_calls) {
		rc = get_cap(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {
		rc = lwfs_call_rpc(&authr_svc, LWFS_OP_GET_CAPS, 
				&args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "unable to call remote method");
		}
	}

	return rc; 
}

int lwfs_bget_cap(
		const lwfs_cid cid,
		const lwfs_opcode opcodes,
		const lwfs_cred *cred,
		lwfs_cap *result)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 

	log_debug(authr_debug_level, "beginning lwfs_bget_cap(cid=%Lu, opcode=%u)",
			cid, opcodes);

	/* remove a container (returns a cap to modify acls) */
	rc = lwfs_get_cap(cid, opcodes, cred, result, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call lwfs_get_cap: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
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
		const lwfs_cap *caps,
		const int num_caps, 
		lwfs_request *req) 
{
	int rc = LWFS_OK; 
	int i;
	lwfs_verify_caps_args args; 
	lwfs_cap_array cap_array; 

	cap_array.lwfs_cap_array_len = num_caps; 
	cap_array.lwfs_cap_array_val = (lwfs_cap *)caps; 

	/* copy arguments to the args structure */
	args.cap_array = &cap_array; 

	/* call the remote procedure */
	if (local_calls) {
		rc = verify_caps(NULL, &args, NULL, NULL); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "error: %s",
				lwfs_err_str(rc));
			req->status = LWFS_REQUEST_ERROR;
			req->error_code = rc; 
			rc = LWFS_OK; 
		}
		else {
			req->status = LWFS_REQUEST_COMPLETE; 
			req->error_code = LWFS_OK; 
		}
	}
	else {
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
					found = FALSE; 
				}

				/* verify that the two caps are the same? */
				else {
					if (0!=memcmp(&caps[i].data, &cap->data, sizeof(lwfs_cap_data))) {
						log_warn(authr_debug_level, 
								"cap in cache does not match");
						//return LWFS_ERR_VERIFYCAP;
					}
				}

			}

			/* if all caps were not found, call the remote method */
			if (!found) {
				int rc2 = LWFS_OK; 

				/* call the remote method */
				rc = lwfs_call_rpc(&authr_svc, LWFS_OP_VERIFY_CAPS, 
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
			rc = lwfs_call_rpc(&authr_svc, LWFS_OP_VERIFY_CAPS, 
					&args, NULL, 0, NULL, req);
			if (rc != LWFS_OK) {
				log_error(authr_debug_level, "unable to call remote method");
			}
		}
	}

	return rc; 
}

int lwfs_bverify_caps(
		const lwfs_cap *caps, const int num_caps)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	lwfs_request req; 

	/* remove a container (returns a cap to modify acls) */
	rc = lwfs_verify_caps(caps, num_caps, &req); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not call lwfs_get_cap: %s",
				lwfs_err_str(rc));
		return rc;
	}

	rc2 = lwfs_wait(&req, &rc); 
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_error(authr_debug_level, "error in remote method: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK) ? rc : rc2; 
	}

	return rc;
}

