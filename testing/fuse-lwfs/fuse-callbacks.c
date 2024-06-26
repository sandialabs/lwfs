/**
 * @file fuse_callbacks.c 
 * 
 * @brief Implementations of the FUSE callbacks used for the LWFS. 
 *
 * @author Ron Oldfield (raoldfi\@sandia.gov)
 * $Revision: 1073 $
 * $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
*/

#include <strings.h>
#include <config.h>

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>

#include "types/types.h"
#include "types/fprint_types.h"
#include "logger/logger.h"
#include "naming/naming_clnt.h"
#include "storage/ss_clnt.h"
#include "authr/authr_clnt.h"
#include "fuse-debug.h"

/* ---- Local variables ---- */
static lwfs_service naming_svc; 
static lwfs_service storage_svc; 
static lwfs_service authr_svc; 

/* my credential */
static lwfs_cred cred; 

/* caps used for access */
static lwfs_cap cap; 
static lwfs_cap modacl_cap; 

/* one container used for all objects */
static lwfs_cid cid; 

/* single txn used for all methods */
static lwfs_txn txn; 


int fuse_callbacks_init(
	const lwfs_service *a_svc, 
	const lwfs_service *n_svc, 
	const lwfs_service *s_svc)
{
	int rc = LWFS_OK; 
	lwfs_opcode opcodes; 
	lwfs_cap create_cid_cap; 
	lwfs_uid_array uid_array; 

	uid_t unix_uid; 

	memcpy(&naming_svc, n_svc, sizeof(lwfs_service));
	memcpy(&storage_svc, s_svc, sizeof(lwfs_service));
	memcpy(&authr_svc, a_svc, sizeof(lwfs_service));

	/* initialize the user credential */
	memset(&cred, 0, sizeof(lwfs_cred));
	unix_uid = getuid(); 
	memcpy(&cred.data.uid, &unix_uid, sizeof(unix_uid));

	/* create a single container for the capability */
	rc = lwfs_bcreate_container(&txn, &create_cid_cap, &modacl_cap); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "unable to create container: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* set the cid */
	cid = modacl_cap.data.cid; 

	/* Create an acls that allows me to read and write to the container */
	uid_array.lwfs_uid_array_len = 1; 
	uid_array.lwfs_uid_array_val = &cred.data.uid; 

	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	rc = lwfs_bcreate_acl(&txn, cid, opcodes, &uid_array, &modacl_cap); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "unable to create acls: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* get the cap that allows me to read and write to the container */
	rc = lwfs_bget_cap(cid, opcodes, &cred, &cap);
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	return rc; 
}


/* ---- Private methods ---- */

static int get_suffix(char *name, const char *path, int len)
{
	/* ptr points to the last '/' */
	char *ptr = rindex(path, '/'); 

	/* copy name */
	strncpy(name, &ptr[1], len); 

	return LWFS_OK;
}

static int get_prefix(char *prefix, const char *path, int maxlen)
{
	int prefix_len; 
	int len; 

	/* ptr points to the last '/' */
	char *ptr = rindex(path, '/'); 

	/* number of bytes to copy */
	prefix_len = (ptr-path)+1; 

	len = (prefix_len > maxlen)? maxlen : prefix_len; 

	/* copy result to the prefix */
	strncpy(prefix, path, len); 

	/* add the null character */
	prefix[len-1] = '\0'; 

	if (strcmp(prefix, "") == 0) {
		prefix[0] = '/';
		prefix[1] = '\0';
	}

	return LWFS_OK;
}

/**
 * @brief Find the entry associated with the directory path.
 *
 * @param path @input the full path. 
 * @param parent @input the object for the resulting 
 *                      parent directory. 
 * @param name  @input the name, extracted from the 
 *                     end of the path. 
 */
