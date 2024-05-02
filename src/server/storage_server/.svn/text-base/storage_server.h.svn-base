/**  
 *   @file ss_srvr.h
 * 
 *   @brief Protype definitions for the storage server API. 
 * 
 *   @author Bill Lawry (wflawry@sandia.gov)
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 */

#ifndef _SS_SRVR_H_
#define _SS_SRVR_H_

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/storage_common/ss_args.h"
#include "common/storage_common/ss_opcodes.h"
#include "common/storage_common/ss_debug.h"
#include "common/storage_common/ss_trace.h"
#include "common/storage_common/ss_xdr.h"

#include "server/rpc_server/rpc_server.h"
#include "server/authr_server/authr_server.h"
#include "client/authr_client/authr_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SS_NUM_BUFS 10
#define DEFAULT_SS_BUFSIZE 1048576

	/**
	 * @brief Enumerate the differnt types of supported 
	 * backend libraries. 
	 */
	enum ss_iolib {
		SS_SYSIO = 0,
		SS_AIO = 1,
		SS_SIMIO = 2,
		SS_EBOFS = 3
	};


	/** 
	 * @brief A structure that holds function pointers
     *        to the object I/O library. 
     */
    struct obj_funcs {

        /** @brief Check for the existence of an object. */
        lwfs_bool (*exists)(const lwfs_obj *);

        /** @brief Create a persistent object. */
        int (*create)(const lwfs_obj *);

        /** @brief Remove a stored object. */
        int (*remove)(const lwfs_obj *);

        /** @brief Read from a stored object. */
        lwfs_ssize (*read)(
                const lwfs_obj *, 
                const lwfs_ssize,
                void *,
                const lwfs_ssize);

        /** @brief Write to a stored object. */
        lwfs_ssize (*write)(
                const lwfs_obj *, 
                const lwfs_ssize,
                void *,
                const lwfs_ssize);

        /** @brief List the attributes of an object. */
        int (*listattrs)(
		const lwfs_obj *,
		lwfs_name_array *names);

        /** @brief Get attributes of an object. */
        int (*getattrs)(
		const lwfs_obj *,
		const lwfs_name_array *names,
		lwfs_attr_array *attrs);

        /** @brief Set attributes of an object. */
        int (*setattrs)(
		const lwfs_obj *,
		const lwfs_attr_array *attrs);

        /** @brief Remove attributes from an object. */
        int (*rmattrs)(
		const lwfs_obj *,
		const lwfs_name_array *names);

        /** @brief Get attributes of an object. */
        int (*getattr)(
		const lwfs_obj *,
		const lwfs_name name,
		lwfs_attr *attr);

        /** @brief Set attributes of an object. */
        int (*setattr)(
		const lwfs_obj *,
		const lwfs_name name,
		const void *value,
		const int size);

        /** @brief Remove attributes from an object. */
        int (*rmattr)(
		const lwfs_obj *,
		const lwfs_name name);

        /** @brief Stat an object. */
	int (*stat) (
		const lwfs_obj *obj,
		lwfs_stat_data *data);


        /** @brief Sync object data to storage. */
        int (*fsync)(
                const lwfs_obj *obj);

        /** @brief Truncate an object. */
        int (*trunc)(
                const lwfs_obj *,
                const lwfs_ssize);

    };


#if defined(__STDC__) || defined(__cplusplus)

    extern const lwfs_svc_op *lwfs_ss_op_array();

	extern int storage_server_init(
			const char *db_path,
			const lwfs_bool db_clear,
			const lwfs_bool db_recover,
			const char *iolib,
			const char *root,
			const int num_bufs,
			const lwfs_size bufsize, 
			const lwfs_service *a_svc,
			lwfs_service *svc); 

	extern int storage_server_fini(
			const lwfs_service *svc);

    /***** the registered functions *****/

    extern int ss_create_obj(
            const lwfs_remote_pid *caller, 
            const ss_create_obj_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_remove_obj(
            const lwfs_remote_pid *caller, 
            const ss_remove_obj_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_read(
            const lwfs_remote_pid *caller, 
            const ss_read_args *args,
            const lwfs_rma *data_addr,
            lwfs_size *res);

    extern int ss_write(
            const lwfs_remote_pid *caller, 
            const ss_write_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_fsync(
            const lwfs_remote_pid *caller, 
            const ss_fsync_args *args, 
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_listattrs(
            const lwfs_remote_pid *caller, 
            const ss_listattrs_args *args, 
            const lwfs_rma *data_addr,
            lwfs_name_array *res);

    extern int ss_getattrs(
            const lwfs_remote_pid *caller, 
            const ss_getattrs_args *args,
            const lwfs_rma *data_addr,
            lwfs_attr_array *res);

    extern int ss_setattrs(
            const lwfs_remote_pid *caller, 
            const ss_setattrs_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_rmattrs(
            const lwfs_remote_pid *caller, 
            const ss_rmattrs_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_getattr(
            const lwfs_remote_pid *caller, 
            const ss_getattr_args *args,
            const lwfs_rma *data_addr,
            lwfs_attr *res);

    extern int ss_setattr(
            const lwfs_remote_pid *caller, 
            const ss_setattr_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_rmattr(
            const lwfs_remote_pid *caller, 
            const ss_rmattr_args *args,
            const lwfs_rma *data_addr,
            void *res);

    extern int ss_stat(
            const lwfs_remote_pid *caller, 
        	const ss_stat_args *args, 
            const lwfs_rma *data_addr,
            lwfs_stat_data *res);

    extern int ss_truncate(
            const lwfs_remote_pid *caller, 
            const ss_truncate_args *args,
            const lwfs_rma *data_addr,
            void *res);

#else /* K&R C */

#endif



#ifdef __cplusplus
}
#endif

#endif 

