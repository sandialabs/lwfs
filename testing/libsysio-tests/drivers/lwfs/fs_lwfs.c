#ifdef __linux__
#define _BSD_SOURCE
#endif


/* system header files  */
#include <stdio.h>					/* for NULL */
#include <stdlib.h>
#ifdef __linux__
#include <string.h>
#endif
#include <unistd.h>
#if !(defined(REDSTORM) || defined(MAX_IOVEC))
#include <limits.h>
#endif
#include <errno.h>
#include <assert.h>
#include <syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#ifdef _HAVE_STATVFS
#include <sys/statvfs.h>
#include <sys/statfs.h>
#endif
#include <utime.h>
#include <sys/uio.h>
#include <sys/queue.h>

/* included from libsysio */
#include "sysio.h"
#include "xtio.h"
#include "fs.h"
#include "mount.h"
#include "inode.h"

/* loca includes */
#include "fs_lwfs.h"

/* included from LWFS */
#include "support/logger/logger.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/storage_client/storage_client_sync.h"
#include "client/naming_client/naming_client_sync.h"

extern int parse_config_file(const char *, lwfs_filesystem *);

/*
 * Kernel version of directory entry.
 */
typedef struct {
	unsigned long ld_ino;
	unsigned long ld_off;
	unsigned short ld_reclen;
	char	ld_name[1];
} linux_dirent;

#include <dirent.h>

log_level sysio_debug_level = LOG_UNDEFINED;

typedef struct {
	int id;
} lwfs_identifier;

typedef struct {
	lwfs_ns_entry ns_entry;         /* Identifies the LWFS entry */
	lwfs_cap cap;                   /* enables operations on the entry */
	struct file_identifier fileid;  /* unique file identifier */
	int oflags;                     /* flags from open */
	unsigned nopens;                /* soft ref count */
	_SYSIO_OFF_T fpos;              /* current position */
	time_t attrtim;
} lwfs_inode;



/* ---- FUNCTION PROTOTYPES ----- */
static int lwfs_inop_lookup(struct pnode *pno,
			      struct inode **inop,
			      struct intent *intnt,
			      const char *path);
static int lwfs_inop_getattr(struct pnode *pno,
			       struct inode *ino,
			       struct intnl_stat *stbuf);
static int lwfs_inop_setattr(struct pnode *pno,
			       struct inode *ino,
			       unsigned mask,
			       struct intnl_stat *stbuf);
static ssize_t lwfs_filldirentries(struct inode *ino,
				     _SYSIO_OFF_T *posp,
				     char *buf,
				     size_t nbytes);
static int lwfs_inop_mkdir(struct pnode *pno, mode_t mode);
static int lwfs_inop_rmdir(struct pnode *pno);
static int lwfs_inop_symlink(struct pnode *pno, const char *data);
static int lwfs_inop_readlink(struct pnode *pno, char *buf, size_t bufsiz);
static int lwfs_inop_open(struct pnode *pno, int flags, mode_t mode);
static int lwfs_inop_close(struct inode *ino);
static int lwfs_inop_link(struct pnode *old, struct pnode *new);
static int lwfs_inop_unlink(struct pnode *pno);
static int lwfs_inop_rename(struct pnode *old, struct pnode *new);
static int lwfs_inop_read(struct inode *ino, struct ioctx *ioctx);
static int lwfs_inop_write(struct inode *ino, struct ioctx *ioctx);
static _SYSIO_OFF_T lwfs_inop_pos(struct inode *ino, _SYSIO_OFF_T off);
static int lwfs_inop_iodone(struct ioctx *ioctx);
static int lwfs_inop_fcntl(struct inode *ino, int cmd, va_list ap, int *rtn);
static int lwfs_inop_sync(struct inode *ino);
static int lwfs_inop_datasync(struct inode *ino);
static int lwfs_inop_ioctl(struct inode *ino,
			     unsigned long int request,
			     va_list ap);
static int lwfs_inop_mknod(struct pnode *pno, mode_t mode, dev_t dev);
#ifdef _HAVE_STATVFS
static int lwfs_inop_statvfs(struct pnode *pno,
			       struct inode *ino,
			       struct intnl_statvfs *buf);
#endif
static void lwfs_inop_gone(struct inode *ino);

static struct inode *lwfs_i_new(struct filesys *,
		time_t,
		struct intnl_stat *,
		const lwfs_ns_entry *);

static struct inode_ops lwfs_i_ops = {
	.inop_lookup = lwfs_inop_lookup,
	.inop_getattr = lwfs_inop_getattr,
	.inop_setattr = lwfs_inop_setattr,
	.inop_filldirentries = lwfs_filldirentries,
	.inop_mkdir = lwfs_inop_mkdir,
	.inop_rmdir = lwfs_inop_rmdir,
	.inop_symlink = lwfs_inop_symlink,
	.inop_readlink = lwfs_inop_readlink,
	.inop_open = lwfs_inop_open,
	.inop_close = lwfs_inop_close,
	.inop_link = lwfs_inop_link,
	.inop_unlink = lwfs_inop_unlink,
	.inop_rename = lwfs_inop_rename,
	.inop_read = lwfs_inop_read,
	.inop_write = lwfs_inop_write,
	.inop_pos = lwfs_inop_pos,
	.inop_iodone = lwfs_inop_iodone,
	.inop_fcntl = lwfs_inop_fcntl,
	.inop_sync = lwfs_inop_sync,
	.inop_datasync = lwfs_inop_datasync,
	.inop_ioctl = lwfs_inop_ioctl,
	.inop_mknod = lwfs_inop_mknod,
#ifdef _HAVE_STATVFS
	.inop_statvfs = lwfs_inop_statvfs,
#endif
	.inop_gone = lwfs_inop_gone
};

static int lwfs_fsswop_mount(const char *source,
			       unsigned flags,
			       const void *data,
			       struct pnode *tocover,
			       struct mount **mntp);

static struct fssw_ops lwfs_fssw_ops = {
	.fsswop_mount = lwfs_fsswop_mount
};

static void lwfs_fsop_gone(struct filesys *fs);

static struct filesys_ops lwfs_inodesys_ops = {
	.fsop_gone = lwfs_fsop_gone,
};

/*
 * LWFS IO path arguments.
 */
typedef struct {
	char             lio_op;	/* 'r' or 'w' */
	lwfs_inode      *lio_lino;	/* lwfs ino */
	lwfs_filesystem *lio_fs;
} lwfs_io;


/* ---- PRIVATE FUNCTION PROTOTYPES ----- */
static int traverse_path(
	lwfs_filesystem *lwfs_fs,
	const char *path,
	lwfs_ns_entry *result);


/*
 * This example driver plays a strange game. It maintains a private,
 * internal mount -- It's own separate, rooted, name space. The local
 * file system's entire name space is available via this tree.
 *
 * This simplifies the implementation. At mount time, we need to generate
 * a path-node to be used as a root. This allows us to look up the needed
 * node in the host name space and leverage a whole lot of support from
 * the system.
 */
static struct mount *lwfs_internal_mount = NULL;

/*
 * Given i-node, return driver private part.
 */
#define I2LI(ino) \
	((lwfs_inode *)((ino)->i_private))

/*
 * Given fs, return driver private part.
 */
#define FS2LFS(fs) \
	((lwfs_filesystem *)(fs)->fs_private)

/*
 * Given pnode, return the fs
 */
#define PNODE_FS(pno) \
	((pno)->p_mount->mnt_fs)

/*
 * Given inode, return the fs
 */
#define INODE_FS(ino) \
	((ino)->i_fs)

/*
 * Given pnode, return its parent
 */
#define PNODE_PARENT(pno) \
	((pno)->p_parent)

/*
 * Given pnode, return the ns_entry
 */
#define PNODE_NS_ENTRY(pno) \
	((lwfs_ns_entry *)((pno)->p_base->pb_ino->i_private))

/*
 * Copy a pnode's name into 'n'
 */
#define COPY_PNODE_NAME(n, pno) \
	memset(n, 0, LWFS_NAME_LEN); \
	memcpy(n, pno->p_base->pb_name.name, pno->p_base->pb_name.len);
	
/*
 * Functions to manipulate storage obj
 */

/**
 * @brief Allocate 'dso_count' in memory lwfs_objs to store references to 
 *        the storage objects that compose a file.
 *
 * @param dso @output array to storage object references
 * @param dso_count @input the number of object reference to allocate
 */