static int traverse_path(
	const char *path, 
	lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	char name[LWFS_NAME_LEN]; 
	char *parent_path = NULL;
	lwfs_ns_entry parent_ent; 

	log_debug(fuse_call_debug_level, "started traverse_path(%s, ...)", path);

	/* error case */
	if (path == NULL) {
		log_error(fuse_debug_level, "invalid path"); 
		errno = ENOENT; 
		return -errno; 
	}


	/* TODO: look for the entry in a cache */


	/* Are we looking up the root? */
	if (strcmp(path, "/") == 0) {
		memcpy(result, LWFS_NAMING_ROOT, sizeof(lwfs_ns_entry)); 
		goto cleanup; 
	}


	/* Ok, so we have to do it the hard way */

	/* allocate a buffer for the parent path */
	parent_path = (char *)malloc(strlen(path));
	if (parent_path == NULL) {
		log_error(fuse_debug_level, "could not allocate parent_path");
		rc = LWFS_ERR_NOSPACE;
	}

	get_suffix(name, path, LWFS_NAME_LEN); 
	get_prefix(parent_path, path, strlen(path)); 

	/* recursive call to get the parent object */
	rc = traverse_path(parent_path, &parent_ent); 
	if (rc != LWFS_OK) {
		log_warn(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/* lookup the entry */
	log_debug(fuse_call_debug_level, "lookup %s", name); 
	rc = lwfs_blookup(NULL, &parent_ent, name, 
			LWFS_LOCK_NULL, &cap, result);
	if (rc != LWFS_OK) {
		errno = ENOENT; 
		rc = -ENOENT; 
		log_warn(fuse_debug_level, "unable to lookup entry: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

cleanup:
	log_debug(fuse_call_debug_level, "freeing parent_path");
	free(parent_path); 
	log_debug(fuse_call_debug_level, "finished traverse_path(%s, ...)", path);
	return rc; 
}




/* ----- Implementation of the callbacks ---------- */

/**
 * @brief Get the attributes.
 *
 * @param path @input path to the entry.
 * @param stbuf @output the structure to set. 
 *
 * The getattr function returns the attributes associated 
 * with a directory entry.  We need to fill in the fields 
 * for the "struct stat" buffer, defined as follows:
 * 
 * \code
 * struct stat {
 *     dev_t         st_dev;      \/\* device \*\/
 *     ino_t         st_ino;      \/\* inode \*\/
 *     mode_t        st_mode;     \/\* protection \*\/
 *     nlink_t       st_nlink;    \/\* number of hard links \*\/
 *     uid_t         st_uid;      \/\* user ID of owner \*\/
 *     gid_t         st_gid;      \/\* group ID of owner \*\/
 *     dev_t         st_rdev;     \/\* device type  (if inode device) \*\/
 *     off_t         st_size;     \/\* total size, in bytes \*\/
 *     blksize_t     st_blksize;  \/\* blocksize for filesystem I/O \*\/
 *     blkcnt_t      st_blocks;   \/\* number of blocks allocated \*\/
 *     time_t        st_atime;    \/\* time of last access \*\/
 *     time_t        st_mtime;    \/\* time of last modification \*\/
 *     time_t        st_ctime;    \/\* time of last status change \*\/
 * };
 * \endcode
 * 
 */
int lwfs_fuse_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	lwfs_ns_entry ent; 
	lwfs_obj_attr attr; 
	lwfs_obj *obj; 

	log_debug(fuse_call_debug_level, "started getattr(%s, ...)", path);

	/* get the object */
	res = traverse_path(path, &ent); 
	if (res != LWFS_OK) {
		log_warn(fuse_debug_level, "cound not traverse path: %s",
				lwfs_err_str(res));
		goto cleanup; 
	}

	log_debug(fuse_debug_level, "traverse_path returned entry with type=%d", 
			ent.entry_obj.type);

	/* get the attributes for the object */
	if (ent.file_obj != NULL) {
		obj = ent.file_obj; 
	}
	else {
		obj = &ent.entry_obj; 
	}

	res = lwfs_bget_attr(NULL, obj, &cap, &attr);
	if (res != LWFS_OK) {
		log_warn(fuse_debug_level, "unable to get attributes: %s",
				lwfs_err_str(res));
		errno = EIO;
		res = -errno;
		goto cleanup;
	}

	if (logging_debug(fuse_debug_level)) {
		fprint_lwfs_obj_attr(stdout, "attr", "DEBUG", &attr);
	}

	memset(stbuf, 0, sizeof(struct stat));

	switch (obj->type) {

		case LWFS_FILE_ENTRY:
			stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
			break; 

		case LWFS_FILE_OBJ:
			stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
			break; 

		case LWFS_DIR_ENTRY:
			stbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
			break; 

		case LWFS_LINK_ENTRY:
			stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
			break; 

		default:
			stbuf->st_mode = 0; 
			break; 
	}

	stbuf->st_dev= 770;			/* device */
	stbuf->st_ino= 1;                       /* inode */
	stbuf->st_nlink= ent.link_cnt;     	/* number of hard links */
	memcpy(&stbuf->st_uid, cred.data.uid, sizeof(uid_t));;		/* user ID of owner */
	memcpy(&stbuf->st_gid, cred.data.uid, sizeof(uid_t));;		/* group ID of owner */
	stbuf->st_rdev= 0;			/* device type (if inode device) */
	stbuf->st_size= attr.size;		/* total size, in bytes */
	stbuf->st_blksize= 1;		/* blocksize for filesystem I/O */
	stbuf->st_blocks= attr.size;	/* number of blocks allocated */
	stbuf->st_atime = attr.atime.seconds;		/* time of last access */
	stbuf->st_mtime= attr.mtime.seconds;		/* time of last modification */
	stbuf->st_ctime= attr.ctime.seconds;		/* time of last change */

	if (logging_debug(fuse_debug_level)) {
		fprintf(stderr, "        mode            0x%08x\n", (int)stbuf->st_mode);
		fprintf(stderr, "        device          %d\n", (int)stbuf->st_dev);
		fprintf(stderr, "        inode           %d\n", (int)stbuf->st_ino);
		fprintf(stderr, "        hard links      %d\n", (int)stbuf->st_nlink);
		fprintf(stderr, "        user ID         %d\n", (int)stbuf->st_uid);
		fprintf(stderr, "        groupd ID       %d\n", (int)stbuf->st_gid);
		fprintf(stderr, "        device type     %d\n", (int)stbuf->st_rdev);
		fprintf(stderr, "        size            %d\n", (int)stbuf->st_size);
		fprintf(stderr, "        block size      %d\n", (int)stbuf->st_blksize);
		fprintf(stderr, "        num blocks      %d\n", (int)stbuf->st_blocks);
		fprintf(stderr, "        last access     %d\n", (int)stbuf->st_atime);
		fprintf(stderr, "        last mod        %d\n", (int)stbuf->st_mtime);
		fprintf(stderr, "        last change     %d\n", (int)stbuf->st_ctime);
	}

cleanup:
	log_debug(fuse_call_debug_level, "finished getattr(%s, ...)", path);

	return res;
}

int lwfs_fuse_readlink(const char *path, char *buf, size_t size)
{
	int res = 0;

	log_debug(fuse_call_debug_level, "called readlink(%s, ...)", path);

	/*
	res = readlink(path, buf, size - 1);
	if(res == -1)
		return -errno;

	buf[res] = '\0';
	*/
	return res;
}


int lwfs_fuse_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	int rc = 0;
	int i;
	lwfs_ns_entry parent; 
	lwfs_ns_entry_array listing; 

	log_debug(fuse_call_debug_level, "started getdir(%s, ...)", path);


	/* get the object */
	rc = traverse_path(path, &parent); 
	if (rc != LWFS_OK) {
		log_warn(fuse_debug_level, "error traversing path: %s",
			strerror(errno));
		goto cleanup;
	}

	/* get the contents of the directory */
	rc = lwfs_blist_dir(&parent, &cap, &listing);
	if (rc != LWFS_OK) {
		log_warn(fuse_debug_level, "error getting listing: %s",
			lwfs_err_str(rc));
		errno = EBADF; 
		rc = -errno; 
		goto cleanup;
	}

	/* fake . and .. */
	rc = filler(h, ".", DT_DIR, 0);
	if (rc != 0) {
		log_error(fuse_debug_level, "error calling \"filler\"");
		goto cleanup;
	}

	rc = filler(h, "..", DT_DIR, 0);
	if (rc != 0) {
		log_error(fuse_debug_level, "error calling \"filler\"");
		goto cleanup;
	}

	/* fill in the real entries */
	for (i=0; i<listing.lwfs_ns_entry_array_len; i++) {
		char *name = listing.lwfs_ns_entry_array_val[i].name; 
		lwfs_obj *obj = &listing.lwfs_ns_entry_array_val[i].entry_obj; 
		int type; 
		
		switch (obj->type) {

			case LWFS_DIR_ENTRY:
				type = DT_DIR; 
				break;

			case LWFS_FILE_ENTRY:
				type = DT_REG;
				break; 

			case LWFS_FILE_OBJ:
				type = DT_REG;
				break; 


			case LWFS_LINK_ENTRY:
				type = DT_LNK;
				break;

			default:
				type = DT_UNKNOWN;
				break;
		}
				
		rc = filler(h, name, type, obj->oid);
		if (rc != 0) {
			log_error(fuse_debug_level, "error calling \"filler\"");
			goto cleanup;
		}
	}

	/*
	DIR *dp;
	struct dirent *de;

	dp = opendir(path);
	if(dp == NULL)
		return -errno;

	while((de = readdir(dp)) != NULL) {
		res = filler(h, de->d_name, de->d_type, de->d_ino);
		if(res != 0)
			break;
	}

	closedir(dp);
	*/

cleanup:

	log_debug(fuse_call_debug_level, "finished getdir(%s, ...)", path);

	return rc;
}

int lwfs_fuse_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int rc = 0;
	lwfs_obj obj; 
	lwfs_ns_entry parent_ent; 
	lwfs_ns_entry file_ent; 
	char *parent_path = NULL;
	char name[LWFS_NAME_LEN];


	log_debug(fuse_call_debug_level, "called mknod(%s, ...)", path);

	/* allocate space for the parent path */
	parent_path = (char *)malloc(strlen(path));
	if (parent_path == NULL) {
		errno = ENOMEM; 
		rc = -errno; 
		goto cleanup; 
	}

	/* extract the file name and parent object from the path */
	get_suffix(name, path, LWFS_NAME_LEN); 
	get_prefix(parent_path, path, strlen(path));

	/* get the parent */
	rc = traverse_path(parent_path, &parent_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}
	

	/* create a new object on the storage server */
	rc = lwfs_bcreate_obj(&storage_svc, &txn, cid, &cap, &obj); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not create obj: %s",
			lwfs_err_str(rc));
		errno = EINVAL;
		rc = -errno; 
		goto cleanup; 
	}
	obj.type = LWFS_FILE_OBJ;

	/* associate the object with the file name */
	rc = lwfs_bcreate_file(&txn, &parent_ent, name, &obj, &cap, &file_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not create file: %s",
			lwfs_err_str(rc));
		errno = EINVAL;
		rc = -errno; 
		goto cleanup; 
	}

	/*
	res = mknod(path, mode, rdev);
	if(res == -1)
		return -errno;
	*/
	
