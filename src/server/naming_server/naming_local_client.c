/**  
 *   @file naming_clnt.c
 * 
 *   @brief Implementation of the client-side API for the LWFS naming service.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   @version $Revision: 791 $
 *   @date $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */


#include "naming_client.h"

/* define the global root dir */
lwfs_ns_entry NAMING_ROOT; 
const lwfs_ns_entry *LWFS_NAMING_ROOT = &NAMING_ROOT;



static lwfs_service naming_svc; 



/** 
 * @brief Return the container ID of an entry. 
 *
 * The \b lwfs_get_cid method returns the 
 * \ref lwfs_cid "container ID" of an entry. 
 *
 * @param entry @input the entry.
 */
lwfs_cid lwfs_get_cid(
		const lwfs_obj *obj)
{
	return obj->cid; 
}

/** 
 * @brief Return the entry type.
 *
 * The \b lwfs_get_etype method returns the 
 * \ref lwfs_obj_type "type" of an entry. 
 *
 * @param entry @input the entry.
 */
int lwfs_get_type(
		const lwfs_obj *obj)
{
	return obj->type; 
}



int lwfs_naming_clnt_init(
	struct naming_options *naming_opts, 
	struct authr_options *authr_opts,
	lwfs_service *authr_svc, 
	lwfs_service *svc)
{
	static const lwfs_ptl_pid DEFAULT_PID = 121; 
	lwfs_process_id my_id; 

	static volatile lwfs_bool init_flag = FALSE; 
	int rc = LWFS_OK;

	if (init_flag) {
		return rc; 
	}

	/* register encoding schemes for the naming operations */
	rc = register_naming_encodings();
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to register encoding schemes: %s",
				lwfs_err_str(rc));
		return rc; 


	/* initialize the rpc library */
	rc = lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to init rpc client: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	lwfs_get_id(&my_id); 

	if (naming_opts->local) {
		local_calls = TRUE;

		/* initialize the local service */
		rc = lwfs_naming_srvr_init(naming_opts, authr_opts, authr_svc, svc);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to init local srvr: %s",
					lwfs_err_str(rc)); 
			return rc; 
		}
	}

	else {
		local_calls = FALSE; 

		/* if the nid is not specified, assume this host */
		if (naming_opts->id.nid == 0) {
			lwfs_process_id my_id; 

			rc = lwfs_get_my_pid(&my_id); 
			if (rc != LWFS_OK) {
				log_error(naming_debug_level, "unable to get process ID: %s",
						lwfs_err_str(rc));
				return rc; 
			}

			naming_opts->id.nid = my_id.nid; 
		}

		rc = lwfs_rpc_ping(naming_opts->id, svc); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not ping authr server");
			return rc; 
		}
	}

	if (svc != NULL) {
		/* store the naming svc descriptor */
		memcpy(&naming_svc, svc, sizeof(lwfs_service));

		/* initialize the rpc descriptors for the methods */
		init_rpc_desc(&naming_svc);
	}

	/* initialize the root directory pointer */
	memset(&NAMING_ROOT, 0, sizeof(lwfs_obj));
	strncpy(NAMING_ROOT.name, "/", LWFS_NAME_LEN); 
	memcpy(&NAMING_ROOT.entry_obj.svc, &naming_svc, sizeof(lwfs_service));
	NAMING_ROOT.entry_obj.type = LWFS_DIR_ENTRY; 
	NAMING_ROOT.entry_obj.oid = 1; 

	init_flag = TRUE; 

	return rc; 
}

int lwfs_naming_clnt_fini(
	struct naming_options *naming_opts, 
	lwfs_service *naming_svc)
{
	return LWFS_OK;
}


