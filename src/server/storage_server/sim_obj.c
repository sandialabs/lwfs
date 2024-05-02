/**  
 *   @file sim_obj.c
 * 
 *   This is the object api for a simulated storage server. 
 *
 *   This object api is used by the registered storage server
 *   functions.  See storage_srvr.c.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 */
#include "config.h"

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "sim_obj.h"

#include "common/types/xdr_types.h"
#include "common/types/types.h"
#include "support/list/tclist.h"

#include "support/hashtable/hashtable.h"
#include "support/hashtable/hash_funcs.h"


static struct obj_funcs sim_obj_funcs = {
    .exists = sim_obj_exists,
    .create = sim_obj_create,
    .remove = sim_obj_remove,
    .read = sim_obj_read,
    .write = sim_obj_write,
    .listattrs = sim_obj_listattrs,
    .getattrs  = sim_obj_getattrs,
    .setattrs  = sim_obj_setattrs,
    .rmattrs   = sim_obj_rmattrs,
    .getattr   = sim_obj_getattr,
    .setattr   = sim_obj_setattr,
    .rmattr    = sim_obj_rmattr,
    .stat = sim_obj_stat,
    .fsync = sim_obj_fsync,
    .trunc = sim_obj_trunc 
};


/********** private functions **********/

static unsigned int oid_hash_func(void *data)
{
	return(RSHash((char *)data, sizeof(lwfs_oid)));
}

static int compare_oids(void *a, void *b)
{
	char ostr[33];
	log_debug(LOG_UNDEFINED, "***** enter compare_oids");
	log_debug(LOG_UNDEFINED, "oid1==0x%s", lwfs_oid_to_string(a, ostr));
	log_debug(LOG_UNDEFINED, "oid2==0x%s", lwfs_oid_to_string(b, ostr));
	// the two are equal if memcmp returns 0
	return (0 == memcmp(a, b, sizeof(lwfs_oid)));
}


/********** end private functions **********/

/********** public functions (public to ss_srvr) **********/


int sim_obj_init(
        const char *root_dir,
        struct obj_funcs *obj_funcs)
{
	int rc = LWFS_OK;

	/* copy root */
	strncpy(root, root_dir, MAX_ROOT_LEN);

	/* insure that our special directory has been created;   */
	if (mkdir(root_dir, 500)){

		if (errno != EEXIST){
			/* some problem */
			log_fatal(ss_debug_level, "failed to create storage server"
					"root directory");
			return LWFS_ERR_STORAGE;

		}
		/* else, assume the directory exists rather than
		 * something like a file with same name exists */
	}

	if (!create_mt_hashtable(MAX_HASHTABLE, oid_hash_func, compare_oids, &oid_hash)) {
		log_error(LOG_ERROR, "failed to create a new hashtable");
		rc = LWFS_ERR;
	}
	
	if (repopulate_oid_hashtable() != LWFS_OK) {
		log_error(LOG_ERROR, "failed to repopulate the oid hashtable");
		rc = LWFS_ERR;
	}
	
	open_files_list = tclist_new(TC_LOCK_STRICT);
	if (open_files_list == NULL)
	{
		log_error(LOG_ERROR, "failed to get a new open_files_list");
		rc = LWFS_ERR;
	}

	/* assign function pointers to the ss_funcs structure */
	memcpy(obj_funcs, &sysio_obj_funcs, sizeof(struct obj_funcs));

	return rc; 
}

/* fini_objects()
 *
 * teardown structures;
 * does not persist any objects
 *
 * returns non-zero upon success;
 */
int sysio_obj_fini()
{
	int rc=LWFS_OK;

	close_all_open_files();

	/*
	while(!objs_is_empty()){
		if (!remove_obj_by_oid(get_top_obj_id())){
			rc = LWFS_ERR_STORAGE;
			break;
		}
	}

	if (rc == LWFS_OK && objs_is_empty()){
		if (rmdir(root) != 0){
			rc = LWFS_ERR_STORAGE;
		}
	}
	if (rc != LWFS_OK){
		log_error(LOG_UNDEFINED, "could not remove directory");
	}
	*/

	tclist_destroy(open_files_list, NULL);

	mt_hashtable_destroy(&oid_hash, free);

	return(rc);
} /* fini_objects() */


/** 
 * @brief Returns the file descriptor of an object. 
 */