cleanup:
	free(parent_path); 
	return rc; 
}

int lwfs_fuse_mkdir(const char *path, mode_t mode)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	char *parent_path = NULL;
	lwfs_ns_entry parent_ent; 
	lwfs_ns_entry dir_ent; 

	log_debug(fuse_call_debug_level, "called mkdir(%s, ...)", path);
	
	/* allocate space for the parent path */
	parent_path = (char *)malloc(strlen(path));
	if (parent_path == NULL) {
		errno = ENOMEM; 
		rc = -errno; 
		goto cleanup; 
	}

	/* extract the file name and parent object from the path */
	get_suffix(name, path, LWFS_NAME_LEN); 
	get_prefix(parent_path, path, strlen(path));

	/* get the parent */
	rc = traverse_path(parent_path, &parent_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/* create the directory */
	rc = lwfs_bcreate_dir(NULL, &parent_ent, name, 
			cid, &cap, &dir_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "error creating dir: %s", 
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/*
	   res = mkdir(path, mode);
	   if(res == -1) {
	   return -errno;
	   }
	 */

cleanup:
	free(parent_path);

	return rc;
}

int lwfs_fuse_unlink(const char *path)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	char *parent_path = NULL;
	lwfs_ns_entry parent_ent; 
	lwfs_ns_entry entry; 

	log_debug(fuse_call_debug_level, "called unlink(%s, ...)", path);

	/* allocate space for the parent path */
	parent_path = (char *)malloc(strlen(path));
	if (parent_path == NULL) {
		errno = ENOMEM; 
		rc = -errno; 
		goto cleanup; 
	}

	/* extract the file name and parent object from the path */
	get_suffix(name, path, LWFS_NAME_LEN); 
	get_prefix(parent_path, path, strlen(path));

	/* get the parent */
	rc = traverse_path(parent_path, &parent_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/*
	   res = unlink(path);
	   if(res == -1)
	   return -errno;
	 */

	/* remove the file from the namespace */
	rc = lwfs_bunlink(NULL, &parent_ent, name, 
			&cap, &entry); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "error removing file: %s", 
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup; 
	}

	/* remove the associated object if the link count is zero */
	if ((entry.link_cnt < 0) && (entry.file_obj != NULL)) {
		log_debug(fuse_debug_level, "Removing object from storage server");
		rc = lwfs_bremove_obj(NULL, entry.file_obj, &cap); 
		if (rc != LWFS_OK) {
			log_error(fuse_debug_level, "error removing object: %s", 
					lwfs_err_str(rc));
			rc = -EIO;
			goto cleanup; 
		}
	}

cleanup:
	free(parent_path);

	return rc;
}

