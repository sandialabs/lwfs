#ifndef EBOFS_OBJ_H
#define EBOFS_OBJ_H

/* see ss_obj.c for description of this object api */

#include "common/types/types.h"


/*----------- THE CORE FUNCTIONS ------------- */
extern int ebofs_obj_init(
        const char *root, 
        struct obj_funcs *obj_funcs);

extern int ebofs_obj_fini();

extern lwfs_bool ebofs_obj_exists(const lwfs_obj *obj);

extern int ebofs_obj_fsync(const lwfs_obj *obj);

extern int ebofs_obj_create(
		const lwfs_obj *obj);

extern int ebofs_obj_remove(
		const lwfs_obj *obj);

extern lwfs_ssize ebofs_obj_read(
		const lwfs_obj *obj, 
		const lwfs_ssize src_offset, 
		void *dest, 
		const lwfs_ssize len);

extern lwfs_ssize ebofs_obj_write(
		const lwfs_obj *dest_obj, 
		const lwfs_ssize dest_offset, 
		void *src, 
		const lwfs_ssize len);

extern int ebofs_obj_getattr(
		const lwfs_obj *obj, 
		const lwfs_name name, 
		lwfs_attr *res);

extern int ebofs_obj_setattr(
		const lwfs_obj *obj, 
		const lwfs_name name, 
		const void *val,
		const int len);

extern int ebofs_obj_rmattr(
		const lwfs_obj *obj, 
		const lwfs_name name);

extern int ebofs_obj_stat(
		const lwfs_obj *obj, 
		lwfs_stat_data *data);

extern int ebofs_obj_trunc(
		const lwfs_obj *obj, 
		const lwfs_ssize size);

/*----------- SUPPORT FUNCTIONS ------------- */


#endif /* EBOFS_OBJ_H */
