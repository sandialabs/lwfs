/*
    This is an interface between FUSE, the Filesystem in Userspace,
    and LWFS, the Light Weight File System.

    $ID:$
*/

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500

/* I'm not sure why this is needed. Including stdlib.h should be enough. */
extern long long int strtoll (__const char *__restrict __nptr,
    char **__restrict __endptr, int __base);
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>	/* For strcmp() */
#include <stdlib.h>	/* For strtoll() */
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#include <getopt.h>
#include <fuse.h>
#include "lwfs.h"
#include "lwfs_xdr.h"


/*
** Constants
*/
#define STORAGESERVER	"/tmp/SS"



/*
** Local functions
*/
static void usage(char *pname);
static int traverse_path(const char *path, lwfs_did_t *parentID, char *nodename);
static int path2nodeID(const char *path, lwfs_did_t *ID);
static void path2name(char *fname, const char *path);

static int xmp_getattr(const char *path, struct stat *stbuf);
static int xmp_readlink(const char *path, char *buf, size_t size);
static int xmp_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler);
static int xmp_mknod(const char *path, mode_t mode, dev_t rdev);
static int xmp_mkdir(const char *path, mode_t mode);
static int xmp_unlink(const char *path);
static int xmp_rmdir(const char *path);
static int xmp_symlink(const char *from, const char *to);
static int xmp_rename(const char *from, const char *to);
static int xmp_link(const char *from, const char *to);
static int xmp_chmod(const char *path, mode_t mode);
static int xmp_chown(const char *path, uid_t uid, gid_t gid);
static int xmp_truncate(const char *path, off_t size);
static int xmp_utime(const char *path, struct utimbuf *buf);
static int xmp_open(const char *path, int flags);
static int xmp_read(const char *path, char *buf, size_t size, off_t offset);
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset);
static int xmp_statfs(struct fuse_statfs *fst);
static int xmp_release(const char *path, int flags);
static int xmp_fsync(const char *path, int isdatasync);



/*
** Setup the fuse functions
*/
static struct fuse_operations xmp_oper=   {
    .getattr	= xmp_getattr,
    .readlink	= xmp_readlink,
    .getdir	= xmp_getdir,
    .mknod	= xmp_mknod,
    .mkdir	= xmp_mkdir,
    .symlink	= xmp_symlink,
    .unlink	= xmp_unlink,
    .rmdir	= xmp_rmdir,
    .rename	= xmp_rename,
    .link	= xmp_link,
    .chmod	= xmp_chmod,
    .chown	= xmp_chown,
    .truncate	= xmp_truncate,
    .utime	= xmp_utime,
    .open	= xmp_open,
    .read	= xmp_read,
    .write	= xmp_write,
    .statfs	= xmp_statfs,
    .release	= xmp_release,
    .fsync	= xmp_fsync
};



/*
** -----------------------------------------------------------------------------
** Main: process command line options, initialize lwfs, and jump into fuse
** -----------------------------------------------------------------------------
*/
int
main(int argc, char *argv[])
{

extern char *optarg;
extern int optind;
int fuseargc;
int ch;

uint64_t nid; 
int verbose;
int done= 0;


    /* Set some defaults */
    verbose= 0;
    nid= 0;


    /* Process command line options, but only after the --  */
    while ((strcmp(argv[optind], "--") != 0) && (argv[optind] != NULL))   {
	optind++;
    }
    fuseargc= optind - 1;
    optind++;

    while (!done)   {
	ch = getopt(argc, argv, "hn:v:");
	if (ch == -1)   {
	    /* Done proecssing command line options */
	    done= 1;
	    break;
	}

	switch (ch) {
	    case 'h': /* help */
		    usage(argv[0]);
		    exit(0);

	    case 'n': /* nid */
		    nid = strtoll(optarg, NULL, 10);
		    fprintf(stderr, "nid is now %lld\n", nid);
		    break;

	    case 'v': /* verbose */
		    verbose= atoi(optarg);
		    break;

	    default:
		    usage(argv[0]);
		    exit(1);
	}
    }


    if (nid == 0) {
        fprintf(stderr, "No nid specified!\n");
	usage(argv[0]);
        exit(1);
    }

    /* Setup logging */
    logger_set_file(stderr);
    logger_set_default_level(verbose);

    /* Initialize LWFS and establish a connection to the MDS */
    fprintf(stderr, "Initializing LWFS with nid %lld\n", nid);
    lwfs_init(nid);

    /* Call fuse to get started... */
    fuse_main(fuseargc, argv, &xmp_oper);
    return 0;

}  /* end of main() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s mount_point [fusermount options] -- -n <nid> "
	"[-h] [-v <lvl>]\n", pname);
    fprintf(stderr, "    -n <nid>    Set nid to <nid> (required!)\n");
    fprintf(stderr, "    -h          This usage message\n");
    fprintf(stderr, "    -v <lvl>    Set verbosity to level <lvl>\n");

}  /* end of usage() */



