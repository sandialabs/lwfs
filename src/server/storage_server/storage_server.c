/**  
 *   @file storage_srvr.c
 * 
 *   This defines the basic functionality of the storage server.  The
 *   storage server does not yet support: recovery from a crash,
 *   recognition of existing files, journals, or locks.
 *
 *   The storage server uses a private object api having function
 *   calls that begin with "my_objs_".  See ss_obj.h and ss_obj.c;
 * 
 *   @author Bill Lawry (wflawry\@sandia.gov)
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 */

#include "config.h"

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#include <sys/stat.h>  /* mkdir */
#include <sys/types.h> /* mkdir */
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <aio.h>

#include "storage_server.h"
#include "storage_db.h"
#include "io_threads.h"
#include "buffer_stack.h"
#include "aio_obj.h"
#include "sysio_obj.h"
#include "ebofs_obj.h"

#include "client/authr_client/authr_client_sync.h"
#include "support/trace/trace.h"

FILE *log_file = NULL;

static lwfs_service _storage_svc; 
static lwfs_service _authr_svc; 
static struct buffer_stack _buffer_stack; 
static int _bufsize;  /* size of a buffer in the buffer stack */

static enum ss_iolib _iolib=SS_SYSIO; 

static struct obj_funcs _obj_funcs; 

struct ss_counter {
    long create_obj;
    long remove_obj;
    long read;
    long write;
    long fsync;
    long listattrs;
    long getattrs;
    long setattrs;
    long rmattrs;
    long listattr;
    long getattr;
    long setattr;
    long rmattr;
    long stat;
    long trunc;
};

static struct ss_counter ss_counter; 

static const lwfs_svc_op _op_array[] = {
	{
		LWFS_OP_CREATE,
		(lwfs_rpc_proc)&ss_create_obj, 
		sizeof(ss_create_obj_args), 
		(xdrproc_t)&xdr_ss_create_obj_args, 
		sizeof(void),
		(xdrproc_t)&xdr_void 
	},
	{
		LWFS_OP_REMOVE,
		(lwfs_rpc_proc)&ss_remove_obj, 
		sizeof(ss_remove_obj_args), 
		(xdrproc_t)&xdr_ss_remove_obj_args, 
		sizeof(void),
		(xdrproc_t)&xdr_void 
	},

	{
		LWFS_OP_READ, 
		(lwfs_rpc_proc)&ss_read, 
		sizeof(ss_read_args), 
		(xdrproc_t)&xdr_ss_read_args, 
		sizeof(lwfs_size), 
		(xdrproc_t)&xdr_lwfs_size 
	},
	{
		LWFS_OP_WRITE, 
		(lwfs_rpc_proc)&ss_write, 
		sizeof(ss_write_args), 
		(xdrproc_t)&xdr_ss_write_args, 
		sizeof(void), 
		(xdrproc_t)&xdr_void 
	},
	{
		LWFS_OP_FSYNC, 
		(lwfs_rpc_proc)&ss_fsync, 
		sizeof(ss_fsync_args), 
		(xdrproc_t)&xdr_ss_fsync_args, 
		sizeof(void), 
		(xdrproc_t)&xdr_void 
	},
	{
		LWFS_OP_LISTATTRS,
		(lwfs_rpc_proc)&ss_listattrs,
		sizeof(ss_listattrs_args),
		(xdrproc_t)&xdr_ss_listattrs_args,
		sizeof(lwfs_name_array),
		(xdrproc_t)&xdr_lwfs_name_array, 
	},
	{
		LWFS_OP_GETATTRS,
		(lwfs_rpc_proc)&ss_getattrs,
		sizeof(ss_getattrs_args),
		(xdrproc_t)&xdr_ss_getattrs_args,
		sizeof(lwfs_attr_array),
		(xdrproc_t)&xdr_lwfs_attr_array, 
	},
	{
		LWFS_OP_SETATTRS,
		(lwfs_rpc_proc)&ss_setattrs,
		sizeof(ss_setattrs_args),
		(xdrproc_t)&xdr_ss_setattrs_args,
		sizeof(void),
		(xdrproc_t)&xdr_void, 
	},
	{
		LWFS_OP_RMATTRS,
		(lwfs_rpc_proc)&ss_rmattrs,
		sizeof(ss_rmattrs_args),
		(xdrproc_t)&xdr_ss_rmattrs_args,
		sizeof(void),
		(xdrproc_t)&xdr_void, 
	},
	{
		LWFS_OP_GETATTR,
		(lwfs_rpc_proc)&ss_getattr,
		sizeof(ss_getattr_args),
		(xdrproc_t)&xdr_ss_getattr_args,
		sizeof(lwfs_attr),
		(xdrproc_t)&xdr_lwfs_attr, 
	},
	{
		LWFS_OP_SETATTR,
		(lwfs_rpc_proc)&ss_setattr,
		sizeof(ss_setattr_args),
		(xdrproc_t)&xdr_ss_setattr_args,
		sizeof(void),
		(xdrproc_t)&xdr_void, 
	},
	{
		LWFS_OP_RMATTR,
		(lwfs_rpc_proc)&ss_rmattr,
		sizeof(ss_rmattr_args),
		(xdrproc_t)&xdr_ss_rmattr_args,
		sizeof(void),
		(xdrproc_t)&xdr_void, 
	},
	{
		LWFS_OP_STAT,
		(lwfs_rpc_proc)&ss_stat,
		sizeof(ss_stat_args),
		(xdrproc_t)&xdr_ss_stat_args,
		sizeof(lwfs_stat_data),
		(xdrproc_t)&xdr_lwfs_stat_data, 
	},
	{
		LWFS_OP_TRUNCATE,
		(lwfs_rpc_proc)&ss_truncate,
		sizeof(ss_truncate_args),
		(xdrproc_t)&xdr_ss_truncate_args,
		sizeof(void),
		(xdrproc_t)&xdr_void 
	},
	{LWFS_OP_NULL}
};