int sysio_obj_getfd(const lwfs_obj *obj)
{
    oid_el *ptr = open_oid(&obj->oid);

    if (ptr == NULL) {
        return -1; 
    }
    else {
        return ptr->fd;
    }
}





/* --------- THE OBJECT INTERFACE ------------ */

/** 
  * @brief Check for existence of the object. 
  *
  * @param obj @input_type Pointer to the object. 
  *
  * @returns TRUE if the object exists, FALSE otherwise.
  */
lwfs_bool sysio_obj_exists(const lwfs_obj *obj)
{
    log_debug(LOG_UNDEFINED, "entered sysio_obj_exists");

    return (find_oid(&obj->oid) != NULL);
}

/* create_obj()
 *
 * assumes that object does not exist;
 * returns zero on failure;
 */

int sysio_obj_create(const lwfs_obj *obj)
{
	int rc = LWFS_OK;
	int fd; 
	oid_el * new_oid = NULL;

	char file_name[64];
	char ostr[33];

	log_debug(ss_debug_level, "entered sysio_obj_create");

	/* FIXME: unsigned long may not work everywhere */
	rc = snprintf(file_name, 64, PATH_STR, root, lwfs_oid_to_string(obj->oid, ostr));
	if (rc == 0 || rc == EOF){
		log_error(LOG_UNDEFINED, "sprintf() failure;");
		return LWFS_ERR_STORAGE;
	}

	/* check for the existence of the file */
	if (access(file_name, F_OK) == 0) {
		log_error(ss_debug_level, "file already exists (%s)", file_name);
		return LWFS_ERR_EXIST;
	}

	/* now really create it (with mode = "rw") */
	fd = creat(file_name, S_IRUSR | S_IWUSR);
	if (fd == -1){
		log_error(ss_debug_level, "failed to open file %s: %s",
			file_name, strerror(errno));
		return LWFS_ERR_STORAGE;
	}
	else {
		rc = close(fd);
		if (rc == -1) {
			log_error(ss_debug_level, "failed to close file %s: %s",
					file_name, strerror(errno));
			return LWFS_ERR_STORAGE;
		}
	}

	/* add an object for tracking purposes; */
	new_oid = add_oid(&(obj->oid));
	if (!new_oid){
		(void)sprintf(log_line, "failed to add oid (0x%s)", lwfs_oid_to_string(obj->oid, ostr));
		log_error(LOG_UNDEFINED, log_line);
		return LWFS_ERR_STORAGE;
	}
	
	/* brittle? */
	strcpy(new_oid->mode, "\0");

	/* Add this object to the list of open files.
	 * This will close files if we're over the limit. 
	 */
	add_to_open_files(new_oid); 

	log_debug(ss_debug_level, "created obj oid=0x%s", lwfs_oid_to_string(new_oid->id, ostr)); 
	
	return rc; 
} 




int sysio_obj_remove(const lwfs_obj *obj)
{
	int rc = LWFS_OK;
	lwfs_name_array attr_names;
	
	log_debug(ss_debug_level, "entered sysio_obj_remove");

	rc = remove_obj_by_oid(&obj->oid);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level,"could not remove object");
		return rc;
	}

	rc = sysio_obj_listattrs(obj, &attr_names);
	if (rc == LWFS_ERR_NOENT) {
		log_debug(ss_debug_level,"no object attrs found");
		/* this is not an error case.  don't confuse the client with non-error cases. */
		return LWFS_OK;
	} else if (rc != LWFS_OK) {
		log_error(ss_debug_level,"could not list object attrs");
		return rc;
	}
	rc = sysio_obj_rmattrs(obj, &attr_names); 
	if (rc != LWFS_OK) {
		log_error(ss_debug_level,"could not remove object attrs");
		return rc;
	}
	
	return rc;
}


/* read_obj()
 *
 * assumes that object already exists;
 *
 * return one upon success;
 */
