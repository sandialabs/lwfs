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


#if defined(REDSTORM) || defined(SCHUTT_PID_ANY_HACK)
#include <mpi.h>
#endif

/* loca includes */
#include "fs_lwfs.h"

/* included from LWFS */
#include "support/logger/logger.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/storage_client/storage_client_sync.h"
#include "client/naming_client/naming_client_sync.h"
#include "common/config_parser/config_parser.h"
#include "common/rpc_common/rpc_common.h"

/* hashtable support */
#include "support/hashtable/hash_funcs.h"
#include "support/hashtable/hashtable.h"

#include "support/trace/trace.h"

#include "sysio_trace.h"
/* 
 * sysio uses the linux sys/queue.h, which defines a partial 
 * TAILQ api.  the tailq.h here defines the complete api.
 * undef the partial api, so we don't get redefinition errors.
 */
#undef TAILQ_ENTRY
#undef TAILQ_HEAD
#undef TAILQ_INIT
#undef TAILQ_INSERT_AFTER
#undef TAILQ_INSERT_HEAD
#undef TAILQ_INSERT_TAIL
#undef TAILQ_REMOVE
#undef TAILQ_FOREACH
#undef TAILQ_FOREACH_REVERSE
#undef TAILQ_HEAD_INITIALIZER
#undef TAILQ_INSERT_BEFORE
#undef TAILQ_LAST
#undef TAILQ_NEXT
#undef TAILQ_PREV
#include "tailq.h"

struct sysio_counter {
    long close; 
    long datasync;
    long fcntl;
    long getattr;
    long ino_gone;
    long ioctl;
    long iodone;
    long link;
    long lookup;
    long mkdir;
    long mknod;
    long open;
    long pos;
    long read;
    long readlink;
    long rename;
    long rmdir;
    long setattr;
    long statvfs;
    long symlink;
    long sync;
    long unlink;
    long write;
    long filldirentries;
    long mount;
    long fs_gone;  /* unmount */
};

static struct sysio_counter sysio_counter;

static const int max_event_data=256;

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
/* log_level sysio_debug_level = LOG_ALL; */

typedef struct {
	dev_t	dev;					/* device number */
	ino_t	ino;					/* i-number */
} lwfs_identifier;

/* 
 * the combination of dirent_oid, inode_oid and parent_oid should
 * be enough to uniquely id an lwfs file to the sysio framework
 */
typedef struct {
	/** The unique id of this dirent */
	lwfs_oid dirent_oid;

	/** The oid of the inode the dirent names. */
	lwfs_oid inode_oid;

	/** The oid for the parent.
	 */
	lwfs_oid parent_oid;
} lwfs_file_identifier;


typedef struct {
	lwfs_ns_entry ns_entry;         /* Identifies the LWFS entry */
	lwfs_cap cap;                   /* enables operations on the entry */
	struct file_identifier fileid;  /* unique file identifier */
	lwfs_identifier ident;		/*  */
	int oflags;                     /* flags from open */
	unsigned nopens;                /* soft ref count */
	_SYSIO_OFF_T fpos;              /* current position */
	time_t attrtim;
	int use_fake_io;		/* if true, then fake the i/o */
} lwfs_inode;

/* ------ Function Prototypes -------- */

static int check_cap_cache(
	lwfs_service *authr_svc, 
	lwfs_cid cid,
	lwfs_opcode opcode,
	lwfs_cred *cred,
	lwfs_cap *cap);

/* ------ CAPABILITY CACHE -------- */

static int cache_hits = 0; 
static int cache_misses = 0; 
static const int cache_size = 1024; 
static struct hashtable cap_ht; 

struct cap_key {
    lwfs_cid cid;
    lwfs_opcode opcode;
};


/* make the hashtable functions type-safe */
DEFINE_HASHTABLE_INSERT(insert_cap, struct cap_key, lwfs_cap);
DEFINE_HASHTABLE_SEARCH(search_cap, struct cap_key, lwfs_cap);
DEFINE_HASHTABLE_REMOVE(remove_cap, struct cap_key, lwfs_cap);

static unsigned int 
hashfromkey(void *key)
{
    return RSHash(key, sizeof(struct cap_key));
}

static int 
equalkeys(void *k1, void *k2)
{
    struct cap_key *key1 = (struct cap_key *)k1;
    struct cap_key *key2 = (struct cap_key *)k2;

    if ((key1->cid == key2->cid) && (key1->opcode == key2->opcode))
	return TRUE;
    else 
	return FALSE;
}


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

struct request_entry {
	lwfs_request *req;
	TAILQ_ENTRY(request_entry) np; /* next and prev pointer for the list */
};
TAILQ_HEAD(request_list, request_entry);

/*
 * LWFS IO path arguments.
 */
typedef struct {
	char                 lio_op;	/* 'r' or 'w' */
	struct inode        *lio_ino;	/* cache the inode */
	lwfs_filesystem     *lio_fs;
	struct request_list *lio_outstanding_requests; /* AIO requests */
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
	((I2LI((pno)->p_base->pb_ino))->ns_entry)

/*
 * Given inode, return the ns_entry
 */
#define INODE_NS_ENTRY(ino) \
	(I2LI(ino)->ns_entry)

/*
 * Copy a pnode's name into 'n'
 */
#define COPY_PNODE_NAME(n, pno) \
	memset(n, 0, LWFS_NAME_LEN); \
	memcpy(n, pno->p_base->pb_name.name, pno->p_base->pb_name.len);
	
/*
 * Cached attributes usable?
 */
#define LWFS_ATTRS_VALID(lino, t) \
	((lino)->attrtim && (t) < (lino)->attrtim)

/*
 * munge a 128bit uuid into a 64bit inode (xor the msb and lsb)
 */
#define UUID_TO_INODE(u) \
	((ino_t)((*((uint64_t *)&(u[0])))^(*((uint64_t *)&(u[8])))))

/**
  * @brief Check the cid cache for this container.
  *
  */
static int check_cid_cache(
	lwfs_service *authr_svc,
	lwfs_txn *txn,
	lwfs_cred *cred,
	lwfs_cid requested_cid, 
	lwfs_cap *create_cid_cap, 
	lwfs_cid *result)
{
    lwfs_cap *modacl_cap = NULL;
    int rc = LWFS_OK;
    lwfs_opcode opcodes;
    lwfs_uid_array uid_array; 

    /* allocate space for the modacl_cap */
    modacl_cap = (lwfs_cap *)malloc(sizeof(lwfs_cap));


    /* If requesting a specific cid, look for it in the cache */

    if (requested_cid != LWFS_CID_ANY) {
	/*
	   struct cap_key key; 
	   key.cid = requested_cid; 
	   key.opcode = LWFS_CONTAINER_MODACL | LWFS_CONTAINER_REMOVE | LWFS_CONTAINER_READ;

	   modacl_cap = search_cap(&cap_ht, &key); 
	 */

	/* use the opcodes used to creating a container */
	opcodes = LWFS_CONTAINER_MODACL 
	    | LWFS_CONTAINER_REMOVE 
	    | LWFS_CONTAINER_READ;


	/* check the cache for the modacl_cap */
	rc = check_cap_cache(authr_svc, requested_cid, 
		opcodes, cred, modacl_cap);

	switch (rc) {
	    case LWFS_OK:
		/* woo hoo! we have the modacl_cap... we're done */
		*result = requested_cid;
		free(modacl_cap); 
		return rc; 

	    case LWFS_ERR_NOENT:
		/* this just means the container does not exist... more to do */
		break; 

	    default:
		/* error */
		log_error(sysio_debug_level, "error geting "
			"container for file: %s", lwfs_err_str(rc));

		free(modacl_cap); 
		return rc; 
	}

	/* the container exists and we have the modacl_cap */
    }

    /* CID is LWFS_CID_ANY or the container does not exist */
    {

	/* create the container by calling the authr service */
	rc = lwfs_create_container_sync(authr_svc, txn, 
		requested_cid, create_cid_cap, modacl_cap);
	switch (rc) {

	    case LWFS_OK:
		break;

	    /* the cid exists, but it is not in the cache ... 
	     * there must be a race condition! */
	    case LWFS_ERR_EXIST:
		{
		/* use the opcodes used to creating a container */
		opcodes = LWFS_CONTAINER_MODACL 
		    | LWFS_CONTAINER_REMOVE 
		    | LWFS_CONTAINER_READ;


		/* check the cache for the modacl_cap */
		rc = check_cap_cache(authr_svc, requested_cid, 
			opcodes, cred, modacl_cap);
		}
		if (rc != LWFS_OK) {
		    log_error(sysio_debug_level, "Big error! creating new "
			    "container for file: %s", lwfs_err_str(rc));
		    free(modacl_cap);
		    return rc; 
		}
		*result = requested_cid; 
		return rc; 


	    default:
		log_error(sysio_debug_level, "error creating new "
			"container for file: %s", lwfs_err_str(rc));
		free(modacl_cap);
		return rc; 

	}

	/* We have just created a container, now we have to create an ACL */

	log_debug(sysio_debug_level,"created container (%d) for file", modacl_cap->data.cid);

	/* add permissions to read and write to objs in this container */
	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	log_debug(sysio_debug_level, "creating acl for new container (%d)", modacl_cap->data.cid);
	uid_array.lwfs_uid_array_len = 1;
	uid_array.lwfs_uid_array_val = &cred->data.uid;

	rc = lwfs_create_acl_sync(authr_svc, txn, modacl_cap->data.cid, opcodes,
		&uid_array, modacl_cap);
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to create acls: %s",
		    lwfs_err_str(rc));
	    free(modacl_cap); 
	    return rc;
	}


	/* add the modacl_cap to the cap cache */
	{
	    struct cap_key *newkey = (struct cap_key *)malloc(sizeof(struct cap_key));
	    newkey->cid = modacl_cap->data.cid; 
	    newkey->opcode = LWFS_CONTAINER_MODACL | LWFS_CONTAINER_REMOVE | LWFS_CONTAINER_READ;

	    if (!insert_cap(&cap_ht, 
			newkey,
			modacl_cap)) 
	    {
		log_error(sysio_debug_level, 
			"could not insert into cache");
		return LWFS_ERR;
	    }
	}

    }

    *result = modacl_cap->data.cid; 
    return rc; 
}


/**
  * @brief Get a capability from a local cache of capabilities.
  *
  */
static int check_cap_cache(
	lwfs_service *authr_svc, 
	lwfs_cid cid,
	lwfs_opcode opcode,
	lwfs_cred *cred,
	lwfs_cap *cap)
{
    int rc = LWFS_OK; 
    lwfs_cap *cached_cap = NULL;
    struct cap_key key; 

    key.cid = cid; 
    key.opcode = opcode; 

    cached_cap = search_cap(&cap_ht, &key); 

    /* case 1: we found the cap */
    if (cached_cap) {
	memcpy(cap, cached_cap, sizeof(lwfs_cap));
	cache_hits++; 
    }

    /* contact the authr service to get the cap */
    else {
	struct cap_key *newkey; 
	lwfs_cap *newcap; 

	cache_misses++; 
	rc = lwfs_get_cap_sync(authr_svc, cid, opcode, cred, cap); 
	if (rc != LWFS_OK) {
	    log_warn(sysio_debug_level, "unable to get cap with cid=%lu: %s",
		    cid, lwfs_err_str(rc));
	    return rc;
	}

	/* allocate a new cap for the cache */
	newcap = (lwfs_cap *)malloc(sizeof(lwfs_cap));
	newkey = (struct cap_key *)malloc(sizeof(struct cap_key));

	memcpy(newcap, cap, sizeof(lwfs_cap));
	memcpy(newkey, &key, sizeof(struct cap_key));

	if (!insert_cap(&cap_ht, 
		    newkey,
		    newcap)) 
	{
	    log_error(sysio_debug_level, 
		    "could not insert into cache");
	    return LWFS_ERR;
	}
    }


    return rc;
}



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

	return rc;
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

	return rc;
}