const lwfs_svc_op *lwfs_ss_op_array()
{
	return _op_array;
}

/* ---- Private methods ---- */

static int ss_verify_cap(
	const lwfs_cap *cap, 
	const lwfs_obj *obj, 
	const lwfs_container_op container_op) 
{
    int rc = LWFS_OK; 
    lwfs_cid real_cid; 
    lwfs_attr cid_attr; 
    
    memset(&real_cid, 0, sizeof(lwfs_cid));
    memset(&cid_attr, 0, sizeof(lwfs_attr));

    log_debug(ss_debug_level, "entered ss_verify_cap");

    /* make sure the container OP is non-zero */
    if (container_op == 0) {
	rc = LWFS_ERR; 
	log_error(ss_debug_level, "container_op == 0");
	goto cleanup; 
    }


    log_debug(ss_debug_level, "verify cap with cid=%d for ops=%d",
	    cap->data.cid, (int)container_op);

    /* verify the capabilitiy */
    /* first we make sure the container_op in the cap matches the 
     * operation we want to perform. */
    if ((container_op & cap->data.container_op) != container_op) {
	log_error(ss_debug_level, "operation does not match cap container_op");
	rc = LWFS_ERR_ACCESS;
	goto cleanup;
    }

//    /* get the real container ID for the object */
//    if (obj != NULL) {
//	rc = _obj_funcs.getattr(obj, "_lwfs_cid", &cid_attr);
//	if (rc != LWFS_OK) {
//	    log_error(ss_debug_level, "could not get cid attribute");
//	    rc = LWFS_ERR_STORAGE;
//	    goto cleanup;
//	}
//	memcpy(&real_cid, cid_attr.value.lwfs_attr_data_val, sizeof(lwfs_cid));
//
//	/* Make sure the container ID for the capability matches
//	 * the real container ID of the stored object */
//	if (real_cid != cap->data.cid) {
//	    rc = LWFS_ERR_ACCESS; 
//	    log_warn(ss_debug_level, "real_cid=%d does not match cap->data.cid=%d",
//		    real_cid, cap->data.cid);
//	    goto cleanup; 
//	}
//    }

    /* Last step is to call verify the cap with the authr svc */
    log_debug(ss_debug_level, "verify caps with authorization svc");
    rc = lwfs_verify_caps_sync(&_authr_svc, cap, 1); 
    if (rc != LWFS_OK) {
	log_error(ss_debug_level, "could not call verify caps: %s",
		lwfs_err_str(rc));
	goto cleanup; 
    }

cleanup:
    if (cid_attr.value.lwfs_attr_data_val)
        free(cid_attr.value.lwfs_attr_data_val);
    if (cid_attr.name)
        free(cid_attr.name);

    return rc; 
}





