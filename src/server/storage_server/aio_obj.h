#ifndef AIO_OBJ_H
#define AIO_OBJ_H

/* see ss_obj.c for description of this object api */

#include "storage_server.h"
#include <unistd.h> 
#include <pthread.h>


extern int aio_obj_init(
    const char *root, 
    struct obj_funcs *obj_funcs);

extern int aio_obj_fini(void);

extern lwfs_ssize aio_obj_read(
		const lwfs_obj *obj, 
		const lwfs_ssize src_offset, 
		void *dest, 
		const lwfs_ssize len);

extern lwfs_ssize aio_obj_write(
		const lwfs_obj *dest_obj, 
		const lwfs_ssize dest_offset, 
		void *src, 
		const lwfs_ssize len);

extern int aio_obj_fsync(const lwfs_obj *obj);

#endif 