static int
sso_alloc_dso(lwfs_obj **dso,
	      int dso_count)
{
	int rc = LWFS_OK;
	
	log_debug(sysio_debug_level, "entered sso_alloc_dso");

	/* allocate 1 storage obj for each of 'dso_count' storage servers */
	*dso = (lwfs_obj *)calloc(dso_count, sizeof(lwfs_obj));
	if (*dso == NULL) {
		log_error(sysio_debug_level, "could not allocate dso array");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_alloc_dso");

	return LWFS_OK;
}

/**
 * @brief Allocate an in memory lwfs_obj to store a reference to 
 *        a storage object that stores 
 *
 * @param mo @output storage object reference to a management obj
 */
static int
sso_alloc_mo(lwfs_obj **mo)
{
	int rc = LWFS_OK;
	
	log_debug(sysio_debug_level, "entered sso_alloc_mo");

	/* allocate a management obj */
	*mo = (lwfs_obj *)calloc(1, sizeof(lwfs_obj));
	if (*mo == NULL) {
		log_error(sysio_debug_level, "could not allocate a management obj");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_alloc_mo");

	return LWFS_OK;
}

/**
 * @brief Initiate and create 'dso_count' lwfs_objs in the 'cid' container 
 *        on the storage servers referenced in 'lwfs_fs'.
 *
 * @param lwfs_fs @input the LWFS filesystem on which to create the objects
 * @param cid @input the container in which to create the objects
 * @param dso @input array to storage object references
 * @param dso_count @input the number of object references in dso
 */
static int
sso_create_dso(lwfs_filesystem *lwfs_fs,
	       lwfs_cid cid,
	       lwfs_obj *dso,
	       int dso_count)
{
	int rc = LWFS_OK;
	int i;
	
	log_debug(sysio_debug_level, "entered sso_create_dso");

	/* create 1 storage obj on each of 'dso_count' storage servers */
	for (i=0; i<dso_count; i++) {
		lwfs_init_obj(&(lwfs_fs->storage_svc[i]),
			      LWFS_FILE_OBJ,
			      cid,
			      LWFS_OID_ANY,
			      &(dso[i]));
		rc = lwfs_create_obj_sync(&lwfs_fs->txn,
					  &(dso[i]),
					  &lwfs_fs->cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "error creating *dso[%d]", i);
			goto cleanup;
		}
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_create_dso");
	
	return LWFS_OK;
}

/**
 * @brief Initiate and create 'dso_count' lwfs_objs in the 'cid' container 
 *        on the storage servers referenced in 'lwfs_fs'.
 *
 * @param lwfs_fs @input the LWFS filesystem on which to create the objects
 * @param cid @input the container in which to create the objects
 * @param dso @input array to storage object references
 */
static int
sso_create_mo(lwfs_filesystem *lwfs_fs,
	      lwfs_cid cid,
	      lwfs_obj *mo)
{
	int rc = LWFS_OK;

	log_debug(sysio_debug_level, "entered sso_create_mo");

	/* allocate a management obj on one of the storage servers */
	lwfs_init_obj(&(lwfs_fs->storage_svc[0]),
		      LWFS_FILE_OBJ,
		      cid,
		      LWFS_OID_ANY,
		      mo);
	rc = lwfs_create_obj_sync(&lwfs_fs->txn,
				  mo,
				  &lwfs_fs->cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error creating mo for %s");
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished sso_create_mo");

	return LWFS_OK;
}

/**
 * @brief Remove 'dso_count' lwfs_objs from the storage servers 
 * referenced in 'lwfs_fs'.
 *
 * @param lwfs_fs @input the LWFS filesystem where the objects reside
 * @param dso @input array of storage objects to remove
 * @param dso_count @input the number of object references in dso
 */
static int
sso_remove_dso(lwfs_filesystem *lwfs_fs,
	       lwfs_obj *dso,
	       int dso_count)
{
	int rc = LWFS_OK;
	int i;
	
	log_debug(sysio_debug_level, "entered sso_remove_dso");

	/* remove 1 storage obj on each of 'dso_count' storage servers */
	for (i=0; i<dso_count; i++) {
		rc = lwfs_remove_obj_sync(&lwfs_fs->txn,
					  &(dso[i]),
					  &lwfs_fs->cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "error removing *dso[%d]", i);
			goto cleanup;
		}
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_remove_dso");
	
	return LWFS_OK;
}

static int
sso_store_mo(lwfs_filesystem *lwfs_fs,
	     lwfs_obj *mo,
	     int chunk_size,
	     lwfs_obj *dso,
	     int dso_count)
{
	int rc = LWFS_OK;

	int offset=0;

	log_debug(sysio_debug_level, "entered sso_store_mo");

	/* put header info in the MO */
	rc = lwfs_write_sync(&lwfs_fs->txn, 
			     mo,
			     offset, (void *)&chunk_size, sizeof(int),
			     &lwfs_fs->cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not write chunk_size: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += sizeof(int);
	rc = lwfs_write_sync(&lwfs_fs->txn, 
			     mo,
			     offset, (void *)&dso_count, sizeof(int),
			     &lwfs_fs->cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not write the dso_count: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += sizeof(int);
	/* put obj refs to DSOs in the MO */
	rc = lwfs_write_sync(&lwfs_fs->txn, 
			     mo,
			     offset, (void *)dso, (dso_count * sizeof(lwfs_obj)),
			     &lwfs_fs->cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not write the dso array: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += (dso_count * sizeof(lwfs_obj));

cleanup:
	log_debug(sysio_debug_level, "finished sso_store_mo");

	return LWFS_OK;
}

static int
sso_load_mo(lwfs_filesystem *lwfs_fs,
	    lwfs_ns_entry *ns_entry)
{
	int rc = LWFS_OK;

	lwfs_obj *mo = ns_entry->file_obj;
	
	int offset = 0;
	
	lwfs_size nbytes = 0;

	log_debug(sysio_debug_level, "entered sso_load_mo");

	ns_entry->d_obj = (lwfs_distributed_obj *)calloc(1, sizeof(lwfs_distributed_obj));
	if (ns_entry->d_obj == NULL) {
		log_error(sysio_debug_level, "could not allocate distributed obj");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}

	/* put header info in the MO */
	rc = lwfs_read_sync(&lwfs_fs->txn, 
			    mo,
			    offset, (void *)&ns_entry->d_obj->chunk_size, sizeof(int),
			    &lwfs_fs->cap,
			    &nbytes);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not read chunk_size: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += sizeof(int);

	rc = lwfs_read_sync(&lwfs_fs->txn, 
			    mo,
			    offset, (void *)&ns_entry->d_obj->ss_obj_count, sizeof(int),
			    &lwfs_fs->cap,
			    &nbytes);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not read the dso_count: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += sizeof(int);
	
	/* allocate space for the dso array */
	ns_entry->d_obj->ss_obj = (lwfs_obj *)calloc(ns_entry->d_obj->ss_obj_count, sizeof(lwfs_obj));
	if (ns_entry->d_obj->ss_obj == NULL) {
		log_error(sysio_debug_level, "could not allocate dso array");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	
	/* put obj refs to DSOs in the MO */
	rc = lwfs_read_sync(&lwfs_fs->txn, 
			    mo,
			    offset, 
			    (void *)ns_entry->d_obj->ss_obj, 
			    (ns_entry->d_obj->ss_obj_count * sizeof(lwfs_obj)),
			    &lwfs_fs->cap,
			    &nbytes);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not read the dso array: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += (ns_entry->d_obj->ss_obj_count * sizeof(lwfs_obj));
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_load_mo");

	return LWFS_OK;
}

static int
sso_create_objs(lwfs_filesystem *lwfs_fs,
		lwfs_ns_entry *parent,
		lwfs_ns_entry *ns_entry)
{
	int rc = LWFS_OK;
	
	int ss_cnt = ns_entry->d_obj->ss_obj_count;
	int chunk_size = ns_entry->d_obj->chunk_size;
	lwfs_obj *dso = ns_entry->d_obj->ss_obj;

	log_debug(sysio_debug_level, "entered sso_create_objs");

	if (logging_debug(sysio_debug_level))
		fprint_lwfs_ns_entry(logger_get_file(), "ns_entry_name", "ns_entry before sso_create_dso", ns_entry);

	/* create 'ss_cnt' storage objs */
	rc = sso_create_dso(lwfs_fs, parent->entry_obj.cid, dso, ss_cnt);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to create dso: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}
	/* create a storage obj to store refs to the DSOs and stripe size info */
	rc = sso_create_mo(lwfs_fs, parent->entry_obj.cid, ns_entry->file_obj);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to create mo: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}
	/* put the DSOs plus stripe size in the MO */
	rc = sso_store_mo(lwfs_fs, ns_entry->file_obj, chunk_size, dso, ss_cnt);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to store the mo: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}

	if (logging_debug(sysio_debug_level))
		fprint_lwfs_ns_entry(logger_get_file(), "ns_entry_name", "ns_entry after sso_create_dso", ns_entry);
		
cleanup:
	log_debug(sysio_debug_level, "finished sso_create_objs");

	return rc;
}

static int
sso_init_objs(lwfs_filesystem *lwfs_fs,
	      lwfs_ns_entry *ns_entry)
{
	int rc = LWFS_OK;
	
	int ss_cnt_available;
	int ss_cnt_desired;
	int default_chunk_size;
	int desired_chunk_size;

	log_debug(sysio_debug_level, "entered sso_init_objs");
	
	ns_entry->d_obj = (lwfs_distributed_obj *)calloc(1, sizeof(lwfs_distributed_obj));

	/* determine the nunber of storage servers available (ss_available) */
	ss_cnt_available = lwfs_fs->num_servers;
	/* determine the number of storage servers the ns_entry wants to use (ss_desired) */
	ss_cnt_desired = 0; /* ns_entry->num_servers */
	/* if ss_cnt_desired is non-sero, use it, otherwise use the default */
	ns_entry->d_obj->ss_obj_count = ss_cnt_desired ? ss_cnt_desired : ss_cnt_available;
	/* allocate 'ss_cnt' data storage objects (DSOs) */
	rc = sso_alloc_dso(&ns_entry->d_obj->ss_obj, ns_entry->d_obj->ss_obj_count);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to allocate dso: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}
	/* determine the default chunk size */
	default_chunk_size = lwfs_fs->default_chunk_size;
	/* determine if the ns_entry specified a chunk size */
	desired_chunk_size = 0; /* ns_entry->chunk_size; */
	/* if default_chunk_size is non-sero, use it, otherwise use the default */
	ns_entry->d_obj->chunk_size = desired_chunk_size ? desired_chunk_size : default_chunk_size;
	/* allocate the management obj (MO) */
	rc = sso_alloc_mo(&ns_entry->file_obj);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to allocate the mo: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished sso_init_objs");
	
	return rc;
}

static void
sso_calc_obj_index_offset(lwfs_filesystem *lwfs_fs,
			  lwfs_ns_entry *ns_entry,
			  _SYSIO_OFF_T file_offset,
			  _SYSIO_OFF_T *obj_index,
			  _SYSIO_OFF_T *obj_offset)
{
	int rc = LWFS_OK;
	
	_SYSIO_OFF_T chunk_number=0;
	_SYSIO_OFF_T stripe_number=0;

	log_debug(sysio_debug_level, "entered sso_calc_obj_index_offset");

	chunk_number = file_offset / ns_entry->d_obj->chunk_size;
	stripe_number = chunk_number / ns_entry->d_obj->ss_obj_count;
	*obj_index  = chunk_number % ns_entry->d_obj->ss_obj_count;
	*obj_offset = (stripe_number * ns_entry->d_obj->chunk_size) + (file_offset % ns_entry->d_obj->chunk_size);

	log_debug(sysio_debug_level, "finished sso_calc_obj_index_offset");
}

static int
sso_next_obj_index(lwfs_ns_entry *ns_entry,
		   int obj_index)
{
	return ( (obj_index++) % ns_entry->d_obj->ss_obj_count );
}

static int
sso_calc_first_bytes_to_io(lwfs_ns_entry *ns_entry,
			   size_t bytes_left,
			   _SYSIO_OFF_T first_obj_offset,
			   _SYSIO_OFF_T first_obj_bytes_avail)
{
	int bytes_to_io=0;
	
	if (first_obj_offset > 0) {
		/* the write starts in the middle of the first obj */
		if (bytes_left > first_obj_bytes_avail) {
			/* the total bytes to write is > bytes left in the first  */
			/* chunk of the first obj, must span multiple objs */
			bytes_to_io = first_obj_bytes_avail;
		} else {
			/* otherwise all the bytes fit in the first chunk of  */
			/* the first obj */
			bytes_to_io = bytes_left;
		}
	} else {
		/* otherwise we are starting at offset 0 of the  */
		/* first chunk of the first obj */
		if (bytes_left > ns_entry->d_obj->chunk_size) {
			/* the total bytes to write is > a whole chunk,  */
			/* must span multiple objs */
			bytes_to_io = ns_entry->d_obj->chunk_size;
		} else {
			/* otherwise all the bytes fit in the first chunk of  */
			/* the first obj */
			bytes_to_io = bytes_left;
		}
	}
	
	return bytes_to_io;
}

static int
sso_doio(lwfs_io *lio, lwfs_obj *obj, _SYSIO_OFF_T off, void *buf, size_t count, lwfs_size *nbytes)
{
	int rc = LWFS_OK;
	
	lwfs_filesystem *lwfs_fs = lio->lio_fs;
	lwfs_ns_entry *ns_entry = &lio->lio_lino->ns_entry;

	log_debug(sysio_debug_level, "entered sso_doio");

	if (lio->lio_op == 'r') {
		log_debug(sysio_debug_level, "offset==%ld; count==%ld", off, count);
		/* read from the object */
		rc = lwfs_read_sync(&lwfs_fs->txn, obj,
				    off, buf, count,
				    &lwfs_fs->cap, nbytes);
		log_debug(sysio_debug_level, "bytes read (nbytes==%u)", *nbytes);
		log_debug(sysio_debug_level, "lwfs_read_sync result: %s", lwfs_err_str(rc));
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "could not read data: %s",
				lwfs_err_str(rc));
			errno = EIO;
			rc = -EIO;
			goto cleanup;
		}
	}
	if (lio->lio_op == 'w') {
		/* write to the object */
		rc = lwfs_write_sync(&lwfs_fs->txn, obj,
				     off, buf, count,
				     &lwfs_fs->cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "could not write data: %s",
				lwfs_err_str(rc));
			errno = EIO;
			rc = -EIO;
			goto cleanup;
		}
		*nbytes = count;
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_doio");

	return rc;
}

static size_t
sso_io(void *buf, size_t count, _SYSIO_OFF_T off, lwfs_io *lio)
{
	int rc = LWFS_OK;

	lwfs_filesystem *lwfs_fs = lio->lio_fs;
	lwfs_ns_entry *ns_entry = &lio->lio_lino->ns_entry;
	lwfs_inode *lino = lio->lio_lino;
	
	_SYSIO_OFF_T first_obj_index=0;
	_SYSIO_OFF_T first_obj_offset=0;
	_SYSIO_OFF_T first_obj_bytes_avail=0;
	
	_SYSIO_OFF_T obj_index=0;
	_SYSIO_OFF_T obj_offset=0;
	
	_SYSIO_OFF_T buf_offset=0;
	_SYSIO_OFF_T file_offset=off;

	/* the number of bytes to read/write during this round of I/O */
	size_t bytes_this_io=0;
	/* the number of bytes left to read/write */
	size_t bytes_left=count;
	/* the number of bytes read/written during to round of I/O */
	lwfs_size nbytes=0;

	log_debug(sysio_debug_level, "entered sso_io");
	
	sso_calc_obj_index_offset(lwfs_fs, ns_entry, file_offset, &first_obj_index, &first_obj_offset);
	first_obj_bytes_avail = ns_entry->d_obj->chunk_size - (first_obj_offset % ns_entry->d_obj->chunk_size);
	
	log_debug(sysio_debug_level, "first_obj_index == %ld; first_obj_offset == %ld; first_obj_bytes_avail == %ld", 
		  first_obj_index, first_obj_offset, first_obj_bytes_avail);
	
	bytes_this_io = sso_calc_first_bytes_to_io(ns_entry, bytes_left, first_obj_offset, first_obj_bytes_avail);

	log_debug(sysio_debug_level, "bytes_this_io == %d", bytes_this_io);

	rc = sso_doio(lio, 
		      &ns_entry->d_obj->ss_obj[first_obj_index], 
		      first_obj_offset, 
		      buf, 
		      bytes_this_io, 
		      &nbytes);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "the I/O failed: %s",
			lwfs_err_str(rc));
	}
	if (nbytes == 0) {
		/* reached EOF */
		log_debug(sysio_debug_level, "reached EOF");
		rc = 0;
		goto cleanup;
	}
	bytes_left -= nbytes;
	buf_offset += nbytes;
	file_offset += nbytes;
	
	log_debug(sysio_debug_level, "bytes_left == %d", bytes_left);
	log_debug(sysio_debug_level, "buf_offset == %ld", buf_offset);
	log_debug(sysio_debug_level, "file_offset == %ld", file_offset);

	sso_calc_obj_index_offset(lwfs_fs, ns_entry, file_offset, &obj_index, &obj_offset);
	while (bytes_left > 0) {
		if (bytes_left > ns_entry->d_obj->chunk_size) {
			bytes_this_io = ns_entry->d_obj->chunk_size;
		} else {
			bytes_this_io = bytes_left;
		}
		rc = sso_doio(lio, 
			      &ns_entry->d_obj->ss_obj[obj_index], 
			      obj_offset, 
			      buf + buf_offset, 
			      bytes_this_io, 
			      &nbytes);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "the I/O failed: %s",
				lwfs_err_str(rc));
			goto cleanup;
		}
		
		bytes_left -= nbytes;
		buf_offset += nbytes;
		file_offset += nbytes;

		log_debug(sysio_debug_level, "bytes_left == %d", bytes_left);
		log_debug(sysio_debug_level, "buf_offset == %ld", buf_offset);
		log_debug(sysio_debug_level, "file_offset == %ld", file_offset);

		if (nbytes < bytes_this_io) {
			log_debug(sysio_debug_level, "short I/O, jumping out");
			rc = buf_offset;
			goto cleanup;
		}

		sso_calc_obj_index_offset(lwfs_fs, ns_entry, file_offset, &obj_index, &obj_offset);
	}
	
	rc = buf_offset;

