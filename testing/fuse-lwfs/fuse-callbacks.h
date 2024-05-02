/**
 * @file fuse_callbacks.h 
 * 
 * @brief Prototypes of the FUSE callbacks used for the LWFS. 
 *
 * @author Ron Oldfield (raoldfi\@sandia.gov)
 * $Revision: 1073 $
 * $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
*/


#ifndef _FUSE_CALLBACKS_H_
#define _FUSE_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	extern int lwfs_fuse_getattr(
			const char *path, 
			struct stat *stbuf);

	extern int lwfs_fuse_readlink(
			const char *path, 
			char *buf, 
			size_t size);


	extern int lwfs_fuse_getdir(
			const char *path, 
			fuse_dirh_t h, 
			fuse_dirfil_t filler);

	extern int lwfs_fuse_mknod(
			const char *path, 
			mode_t mode, 
			dev_t rdev);

	extern int lwfs_fuse_mkdir(
			const char *path, 
			mode_t mode);

	extern int lwfs_fuse_unlink(
			const char *path);

	extern int lwfs_fuse_rmdir(
			const char *path);

	extern int lwfs_fuse_symlink(
			const char *from,
			const char *to);

	extern int lwfs_fuse_rename(
			const char *from, 
			const char *to);

	extern int lwfs_fuse_link(
			const char *from, 
			const char *to);

	extern int lwfs_fuse_chmod(
			const char *path, 
			mode_t mode);

	extern int lwfs_fuse_chown(
			const char *path, 
			uid_t uid, 
			gid_t gid);

	extern int lwfs_fuse_truncate(
			const char *path, 
			off_t size);

	extern int lwfs_fuse_utime(
			const char *path, 
			struct utimbuf *buf);


	extern int lwfs_fuse_open(
			const char *path, 
			struct fuse_file_info *fi);

	extern int lwfs_fuse_read(
			const char *path, 
			char *buf, 
			size_t size, 
			off_t offset,
			struct fuse_file_info *fi);

	extern int lwfs_fuse_write(
			const char *path, 
			const char *buf, 
			size_t size,
			off_t offset, 
			struct fuse_file_info *fi);

	extern int lwfs_fuse_statfs(
			const char *path, 
			struct statfs *stbuf);

	extern int lwfs_fuse_release(
			const char *path, 
			struct fuse_file_info *fi);

	extern int lwfs_fuse_fsync(
			const char *path, 
			int isdatasync,
			struct fuse_file_info *fi);

#ifdef HAVE_SETXATTR
	/* xattr operations are optional and can safely be left unimplemented */
	extern int lwfs_fuse_setxattr(
			const char *path, 
			const char *name, 
			const char *value,
			size_t size, 
			int flags); 

	extern int lwfs_fuse_getxattr(
			const char *path, 
			const char *name, 
			char *value,
			size_t size);

	extern int lwfs_fuse_listxattr(
			const char *path, 
			char *list, 
			size_t size);

	extern int lwfs_fuse_removexattr(
			const char *path, 
			const char *name);

#endif /* HAVE_SETXATTR */


#else 

#endif 

#ifdef __cplusplus
}
#endif


#endif