static void knuth_shuffle(int *array, int len)
{
    int i; 
    static int init=0; 
    static int rank=0; 


    if (!init) {
#ifdef REDSTORM
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
	/*
	lwfs_remote_pid pid; 
	lwfs_get_pid(&pid); 
	*/

	/* seed based on the nid of the process */
	srand((unsigned int)rank);
    }

    /*
    for(i=len-1; i>=0 ;i--)
    {
	index = rand() % (i+1); 

	tmp = array[i]; 
	array[i] = array[index];
	array[index] = tmp; 
    }
    */

    for(i=0; i<len ;i++)
    {
	array[i] = (rank + i) % len; 
    }

    /*fprintf(logger_get_file(), "%d: knuth_shuffle: index[0]=%d\n",rank, array[0]);*/
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
sso_create_dso(
	lwfs_txn *txn, 
	lwfs_filesystem *lwfs_fs,
	lwfs_cid cid,
	lwfs_cap *cap,
	lwfs_obj *dso,
	int dso_count)
{
	int rc = LWFS_OK;
	int i;
	char ostr[33];

	int tries=0;
	const int max_tries = 3;

	int *indices = (int *)malloc(dso_count*sizeof(int));

	/* initialize the indices */
	for (i=0; i<dso_count; i++) {
	    indices[i]=i;
	}
	
	/* shuffle the indices */
	knuth_shuffle(indices, dso_count);

	/* initialize the indices */
	/*
	for (i=0; i<dso_count; i++) {
	    fprintf(logger_get_file(), "sso_create_dso: indices[%d]=%i\n",i,indices[i]);
	}
	*/
	
	log_debug(sysio_debug_level, "entered sso_create_dso");

	/* create 1 storage obj on each of 'dso_count' storage servers */
	tries=0;
	for (i=0; i<dso_count; i++) {
	    do {
		lwfs_init_obj(&(lwfs_fs->storage_svc[indices[i]]),
			LWFS_FILE_OBJ,
			cid,
			LWFS_OID_ANY,
			&(dso[i]));

		tries++;
		rc = lwfs_create_obj_sync(txn,
			&(dso[i]),
			cap);
		if ((rc != LWFS_OK) && (rc != LWFS_ERR_EXIST)) {
		    log_warn(sysio_debug_level, "could not create *dso[%d] "
			    "oid=0x%s, try #%d: %s", 
			    i, 
			    lwfs_oid_to_string(dso[i].oid, ostr), 
			    tries, 
			    lwfs_err_str(rc));
		    goto cleanup;
		}
		if (rc == LWFS_ERR_EXIST) {
		    log_debug(sysio_debug_level, "could not create *dso[%d] "
			    "oid=0x%s, try #%d of %d: %s", 
			    i, 
			    lwfs_oid_to_string(dso[i].oid, ostr), 
			    tries, 
			    max_tries, 
			    lwfs_err_str(rc));
		}
	    } while ((rc == LWFS_ERR_EXIST) && (tries < max_tries)); 

	    if (rc == LWFS_ERR_EXIST) {
		log_error(sysio_debug_level, "error creating *dso[%d] "
			"oid=0x%s, failed after %d tries: %s", 
			i, 
			lwfs_oid_to_string(dso[i].oid, ostr), 
			tries, 
			lwfs_err_str(rc));
		goto cleanup;
	    }
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_create_dso");
	free(indices);
	
	return rc;
}

/**
 * @brief Initiate and create 'dso_count' lwfs_objs in the 'cid' container 
 *        on the storage servers referenced in 'lwfs_fs'.
 *
 * @param lwfs_fs @input the LWFS filesystem on which to create the objects
 * @param cid @input the container in which to create the objects
 * @param cap @input the capability that allows the operation
 * @param dso @input array to storage object references
 */
static int
sso_create_mo(
	lwfs_txn *txn, 
	lwfs_filesystem *lwfs_fs,
	lwfs_cid cid,
	lwfs_cap *cap,
	lwfs_obj *mo)
{
    int rc = LWFS_OK;
    int tries = 0; 
    const int max_tries = 50; 
    int index; 
    char ostr[33];

    log_debug(sysio_debug_level, "entered sso_create_mo");

    do {
	tries++; 

	/* select a storage server at random */
	index = rand() % lwfs_fs->num_servers; 

	/* allocate a management obj on one of the storage servers */
	lwfs_init_obj(&(lwfs_fs->storage_svc[index]),
		LWFS_FILE_OBJ,
		cid,
		LWFS_OID_ANY,
		mo);
	rc = lwfs_create_obj_sync(txn,
		mo,
		cap);
	if (rc != LWFS_OK) {
	    log_warn(sysio_debug_level, "could not create mo (oid=0x%s) "
		    "attempt %d", lwfs_oid_to_string(mo->oid, ostr), tries);
	    if (rc != LWFS_ERR_EXIST) {
		goto cleanup;
	    }
	}

    } while ((rc==LWFS_ERR_EXIST) && (tries < max_tries));

    if (rc == LWFS_ERR_EXIST) {
	log_error(sysio_debug_level, "failed to create mo (oid=0x%s) "
		"after %d attempts", lwfs_oid_to_string(mo->oid, ostr), tries);
	goto cleanup;
    }

    log_debug(sysio_debug_level, "finished sso_create_mo");

cleanup:

    return rc;
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
	       lwfs_cap *cap,
	       int dso_count)
{
	int rc = LWFS_OK;
	int i;
	
	log_debug(sysio_debug_level, "entered sso_remove_dso");

	/* remove 1 storage obj on each of 'dso_count' storage servers */
	for (i=0; i<dso_count; i++) {
		rc = lwfs_remove_obj_sync(&lwfs_fs->txn,
					  &(dso[i]),
					  cap);
		if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "error removing *dso[%d]", i);
			goto cleanup;
		}
	}
	
cleanup:
	log_debug(sysio_debug_level, "finished sso_remove_dso");
	
	return rc;
}

static int
sso_store_mo(
	lwfs_txn *txn,
	lwfs_filesystem *lwfs_fs,
	lwfs_obj *mo,
	int chunk_size,
	lwfs_cap *cap,
	lwfs_obj *dso,
	int dso_count)
{
	int rc = LWFS_OK;

	int offset=0;
	int csize = chunk_size; 

	log_debug(sysio_debug_level, "entered sso_store_mo");

	/* put header info in the MO */
	rc = lwfs_write_sync(txn, 
			     mo,
			     offset, (void *)&csize, sizeof(int),
			     cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not write chunk_size: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	log_debug(sysio_debug_level, "wrote the header");

	offset += sizeof(int);
	rc = lwfs_write_sync(txn, 
			     mo,
			     offset, (void *)&dso_count, sizeof(int),
			     cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not write the dso_count: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}

	log_debug(sysio_debug_level, "wrote dso_count");

	offset += sizeof(int);
	/* put obj refs to DSOs in the MO */
	rc = lwfs_write_sync(txn, 
			     mo,
			     offset, (void *)dso, (dso_count * sizeof(lwfs_obj)),
			     cap);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "could not write the dso array: %s",
			lwfs_err_str(rc));
		errno = EIO;
		goto cleanup;
	}
	offset += (dso_count * sizeof(lwfs_obj));

cleanup:
	log_debug(sysio_debug_level, "finished sso_store_mo");

	return rc;
}

static int
sso_load_mo(
	lwfs_filesystem *lwfs_fs, 
	lwfs_ns_entry *ns_entry)
{
    int rc = LWFS_OK;

    lwfs_obj *mo = ns_entry->file_obj;
    lwfs_cid cid = mo->cid; 
    lwfs_cap cap; 

    int offset = 0;

    lwfs_size nbytes = 0;

    log_debug(sysio_debug_level, "entered sso_load_mo");

    /* get the capability that allows us to read objects in the container */
    rc = check_cap_cache(&lwfs_fs->authr_svc, cid,
	    LWFS_CONTAINER_READ, &lwfs_fs->cred, &cap); 
    if (rc != LWFS_OK) {
	log_error(sysio_debug_level, "unable to get cap: %s",
		lwfs_err_str(rc));
	return rc;
    }


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
	    &cap,
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
	    &cap,
	    &nbytes);
    if (rc != LWFS_OK) {
	log_error(sysio_debug_level, "could not read the dso_count: %s",
		lwfs_err_str(rc));
	errno = EIO;
	goto cleanup;
    }
    offset += sizeof(int);

    /* allocate space for the dso array */
    ns_entry->d_obj->ss_obj = (lwfs_obj *)
	calloc(ns_entry->d_obj->ss_obj_count, sizeof(lwfs_obj)); 
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
	    &cap,
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

    return rc;
}

static int
sso_create_objs(
	lwfs_txn *txn,
	lwfs_filesystem *lwfs_fs,
	lwfs_ns_entry *parent,
	lwfs_cap *cap,
	lwfs_ns_entry *ns_entry)
{
	int rc = LWFS_OK;
	
	int ss_cnt = ns_entry->d_obj->ss_obj_count;
	int chunk_size = ns_entry->d_obj->chunk_size;
	lwfs_obj *dso = ns_entry->d_obj->ss_obj;
	lwfs_cid cid = cap->data.cid;

	log_debug(sysio_debug_level, "entered sso_create_objs");

	if (logging_debug(sysio_debug_level))
		fprint_lwfs_ns_entry(logger_get_file(), "ns_entry_name", "DEBUG fs_lwfs.c:sso_create_objs", ns_entry);

	/* create 'ss_cnt' storage objs */
	rc = sso_create_dso(txn, lwfs_fs, cid, cap, dso, ss_cnt);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to create dso: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}
	/* create a storage obj to store refs to the DSOs and stripe size info */
	rc = sso_create_mo(txn, lwfs_fs, cid, cap, ns_entry->file_obj);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to create mo: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}
	/* put the DSOs plus stripe size in the MO */
	rc = sso_store_mo(txn, lwfs_fs, ns_entry->file_obj, chunk_size, cap, dso, ss_cnt);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "failed to store the mo: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}

	if (logging_debug(sysio_debug_level))
		fprint_lwfs_ns_entry(logger_get_file(), "ns_entry_name", "DEBUG fs_lwfs.c:sso_create_objs", ns_entry);
		
cleanup:
	log_debug(sysio_debug_level, "finished sso_create_objs");

	return rc;
}

static int
sso_init_objs(
	lwfs_filesystem *lwfs_fs,
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
    /* if desired_chunk_size is non-sero, use it, otherwise use the default */
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
	_SYSIO_OFF_T chunk_number=0;
	_SYSIO_OFF_T stripe_number=0;

	log_debug(sysio_debug_level, "entered sso_calc_obj_index_offset");

	chunk_number = file_offset / ns_entry->d_obj->chunk_size;
	stripe_number = chunk_number / ns_entry->d_obj->ss_obj_count;
	*obj_index  = chunk_number % ns_entry->d_obj->ss_obj_count;
	*obj_offset = (stripe_number * ns_entry->d_obj->chunk_size) + (file_offset % ns_entry->d_obj->chunk_size);

	log_debug(sysio_debug_level, "finished sso_calc_obj_index_offset");
}

