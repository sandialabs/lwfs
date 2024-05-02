/**  
 *   @file storage_clnt.c
 * 
 *   @brief Implementation of the client-side methods for the 
 *   storage server API. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Bill Lawry (wflawry\@sandia.gov)
 *   $Revision: 738 $
 *   $Date: 2006-06-27 09:01:24 -0600 (Tue, 27 Jun 2006) $
 */

#include <errno.h>

#include "common/types/types.h"
#include "common/types/fprint_types.h"

#include "common/storage_common/ss_xdr.h"
#include "common/storage_common/ss_clnt.h"
#include "common/storage_common/ss_debug.h"
#include "sysio_obj.h"
#include "ss_srvr.h" /* until we have the rpc layer, we call the funcs directly */

static lwfs_bool local_calls = FALSE;
static lwfs_service storage_svc;  /* only used if local */


/* ---- Private methods ---- */


/** 
 * @brief Read the storage server input file.
 */
static int read_ss_file(
        const char *fname,
        int num_ss,
        lwfs_remote_pid *ss)
{
    int i; 
    int rc = LWFS_OK; 
    FILE *fp; 

    fp = fopen(fname, "r");
    if (!fp) {
        log_error(ss_debug_level, "unable to open ss_file \"%s\": %s", fname, strerror(errno));
        return LWFS_ERR; 
    }

    for (i=0; i<num_ss; i++) {
        rc = fscanf(fp, "%u %u", &ss[i].nid, &ss[i].pid);
        if (rc != 2) {
            log_error(ss_debug_level, "errors reading ss_file: line %d", i);
            rc = LWFS_ERR; 
            goto cleanup;
        }
        rc = LWFS_OK;
    }

cleanup: 
    fclose(fp); 
    return rc; 
}



/**
 * @brief Initialize a storage server client.
 *
 * Performs any required initialization procedures before
 * calling the storage server client methods.
 *
 * @param pid @input the process ID to use for the client. 
 */
int lwfs_ss_clnt_init(
		struct ss_options *ss_opts, 
		struct authr_options *authr_opts,
		lwfs_service *authr_svc,
		lwfs_service **svc_array) 
{
	static lwfs_bool initialized = FALSE; 
	int rc = LWFS_OK; 
	lwfs_remote_pid my_id; 

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

    /* initialize the options for the buffer stack */
    if (!ss_opts->bufsize) {
        ss_opts->bufsize = DEFAULT_SS_BUFSIZE;
    }
    if (!ss_opts->num_bufs) {
        ss_opts->num_bufs = DEFAULT_SS_NUM_BUFS;
    }

	/* initialize this process as an RPC client */
	rc = lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to initialize RPC client: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	lwfs_get_id(&my_id); 


	/* do this if we do want a local storage server */
	if (ss_opts->local) {
		ss_opts->num_servers = 1;  // can only have one server 

		/* allocate the server id */
		ss_opts->server_ids = 
			(lwfs_remote_pid *)malloc(1*sizeof(lwfs_remote_pid));

		*svc_array = (lwfs_service *)malloc(sizeof(lwfs_service)); 

		/* set the IDs of this server  */
		ss_opts->server_ids[0].nid = my_id.nid; 
		ss_opts->server_ids[0].pid = my_id.pid; 

		/* set the local_calls global variable */ 
		local_calls = TRUE; 

		/* initialize the storage server */
		rc = lwfs_ss_srvr_init(ss_opts, authr_opts, authr_svc, *svc_array); 
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to initialize local storage svc: %s",
					lwfs_err_str(rc));
			return rc; 
		}

		/* save svc for later */
		memcpy(&storage_svc, *svc_array, sizeof(lwfs_service));
	}

	/* do this if we want to access several remote servers */
	else {
		int i; 
		lwfs_service *ss_svc = NULL;

		/* if the server file is null, assume the server is on this host */
		if (ss_opts->server_file == NULL) {
			ss_opts->num_servers = 1; 
			*svc_array = (lwfs_service *)malloc(sizeof(lwfs_service));
			ss_opts->server_ids = (lwfs_remote_pid *)malloc(sizeof(lwfs_remote_pid)); 

			ss_opts->server_ids[0].pid = LWFS_SS_PID;
			ss_opts->server_ids[0].nid = my_id.nid; 
		}
		else {
			ss_opts->server_ids = (lwfs_remote_pid *)
				malloc(ss_opts->num_servers * sizeof(lwfs_remote_pid));

			*svc_array = (lwfs_service *)malloc(ss_opts->num_servers * sizeof(lwfs_service));


			/* read the list of server ids */
			rc = read_ss_file(ss_opts->server_file, 
					ss_opts->num_servers, ss_opts->server_ids);
			if (rc != LWFS_OK) {
				log_error(ss_debug_level, "%s", lwfs_err_str(rc));
				return rc; 
			}
		}

		ss_svc = *svc_array; 
		for (i=0; i<ss_opts->num_servers; i++) {
			/* Ping the storage server to get the service information */
			rc = lwfs_get_service(ss_opts->server_ids[i], &ss_svc[i]); 
			if (rc != LWFS_OK) {
				log_error(ss_debug_level, "unable to get authr svc");
				return rc; 
			}
		}
	}

	initialized = TRUE; 

	return rc; 
}