cleanup:
	log_debug(sysio_debug_level, "finished sso_io");

	return rc;
}
       

/*
 * Initialize this driver.
 */
int
_sysio_lwfs_init()
{
	char *use_logger=NULL;
	
	/*
	 * Capture current process umask and reset our process umask to
	 * zero. All permission bits to open/creat/setattr are absolute --
	 * They've already had a umask applied, when appropriate.
	 */
	_sysio_umask = syscall(SYSIO_SYS_umask, 0);

	use_logger = getenv("LWFS_USE_LOGGER");
	if ((use_logger != NULL) && (!strcmp("TRUE", use_logger))) { 
		logger_init(LOG_ALL, NULL);
	} else {
		logger_init(LOG_OFF, NULL);
	}

	return _sysio_fssw_register("lwfs", &lwfs_fssw_ops);
}

static int copy_lwfs_stat(struct intnl_stat *stbuf,
			  lwfs_stat_data *stat_data)
{
	log_debug(sysio_debug_level, "entered copy_lwfs_stat");

	stbuf->st_size=stat_data->size;				/* total size, in bytes */
	stbuf->st_blocks=(stat_data->size/stbuf->st_blksize);	/* number of whole blocks allocated */
	if ((stat_data->size % stbuf->st_blksize) != 0) {
		stbuf->st_blocks++;				/* add one if there is a partial block */
	}
	stbuf->st_atime=stat_data->atime.seconds;		/* time of last access */
	stbuf->st_mtime=stat_data->mtime.seconds;		/* time of last modification */
	stbuf->st_ctime=stat_data->ctime.seconds;		/* time of last change */

	log_debug(sysio_debug_level, "finished copy_lwfs_stat");

	return LWFS_OK;
}

static int set_mode_by_type(struct intnl_stat *stbuf,
			    int obj_type)
{
	log_debug(sysio_debug_level, "entered set_mode_by_type");

	switch (obj_type) {

		case LWFS_FILE_ENTRY:
			stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
			break;

		case LWFS_FILE_OBJ:
			stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
			break;

		case LWFS_DIR_ENTRY:
		case LWFS_NS_OBJ:
			stbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
			break;

		case LWFS_LINK_ENTRY:
			stbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
			break;

		default:
			stbuf->st_mode = 0;
			break;
	}

	log_debug(sysio_debug_level, "finished set_mode_by_type");

	return LWFS_OK;
}

static int set_stat_from_inode(struct intnl_stat *stbuf,
			       const struct filesys *fs,
			       const lwfs_ns_entry *entry)
{
	log_debug(sysio_debug_level, "entered set_stat_from_inode");

	stbuf->st_dev = fs->fs_dev;							/* device */
	stbuf->st_ino = entry->entry_obj.oid;						/* inode */
	stbuf->st_nlink = entry->link_cnt;     						/* number of hard links */
	memcpy(&stbuf->st_uid, FS2LFS(fs)->cap.data.cred.data.uid, sizeof(uid_t));	/* user ID of owner */
	memcpy(&stbuf->st_gid, FS2LFS(fs)->cap.data.cred.data.uid, sizeof(uid_t));	/* group ID of owner */
	stbuf->st_rdev = 0;								/* device type (if inode device) */
	if (entry->d_obj != NULL) {
		stbuf->st_blksize = entry->d_obj->chunk_size;				/* blocksize for filesystem I/O */
	} else {
		stbuf->st_blksize = FS2LFS(fs)->default_chunk_size;				/* blocksize for filesystem I/O */
	}

	log_debug(sysio_debug_level, "finished set_stat_from_inode");

	return LWFS_OK;
}

static int init_fileobj(lwfs_ns_entry *ns_entry,
			lwfs_service *storage_svc)
{
	ns_entry->file_obj = (lwfs_obj *)malloc(sizeof(lwfs_obj));

	log_debug(sysio_debug_level, "entered init_fileobj");

	if (ns_entry->file_obj == NULL)
	{
		log_debug(sysio_debug_level, "failed to alloc file_obj");
		return LWFS_ERR_NOSPACE;
	}
	memcpy(&ns_entry->file_obj->svc, storage_svc, sizeof(lwfs_service));
	ns_entry->file_obj->type = LWFS_FILE_OBJ;

	log_debug(sysio_debug_level, "finished init_fileobj");

	return LWFS_OK;
}


static int
create_namespace(lwfs_filesystem *lwfs_fs, char *ns_name)
{
	int rc=LWFS_OK;
	lwfs_cid cid=123456;

	rc = lwfs_create_namespace_sync(&lwfs_fs->naming_svc,
					&lwfs_fs->txn,
					ns_name,
					cid,
					&lwfs_fs->namespace);
	if (rc != LWFS_OK) {
		log_debug(sysio_debug_level, "could not create namespace (%s): %s",
				ns_name, lwfs_err_str(rc));
	}

	return rc;
}