/** 
 * @brief Create a new directory.
 *
 * The \b lwfs_create_dir method creates a new directory entry using the
 * provided container ID and adds the new entry to the list 
 * of entries in the parent directory. 
 *
 * @param txn_id @input transaction ID.
 * @param parent @input the parent directory. 
 * @param name   @input the name of the new directory. 
 * @param cid    @input the container ID to use for the directory.  
 * @param cap    @input the capability that allows the operation.
 * @param result @output handle to the new directory entry. 
 * @param req    @output the request handle (used to test for completion)
 *
 * @remark <b>Ron (11/30/2004):</b>  I'm not sure we want the client to supply
 *         the container ID for the directory.  If the naming service generates
 *         the container ID, it can have some control over the access permissions
 *         authorized to the client.  For example, the naming service may allow
 *         clients to perform an add_file operation (through the nameservice API), 
 *         but not grant generic permission to write to the directory object. 
 *
 * @remark <b>Ron (12/04/2004):</b> Barney addressed the above concern by 
 *         mentioning that assigning a container ID to an entry does not 
 *         necessarily mean that the data associated with the entry (e.g.,
 *         the list of other entries in a directory) will be created using the
 *         provided container.  The naming service could use a private container 
 *         ID for the data.  That way, we can use the provided container ID 
 *         for the access policy for namespace entries, but at the same
 *         time, only allow access to the entry data through the naming API. 
 */