/**
 * @brief Finalize a storage server client.
 *
 * Performs any required shutdown procedures before
 * calling any of the storage server client API.
 */
int lwfs_ss_clnt_fini(struct ss_options *ss_opts, lwfs_service **svc_array)
{
	int rc = LWFS_OK; 

	if (ss_opts != NULL)
	{
		if (ss_opts->local) {
			/* close the storage server */
			rc = lwfs_ss_srvr_fini(&storage_svc); 
		}
		else {
			/* free the storage service array */
			free(*svc_array); 
			*svc_array = NULL; 
	
			/* free the server IDs */
			free(ss_opts->server_ids); 
			ss_opts->server_ids = NULL; 
		}
	}

	rc = lwfs_rpc_fini();

	return rc; 
}


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
int lwfs_create_obj(
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid, 
		const lwfs_cap *cap, 
		lwfs_obj *res,
		lwfs_request *req)
{
	int rc = LWFS_OK;  /* return code */

	ss_create_obj_args args;

	memset(&args, 0, sizeof(ss_create_obj_args));
	memset(res, 0, sizeof(lwfs_obj));

	/* initialize the arguments */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.cid    = cid; 
	args.cap    = (lwfs_cap *)cap; 

	log_debug(ss_debug_level, "calling rpc for create_obj");


	if (local_calls) {
		/* call the server-side stub directly */
		rc = ss_create_obj(NULL, &args, NULL, res);
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
		/* send an rpc request */
		rc = lwfs_call_rpc(svc, LWFS_OP_CREATE_OBJ, 
				&args, NULL, 0, res, req);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc; 
} /* lwfs_create_object() */

int lwfs_bcreate_obj(
		const lwfs_service *svc,
		const lwfs_txn *txn_id,
		const lwfs_cid cid, 
		const lwfs_cap *cap, 
		lwfs_obj *res)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_create_obj(svc, txn_id, cid, cap, res, &req);
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
int lwfs_remove_obj(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_remove_obj_args args;

	/* initialize the arguments */
	args.txn_id = (lwfs_txn *)txn_id; 
	args.obj = (lwfs_obj *)obj;
	args.cap = (lwfs_cap *)cap; 

	log_debug(ss_debug_level, "calling rpc for lwfs_remove_obj");


	if (local_calls) {
		/* call the server-side stub directly */
		rc = ss_remove_obj(NULL, &args, NULL, NULL);
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
		rc = lwfs_call_rpc(&obj->svc, LWFS_OP_REMOVE_OBJ,
				&args, NULL, 0, NULL, req);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc;
} 

int lwfs_bremove_obj(
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

	/* initialize the arguments */
	memset(&args, 0, sizeof(ss_read_args));
	args.txn_id = (lwfs_txn *)txn_id;
	args.src_obj = (lwfs_obj *)src_obj; 
	args.src_offset = src_offset; 
	args.len = len; 
	args.cap = (lwfs_cap *)cap; 

	if (local_calls) {
		lwfs_rma data_addr; 

		/* initialize the md for the data */
		memset(&data_addr, 0, sizeof(lwfs_rma));
		data_addr.local_buf = (u_quad_t)buf; 

		/* call the server-side stub directly */
		rc = ss_read(NULL, &args, &data_addr, result);
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
		rc = lwfs_call_rpc(&src_obj->svc, LWFS_OP_READ, 
				&args, buf, len, result, req);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc;
} /* lwfs_read() */


int lwfs_bread(
		const lwfs_txn *txn_id,
		const lwfs_obj *src_obj, 
		const lwfs_size src_offset, 
		void *dest, 
		const lwfs_size len, 
		const lwfs_cap *cap, 
		lwfs_size *result)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;
	lwfs_request req; 

	rc = lwfs_read(txn_id, src_obj, src_offset, dest, len, cap, result, &req);
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

	/* initialize the arguments */
	memset(&args, 0, sizeof(ss_write_args));
	args.txn_id = (lwfs_txn *)txn_id; 
	args.dest_obj = (lwfs_obj *)dest_obj;
	args.dest_offset = dest_offset; 
	args.len = len; 
	args.cap = (lwfs_cap *)cap; 

	if (local_calls) {
		lwfs_rma data_addr; 

		/* initialize the md for the data */
		memset(&data_addr, 0, sizeof(lwfs_rma));
		data_addr.local_buf = (u_quad_t)buf;

		/* call the server-side stub directly */
		rc = ss_write(NULL, &args, &data_addr, NULL);
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
		rc = lwfs_call_rpc(&dest_obj->svc, LWFS_OP_WRITE, 
				&args, (void *)buf, len, NULL, req);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc;

} /* lwfs_write() */

int lwfs_bwrite(
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
	
int lwfs_fsync(
        const lwfs_txn *txn_id,
        const lwfs_obj *obj, 
        const lwfs_cap *cap, 
        lwfs_request *req) 
{
    int rc = LWFS_OK;
    ss_fsync_args args;

    /* initialize the arguments */
    memset(&args, 0, sizeof(ss_fsync_args));
    args.txn_id = (lwfs_txn *)txn_id; 
    args.obj = (lwfs_obj *)obj;
    args.cap = (lwfs_cap *)cap; 

    if (local_calls) {

        /* call the server-side stub directly */
        rc = ss_fsync(NULL, &args, NULL, NULL);
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
        rc = lwfs_call_rpc(&obj->svc, LWFS_OP_FSYNC, 
			&args, NULL, 0, NULL, req);
        if (rc != LWFS_OK) {
            log_error(ss_debug_level, "unable to call remote method: %s",
                    lwfs_err_str(rc));
            return rc; 
        }
    }

    return rc;
}

int lwfs_bfsync(
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
int lwfs_get_obj_attr(
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

	if (local_calls) {
		rc = ss_get_attr(NULL, &args, NULL, res);
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
		rc = lwfs_call_rpc(&obj->svc, LWFS_OP_GET_OBJ_ATTR, 
				&args, NULL, 0, res, req);

		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to call remote method: %s",
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
int lwfs_bget_obj_attr(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_cap *cap, 
		lwfs_obj_attr *res)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK; 
	lwfs_request req; 

	rc = lwfs_get_obj_attr(txn_id, obj, cap, res, &req);
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
int lwfs_truncate(
		const lwfs_txn *txn_id,
		const lwfs_obj *obj, 
		const lwfs_ssize size, 
		const lwfs_cap *cap, 
		lwfs_request *req)
{
	int rc = LWFS_OK;

	ss_truncate_args args;

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

	if (local_calls) {
		/* all other cases call the storage server */
		rc = ss_truncate(NULL, &args, NULL, NULL);
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
		rc = lwfs_call_rpc(&obj->svc, LWFS_OP_TRUNCATE, 
				&args, NULL, 0, NULL, req);

		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to call remote method: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc;
} /* lwfs_truncate() */


int lwfs_btruncate(
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