static int
lwfs_connect(lwfs_filesystem *lwfs_fs)
{
	int rc = LWFS_OK;
	lwfs_opcode opcodes;
	lwfs_cap create_cid_cap;
	lwfs_uid_array uid_array;

	lwfs_bool first_time=TRUE;

	uid_t unix_uid;

	const lwfs_ns_entry *root=NULL;

	char ns_name[1024];

	log_debug(sysio_debug_level, "entered lwfs_connect");

	/* lookup services */
	/* look in fuse-lwfs.c:main */
	log_debug(sysio_debug_level,
		  "looking up namespace (%s)",
		  lwfs_fs->namespace.name);

	strcpy(ns_name, lwfs_fs->namespace.name);
	rc = lwfs_get_namespace_sync(&lwfs_fs->naming_svc,
				     ns_name,
				     &lwfs_fs->namespace);
	if (rc == LWFS_ERR_NOENT) {
		log_debug(sysio_debug_level,
			  "namespace (%s) not found.  that's ok - we'll create it now",
			  ns_name);
		rc = create_namespace(lwfs_fs, ns_name);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "error creating namespace (%s): %s",
					ns_name, lwfs_err_str(rc));
			return rc;
		}
	} else if ((rc != LWFS_OK) && (rc != LWFS_ERR_NOENT)) {
		log_error(sysio_debug_level, "error looking up namespace (%s): %s",
				ns_name, lwfs_err_str(rc));
		return rc;
	}

	root = &lwfs_fs->namespace.ns_entry;

	/* initialize the user credential */
	memset(&lwfs_fs->cred, 0, sizeof(lwfs_cred));
	unix_uid = getuid(); 
	memcpy(&lwfs_fs->cred.data.uid, &unix_uid, sizeof(unix_uid));

	/* set the cid */
	lwfs_fs->cid = root->entry_obj.cid;

	/* get the cap that allows me to create containers */
	rc = lwfs_get_cap_sync(&lwfs_fs->authr_svc, lwfs_fs->cid, LWFS_CONTAINER_CREATE, &lwfs_fs->cred, &create_cid_cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* try to create a container for root with a known id (lwfs_fs->cid) */
	rc = lwfs_create_container_sync(&lwfs_fs->authr_svc, &lwfs_fs->txn, lwfs_fs->cid,
					&create_cid_cap, &lwfs_fs->modacl_cap);
	if (rc == LWFS_ERR_EXIST) {
		/*
		 * no problem, the root container exists, because we have
		 * mounted this filesystem before.  Get a modacl capability.
		 */
		log_debug(sysio_debug_level, "root container already exists, get a modacl cap");
		first_time=FALSE;
		opcodes = LWFS_CONTAINER_MODACL;
		rc = lwfs_get_cap_sync(&lwfs_fs->authr_svc, lwfs_fs->cid, opcodes,
				       &lwfs_fs->cred, &lwfs_fs->modacl_cap);
	}
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to create container or lookup the root container: %s",
			lwfs_err_str(rc));
		return rc;
	}

	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	if (first_time == TRUE) {
		log_debug(sysio_debug_level, "creating acl for root container (%d)", lwfs_fs->cid);
		/* Create an acl that allows me to read and write to the container */
		uid_array.lwfs_uid_array_len = 1;
		uid_array.lwfs_uid_array_val = &lwfs_fs->cred.data.uid;

		rc = lwfs_create_acl_sync(&lwfs_fs->authr_svc, &lwfs_fs->txn, lwfs_fs->cid, opcodes,
					  &uid_array, &lwfs_fs->modacl_cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "unable to create acls: %s",
				  lwfs_err_str(rc));
			return rc;
		}
	} else {
#ifdef GET_ROOT_ACL
		log_debug(sysio_debug_level, "getting acl for root container (%d)", lwfs_fs->cid);
		/* the acl should already exist.  update it (get + mod). */
		rc = lwfs_get_acl_sync(&lwfs_fs->authr_svc, lwfs_fs->cid, opcodes,
				       &lwfs_fs->modacl_cap, &uid_array);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "unable to create acls: %s",
				  lwfs_err_str(rc));
			return rc;
		}
#endif

		/* check if my creds are on the acl */
		log_debug(sysio_debug_level, "checking if acl has my creds for root container (%d)", lwfs_fs->cid);

		/* Update the acl that allows me to read and write the container */
		log_debug(sysio_debug_level, "add my creds to the acl for root container (%d)", lwfs_fs->cid);
		uid_array.lwfs_uid_array_len = 1;
		uid_array.lwfs_uid_array_val = &lwfs_fs->cred.data.uid;
		rc = lwfs_mod_acl_sync(&lwfs_fs->authr_svc, &lwfs_fs->txn, lwfs_fs->cid,
				       opcodes, &uid_array, NULL, &lwfs_fs->modacl_cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "unable to modify acl: %s",
				  lwfs_err_str(rc));
			return rc;
		}
	}

	/* get the cap that allows me to read and write to the container */
	log_debug(sysio_debug_level, "get a cap for root container (%d)", lwfs_fs->cid);
	rc = lwfs_get_cap_sync(&lwfs_fs->authr_svc, lwfs_fs->cid, opcodes, &lwfs_fs->cred, &lwfs_fs->cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	}

	log_debug(sysio_debug_level, "finished lwfs_connect");

	return rc;
}

static int
lwfs_stat_ns_entry(
	struct filesys *fs,
	lwfs_ns_entry *ns_entry,
	time_t t,
	struct intnl_stat *buf)
{
	int rc = LWFS_OK;
	int i;
	lwfs_stat_data attr;
	lwfs_stat_data tmp_attr;
	struct intnl_stat stbuf;
	const lwfs_obj *obj = NULL;
	lwfs_filesystem *lwfs_fs = FS2LFS(fs);

	log_debug(sysio_debug_level, "entered lwfs_stat_ns_entry(parent=%d,name=%s)",
				     (int)ns_entry->parent_oid,
				     ns_entry->name);

	if (ns_entry) {
		if (logging_debug(sysio_debug_level))
			fprint_lwfs_ns_entry(logger_get_file(), "ns_entry_name", "ns_entry_prefix", ns_entry);

		/* get the attributes for the object */
		if (ns_entry->file_obj != NULL) {
			obj = ns_entry->file_obj;
		}
		else {
			log_warn(sysio_debug_level, "cannot stat non-file obj");
			obj = &ns_entry->entry_obj;
		}

		memset(&stbuf, 0, sizeof(struct stat));

		set_mode_by_type(&stbuf, obj->type);
		set_stat_from_inode(&stbuf, fs, ns_entry);

		if (obj->type == LWFS_FILE_OBJ) {
			if (ns_entry->d_obj == NULL) {
				rc = sso_load_mo(lwfs_fs, ns_entry);
				if (rc != LWFS_OK) {
					log_warn(sysio_debug_level, "unable to load management object: %s",
							lwfs_err_str(rc));
					goto cleanup;
				}
			}
#ifdef USE_GETATTR
			rc = lwfs_getattr_sync(&lwfs_fs->txn, obj, &lwfs_fs->cap, &attr);
			if (rc != LWFS_OK) {
				log_warn(sysio_debug_level, "unable to get attributes: %s",
						lwfs_err_str(rc));
				errno = EIO;
				rc = -errno;
				goto cleanup;
			}

			if (logging_debug(sysio_debug_level)) {
				fprint_lwfs_obj_attr(logger_get_file(), "attr", "DEBUG", &attr);
			}
#endif
			rc = lwfs_stat_sync(&lwfs_fs->txn, &ns_entry->d_obj->ss_obj[0], &lwfs_fs->cap, &attr);
			if (rc != LWFS_OK) {
				log_warn(sysio_debug_level, "unable to stat: %s",
						lwfs_err_str(rc));
				errno = EIO;
				rc = -errno;
				goto cleanup;
			}
			for (i=1; i<ns_entry->d_obj->ss_obj_count; i++) {
				rc = lwfs_stat_sync(&lwfs_fs->txn, &ns_entry->d_obj->ss_obj[i], &lwfs_fs->cap, &tmp_attr);
				if (rc != LWFS_OK) {
					log_warn(sysio_debug_level, "unable to stat: %s",
							lwfs_err_str(rc));
					errno = EIO;
					rc = -errno;
					goto cleanup;
				}
				attr.size += tmp_attr.size;
			}

			copy_lwfs_stat(&stbuf, &attr);

			if (logging_debug(sysio_debug_level)) {
				fprint_lwfs_stat_data(logger_get_file(), "attr", "DEBUG", &attr);
			}
		}

		if (logging_debug(sysio_debug_level)) {
			FILE *fp = logger_get_file();
			fprintf(fp, "        mode            0x%08x\n", (int)stbuf.st_mode);
			fprintf(fp, "        device          %d\n", (int)stbuf.st_dev);
			fprintf(fp, "        inode           %d\n", (int)stbuf.st_ino);
			fprintf(fp, "        hard links      %d\n", (int)stbuf.st_nlink);
			fprintf(fp, "        user ID         %d\n", (int)stbuf.st_uid);
			fprintf(fp, "        groupd ID       %d\n", (int)stbuf.st_gid);
			fprintf(fp, "        device type     %d\n", (int)stbuf.st_rdev);
			fprintf(fp, "        size            %d\n", (int)stbuf.st_size);
			fprintf(fp, "        block size      %d\n", (int)stbuf.st_blksize);
			fprintf(fp, "        num blocks      %d\n", (int)stbuf.st_blocks);
			fprintf(fp, "        last access     %d\n", (int)stbuf.st_atime);
			fprintf(fp, "        last mod        %d\n", (int)stbuf.st_mtime);
			fprintf(fp, "        last change     %d\n", (int)stbuf.st_ctime);
		}
	}

	if (buf) {
		*buf = stbuf;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_stat_ns_entry");

	return rc;
}


static int
lwfs_stat_inode(
	struct filesys *fs,
	struct inode *ino,
	time_t t,
	struct intnl_stat *buf)
{
	int rc = LWFS_OK;
	lwfs_inode *lino;
	struct intnl_stat stbuf;

	log_debug(sysio_debug_level, "entered lwfs_stat_inode");

	/* set the lwfs_inode */
	lino = ino ? I2LI(ino) : NULL;

