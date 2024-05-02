/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

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
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;

	fprintf(stdout, "getattr(%s, ...)\n", path); 

	res = lstat(path, stbuf);
	if(res == -1)
		return -errno;

	/*
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
	*/


	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
    int res;

    fprintf(stdout, "readlink(%s, ...)\n", path); 

    res = readlink(path, buf, size - 1);
    if(res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}


static int xmp_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	DIR *dp;
	struct dirent *de;
	int res = 0;
	int count=0; 

	fprintf(stdout, "getdir(%s, ...)\n", path); 

	dp = opendir(path);
	if(dp == NULL)
		return -errno;

	while((de = readdir(dp)) != NULL) {
		fprintf(stdout, "getdir[%d] = (name=%s, type=%d, ino=%d)\n", 
			count++, de->d_name, de->d_type, (int)de->d_ino); 

		res = filler(h, de->d_name, de->d_type, de->d_ino);
		if(res != 0)
			break;
	}

	closedir(dp);
	return res;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res;

    fprintf(stdout, "mknod(%s, ...)\n", path); 

    res = mknod(path, mode, rdev);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    int res;

    fprintf(stdout, "mkdir(%s, ...)\n", path); 

    res = mkdir(path, mode);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_unlink(const char *path)
{
    int res;

    fprintf(stdout, "unlink(%s, ...)\n", path); 

    res = unlink(path);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_rmdir(const char *path)
{
    int res;

    fprintf(stdout, "rmdir(%s, ...)\n", path); 

    res = rmdir(path);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
    int res;

    fprintf(stdout, "symlink(from=%s, to=%s)\n", from, to); 

    res = symlink(from, to);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    int res;

    fprintf(stdout, "rename(from=%s, to=%s)\n", from, to); 

    res = rename(from, to);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_link(const char *from, const char *to)
{
    int res;

    fprintf(stdout, "link(from=%s, to=%s)\n", from, to); 

    res = link(from, to);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    int res;

    fprintf(stdout, "chmod(%s, ...)\n", path); 

    res = chmod(path, mode);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    fprintf(stdout, "chown(%s, ...)\n", path); 

    res = lchown(path, uid, gid);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
    int res;

    fprintf(stdout, "truncate(%s, ...)\n", path); 

    res = truncate(path, size);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_utime(const char *path, struct utimbuf *buf)
{
    int res;

    fprintf(stdout, "utime(%s, ...)\n", path); 

    res = utime(path, buf);
    if(res == -1)
        return -errno;

    return 0;
}


static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res;

    fprintf(stdout, "open(%s, ...)\n", path); 

    res = open(path, fi->flags);
    if(res == -1)
        return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    int fd;
    int res;

    fprintf(stdout, "read(%s, ...)\n", path); 

    (void) fi;
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
                     off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;

    fprintf(stdout, "write(%s, ...)\n", path); 

    (void) fi;
    fd = open(path, O_WRONLY);
    if(fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if(res == -1)
        res = -errno;

    close(fd);
    return res;
}

static int xmp_statfs(const char *path, struct statfs *stbuf)
{
    int res;

    fprintf(stdout, "statfs(%s, ...)\n", path); 

    res = statfs(path, stbuf);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */

    fprintf(stdout, "release(%s, ...)\n", path); 

    (void) path;
    (void) fi;
    return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi)
{
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */

    fprintf(stdout, "fsync(%s, ...)\n", path); 

    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags)
{
    int res = lsetxattr(path, name, value, size, flags);
    if(res == -1)
        return -errno;
    return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
                    size_t size)
{
    int res = lgetxattr(path, name, value, size);
    if(res == -1)
        return -errno;
    return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
    int res = llistxattr(path, list, size);
    if(res == -1)
        return -errno;
    return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
    int res = lremovexattr(path, name);
    if(res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
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
    .fsync	= xmp_fsync,
#ifdef HAVE_SETXATTR
    .setxattr	= xmp_setxattr,
    .getxattr	= xmp_getxattr,
    .listxattr	= xmp_listxattr,
    .removexattr= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &xmp_oper);
}
