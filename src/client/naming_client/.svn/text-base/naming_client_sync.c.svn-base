/**  
 *   @file naming_clnt_sync.c
 * 
 *   @brief Implementation of the synchrnous client-side API for the 
 *          LWFS naming service.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   @version $Revision: 791 $
 *   @date $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */


#include "naming_client_sync.h"
#include "naming_client.h"


/** 
 * @brief Create a new namespace.
 *
 * @ingroup naming_api
 *
 * The \b lwfs_create_namespace method creates a new namespace using the
 * provided container ID. 
 *
 * @param svc    @input lwfs naming service.
 * @param txn_id @input transaction ID.
 * @param name   @input the name of the new namespace. 
 * @param cid    @input the container ID to use for the namespace.  
 * @param result @output handle to the new namespace. 
 */
int lwfs_create_namespace_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const char *name, 
			const lwfs_cid cid,
			lwfs_namespace *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_create_namespace(svc, txn_id, name, cid, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_create_namespace: %s",
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

	if (logging_debug(naming_debug_level))
		fprint_lwfs_namespace(logger_get_file(), "namespace", "lwfs_create_namespace created ->", result);

	return rc; 
}


/** 
 * @brief Remove a namespace.
 *
 * @ingroup naming_api
 *
 * The \b lwfs_remove_namespace method removes an namespace.  
 *
 * @param txn_id @input transaction ID.
 * @param name   @input the name of the namespace to remove.
 * @param cap    @input the capability that allows the client to remove the namespace.
 * @param result @output handle to the namespace that was removed. 
 */
int lwfs_remove_namespace_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const char *name, 
			const lwfs_cap *cap, 
			lwfs_namespace *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_remove_namespace(svc, txn_id, name, cap, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_remove_namespace: %s",
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

	if (logging_debug(naming_debug_level))
		fprint_lwfs_namespace(logger_get_file(), "namespace", "lwfs_remove_namespace removed ->", result);

	return rc; 
}


/** 
 * @brief Get a namespace.
 *
 * @ingroup naming_api
 *
 * The \b lwfs_get_namespace method gets a namespace reference. 
 *
 * @param svc    @input lwfs naming service.
 * @param name   @input the name of the namespace. 
 * @param result @output handle to the new namespace. 
 */
int lwfs_get_namespace_sync(
			const lwfs_service *svc, 
			const char *name, 
			lwfs_namespace *result)
{
    int rc = LWFS_OK; 
    int rc2 = LWFS_OK; 
    lwfs_request req; 

    /* call the asynchronous function */
    rc = lwfs_get_namespace(svc, name, result, &req); 
    if (rc != LWFS_OK) {
	log_error(naming_debug_level, "could not call lwfs_get_namespace: %s",
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

    if (logging_debug(naming_debug_level))
	fprint_lwfs_namespace(logger_get_file(), "namespace", "lwfs_get_namespace found ->", result);

    return rc; 
}


/** 
 * @brief Get a list of namespaces that exist on the naming server.
 *
 * @ingroup naming_api
 *
 * The \b lwfs_list_namespaces method returns the list of namespaces available 
 * on the naming server. 
 *
 * @param svc    @input_type Points to the naming service descriptor. 
 * @param result @output_type space for the result.
 */
int lwfs_list_namespaces_sync(
			const lwfs_service *svc,
			lwfs_namespace_array *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_list_namespaces(svc, result, &req); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not call lwfs_list_namespace: %s",
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

	if (logging_debug(naming_debug_level))
		fprint_lwfs_namespace_array(logger_get_file(), "namespace", "lwfs_list_namespaces found ->", result);

	return rc; 
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
 *
 */
int lwfs_create_dir_sync(
		const lwfs_service *svc, 
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
	rc = lwfs_create_dir(svc, txn_id, parent, name, cid, cap, result, &req); 
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
		log_warn(naming_debug_level, "error in remote operation: %s",
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
int lwfs_remove_dir_sync(
		const lwfs_service *svc,
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
	rc = lwfs_remove_dir(svc, txn_id, parent, name, cap, result, &req); 
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
		log_warn(naming_debug_level, "error in remote operation: %s",
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
int lwfs_create_file_sync(
		const lwfs_service *svc, 
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
	rc = lwfs_create_file(svc, txn_id, parent, name, obj, cap, result, &req); 
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
		log_warn(naming_debug_level, "error in remote operation: %s",
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
int lwfs_create_link_sync(
		const lwfs_service *svc, 
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
	rc = lwfs_create_link(svc, txn_id, parent, name, cap, 
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
		log_warn(naming_debug_level, "error in remote operation: %s",
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
int lwfs_unlink_sync(
		const lwfs_service *svc,
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
	rc = lwfs_unlink(svc, txn_id, parent, name, cap, result, &req); 
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
		log_warn(naming_debug_level, "error in remote operation: %s",
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
int lwfs_lookup_sync(
		const lwfs_service *svc,
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
    rc = lwfs_lookup(svc, txn_id, parent, name, lock_type, cap, result, &req); 
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
	log_warn(naming_debug_level, "error in remote operation..."
		" could not lookup entry \"%s\": %s",
		name, lwfs_err_str(rc));
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
int lwfs_list_dir_sync(
		const lwfs_service *svc, 
		const lwfs_ns_entry *parent,
		const lwfs_cap *cap,
		lwfs_ns_entry_array *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	/* call the asynchronous function */
	rc = lwfs_list_dir(svc, parent, cap, result, &req); 
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
		log_warn(naming_debug_level, "error in remote operation: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}
