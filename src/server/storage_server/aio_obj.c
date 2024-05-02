/**  
 *   @file aio_obj.c
 * 
 *   This is the object api that uses the asychronous i/o library.
 *
 *   This object api is used by the registered storage server
 *   functions.  See storage_srvr.c.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <aio.h>

#include "config.h"

#if STDC_HEADERS
#include <string.h>  /* find memcpy */
#endif

#ifndef HAVE_SIGVAL_T
/*
 *  Among others, Darwin doesn't have this typedef
 */
typedef union sigval sigval_t;
#endif

#include "storage_server.h"
#include "aio_obj.h"
#include "sysio_obj.h"

struct aio_obj_args {
    int rank;
	lwfs_bool free_buf; 
    struct aiocb *aiocb;
};


int aio_obj_init(
        const char *root_dir, 
        struct obj_funcs *obj_funcs)
{
	int rc = LWFS_OK;

	/* initialize sysio object library */
	rc = sysio_obj_init(root_dir, obj_funcs);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to initialize aio");
		return rc; 
	}

	/* set the function pointers to use sysio */
	obj_funcs->exists = sysio_obj_exists; 
	obj_funcs->create = sysio_obj_create; 
	obj_funcs->remove = sysio_obj_remove; 
	obj_funcs->listattrs = NULL;
	obj_funcs->getattrs  = NULL;
	obj_funcs->setattrs  = NULL;
	obj_funcs->rmattrs   = NULL;
	obj_funcs->getattr = NULL;
	obj_funcs->setattr = NULL;
	obj_funcs->rmattr  = NULL;
	obj_funcs->stat = sysio_obj_stat;
	obj_funcs->trunc = sysio_obj_trunc; 

	/* these functions should use the aio library */
	obj_funcs->read = aio_obj_read; 
	obj_funcs->write = aio_obj_write; 
	obj_funcs->fsync = aio_obj_fsync; 

	/* initialize aio */
	/*
	memset(&aioinit, 0, sizeof(struct aioinit));
	aioinit.aio_threads = 1; 
	aioinit.aio_num = 1; 

	aio_init(&aioinit);
	*/

	return rc; 
}

int aio_obj_fini() 
{
    return sysio_obj_fini();
}


/* my_objs_read_obj()
 *
 * assumes that object already exists;
 *
 * return one upon success;
 */
lwfs_ssize aio_obj_read(
		const lwfs_obj *obj, 
		const lwfs_ssize src_offset, 
		void *dest, 
		const lwfs_ssize len)
{
    return sysio_obj_read(obj, src_offset, dest, len);
}


static volatile int pending_writes = 0; 


/** 
 * @brief Callback for asynchronous write. 
 *
 * This function gets called when an asynchronous write completes.
 * 
 */
static void write_callback(sigval_t s)
{
	struct aio_obj_args *args = (struct aio_obj_args *)s.sival_ptr;
	struct aiocb *aiocb = args->aiocb; 

	pending_writes--; 

	log_debug(ss_debug_level, "%d: Finished async write, pending_writes=%d\n", 
			args->rank, pending_writes);

	if (args->free_buf) {
		free((char *)aiocb->aio_buf);
	}

	/* free the aiocb structure */
	free(aiocb);
	
	/* free the args structure */
	free(args);
}


/* my_objs_write_obj()
 *
 * assumes that object already exists;
 *
 * returns one upon success;
 */
lwfs_ssize aio_obj_write(
		const lwfs_obj *obj, 
		const lwfs_ssize offset, 
		void *src, 
		const lwfs_ssize len)
{
	struct aio_obj_args *args = NULL;
	int fd; 


	/* get the file descriptor for the object */
	fd = sysio_obj_getfd(obj); 
	if (fd == -1) {
		log_error(ss_debug_level, "could not get file descriptor");
		return 0;
	}

	args = malloc(sizeof(struct aio_obj_args));
	args->rank = lwfs_thread_pool_getrank(); 
	args->aiocb = (struct aiocb *)malloc(sizeof(struct aiocb));

	/* initialize aoicb */
	memset(args->aiocb, 0, sizeof(struct aiocb));
	args->aiocb->aio_fildes = fd; 
	args->aiocb->aio_offset = offset; 
	args->aiocb->aio_sigevent.sigev_notify = SIGEV_THREAD;
	args->aiocb->aio_sigevent.sigev_notify_function = write_callback;
	args->aiocb->aio_sigevent.sigev_value.sival_ptr = args;

	/* prepare for the write */
	args->aiocb->aio_buf = src;
	args->aiocb->aio_nbytes = len;
	args->free_buf = TRUE;

	if (aio_write(args->aiocb) != 0) {
		log_error(ss_debug_level, "error calling aio_write: %s",
				strerror(errno));
		return 0; 
	}
	pending_writes++; 

	/* if we get here, we succeded */
	return len; 
} 


/* aio_fsync()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 *
 */
int aio_obj_fsync(
	const lwfs_obj *obj)
{
	struct aiocb sync_aiocb; 
	int fd; 

	/* get the file descriptor for the object */
	fd = sysio_obj_getfd(obj); 
	if (fd == -1) {
		log_error(ss_debug_level, "could not get file descriptor");
		return LWFS_ERR_NO_OBJ;
	}

	memset(&sync_aiocb, 0, sizeof(struct aiocb));
	sync_aiocb.aio_fildes = fd;
#ifndef HAVE_FLAG_O_DSYNC
	/*
	 *  Ahh, Mac OS.  Darwin doesn't support the O_DSYNC flag so we fall back 
	 *  to O_SYNC (fsync() behavior as opposed to fdatasync() behavior)
	 */
	if (aio_fsync(O_SYNC, &sync_aiocb) != 0) {
#else
	if (aio_fsync(O_DSYNC, &sync_aiocb) != 0) {
#endif
		log_error(ss_debug_level, "unable to schedule fsync file");
		return LWFS_ERR_STORAGE;
	}

	/* wait for sync to complete */
	while (aio_error(&sync_aiocb) == EINPROGRESS);

	if (aio_return(&sync_aiocb) != 0) { 
		log_error(ss_debug_level, "fsync returned error: %s\n", 
				strerror(errno));
		return LWFS_ERR_STORAGE;
	}
	log_debug(ss_debug_level, "fsync completed, pending=%d\n", pending_writes);

	return LWFS_OK;
} 