int lwfs_fuse_rmdir(const char *path)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	char *parent_path = NULL;
	lwfs_ns_entry parent_ent; 
	lwfs_ns_entry dir_ent; 

	log_debug(fuse_call_debug_level, "called rmdir(%s, ...)", path);

	/* allocate space for the parent path */
	parent_path = (char *)malloc(strlen(path));
	if (parent_path == NULL) {
		errno = ENOMEM; 
		rc = -errno; 
		goto cleanup; 
	}

	/* extract the dir name and parent object from the path */
	get_suffix(name, path, LWFS_NAME_LEN); 
	get_prefix(parent_path, path, strlen(path));

	/* get the parent */
	rc = traverse_path(parent_path, &parent_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}


	/* remove the directory from the namespace */
	rc = lwfs_bremove_dir(NULL, &parent_ent, name, 
			&cap, &dir_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "error removing file: %s", 
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup; 
	}

cleanup:
	free(parent_path);

	return rc;

	/*
	   res = rmdir(path);
	   if(res == -1)
	   return -errno;
	 */
}


int lwfs_fuse_symlink(const char *from, const char *to)
{
	log_debug(fuse_call_debug_level, "called symlink(%s, %s)", from,to);

	log_error(fuse_debug_level, "symlink not supported");
	return -EIO;

	/*
	res = symlink(from, to);
	if(res == -1)
		return -errno;
	*/
}