/*
** -----------------------------------------------------------------------------
** Fuse functions
** -----------------------------------------------------------------------------
*/
static int
xmp_getattr(const char *path, struct stat *stbuf)
{

lwfs_did_t parentID;
char nodename[LWFS_NAME_LEN];
lwfs_obj_ref_t dir;
lwfs_cap_t cap;
mds_res_N_rc_t result;
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname;
int rc;


    fprintf(stderr, "======= Calling xmp_getattr(path %s, ...)\n", path);
    memset(stbuf, 0, sizeof(struct stat));

    if (traverse_path(path, &parentID, nodename) != 0)   {
	errno= ENOENT;
	fprintf(stderr, "    ----------- xmp_getattr() traverse_path() FAILED %d (%s)\n",
	    errno, strerror(errno));
	return -errno;
    }
    fprintf(stderr, "    ----------- xmp_getattr() node name is %s\n", nodename);

    strncpy(name, nodename, LWFS_NAME_LEN);
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= parentID;

    /* FIXME: We need the mds_getattr call! */
    memset(&result, 0, sizeof(result));
    memset(&request, 0, sizeof(request));
    rc= mds_lookup(&dir, &name, &cap, &result, &request);
    if (rc != LWFS_OK) {
	errno= ENOENT;
	fprintf(stderr, "    ----------- xmp_getattr() mds_lookup() FAILED %d (%s)\n",
	    errno, strerror(errno));
	return -errno;
    }

    rc= lwfs_comm_wait(&request);
    if (rc != LWFS_OK) {
	errno= ENOENT;
	fprintf(stderr, "    ----------- xmp_getattr() mds_comm_wait() FAILED %d (%s)\n",
	    errno, strerror(errno));
	return -errno;
    }

    if (result.ret != LWFS_OK)   {
	errno= ENOENT;
	fprintf(stderr, "    ----------- xmp_getattr() mds_lookup() returned %d: %d (%s)\n",
	    result.ret, errno, strerror(errno));
	return -errno;
    }


    /*
    ** FIXME:
    ** These hard-coded constants should be MDS_REG, MDS_DIR, and MDS_LNK.
    */
    if (result.node.type == 1)   {
	stbuf->st_mode= S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;     /* regular file */
	fprintf(stderr, "    ----------- xmp_getattr() It's a regular file\n");
    } else if (result.node.type == 2)   {
	stbuf->st_mode= S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;     /* directory */
	fprintf(stderr, "    ----------- xmp_getattr() It's a directory\n");
    } else if (result.node.type == 3)   {
	/* This is actually a hard link */
	stbuf->st_mode= S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;     /* regular file */
	fprintf(stderr, "    ----------- xmp_getattr() It's a link\n");
    } else   {
	stbuf->st_mode= 0;     /* ??? */
	fprintf(stderr, "    ----------- xmp_getattr() Unknown file type\n");
	errno= EFAULT;
	return -errno;
    }

    stbuf->st_dev= 1;			/* device */
    stbuf->st_ino= parentID.part4;      /* inode */
    stbuf->st_nlink= result.node.links;	/* number of hard links */
    stbuf->st_uid= 501;			/* user ID of owner */
    stbuf->st_gid= 100;			/* group ID of owner */
    stbuf->st_rdev= 0;			/* device type (if inode device) */
    stbuf->st_size= 0;			/* total size, in bytes */
    stbuf->st_blksize= 512;		/* blocksize for filesystem I/O */
    stbuf->st_blocks= 0;		/* number of blocks allocated */
    stbuf->st_atime= 0;			/* time of last access */
    stbuf->st_mtime= 0;			/* time of last modification */
    stbuf->st_ctime= 0;			/* time of last change */

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
    fprintf(stderr, "    ----------- xmp_getattr() OK\n");

    return 0;

}  /* end of xmp_getattr() */