	if (lino) {
		rc = lwfs_stat_ns_entry(fs, &lino->ns_entry, t, &stbuf);
		if (rc != LWFS_OK) {
			log_debug(sysio_debug_level, "error from lwfs_stat_sync");
			goto cleanup;
		}
	}

	if (lino) {
		lino->attrtim = t;
		*buf = stbuf;
		goto cleanup;
	}

	if (buf) {
		*buf = stbuf;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_stat_inode");

	return rc;
}


static int
lwfs_stat_path(
	struct filesys *fs,
	const char *path,
	time_t t,
	struct intnl_stat *buf)
{
	int rc = LWFS_OK;
	lwfs_inode *lino;
	struct intnl_stat stbuf;
	lwfs_ns_entry ns_entry;

	log_debug(sysio_debug_level, "entered lwfs_stat_path");

	if (path) {
		traverse_path(FS2LFS(fs), path, &ns_entry);

		if (logging_debug(sysio_debug_level))
			fprint_lwfs_ns_entry(logger_get_file(), "ns_entry", "traverse_path found ->", &ns_entry);

		rc = lwfs_stat_ns_entry(fs, &ns_entry, t, &stbuf);
		if (rc != LWFS_OK) {
			log_debug(sysio_debug_level, "error from lwfs_stat_sync");
			goto cleanup;
		}
	}

	if (buf) {
		*buf = stbuf;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_stat_path");

	return rc;
}


/*
 * Create private, internal, view of the hosts name space.
 */
static int
create_internal_namespace(const void *data)
{
	char	*opts=NULL;
	ssize_t	len;
	char	*cp;
	lwfs_filesystem *lwfs_fs;
	int	err;
	lwfs_ns_entry *root=NULL;
	struct mount *mnt;
	struct inode *rootino;
	struct pnode_base *rootpb;
	static struct qstr noname = { NULL, 0, 0 };
	struct filesys *fs;
	time_t	t;
	struct intnl_stat stbuf;
	unsigned long ul;
	static struct option_value_info v[] = {
		{ "atimo",	"30" },
		{ NULL,		NULL }
	};
	const char *config_file = (const char *)data;

	log_debug(sysio_debug_level, "entered create_internal_namespace");

	if (lwfs_internal_mount) {
		/*
		 * Reentered!
		 */
		abort();
	}

	/*
	 * We maintain an artificial, internal, name space in order to
	 * have access to fully qualified path names in the various routines.
	 * Initialize that name space now.
	 */
	fs = NULL;
	mnt = NULL;
	rootino = NULL;
	rootpb = NULL;

	/*
	 * This really should be per-mount. Hmm, but that's best done
	 * as proper sub-mounts in the core and not this driver. We reconcile
	 * now, here, by putting the mount options on the file system. That
	 * means they are global and only can be passed at the initial mount.
	 *
	 * Maybe do it right some day?
	 */

	/* ----- INITIALIZE LWFS ----- */

	lwfs_fs = (lwfs_filesystem *)calloc(1, sizeof(lwfs_filesystem));
	if (lwfs_fs == NULL) {
		err = -ENOSPC;
		goto error;
	}

	/* ---------------- */
	ul = 0;
	/* ---------------- */
	lwfs_fs->atimo = ul;
	if ((unsigned long)lwfs_fs->atimo != ul) {
		err = -EINVAL;
		goto error;
	}

	/* data should point to the LWFS config file */
	if (config_file == NULL) {
		return -EINVAL;
	}

	/* initialize LWFS RPC */
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);


	/* parse the configuration file */
	err = parse_config_file(config_file, lwfs_fs);
	if (err != LWFS_OK) {
		log_error(sysio_debug_level, "could not parse file \"%s\": %s",
				data, lwfs_err_str(err));
		return -EINVAL;
	}
	if (logging_debug(sysio_debug_level)) {
		int i;
		fprint_lwfs_service(logger_get_file(), "naming_svc", "naming_svc", &lwfs_fs->naming_svc);
		fprint_lwfs_service(logger_get_file(), "authr_svc", "authr_svc", &lwfs_fs->authr_svc);
		for (i=0; i<lwfs_fs->num_servers; i++) {
			fprint_lwfs_service(logger_get_file(), "storage_svc", "storage_svc", &(lwfs_fs->storage_svc[i]));
		}
		fprint_lwfs_namespace(logger_get_file(), "namespace", "before connect", &lwfs_fs->namespace);
	}

	err = lwfs_connect(lwfs_fs);
	if (err != LWFS_OK) {
		log_error(sysio_debug_level, "could not lookup lwfs services, get creds and/or get caps: %s",
				lwfs_err_str(err));
		return -EINVAL;
	}

	if (logging_debug(sysio_debug_level)) {
		fprint_lwfs_namespace(logger_get_file(), "namespace", "after connect", &lwfs_fs->namespace);
	}

	root = &lwfs_fs->namespace.ns_entry;

	/* ----- END INITIALIZE LWFS ----- */

	/* This registers the ops and puts lwfs_fs into fs->private */
	fs = _sysio_fs_new(&lwfs_inodesys_ops, 0, lwfs_fs);
	if (!fs) {
		err = -ENOMEM;
		goto error;
	}

	/*
	 * Get root i-node.
	 */
	memset(&stbuf, 0, sizeof(struct intnl_stat));
	t = _SYSIO_LOCAL_TIME();
	err = lwfs_stat_ns_entry(fs, root, 0, &stbuf);
	if (err)
		goto error;

	rootino = lwfs_i_new(fs, t + FS2LFS(fs)->atimo, &stbuf, root);
	if (!rootino) {
		err = -ENOMEM;
		goto error;
	}

	/*
	 * Generate base path-node for root.
	 */
	rootpb = _sysio_pb_new(&noname, NULL, rootino);
	if (!rootpb) {
		err = -ENOMEM;
		goto error;
	}

	/*
	 * Mount it. This name space is disconnected from the
	 * rest of the system -- Only available within this driver.
	 */
	err = _sysio_do_mount(fs, rootpb, 0, NULL, &mnt);
	if (err)
		goto error;

	lwfs_internal_mount = mnt;

	log_debug(sysio_debug_level, "finished create_internal_namespace (success)");

	return 0;
error:
	if (mnt) {
		if (_sysio_do_unmount(mnt) != 0)
			abort();
		lwfs_fs = NULL;
		fs = NULL;
		rootpb = NULL;
		rootino = NULL;
	}
	if (rootpb)
		_sysio_pb_gone(rootpb);
	if (fs) {
		FS_RELE(fs);
		lwfs_fs = NULL;
	}
	if (lwfs_fs)
		free(lwfs_fs);
	if (opts)
		free(opts);

	log_debug(sysio_debug_level, "finished create_internal_namespace (error)");

	return err;
}



static int
lwfs_fsswop_mount(const char *source,
		    unsigned flags,
		    const void *data,
		    struct pnode *tocover,
		    struct mount **mntp)
{
	int err = 0;
	struct nameidata nameidata;
	struct mount *mnt;

	log_debug(sysio_debug_level, "entered lwfs_fsswop_mount");

	/*
	 * Caller must use fully qualified path names when specifying
	 * the source.
	 */
	if (*source != '/')
		return -ENOENT;

	if (!lwfs_internal_mount) {
		err = create_internal_namespace(data);
		if (err)
			return err;
		*mntp = lwfs_internal_mount;
	}
	else if (data && *(char *)data)
		return -EINVAL;

	/*
	 * Lookup the source in the internally maintained name space.
	 */
	ND_INIT(&nameidata, 0, source, lwfs_internal_mount->mnt_root, NULL);
	err = _sysio_path_walk(lwfs_internal_mount->mnt_root, &nameidata);
	if (err)
		return err;

	/*
	 * Have path-node specified by the given source argument. Let the
	 * system finish the job, now.
	 */
	err =
	    _sysio_do_mount(lwfs_internal_mount->mnt_fs,
			    nameidata.nd_pno->p_base,
			    flags,
			    tocover,
			    &mnt);
	/*
	 * Release the internal name space pnode and clean up any
	 * aliases we might have generated. We really don't need to cache them
	 * as they are only used at mount time..
	 */
	P_RELE(nameidata.nd_pno);
	(void )_sysio_p_prune(lwfs_internal_mount->mnt_root);

	if (!err) {
		FS_REF(lwfs_internal_mount->mnt_fs);
		*mntp = mnt;
	}

	log_debug(sysio_debug_level, "finished lwfs_fsswop_mount");