int lwfs_create_dir(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_cid cid,
		const lwfs_cap *cap, 
		lwfs_ns_entry *result,
		lwfs_request *req) 
{
	int rc = LWFS_OK;
	naming_create_dir_args args; 

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.name = (lwfs_name)name; 
	args.cid = cid; 
	args.cap = (lwfs_cap *)cap; 

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	if (local_calls) {
		/* call the server stub */
		rc = naming_create_dir(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not create dir: %s",
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
		rc = lwfs_call_rpc(&rpc_create_dir, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_bcreate_dir(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_cid cid,
		const lwfs_cap *cap, 
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_create_dir(txn_id, parent, name, cid, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_create_dir: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

/** 
 * @brief Remove a directory.
 *
 * The \b lwfs_remove_dir method removes an empty 
 * directory entry from the parent directory.  
 *
 * @param txn_id @input transaction ID.
 * @param parent @input parent of the dir to remove.
 * @param name   @input the name of the directory to remove.
 * @param cap    @input the capability that allows the client to modify 
 *                      the parent directory.
 * @param result @output handle to the directory entry that was removed. 
 * @param req    @output the request handle (used to test for completion). 
 *
 * @note There is no requirement that the target directory be 
 *       empty for the remove to complete successfully.  It is 
 *       the responsibility of the file system implementation
 *       to enforce such a policy. 
 * 
 * @remark <b>Ron (11/30/2004):</b> Is it enough to only require a capability 
 *          that allows the client to modify the contents of the parent 
 *          directory, or should we also have a capability that allows 
 *          removal of a particular directory entry? 
 * @remark  <b>Ron (11/30/2004):</b> The interfaces to remove files and dirs
 *          reference the target file/dir by name.  Should we also have methods
 *          to remove a file/dir by its lwfs_obj handle?  This is essentially
 *          what Rolf did in the original implementation. 
 */
int lwfs_remove_dir(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_cap *cap, 
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	naming_remove_dir_args args; 

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.name = (lwfs_name)name; 
	args.cap = (lwfs_cap *)cap; 

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	if (local_calls) {
		/* call the server stub */
		rc = naming_remove_dir(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not remove dir: %s",
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
		rc = lwfs_call_rpc(&rpc_remove_dir, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_bremove_dir(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_cap *cap, 
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 


	/* call the asynchronous function */
	rc = lwfs_remove_dir(txn_id, parent, name, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call async func: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

/** 
 * @brief Create a new file.
 * 
 * The \b lwfs_create_file method creates a new file entry
 * in the specified parent directory. Associated
 * with the file is a single storage server object
 * that the client creates prior to calling 
 * \b lwfs_create_file method.  
 *
 * @param txn_id @input transaction ID.
 * @param parent @input the parent directory.
 * @param name   @input the name of the new file. 
 * @param obj    @input the object to associate with the file. 
 * @param cap    @input the capability that allows the operation.
 * @param req    @output the request handle (used to test for completion)
 * 
 */
int lwfs_create_file(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_obj *obj,
		const lwfs_cap *cap, 
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	naming_create_file_args args; 

	log_debug(naming_debug_level, "starting lwfs_create_file(..., %s, ...)",
		name); 

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.name = (lwfs_name)name; 
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 

	/* make sure the result is zero'd out */
	memset(result, 0, sizeof(lwfs_ns_entry));

	if (local_calls) {
		/* call the server stub */
		rc = naming_create_file(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not create file: %s",
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
		rc = lwfs_call_rpc(&rpc_create_file, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	log_debug(naming_debug_level, "finished lwfs_create_file(..., %s, ...)",
		name); 

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_bcreate_file(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_obj *obj,
		const lwfs_cap *cap, 
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_create_file(txn_id, parent, name, obj, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_create_file: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

/** 
 * @brief Create a link. 
 * 
 * The \b lwfs_create_link method creates a new link entry in the 
 * parent directory and associates the link with an existing 
 * namespace entry (file or directory) with the link. 
 * 
 * @param txn_id @input transaction ID.
 * @param parent @input the parent directory. 
 * @param name   @input the name of the new link.
 * @param target @input the entry to link with. 
 * @param cap    @input the capability that allows the client to add 
 *                      a link to the parent directory.
 * @param result @output the new link entry. 
 * @param req    @output the request handle (used to test for completion). 
 * 
 */
int lwfs_create_link(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_cap *cap, 
		const lwfs_ns_entry *target_parent,
		const char *target_name, 
		const lwfs_cap *target_cap,
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	naming_create_link_args args; 

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.name = (lwfs_name)name; 
	args.cap = (lwfs_cap *)cap; 
	args.target_parent = (lwfs_obj *)&target_parent->entry_obj; 
	args.target_name = (lwfs_name)target_name; 
	args.target_cap = (lwfs_cap *)target_cap; 

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	if (local_calls) {
		/* call the server stub */
		rc = naming_create_link(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not create dir: %s",
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
		rc = lwfs_call_rpc(&rpc_create_link, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_bcreate_link(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_cap *cap, 
		const lwfs_ns_entry *target_parent,
		const char *target_name, 
		const lwfs_cap *target_cap,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_create_link(txn_id, parent, name, cap, 
			target_parent, target_name, target_cap, 
			result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_create_link: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}

/** 
 * @brief Unlink a namespace entry. 
 *
 * This method removes a namespace entry from the specified 
 * parent directory.  It does not remove any storage 
 * associated with the link (e.g., the file object).
 *
 * @param txn_id @input transaction ID.
 * @param parent @input the parent directory.
 * @param name @input the name of the link entry.
 * @param cap @input the capability that allows us to 
 *            remove the link from the parent.
 * @param req @output the request handle (used to test for completion). 
 * 
 */
int lwfs_unlink(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name, 
		const lwfs_cap *cap,
		lwfs_ns_entry *result,
		lwfs_request *req) 
{
	int rc = LWFS_OK;
	naming_unlink_args args; 

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.name = (lwfs_name)name; 
	args.cap = (lwfs_cap *)cap; 

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	if (local_calls) {
		/* call the server stub */
		rc = naming_unlink(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not remove link: %s",
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
		rc = lwfs_call_rpc(&rpc_unlink, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_bunlink(
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name, 
		const lwfs_cap *cap,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_unlink(txn_id, parent, name, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_unlink: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}


/** 
 * @brief Find an entry in a directory.
 *
 * The \b lwfs_lookup method finds an entry by name and acquires 
 * a lock for the entry. We incorporated an (optional) implicit lock into 
 * the method to remove a potential race condition where an outside
 * process could modify the entry between the time the lookup call
 * found the entry and the client performs an operation on the entry. 
 * The resulting lock_id is encoded in the \ref lwfs_obj
 * data structure.
 * 
 * @param txn_id @input the transaction ID (used only to acquire the lock). 
 * @param parent @input the parent directory.
 * @param name   @input the name of the entry to look up. 
 * @param lock_type @input the type of lock to get on the entry. 
 * @param cap    @input the capability that allows us read from the directory.
 * @param result @output the entry.
 * @param req @output the request handle (used to test for completion).
 */
int lwfs_lookup(
		const lwfs_txn *txn_id, 
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_lock_type lock_type, 
		const lwfs_cap *cap, 
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	naming_lookup_args args; 

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.name = (lwfs_name)name; 
	args.lock_type = lock_type; 
	args.cap = (lwfs_cap *)cap; 

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	if (local_calls) {
		/* call the server stub */
		rc = naming_lookup(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not lookup: %s",
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
		rc = lwfs_call_rpc(&rpc_lookup, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_blookup(
		const lwfs_txn *txn_id, 
		const lwfs_ns_entry *parent, 
		const char *name, 
		const lwfs_lock_type lock_type, 
		const lwfs_cap *cap, 
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_lookup(txn_id, parent, name, lock_type, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_lookup: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_warn(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}


/** 
 * @brief Read the contents of a directory.
 *
 * The \b lwfs_list_dir method returns the list of entries in the 
 * parent directory. 
 *
 * @param parent @input the parent directory. 
 * @param cap    @input the capability that allows us to read the contents of the parent.
 * @param result @output space for the result.
 * @param req    @output the request handle (used to test for completion). 
 * 
 * @remarks <b>Ron (12/01/2004):</b> I removed the transaction ID from this call
 *          because it does not change the state of the system and we do 
 *          not acquire locks on the entries returned. 
 *
 * @remark <b>Ron (12/7/2004):</b> I wonder if we should add another argument 
 *         to filter to results on the server.  For example, only return 
 *         results that match the provided regular expression. 
 */
int lwfs_list_dir(
		const lwfs_ns_entry *parent,
		const lwfs_cap *cap,
		lwfs_ns_entry_array *result,
		lwfs_request *req) 
{
	int rc = LWFS_OK;
	naming_list_dir_args args; 

	memset(&args, 0, sizeof(args));
	args.parent = (lwfs_obj *)&parent->entry_obj; 
	args.cap = (lwfs_cap *)cap; 

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry_array));

	if (local_calls) {
		/* call the server stub */
		rc = naming_list_dir(NULL, &args, NULL, result); 
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not list dir: %s",
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
		rc = lwfs_call_rpc(&rpc_list_dir, &args, NULL, 0, result, req);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc)); 
		}
	}

	return rc; 
}

/**
 * @brief Create a directory on the naming server and block until complete. 
 *
 * This function calls \ref lwfs_create_dir and waits for completion. 
 */
int lwfs_blist_dir(
		const lwfs_ns_entry *parent,
		const lwfs_cap *cap,
		lwfs_ns_entry_array *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_list_dir(parent, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_list_dir: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* wait for completion */
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "error waiting for request: %s",
				lwfs_err_str(rc2)); 
		return rc2; 
	}

	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}



/** 
 * @brief Get the attributes of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_get_attr method fetches the attributes
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param cap @input the capability that allows the operation.
 * @param result @output the attributes.
 * @param req @output the request handle (used to test for completion)
 *
 * @returns the object attributes in the result field.
 */
int lwfs_get_attr(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_obj_attr *res,
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_get_attr_args args;

	/* initialize the args */
	memset(&args, 0, sizeof(ss_get_attr_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 

	rpc_get_attr.svc = (lwfs_service *)&obj->svc;

	if (local_calls) {
		/* The problem with having local calls is that 
		 * we're not sure which version of the get_attr
		 * method to call: the storage server or the 
		 * name server. We'll decide based on object 
		 * type. 
		 */
		switch (obj->type) {

			case LWFS_DIR_ENTRY:
				/* in the case of dirs, call nameserver. */
				rc = naming_get_attr(NULL, &args, NULL, res);
				break; 

			case LWFS_LINK_ENTRY:
				/* in the case of links, call nameserver. */
				rc = naming_get_attr(NULL, &args, NULL, res);
				break; 

			case LWFS_FILE_ENTRY:
				/* in the case of links, call nameserver. */
				rc = naming_get_attr(NULL, &args, NULL, res);
				break; 

			default:
				/* all other cases call the storage server */
				rc = ss_get_attr(NULL, &args, NULL, res);
				break;
		}	

		if (rc != LWFS_OK) {
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


		/* send a request to execute the remote procedure */
		rc = lwfs_call_rpc(&rpc_get_attr, &args, NULL, 0, res, req);

		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc;
} /* lwfs_get_attr() */

/** 
 * @brief Get the attributes of an object. 
 *
 * @ingroup ss_api
 *  
 * The \ref lwfs_bget_attr method fetches the attributes
 * (defined in \ref lwfs_obj_attr) of a specified 
 * storage server object. 
 *
 * This method blocks until the operation completes. 
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param cap @input the capability that allows the operation.
 * @param result @output the attributes.
 *
 * @returns the object attributes in the result field.
 */
int lwfs_bget_attr(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_obj_attr *res)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_get_attr(txn_id, obj, cap, res, &req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "failed async method: %s",
			lwfs_err_str(rc));
		return rc; 
	}
	
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(naming_debug_level, "failed waiting for result:%s",
			lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


