/**
 *   @file naming_clnt.c
 *
 *   @brief Implementation of the client-side API for the LWFS naming service.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   @version $Revision: 1431 $
 *   @date $Date: 2007-05-21 07:58:18 -0600 (Mon, 21 May 2007) $
 */
#include "config.h"

#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif



#include "naming_client.h"


/* ------------------ PRIVATE FUNCTIONS ---------------- */

/**
 * @brief Initialize the client (executes only once)
 */
static int naming_client_init(const lwfs_service *naming_svc)
{
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
	}


	/* initialize the rpc library */
	rc = lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to init rpc client: %s",
				lwfs_err_str(rc));
		return rc;
	}

	init_flag = TRUE;

	return rc;
}


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

/**
 * @brief Create a new namespace.
 *
 * The \b lwfs_create_namespace method creates a new namespace using the
 * provided container ID.
 *
 * @param svc    @input lwfs naming service.
 * @param txn_id @input transaction ID.
 * @param name   @input the name of the new namespace.
 * @param cid    @input the container ID to use for the namespace.
 * @param result @output handle to the new namespace.
 * @param req    @output the request handle (used to test for completion)
 *
 * @remark <b>Todd (12/13/2006):</b>  Notice that creating a namespace
 * does not requires a capability.  The namespace is created in the
 * \ref lwfs_cid "container ID" given.  Normal access control policies
 * apply.
 */
int lwfs_create_namespace(
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const char *name,
		const lwfs_cid cid,
		lwfs_namespace *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_create_namespace_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	memset(result, 0, sizeof(lwfs_namespace));

	args.txn_id = (lwfs_txn *)txn_id;
	args.name = (lwfs_name)name;
	args.cid = (lwfs_cid)cid;

	/* send an rpc request */
	rc = lwfs_call_rpc(svc, LWFS_OP_CREATE_NAMESPACE,
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc;
	}

	return rc;
}


/**
 * @brief Remove a namespace.
 *
 * The \b lwfs_remove_namespace method removes an namespace.
 *
 * @param txn_id @input transaction ID.
 * @param name   @input the name of the namespace to remove.
 * @param cap    @input the capability that allows the client to remove the namespace.
 * @param result @output handle to the namespace that was removed.
 * @param req    @output the request handle (used to test for completion).
 *
 * @note There is no requirement that the target namespace be
 *       empty for the remove to complete successfully.  It is
 *       the responsibility of the file system implementation
 *       to enforce such a policy.
 */
int lwfs_remove_namespace(
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const char *name,
		const lwfs_cap *cap,
		lwfs_namespace *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_remove_namespace_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	memset(result, 0, sizeof(lwfs_namespace));

	args.txn_id = (lwfs_txn *)txn_id;
	args.name = (lwfs_name)name;
	args.cap = (lwfs_cap *)cap;

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_REMOVE_NAMESPACE,
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc;
	}

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
 * @param req    @output the request handle (used to test for completion)
 *
 * @remark <b>Todd (12/13/2006):</b>  Notice that creating a namespace
 * does not requires a capability.  The namespace is created in the
 * \ref lwfs_cid "container ID" given.  Normal access control policies
 * apply.
 */
int lwfs_get_namespace(
		const lwfs_service *svc,
		const char *name,
		lwfs_namespace *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_get_namespace_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	memset(result, 0, sizeof(lwfs_namespace));

	args.name = (lwfs_name)name;

	if (logging_debug(naming_debug_level))
		fprint_lwfs_name(logger_get_file(), "args->name", "DEBUG\t", &args.name);

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_GET_NAMESPACE,
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc;
	}

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
 * @param req    @output_type the request handle (used to test for completion).
 *
 * @remark <b>Todd (12/14/2006):</b> This comment from \b lwfs_list_dir applies
 *         here as well - "I wonder if we should add another argument
 *         to filter to results on the server.  For example, only return
 *         results that match the provided regular expression".
 */
int lwfs_list_namespaces(
		const lwfs_service *svc,
		lwfs_namespace_array *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(result, 0, sizeof(lwfs_namespace_array));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_LIST_NAMESPACES,
			NULL	, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc;
	}

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
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name,
		const lwfs_cid cid,
		const lwfs_cap *cap,
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_create_dir_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	memset(result, 0, sizeof(lwfs_ns_entry));

	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_ns_entry *)parent;
	args.name = (lwfs_name)name;
	args.cid = cid;
	args.cap = (lwfs_cap *)cap;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* send an rpc request */
	rc = lwfs_call_rpc(svc, LWFS_OP_CREATE_DIR,
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
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
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name,
		const lwfs_cap *cap,
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_remove_dir_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_ns_entry *)parent;
	args.name = (lwfs_name)name;
	args.cap = (lwfs_cap *)cap;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_REMOVE_DIR,
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
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
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name,
		const lwfs_obj *obj,
		const lwfs_cap *cap,
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_create_file_args args;

	log_debug(naming_debug_level, "starting lwfs_create_file(..., %s, ...)",
			name);

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);


	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_ns_entry *)parent;
	args.name = (lwfs_name)name;
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap;

	/* make sure the result is zero'd out */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_CREATE_FILE,
			&args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
	}

	log_debug(naming_debug_level, "finished lwfs_create_file(..., %s, ...)",
			name);

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
		const lwfs_service *svc,
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
	lwfs_create_link_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_ns_entry *)parent;
	args.name = (lwfs_name)name;
	args.cap = (lwfs_cap *)cap;
	args.target_parent = (lwfs_ns_entry *)target_parent;
	args.target_name = (lwfs_name)target_name;
	args.target_cap = (lwfs_cap *)target_cap;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_CREATE_LINK, &args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
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
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name,
		const lwfs_cap *cap,
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_unlink_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_ns_entry *)parent;
	args.name = (lwfs_name)name;
	args.cap = (lwfs_cap *)cap;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_UNLINK, &args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
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
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_ns_entry *parent,
		const char *name,
		const lwfs_lock_type lock_type,
		const lwfs_cap *cap,
		lwfs_ns_entry *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_lookup_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.parent = (lwfs_ns_entry *)parent;
	args.name = (lwfs_name)name;
	args.lock_type = lock_type;
	args.cap = (lwfs_cap *)cap;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_LOOKUP, &args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
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
		const lwfs_service *svc,
		const lwfs_ns_entry *parent,
		const lwfs_cap *cap,
		lwfs_ns_entry_array *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	lwfs_list_dir_args args;

	/* initialize the naming client (executed only once) */
	naming_client_init(svc);

	memset(&args, 0, sizeof(args));
	args.parent = (lwfs_ns_entry *)parent;
	args.cap = (lwfs_cap *)cap;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry_array));

	/* call the remote procedure */
	rc = lwfs_call_rpc(svc, LWFS_OP_LIST_DIR, &args, NULL, 0, result, req);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
	}

	return rc;
}