int lwfs_fuse_rename(const char *from, const char *to)
{
	log_debug(fuse_call_debug_level, "called rename(%s, %s)", from,to);
	log_error(fuse_debug_level, "rename not supported");
	return -EIO;

	/*
	res = rename(from, to);
	if(res == -1)
		return -errno;
	*/
}

int lwfs_fuse_link(const char *from, const char *to)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	char *parent_path = NULL;
	char target_name[LWFS_NAME_LEN];
	char *target_path = NULL;
	lwfs_ns_entry parent_ent; 
	lwfs_ns_entry target_parent_ent; 
	lwfs_ns_entry link_ent; 

	log_debug(fuse_call_debug_level, "called link(%s, %s)", from,to);

	/* allocate space for the parent path */
	parent_path = (char *)malloc(strlen(to));
	if (parent_path == NULL) {
		errno = ENOMEM; 
		rc = -errno; 
		goto cleanup; 
	}

	/* allocate space for the target path */
	target_path = (char *)malloc(strlen(from));
	if (target_path == NULL) {
		errno = ENOMEM; 
		rc = -errno; 
		goto cleanup; 
	}

	/* extract the dir name and parent object from the path */
	get_suffix(name, to, LWFS_NAME_LEN); 
	get_prefix(parent_path, to, strlen(to));

	/* get the parent */
	rc = traverse_path(parent_path, &parent_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	/* extract the dir name and parent object from the target path */
	get_suffix(target_name, from, LWFS_NAME_LEN); 
	get_prefix(target_path, from, strlen(from));

	/* get the parent */
	rc = traverse_path(target_path, &target_parent_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}


	/* create a link to the target */
	rc = lwfs_bcreate_link(NULL, &parent_ent, name, &cap, 
			&target_parent_ent, target_name, &cap, 
			&link_ent); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "error creating link: %s", 
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup; 
	}

