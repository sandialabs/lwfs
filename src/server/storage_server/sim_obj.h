#ifndef SIM_OBJ_H
#define SIM_OBJ_H

/* see ss_obj.c for description of this object api */

#include "storage_server.h"
#include <unistd.h> 
#include <pthread.h>


#define ROOT_DIRECTORY  "ss_test_dir"
#define SS_OBJ_HASH_SIZE (1024) /* must be power of 2 */


/*----------- THE CORE FUNCTIONS ------------- */

extern int sim_obj_init(
        const char *root, 
        struct obj_funcs *obj_funcs);

extern int sim_obj_fini();

extern lwfs_bool sim_obj_exists(const lwfs_obj *obj);

extern int sim_obj_fsync(const lwfs_obj *obj);

extern int sim_obj_create(
		const lwfs_obj *obj);

extern int sim_obj_remove(
		const lwfs_obj *obj);

extern lwfs_ssize sim_obj_read(
		const lwfs_obj *obj, 
		const lwfs_ssize src_offset, 
		void *dest, 
		const lwfs_ssize len);

extern lwfs_ssize sim_obj_write(
		const lwfs_obj *dest_obj, 
		const lwfs_ssize dest_offset, 
		void *src, 
		const lwfs_ssize len);

extern int sim_obj_listattrs(
		const lwfs_obj *obj, 
		lwfs_name_array *names);

extern int sim_obj_getattrs(
		const lwfs_obj *obj, 
		const lwfs_name_array *names,
		lwfs_attr_array *attrs);

extern int sim_obj_setattrs(
		const lwfs_obj *obj, 
		const lwfs_attr_array *attrs);

extern int sim_obj_rmattrs(
		const lwfs_obj *obj, 
		const lwfs_name_array *names);

extern int sim_obj_getattr(
		const lwfs_obj *obj, 
		const lwfs_name name,
		lwfs_attr *res);

extern int sim_obj_setattr(
		const lwfs_obj *obj, 
		const lwfs_name name,
		const void *value,
		const int size);

extern int sim_obj_rmattr(
		const lwfs_obj *obj, 
		const lwfs_name name);
	
extern int sim_obj_stat(
		const lwfs_obj *obj, 
		lwfs_stat_data *val);

extern int sim_obj_trunc(
		const lwfs_obj *obj, 
		const lwfs_ssize size);

#endif /* SIM_OBJ_H */