	return err;
}


/* ------- PRIVATE FUNCTIONS  --------- */

static int get_suffix(char *name, const char *path, int len)
{
	/* ptr points to the last '/' */
	char *ptr = rindex(path, '/');

	log_debug(sysio_debug_level, "entered get_suffix");

	/* copy name */
	strncpy(name, &ptr[1], len);

	log_debug(sysio_debug_level, "finished get_suffix");

	return LWFS_OK;
}

static int get_prefix(char *prefix, const char *path, int maxlen)
{
	int prefix_len;
	int len;

	log_debug(sysio_debug_level, "entered get_prefix");

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

	log_debug(sysio_debug_level, "finished get_prefix");

	return LWFS_OK;
}

static int
parse_path(struct pnode *pno, lwfs_ns_entry *entry)
{
	int rc=LWFS_OK;
	lwfs_filesystem *lwfs_fs=NULL;
	char *fqpath=NULL;

	log_debug(sysio_debug_level, "entered parse_path");

	fqpath = _sysio_pb_path(pno->p_base, '/');
	if (!fqpath) {
		errno = ENOMEM;
		rc = -ENOMEM;
		goto cleanup;
	}
	lwfs_fs = FS2LFS(PNODE_FS(pno));

	rc = traverse_path(lwfs_fs, fqpath, entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:
	if (fqpath != NULL)
		free(fqpath);

	log_debug(sysio_debug_level, "finished parse_path");

	return rc;
}

static int
parse_parent_path(struct pnode *pno, lwfs_ns_entry *parent_ent, char *name)
{
	int rc=LWFS_OK;
	lwfs_filesystem *lwfs_fs=NULL;
	char *fqpath=NULL;
	char *parent_path=NULL;

	log_debug(sysio_debug_level, "entered parse_parent_path(%s, ...)", name);

	fqpath = _sysio_pb_path(pno->p_base, '/');
	if (!fqpath) {
		errno = ENOMEM;
		rc = -ENOMEM;
		goto cleanup;
	}
	lwfs_fs = FS2LFS(PNODE_FS(pno));

	/* allocate space for the parent path */
	parent_path = (char *)malloc(strlen(fqpath));
	if (parent_path == NULL) {
		errno = ENOMEM;
		rc = -errno;
		goto cleanup;
	}

	/* extract the file name and parent object from the path */
	get_suffix(name, fqpath, LWFS_NAME_LEN);
	get_prefix(parent_path, fqpath, strlen(fqpath));

	/* get the parent */
	rc = traverse_path(lwfs_fs, parent_path, parent_ent);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:
	if (fqpath != NULL)
		free(fqpath);
	if (parent_path != NULL)
		free(parent_path);

	log_debug(sysio_debug_level, "finished parse_parent_path(%s, ...)", name);

	return rc;
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
	lwfs_filesystem *lwfs_fs,
	const char *path,
	lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	char name[LWFS_NAME_LEN];
	char *parent_path = NULL;
	lwfs_ns_entry parent_ent;

	log_debug(sysio_debug_level, "entered traverse_path(path=%s)", path);

	/* error case */
	if (path == NULL) {
		log_error(sysio_debug_level, "invalid path");
		errno = ENOENT;
		rc = -errno;
		goto cleanup;
	}


	/* TODO: look for the entry in a cache */


	/* Are we looking up the root? */
	if (strcmp(path, "/") == 0) {
		memcpy(result, &lwfs_fs->namespace.ns_entry, sizeof(lwfs_ns_entry));
		goto cleanup;
	}


	/* Ok, so we have to do it the hard way */

	/* allocate a buffer for the parent path */
	parent_path = (char *)malloc(strlen(path));
	if (parent_path == NULL) {
		log_error(sysio_debug_level, "could not allocate parent_path");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}

	get_suffix(name, path, LWFS_NAME_LEN);
	get_prefix(parent_path, path, strlen(path));

	/* recursive call to get the parent object */
	rc = traverse_path(lwfs_fs, parent_path, &parent_ent);
	if (rc != LWFS_OK) {
		log_warn(sysio_debug_level, "could not traverse path: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* lookup the entry */
	log_debug(sysio_debug_level, "lookup %s", name);
	rc = lwfs_lookup_sync(&lwfs_fs->naming_svc, &lwfs_fs->txn, &parent_ent, name,
			LWFS_LOCK_NULL, &lwfs_fs->cap, result);
	if (rc != LWFS_OK) {
		errno = ENOENT;
		rc = -ENOENT;
		log_warn(sysio_debug_level, "unable to lookup entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	if (logging_debug(sysio_debug_level))
		fprint_lwfs_ns_entry(logger_get_file(), "traverse_path.parent_ent", "DEBUG", &parent_ent);

cleanup:
	if (parent_path != NULL) {
		log_debug(sysio_debug_level, "freeing parent_path");
		free(parent_path);
	}

	log_debug(sysio_debug_level, "finished traverse_path(path=%s)", path);

	return rc;
}


/**
 * @brief Create a new inode.
 *
 * All information about an LWFS file is in the
 * lwfs_ns_entry ns_entry structure.
 *
 */
static struct inode *
lwfs_i_new(
	struct filesys *fs,
	time_t expiration,
	struct intnl_stat *buf,
	const lwfs_ns_entry *ns_entry)
{
	lwfs_inode *lino=NULL;
	struct inode *ino=NULL;

	log_debug(sysio_debug_level, "entered lwfs_i_new(parent=%d,name=%s)",
				     (int)ns_entry->parent_oid,
				     ns_entry->name);

	lino = calloc(1, sizeof(lwfs_inode));
	if (!lino) {
		ino = NULL;
		goto cleanup;
	}

	/* initialize the lwfs inode */
	memcpy(&lino->ns_entry, ns_entry, sizeof(lwfs_ns_entry));
	lino->fileid.fid_data = &lino->ns_entry;
	lino->fileid.fid_len = sizeof(lino->ns_entry);
	lino->oflags = 0;
	lino->nopens = 0;
	lino->fpos = 0;
	lino->attrtim = expiration;

	/* create the sysio inode */
	ino = _sysio_i_new(fs, &lino->fileid, buf, 0,
			&lwfs_i_ops, lino);
	if (!ino)
		free(lino);

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_i_new");

	return ino;
}

/*
 * Find, and validate, or create i-node by host-relative path. Returned i-node
 * is referenced.
 */
static int
lwfs_i_bind_path(
	struct filesys *fs,
	char *path,
	time_t t,
	struct inode **inop)
{
	struct intnl_stat ostbuf, stbuf;
	int	err = LWFS_OK;
	struct inode *ino = NULL;
	lwfs_ns_entry ns_entry;

	log_debug(sysio_debug_level, "entered lwfs_i_bind_path(path=%s)", path);

	if (*inop)
		ostbuf = (*inop)->i_stbuf;

	err = traverse_path(FS2LFS(fs), path, &ns_entry);
	if (err)
		goto cleanup;

	err = lwfs_stat_path(fs, path, t, &stbuf);
	if (err)
		goto cleanup;

#ifdef VALIDATE_INO
	/*
	 * Validate?
	 */
	if (*inop) {
		if (!lwfs_i_invalid(*inop, &ostbuf))
			goto cleanup;
		/*
		 * Invalidate.
		 */
		_sysio_i_undead(*inop);
		*inop = NULL;
	}
#endif

	if (!(ino = lwfs_i_new(fs, t + FS2LFS(fs)->atimo, &stbuf, &ns_entry))) {
		err = -ENOMEM;
		goto cleanup;
	}

	*inop = ino;

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_i_bind_path");

	return err;
}

/*
 * Find, and validate, or create i-node by host-relative path. Returned i-node
 * is referenced.
 */
static int
lwfs_i_bind_ns_entry(
	struct filesys *fs,
	lwfs_ns_entry *ns_entry,
	time_t t,
	struct inode **inop)
{
	struct intnl_stat ostbuf, stbuf;
	int	err = LWFS_OK;
	struct inode *ino = NULL;

	log_debug(sysio_debug_level, "entered lwfs_i_bind_ns_entry(parent=%d,name=%s)",
				     (int)ns_entry->parent_oid,
				     ns_entry->name);

#ifdef VALIDATE_INO
	if (*inop)
		ostbuf = (*inop)->i_stbuf;
#endif

	err = lwfs_stat_ns_entry(fs, ns_entry, t, &stbuf);
	if (err)
		goto cleanup;

#ifdef VALIDATE_INO
	/*
	 * Validate?
	 */
	if (*inop) {
		if (!lwfs_i_invalid(*inop, &ostbuf))
			goto cleanup;
		/*
		 * Invalidate.
		 */
		_sysio_i_undead(*inop);
		*inop = NULL;
	}
#endif

	if (!(ino = lwfs_i_new(fs, t + FS2LFS(fs)->atimo, &stbuf, ns_entry))) {
		err = -ENOMEM;
		goto cleanup;
	}

	*inop = ino;

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_i_bind_ns_entry");

	return err;
}

/* ------- FILE SYSTEM OPERATIONS --------- */

static int
lwfs_inop_lookup(struct pnode *pno,
		   struct inode **inop,
		   struct intent *intnt __IS_UNUSED,
		   const char *path __IS_UNUSED)
{
	time_t          t;
	char           *fqpath;
	struct filesys *fs;
	int             err;

	log_debug(sysio_debug_level, "entered lwfs_inop_lookup(parent=%d,name=%s)",
				     pno->p_base->pb_ino ? (int)(I2LI(pno->p_base->pb_ino)->ns_entry.parent_oid) : -1,
				     pno->p_base->pb_ino ? I2LI(pno->p_base->pb_ino)->ns_entry.name : "inop.null");

	assert(pno);

	*inop = pno->p_base->pb_ino;
	fs = PNODE_FS(pno);

	t = _SYSIO_LOCAL_TIME();

#ifdef USE_CACHED_ATTRS
	/*
	 * Try to use the cached attributes unless the intent
	 * indicates we are looking up the last component and
	 * caller wants attributes. In that case, force a refresh.
	 */
	if (*inop &&
	    (path || !intnt || (intnt->int_opmask & INT_GETATTR) == 0) &&
	    NATIVE_ATTRS_VALID(I2LI(*inop), t))
		return 0;
#endif

	if (*inop) {
		err = lwfs_i_bind_ns_entry(fs, &(I2LI(*inop)->ns_entry), t + FS2LFS(fs)->atimo, inop);
	} else {
		/*
		 * Don't have an inode yet. Because we translate everything back to
		 * a single name space for the host, we will assume the object the
		 * caller is looking for has no existing alias in our internal
		 * name space. We don't see the same file on different mounts in the
		 * underlying host FS as the same file.
		 *
		 * The file identifier *will* be unique. It's got to have a different
		 * dev.
		 */
		fqpath = _sysio_pb_path(pno->p_base, '/');
		if (!fqpath)
			return -ENOMEM;
		err = lwfs_i_bind_path(fs, fqpath, t + FS2LFS(fs)->atimo, inop);
		free(fqpath);
	}
	if (err)
		*inop = NULL;

	log_debug(sysio_debug_level, "finished lwfs_inop_lookup");

	return err;
}

/**
 * @brief Get the attributes of an object.
 *
 * @ingroup libsysio_lwfs_driver
 *
 * The \b lwfs_inop_getattr method fetches the attributes
 * (defined in \ref lwfs_obj_attr) of pnode/inode.
 *
 * @param pno @input the pnode to stat (if not NULL)
 * @param ino @input the inode to stat (if not NULL)
 * @param stbuf @output the stat buffer
 *
 * @returns the object attributes in the stbuf field.
 *
 * @remark <b>Todd (01/10/2007):</b> This impl stats the storage obj.  It
 * does not use named attributes.  Maybe some day.  Because it stats the
 * storage obj, this does not completely work for non-file objs
 * (direntories), because they do not have a storage obj assocaited with
 * them.
 */
static int
lwfs_inop_getattr(struct pnode *pno,
		    struct inode *ino,
		    struct intnl_stat *stbuf)
{
	int res = 0;
	struct filesys *fs=NULL;;
	lwfs_filesystem *lwfs_fs=NULL;;
	lwfs_ns_entry *ent=NULL;
	lwfs_obj *obj=NULL;;
	time_t	t;
	lwfs_stat_data attr;

	log_debug(sysio_debug_level, "entered lwfs_inop_getattr");

	t = _SYSIO_LOCAL_TIME();

	res = 0;	/* compiler cookie */
	if (pno) {
		/* get the object */
		ent = (lwfs_ns_entry *)pno->p_parent->p_base->pb_ino->i_private;
		fs = PNODE_FS(pno);
		lwfs_fs = FS2LFS(PNODE_FS(pno));
	} else if (ino) {
		ent = &(I2LI(ino)->ns_entry);
		fs = INODE_FS(ino);
		lwfs_fs = FS2LFS(INODE_FS(ino));
	} else {
		/*
		 * Dev inodes don't open in this driver. We won't have
		 * a file descriptor with which to do the deed then. Satisfy
		 * the request from the cached copy of the attributes.
		 */
		(void )memcpy(stat,
			      &ino->i_stbuf,
			      sizeof(struct intnl_stat));
	}

	if (ent) lwfs_stat_ns_entry(fs, ent, t, stbuf);

	log_debug(sysio_debug_level, "finished lwfs_inop_getattr");

	return res;
}


static int
lwfs_inop_setattr(struct pnode *pno,
		    struct inode *ino,
		    unsigned mask,
		    struct intnl_stat *stat)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_setattr");
	log_debug(sysio_debug_level, "finished lwfs_inop_setattr");

	return -ENOSYS;
}

/*
 * Calculate size of a directory entry given length of the entry name.
 */
#define LWFS_D_RECLEN(namlen) \
	(((size_t )&((struct intnl_dirent *)0)->d_name + \
	  (namlen) + 1 + sizeof(void *)) & \
	 ~(sizeof(void *) - 1))

static int
lwfs_dirent_filler(struct intnl_dirent *d_entry,
		   lwfs_ns_entry *ns_entry)
{
	int rc=LWFS_OK;
	lwfs_obj *obj = &ns_entry->entry_obj;

	log_debug(sysio_debug_level, "started lwfs_dirent_filler");

	strcpy(d_entry->d_name, ns_entry->name);
	d_entry->d_reclen = LWFS_D_RECLEN(strlen(d_entry->d_name));
	d_entry->d_ino = (ino_t)obj->oid;
	switch (obj->type) {
		case LWFS_DIR_ENTRY:
			d_entry->d_type = DT_DIR;
			break;
		case LWFS_FILE_ENTRY:
			d_entry->d_type = DT_REG;
			break;
		case LWFS_FILE_OBJ:
			d_entry->d_type = DT_REG;
			break;
		case LWFS_LINK_ENTRY:
			d_entry->d_type = DT_LNK;
			break;
		default:
			d_entry->d_type = DT_UNKNOWN;
			break;
	}

#ifdef _DIRENT_HAVE_D_OFF
	d_entry->d_off = d_entry->d_reclen;
#endif
#ifdef _DIRENT_HAVE_D_NAMLEN
	d_entry->d_namlen = strlen(d_entry->d_name);
#endif

	if (logging_debug(sysio_debug_level)) 
		fprint_lwfs_ns_entry(logger_get_file(), "DEBUG", "ns_entry", ns_entry);

	log_debug(sysio_debug_level, "finished lwfs_dirent_filler");

	return rc;
}

static ssize_t
lwfs_filldirentries(struct inode *ino,
		       _SYSIO_OFF_T *posp,
		       char *buf,
		       size_t nbytes)
{
	int rc = 0;
	int cc = 0;
	int i;
	lwfs_ns_entry *parent = &(I2LI(ino)->ns_entry);
	lwfs_ns_entry_array listing;
	lwfs_filesystem *lwfs_fs = FS2LFS(INODE_FS(ino));
	struct intnl_dirent *d_entry = NULL;
	int entries_found = 0;
	int starting_index = 0;
	int bytes_left = nbytes;

	log_debug(sysio_debug_level, "entered lwfs_filldirentries");

	log_debug(sysio_debug_level, "on enter - *posp == %d", *posp);
	log_debug(sysio_debug_level, "on enter -  posp == 0x%08x", posp);

	/* get the contents of the directory */
	rc = lwfs_list_dir_sync(&lwfs_fs->naming_svc, parent, &lwfs_fs->cap, &listing);
	if (rc != LWFS_OK) {
		log_warn(sysio_debug_level, "error getting listing: %s",
			lwfs_err_str(rc));
		errno = EBADF;
		rc = -errno;
		goto cleanup;
	}

#ifdef ADD_FAKE_DOT_DOTDOT
	/* fake . and .. */
	rc = lwfs_dirent_filler(h, ".", DT_DIR, 0);
	if (rc != 0) {
		log_error(sysio_debug_level, "error calling \"filler\"");
		goto cleanup;
	}

	rc = lwfs_dirent_filler(h, "..", DT_DIR, 0);
	if (rc != 0) {
		log_error(sysio_debug_level, "error calling \"filler\"");
		goto cleanup;
	}
#endif

	d_entry = (struct intnl_dirent *)buf;
	/* skip over entries that have been processed previously */
	starting_index = *posp;
	/* entries found in parent */
	entries_found = listing.lwfs_ns_entry_array_len;
	/* fill intnl_dirents from the ns_entries */
	for (i=starting_index; i<entries_found; i++) {
		int n;
		if ((bytes_left - LWFS_D_RECLEN(strlen(listing.lwfs_ns_entry_array_val[i].name))) < 0) {
			break;
		}
		(*posp)++;
		lwfs_dirent_filler(d_entry, &listing.lwfs_ns_entry_array_val[i]);
		d_entry->d_off = *posp;
		n = d_entry->d_reclen;
		cc += n;
		bytes_left -= n;
		d_entry = (struct intnl_dirent *)((char *)d_entry + n);
	}

	rc = cc;
	log_debug(sysio_debug_level, "on exit - cc == %d", cc);
	log_debug(sysio_debug_level, "on exit - *posp == %d", *posp);

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_filldirentries");

	return rc;
}


static int
lwfs_inop_mkdir(struct pnode *pno, mode_t mode)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	lwfs_ns_entry *parent;
	lwfs_ns_entry dir_ent;
	lwfs_filesystem *lwfs_fs=FS2LFS(PNODE_FS(pno));

	log_debug(sysio_debug_level, "entered lwfs_inop_mkdir");

	COPY_PNODE_NAME(name, pno);
	parent = PNODE_NS_ENTRY(PNODE_PARENT(pno));

	/* create the directory */
	rc = lwfs_create_dir_sync(&lwfs_fs->naming_svc,
				  &lwfs_fs->txn,
				  parent, name,
				  parent->entry_obj.cid,
				  &lwfs_fs->cap,
				  &dir_ent);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error creating dir (%s): %s",
				name, lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_mkdir");

	return rc;
}

static int
lwfs_inop_rmdir(struct pnode *pno)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	lwfs_ns_entry *parent;
	lwfs_ns_entry entry;
	lwfs_filesystem *lwfs_fs=FS2LFS(PNODE_FS(pno));

	log_debug(sysio_debug_level, "entered lwfs_inop_rmdir");

	COPY_PNODE_NAME(name, pno);
	parent = PNODE_NS_ENTRY(PNODE_PARENT(pno));

	/* remove the file from the namespace */
	rc = lwfs_remove_dir_sync(&lwfs_fs->naming_svc,
				  &lwfs_fs->txn,
				  parent,
				  name,
				  &lwfs_fs->cap,
				  &entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error removing directory (%s): %s",
				name, lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_rmdir");

	return rc;
}

static int
lwfs_inop_symlink(struct pnode *pno, const char *data)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_symlink");
	log_debug(sysio_debug_level, "finished lwfs_inop_symlink");

	return -ENOSYS;
}

static int
lwfs_inop_readlink(struct pnode *pno, char *buf, size_t bufsiz)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_readlink");
	log_debug(sysio_debug_level, "finished lwfs_inop_readlink");

	return -ENOSYS;
}

static int
create_file(struct pnode *pno, mode_t mode)
{
	struct inode        *dino;
	struct inode        *ino;
	struct incore_inode *icino;
	lwfs_filesystem     *lwfs_fs=NULL;
	int                  rc = 0;
	char                 name[LWFS_NAME_LEN];
	lwfs_ns_entry       *parent;
	lwfs_ns_entry        entry;
	lwfs_ns_entry        new_entry;
	lwfs_obj             file_obj;
	ino_t                inum;
	struct intnl_stat    stat;

	log_debug(sysio_debug_level, "entered create_file");

	COPY_PNODE_NAME(name, pno);
	parent = PNODE_NS_ENTRY(PNODE_PARENT(pno));

	dino = pno->p_parent->p_base->pb_ino;
	assert(dino);

	lwfs_fs=FS2LFS(PNODE_FS(pno));
	
	rc = sso_init_objs(lwfs_fs, &entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error allocating distributed object for %s", name);
		goto cleanup;
	}
	rc = sso_create_objs(lwfs_fs, parent, &entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error creating distributed object for %s", name);
		goto cleanup;
	}
	rc = lwfs_create_file_sync(&lwfs_fs->naming_svc,
				   &lwfs_fs->txn,
				   parent,
				   name,
				   entry.file_obj,
				   &lwfs_fs->cap,
				   &new_entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error creating ns_entry for %s", name);
/*
 * 		rollback_ss_obj();
 */
		goto cleanup;
	}

	new_entry.d_obj = entry.d_obj;
	
	inum = (ino_t)new_entry.entry_obj.oid;

	/*
	 * Must create a new, regular, file.
	 */
	(void )memset(&stat, 0, sizeof(struct intnl_stat));
	stat.st_dev = PNODE_FS(pno)->fs_dev;
	stat.st_mode = S_IFREG | (mode & 07777);
	stat.st_nlink = 1;
	stat.st_uid = getuid();
	stat.st_gid = getgid();
	stat.st_rdev = 0;
	stat.st_size = 0;
	stat.st_blocks = 0;
	stat.st_ctime = stat.st_mtime = stat.st_atime = 0;
	stat.st_blksize = new_entry.d_obj->chunk_size;
	stat.st_ino = inum;
#ifdef HAVE__ST_INO
	stat.__st_ino = inum;
#endif

	ino = lwfs_i_new(INODE_FS(dino),
			 _SYSIO_LOCAL_TIME() + lwfs_fs->atimo,
			 &stat,
			 &new_entry);
	if (!ino) {
/*
 * 		rollback_naming_obj();
 */
		rc = -ENOMEM;
		goto cleanup;
	}

	pno->p_base->pb_ino = ino;

cleanup:
	log_debug(sysio_debug_level, "finished create_file");

	return rc;
}

static int
lwfs_inop_open(struct pnode *pno, int flags, mode_t mode)
{
	int rc=0;

	log_debug(sysio_debug_level, "entered lwfs_inop_open");

	/*
	 * File is open. Load the DSO if needed.
	 */
	if (pno->p_base->pb_ino) {
		log_debug(sysio_debug_level, "file already open");
		
		if ((PNODE_NS_ENTRY(pno)->entry_obj.type == LWFS_FILE_ENTRY) && 
		    (PNODE_NS_ENTRY(pno)->d_obj == NULL)) {
			/* populate the dso */
			rc = sso_load_mo(FS2LFS(PNODE_FS(pno)),
					 PNODE_NS_ENTRY(pno));
			if (rc != LWFS_OK) {
				log_error(naming_debug_level, "error loading management obj: %s",
					  lwfs_err_str(rc));
				goto cleanup;
			}
		}
		goto cleanup;
	}

	rc = create_file(pno, mode);

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_open");

	return rc;
}

static int
lwfs_inop_close(struct inode *ino)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_close");
	log_debug(sysio_debug_level, "finished lwfs_inop_close");

	return LWFS_OK;
}

static int
lwfs_inop_link(struct pnode *target, struct pnode *link)
{
	int rc = 0;
	char          target_name[LWFS_NAME_LEN];
	lwfs_ns_entry *target_parent;
	char          link_name[LWFS_NAME_LEN];
	lwfs_ns_entry *link_parent;
	lwfs_ns_entry link_entry;
	lwfs_filesystem *lwfs_fs=FS2LFS(PNODE_FS(target));

	log_debug(sysio_debug_level, "entered lwfs_inop_link");

	/* get the name and parent ns_entry of the new link */
	COPY_PNODE_NAME(link_name, link);
	link_parent = PNODE_NS_ENTRY(PNODE_PARENT(link));
	/* get the name and parent ns_entry of the link target */
	COPY_PNODE_NAME(target_name, target);
	target_parent = PNODE_NS_ENTRY(PNODE_PARENT(target));

	/* create a link to the target */
	rc = lwfs_create_link_sync(&lwfs_fs->naming_svc,
				   &lwfs_fs->txn,
				   link_parent, link_name,
				   &lwfs_fs->cap,
				   target_parent, target_name,
				   &lwfs_fs->cap,
				   &link_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "error creating link: %s",
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_link");

	return rc;
}

static int
lwfs_inop_unlink(struct pnode *pno)
{
	int rc = 0;
	char name[LWFS_NAME_LEN];
	lwfs_ns_entry *parent;
	lwfs_ns_entry entry;
	lwfs_filesystem *lwfs_fs=FS2LFS(PNODE_FS(pno));

	log_debug(sysio_debug_level, "entered lwfs_inop_unlink");

	COPY_PNODE_NAME(name, pno);
	parent = PNODE_NS_ENTRY(PNODE_PARENT(pno));

	/* remove the file from the namespace */
	rc = lwfs_unlink_sync(&lwfs_fs->naming_svc, &lwfs_fs->txn, parent, name,
			      &lwfs_fs->cap, &entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error removing file: %s",
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	}

	/* remove the associated object if the link count is zero */
	if ((entry.link_cnt == 0) && (entry.file_obj != NULL)) {
		log_debug(sysio_debug_level, "Removing object from storage server");

		if (entry.entry_obj.type == LWFS_FILE_ENTRY) { 
			/* if this is a file obj, populate the dso and remove it */
			if (entry.d_obj == NULL) {
				/* populate the dso */
				rc = sso_load_mo(lwfs_fs,
						 &entry);
				if (rc != LWFS_OK) {
					log_error(naming_debug_level, "error loading management obj: %s",
						  lwfs_err_str(rc));
					goto cleanup;
				}
			}
			
			rc = sso_remove_dso(lwfs_fs, 
					    entry.d_obj->ss_obj,
					    entry.d_obj->ss_obj_count);
		}

		/* regardless of type, remove the primary obj */
		rc = lwfs_remove_obj_sync(&lwfs_fs->txn, entry.file_obj, &lwfs_fs->cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "error removing object: %s",
					lwfs_err_str(rc));
			rc = -EIO;
			goto cleanup;
		}
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_unlink");

	return rc;
}

static int
lwfs_inop_rename(struct pnode *old, struct pnode *new)
{
	int rc = LWFS_OK;

	log_debug(sysio_debug_level, "entered lwfs_inop_rename");

	rc = lwfs_inop_link(old, new);
	if (rc != LWFS_OK) {
		log_debug(sysio_debug_level, "failed to link: %s", lwfs_err_str(rc));
		goto cleanup;
	}
	rc = lwfs_inop_unlink(old);
	if (rc != LWFS_OK) {
		log_debug(sysio_debug_level, "failed to unlink: %s", lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_rename");

	return rc;
}

/** This is a very naive implementation.  It does everything serially, 
 *  even if there are multiple servers
 * 
 *  Fix Me.
 * 
 */
static ssize_t
dopio(void *buf, size_t count, _SYSIO_OFF_T off, lwfs_io *lio)
{
	int rc = 0;
	lwfs_ns_entry *entry = &(lio->lio_lino->ns_entry);
	lwfs_filesystem *lwfs_fs = lio->lio_fs;
	lwfs_size nbytes;

	log_debug(sysio_debug_level, "entered dopio");

	if (entry->d_obj == NULL) {
		log_error(sysio_debug_level, "distributed obj not initialized");
		errno = EIO;
		return -EIO;
	}
	
	nbytes = sso_io(buf, count, off, lio);

	log_debug(sysio_debug_level, "finished dopio");

	return nbytes;
}

static ssize_t
doiov(const struct iovec *iov,
      int count,
      _SYSIO_OFF_T off,
      ssize_t limit,
      lwfs_io *lio)
{
	ssize_t	cc;

#if !(defined(REDSTORM) || defined(MAX_IOVEC))
#define MAX_IOVEC      INT_MAX
#endif

	log_debug(sysio_debug_level, "entered doiov");

	if (count <= 0)
		return -EINVAL;

	/* not really enumerate */
	/* iterate thru the io vectors calling 'dopio' for each  */
	cc = _sysio_enumerate_iovec(iov,
				    count,
				    off,
				    limit,
				    (ssize_t (*)(void *,
						 size_t,
						 _SYSIO_OFF_T,
						 void *))dopio,
				    lio);
	if (cc < 0) {
		cc = -errno;
	} else {
		lio->lio_lino->fpos += cc;
	}

	log_debug(sysio_debug_level, "finished doiov");

	return cc;

#if !(defined(REDSTORM) || defined(MAX_IOVEC))
#undef MAX_IOVEC
#endif
}

static int
doio(char op, struct ioctx *ioctx)
{
	lwfs_io arguments;
	ssize_t	cc;

	log_debug(sysio_debug_level, "entered doio");

	arguments.lio_op = op;
	arguments.lio_lino = I2LI(ioctx->ioctx_ino);
	arguments.lio_fs = FS2LFS(INODE_FS(ioctx->ioctx_ino));

	cc =
	    /* not really enumerate */
	    /* coalesce extents, then iterate thru the extents calling 'doiov' for each  */
	    _sysio_enumerate_extents(ioctx->ioctx_xtv, ioctx->ioctx_xtvlen,
				     ioctx->ioctx_iov, ioctx->ioctx_iovlen,
				     (ssize_t (*)(const struct iovec *,
						  int,
						  _SYSIO_OFF_T,
						  ssize_t,
						  void *))doiov,
				     &arguments);
	if ((ioctx->ioctx_cc = cc) < 0) {
		ioctx->ioctx_errno = -ioctx->ioctx_cc;
		ioctx->ioctx_cc = -1;
	}

	log_debug(sysio_debug_level, "finished doio");

	return 0;
}

static int
lwfs_inop_read(struct inode *ino __IS_UNUSED, struct ioctx *ioctx)
{
	int rc=0;

	log_debug(sysio_debug_level, "entered lwfs_inop_read");
	rc = doio('r', ioctx);
	log_debug(sysio_debug_level, "finished lwfs_inop_read");

	return rc;
}

static int
lwfs_inop_write(struct inode *ino __IS_UNUSED, struct ioctx *ioctx)
{
	int rc=0;

	log_debug(sysio_debug_level, "entered lwfs_inop_write");
	rc = doio('w', ioctx);
	log_debug(sysio_debug_level, "finished lwfs_inop_write");

	return rc;
}

static _SYSIO_OFF_T
lwfs_inop_pos(struct inode *ino, _SYSIO_OFF_T off)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_pos");
	log_debug(sysio_debug_level, "finished lwfs_inop_pos");

	return off;
}

static int
lwfs_inop_iodone(struct ioctx *ioctxp __IS_UNUSED)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_iodone");
	log_debug(sysio_debug_level, "finished lwfs_inop_iodone");

	return 1;
}

static int
lwfs_inop_fcntl(struct inode *ino,
		  int cmd,
		  va_list ap,
		  int *rtn)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_fcntl");
	log_debug(sysio_debug_level, "finished lwfs_inop_fcntl");

	return -ENOSYS;
}

static int
lwfs_inop_mknod(struct pnode *pno __IS_UNUSED,
		  mode_t mode __IS_UNUSED,
		  dev_t dev __IS_UNUSED)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_statvfs");
	log_debug(sysio_debug_level, "finished lwfs_inop_statvfs");

