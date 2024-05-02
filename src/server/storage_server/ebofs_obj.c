/**  
 *   @file ebofs_obj.c
 * 
 *   This is the LWFS object api for ebofs objects. 
 *
 *   TODO: Need more descriptive return codes. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 */

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "support/logger/logger.h"
#include "storage_server.h"

#include <ebofs/ebofs.h>
#include "ebofs_obj.h"

/* PATH_STR is used to generate file name paths */
#define PATH_STR "%s/%#08lx"


/* The ebofs file system data structure. */
static struct Ebofs *ebofs = NULL; 

/********** public functions (public to ss_srvr) **********/


/**
  * @brief Initialize the ebofs object library.
  *
  * @param devfs @input_type Path to the ebofs block device. 
  * @param obj_funcs @output_type Pointers to the ebofs functions.
  */
int ebofs_obj_init(
        const char *devfs,
        struct obj_funcs *obj_funcs)
{
	int rc = LWFS_OK;

	/* mount the ebofs FS */
	ebofs = ebofs_mount((char *)devfs); 
	if (ebofs == NULL) {
		log_fatal(ss_debug_level, "Could not mount EBOFS to \"%s\"", devfs);
		rc = LWFS_ERR_NOENT;
	}

	/* assign function pointers */
	obj_funcs->exists = ebofs_obj_exists; 
	obj_funcs->create = ebofs_obj_create; 
	obj_funcs->remove = ebofs_obj_remove; 
	obj_funcs->read = ebofs_obj_read; 
	obj_funcs->write = ebofs_obj_write; 
	obj_funcs->stat = ebofs_obj_stat; 
	obj_funcs->listattrs = NULL;
	obj_funcs->getattrs  = NULL;
	obj_funcs->setattrs  = NULL;
	obj_funcs->rmattrs   = NULL;
	obj_funcs->getattr = ebofs_obj_getattr;
	obj_funcs->setattr = ebofs_obj_setattr;
	obj_funcs->rmattr  = ebofs_obj_rmattr;
	obj_funcs->fsync = ebofs_obj_fsync;
	obj_funcs->trunc = ebofs_obj_trunc;
	
	return rc; 
}

/* fini_objects()
 *
 * teardown structures;
 * does not persist any objects
 *
 * returns non-zero upon success;
 */
int ebofs_obj_fini()
{
	int rc = LWFS_OK;
	ebofs_umount(ebofs);
	return rc; 
}


/*
 * The Ebofs oid is an unsigned int, the lwfs_oid
 * is a char *. 
 */
unsigned int to_ebofs_oid(const lwfs_oid oid)
{
    unsigned int ebofs_oid = 0; 

    memcpy(&ebofs_oid, oid, sizeof(unsigned int)); 
    return ebofs_oid; 
}


/* --------- THE OBJECT INTERFACE ------------ */

/** 
  * @brief Check for existence of the object. 
  *
  * @param obj @input_type Pointer to the object. 
  *
  * @returns TRUE if the object exists, FALSE otherwise.
  */
lwfs_bool ebofs_obj_exists(const lwfs_obj *obj)
{
	log_debug(LOG_UNDEFINED, "entered ebofs_obj_exists");
	assert(ebofs);
	assert(obj);

	return obj_exists(ebofs, to_ebofs_oid(obj->oid)); 
}

/* create_obj()
 *
 * assumes that object does not exist;
 * returns zero on failure;
 */

int ebofs_obj_create(const lwfs_obj *obj)
{
	int rc = LWFS_OK;
	assert(ebofs);
	assert(obj);

	log_debug(LOG_UNDEFINED, "entered ebofs_obj_exists");

	/* check to see if the object already exists */
	if (obj_exists(ebofs, to_ebofs_oid(obj->oid))) {
		rc = LWFS_ERR_EXIST; 
	}
	else if (obj_create(ebofs, to_ebofs_oid(obj->oid)) < 0) {
		rc = LWFS_ERR_STORAGE; 
	}

	return rc; 
}



int ebofs_obj_remove(const lwfs_obj *obj)
{
	int rc = LWFS_OK;
	assert(ebofs);
	assert(obj);

	if (obj_remove(ebofs, to_ebofs_oid(obj->oid)) < 0) {
		rc = LWFS_ERR_STORAGE;
	}
	return rc; 
}


/* read_obj()
 *
 * assumes that object already exists;
 *
 * return the number of bytes
 */
