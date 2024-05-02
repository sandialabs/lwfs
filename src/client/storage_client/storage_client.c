/**  
 *   @file storage_clnt.c
 * 
 *   @brief Implementation of the client-side methods for the 
 *   storage server API. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Bill Lawry (wflawry\@sandia.gov)
 *   $Revision: 1532 $
 *   $Date: 2007-09-11 16:04:32 -0600 (Tue, 11 Sep 2007) $
 */

#include <errno.h>
#include <time.h>
#include "config.h"

#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif


/*#include "portals3.h"*/
#include "storage_client.h"
#include "support/timer/timer.h"
#include "support/ptl_uuid/ptl_uuid.h"


/* ---- Private methods ---- */

static int ss_init()
{
	static lwfs_bool initialized = FALSE; 
	int rc = LWFS_OK; 

	if (initialized) {
		return rc;
	}

	/* register message encoding schemes for the operations */
	rc = register_ss_encodings();
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to register encodings: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	initialized = TRUE; 

	return rc; 
}


static void gen_unique_oid(lwfs_oid *oid)
{
    uuid_t *uuid;
    size_t oid_size = sizeof(lwfs_oid);
    
    uuid_create(&uuid);
    uuid_make(uuid, UUID_MAKE_V1);
    uuid_export(uuid, UUID_FMT_BIN, (void *)&oid, &oid_size);
    uuid_destroy(uuid);
    

    return; 
}

/* ------- Private methods end ------------ */


/**
 * @brief Initialize the object data structure.
 */
int lwfs_init_obj(
		const lwfs_service *svc, 
		const int type,
		const lwfs_cid cid,
		const lwfs_oid oid,
		lwfs_obj *obj)
{
	int rc = LWFS_OK;

	memset(obj, 0, sizeof(lwfs_obj));
	obj->type = type; 
	obj->cid = cid; 
	memcpy(obj->oid, oid, sizeof(lwfs_oid)); 
	memcpy(&obj->svc, svc, sizeof(lwfs_service));

	/* generate a unique ID */
	if (lwfs_is_oid_any(oid)) { 
		/* TODO: do we need to generate a uuid here? */
		gen_unique_oid(&obj->oid);
	}

	return rc; 
}


/** 
 * @brief Create a new object on a storage server. 
 *
 * The \ref lwfs_create_obj method sends a request to a 
 * storage server to create an object.  The object data
 * structure contains information about the type, container ID,
 * and object ID to use for the new object.  The server does not 
 * return any information other than an success or fail message.
 * See \ref lwfs_obj_init to generate the object data structure. 
 *
 *
 * @param svc @input the service descriptor of the storage service. 
 * @param txn_id @input transaction ID.
 * @param cid @input the container ID for the new object.
 * @param cid @input the object ID for the new object.
 * @param cap @input the capability that allows creation of the 
 *            object. 
 * @param result @output a new object reference. 
 * @param req @input the request handle (used to test for completion). 
 *
 * @returns a new \ref lwfs_obj_ref "object reference" in the result field. 
 */
