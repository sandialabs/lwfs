/**  
 *   @file storage_client_sync.c
 * 
 *   @brief Implementation of the synchronous client-side methods for the 
 *   storage server API. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Bill Lawry (wflawry\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#include <errno.h>

#include "storage_client_sync.h"


/* ---- Private methods ---- */



/** 
 * @brief Create a new object. 
 *
 * The \b lwfs_create_object method creates a new object 
 * on the specified storage server.
 *
 * @param svc @input the service descriptor of the storage service. 
 * @param txn_id @input transaction ID.
 * @param cid @input the container ID for the new object.
 * @param cap @input the capability that allows creation of the 
 *            object. 
 * @param result @output a new object reference. 
 * @param req @input the request handle (used to test for completion). 
 *
 * @returns a new \ref lwfs_obj_ref "object reference" in the result field. 
 */
int lwfs_create_obj_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj,
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_create_obj(txn_id, obj, cap, &req);
	if (rc != LWFS_OK)  {
		log_error(ss_debug_level, "could not call ss_create_obj");
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc);
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_debug(ss_debug_level, "error: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK)? rc : rc2; 
	}

	return rc; 
}


/** 
 * @brief Remove an object. 
 *
 * This method sends an asynchronous request to 
 * the storage server asking it to remove an object.
 *
 * @param txn_id @input transaction ID.
 * @param obj    @input object to remove
 * @param cap    @input capability that allows the operation.
 * @param req    @output request handle (used to test for completion). 
 *
 * @returns the removed object (an \ref lwfs_obj_ref "object reference")
 * in the result field. 
 */
int lwfs_remove_obj_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_remove_obj(txn_id, obj, cap, &req);
	if (rc != LWFS_OK)  {
		log_error(ss_debug_level, "could not call remove_obj");
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc);
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_debug(ss_debug_level, "error: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK)? rc : rc2; 
	}

	return rc; 
}

/** 
 * @brief Read from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_read method attempts to read \em count bytes from the 
 * remote object \em src_obj. 
 *
 * @param txn_id @input transaction ID.
 * @param src_obj @input reference to the source object. 
 * @param src_offset @input where to start reading.
 * @param buf @input  where to put the data.
 * @param count @input the number of bytes to read. 
 * @param cap @input the capability that allows the operation.
 * @param result @output the number of bytes actually read. 
 * @param req @output the request handle (used to test for completion). 
 */
int lwfs_read_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *src_obj, 
		const lwfs_size src_offset, 
		void *buf, 
		const lwfs_size len, 
		const lwfs_cap *cap, 
		lwfs_size *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_read(txn_id, src_obj, src_offset, buf, len, cap, result, &req);
	if (rc != LWFS_OK)  {
		log_error(ss_debug_level, "could not read from obj");
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc);
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_debug(ss_debug_level, "error: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK)? rc : rc2; 
	}

	return rc; 
}


/** 
 * @brief Write to an object. 
 *
 * @ingroup ss_api 
 *  
 * The \b lwfs_write method writes a contiguous array of bytes 
 * to a storage server object. 
 *
 * @param txn_id @input transaction ID.
 * @param dest_obj @input reference to the object to write to. 
 * @param dest_offset @input where to start writing.
 * @param buf @input the client-side buffer. 
 * @param len @input the number of bytes to write. 
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion). 
 */
int lwfs_write_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *dest_obj, 
		const lwfs_size dest_offset, 
		const void *buf, 
		const lwfs_size len, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_write(txn_id, dest_obj, dest_offset, buf, len, cap, &req);
	if (rc != LWFS_OK)  {
		log_error(ss_debug_level, "could not write to obj");
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc);
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_debug(ss_debug_level, "error: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK)? rc : rc2; 
	}

	return rc; 
}
	
int lwfs_fsync_sync(
        const lwfs_txn *txn_id,
        const lwfs_obj *obj, 
        const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_fsync(txn_id, obj, cap, &req);
	if (rc != LWFS_OK)  {
		log_error(ss_debug_level, "could not call lwfs_fsync");
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc);
	if ((rc != LWFS_OK) || (rc2 != LWFS_OK)) {
		log_debug(ss_debug_level, "error: %s",
				(rc != LWFS_OK)? lwfs_err_str(rc) : lwfs_err_str(rc2));
		return (rc != LWFS_OK)? rc : rc2; 
	}

	return rc; 
}


/** 
 * @brief List all attributes from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_listattrs_sync method fetches a list of names of attributes
 * of a specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param cap @input the capability that allows the operation.
 *
 * @returns the names of the object attributes in the result field.
 */
int lwfs_listattrs_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_name_array *res)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_listattrs(txn_id, obj, cap, res, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}

/** 
 * @brief Get the attributes of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_getattrs_sync method fetches the attributes
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param names @input the names of the attrs to get.
 * @param cap @input the capability that allows the operation.
 *
 * @returns the object attributes in the result field.
 */
int lwfs_getattrs_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name_array *names, 
		const lwfs_cap *cap, 
		lwfs_attr_array *res)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_getattrs(txn_id, obj, names, cap, res, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


/** 
 * @brief Set the attributes of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_setattrs_sync method sets the attributes
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param attrs @input the attributes to set.
 * @param cap @input the capability that allows the operation.
 */
int lwfs_setattrs_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_attr_array *attrs, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_setattrs(txn_id, obj, attrs, cap, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


/** 
 * @brief Remove attributes from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_rmattrs_sync method removes the attributes
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param names @input the names of the attrs to remove.
 * @param cap @input the capability that allows the operation.
 */
int lwfs_rmattrs_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name_array *names, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_rmattrs(txn_id, obj, names, cap, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


/** 
 * @brief Get a named attribute of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_getattr_sync method fetches a named attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param name @input the name of the attr to get.
 * @param cap @input the capability that allows the operation.
 *
 * @returns the object attribute in the result field.
 */
int lwfs_getattr_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name name, 
		const lwfs_cap *cap, 
		lwfs_attr *attr)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_getattr(txn_id, obj, name, cap, attr, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


/** 
 * @brief Set the attribute of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_setattr_sync method sets the attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param attr @input the attribute to set.
 * @param cap @input the capability that allows the operation.
 */
int lwfs_setattr_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name name,
		const void *value,
		const int size, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_setattr(txn_id, obj, name, value, size, cap, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


/** 
 * @brief Remove an attribute from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_rmattr_sync method removes the attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param names @input the names of the attrs to remove.
 * @param cap @input the capability that allows the operation.
 */
int lwfs_rmattr_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name name, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_rmattr(txn_id, obj, name, cap, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}


/** 
 * @brief Stat the object.
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_setattr_sync method fetches the attributes
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
int lwfs_stat_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap,
		lwfs_stat_data *res)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_stat(txn_id, obj, cap, res, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed async method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
				lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}

/** 
 * @brief Truncate an object. 
 *
 * @ingroup ss_api
 *  
 * The \ref lwfs_truncate method fixes the size of an 
 * object to exactly \em size bytes.  If the new size 
 * is greater than the current size, the remaning bytes
 * are filled with zeros. 
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param size @input the new size of the object. 
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 */
int lwfs_truncate_sync(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_ssize size, 
		const lwfs_cap *cap)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_truncate(txn_id, obj, size, cap, &req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "failed truncate method: %s",
			lwfs_err_str(rc));
		return rc; 
	}
	
	rc2 = lwfs_wait(&req, &rc); 
	if (rc2 != LWFS_OK) {
		log_error(ss_debug_level, "failed waiting for result:%s",
			lwfs_err_str(rc));
		return rc2; 
	}

	return rc; 
}