//static int ss_

/* ---- LWFS_SS methods ---- */

int storage_server_init(
		const char *db_path,
		const lwfs_bool db_clear,
		const lwfs_bool db_recover,
		const char *iolib_str,
		const char *root,
		const int num_bufs,
		const lwfs_size bufsize, 
		const lwfs_service *a_svc,
		lwfs_service *svc)
{
	static volatile lwfs_bool initialized = FALSE; 

	int rc = LWFS_OK; 
	int i; 


	if (initialized) {
		return rc;
	}
	else {
		initialized = TRUE;
	}

	/* initialize the counters */
	memset(&ss_counter, 0, sizeof(struct ss_counter));

	trace_reset_count(TRACE_SS_CREATE_OBJ, 0, "init create count");
	trace_reset_count(TRACE_SS_REMOVE_OBJ, 0, "init remove count");
	trace_reset_count(TRACE_SS_READ, 0, "init read count");
	trace_reset_count(TRACE_SS_WRITE, 0, "init write count");
	trace_reset_count(TRACE_SS_FSYNC, 0, "init fsync count");
	trace_reset_count(TRACE_SS_LISTATTR, 0, "init listattr count");
	trace_reset_count(TRACE_SS_GETATTR, 0, "init getattr count");
	trace_reset_count(TRACE_SS_SETATTR, 0, "init setattr count");
	trace_reset_count(TRACE_SS_RMATTR, 0, "init rmattr count");
	trace_reset_count(TRACE_SS_STAT, 0, "init stat count");
	trace_reset_count(TRACE_SS_TRUNC, 0, "init trunc count");


	/* configure the sysio library */
	if (strcmp(iolib_str, "sysio") == 0) {

	    _iolib = SS_SYSIO;
	    rc = sysio_obj_init(root, &_obj_funcs); 
	    if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not initialize stdio objects");
		return rc; 
	    }

	    /* initialize the database for the attributes */
	    rc = ss_db_init(db_path, db_clear, db_recover);
	    if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to initialize the ss attr db: %s",
			lwfs_err_str(rc));
		return rc;
	    }
	}

	/* configure the aio library */
	else if (strcmp(iolib_str, "aio") == 0) {
		_iolib = SS_AIO;
		rc = aio_obj_init(root, &_obj_funcs); 
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not initialize aio objects");
			return rc; 
		}
	}

	/* configure the sim library */
	else if (strcmp(iolib_str, "sim") == 0) {
		_iolib = SS_SIMIO;
		/*
		   rc = sim_obj_init(ss_opts->root, &_obj_funcs); 
		   if (rc != LWFS_OK) {
		   log_error(ss_debug_level, "could not initialize aio objects");
		   return rc; 
		   }
		 */
	}

	/* configure the ebofs library */
	else if (strcmp(iolib_str, "ebofs") == 0) {
		_iolib = SS_EBOFS; 

#ifdef HAVE_EBOFS
		rc = ebofs_obj_init(root, &_obj_funcs);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not initialize ebofs obj library");
			return rc; 
		}
#else
		log_error(ss_debug_level, "EBOFS is not available");
		return LWFS_ERR_NOTSUPP;