lwfs_ssize sysio_obj_read(
		const lwfs_obj *obj, 
		const lwfs_ssize src_offset, 
		void *dest, 
		const lwfs_ssize len)
{
	size_t offset; 
	oid_el *current = NULL;
	lwfs_ssize count = 0;

	char file_name[64];
	char ostr[33];

	log_debug(ss_debug_level, "entered sysio_obj_read");

	current = open_oid(&obj->oid);
	if (current == NULL) {
		log_error(ss_debug_level,
                "could not read because object (0x%s) not found", 
                lwfs_oid_to_string(obj->oid, ostr));
		return count; 
	}

	/* see if the mode is set to allows reads */
	if (current->fd){
		if (!can_read_file(current->mode))
		{
			remove_from_open_files(current);
		}
	}

	/* do we need to open the file? */
	/*
	close(current->fd); 
	current->fd = 0; 
	*/
	if (current->fd == 0){
		int ret = 0;
		ret = snprintf(file_name, 64, PATH_STR, root, lwfs_oid_to_string(obj->oid, ostr));
		if (ret == 0 || ret == EOF){
			log_error(ss_debug_level, "sprintf() failed;");
			return count;
		}


		/* need error checking? */
		current->fd = open(file_name, openflags);
		if (current->fd == -1){
			log_error(ss_debug_level, "could not open file for read");
			return count;
		}

		/* brittle? */
		(void)strcpy(current->mode, "r+");

		add_to_open_files(current);
	}

	/* seek to the right location */
	offset = lseek(current->fd, src_offset, SEEK_SET); 
	if(offset == -1) {
		log_error(ss_debug_level, "lseek() failed: %s",
				strerror(errno));
		return LWFS_ERR;
	}

	/* now we can do the read */
	if (len > 0) {

		ssize_t bytes_read = read(current->fd, dest, len); 
		log_debug(ss_debug_level, "bytes_read == %ld", bytes_read);
		if (bytes_read == -1) {
			log_error(ss_debug_level, "unable to read: %s",
					strerror(errno));
			count = 0; 
			return count; 
		}

		count = bytes_read; 
	}
	else {
		/* nothing to do here */
		count = 0;
	}

	log_debug(ss_debug_level, "finished sysio_obj_read");

	return count;

} /* read_obj() */


/* write_obj()
 *
 * assumes that object already exists;
 *
 * returns one upon success;
 */
lwfs_ssize sysio_obj_write(
		const lwfs_obj *dest_obj, 
		const lwfs_ssize dest_offset, 
		void *src, 
		const lwfs_ssize len)
{
    lwfs_ssize count=0; 
    int rc = 0;

    oid_el * result = NULL;

    char file_name[64];
    char ostr[33];

    log_debug(ss_debug_level, "entered sysio_obj_write");
	
    result = open_oid(&dest_obj->oid);

    if (result){

        if (result->fd && !can_write_file(result->mode)){
            (void)sprintf(log_line, "removing file (0x%s) from open list since can't write", lwfs_oid_to_string(result->id, ostr));
            log_info(LOG_UNDEFINED, log_line);
            remove_from_open_files(result);
        }

        if (result->fd == 0){
	    rc = snprintf(file_name, 64, PATH_STR, root, lwfs_oid_to_string(result->id, ostr));
            if (rc == 0 || rc == EOF){
                log_error(LOG_UNDEFINED, "sprintf() failed");
                return 0;
            }

            result->fd = open(file_name, openflags);

            if (result->fd < 0){
                (void) sprintf(log_line, "open() failed for file %s", file_name);
                log_error(LOG_UNDEFINED, log_line);
                return 0;
            }

            /* need error checking? */
            (void)strcpy(result->mode, "r+");
            (void)sprintf(log_line, "adding file (0x%s) to open list so can write", lwfs_oid_to_string(result->id, ostr));
            log_info(LOG_UNDEFINED, log_line);

            add_to_open_files(result);
        }

        log_debug(ss_debug_level, "seeking to offset %ld", dest_offset); 

        if (lseek(result->fd, dest_offset, SEEK_SET) == -1) {
            (void) sprintf(log_line, "lseek() failed for file %s, offset=%ld", 
                           file_name, (unsigned long)dest_offset);
            log_error(LOG_UNDEFINED, log_line);
        }
        else{
            /* do the write */
            if (len > 0) {
                log_debug(ss_debug_level, "bytes written == %ld", len); 

                count = write(result->fd, src, len);
            }

            rc = 1;
        }
    }
    else{
        (void)sprintf(log_line, "could not write because object id (%s) not found", lwfs_oid_to_string(result->id, ostr));
        log_info(LOG_UNDEFINED, log_line);
    }

    log_debug(ss_debug_level, "finished write", len); 

	/* this is where we free the buffer */
	free(src); 

    return count;

} /* write_obj() */


/* fsync()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 *
 */