	return -ENOSYS;
}

#ifdef _HAVE_STATVFS
static int
lwfs_inop_statvfs(struct pnode *pno,
		    struct inode *ino,
		    struct intnl_statvfs *buf)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_statvfs");
	log_debug(sysio_debug_level, "finished lwfs_inop_statvfs");

	return -ENOSYS;
}
#endif

static int
lwfs_inop_sync(struct inode *ino)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_sync");
	log_debug(sysio_debug_level, "finished lwfs_inop_sync");

	return 0;
}

static int
lwfs_inop_datasync(struct inode *ino)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_datasync");
	log_debug(sysio_debug_level, "finished lwfs_inop_datasync");

	return -ENOSYS;
}

static int
lwfs_inop_ioctl(struct inode *ino __IS_UNUSED,
		  unsigned long int request __IS_UNUSED,
		  va_list ap __IS_UNUSED)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_ioctl");
	log_debug(sysio_debug_level, "finished lwfs_inop_ioctl");

	return -ENOTTY;
}

static void
lwfs_inop_gone(struct inode *ino)
{
	log_debug(sysio_debug_level, "entered lwfs_inop_gone");
	log_debug(sysio_debug_level, "finished lwfs_inop_gone");

	return;
}

static void
lwfs_fsop_gone(struct filesys *fs __IS_UNUSED)
{
	log_debug(sysio_debug_level, "entered lwfs_fsop_gone");
	log_debug(sysio_debug_level, "finished lwfs_fsop_gone");

	return;
}