#endif

	}

	else {
		log_error(ss_debug_level, "library \"%s\" not supported", _iolib);
		return LWFS_ERR_NOTSUPP; 
	}

	log_debug(ss_debug_level, "allocating %d buffers for the I/O threads", 
			num_bufs);

	/* initialize the buffer stack */
	rc = buffer_stack_init(&_buffer_stack); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not initialize buffer stack");
		return rc; 
	}

	/* fill the buffer stack */
	for (i=0; i<num_bufs; i++) {

		/* allocate the buffer structure */
		struct io_buffer *iobuf = (struct io_buffer *)
			malloc(sizeof(struct io_buffer));
		if (!iobuf) {
			log_error(ss_debug_level, "unable to allocate io buffer structure");
			rc = LWFS_ERR_NOSPACE;
			goto abort; 
		}

		/* allocate the buffer */
		iobuf->buf = malloc(bufsize);
		if (!iobuf->buf) {
			log_error(ss_debug_level, "unable to allocate io buffer");
			rc = LWFS_ERR_NOSPACE;
			goto abort; 
		}

		/* store bufsize in a global variable */
		_bufsize = bufsize; 
		iobuf->len = bufsize; 

		/* push onto the stack */
		rc = buffer_stack_push(&_buffer_stack, iobuf);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "unable to push buffer onto stack");
			goto abort; 
		}
	}

	/* initialize the service to receive requests */
	rc = lwfs_service_init(LWFS_SS_MATCH_BITS, LWFS_SHORT_REQUEST_SIZE, svc); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to initialize service");
		return rc; 
	}

	/* copy the 15 storage server ops into our list of supported operations */
	rc = lwfs_service_add_ops(svc, lwfs_ss_op_array(), 15);
	if (rc != LWFS_OK) {
		log_fatal(ss_debug_level, "Could not add storage server ops");
		return rc; 
	}

	/* save the service descriptors */
	memcpy(&_storage_svc, svc, sizeof(lwfs_service)); 
	memcpy(&_authr_svc, a_svc, sizeof(lwfs_service)); 

	

	return rc; 

abort:
	return rc; 
} 


/**
 * @brief Finalize the lwfs storage server. 
 */
int storage_server_fini(
	const lwfs_service *svc)
{
    int rc = LWFS_OK;

    /* finialize the service library */
    rc = lwfs_service_fini(svc);
    if (rc != LWFS_OK) {
	log_error(ss_debug_level, "unable to finalize service: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    /* cleanup the different IO libraries */
    switch (_iolib) {
	case SS_SYSIO:
	    rc = sysio_obj_fini(); 
	    if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not finialize sysio objects");
		return rc; 
	    }
	    break;

	case SS_AIO:
	    rc = aio_obj_fini(); 
	    if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not finialize aio objects");
		return rc; 
	    }
	    break;

	case SS_SIMIO:
	    /*
	       rc = sim_obj_fini(); 
	       if (rc != LWFS_OK) {
	       log_error(ss_debug_level, "could not finialize aio objects");
	       return rc; 
	       }
	     */
	    break;

	case SS_EBOFS:
	    break;

	default:
	    log_error(ss_debug_level, "library %d not supported", _iolib);
	    return LWFS_ERR_NOTSUPP; 
    }

    /* stop the io_threads */
    /*
       rc = stop_io_threads();
       if (rc != LWFS_OK){
       log_error(ss_debug_level, "could not stop IO threads");
       return rc; 
       }
     */

    /* clean up the buffer stack */
    rc = buffer_stack_destroy(&_buffer_stack);
    if (rc != LWFS_OK){
	log_error(ss_debug_level, "could not destroy buffer stack");
	return rc;
    }

    /* print out ss statistics */
    fprintf(logger_get_file(), "--- Storage Service Stats ---\n");
    fprintf(logger_get_file(), "  Operation counts:\n");
    fprintf(logger_get_file(), "\tcreate_obj = %ld\n", ss_counter.create_obj);
    fprintf(logger_get_file(), "\tremove_obj = %ld\n", ss_counter.remove_obj);
    fprintf(logger_get_file(), "\tread = %ld\n", ss_counter.read);
    fprintf(logger_get_file(), "\twrite = %ld\n", ss_counter.write);
    fprintf(logger_get_file(), "\tfsync = %ld\n", ss_counter.fsync);
    fprintf(logger_get_file(), "\tlistattr = %ld\n", ss_counter.listattrs+ss_counter.listattr);
    fprintf(logger_get_file(), "\tgetattr = %ld\n", ss_counter.getattrs+ss_counter.getattr);
    fprintf(logger_get_file(), "\tsetattr = %ld\n", ss_counter.setattrs+ss_counter.setattr);
    fprintf(logger_get_file(), "\trmattr = %ld\n", ss_counter.rmattrs+ss_counter.rmattr);
    fprintf(logger_get_file(), "\tstat = %ld\n", ss_counter.stat);
    fprintf(logger_get_file(), "\ttrunc = %ld\n", ss_counter.trunc);
    fprintf(logger_get_file(), "-----------------------------\n");

    if (log_file){
	fclose(log_file);
    }

    return rc;
} /* lwfs_remove_obj_srvr() */




/***** the registered functions *****/