lwfs_ssize ebofs_obj_read(
		const lwfs_obj *obj, 
		const lwfs_ssize offset, 
		void *dest, 
		const lwfs_ssize len)
{
	assert(ebofs);
	assert(obj);

	return obj_read(ebofs, to_ebofs_oid(obj->oid), offset, len, dest); 
}
 /* read_obj() */


/* write_obj()
 *
 * assumes that object already exists;
 *
 * returns the number of bytes written.
 */
lwfs_ssize ebofs_obj_write(
		const lwfs_obj *obj, 
		const lwfs_ssize offset, 
		void *src, 
		const lwfs_ssize len)
{
	assert(ebofs);
	assert(obj);

	return obj_write(ebofs, to_ebofs_oid(obj->oid), offset, len, src); 
}
 /* write_obj() */


/* fsync()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 *
 */
int ebofs_obj_fsync(
	const lwfs_obj *obj)
{
	return LWFS_ERR_NOTSUPP;
}

/** 
 * @brief Get object attributes.
 *
 * @param obj @input_type Object pointer.
 * @param 
 *
 */
int ebofs_obj_getattr(
	const lwfs_obj *obj, 
	const lwfs_name name,
	lwfs_attr *res)
{
	int rc = LWFS_OK;
	
	void *val = calloc(1, sizeof(LWFS_NAME_LEN));
	int maxsize = LWFS_NAME_LEN;
	int actual_size = 0;
	
	assert(ebofs);
	assert(obj);

	/* make sure the object exists */
	if (!obj_exists(ebofs, to_ebofs_oid(obj->oid))) {
		return LWFS_ERR_NOENT; 
	}

	if ((actual_size = obj_getattr(ebofs, to_ebofs_oid(obj->oid), 
			name, val, maxsize)) < 0) {
		log_error(ss_debug_level, "could not get object attributes");
		rc = LWFS_ERR_NOENT;
	}

	res->name = calloc(1, strlen(name)+1);
	strcpy(res->name, name);
	res->value.lwfs_attr_data_len = actual_size;
	res->value.lwfs_attr_data_val = calloc(1, actual_size);
	memcpy(res->value.lwfs_attr_data_val, val, actual_size);

	log_debug(ss_debug_level, "res->name==%s", res->name);
	log_debug(ss_debug_level, "res->value.len==%d", res->value.lwfs_attr_data_len);
	log_debug(ss_debug_level, "res->value.val==%s", res->value.lwfs_attr_data_val);

	return rc;
}

int ebofs_obj_setattr(
		const lwfs_obj *obj,
		const lwfs_name name,
		const void *value,
		const int size)
{
	int rc = LWFS_OK;
	assert(ebofs);
	assert(obj);

	/* make sure the object exists */
	if (!obj_exists(ebofs, to_ebofs_oid(obj->oid))) {
		return LWFS_ERR_NOENT; 
	}

	if (obj_setattr(ebofs, to_ebofs_oid(obj->oid), name, value, size) < 0) {
		log_error(ss_debug_level, "error in setattr");
		rc = LWFS_ERR_STORAGE; 
	}

	return rc; 
}


int ebofs_obj_rmattr(
		const lwfs_obj *obj,
		const lwfs_name name)
{
	int rc = LWFS_OK;
	assert(ebofs);
	assert(obj);

	/* make sure the object exists */
	if (!obj_exists(ebofs, to_ebofs_oid(obj->oid))) {
		return LWFS_ERR_NOENT; 
	}

	if (obj_rmattr(ebofs, to_ebofs_oid(obj->oid), name) < 0) {
		log_error(ss_debug_level, "error in rmattr");
		rc = LWFS_ERR_STORAGE; 
	}

	return rc; 
}

int ebofs_obj_stat(
		const lwfs_obj *obj,
		lwfs_stat_data *lwfs_data)
{
	int rc = LWFS_OK;
	struct stat unix_data; 

	if (obj_stat(ebofs, to_ebofs_oid(obj->oid), &unix_data) < 0) {
		rc = LWFS_ERR_STORAGE; 
	}

	return rc; 
}

 /*  get_attr() */


/* get_attr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 *
 * currently only valid for attr.size;
 */
int ebofs_obj_trunc(
	const lwfs_obj *obj, 
	const lwfs_ssize size)
{

	/*
	if (obj_trunc(ebofs, obj->oid, size) < 0) {
		rc = LWFS_ERR_STORAGE; 
	}

	return LWFS_OK; 
	*/
	return LWFS_ERR_NOTSUPP;
} /*  get_attr() */