int sysio_obj_fsync(
	const lwfs_obj *obj)
{
    int rc = LWFS_OK;
    oid_el * result = NULL;

    log_debug(ss_debug_level, "entered sysio_obj_fsync");
	
    result = open_oid(&obj->oid);

    if (result){
        /* flush all data to disk */
#ifdef HAVE_FDATASYNC
        if (fdatasync(result->fd) == -1) {
#else
	  if (fsync(result->fd) == -1) {
#endif
            log_error(ss_debug_level, "could not fsync file (fd=%d): %s",
                    result->fd, strerror(errno));
            rc = LWFS_ERR_STORAGE; 
        }
    }
    else {
        rc = LWFS_ERR_STORAGE;
    }

    return rc;

} /*  get_attr() */

/* get_attr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 *
 * currently only valid for attr.size;
 */
int sysio_obj_stat(
	const lwfs_obj *obj, 
	lwfs_stat_data *res)
{
	int rc = LWFS_OK;
	oid_el * result = NULL;

	log_debug(ss_debug_level, "entered sysio_obj_stat");
	
	result = open_oid(&obj->oid);

	if (result){
		struct stat stats; 

		if (fstat(result->fd, &stats) == -1) {
			log_error(ss_debug_level, "count not fstat file: %s",
					strerror(errno));
			return LWFS_ERR; 
		}

		memset(res, 0, sizeof(lwfs_stat_data));

		res->size = stats.st_size; 
		res->atime.seconds = stats.st_atime; 
		res->mtime.seconds = stats.st_mtime; 
		res->ctime.seconds = stats.st_ctime; 

		rc = LWFS_OK;
	}

	return rc;

} /*  get_attr() */


/* get_attr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 *
 * currently only valid for attr.size;
 */
int sysio_obj_trunc(
	const lwfs_obj *obj, 
	const lwfs_ssize size)
{
	int rc = 0;
	oid_el * result = NULL;

	log_debug(ss_debug_level, "entered sysio_obj_trunc");
	
	result = open_oid(&obj->oid);

	if (result){
		if (ftruncate(result->fd, size) == -1) {
			log_error(ss_debug_level, "could not truncate file: %s",
				strerror(errno));
			return 0; 
		}
		rc = 1;
	}

	return rc;

} /*  get_attr() */

/* listattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_listattrs(
	const lwfs_obj *obj, 
	lwfs_name_array *names)
{
	int rc = LWFS_OK;
	int i=0;
	unsigned int count=0;

	log_debug(ss_debug_level, "entered sysio_obj_listattrs");
	
	ss_db_get_attr_count(&obj->oid, &count);
	names->lwfs_name_array_len = count;
	names->lwfs_name_array_val = calloc(count, sizeof(char *));
	for (i=0;i<count;i++) {
		names->lwfs_name_array_val[i] = calloc(1, sizeof(char)*LWFS_NAME_LEN);
	}
	
	rc = ss_db_get_all_attr_names_by_oid(&obj->oid, names->lwfs_name_array_val, &count);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not sysio_obj_listattrs: %s",
			lwfs_err_str(rc));
		goto cleanup; 
	}

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_listattrs");

	return rc;
} /*  ss_listattr() */

/* ss_getattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_getattrs(
	const lwfs_obj *obj, 
	const lwfs_name_array *names,
	lwfs_attr_array *res)
{
	int rc = LWFS_OK;
	int i=0;
	
	ss_db_attr_entry attr;

	log_debug(ss_debug_level, "entered sysio_obj_getattrs");
	
	res->lwfs_attr_array_len = names->lwfs_name_array_len;
	res->lwfs_attr_array_val = calloc(names->lwfs_name_array_len, sizeof(lwfs_attr));

	for (i=0; i<names->lwfs_name_array_len; i++) {
		rc = ss_db_get_attr_by_name(&obj->oid, 
					    names->lwfs_name_array_val[i],
					    &attr);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not sysio_obj_getattrs: %s",
				lwfs_err_str(rc));
			goto cleanup; 
		}
		res->lwfs_attr_array_val[i].name = calloc(1, strlen(attr.name)+1);
		strcpy(res->lwfs_attr_array_val[i].name, attr.name);
		res->lwfs_attr_array_val[i].value.lwfs_attr_data_len = attr.size;
		res->lwfs_attr_array_val[i].value.lwfs_attr_data_val = calloc(1, attr.size);
		memcpy(res->lwfs_attr_array_val[i].value.lwfs_attr_data_val, attr.value, attr.size);
	}

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_getattrs");

	return rc;
} /*  ss_getattr() */

/* ss_setattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_setattrs(
	const lwfs_obj *obj, 
	const lwfs_attr_array *attrs)
{
	int rc = LWFS_OK;
	int i=0;
	ss_db_attr_entry attr;

	log_debug(ss_debug_level, "entered sysio_obj_setattrs");
	
	for (i=0;i<attrs->lwfs_attr_array_len; i++) {
		memcpy(&attr.oid, &obj->oid, sizeof(lwfs_oid));
		strcpy(attr.name, attrs->lwfs_attr_array_val[i].name);
		attr.size = attrs->lwfs_attr_array_val[i].value.lwfs_attr_data_len;
		memcpy(attr.value, attrs->lwfs_attr_array_val[i].value.lwfs_attr_data_val, attr.size);
		rc = ss_db_attr_put(&attr, 0);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not sysio_obj_setattrs: %s",
				lwfs_err_str(rc));
			goto cleanup; 
		}
	}

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_setattrs");

	return rc;
} /*  ss_setattr() */

/* ss_rmattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_rmattrs(
	const lwfs_obj *obj, 
	const lwfs_name_array *names)
{
	int rc = LWFS_OK;
	int i=0;
	ss_db_attr_entry attr;

	log_debug(ss_debug_level, "entered sysio_obj_rmattrs");
	
	for (i=0;i<names->lwfs_name_array_len; i++) {
		rc = ss_db_attr_del(&obj->oid, 
				    names->lwfs_name_array_val[i],
				    &attr);
		if (rc != LWFS_OK) {
			log_error(ss_debug_level, "could not sysio_obj_rmattrs: %s",
				lwfs_err_str(rc));
			goto cleanup; 
		}
	}

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_rmattrs");

	return rc;
} /*  ss_rmattr() */

/* ss_getattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_getattr(
	const lwfs_obj *obj, 
	const lwfs_name name,
	lwfs_attr *res)
{
	int rc = LWFS_OK;
	
	ss_db_attr_entry attr;

	log_debug(ss_debug_level, "entered sysio_obj_getattr");
	
	rc = ss_db_get_attr_by_name(&obj->oid, 
				    name,
				    &attr);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not sysio_obj_getattr: %s",
			lwfs_err_str(rc));
		goto cleanup; 
	}
	res->name = calloc(1, strlen(attr.name)+1);
	strcpy(res->name, attr.name);
	res->value.lwfs_attr_data_len = attr.size;
	res->value.lwfs_attr_data_val = calloc(1, attr.size);
	memcpy(res->value.lwfs_attr_data_val, attr.value, attr.size);
	
	log_debug(ss_debug_level, "res->name==%s", res->name);
	log_debug(ss_debug_level, "res->value.len==%d", res->value.lwfs_attr_data_len);
	log_debug(ss_debug_level, "res->value.val==%s", res->value.lwfs_attr_data_val);

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_getattr");

	return rc;
} /*  ss_getattr() */

/* ss_setattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_setattr(
	const lwfs_obj *obj, 
	const lwfs_name name,
	const void *value,
	const int size)
{
	int rc = LWFS_OK;
	ss_db_attr_entry db_attr;

	log_debug(ss_debug_level, "entered sysio_obj_setattr");
	
	memcpy(&db_attr.oid, &obj->oid, sizeof(lwfs_oid));
	strcpy(db_attr.name, name);
	db_attr.size = size;
	memcpy(db_attr.value, value, db_attr.size);
	rc = ss_db_attr_put(&db_attr, 0);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not sysio_obj_setattr: %s",
			lwfs_err_str(rc));
		goto cleanup; 
	}

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_setattr");

	return rc;
} /*  ss_setattr() */

/* ss_rmattr()
 *
 * assumes object already exists;
 *
 * returns one upon success;
 */
int sysio_obj_rmattr(
	const lwfs_obj *obj, 
	const lwfs_name name)
{
	int rc = LWFS_OK;
	ss_db_attr_entry attr;

	log_debug(ss_debug_level, "entered sysio_obj_rmattr");
	
	rc = ss_db_attr_del(&obj->oid, 
			    name,
			    &attr);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not sysio_obj_rmattr: %s",
			lwfs_err_str(rc));
		goto cleanup; 
	}

cleanup:
	log_debug(ss_debug_level, "finished sysio_obj_rmattr");

	return rc;
} /*  ss_rmattr() */