/**
 * @brief Create a new object on the storage server. 
 * 
 * This particular implementation of the server-side
 * stub of lwfs_create_object, represents an object as a 
 * single UNIX file in a special directory.
 */
int ss_create_obj(
		const lwfs_remote_pid *caller, 
		const ss_create_obj_args *args, 
		const lwfs_rma *data_addr,
		void *res)
{
    int rc = LWFS_OK; 
    char ostr[33];

    /* extract the arguments */
    /*const lwfs_txn *txn_id = args->txn_id; */
    const lwfs_obj *obj = args->obj;
    const lwfs_cap *cap = args->cap;
    lwfs_cid real_cid = cap->data.cid; 

    ss_counter.create_obj++;
    int interval_id = ss_counter.create_obj; 
    int thread_id = lwfs_thread_pool_getrank();

    trace_start_interval(interval_id, thread_id); 

    log_debug(ss_debug_level, "entered ss_create_obj");

    /* verify the capability */
    rc = ss_verify_cap(cap, NULL, LWFS_CONTAINER_WRITE); 
    if (rc != LWFS_OK) {
	log_error(ss_debug_level, "could not verify capability : %s",
		lwfs_err_str(rc));
	goto cleanup;
    }

    /* create a persistent object */
    rc = _obj_funcs.create(obj); 
    if (rc != LWFS_OK) {
	log_error(ss_debug_level, "unable to create object "
		"(cid=%llu, oid=0x%s): %s", obj->cid, lwfs_oid_to_string(obj->oid, ostr), 
		lwfs_err_str(rc)); 
	goto cleanup; 
    }

    /* create container ID attribute for the object */
    rc = _obj_funcs.setattr(obj, "_lwfs_cid", &real_cid, sizeof(lwfs_cid));  
    if (rc != LWFS_OK) {
	log_error(ss_debug_level, "could not set attribute");
	rc = LWFS_ERR_STORAGE;
	goto cleanup; 
    }

cleanup:

    trace_end_interval(interval_id, TRACE_SS_CREATE_OBJ, thread_id, "create_obj");

    return rc; 
} /* ss_create_object() */


int ss_remove_obj(
		const lwfs_remote_pid *caller, 
		const ss_remove_obj_args *args, 
		const lwfs_rma *data_addr,
		void *res)
{
    int rc = LWFS_OK;

    /* extract the arguments */
    //const lwfs_txn *txn_id = args->txn_id; 
    const lwfs_obj *obj = args->obj;
    const lwfs_cap *cap = args->cap;

    ss_counter.remove_obj++;
    int interval_id = ss_counter.remove_obj; 
    int thread_id = lwfs_thread_pool_getrank(); 

    trace_start_interval(interval_id, thread_id);


    log_debug(ss_debug_level, "entered ss_remove_obj");

    /* do the ssid & vid make sense?  */

    /* verify that the object exists */
    if (!_obj_funcs.exists(obj)) {
	rc = LWFS_ERR_NO_OBJ;
	goto cleanup;
    }

    /* verify the capability */
    rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_WRITE); 
    if (rc != LWFS_OK) {
	log_error(ss_debug_level, "could not verify capability : %s",
		lwfs_err_str(rc));
	goto cleanup;
    }

    rc = _obj_funcs.remove(obj); 
    if (rc != LWFS_OK) {
	log_error(ss_debug_level,"could not remove object");
	goto cleanup;
    }

cleanup:
    trace_end_interval(interval_id, TRACE_SS_REMOVE_OBJ, thread_id, "remove_obj");

    return rc;
} /* ss_remove_object() */