cleanup:
	free(parent_path);
	free(target_path);

	return rc;

	/*
	res = link(from, to);
	if(res == -1)
		return -errno;
	*/
}

int lwfs_fuse_chmod(const char *path, mode_t mode)
{
	log_debug(fuse_call_debug_level, "called chmod(%s...)", path);
	log_error(fuse_debug_level, "chmod not supported");
	return -EIO;

	/*
	res = chmod(path, mode);
	if(res == -1)
		return -errno;
	*/
}

int lwfs_fuse_chown(const char *path, uid_t uid, gid_t gid)
{
	log_debug(fuse_call_debug_level, "called chown(%s...)", path);
	log_error(fuse_debug_level, "chown not supported");
	return -EIO;

	/*
	res = lchown(path, uid, gid);
	if(res == -1)
		return -errno;
	*/
}

int lwfs_fuse_truncate(const char *path, off_t size)
{
	int rc = LWFS_OK;
	lwfs_ns_entry ent; 

	log_debug(fuse_call_debug_level, "called truncate(%s, %d)", path, size);

	/* get the object */
	rc = traverse_path(path, &ent); 
	if (rc != LWFS_OK) {
		log_warn(fuse_debug_level, "cound not traverse path: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* truncate the file object */
	if (ent.file_obj == NULL) {
		log_error(fuse_debug_level, "NULL file object in entry: %s",
			lwfs_err_str(rc));
		errno = EIO;
		return -EIO;
	}

	rc = lwfs_btruncate(&txn, ent.file_obj, size, &cap); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not truncate object: %s",
			lwfs_err_str(rc));
		errno = EIO;
		return -EIO;
	}

	/*
	res = truncate(path, size);
	if(res == -1)
		return -errno;
	*/

	return rc; 
}

int lwfs_fuse_utime(const char *path, struct utimbuf *buf)
{
	log_debug(fuse_call_debug_level, "called utime(%s...)", path);
	log_error(fuse_debug_level, "utime not supported");
	return -EIO;

	/*
	res = utime(path, buf);
	if(res == -1)
		return -errno;
	*/
}


int lwfs_fuse_open(const char *path, struct fuse_file_info *fi)
{
	int res = 0;

	log_debug(fuse_call_debug_level, "called open(%s...)", path);

	/*
	res = open(path, fi->flags);
	if(res == -1)
		return -errno;

	close(res);
	*/
	return res;
}

int lwfs_fuse_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	int rc = 0;
	lwfs_ns_entry ent; 
	int count; 

	(void)fi;  /* not used */

	log_debug(fuse_call_debug_level, "called read(%s, offset=%d, len=%d, ...)",
		path, offset, size);

	/* get the object */
	rc = traverse_path(path, &ent); 
	if (rc != LWFS_OK) {
		log_warn(fuse_debug_level, "cound not traverse path: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	if (ent.file_obj == NULL) {
		log_error(fuse_debug_level, "NULL file object in entry");
		errno = EIO;
		return -EIO;
	}

	/* read from to the object */
	rc = lwfs_bread(&txn, ent.file_obj, offset, buf, size, &cap, &count); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not write data: %s",
			lwfs_err_str(rc));
		errno = EIO;
		return -EIO;
	}

	/*
	int fd;

	(void) fi;
	fd = open(path, O_RDONLY);
	if(fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if(res == -1)
		res = -errno;

	close(fd);
	*/

	/* return the number of bytes actually read */
	return count; 
}