#if 0
static int
sso_next_obj_index(lwfs_ns_entry *ns_entry,
		   int obj_index)
{
	return ( (obj_index++) % ns_entry->d_obj->ss_obj_count );
}
#endif

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
sso_doio(
	lwfs_io *lio_session, 
	lwfs_obj *obj, 
	_SYSIO_OFF_T off, 
	void *buf, 
	size_t count, 
	lwfs_size *nbytes)
{
    int rc = LWFS_OK;
    lwfs_cap cap;
    lwfs_cid cid = obj->cid; 
    char ostr[33];

    lwfs_filesystem *lwfs_fs = lio_session->lio_fs;
    /* lwfs_ns_entry *ns_entry = &I2LI(lio_session->lio_ino)->ns_entry; */
    struct request_entry *entry = NULL;
    lwfs_request *req = calloc(1, sizeof(lwfs_request));
    if (req == NULL) {
	goto cleanup;
    }

    log_debug(sysio_debug_level, "entered sso_doio");

    if (lio_session->lio_op == 'r') {

	/* get the cap that allows me to read from objs in a container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, cid,
		LWFS_CONTAINER_READ, &lwfs_fs->cred, &cap); 
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	}
	log_debug(sysio_debug_level, "read from obj with cid=%d with cap for cid=%d", 
		obj->cid, cap.data.cid);
	log_debug(sysio_debug_level, "read(offset==%d, count==%d)", (int)off, (int)count);

	if (I2LI(lio_session->lio_ino)->use_fake_io == FALSE) {
		/* read from the object */
		rc = lwfs_read(&lwfs_fs->txn, obj,
			off, buf, count,
			&cap, nbytes,
			req);
		log_debug(sysio_debug_level, "lwfs_read result: %s", lwfs_err_str(rc));
		if (rc != LWFS_OK) {
		    log_error(sysio_debug_level, "could not issue async read: %s",
			    lwfs_err_str(rc));
		    errno = EIO;
		    rc = -EIO;
		    goto cleanup;
		}
	} else {
		log_debug(sysio_debug_level, "lwfs_read faking read from oid (%s)", 
			lwfs_oid_to_string(obj->oid, ostr));
		free(req);
		req = NULL;
		if (lio_session->lio_ino->i_stbuf.st_size >= (off+count)) {
			*nbytes = count;
		} else {
			*nbytes = lio_session->lio_ino->i_stbuf.st_size - off;
		}
		memset(buf, 0, count);
		/* THK TODO:  */
		I2LI(lio_session->lio_ino)->fpos = (off + count);
	}
    }
    if (lio_session->lio_op == 'w') {

	/* get the cap that allows me to read from objs in a container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, cid,
		LWFS_CONTAINER_WRITE, &lwfs_fs->cred, &cap); 
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	}
	log_debug(sysio_debug_level, "write to obj with cid=%d with cap for cid=%d", 
		obj->cid, cap.data.cid);
	log_debug(sysio_debug_level, "offset==%d; count==%d", (int)off, (int)count);
		
	if (I2LI(lio_session->lio_ino)->use_fake_io == FALSE) {
		/* write to the object */
		rc = lwfs_write(&lwfs_fs->txn, obj,
			off, buf, count,
			&cap,
			req);
		log_debug(sysio_debug_level, "lwfs_write result: %s", lwfs_err_str(rc));
		if (rc != LWFS_OK) {
		    log_error(sysio_debug_level, "could not issue async write: %s",
			    lwfs_err_str(rc));
		    errno = EIO;
		    rc = -EIO;
		    goto cleanup;
		}
	} else {
		log_debug(sysio_debug_level, "lwfs_write faking write to oid (%s)", 
			lwfs_oid_to_string(obj->oid, ostr));
		free(req);
		req = NULL;
		/* THK TODO:  */
		I2LI(lio_session->lio_ino)->fpos = (off + count);
		if ((off + count) > lio_session->lio_ino->i_stbuf.st_size) {
			lio_session->lio_ino->i_stbuf.st_size = (off + count);
		}
	}
    }

    *nbytes = count;

    entry = calloc(1, sizeof(struct request_entry));
    if (entry == NULL) {
    	free(req);
    	rc = -ENOMEM;
    	goto cleanup;
    }
    entry->req = req;
    log_debug(LOG_ALL, "entry==%p, req==%p", entry, req);
    TAILQ_INSERT_TAIL(lio_session->lio_outstanding_requests, entry, np);
//    lio_session->lio_outstanding_request = req;

cleanup:
    log_debug(sysio_debug_level, "finished sso_doio");

    return rc;
}

static size_t
sso_io(void *buf, size_t count, _SYSIO_OFF_T off, lwfs_io *lio_session)
{
	int rc = LWFS_OK;

	lwfs_filesystem *lwfs_fs = lio_session->lio_fs;
	lwfs_ns_entry *ns_entry = &I2LI(lio_session->lio_ino)->ns_entry;
	/* lwfs_inode *lino = I2LI(lio_session->lio_ino); */
	
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
	
	if ((lio_session->lio_op == 'r') &&
	    (I2LI(lio_session->lio_ino)->fpos + count > lio_session->lio_ino->i_stbuf.st_size)) {
		bytes_left = lio_session->lio_ino->i_stbuf.st_size - I2LI(lio_session->lio_ino)->fpos;
	}
	
	sso_calc_obj_index_offset(lwfs_fs, ns_entry, file_offset, &first_obj_index, &first_obj_offset);
	first_obj_bytes_avail = ns_entry->d_obj->chunk_size - (first_obj_offset % ns_entry->d_obj->chunk_size);
	
	log_debug(sysio_debug_level, "first_obj_index == %ld; first_obj_offset == %ld; first_obj_bytes_avail == %ld", 
		  first_obj_index, first_obj_offset, first_obj_bytes_avail);
	
	bytes_this_io = sso_calc_first_bytes_to_io(ns_entry, bytes_left, first_obj_offset, first_obj_bytes_avail);

	log_debug(sysio_debug_level, "bytes_this_io == %d; bytes_left == %d", bytes_this_io, bytes_left);
	log_debug(sysio_debug_level, "op == %c; fpos == %d; st_size == %d",
				     lio_session->lio_op,
				     (int)I2LI(lio_session->lio_ino)->fpos,
				     (int)lio_session->lio_ino->i_stbuf.st_size);
	
	if ((lio_session->lio_op == 'r') &&
	    (I2LI(lio_session->lio_ino)->fpos == lio_session->lio_ino->i_stbuf.st_size)) {
		/* reached EOF */
		log_debug(sysio_debug_level, "reached EOF");
		rc = 0;
		goto cleanup;
	}

	log_debug(sysio_debug_level, "performing I/O on container %d", ns_entry->entry_obj.cid);
	assert(ns_entry->entry_obj.cid == ns_entry->d_obj->ss_obj[first_obj_index].cid);

	rc = sso_doio(lio_session, 
		      &ns_entry->d_obj->ss_obj[first_obj_index], 
		      first_obj_offset, 
		      buf, 
		      bytes_this_io, 
		      &nbytes);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "the I/O failed: %s",
			lwfs_err_str(rc));
	}