int ss_read(
	const lwfs_remote_pid *caller, 
	const ss_read_args *args, 
	const lwfs_rma *data_addr,
	lwfs_size *res)
{
    int rc = LWFS_OK;
    void *buf = NULL; 
    uint32_t count; 

    /* extract the arguments */
    //const lwfs_txn *txn_id = args->txn_id; 
    const lwfs_obj *src_obj = args->src_obj;
    const lwfs_size src_offset = args->src_offset; 
    const lwfs_size len = args->len; 
    const lwfs_cap *cap = args->cap;

    ss_counter.read++;
    int interval_id = ss_counter.read;
    int thread_id = lwfs_thread_pool_getrank();

    trace_start_interval(interval_id, thread_id); 


    log_debug(ss_debug_level, "entered ss_read");

    /* verify that the object exists */
    if (!_obj_funcs.exists(src_obj)) {
	rc = LWFS_ERR_NO_OBJ;
	goto cleanup; 
    }

    /* verify the capability */
    rc = ss_verify_cap(cap, src_obj, LWFS_CONTAINER_READ); 
    if (rc != LWFS_OK) {
	log_warn(ss_debug_level, "could not verify capability."
		" Returning %s to client",
		lwfs_err_str(rc));
	goto cleanup; 
    }

    /* allocate space for data */
    if (len){
	buf = malloc(len);
	if (buf == NULL) {
	    rc = LWFS_ERR_NOSPACE;
	    log_warn(ss_debug_level, 
		    "could not malloc a read buffer (len==%d)"
		    " Returning %s to client", len,
		    lwfs_err_str(rc));
	    goto cleanup; 
	}
    }

    /* read the data */
    count = _obj_funcs.read(src_obj, src_offset, 
	    buf, args->len);
    if (count == -1) {
	rc = LWFS_ERR_STORAGE;
	*res = 0; 
	goto cleanup; 
    }

    /* send the data to the client along the special data path */
    if (count > 0){
	rc = lwfs_put_data(buf, count, data_addr); 
	if (rc != LWFS_OK) {
	    log_error(ss_debug_level, "could not \"put\" data on client: %s",
		    lwfs_err_str(rc));
	}
    }

    *res = count; 

cleanup:
//malloc_report();

    trace_end_interval(interval_id, TRACE_SS_READ, thread_id, "read");

    if (buf != NULL) free(buf); 
    return rc;

} /* ss_read() */



int ss_write(
		const lwfs_remote_pid *caller, 
		const ss_write_args *args, 
		const lwfs_rma *data_addr,
		void *res)
{
	int rc = LWFS_OK;
	ssize_t bytes_written = 0; 

	/* extract the arguments */
	//const lwfs_txn *txn_id = args->txn_id; 
	const lwfs_obj *dest_obj = args->dest_obj;
	const lwfs_size dest_offset = args->dest_offset; 
	const lwfs_size len = args->len; 
	const lwfs_cap *cap = args->cap;

	/* allocate the buffer 
	 * TODO: Where should this buffer be freed? 
	 * Right now it is in the write func. This 
	 * is necessary in case we are using threaded
	 * I/O operations. 
	 */
	void *buf = malloc(len); 

	ss_counter.write++;
	int interval_id = ss_counter.write; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id);

	log_debug(ss_debug_level, "entered ss_write");

	/* verify that the object exists */
	if (!_obj_funcs.exists(dest_obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, dest_obj, LWFS_CONTAINER_WRITE); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* Get the data from the client */
	log_debug(ss_debug_level, "thread %d: start transferring data\n", 
			lwfs_thread_pool_getrank());
	rc = lwfs_get_data(buf, len, data_addr); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to fetch data: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	log_debug(ss_debug_level, "thread %d: finished transferring data", 
				lwfs_thread_pool_getrank() );

	bytes_written = _obj_funcs.write(dest_obj, dest_offset, buf, len);
	if (bytes_written != len) {
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

cleanup: 

	trace_end_interval(interval_id, TRACE_SS_WRITE, thread_id, "write");

	/* check for an error before we free the buffer */
	if (rc != LWFS_OK) {
	    if (buf) free(buf);
	}
	return rc;

}/* ss_write() */


int ss_fsync(
        const lwfs_remote_pid *caller, 
        const ss_fsync_args *args, 
        const lwfs_rma *data_addr,
        void *res)
{
	int rc = LWFS_OK;
	lwfs_obj *obj = args->obj; 
	lwfs_cap *cap = args->cap; 


	ss_counter.fsync++;
	int interval_id = ss_counter.fsync; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_WRITE); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	rc = _obj_funcs.fsync(obj); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to fsync");
		goto cleanup;
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_FSYNC, thread_id, "fsync");

	return rc; 
}