int lwfs_fuse_write(const char *path, const char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi)
{
	int rc = 0;
	lwfs_ns_entry ent; 

	log_debug(fuse_call_debug_level, "called write(%s...)", path);

	/* get the object */
	rc = traverse_path(path, &ent); 
	if (rc != LWFS_OK) {
		log_warn(fuse_debug_level, "cound not traverse path: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	if (ent.file_obj == NULL) {
		log_error(fuse_debug_level, "NULL file object in entry");
		errno = EIO;
		return -EIO;
	}

	/* write to the object */
	rc = lwfs_bwrite(&txn, ent.file_obj, offset, buf, size, &cap); 
	if (rc != LWFS_OK) {
		log_error(fuse_debug_level, "could not write data: %s",
			lwfs_err_str(rc));
		errno = EIO;
		return -EIO;
	}


	/*
	int fd;

	(void) fi;
	fd = open(path, O_WRONLY);
	if(fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if(res == -1)
		res = -errno;

	close(fd);
	*/
	return size;
}

int lwfs_fuse_statfs(const char *path, struct statfs *stbuf)
{
	int res = 0;

	log_debug(fuse_call_debug_level, "called statfs(%s...)", path);

	memset(stbuf, 0, sizeof(struct statfs));

	stbuf->f_bsize   = 500; 
	stbuf->f_blocks  = 1000000;
	stbuf->f_bfree   = 999990;
	stbuf->f_bavail  = 999999; 
	stbuf->f_files   = 999999;
	stbuf->f_ffree   = 999999;
	stbuf->f_namelen = LWFS_NAME_LEN; 

	/*
	res = statfs(path, stbuf);
	if(res == -1)
		return -errno;
	*/

	return res;
}

int lwfs_fuse_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.  This method is optional and can safely be left
	   unimplemented */

	log_debug(fuse_call_debug_level, "called release(%s...)", path);

	(void) path;
	(void) fi;
	return 0;
}

int lwfs_fuse_fsync(const char *path, int isdatasync,
		struct fuse_file_info *fi)
{
	/* Just a stub.  This method is optional and can safely be left
	   unimplemented */

	log_debug(fuse_call_debug_level, "called fsync(%s...)", path);

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
int lwfs_fuse_setxattr(const char *path, const char *name, const char *value,
		size_t size, int flags)
{
	int res = 0;
	

	log_debug(fuse_call_debug_level, "called setxattr(%s...)", path);

	/*
	res = lsetxattr(path, name, value, size, flags);
	if(res == -1)
		return -errno;
	*/
	return res;
}

int lwfs_fuse_getxattr(const char *path, const char *name, char *value,
		size_t size)
{
	int res = 0; 
	
	log_debug(fuse_call_debug_level, "called getxattr(%s...)", path);

	/*
	res = lgetxattr(path, name, value, size);
	if(res == -1)
		return -errno;
	*/
	return res;
}

int lwfs_fuse_listxattr(const char *path, char *list, size_t size)
{
	int res = 0; 
	
	log_debug(fuse_call_debug_level, "called listxattr(%s...)", path);

	/*
	res = llistxattr(path, list, size);
	if(res == -1)
		return -errno;
	*/
	return res;
}

int lwfs_fuse_removexattr(const char *path, const char *name)
{
	int res = 0;
	
	log_debug(fuse_call_debug_level, "called removexattr(%s...)", path);

	/*
	res = lremovexattr(path, name);
	if(res == -1)
		return -errno;
	*/
	return res;
}
#endif /* HAVE_SETXATTR */