//	if (nbytes == 0) {
//		/* reached EOF */
//		log_debug(sysio_debug_level, "reached EOF");
//		rc = 0;
//		goto cleanup;
//	}
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
		rc = sso_doio(lio_session, 
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
 * (re)initialize the logger to use a common file 
 */
static void
init_common_log_file()
{
    int log_level = LOG_ERROR;
    char *logfile = NULL; 
    char *env = NULL;
    FILE *log_ptr = NULL;

    /* extract environment variables for LWFS */
    env = getenv("LWFS_LOG_LEVEL");
    if (env != NULL) {
	log_level = atoi(env);
    }

    env = getenv("LWFS_LOG_FILE");
    if (env != NULL) {
	logfile = env; 
    }
    
    log_ptr = logger_get_file();
    if ((log_ptr != NULL) && (log_ptr != stdout) && (log_ptr != stderr)) {
    	fclose(logger_get_file());
    }

    /* initialize the logger */
    logger_init(log_level, logfile);
    
    log_debug(sysio_debug_level, "initialized common logger file (log_level==%d)", log_level);
}

/*
 * (re)initialize the logger to use one file per process 
 */
static void
init_per_node_log_file()
{
	lwfs_remote_pid myid;  
	FILE *log_ptr = NULL;
	char *env = getenv("LWFS_LOG_FILE");
	if (env != NULL) {
		int log_level = LOG_ERROR;
		int log_file_name_len = strlen(env);
		char *log_file_name = calloc((log_file_name_len+20), sizeof(char));
		
	        lwfs_get_id(&myid); 
		snprintf(log_file_name, log_file_name_len+20, "%s.%d.%d", env, myid.nid, myid.pid);

		env = getenv("LWFS_LOG_LEVEL");
		if (env != NULL) {
			log_level = atoi(env);
		}

		log_ptr = logger_get_file();
		if ((log_ptr != NULL) && (log_ptr != stdout) && (log_ptr != stderr)) {
			fclose(logger_get_file());
		}
		
		logger_init(log_level, log_file_name);

		log_debug(sysio_debug_level, "initialized per node logger file (log_level==%d)", log_level);
		
		free(log_file_name);
	}
}

/*
 * Initialize this driver.
 */
int
_sysio_lwfs_init()
{
    init_common_log_file();

    //rpc_debug_level=LOG_OFF;

	/* initialize the counters */
	memset(&sysio_counter, 0, sizeof(struct sysio_counter));

	trace_reset_count(TRACE_SYSIO_INO_LOOKUP, 0, "init ino lookup");
	trace_reset_count(TRACE_SYSIO_INO_GETATTR, 0, "init ino getattr");
	trace_reset_count(TRACE_SYSIO_INO_SETATTR, 0, "init ino setattr");
	trace_reset_count(TRACE_SYSIO_INO_FILLDIRENTRIES, 0, "init ino filldirentries");
	trace_reset_count(TRACE_SYSIO_INO_MKDIR, 0, "init ino mkdir");
	trace_reset_count(TRACE_SYSIO_INO_RMDIR, 0, "init ino rmdir");
	trace_reset_count(TRACE_SYSIO_INO_SYMLINK, 0, "init ino symlink");
	trace_reset_count(TRACE_SYSIO_INO_READLINK, 0, "init ino readlink");
	trace_reset_count(TRACE_SYSIO_INO_OPEN, 0, "init ino open");
	trace_reset_count(TRACE_SYSIO_INO_CLOSE, 0, "init ino close");
	trace_reset_count(TRACE_SYSIO_INO_LINK, 0, "init ino link");
	trace_reset_count(TRACE_SYSIO_INO_UNLINK, 0, "init ino unlink");
	trace_reset_count(TRACE_SYSIO_INO_RENAME, 0, "init ino rename");
	trace_reset_count(TRACE_SYSIO_INO_READ, 0, "init ino read");
	trace_reset_count(TRACE_SYSIO_INO_WRITE, 0, "init ino write");
	trace_reset_count(TRACE_SYSIO_INO_POS, 0, "init ino pos");
	trace_reset_count(TRACE_SYSIO_INO_IODONE, 0, "init ino iodone");
	trace_reset_count(TRACE_SYSIO_INO_FCNTL, 0, "init ino fcntl");
	trace_reset_count(TRACE_SYSIO_INO_SYNC, 0, "init ino sync");
	trace_reset_count(TRACE_SYSIO_INO_DATASYNC, 0, "init ino datasync");
	trace_reset_count(TRACE_SYSIO_INO_IOCTL, 0, "init ino ioctl");
	trace_reset_count(TRACE_SYSIO_INO_MKNOD, 0, "init ino mknod");
	trace_reset_count(TRACE_SYSIO_INO_STATVFS, 0, "init ino statvfs");
	trace_reset_count(TRACE_SYSIO_INO_GONE, 0, "init ino gone");
	trace_reset_count(TRACE_SYSIO_FS_MOUNT, 0, "init fs mount");
	trace_reset_count(TRACE_SYSIO_FS_GONE, 0, "init fs gone");

    log_debug(sysio_debug_level, "registering LWFS with libsysio");

    return _sysio_fssw_register("lwfs", &lwfs_fssw_ops);
}

static int copy_lwfs_stat(struct intnl_stat *stbuf,
			  lwfs_stat_data *stat_data)
{
    int rc = LWFS_OK;
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

    return rc;
}

static int set_mode_by_type(struct intnl_stat *stbuf,
			    int obj_type)
{
    int rc = LWFS_OK;

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

    return rc;
}

static int set_stat_from_inode(struct intnl_stat *stbuf,
			       const struct filesys *fs,
			       const lwfs_ns_entry *entry)
{
    int rc = LWFS_OK;

    log_debug(sysio_debug_level, "entered set_stat_from_inode");

    stbuf->st_dev = fs->fs_dev;							/* device */
    stbuf->st_ino = UUID_TO_INODE(entry->inode_oid);				/* inode */
    stbuf->st_nlink = entry->link_cnt;     					/* number of hard links */
    memcpy(&stbuf->st_uid, FS2LFS(fs)->cap.data.cred.data.uid, sizeof(uid_t));	/* user ID of owner */
    memcpy(&stbuf->st_gid, FS2LFS(fs)->cap.data.cred.data.uid, sizeof(uid_t));	/* group ID of owner */
    stbuf->st_rdev = 0;								/* device type (if inode device) */
    if (entry->d_obj != NULL) {
	stbuf->st_blksize = entry->d_obj->chunk_size;				/* blocksize for filesystem I/O */
    } else {
	stbuf->st_blksize = FS2LFS(fs)->default_chunk_size;				/* blocksize for filesystem I/O */
    }

    log_debug(sysio_debug_level, "finished set_stat_from_inode");

    return rc;
}

#if 0
static int init_fileobj(lwfs_ns_entry *ns_entry,
			lwfs_service *storage_svc)
{
    int rc = LWFS_OK;
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

    return rc;
}
#endif

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
	int rank;

	lwfs_bool first_time=TRUE;


	uid_t unix_uid;

	const lwfs_ns_entry *root=NULL;

	char ns_name[1024];


	lwfs_remote_pid rpid; 
	lwfs_get_id(&rpid); 

	rank = rpid.pid;

	log_debug(sysio_debug_level, "entered lwfs_connect (nid=%d, pid=%d)", rpid.nid, rpid.pid);

	log_debug(sysio_debug_level, "lwfs_connect, pid=%d",rank);

	/* lookup services */
	/* look in fuse-lwfs.c:main */
	log_debug(sysio_debug_level,
		  "looking up namespace (%s)",
		  lwfs_fs->namespace.name);

	/* try to create the namespace */
	strcpy(ns_name, lwfs_fs->namespace.name);
	rc = create_namespace(lwfs_fs, ns_name);

	switch (rc) {
	    case LWFS_OK:
		break;

	    case LWFS_ERR_EXIST:
		/* look up the namespace */
		rc = lwfs_get_namespace_sync(&lwfs_fs->naming_svc,
			ns_name,
			&lwfs_fs->namespace);
		if (rc != LWFS_OK) {
		    log_error(sysio_debug_level, "namespace (%s) not found: %s",
			    ns_name, lwfs_err_str(rc));
		    return rc; 
		}
		break; 

	    default:
		log_error(sysio_debug_level, "error creating namespace (%s): %s",
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
	rc = check_cap_cache(&lwfs_fs->authr_svc, lwfs_fs->cid,
		LWFS_CONTAINER_CREATE, &lwfs_fs->cred, &create_cid_cap); 
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	}


	/* If using same cid for all files, have node 0 create it */
	if (rank == 0) {

	    char *env = getenv("LWFS_FILE_CID");
	    if (env != NULL) {
		lwfs_cid requested_cid = atoi(env);
		lwfs_cid file_cid; 
		lwfs_cap modacl_cap;

		rc = lwfs_create_container_sync(&lwfs_fs->authr_svc, 
			&lwfs_fs->txn, requested_cid, &create_cid_cap, 
			&modacl_cap);

		/* need a new container and caps for this file */
		rc = check_cid_cache(&lwfs_fs->authr_svc, &lwfs_fs->txn, 
			&lwfs_fs->cred, requested_cid, &create_cid_cap, 
			&file_cid); 
		if (rc != LWFS_OK) {
		    log_error(sysio_debug_level, "unable to get container: %s",
			    lwfs_err_str(rc));
		    return rc;
		}

		assert(file_cid == requested_cid);
	    }
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
		rc = check_cap_cache(&lwfs_fs->authr_svc, lwfs_fs->cid, opcodes,
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
	rc = check_cap_cache(&lwfs_fs->authr_svc, lwfs_fs->cid, opcodes,
		&lwfs_fs->cred, &lwfs_fs->cap); 
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	}

	log_debug(sysio_debug_level, "finished lwfs_connect (nid=%d, pid=%d)", rpid.nid, rpid.pid);

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
    lwfs_cap stat_entry_cap; 
    char ostr[33];

    log_debug(sysio_debug_level, "entered lwfs_stat_ns_entry(parent=%s,name=%s)",
	    lwfs_oid_to_string(ns_entry->parent_oid, ostr),
	    ns_entry->name);

    if (ns_entry) {
	lwfs_cid entry_cid = ns_entry->entry_obj.cid; 

	/* get the capability that allows us to read objects in the container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, entry_cid,
		LWFS_CONTAINER_READ, &lwfs_fs->cred, &stat_entry_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	log_debug(sysio_debug_level, "stat ns_entry with cid=%d", entry_cid); 


	if (logging_debug(sysio_debug_level))
	    fprint_lwfs_ns_entry(logger_get_file(), "ns_entry_name", 
		    "DEBUG fs_lwfs.c:lwfs_stat_ns_entry", ns_entry);

	/* get the attributes for the object */
	if (ns_entry->file_obj != NULL) {
	    obj = ns_entry->file_obj;
	}
	else {
	    log_warn(sysio_debug_level, "cannot stat non-file obj");
	    obj = &ns_entry->entry_obj;
	}

	memset(&stbuf, 0, sizeof(struct intnl_stat));

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
	    rc = lwfs_getattr_sync(&lwfs_fs->txn, obj, &stat_entry_cap, &attr);
	    if (rc != LWFS_OK) {
		log_warn(sysio_debug_level, "unable to get attributes: %s",
			lwfs_err_str(rc));
		errno = EIO;
		rc = -errno;
		goto cleanup;
	    }

	    if (logging_debug(sysio_debug_level)) {
		fprint_lwfs_obj_attr(logger_get_file(), "attr", "DEBUG fs_lwfs.c:lwfs_stat_ns_entry", &attr);
	    }
#endif
	    rc = lwfs_stat_sync(&lwfs_fs->txn, &ns_entry->d_obj->ss_obj[0], &stat_entry_cap, &attr);
	    if (rc != LWFS_OK) {
		log_warn(sysio_debug_level, "unable to stat: %s",
			lwfs_err_str(rc));
		errno = EIO;
		rc = -errno;
		goto cleanup;
	    }
	    for (i=1; i<ns_entry->d_obj->ss_obj_count; i++) {
		rc = lwfs_stat_sync(&lwfs_fs->txn, &ns_entry->d_obj->ss_obj[i], &stat_entry_cap, &tmp_attr);
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
		fprint_lwfs_stat_data(logger_get_file(), "attr", "DEBUG fs_lwfs.c:lwfs_stat_ns_entry", &attr);
	    }
	}

	log_debug(sysio_debug_level, "mode            0x%08x", (int)stbuf.st_mode);
	log_debug(sysio_debug_level, "device          %d", (int)stbuf.st_dev);
	log_debug(sysio_debug_level, "inode           %u", (int)stbuf.st_ino);
	log_debug(sysio_debug_level, "hard links      %d", (int)stbuf.st_nlink);
	log_debug(sysio_debug_level, "user ID         %d", (int)stbuf.st_uid);
	log_debug(sysio_debug_level, "groupd ID       %d", (int)stbuf.st_gid);
	log_debug(sysio_debug_level, "device type     %d", (int)stbuf.st_rdev);
	log_debug(sysio_debug_level, "size            %d", (int)stbuf.st_size);
	log_debug(sysio_debug_level, "block size      %d", (int)stbuf.st_blksize);
	log_debug(sysio_debug_level, "num blocks      %d", (int)stbuf.st_blocks);
	log_debug(sysio_debug_level, "last access     %d", (int)stbuf.st_atime);
	log_debug(sysio_debug_level, "last mod        %d", (int)stbuf.st_mtime);
	log_debug(sysio_debug_level, "last change     %d", (int)stbuf.st_ctime);
    }

    if (buf) {
	*buf = stbuf;
	goto cleanup;
    }

cleanup:
    log_debug(sysio_debug_level, "finished lwfs_stat_ns_entry");

    return rc;
}

#if 0
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
#endif

static int
lwfs_stat_path(
	struct filesys *fs,
	const char *path,
	time_t t,
	struct intnl_stat *buf)
{
	int rc = LWFS_OK;
	/* lwfs_inode *lino; */
	struct intnl_stat stbuf;
	lwfs_ns_entry ns_entry;

	log_debug(sysio_debug_level, "entered lwfs_stat_path");
	
	memset(&ns_entry, 0, sizeof(lwfs_ns_entry));

	if (path) {
		traverse_path(FS2LFS(fs), path, &ns_entry);

		if (logging_debug(sysio_debug_level))
			fprint_lwfs_ns_entry(logger_get_file(), "ns_entry", "DEBUG fs_lwfs.c:lwfs_stat_path - traverse_path found -> ", &ns_entry);

		rc = lwfs_stat_ns_entry(fs, &ns_entry, t, &stbuf);
		if (rc != LWFS_OK) {
			log_warn(sysio_debug_level, "error from lwfs_stat_sync: %s",
				lwfs_err_str(rc));
			goto cleanup;
		}
	}

	if (buf) {
		*buf = stbuf;
		goto cleanup;
	}

cleanup:
	if (ns_entry.d_obj != NULL) {
		if (ns_entry.d_obj->ss_obj != NULL)
			free(ns_entry.d_obj->ss_obj);
		free(ns_entry.d_obj);
	}

	log_debug(sysio_debug_level, "finished lwfs_stat_path");

	return rc;
}


/*
 * Create private, internal, view of the hosts name space.
 *
 * This is a collective call. 
 */
static int
create_internal_namespace(const char *source)
{
    char	*opts=NULL;
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
    int mypid = PTL_PID_ANY;
    /*
       static struct option_value_info v[] = {
       { "atimo",	"30" },
       { NULL,		NULL }
       };
     */

    log_debug(sysio_debug_level, "entered create_internal_namespace");
    log_debug(sysio_debug_level, "config_file = \"%s\"",source);

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

    /* source should point to the LWFS config file */
    if (source == NULL) {
	return -EINVAL;
    }

#ifdef SCHUTT_PID_ANY_HACK
	{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	mypid = 130 + rank;
	} 
#endif

#if defined(PTL_IFACE_CLIENT)
	lwfs_ptl_init(PTL_IFACE_CLIENT, mypid);
#else
	lwfs_ptl_init(PTL_IFACE_DEFAULT, mypid);
#endif

	/* initialize LWFS RPC */
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

	if (getenv("LWFS_LOG_FILE_PER_NODE") != NULL) {
		init_per_node_log_file();
	}

	struct lwfs_config lwfs_cfg; 

	/* initialize the lwfs_cfg */
	memset(&lwfs_cfg, 0, sizeof(struct lwfs_config));

	/* parse the configuration file */
	log_debug(sysio_debug_level, "parsing config file \"%s\"", source);

	err = parse_lwfs_config_file(source, &lwfs_cfg);
	if (err != LWFS_OK) {
	    log_error(sysio_debug_level, "could not parse file \"%s\": %s",
		    source, lwfs_err_str(err));
	    return -EINVAL;
	}

	/*----- initialize the lwfs filesystem (get service descriptions) ---*/

	/* get authr svc */
	log_debug(sysio_debug_level, "getting authr info from (nid=%llu,pid=%llu)", 
		lwfs_cfg.authr_id.nid, (unsigned long long)lwfs_cfg.authr_id.pid);
	err = lwfs_get_service(lwfs_cfg.authr_id, &lwfs_fs->authr_svc);
	if (err != LWFS_OK) {
	    log_error(sysio_debug_level, "could not get authr svc: %s",
		    lwfs_err_str(err));
	    return -EINVAL;
	}

	/* get naming service */
	err = lwfs_get_service(lwfs_cfg.naming_id, &lwfs_fs->naming_svc);
	if (err != LWFS_OK) {
	    log_error(sysio_debug_level, "could not get authr svc: %s",
		    lwfs_err_str(err));
	    return -EINVAL;
	}

	/* get the namespace name */
	strcpy(lwfs_fs->namespace.name, lwfs_cfg.namespace_name);

	/* get the chunksize */
	lwfs_fs->default_chunk_size = lwfs_cfg.ss_chunksize;

	/* get the storage services */
	lwfs_fs->num_servers = lwfs_cfg.ss_num_servers;

	lwfs_fs->storage_svc = (lwfs_service *)
	    malloc(lwfs_fs->num_servers*sizeof(lwfs_service));
	err = lwfs_get_services(lwfs_cfg.ss_server_ids, lwfs_fs->num_servers, 
		lwfs_fs->storage_svc);
	if (err != LWFS_OK) {
	    log_error(sysio_debug_level, "could not get authr svc: %s",
		    lwfs_err_str(err));
	    return -EINVAL;
	}
	
	lwfs_fs->num_fake_io_patterns = lwfs_cfg.ss_num_fake_io_patterns;
	lwfs_fs->fake_io_patterns = lwfs_cfg.ss_fake_io_patterns;


	if (logging_debug(sysio_debug_level)) {
	    int i;
	    fprint_lwfs_service(logger_get_file(), "naming_svc", "DEBUG fs_lwfs.c:create_internal_namespace - naming_svc", &lwfs_fs->naming_svc);
	    fprint_lwfs_service(logger_get_file(), "authr_svc", "DEBUG fs_lwfs.c:create_internal_namespace - authr_svc", &lwfs_fs->authr_svc);
	    for (i=0; i<lwfs_fs->num_servers; i++) {
		fprint_lwfs_service(logger_get_file(), "storage_svc", "DEBUG fs_lwfs.c:create_internal_namespace - storage_svc", &(lwfs_fs->storage_svc[i]));
	    }
	    for (i=0; i<lwfs_fs->num_fake_io_patterns; i++) {
		log_debug(sysio_debug_level, "fake_io_patterns[%d]=%s", i, lwfs_fs->fake_io_patterns[i]);
	    }
	    fprint_lwfs_namespace(logger_get_file(), "namespace", "DEBUG fs_lwfs.c:create_internal_namespace - before connect", &lwfs_fs->namespace);
	}

	err = lwfs_connect(lwfs_fs);
	if (err != LWFS_OK) {
	    log_error(sysio_debug_level, "could not lookup lwfs services, get creds and/or get caps: %s",
		    lwfs_err_str(err));
	    return -EINVAL;
	}

	if (logging_debug(sysio_debug_level)) {
	    fprint_lwfs_namespace(logger_get_file(), "namespace", "DEBUG fs_lwfs.c:create_internal_namespace - after connect", &lwfs_fs->namespace);
	}

	root = calloc(1, sizeof(lwfs_ns_entry));
	if (!root) {
	    err = -ENOMEM;
	    goto error;
	}
	memcpy(root, &lwfs_fs->namespace.ns_entry, sizeof(lwfs_ns_entry));

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
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.filldirentries++;
	interval_id = sysio_counter.filldirentries; 
	snprintf(event_data, max_event_data, "fs_mount"); 
	trace_event(TRACE_SYSIO_FS_MOUNT, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_fsswop_mount, "
		"source=\"%s\", flags=%d, data=\"%s\"",
		source, flags, (const char *)data);

	/* allocate the capability cache */
	err = create_hashtable(cache_size, hashfromkey, equalkeys, &cap_ht);
	if (err != 1) {
	    log_error(sysio_debug_level, "unable to create capability cache");
	    return LWFS_ERR_NOSPACE;
	}


	/*
	 * Caller must use fully qualified path names when specifying
	 * the source.
	 */
	if (*source != '/')
		return -ENOENT;

	if (!lwfs_internal_mount) {
		err = create_internal_namespace((const char *)data);
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

	/*fprintf(stderr, "sysio mount complete\n");*/
	log_debug(sysio_debug_level, "finished lwfs_fsswop_mount");

	snprintf(event_data, max_event_data, "fs_mount"); 
	trace_end_interval(interval_id, TRACE_SYSIO_FS_MOUNT, 
		0, event_data);

	return err;
}

#ifdef VALIDATE_INO
static int
lwfs_i_invalid(struct inode *inop, struct intnl_stat *stat)
{
	lwfs_inode *lino;

	log_debug(sysio_debug_level, "entered lwfs_i_invalid");

	/*
	 * Validate passed in inode against stat struct info
	 */
	lino = I2LI(inop);
	
	if (!lino->attrtim ||
	    (lino->ident.dev != stat->st_dev ||
	     lino->ident.ino != stat->st_ino ||
	     ((inop)->i_stbuf.st_mode & S_IFMT) != (stat->st_mode & S_IFMT)) ||
	    (((inop)->i_stbuf.st_rdev != stat->st_rdev) &&
	       (S_ISCHR((inop)->i_stbuf.st_mode) ||
	        S_ISBLK((inop)->i_stbuf.st_mode))))
	{
		lino->attrtim = 0;			/* invalidate attrs */
		goto invalid;
	}

	log_debug(sysio_debug_level, "finished lwfs_i_invalid (valid)");

	return 0;

invalid:
	log_debug(sysio_debug_level, "finished lwfs_i_invalid (invalid)");

	return 1;
}
#endif

/* ------- PRIVATE FUNCTIONS  --------- */

static int get_suffix(char *name, const char *path, int len)
{
    int rc = LWFS_OK;
    /* ptr points to the last '/' */
    char *ptr = rindex(path, '/');

    log_debug(sysio_debug_level, "entered get_suffix");

    /* copy name */
    strncpy(name, &ptr[1], len);

    log_debug(sysio_debug_level, "finished get_suffix");

    return rc;
}

static int get_prefix(char *prefix, const char *path, int maxlen)
{
    int rc = LWFS_OK;
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

    return rc;
}

#if 0
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
#endif

#if 0
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
#endif

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
	lwfs_cid parent_cid; 
	lwfs_cap parent_cap;

	log_debug(sysio_debug_level, "entered traverse_path(path=%s)", path);
	
	memset(&parent_ent, 0, sizeof(lwfs_ns_entry));
	
	if (logging_debug(sysio_debug_level)) {
		fprint_lwfs_ns_entry(logger_get_file(), "result", "DEBUG fs_lwfs.c:traverse_path", result);
	}

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

	parent_cid = parent_ent.entry_obj.cid; 

	/* get the capability that allows us to read objects in the parent container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, parent_cid,
		LWFS_CONTAINER_READ, &lwfs_fs->cred, &parent_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	/* lookup the entry */
	log_debug(sysio_debug_level, "lookup %s", name);
	rc = lwfs_lookup_sync(&lwfs_fs->naming_svc, &lwfs_fs->txn, &parent_ent, name,
			LWFS_LOCK_NULL, &parent_cap, result);
	if (rc != LWFS_OK) {
		errno = ENOENT;
		rc = -ENOENT;
		log_warn(sysio_debug_level, "unable to lookup entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	if (logging_debug(sysio_debug_level))
		fprint_lwfs_ns_entry(logger_get_file(), "traverse_path.parent_ent", "DEBUG fs_lwfs.c:traverse_path", &parent_ent);

cleanup:
	if (parent_path != NULL) {
		log_debug(sysio_debug_level, "freeing parent_path");
		free(parent_path);
	}

	if (logging_debug(sysio_debug_level)) {
		fprint_lwfs_ns_entry(logger_get_file(), "result", "DEBUG fs_lwfs.c:traverse_path cleanup", result);
	}

	log_debug(sysio_debug_level, "finished traverse_path(path=%s)", path);

	return rc;
}


static void
lwfs_ns_entry_deep_copy(lwfs_ns_entry *dst, const lwfs_ns_entry *src)
{
	memset(dst, 0, sizeof(lwfs_ns_entry));
	memcpy(dst, src, sizeof(lwfs_ns_entry));
	if (src->file_obj != NULL) {
		dst->file_obj = calloc(1, sizeof(lwfs_obj));
		memcpy(dst->file_obj, src->file_obj, sizeof(lwfs_obj));
	}
	if (src->d_obj != NULL) {
		dst->d_obj = calloc(1, sizeof(lwfs_distributed_obj));
		memcpy(dst->d_obj, src->d_obj, sizeof(lwfs_distributed_obj));
		if (src->d_obj->ss_obj != NULL) {
			dst->d_obj->ss_obj = calloc(dst->d_obj->ss_obj_count, sizeof(lwfs_obj));
			memcpy(dst->d_obj->ss_obj, src->d_obj->ss_obj, dst->d_obj->ss_obj_count * sizeof(lwfs_obj));
		}
	}	
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
	lwfs_file_identifier *lfid=NULL;
	char ostr[33];

	log_debug(sysio_debug_level, "entered lwfs_i_new(parent=%s,name=%s,ns_entry=%p)",
				     lwfs_oid_to_string(ns_entry->parent_oid, ostr),
				     ns_entry->name,
				     ns_entry);

	lino = calloc(1, sizeof(lwfs_inode));
	if (!lino) {
		ino = NULL;
		goto cleanup;
	}
	
	lfid = calloc(1,sizeof(lwfs_file_identifier));
	memcpy(&lfid->dirent_oid, &ns_entry->dirent_oid, sizeof(lwfs_oid));
	memcpy(&lfid->inode_oid, &ns_entry->inode_oid, sizeof(lwfs_oid));
	memcpy(&lfid->parent_oid, &ns_entry->parent_oid, sizeof(lwfs_oid));
	
	/* initialize the lwfs inode */
	lwfs_ns_entry_deep_copy(&lino->ns_entry, ns_entry);
	lino->ident.dev = fs->fs_dev;
	lino->ident.ino = UUID_TO_INODE(ns_entry->inode_oid);
	lino->fileid.fid_data = lfid;
	lino->fileid.fid_len = sizeof(lwfs_file_identifier);
	lino->oflags = 0;
	lino->nopens = 0;
	lino->fpos = 0;
	lino->attrtim = expiration;
	lino->use_fake_io = FALSE;

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

	memset(&ns_entry, 0, sizeof(lwfs_ns_entry));
	
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
	struct intnl_stat stbuf;
	int	err = LWFS_OK;
	struct inode *ino = NULL;
	char ostr[33];

	log_debug(sysio_debug_level, "entered lwfs_i_bind_ns_entry(parent=%s,name=%s)",
				     lwfs_oid_to_string(ns_entry->parent_oid, ostr),
				     ns_entry->name);

#ifdef VALIDATE_INO
	struct intnl_stat ostbuf;
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

static int
use_fake_io(
	lwfs_filesystem *lwfs_fs,
	char *filename)
{
	int    num_patterns = lwfs_fs->num_fake_io_patterns;
	char **patterns     = lwfs_fs->fake_io_patterns;
	int i;
	
	for (i=0; i<num_patterns;i++) {
		log_debug(sysio_debug_level, "comparing %s to %s", filename, patterns[i]);
		char *location = strstr(filename, patterns[i]);
		if (location == filename) {
			log_debug(sysio_debug_level, "%s matches %s", filename, patterns[i]);
			return(TRUE);
		}
		log_debug(sysio_debug_level, "%s does not match %s", filename, patterns[i]);
	}
	
	return(FALSE);
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
	int             err=0;
	char            ostr[33];
	char           *parent_ostr=NULL;
	char            filename[LWFS_NAME_LEN];
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.lookup++;
	interval_id = sysio_counter.lookup; 
	snprintf(event_data, max_event_data, "lookup"); 
	trace_event(TRACE_SYSIO_INO_LOOKUP, 0, event_data);
	trace_start_interval(interval_id, 0); 


	parent_ostr = pno->p_base->pb_ino ? lwfs_oid_to_string((I2LI(pno->p_base->pb_ino)->ns_entry.parent_oid), ostr) : "-1";
	strncpy(filename, pno->p_base->pb_ino ? I2LI(pno->p_base->pb_ino)->ns_entry.name : "inop.null", LWFS_NAME_LEN);
	log_debug(sysio_debug_level, "entered lwfs_inop_lookup(parent=%s,name=%s)",
				     parent_ostr,
				     filename);

	assert(pno);

	*inop = pno->p_base->pb_ino;
	fs = PNODE_FS(pno);

	t = _SYSIO_LOCAL_TIME();

	/*
	 * Try to use the cached attributes unless the intent
	 * indicates we are looking up the last component and
	 * caller wants attributes. In that case, force a refresh.
	 */
	if (*inop &&
	    (path || !intnt || (intnt->int_opmask & INT_GETATTR) == 0) &&
	    LWFS_ATTRS_VALID(I2LI(*inop), t)) {
	    	err = 0;
	    	goto cleanup;
	}

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
		if (!fqpath) {
			err = -ENOMEM;
			goto cleanup;
		}
		log_debug(sysio_debug_level, "before bind_path - *inop==%p", *inop);
		err = lwfs_i_bind_path(fs, fqpath, t + FS2LFS(fs)->atimo, inop);
		log_debug(sysio_debug_level, "after bind_path  - *inop==%p", *inop);
		get_suffix(filename, fqpath, LWFS_NAME_LEN);
		free(fqpath);
	}
	if (*inop != NULL) {
		if (use_fake_io(FS2LFS(fs), filename) == TRUE) {
			log_debug(LOG_ALL, "faking i/o for %s", filename);
			I2LI(*inop)->use_fake_io = TRUE;
		}
	}
	if (err)
		*inop = NULL;

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_lookup");

	snprintf(event_data, max_event_data, "lookup"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_LOOKUP, 
		0, event_data);

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
	struct filesys  *fs=NULL;;
	lwfs_filesystem *lwfs_fs=NULL;
	lwfs_ns_entry   *ent=NULL;
	/* lwfs_obj *obj=NULL; */
	time_t t;
	/* lwfs_stat_data attr; */
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.getattr++;
	interval_id = sysio_counter.getattr; 
	snprintf(event_data, max_event_data, "getattr"); 
	trace_event(TRACE_SYSIO_INO_GETATTR, 0, event_data);
	trace_start_interval(interval_id, 0); 

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

	snprintf(event_data, max_event_data, "getattr"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_GETATTR, 
		0, event_data);

	return res;
}


static int
lwfs_inop_setattr(struct pnode *pno,
		    struct inode *ino,
		    unsigned mask,
		    struct intnl_stat *stat)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.setattr++;
	interval_id = sysio_counter.setattr; 
	snprintf(event_data, max_event_data, "setattr"); 
	trace_event(TRACE_SYSIO_INO_SETATTR, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_setattr");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "setattr is not implemented");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "finished lwfs_inop_setattr");

	snprintf(event_data, max_event_data, "setattr"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_SETATTR, 
		0, event_data);

	return 0;
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
	d_entry->d_ino = UUID_TO_INODE(ns_entry->inode_oid);
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
		fprint_lwfs_ns_entry(logger_get_file(), "DEBUG", "DEBUG fs_lwfs.c:lwfs_dirent_filler", ns_entry);

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
	lwfs_cid parent_cid; 
	lwfs_cap parent_cap; 
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.filldirentries++;
	interval_id = sysio_counter.filldirentries; 
	snprintf(event_data, max_event_data, "filldirentries"); 
	trace_event(TRACE_SYSIO_INO_FILLDIRENTRIES, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_filldirentries");

	log_debug(sysio_debug_level, "on enter - *posp == %d", *posp);
	log_debug(sysio_debug_level, "on enter -  posp == 0x%08x", posp);

	memset(&listing, 0, sizeof(lwfs_ns_entry_array));
	memset(&parent_cap, 0, sizeof(lwfs_cap));

	parent_cid = parent->entry_obj.cid; 

	/* get the capability that allows us to read objects from the parent container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, parent_cid,
		LWFS_CONTAINER_READ, &lwfs_fs->cred, &parent_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	if (logging_debug(sysio_debug_level)) 
		fprint_lwfs_ns_entry(logger_get_file(), "parent", "DEBUG", parent);

	/* get the contents of the directory */
	rc = lwfs_list_dir_sync(&lwfs_fs->naming_svc, parent, &parent_cap, &listing);
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

	snprintf(event_data, max_event_data, "filldirentries"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_FILLDIRENTRIES, 
		0, event_data);

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
	lwfs_cid parent_cid; 
	lwfs_cap parent_cap; 
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.mkdir++;
	interval_id = sysio_counter.mkdir; 
	snprintf(event_data, max_event_data, "mkdir"); 
	trace_event(TRACE_SYSIO_INO_MKDIR, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_mkdir");
	
	memset(&dir_ent, 0, sizeof(lwfs_ns_entry));

	COPY_PNODE_NAME(name, pno);
	parent = &PNODE_NS_ENTRY(PNODE_PARENT(pno));

	parent_cid = parent->entry_obj.cid; 

	/* get the capability that allows us to read objects from the parent container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, parent_cid,
		LWFS_CONTAINER_WRITE, &lwfs_fs->cred, &parent_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	/* create the directory */
	rc = lwfs_create_dir_sync(&lwfs_fs->naming_svc,
				  &lwfs_fs->txn,
				  parent, name,
				  parent->entry_obj.cid,
				  &parent_cap,
				  &dir_ent);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error creating dir (%s): %s",
				name, lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_mkdir");

	snprintf(event_data, max_event_data, "mkdir"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_MKDIR, 
		0, event_data);

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
	lwfs_cid parent_cid; 
	lwfs_cap parent_cap; 
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.rmdir++;
	interval_id = sysio_counter.rmdir; 
	snprintf(event_data, max_event_data, "rmdir"); 
	trace_event(TRACE_SYSIO_INO_RMDIR, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_rmdir");
	
	memset(&entry, 0, sizeof(lwfs_ns_entry));

	COPY_PNODE_NAME(name, pno);
	parent = &PNODE_NS_ENTRY(PNODE_PARENT(pno));


	parent_cid = parent->entry_obj.cid; 

	/* get the capability that allows us to read objects from the parent container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, parent_cid,
		LWFS_CONTAINER_WRITE, &lwfs_fs->cred, &parent_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	/* remove the file from the namespace */
	rc = lwfs_remove_dir_sync(&lwfs_fs->naming_svc,
				  &lwfs_fs->txn,
				  parent,
				  name,
				  &parent_cap,
				  &entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error removing directory (%s): %s",
				name, lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_rmdir");

	snprintf(event_data, max_event_data, "rmdir"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_RMDIR, 
		0, event_data);

	return rc;
}

static int
lwfs_inop_symlink(struct pnode *pno, const char *data)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.symlink++;
	interval_id = sysio_counter.symlink; 
	snprintf(event_data, max_event_data, "symlink"); 
	trace_event(TRACE_SYSIO_INO_SYMLINK, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_symlink");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "symlink is not implemented");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "finished lwfs_inop_symlink");

	snprintf(event_data, max_event_data, "symlink"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_SYMLINK, 
		0, event_data);

	return -ENOSYS;
}

static int
lwfs_inop_readlink(struct pnode *pno, char *buf, size_t bufsiz)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.readlink++;
	interval_id = sysio_counter.readlink; 
	snprintf(event_data, max_event_data, "readlink"); 
	trace_event(TRACE_SYSIO_INO_READLINK, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_readlink");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "readlink is not implemented");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "finished lwfs_inop_readlink");

	snprintf(event_data, max_event_data, "readlink"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_READLINK, 
		0, event_data);

	return -ENOSYS;
}



/**
  * @brief Create a new file.
  *
  * This function implements the code necessary to create a new file
  * for the libsysio driver.  This implementation creates one object
  * per available storage server and stripes the data across the storage
  * servers based on the chunk_size parameter.  For access control, we 
  * create a new container and allocate objects for the parallel file using
  * the same container. 
  */
static int
create_file(struct pnode *pno, mode_t mode)
{
    struct inode        *dino;
    struct inode        *ino;
    lwfs_filesystem     *lwfs_fs=NULL;
    int                  rc = 0;
    char                 name[LWFS_NAME_LEN];
    lwfs_ns_entry       *parent;
    lwfs_ns_entry        entry;
    lwfs_ns_entry        new_entry;
    ino_t                inum;
    struct intnl_stat    stat;
    static lwfs_cid CID_UNUSED = LWFS_CID_ANY; 
    lwfs_txn *create_txn = NULL;  /* transaction for this operation */
    lwfs_cid file_cid; 
    lwfs_cap create_file_cap; 
    lwfs_cap create_objs_cap; 
    int miss_count; 

    log_debug(sysio_debug_level, "entered create_file");

    memset(&entry, 0, sizeof(lwfs_ns_entry));
    memset(&new_entry, 0, sizeof(lwfs_ns_entry));

    COPY_PNODE_NAME(name, pno);
    parent = &PNODE_NS_ENTRY(PNODE_PARENT(pno));

    dino = pno->p_parent->p_base->pb_ino;
    assert(dino);

    lwfs_fs=FS2LFS(PNODE_FS(pno));

    /* TODO: create a new transaction for this operation */
    create_txn = &lwfs_fs->txn;

    /* Setup the permissions for this file. 
     * 1) get cap to create container
     * 2) create container
     * 3) add rw access to container
     * 4) get cap that allows rw access
     */
    {
	lwfs_opcode opcodes; 
	static lwfs_cap create_cid_cap; 
	static lwfs_bool initialized = FALSE; 
	lwfs_cid requested_cid;
	char *env = NULL;

	if (!initialized) {
	    /* Get the cap that allows me to create containers (do this only once).
	     * Since anybody is allowed to create containers, the CID argument
	     * for the check_cap_cache call is unimportant. 
	     */
	    rc = check_cap_cache(&lwfs_fs->authr_svc, CID_UNUSED,
		    LWFS_CONTAINER_CREATE, &lwfs_fs->cred, &create_cid_cap); 
	    if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	    }

	    initialized = TRUE; 
	}


	env = getenv("LWFS_FILE_CID");
	if (env != NULL) {
	    requested_cid = atoi(env);
	}
	else {
	    requested_cid = LWFS_CID_ANY;
	}

	/* need a new container and caps for this file */
	rc = check_cid_cache(&lwfs_fs->authr_svc, create_txn, &lwfs_fs->cred,
		requested_cid, &create_cid_cap, &file_cid); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get container: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	/* we want permission ot read and write to objs in this container */
	opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;

	/* Get the cap that allows read/write access */
	rc = check_cap_cache(&lwfs_fs->authr_svc, file_cid,
		opcodes, &lwfs_fs->cred, &create_objs_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	/* At this point, we have a capability that allows create,read, 
	 * write of new objects.  Now we need a capability that allows
	 * us to create a namespace entry in the parent directory. 
	 */
	rc = check_cap_cache(&lwfs_fs->authr_svc, parent->entry_obj.cid,
		opcodes, &lwfs_fs->cred, &create_file_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}
    }

    rc = sso_init_objs(lwfs_fs, &entry);
    if (rc != LWFS_OK) {
	log_error(sysio_debug_level, "error allocating distributed object for %s", name);
	goto cleanup;
    }
    rc = sso_create_objs(create_txn, lwfs_fs, parent, &create_objs_cap, &entry);
    if (rc != LWFS_OK) {
	log_error(sysio_debug_level, "error creating distributed object for %s", name);
	goto cleanup;
    }

    miss_count = 0; 
    do {
	rc = lwfs_create_file_sync(&lwfs_fs->naming_svc,
		create_txn,
		parent,
		name,
		entry.file_obj,
		&create_file_cap,
		&new_entry);
	if ((rc != LWFS_OK) && (rc != LWFS_ERR_EXIST)) {
	    log_error(sysio_debug_level, "error creating ns_entry for %s: %s", name, lwfs_err_str(rc));
	    /*
	     * 		rollback_ss_obj();
	     */
	    goto cleanup;
	}

	if (rc == LWFS_ERR_EXIST) {
	    miss_count++; 
	    log_warn(sysio_debug_level, 
		    "ns_entry for %s exists (attempt %d)", 

		    name, miss_count);
	}
    } while ((rc == LWFS_ERR_EXIST) && miss_count < 3);

    if (rc == LWFS_ERR_EXIST) {
	log_error(sysio_debug_level, 
		"obj for ns_entry for %s exists (failed after %d attempt)", 
		name, miss_count);
	goto cleanup;
    }


    new_entry.d_obj = entry.d_obj;

    inum = UUID_TO_INODE(new_entry.inode_oid);

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
    if (ino != NULL) {
	if (use_fake_io(lwfs_fs, name) == TRUE) {
	    log_debug(sysio_debug_level, "faking i/o for %s", name);
		I2LI(ino)->use_fake_io = TRUE;
	}
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
    struct inode *ino=NULL;
    static int open_recursive_depth=0;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.open++;
	interval_id = sysio_counter.open; 
	snprintf(event_data, max_event_data, "open"); 
	trace_event(TRACE_SYSIO_INO_OPEN, 0, event_data);
	trace_start_interval(interval_id, 0); 

    log_debug(sysio_debug_level, "entered lwfs_inop_open");

    open_recursive_depth++;

    log_debug(sysio_debug_level, "current recursion depth=%d", open_recursive_depth);
    /*
     * File is open. Load the DSO if needed.
     */
    if (pno->p_base->pb_ino) {
	log_debug(sysio_debug_level, "file already open");

	if ((PNODE_NS_ENTRY(pno).entry_obj.type == LWFS_FILE_ENTRY) && 
		(PNODE_NS_ENTRY(pno).d_obj == NULL)) {
	    /* populate the dso */
	    rc = sso_load_mo(FS2LFS(PNODE_FS(pno)),
		    &PNODE_NS_ENTRY(pno));
	    if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error loading management obj: %s",
			lwfs_err_str(rc));
		goto cleanup;
	    }
	}

	goto cleanup;
    }

    rc = create_file(pno, mode);
    
    /* FIX ME: This is a hack.  A real naming service should handle this better. */
    if (rc == LWFS_ERR_EXIST) {
    	/* between the lookup that returned LWFS_ERR_NOENT and the time open 
    	 * was called, someone else created the file.  try another lookup. */
	log_debug(sysio_debug_level, "couldn't create.  file exists.  try to lookup.");
    	rc = lwfs_inop_lookup(pno, &ino, NULL, NULL);
    	pno->p_base->pb_ino = ino;
    	log_debug(sysio_debug_level, "after lookup retry: pb_ino==%p, ino==%p", pno->p_base->pb_ino, ino);
    	if (rc == LWFS_OK) {
    		/* retry the open */
		log_debug(sysio_debug_level, "lookup retry succeeded.  now retry the open.");
    		rc = lwfs_inop_open(pno, flags, mode);
    		if (rc == LWFS_OK) {
			log_debug(sysio_debug_level, "open retry succeeded.");
    		} else {
    			goto cleanup;
    		}
	} else {
		goto cleanup;
	}
    }

cleanup:
    open_recursive_depth--;
    log_debug(sysio_debug_level, "finished lwfs_inop_open");

	snprintf(event_data, max_event_data, "open"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_OPEN, 
		0, event_data);

    return rc;
}

static int
lwfs_inop_close(struct inode *ino)
{
    int rc = LWFS_OK;

	int interval_id;
	char event_data[max_event_data];

	sysio_counter.close++;
	interval_id = sysio_counter.close; 
	snprintf(event_data, max_event_data, "close"); 
	trace_event(TRACE_SYSIO_INO_CLOSE, 0, event_data);
	trace_start_interval(interval_id, 0); 


    log_debug(sysio_debug_level, "entered lwfs_inop_close");
    log_debug(sysio_debug_level, "finished lwfs_inop_close");

	snprintf(event_data, max_event_data, "close"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_CLOSE, 
		0, event_data);

    return rc;
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
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.link++;
	interval_id = sysio_counter.link; 
	snprintf(event_data, max_event_data, "link"); 
	trace_event(TRACE_SYSIO_INO_LINK, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_link");
	
	memset(&link_entry, 0, sizeof(lwfs_ns_entry));

	/* get the name and parent ns_entry of the new link */
	COPY_PNODE_NAME(link_name, link);
	link_parent = &PNODE_NS_ENTRY(PNODE_PARENT(link));
	/* get the name and parent ns_entry of the link target */
	COPY_PNODE_NAME(target_name, target);
	target_parent = &PNODE_NS_ENTRY(PNODE_PARENT(target));

	/* create a link to the target */
	rc = lwfs_create_link_sync(&lwfs_fs->naming_svc,
				   &lwfs_fs->txn,
				   link_parent, link_name,
				   &lwfs_fs->cap,
				   target_parent, target_name,
				   &lwfs_fs->cap,
				   &link_entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error creating link: %s",
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_link");

	snprintf(event_data, max_event_data, "link"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_LINK, 
		0, event_data);

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
	lwfs_cap parent_cap;
	lwfs_cid parent_cid; 
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.unlink++;
	interval_id = sysio_counter.unlink; 
	snprintf(event_data, max_event_data, "unlink"); 
	trace_event(TRACE_SYSIO_INO_UNLINK, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_unlink");

	memset(&entry, 0, sizeof(lwfs_ns_entry));

	COPY_PNODE_NAME(name, pno);
	parent = &PNODE_NS_ENTRY(PNODE_PARENT(pno));

	parent_cid = parent->entry_obj.cid; 

	/* get the capability that allows us to remove entries from the container */
	rc = check_cap_cache(&lwfs_fs->authr_svc, parent_cid,
		LWFS_CONTAINER_WRITE, &lwfs_fs->cred, &parent_cap); 
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "unable to get cap: %s",
		    lwfs_err_str(rc));
	    return rc;
	}

	/* remove the file from the namespace */
	rc = lwfs_unlink_sync(&lwfs_fs->naming_svc, &lwfs_fs->txn, 
		parent, name, &parent_cap, &entry);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error removing file: %s",
				lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	}

	/* remove the associated object if the link count is zero */
	if ((entry.link_cnt == 0) && (entry.file_obj != NULL)) {

	    lwfs_cap obj_cap; 
	    lwfs_cid obj_cid = entry.file_obj->cid; 

	    log_debug(sysio_debug_level, "Removing object from storage server");

	    /* get the capability that allows us to remove objects from the container */
	    rc = check_cap_cache(&lwfs_fs->authr_svc, obj_cid,
		    LWFS_CONTAINER_WRITE, &lwfs_fs->cred, &obj_cap); 
	    if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "unable to get cap: %s",
			lwfs_err_str(rc));
		return rc;
	    }

	    if (entry.entry_obj.type == LWFS_FILE_ENTRY) { 
		/* if this is a file obj, populate the dso and remove it */
		if (entry.d_obj == NULL) {
		    /* populate the dso */
		    rc = sso_load_mo(lwfs_fs,
			    &entry);
		    if (rc != LWFS_OK) {
			log_error(sysio_debug_level, "error loading management obj: %s",
				lwfs_err_str(rc));
			goto cleanup;
		    }
		}

		rc = sso_remove_dso(lwfs_fs, 
			entry.d_obj->ss_obj,
			&obj_cap,
			entry.d_obj->ss_obj_count);
	    }

	    /* regardless of type, remove the primary obj */
	    rc = lwfs_remove_obj_sync(&lwfs_fs->txn, entry.file_obj, &obj_cap);
	    if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "error removing object: %s",
			lwfs_err_str(rc));
		rc = -EIO;
		goto cleanup;
	    }
	}

cleanup:
	log_debug(sysio_debug_level, "finished lwfs_inop_unlink");

	snprintf(event_data, max_event_data, "unlink"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_UNLINK, 
		0, event_data);

	return rc;
}

static int
lwfs_inop_rename(struct pnode *old, struct pnode *new)
{
	int rc = LWFS_OK;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.rename++;
	interval_id = sysio_counter.rename; 
	snprintf(event_data, max_event_data, "rename"); 
	trace_event(TRACE_SYSIO_INO_RENAME, 0, event_data);
	trace_start_interval(interval_id, 0); 

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

	snprintf(event_data, max_event_data, "rename"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_RENAME, 
		0, event_data);

	return rc;
}

/** This is a very naive implementation.  It does everything serially, 
 *  even if there are multiple servers
 * 
 *  Fix Me.
 * 
 */
static ssize_t
dopio(void *buf, size_t count, _SYSIO_OFF_T off, void *private)
{
	lwfs_io *lio_session = (lwfs_io *)private;
	lwfs_ns_entry *entry = &(I2LI(lio_session->lio_ino)->ns_entry);
	lwfs_size nbytes=0;

	log_debug(sysio_debug_level, "entered dopio");

	if (entry->d_obj == NULL) {
		log_error(sysio_debug_level, "distributed obj not initialized");
		errno = EIO;
		return -EIO;
	}
	
	nbytes = sso_io(buf, count, off, lio_session);

	log_debug(sysio_debug_level, "finished dopio");

	return nbytes;
}

static ssize_t
doiov(const struct iovec *iov,
      int count,
      _SYSIO_OFF_T off,
      ssize_t limit,
      void *private)
{
	lwfs_io *lio_session = (lwfs_io *)private;
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
				    dopio,
				    lio_session);
	if (cc < 0) {
		cc = -errno;
	} else {
		I2LI(lio_session->lio_ino)->fpos += cc;
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
	lwfs_io *lio_session;
	ssize_t	cc;

	log_debug(sysio_debug_level, "entered doio");

	lio_session = calloc(1, sizeof(lwfs_io));
	lio_session->lio_op = op;
	lio_session->lio_ino = ioctx->ioctx_ino	;
	lio_session->lio_fs = FS2LFS(INODE_FS(ioctx->ioctx_ino));
	lio_session->lio_outstanding_requests = calloc(1, sizeof(struct request_list));
	TAILQ_INIT(lio_session->lio_outstanding_requests);
	
	log_debug(LOG_ALL, "lio_session==%p, lio_outstanding_requests==%p", lio_session, lio_session->lio_outstanding_requests);
	
	ioctx->ioctx_private = lio_session;

	cc =
	    /* not really enumerate */
	    /* coalesce extents, then iterate thru the extents calling 'doiov' for each  */
	    _sysio_enumerate_extents(ioctx->ioctx_xtv, ioctx->ioctx_xtvlen,
				     ioctx->ioctx_iov, ioctx->ioctx_iovlen,
				     doiov,
				     lio_session);
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
	ino_t this_inode = ioctx->ioctx_ino->i_stbuf.st_ino;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.read++;
	interval_id = sysio_counter.read; 
	snprintf(event_data, max_event_data, "read"); 
	trace_event(TRACE_SYSIO_INO_READ, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_read (ino==%u)", this_inode);

	rc = doio('r', ioctx);

	log_debug(sysio_debug_level, "finished lwfs_inop_read");

	snprintf(event_data, max_event_data, "read"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_READ, 
		0, event_data);

	return rc;
}

static int
lwfs_inop_write(struct inode *ino __IS_UNUSED, struct ioctx *ioctx)
{
	int rc=0;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.write++;
	interval_id = sysio_counter.write; 
	snprintf(event_data, max_event_data, "write"); 
	trace_event(TRACE_SYSIO_INO_WRITE, 0, event_data);
	trace_start_interval(interval_id, 0); 

	ino_t this_inode = ioctx->ioctx_ino->i_stbuf.st_ino;

	log_debug(sysio_debug_level, "entered lwfs_inop_write (ino==%u)", this_inode);

	rc = doio('w', ioctx);

	log_debug(sysio_debug_level, "finished lwfs_inop_write");

	snprintf(event_data, max_event_data, "write"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_WRITE, 
		0, event_data);

	return rc;
}

static _SYSIO_OFF_T
lwfs_inop_pos(struct inode *ino, _SYSIO_OFF_T off)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.pos++;
	interval_id = sysio_counter.pos; 
	snprintf(event_data, max_event_data, "pos"); 
	trace_event(TRACE_SYSIO_INO_POS, 0, event_data);
	trace_start_interval(interval_id, 0); 

	ino_t this_inode = ino->i_stbuf.st_ino;

	log_debug(sysio_debug_level, "entered lwfs_inop_pos (ino==%u)", this_inode);

	log_debug(sysio_debug_level, "setting fpos to %u", off);
	I2LI(ino)->fpos = off;
	if (off > ino->i_stbuf.st_size) {
		log_debug(sysio_debug_level, "new fpos > st_size, setting st_size to %u", off);
		ino->i_stbuf.st_size = off;
	}

	log_debug(sysio_debug_level, "finished lwfs_inop_pos");

	snprintf(event_data, max_event_data, "pos"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_OPEN, 
		0, event_data);

	return I2LI(ino)->fpos;
}


static int
lwfs_inop_iodone(struct ioctx *ioctxp)
{
	int rc=1;
	
	lwfs_io *lio_session = (lwfs_io *)ioctxp->ioctx_private;
	int wait_rc;   /* result of the wait call */
	int remote_rc; /* result of the remote operation */
	struct request_entry *entry = NULL;
	lwfs_request *req = NULL;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.filldirentries++;
	interval_id = sysio_counter.filldirentries; 
	snprintf(event_data, max_event_data, "iodone"); 
	trace_event(TRACE_SYSIO_INO_IODONE, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_iodone");

	log_debug(LOG_ALL, "lio_session==%p, lio_outstanding_requests==%p", lio_session, lio_session->lio_outstanding_requests);

	if (TAILQ_EMPTY(lio_session->lio_outstanding_requests)) {
		log_debug(sysio_debug_level, "no outstanding AIO requests to wait for");
		rc = 1; /* success */
		goto cleanup;
	}

	while ((entry = TAILQ_FIRST(lio_session->lio_outstanding_requests)) != NULL) {
		TAILQ_REMOVE(lio_session->lio_outstanding_requests, entry, np);
		req = entry->req;
//	if (lio_session->lio_outstanding_request != NULL) {
//		req = lio_session->lio_outstanding_request;
		log_debug(LOG_ALL, "entry==%p, req==%p", entry, req);
		if (req != NULL) {
			wait_rc = lwfs_wait(req, &remote_rc);
			if (wait_rc != LWFS_OK) {
				log_error(sysio_debug_level, "wait failed: %s",
					  lwfs_err_str(wait_rc));
				rc = 0; /* failure */
				goto cleanup; 
			}
			if (remote_rc != LWFS_OK) {
				log_error(sysio_debug_level, "remote operation failed: %s",
					  lwfs_err_str(remote_rc));
				rc = 0; /* failure */
				goto cleanup; 
			}
			free(req);
			req = NULL;
			
			/* THK TODO:  update fpos and st_size */
		} else {
			log_debug(sysio_debug_level, "req is NULL.  must be doing fake I/O.");
		}
		free(entry);
		entry = NULL;
	}
	
#ifdef STAT_AFTER_IODONE
	/* 
	 * THK TODO: This is probably very slow.  What are we really trying to accomplish? 
	 * Knowing the actual file size?  Track that at IO time.
	 */
	lwfs_stat_ns_entry(INODE_FS(lio_session->lio_ino), &(INODE_NS_ENTRY(lio_session->lio_ino)), 0, &lio_session->lio_ino->i_stbuf);
#endif


cleanup:
	if (lio_session != NULL) {
//		free(lio_session->lio_outstanding_requests);
		free(lio_session);
	}
	
	if (req != NULL) free(req);
	if (entry != NULL) free(entry);

	log_debug(sysio_debug_level, "finished lwfs_inop_iodone");

	snprintf(event_data, max_event_data, "iodone"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_IODONE, 
		0, event_data);

	return rc;
}

static int
lwfs_inop_fcntl(struct inode *ino,
		  int cmd,
		  va_list ap,
		  int *rtn)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.fcntl++;
	interval_id = sysio_counter.fcntl; 
	snprintf(event_data, max_event_data, "fcntl"); 
	trace_event(TRACE_SYSIO_INO_FCNTL, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_fcntl");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "fcntl is not implemented");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "finished lwfs_inop_fcntl");

	snprintf(event_data, max_event_data, "fcntl"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_FCNTL, 
		0, event_data);

	return -ENOSYS;
}

static int
lwfs_inop_mknod(struct pnode *pno __IS_UNUSED,
		  mode_t mode __IS_UNUSED,
		  dev_t dev __IS_UNUSED)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.mknod++;
	interval_id = sysio_counter.mknod; 
	snprintf(event_data, max_event_data, "mknod"); 
	trace_event(TRACE_SYSIO_INO_MKNOD, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_mknod");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "mknod is not implemented");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "finished lwfs_inop_mknod");

	snprintf(event_data, max_event_data, "mknod"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_MKNOD, 
		0, event_data);

	return -ENOSYS;
}

#ifdef _HAVE_STATVFS
static int
lwfs_inop_statvfs(struct pnode *pno,
		    struct inode *ino,
		    struct intnl_statvfs *buf)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.statvfs++;
	interval_id = sysio_counter.statvfs; 
	snprintf(event_data, max_event_data, "statvfs"); 
	trace_event(TRACE_SYSIO_INO_STATVFS, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_statvfs");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "statvfs is not implemented");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "********************************************");
	log_debug(sysio_debug_level, "finished lwfs_inop_statvfs");

	snprintf(event_data, max_event_data, "statvfs"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_STATVFS, 
		0, event_data);

	return -ENOSYS;
}
#endif

static int
lwfs_inop_sync(struct inode *ino)
{
    int rc=LWFS_OK;
    int i=0;
    char ostr[33];
    lwfs_filesystem *lwfs_fs=FS2LFS(INODE_FS(ino));
    lwfs_ns_entry *entry = &(INODE_NS_ENTRY(ino));
    lwfs_cid obj_cid; 
    lwfs_cap obj_cap; 
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.sync++;
	interval_id = sysio_counter.sync; 
	snprintf(event_data, max_event_data, "sync"); 
	trace_event(TRACE_SYSIO_INO_SYNC, 0, event_data);
	trace_start_interval(interval_id, 0); 

    log_debug(sysio_debug_level, "entered lwfs_inop_sync");

    if ((entry->entry_obj.type == LWFS_FILE_ENTRY) && 
	    (entry->d_obj == NULL)) {
	log_error(sysio_debug_level, "loading management obj");
	/* populate the dso */
	rc = sso_load_mo(FS2LFS(INODE_FS(ino)),
		&INODE_NS_ENTRY(ino));
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "error loading management obj: %s",
		    lwfs_err_str(rc));
	    goto cleanup;
	}
    }

    obj_cid = entry->entry_obj.cid;

    /* get the capability that allows us to remove objects from the container */
    rc = check_cap_cache(&lwfs_fs->authr_svc, obj_cid,
	    LWFS_CONTAINER_WRITE, &lwfs_fs->cred, &obj_cap); 
    if (rc != LWFS_OK) {
	log_error(sysio_debug_level, "unable to get cap: %s",
		lwfs_err_str(rc));
	return rc;
    }


    for (i=0; i < entry->d_obj->ss_obj_count; i++) {
	rc = lwfs_fsync_sync(&lwfs_fs->txn,
		&entry->d_obj->ss_obj[i],
		&obj_cap);
	if (rc != LWFS_OK) {
	    log_error(sysio_debug_level, "error syncing obj (oid=0x%s): %s",
		    lwfs_oid_to_string((INODE_NS_ENTRY(ino).d_obj->ss_obj[i].oid), ostr), 
		    lwfs_err_str(rc));
	    goto cleanup;
	}
    }

cleanup:

    log_debug(sysio_debug_level, "finished lwfs_inop_sync");

	snprintf(event_data, max_event_data, "sync"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_SYNC, 
		0, event_data);

    return rc;
}

static int
lwfs_inop_datasync(struct inode *ino)
{
	int rc=LWFS_OK;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.datasync++;
	interval_id = sysio_counter.datasync; 
	snprintf(event_data, max_event_data, "datasync"); 
	trace_event(TRACE_SYSIO_INO_DATASYNC, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_datasync");

	rc = lwfs_inop_sync(ino);

	log_debug(sysio_debug_level, "finished lwfs_inop_datasync");

	snprintf(event_data, max_event_data, "datasync"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_DATASYNC, 
		0, event_data);

	return rc;
}

static int
lwfs_inop_ioctl(struct inode *ino __IS_UNUSED,
		  unsigned long int request __IS_UNUSED,
		  va_list ap __IS_UNUSED)
{
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.ioctl++;
	interval_id = sysio_counter.ioctl; 
	snprintf(event_data, max_event_data, "ioctl"); 
	trace_event(TRACE_SYSIO_INO_IOCTL, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_ioctl");
	log_debug(sysio_debug_level, "finished lwfs_inop_ioctl");

	snprintf(event_data, max_event_data, "ioctl"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_IOCTL, 
		0, event_data);

	return -ENOTTY;
}

static void
lwfs_inop_gone(struct inode *ino)
{
	lwfs_inode *lino = (lwfs_inode *)ino->i_private;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.ino_gone++;
	interval_id = sysio_counter.ino_gone; 
	snprintf(event_data, max_event_data, "ino_gone"); 
	trace_event(TRACE_SYSIO_INO_GONE, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_inop_gone(%s)", lino->ns_entry.name);
	
	log_debug(sysio_debug_level, "lino == %p", lino);
	log_debug(sysio_debug_level, "lino->ns_entry == %p", &lino->ns_entry);
	if (logging_debug(sysio_debug_level)) {
		fprint_lwfs_ns_entry(logger_get_file(), 
				     "lino->ns_entry", 
				     "DEBUG fs_lwfs.c:lwfs_inop_gone - about to free", 
				     &lino->ns_entry);
	}

	if (lino != NULL) {
		if (lino->ns_entry.file_obj != NULL)
			log_debug(sysio_debug_level, "lino->ns.file_obj == %p", lino->ns_entry.file_obj);
			free(lino->ns_entry.file_obj);
		if (lino->ns_entry.d_obj != NULL) {
			if (lino->ns_entry.d_obj->ss_obj != NULL)
				free(lino->ns_entry.d_obj->ss_obj);
		        free(lino->ns_entry.d_obj);
		}
		free(lino->fileid.fid_data);
		free(lino);
	}

	log_debug(sysio_debug_level, "finished lwfs_inop_gone");

	snprintf(event_data, max_event_data, "ino_gone"); 
	trace_end_interval(interval_id, TRACE_SYSIO_INO_GONE, 
		0, event_data);

	return;
}

static void
lwfs_fsop_gone(struct filesys *fs)
{
	lwfs_namespace *ns = &FS2LFS(fs)->namespace;
	int interval_id;
	char event_data[max_event_data];

	sysio_counter.fs_gone++;
	interval_id = sysio_counter.fs_gone; 
	snprintf(event_data, max_event_data, "fs_gone"); 
	trace_event(TRACE_SYSIO_FS_GONE, 0, event_data);
	trace_start_interval(interval_id, 0); 

	log_debug(sysio_debug_level, "entered lwfs_fsop_gone");

	if (ns != NULL) {
		if (ns->ns_entry.file_obj != NULL)
			free(ns->ns_entry.file_obj);
		if (ns->ns_entry.d_obj != NULL) {
			if (ns->ns_entry.d_obj->ss_obj != NULL)
				free(ns->ns_entry.d_obj->ss_obj);
		        free(ns->ns_entry.d_obj);
		}
	}

	free(FS2LFS(fs));

	/* release the caps cache */
	hashtable_destroy(&cap_ht, free); 

	log_debug(sysio_debug_level, "cache_hits=%d, cache_misses=%d\n",cache_hits, cache_misses);
	
//	malloc_report();

	log_debug(sysio_debug_level, "finished lwfs_fsop_gone");
	
	if (logger_get_file() != NULL) {
		fclose(logger_get_file());
	}
	
	snprintf(event_data, max_event_data, "fs_gone"); 
	trace_end_interval(interval_id, TRACE_SYSIO_FS_GONE, 
		0, event_data);

	return;
}