int ss_listattrs(
        const lwfs_remote_pid *caller, 
        const ss_listattrs_args *args, 
        const lwfs_rma *data_addr,
        lwfs_name_array *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;

	ss_counter.listattrs++;
	int interval_id = ss_counter.listattrs; 
	int thread_id = lwfs_thread_pool_getrank();

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_listattr");

	/* make sure the function is supported */
	if (_obj_funcs.listattrs == NULL) {
		log_error(ss_debug_level, "listattr not supported");
		rc = LWFS_ERR_NOTSUPP;
		goto cleanup; 
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup; 
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_READ); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/* list attribute names */
	rc = _obj_funcs.listattrs(obj, res);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not get attributes");
		rc = LWFS_ERR_STORAGE;
		goto cleanup; 
	}

cleanup:

	trace_end_interval(interval_id, TRACE_SS_LISTATTR, thread_id, "listattr");

	return rc;
} /* ss_listattrs() */

int ss_getattrs(
        const lwfs_remote_pid *caller, 
        const ss_getattrs_args *args, 
        const lwfs_rma *data_addr,
        lwfs_attr_array *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;
	const lwfs_name_array *names = (const lwfs_name_array *)args->names;

	ss_counter.getattrs++;
	int interval_id = ss_counter.getattrs; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_getattrs");

	/* make sure the function is supported */
	if (_obj_funcs.getattrs == NULL) {
		log_error(ss_debug_level, "getattrs not supported");
		rc = LWFS_ERR_NOTSUPP;
		goto cleanup;
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_READ); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* get attributes */
	rc = _obj_funcs.getattrs(obj, names, res);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not get attributes");
		rc = LWFS_ERR_STORAGE;
		goto cleanup; 
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_GETATTR, thread_id, "getattr");

	return rc;
} /* ss_getattrs() */

int ss_setattrs(
        const lwfs_remote_pid *caller, 
        const ss_setattrs_args *args, 
        const lwfs_rma *data_addr,
        void *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;
	const lwfs_attr_array *attrs = (const lwfs_attr_array *)args->attrs;

	ss_counter.setattrs++;
	int interval_id = ss_counter.setattrs; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_setattrs");

	/* make sure the function is supported */
	if (_obj_funcs.setattrs == NULL) {
		log_error(ss_debug_level, "setattrs not supported");
		rc =  LWFS_ERR_NOTSUPP;
		goto cleanup;
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_WRITE); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* set attributes */
	rc = _obj_funcs.setattrs(obj, attrs);  
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not set attributes");
		rc = LWFS_ERR_STORAGE;
		goto cleanup; 
	}

cleanup:

	trace_end_interval(interval_id, TRACE_SS_SETATTR, thread_id, "setattrs");

	return rc;
} /* ss_setattrs() */


int ss_rmattrs(
        const lwfs_remote_pid *caller, 
        const ss_rmattrs_args *args, 
        const lwfs_rma *data_addr,
        void *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;
	const lwfs_name_array *names = (const lwfs_name_array *)args->names;

	ss_counter.rmattrs++;
	int interval_id = ss_counter.rmattrs; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_rmattrs");

	/* make sure the function is supported */
	if (_obj_funcs.rmattrs == NULL) {
		log_error(ss_debug_level, "rmattrs not supported");
		rc = LWFS_ERR_NOTSUPP;
		goto cleanup;
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_READ); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* get attributes */
	rc = _obj_funcs.rmattrs(obj, names);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not remove attributes");
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_RMATTR, thread_id, "rmattrs");
	return rc;
} /* ss_rmattrs() */


int ss_getattr(
        const lwfs_remote_pid *caller, 
        const ss_getattr_args *args, 
        const lwfs_rma *data_addr,
        lwfs_attr *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;
	const lwfs_name name = (const lwfs_name)args->name;

	ss_counter.getattr++;
	int interval_id = ss_counter.getattr; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 


	log_debug(ss_debug_level, "entered ss_getattr");

	/* make sure the function is supported */
	if (_obj_funcs.getattr == NULL) {
		log_error(ss_debug_level, "getattr not supported");
		rc = LWFS_ERR_NOTSUPP;
		goto cleanup; 
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup; 
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_READ); 
	if (rc != LWFS_OK) {
	    log_error(ss_debug_level, "could not verify capability : %s",
		    lwfs_err_str(rc));
	    goto cleanup; 
	}

	/* get attributes */
	rc = _obj_funcs.getattr(obj, name, res);
	if (rc != LWFS_OK) {
	    log_error(ss_debug_level, "could not get attribute");
	    rc = LWFS_ERR_STORAGE;
	    goto cleanup; 
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_GETATTR, thread_id, "getattr");

	return rc;
} /* ss_getattr() */