static int xmp_readlink(const char *path, char *buf, size_t size)
{
    int res;

    fprintf(stderr, "======= Calling xmp_readlink(path %s, ...)\n", path);
    res = readlink(path, buf, size - 1);
    if(res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}


static int
xmp_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{

lwfs_did_t dirID;
int type;
lwfs_obj_ref_t dir;
lwfs_cap_t cap;
mds_readdir_res_t result;
lwfs_request_t request;
dlist entry;
int rc;


    fprintf(stderr, "======= Calling xmp_getdir(path %s, ...)\n", path);

    if (path2nodeID(path, &dirID) != 0)   {
        return -EIO;
    }

    /* Read the directory from the MDS */
    /* FIXME: Later we may have to do this in pieces */
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= dirID;

    fprintf(stderr, "    ----------- xmp_getdir() Calling mds_readdir\n");
    memset(&result, 0, sizeof(result));
    memset(&request, 0, sizeof(request));
    rc= mds_readdir(&dir, &cap, &result, &request);
    if (rc != LWFS_OK) {
	return -EBADF;
    }
    fprintf(stderr, "    ----------- xmp_getdir() Back from mds_readdir\n");

    rc= lwfs_comm_wait(&request);
    fprintf(stderr, "    ----------- xmp_getdir() Back from lwfs_comm_wait, rc %d\n", rc);
    if (rc != LWFS_OK) {
	return -EBADF;
    }

    rc= 0;
    /* Fake . and .. */
    rc= filler(h, ".", 4);
    fprintf(stderr, "    ----------- xmp_getdir() Processing type 4, \".\"\n");
    if (rc != 0)   {
	return rc;
    }
    rc= filler(h, "..", 4);
    fprintf(stderr, "    ----------- xmp_getdir() Processing type 4, \"..\"\n");
    if (rc != 0)   {
	return rc;
    }

    /* Now process the 'real" ones */
    if (result.return_code == LWFS_OK)   {
        entry= result.start;
	fprintf(stderr, "    ----------- xmp_getdir() Got data\n");
        while (entry)   {
	    if (entry->oref.type == MDS_REG)   {
		type= 8;
	    } else if (entry->oref.type == MDS_DIR)   {
		type= 4;
	    } else if (entry->oref.type == MDS_LNK)   {
		/*
		** We actually support hard links to directories, but fuse
		** will never ask us to create one, so we can be confident that
		** this is a link to a regular file.
		*/
		type= 8;
	    } else   {
		type= 0;
	    }

	    fprintf(stderr, "    ----------- xmp_getdir() Processing type %d, \"%s\"\n",
		type, entry->name);
	    rc= filler(h, entry->name, type);
	    if (rc != 0)   {
		break;
	    }
            entry= entry->next;
        }
    }

    fprintf(stderr, "    ----------- xmp_getdir() rc %d\n", rc);
    return rc;

}  /* end of xmp_getdir() */



static int
xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{

lwfs_obj_ref_t node;
lwfs_cap_t cap;
lwfs_did_t parentID;
mds_res_N_rc_t result;
lwfs_request_t request;
char nodename[LWFS_NAME_LEN];
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname;
int rc;


    fprintf(stderr, "======= Calling xmp_mknod(path %s, ...)\n", path);
    if (traverse_path(path, &parentID, nodename) != 0)   {
        return -EIO;
    }

    strncpy(name, nodename, LWFS_NAME_LEN);
    memset(&node, 0, sizeof(node));
    node.dbkey= parentID;

    memset(&result, 0, sizeof(result));
    memset(&request, 0, sizeof(request));
    rc= mds_create(&node, &name, &cap, &result, &request);

    if (rc != LWFS_OK) {
        return -EIO;
    }

    rc= lwfs_comm_wait(&request);

    if (rc != LWFS_OK) {
        return -EIO;
    }

    fprintf(stderr, "    ----------- xmp_mknod() OK\n");
    return 0;

}  /* end of xmp_mknod() */



static int
xmp_mkdir(const char *path, mode_t mode)
{

lwfs_obj_ref_t dir;
lwfs_cap_t cap;
lwfs_did_t parentID;
mds_res_N_rc_t result;
lwfs_request_t request;
char dirname[LWFS_NAME_LEN];
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname;
int rc;


    fprintf(stderr, "======= Calling xmp_mkdir(path %s, ...)\n", path);
    if (traverse_path(path, &parentID, dirname) != 0)   {
        return -EIO;
    }

    strncpy(name, dirname, LWFS_NAME_LEN);
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= parentID;

    memset(&result, 0, sizeof(result));
    memset(&request, 0, sizeof(request));
    rc= mds_mkdir(&dir, &name, &cap, &result, &request);

    if (rc != LWFS_OK) {
        return -EIO;
    }

    rc= lwfs_comm_wait(&request);

    if (rc != LWFS_OK) {
        return -EIO;
    }

    fprintf(stderr, "    ----------- xmp_mkdir() OK\n");

    return 0;

}  /* end of xmp_mkdir() */



static int
xmp_unlink(const char *path)
{

int fd;
lwfs_obj_ref_t node;
lwfs_did_t nodeID;
lwfs_obj_ref_t parent;
lwfs_did_t parentID;
lwfs_cap_t cap;
mds_res_note_rc_t result;
mds_res_N_rc_t LookUpResult;
lwfs_request_t request;
int rc;
char fname[LWFS_NAME_LEN];
char nodename[LWFS_NAME_LEN];
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int remove;


    fprintf(stderr, "======= Calling xmp_unlink(path %s)\n", path);
    /* Remove it from the "storage server" while we still can get its ID */
    if (traverse_path(path, &parentID, nodename) != 0)   {
        return -EIO;
    }
    remove= 1;

    /* We only do this for regular files, not hard links */
    strncpy(name, nodename, LWFS_NAME_LEN);
    memset(&parent, 0, sizeof(parent));
    memset(&LookUpResult, 0, sizeof(LookUpResult));
    memset(&request, 0, sizeof(request));
    parent.dbkey= parentID;
    rc= mds_lookup(&parent, &name, &cap, &LookUpResult, &request);
    if (rc != LWFS_OK) {
	return -1;
    }

    rc= lwfs_comm_wait(&request);
    if (rc != LWFS_OK) {
	return -1;
    }

    if (LookUpResult.node.type != 1)   {
	fprintf(stderr, "    ----------- xmp_unlink() not removing SS file because "
	    "it is a link\n");
	#ifdef rrr
	/*
	** Look up the thing we are linking to. If it's count is 1, we should
	** remove the object on the storage server.
	*/
	memset(&node, 0, sizeof(node));
	node.dbkey= LookUpResult.node.linkto;
	memset(&result, 0, sizeof(result));
	memset(&request, 0, sizeof(request));
	rc= mds_lookup2(&node, &cap, &result, &request);
	if (rc != LWFS_OK) {
	    return -1;
	}

	comm_wait

	check link_cnt
	#endif 
	remove= 0;
    }


    /* And we only do this, if the link count is 0 */
    if (LookUpResult.node.links != 0)   {
	fprintf(stderr, "    ----------- xmp_unlink() not removing SS file because "
	    "link cnt != 0\n");
	remove= 0;
    }


    /* Now remove it from the "storage server" */
    if (remove)   {
	path2name(fname, path);
	fd = unlink(fname);
	if(fd == -1)
	    return -errno;
    }


    /* Now remove it from the MDS */
    if (path2nodeID(path, &nodeID) != 0)   {
        return -EIO;
    }
    memset(&node, 0, sizeof(node));
    node.dbkey= nodeID;

    rc= mds_remove(&node, &cap, &result, &request);
    if (rc != LWFS_OK) {
        return -EIO;
    }

    rc= lwfs_comm_wait(&request);

    if (rc != LWFS_OK) {
        return -EIO;
    }

    fprintf(stderr, "    ----------- xmp_unlink() OK\n");
    return 0;

}  /* end of xmp_unlink() */



static int
xmp_rmdir(const char *path)
{

lwfs_obj_ref_t dir;
lwfs_did_t dirID;
lwfs_cap_t cap;
mds_res_note_rc_t result;
lwfs_request_t request;
int rc;


    fprintf(stderr, "======= Calling xmp_rmdir(path %s)\n", path);
    if (path2nodeID(path, &dirID) != 0)   {
        return -EIO;
    }

    memset(&dir, 0, sizeof(dir));
    dir.dbkey= dirID;

    rc= mds_rmdir(&dir, &cap, &result, &request);
    if (rc != LWFS_OK) {
        return -EIO;
    }

    rc= lwfs_comm_wait(&request);

    if (rc != LWFS_OK) {
        return -EIO;
    }

    fprintf(stderr, "    ----------- xmp_rmdir() OK\n");
    return 0;

}  /* end of xmp_rmdir() */



static int
xmp_symlink(const char *from, const char *to)
{

    fprintf(stderr, "======= Calling xmp_symlink(from %s, to %s) %s\n", from, to,
	"NOT Supported");
    return -1;

}  /* end of xmp_symlink() */


static int
xmp_rename(const char *from, const char *to)
{

lwfs_obj_ref_t node; 
lwfs_did_t nodeID;
lwfs_did_t parentID;
char newNodeName[LWFS_NAME_LEN];
lwfs_obj_ref_t newdir; 
lwfs_cap_t srccap, destcap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    fprintf(stderr, "======= Calling xmp_rename(from %s, to %s)\n", from, to);
    /* Figure out the ID of the node we are about to rename */
    if (path2nodeID(from, &nodeID) != 0)   {
        return -EIO;
    }
    fprintf(stderr, "        node ID       0x%08x%08x%08x%08x\n",
	(unsigned int)nodeID.part1, (unsigned int)nodeID.part2,
	(unsigned int)nodeID.part3, (unsigned int)nodeID.part4);

    /* Figure out the new name and the new direcotry ID */
    if (traverse_path(to, &parentID, newNodeName) != 0)   {
        return -EIO;
    }
    fprintf(stderr, "        new parent ID 0x%08x%08x%08x%08x\n",
	(unsigned int)parentID.part1, (unsigned int)parentID.part2,
	(unsigned int)parentID.part3, (unsigned int)parentID.part4);
    fprintf(stderr, "        new name      \"%s\"\n", newNodeName);


    strncpy(name, newNodeName, LWFS_NAME_LEN); 
    memset(&node, 0, sizeof(node));
    node.dbkey= nodeID;
    memset(&newdir, 0, sizeof(newdir));
    newdir.dbkey= parentID;

    rc= mds_rename(&node, &srccap, &newdir, &name, &destcap, &result, &request);
    if (rc != LWFS_OK) {
	return -EIO;
    }

    rc= lwfs_comm_wait(&request); 

    if (rc != LWFS_OK) {
	return -EIO;
    }

    fprintf(stderr, "    ----------- xmp_rename() rc %d\n", result.ret);
    return result.ret;

}  /* end of xmp_rename() */


static int xmp_link(const char *from, const char *to)
{

lwfs_obj_ref_t target; 
lwfs_did_t targetID;
lwfs_did_t destdirID;
char newLinkName[LWFS_NAME_LEN];
lwfs_obj_ref_t destdir; 
lwfs_cap_t destcap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    fprintf(stderr, "======= Calling xmp_link(from %s, to %s)\n", from, to);
    /* Figure out the ID of the node we are about to link to */
    if (path2nodeID(from, &targetID) != 0)   {
        return -EIO;
    }
    fprintf(stderr, "        target ID      0x%08x%08x%08x%08x\n",
	(unsigned int)targetID.part1, (unsigned int)targetID.part2,
	(unsigned int)targetID.part3, (unsigned int)targetID.part4);

    /* Figure out the new name and the new direcotry ID */
    if (traverse_path(to, &destdirID, newLinkName) != 0)   {
        return -EIO;
    }
    fprintf(stderr, "        directory ID   0x%08x%08x%08x%08x\n",
	(unsigned int)destdirID.part1, (unsigned int)destdirID.part2,
	(unsigned int)destdirID.part3, (unsigned int)destdirID.part4);
    fprintf(stderr, "        Link name      \"%s\"\n", newLinkName);


    strncpy(name, newLinkName, LWFS_NAME_LEN); 
    memset(&target, 0, sizeof(target));
    target.dbkey= targetID;
    memset(&destdir, 0, sizeof(destdir));
    destdir.dbkey= destdirID;

    rc= mds_link(&target, &destdir, &name, &destcap, &result, &request);
    if (rc != LWFS_OK) {
	return -EIO;
    }

    rc= lwfs_comm_wait(&request); 

    if (rc != LWFS_OK) {
	return -EIO;
    }

    fprintf(stderr, "    ----------- xmp_link() rc %d\n", result.ret);
    return result.ret;

}  /* end of xmp_link() */



static int xmp_chmod(const char *path, mode_t mode)
{
    int res;

    fprintf(stderr, "======= Calling xmp_chmod(path %s, mode 0x%0x)\n", path, mode);
    res = chmod(path, mode);
    if(res == -1)
        return -errno;
    
    return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    fprintf(stderr, "======= Calling xmp_chown(path %s, ...)\n", path);
    res = lchown(path, uid, gid);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
    int res;
    
    fprintf(stderr, "======= Calling xmp_truncate(path %s, size %d)\n", path, (int)size);
    res = truncate(path, size);
    if(res == -1)
        return -errno;

    return 0;
}

static int
xmp_utime(const char *path, struct utimbuf *buf)
{

int fd;
char fname[LWFS_NAME_LEN];

    
    fprintf(stderr, "======= Calling xmp_utime(path %s, ...)\n", path);
    path2name(fname, path);
    fd = utime(fname, buf);
    if(fd == -1)
        return -errno;

    fprintf(stderr, "    ----------- xmp_utime() OK\n");
    return 0;

}  /* end of xmp_utime() */


static int
xmp_open(const char *path, int flags)
{

int fd;
char fname[LWFS_NAME_LEN];


    fprintf(stderr, "======= Calling xmp_open(path %s, ...)\n", path);
    path2name(fname, path);
    fd = open(fname, flags);
    if(fd == -1) 
        return -errno;

    close(fd);
    fprintf(stderr, "    ----------- xmp_open() OK\n");
    return 0;

}  /* end of xmp_open() */



static int xmp_read(const char *path, char *buf, size_t size, off_t offset)
{
    int fd;
    int res;

    fprintf(stderr, "======= Calling xmp_read(path %s, ...)\n", path);
    fd = open(path, O_RDONLY);
    if(fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if(res == -1)
        res = -errno;
    
    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset)
{
    int fd;
    int res;

    fprintf(stderr, "======= Calling xmp_write(path %s, ...)\n", path);
    fd = open(path, O_WRONLY);
    if(fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if(res == -1)
        res = -errno;
    
    close(fd);
    return res;
}

static int
xmp_statfs(struct fuse_statfs *fst)
{
    fprintf(stderr, "======= Calling xmp_statfs()\n");

    fst->block_size  = 512;
    fst->blocks      = 1000000;
    fst->blocks_free =  999990;
    fst->files       =  999999;
    fst->files_free  =  999999;
    fst->namelen     = LWFS_NAME_LEN;

    return 0;

}  /* end of xmp_statfs() */


static int
xmp_release(const char *path, int flags)
{
    /* Just a stub.  This method is optional and can safely be left unimplemented */
    fprintf(stderr, "======= Calling xmp_release(path %s, ...), OK\n", path);
    return 0;

}  /* end of xmp_release() */


static int
xmp_fsync(const char *path, int isdatasync)
{
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */

    fprintf(stderr, "======= Calling xmp_fsync(path %s, ...)\n", path);
    (void) path;
    (void) isdatasync;
    return 0;

}  /* end of xmp_fsync() */


/*
** Traverse a path name and return the ID of the last element. For example
**
**     /tmp/foo/bar
**
** would return the ID of the node "bar".
** 
** return 0 if successful, and -1 otherwise.
*/
static int
path2nodeID(const char *path, lwfs_did_t *ID)
{

char *sptr;
char *eptr;
char dirname[LWFS_NAME_LEN];
lwfs_obj_ref_t dir;
lwfs_cap_t cap;
mds_res_N_rc_t result;
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname;
int rc;


    /* start at root */
    ID->part1= 0;
    ID->part2= 0;
    ID->part3= 0;
    ID->part4= 0;


    if ((path[0] == '/') && (path[1] == 0))   {
	fprintf(stderr, "        path2nodeID() root \'/\' only\n");
	return 0;
    }


    /* Now traverse the path */
    sptr= (char *)path;
    eptr= (char *)path;

    while (1)   {
	while ((*eptr != '/') && (*eptr != '\0'))   {
	    eptr++;
	}
	/* Now we have pointers the begining and end of the next dir name */
	strncpy(dirname, sptr, eptr - sptr + 1);
	if (eptr == sptr)   {
	    dirname[eptr - sptr + 1]= '\0';
	} else   {
	    dirname[eptr - sptr]= '\0';
	}

	/* get the ID for dirname */
	fprintf(stderr, "        path2nodeID() mds_lookup \"%s\"\n", dirname);
	strncpy(name, dirname, LWFS_NAME_LEN);
	memset(&dir, 0, sizeof(dir));
	memset(&result, 0, sizeof(result));
	memset(&request, 0, sizeof(request));
	dir.dbkey= *ID;
	rc= mds_lookup(&dir, &name, &cap, &result, &request);
	if (rc != LWFS_OK) {
	    fprintf(stderr, "        path2nodeID() mds_lookup() FAILED\n");
	    return -1;
	}

	rc= lwfs_comm_wait(&request);
	if (rc != LWFS_OK) {
	    fprintf(stderr, "        path2nodeID() lwfs_comm_wait() FAILED\n");
	    return -1;
	}

	*ID= result.node.dbkey;

	if (*eptr == '\0')   {
	    /* we're done */
	    break;
	}

	eptr++;
	sptr= eptr;
    }

    fprintf(stderr, "        path2nodeID() ID 0x%08x%08x%08x%08x\n",
	(unsigned int)ID->part1, (unsigned int)ID->part2,
	(unsigned int)ID->part3, (unsigned int)ID->part4);
    return 0;

}  /* end of path2nodeID() */



/*
** Traverse a path name. Return the ID of the parent directory and the name of
** the last element. For example
**
**     /tmp/foo/bar
**
** would return the ID of the directory "foo" in parentID, and the name "bar"
** in nodename.
** return 0 if successful, and -1 otherwise.
*/
static int
traverse_path(const char *path, lwfs_did_t *parentID, char *nodename)
{

char *sptr;
char *eptr;
char dirname[LWFS_NAME_LEN];
lwfs_obj_ref_t dir;
lwfs_cap_t cap;
mds_res_N_rc_t result;
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname;
int rc;
char *base;


    if ((path[0] == '/') && (path[1] == 0))   {
	fprintf(stderr, "        traverse_path() root \'/\' only\n");
	strcpy(nodename, "/");
	parentID->part1= 0;
	parentID->part2= 0;
	parentID->part3= 0;
	parentID->part4= 0;
	return 0;
    }


    base= (char *)path + strlen(path); /* base now points at \0 */
    while (base >= path)   {
	base--;
	if (*base == '/')   {
	    break;
	}
    }
    base++;

    strcpy(nodename, base);
    fprintf(stderr, "        traverse_path() nodename is \"%s\"\n", nodename);

    /* Now traverse the path */
    sptr= (char *)path;
    eptr= (char *)path;

    /* start at root */
    parentID->part1= 0;
    parentID->part2= 0;
    parentID->part3= 0;
    parentID->part4= 0;

    while (1)   {
	while ((*eptr != '/') && (*eptr != '\0'))   {
	    eptr++;
	}
	/* Now we have pointers the begining and end of the next dir name */
	strncpy(dirname, sptr, eptr - sptr + 1);
	if (eptr == sptr)   {
	    dirname[eptr - sptr + 1]= '\0';
	} else   {
	    dirname[eptr - sptr]= '\0';
	}
	fprintf(stderr, "        traverse_path() looking at \"%s\"\n", dirname);

	if (strcmp(dirname, nodename) == 0)   {
	    /* we're done */
	    break;
	}

	/* get the ID for dirname */
	fprintf(stderr, "        traverse_path() mds_lookup \"%s\"\n", dirname);
	strncpy(name, dirname, LWFS_NAME_LEN);
	memset(&dir, 0, sizeof(dir));
	memset(&result, 0, sizeof(result));
	memset(&request, 0, sizeof(request));
	dir.dbkey= *parentID;
	fprintf(stderr, "        traverse_path() dir.dbkey 0x%08x%08x%08x%08x\n",
	    (unsigned int)dir.dbkey.part1, (unsigned int)dir.dbkey.part2,
	    (unsigned int)dir.dbkey.part3, (unsigned int)dir.dbkey.part4);
	rc= mds_lookup(&dir, &name, &cap, &result, &request);
	if (rc != LWFS_OK) {
	    fprintf(stderr, "        traverse_path() mds_lookup() FAILED\n");
	    return -1;
	}

	rc= lwfs_comm_wait(&request);
	if (rc != LWFS_OK) {
	    fprintf(stderr, "        traverse_path() lwfs_comm_wait() FAILED\n");
	    return -1;
	}
	fprintf(stderr, "        traverse_path() got key 0x%08x%08x%08x%08x\n",
	    (unsigned int)result.node.dbkey.part1, (unsigned int)result.node.dbkey.part2,
	    (unsigned int)result.node.dbkey.part3, (unsigned int)result.node.dbkey.part4);

	*parentID= result.node.dbkey;
	eptr++;
	sptr= eptr;
    }

    fprintf(stderr, "        traverse_path() parent ID 0x%08x%08x%08x%08x node "
	"name \"%s\"\n", (unsigned int)parentID->part1, (unsigned int)parentID->part2,
	(unsigned int)parentID->part3, (unsigned int)parentID->part4, nodename);
    return 0;

}  /* end of traverse_path() */


static void
path2name(char *fname, const char *path)
{

lwfs_did_t nodeID;
char nodeName[LWFS_NAME_LEN];


    if (path2nodeID(path, &nodeID) != 0)   {
	strcpy(fname, "");
        return;
    }

    sprintf(nodeName, "Node%08x%08x%08x%08x", (unsigned int)nodeID.part1,
	(unsigned int)nodeID.part2, (unsigned int)nodeID.part3,
	(unsigned int)nodeID.part4);

    strcpy(fname, STORAGESERVER);
    strcat(fname, nodeName);

}  /* end of path2name() */