int lwfs_create_obj(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;  /* return code */

	ss_create_obj_args args;

	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	if (lwfs_is_oid_any(obj->oid)) {
		log_error(ss_debug_level, "must initialize oid");
		return LWFS_ERR_STORAGE;
	}


	/* initialize the arguments */
	memset(&args, 0, sizeof(ss_create_obj_args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.obj    = (lwfs_obj *)obj; 
	args.cap    = (lwfs_cap *)cap; 

	log_debug(ss_debug_level, "calling rpc for create_obj");

	/* send an rpc request */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_CREATE, 
			&args, NULL, 0, NULL, req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
} /* lwfs_create_object() */



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
int lwfs_remove_obj(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_remove_obj_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the arguments */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 

	log_debug(ss_debug_level, "calling rpc for lwfs_remove_obj");


	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_REMOVE,
			&args, NULL, 0, NULL, req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
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
int lwfs_read(
		const lwfs_txn *txn_id,
		const lwfs_obj *src_obj, 
		const lwfs_size src_offset, 
		void *buf, 
		const lwfs_size len, 
		const lwfs_cap *cap, 
		lwfs_size *result,
		lwfs_request *req)
{
	int rc = LWFS_OK;
	ss_read_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the arguments */
	memset(&args, 0, sizeof(ss_read_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.src_obj = (lwfs_obj *)src_obj; 
	args.src_offset = src_offset; 
	args.len = len; 
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&src_obj->svc, LWFS_OP_READ, 
			&args, buf, len, result, req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_read() */




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
int lwfs_write(
		const lwfs_txn *txn_id,
		const lwfs_obj *dest_obj, 
		const lwfs_size dest_offset, 
		const void *buf, 
		const lwfs_size len, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;
	ss_write_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the arguments */
	memset(&args, 0, sizeof(ss_write_args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.dest_obj = (lwfs_obj *)dest_obj;
	args.dest_offset = dest_offset; 
	args.len = len; 
	args.cap = (lwfs_cap *)cap; 


	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&dest_obj->svc, LWFS_OP_WRITE, 
			&args, (void *)buf, len, NULL, req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;

} /* lwfs_write() */

	
int lwfs_fsync(
        const lwfs_txn *txn_id,
        const lwfs_obj *obj, 
        const lwfs_cap *cap, 
        lwfs_request *req) 
{
	int rc = LWFS_OK;
	ss_fsync_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the arguments */
	memset(&args, 0, sizeof(ss_fsync_args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 


	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_FSYNC, 
			&args, NULL, 0, NULL, req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
}


/** 
 * @brief Stat the object.
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_setattr method fetche a named attribute
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
int lwfs_stat(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_stat_data *res,
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_stat_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_stat_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_STAT, 
			&args, NULL, 0, res, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_get_attr() */


/** 
 * @brief Create an attribute array from the component parts.
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_create_attr_array creates an lwfs_attr_array from 
 * arrays of attr names, attr values and attr value lengths.
 *
 * @param names @input array of attribute names
 * @param values @input array of attribute values
 * @param sizes @input array of attribute value sizes
 * @param num_attrs @input the number of attributes to create
 * @param attrs @output the request handle (used to test for completion)
 *
 * @returns the names of the object attributes in the result field.
 */
int lwfs_create_attr_array(
		const lwfs_name *names,
		const void **values, 
		const int *sizes, 
		const int num_attrs,
		lwfs_attr_array *attrs)
{
	int i=0;
	
	attrs->lwfs_attr_array_len = num_attrs;
	attrs->lwfs_attr_array_val = calloc(num_attrs, sizeof(lwfs_attr));
	for (i=0;i<num_attrs;i++) {
		attrs->lwfs_attr_array_val[i].name = calloc(1, sizeof(char)*LWFS_NAME_LEN);
		strncpy(attrs->lwfs_attr_array_val[i].name, names[i], LWFS_NAME_LEN);
		attrs->lwfs_attr_array_val[i].value.lwfs_attr_data_len = sizes[i];
		attrs->lwfs_attr_array_val[i].value.lwfs_attr_data_val = calloc(1, sizeof(char)*sizes[i]);
		memcpy(attrs->lwfs_attr_array_val[i].value.lwfs_attr_data_val, values[i], sizes[i]);
	}

	if (logging_debug(ss_debug_level)) {
		for (i=0;i<attrs->lwfs_attr_array_len;i++) {
			log_debug(ss_debug_level, "attr.name=%s; attr.value=%s",
				  attrs->lwfs_attr_array_val[i].name,
				  attrs->lwfs_attr_array_val[i].value.lwfs_attr_data_val); 
		}
	}


	return LWFS_OK;
} /* lwfs_create_attr_array() */


/** 
 * @brief Create an attribute array from the component parts.
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_create_attr_array creates an lwfs_attr_array from 
 * arrays of attr names, attr values and attr value lengths.
 *
 * @param names @input array of attribute names
 * @param values @input array of attribute values
 * @param sizes @input array of attribute value sizesvi configu
 * @param num_attrs @input the number of attributes to create
 * @param attrs @output the request handle (used to test for completion)
 *
 * @returns the names of the object attributes in the result field.
 */
int lwfs_create_name_array(
		const lwfs_name *names,
		const int num_names,
		lwfs_name_array *name_array)
{
	int i=0;
	
	name_array->lwfs_name_array_len = num_names;
	name_array->lwfs_name_array_val = calloc(num_names, sizeof(lwfs_name *));
	for (i=0;i<num_names;i++) {
		name_array->lwfs_name_array_val[i]=calloc(1, sizeof(char)*LWFS_NAME_LEN);
		strncpy(name_array->lwfs_name_array_val[i], names[i], LWFS_NAME_LEN);
	}

	if (logging_debug(ss_debug_level)) {
		for (i=0;i<name_array->lwfs_name_array_len;i++) {
			log_debug(ss_debug_level, "attr.name=%s",
				  name_array->lwfs_name_array_val[i]); 
		}
	}


	return LWFS_OK;
} /* lwfs_create_name_array() */


/** 
 * @brief List all attributes from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_listattr method fetches a list of names of attributes
 * of a specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 *
 * @returns the names of the object attributes in the result field.
 */
int lwfs_listattrs(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_name_array *res, 
		lwfs_request *req)
{
	int rc = LWFS_OK;
	ss_listattrs_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_listattrs_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 

	memset(res, 0, sizeof(lwfs_name_array)); 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_LISTATTRS, 
			&args, NULL, 0, res, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_listattrs() */


/** 
 * @brief Get the attributes of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_getattrs method fetches the named attributes
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param names @input the names of the attrs to get.
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 *
 * @returns the object attributes in the result field.
 */
int lwfs_getattrs(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name_array *names, 
		const lwfs_cap *cap, 
		lwfs_attr_array *res,
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_getattrs_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_getattrs_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.names = (lwfs_name_array *)names;
	args.cap = (lwfs_cap *)cap; 

	memset(res, 0, sizeof(lwfs_attr_array)); 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_GETATTRS, 
			&args, NULL, 0, res, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_getattrs() */


/** 
 * @brief Set the attributes of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_setattrs method sets a named attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param attrs @input the attributes to set.
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 */
int lwfs_setattrs(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_attr_array *attrs, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_setattrs_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_setattrs_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.attrs = (lwfs_attr_array *)attrs;
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_SETATTRS, 
			&args, NULL, 0, NULL, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_setattrs() */


/** 
 * @brief Remove an attribute from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_rmattrs method removes named attributes
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param names @input the names of the attrs to remove.
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 */
int lwfs_rmattrs(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name_array *names, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;
	ss_rmattrs_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_rmattrs_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.names = (lwfs_name_array *)names;
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_RMATTRS, 
			&args, NULL, 0, NULL, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_rmattrs() */


/** 
 * @brief Get a named attribute of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_getattr method fetches a named attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param name @input the name of the attr to get.
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 *
 * @returns the object attribute in the result field.
 */
int lwfs_getattr(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name name, 
		const lwfs_cap *cap,
		lwfs_attr *attr, 
		lwfs_request *req)
{
	int rc = LWFS_OK;
	
	ss_getattr_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_getattr_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.name = (lwfs_name)name;
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_GETATTR, 
			&args, NULL, 0, attr, req);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}
	
	return rc;
} /* lwfs_getattr() */


/** 
 * @brief Set the attribute of an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_setattr method sets a named attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param attr @input the attribute to set.
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 */
int lwfs_setattr(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name name,
		const void *value,
		const int size, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;
	
	lwfs_attr attr;

	ss_setattr_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}
	
	memset(&attr, 0, sizeof(lwfs_attr));
	attr.name = calloc(1, sizeof(char)*(strlen(name)+1));
	strcpy(attr.name, name);
	attr.value.lwfs_attr_data_len = size;
	attr.value.lwfs_attr_data_val = calloc(1, sizeof(char)*size);
	memcpy(attr.value.lwfs_attr_data_val, value, size);

	/* initialize the args */
	memset(&args, 0, sizeof(ss_setattrs_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.attr = (lwfs_attr *)&attr;
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_SETATTR, 
			&args, NULL, 0, NULL, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

cleanup:
	free(attr.value.lwfs_attr_data_val);
	free(attr.name);

	return rc;
} /* lwfs_setattrs() */


/** 
 * @brief Remove an attribute from an object. 
 *
 * @ingroup ss_api
 *  
 * The \b lwfs_rmattr method removes named attribute
 * (defined in \ref lwfs_obj_attr) of a 
 * specified storage server object.
 *
 * @param txn_id @input transaction ID.
 * @param obj @input the object. 
 * @param name @input the name of the attr to remove.
 * @param cap @input the capability that allows the operation.
 * @param req @output the request handle (used to test for completion)
 */
int lwfs_rmattr(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_name name, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;
	ss_rmattr_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_rmattrs_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.name = (lwfs_name)name;
	args.cap = (lwfs_cap *)cap; 

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_RMATTR, 
			&args, NULL, 0, NULL, req);

	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_rmattrs() */


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
int lwfs_truncate(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_ssize size, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_truncate_args args;

	/* initialize the storage client (if necessary) */
	if (ss_init() != LWFS_OK) {
		log_error(ss_debug_level, "failed to initialize storage client");
		return rc;
	}

	/* initialize the args */
	memset(&args, 0, sizeof(ss_truncate_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.obj = (lwfs_obj *)obj;
	args.size = size;
	args.cap = (lwfs_cap *)cap; 

	/* the set size method only operates on file objects */
	if (obj->type != LWFS_FILE_OBJ) {
		log_error(ss_debug_level, "invalid object type");
		return LWFS_ERR_NOTFILE;
	}

	/* send a request to execute the remote procedure */
	rc = lwfs_call_rpc(&obj->svc, LWFS_OP_TRUNCATE, 
			&args, NULL, 0, NULL, req);
		if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to call remote method: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	return rc;
} /* lwfs_truncate() */