int ss_setattr(
        const lwfs_remote_pid *caller, 
        const ss_setattr_args *args, 
        const lwfs_rma *data_addr,
        void *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;
	const lwfs_attr *attr = (const lwfs_attr *)args->attr;

	ss_counter.setattr++;
	int interval_id = ss_counter.setattr; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_setattr");

	/* make sure the function is supported */
	if (_obj_funcs.setattr == NULL) {
		log_error(ss_debug_level, "setattr not supported");
		rc =  LWFS_ERR_NOTSUPP;
		goto cleanup;
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_WRITE); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* set attributes */
	rc = _obj_funcs.setattr(obj, attr->name, attr->value.lwfs_attr_data_val, attr->value.lwfs_attr_data_len);  
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not set attribute");
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_SETATTR, thread_id, "setattr");
	return rc;
} /* ss_setattr() */


int ss_rmattr(
        const lwfs_remote_pid *caller, 
        const ss_rmattr_args *args, 
        const lwfs_rma *data_addr,
        void *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	/*const lwfs_txn *txn_id = args->txn_id; */
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;
	const lwfs_name name = (const lwfs_name)args->name;

	ss_counter.rmattr++;
	int interval_id = ss_counter.rmattr;
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_rmattr");

	/* make sure the function is supported */
	if (_obj_funcs.rmattr == NULL) {
		log_error(ss_debug_level, "rmattr not supported");
		rc = LWFS_ERR_NOTSUPP;
		goto cleanup;
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup;
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_READ); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* get attributes */
	rc = _obj_funcs.rmattr(obj, name);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not remove attribute");
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_RMATTR, thread_id, "rmattr");

	return rc;
} /* ss_rmattr() */

int ss_stat(
        const lwfs_remote_pid *caller, 
        const ss_stat_args *args, 
        const lwfs_rma *data_addr,
        lwfs_stat_data *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	//const lwfs_txn *txn_id = args->txn_id; 
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;

	ss_counter.stat++;
	int interval_id = ss_counter.stat; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_stat");

	/* make sure the function is supported */
	if (_obj_funcs.stat == NULL) {
		log_error(ss_debug_level, "stat not supported");
		rc =  LWFS_ERR_NOTSUPP;
		goto cleanup; 
	}

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup; 
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_READ); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/* set attributes */
	rc = _obj_funcs.stat(obj, res);  
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not get attributes");
		rc = LWFS_ERR_STORAGE;
		goto cleanup; 
	}

cleanup: 
	trace_end_interval(interval_id, TRACE_SS_STAT, thread_id, "stat");
	return rc;
} /* ss_get_attr() */


int ss_truncate(
		const lwfs_remote_pid *caller, 
		const ss_truncate_args *args, 
		const lwfs_rma *data_addr, 
		void *res)
{
	int rc = LWFS_OK;

	/* extract the arguments */
	//const lwfs_txn *txn_id = args->txn_id; 
	const lwfs_obj *obj = args->obj;
	const lwfs_ssize size = args->size; 
	const lwfs_cap *cap = args->cap;

	ss_counter.trunc++;
	int interval_id = ss_counter.trunc; 
	int thread_id = lwfs_thread_pool_getrank(); 

	trace_start_interval(interval_id, thread_id); 

	log_debug(ss_debug_level, "entered ss_truncate");

	/* verify that the object exists */
	if (!_obj_funcs.exists(obj)) {
		rc = LWFS_ERR_NO_OBJ;
		goto cleanup; 
	}

	/* verify the capability */
	rc = ss_verify_cap(cap, obj, LWFS_CONTAINER_WRITE); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not verify capability : %s",
			lwfs_err_str(rc));
		goto cleanup; 
	}


	/* change the size of the object */
	rc = _obj_funcs.trunc(obj, size);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not get attributes");
		rc = LWFS_ERR_STORAGE;
		goto cleanup; 
	}

cleanup:
	trace_end_interval(interval_id, TRACE_SS_TRUNC, thread_id, "trunc");
	return rc;
} /* ss_get_attr() */

