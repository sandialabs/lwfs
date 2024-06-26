/**  
 *   @file naming_server.h
 * 
 *   @brief Prototype definitions for server-side methods for 
 *   the LWFS naming service.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @version $Revision: 1189 $
 *   @date $Date: 2007-02-07 14:30:35 -0700 (Wed, 07 Feb 2007) $
 */

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/naming_common/naming_args.h"
#include "common/naming_common/naming_debug.h"
#include "common/naming_common/naming_xdr.h"
#include "common/naming_common/naming_opcodes.h"

#include "server/rpc_server/rpc_server.h"
#include "client/authr_client/authr_client.h"
#include "client/storage_client/storage_client.h"

#ifndef _LWFS_NAMING_SRVR_H_
#define _LWFS_NAMING_SRVR_H_

#ifdef __cplusplus
extern "C" {
#endif

//extern const lwfs_svc_op naming_op_array[]; 

#if defined(__STDC__) || defined(__cplusplus)

	/**
	 * @brief Initialize a naming server.
	 */
	extern int naming_server_init(
		const char *dp_path,
		const lwfs_bool db_clear,
		const lwfs_bool db_recover,
		const lwfs_service *authr_svc, 
		lwfs_service *svc);

	extern int naming_server_fini(lwfs_service *naming_svc);

	/**
	 * @brief Return the array of operation descriptions supported
	 *  by the naming service. 
	 *
	 *  This function is used when starting the naming service or 
	 *  starting a thread to run the naming service. 
	 */
	extern const lwfs_svc_op *lwfs_naming_op_array(); 

	/**
	 * @brief Create a new namespace.
	 *
	 * @param caller @input the process that called this function.
	 * @param args   @input the arguments structure used by this function.
	 * @param data_addr @input not used.
	 * @param result  @output the resulting object structure.
	 *
	 * The \b lwfs_create_namespace method creates a new namespace using the
	 * provided container ID.
	 *
	 * @remark <b>Todd (12/13/2006):</b>  Notice that creating a namespace
	 * does not requires a capability.  The namespace is created in the
	 * \ref lwfs_cid "container ID" given.  Normal access control policies
	 * apply.
	 */
	extern int naming_create_namespace(
			const lwfs_remote_pid *caller,
			const lwfs_create_namespace_args *args,
			const lwfs_rma *data_addr,
			lwfs_namespace *result);
	
	/**
	 * @brief Remove a namespace.
	 *
	 * @param caller @input the process that called this function.
	 * @param args   @input the arguments structure used by this function.
	 * @param data_addr @input not used.
	 * @param result  @output the resulting object structure.
	 *
	 * The \b lwfs_remove_namespace method removes an empty namespace.
	 *
	 * @note There is no requirement that the target namespace be
	 *       empty for the remove to complete successfully.  It is
	 *       the responsibility of the file system implementation
	 *       to enforce such a policy.
	 */
	extern int naming_remove_namespace(
			const lwfs_remote_pid *caller,
			const lwfs_remove_namespace_args *args,
			const lwfs_rma *data_addr,
			lwfs_namespace *result);
	
	/**
	 * @brief Find an entry in a directory.
	 *
	 * The \b lwfs_lookup method finds an entry by name and acquires
	 * a lock for the entry. We incorporated an implicit lock into
	 * the method to remove a potential race condition where an outside
	 * process could modify the entry between the time the lookup call
	 * found the entry and the client performs an operation on the entry.
	 */
	extern int naming_get_namespace(
			const lwfs_remote_pid *caller,
			const lwfs_get_namespace_args *args,
			const lwfs_rma *data_addr,
			lwfs_namespace *result);
	
	/**
	 * @brief Get a list of namespaces that exist on this naming server.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_list_namespaces method returns the list of namespaces available
	 * on this naming server.
	 *
	 * @remark <b>Todd (12/14/2006):</b> This comment from \b lwfs_list_dir applies
	 *         here as well - "I wonder if we should add another argument
	 *         to filter to results on the server.  For example, only return
	 *         results that match the provided regular expression".
	 */
	extern int naming_list_namespaces(
			const lwfs_service *svc,
			lwfs_namespace_array *result);

	/** 
	 * @brief Create a new directory.
	 *
	 * The \ref lwfs_create_dir method creates a new directory entry using the
	 * provided container ID and adds the new entry to the list 
	 * of entries in the parent directory. 
	 *
	 * @remark <b>Ron (11/30/2004):</b>  I'm not sure we want the client to supply
	 *         the container ID for the directory.  If the naming service generates
	 *         the container ID, it can have some control over the access permissions
	 *         authorized to the client.  For example, the naming service may allow
	 *         clients to perform an add_file operation (through the nameservice API), 
	 *         but not grant generic permission to write to the directory object. 
	 *
	 * @remark <b>Ron (12/04/2004):</b> Barney addressed the above concern by 
	 *         mentioning that assigning a container ID to an entry does not 
	 *         necessarily mean that the data associated with the entry (e.g.,
	 *         the list of other entries in a directory) will be created using the
	 *         provided container.  The naming service could use a private container 
	 *         ID for the data.  That way, we can use the provided container ID 
	 *         for the access policy for namespace entries, but at the same
	 *         time, only allow access to the entry data through the naming API. 
	 */
	extern int naming_create_dir(
			const lwfs_remote_pid *caller, 
			const lwfs_create_dir_args *args,
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Remove a directory.
	 *
	 * 
	 * The \ref lwfs_remove_dir method removes an empty 
	 * directory entry from the parent directory.  
	 *
	 * @remark <b>Ron (11/30/2004):</b> Is it enough to only require a capability 
	 *          that allows the client to modify the contents of the parent 
	 *          directory, or should we also have a capability that allows 
	 *          removal of a particular directory entry? 
	 */
	extern int naming_remove_dir(
			const lwfs_remote_pid *caller, 
			const lwfs_remove_dir_args *args,
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Create a new file.
	 * 
	 *  
	 * The \ref lwfs_create_file method creates a new file entry
	 * in the specified parent directory. 
	 */
	extern int naming_create_file(
			const lwfs_remote_pid *caller, 
			const lwfs_create_file_args *args,
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Remove a file. 
	 *
	 * \ref lwfs_remove_file removes a file entry from the 
	 * specified parent directory. 
	 *
	 * @note
	 * This method does not remove the object associated with the 
	 * file entry.  It is the responsibility of the file system
	 * implementation to remove all objects associated with the
	 * file. 
	 *
	 *
	 */
	extern int naming_remove_file(
			const lwfs_remote_pid *caller, 
			const lwfs_remove_file_args *args,
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result);


	/** 
	 * @brief Create a link. 
	 * 
	 *
	 * The \b lwfs_create_link method creates a new link entry in the 
	 * parent directory and associates the link with an existing 
	 * namespace entry (file or directory) with the link. 
	 */
	extern int naming_create_link(
			const lwfs_remote_pid *caller, 
			const lwfs_create_link_args *args,
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Remove a link.
	 *
	 *
	 * This method removes a namepace entry from the 
	 * specified parent directory. It does not modify or 
	 * remove the entry referenced by the link. 
	 *
	 * @remark <b>Ron (12/01/2004) </b> See the remarks for \ref lwfs_remove_dir. 
	 */
	extern int naming_unlink(
			const lwfs_remote_pid *caller, 
			const lwfs_unlink_args *args,
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result); 

	/** 
	 * @brief Find an entry in a directory.
	 *
	 * The \b lwfs_lookup method finds an entry by name and acquires 
	 * a lock for the entry. We incorporated an implicit lock into 
	 * the method to remove a potential race condition where an outside
	 * process could modify the entry between the time the lookup call
	 * found the entry and the client performs an operation on the entry. 
	 * 
	 */
	extern int naming_lookup(
			const lwfs_remote_pid *caller, 
			const lwfs_lookup_args *args, 
			const lwfs_rma *data_addr, 
			lwfs_ns_entry *result);


	/** 
	 * @brief Read the contents of a directory.
	 *
	 *
	 * The \b lwfs_list_dir method returns the list of entries in the 
	 * parent directory. 
	 * 
	 * @remarks <b>Ron (12/01/2004):</b> I removed the transaction ID from this call
	 *          because it does not change the state of the system and we do 
	 *          not acquire locks on the entries returned. 
	 *
	 * @remark <b>Ron (12/7/2004):</b> I wonder if we should add another argument 
	 *         to filter to results on the server.  For example, only return 
	 *         results that match the provided regular expression. 
	 */
	extern int naming_list_dir(
			const lwfs_remote_pid *caller, 
			const lwfs_list_dir_args *args, 
			const lwfs_rma *data_addr, 
			lwfs_ns_entry_array *result);

	extern int lwfs_list_all();

	/**
	 * @brief Get the attributes from a namespace entry. 
	 *
	 */
	extern int naming_stat(
			const lwfs_remote_pid *caller,
			const lwfs_name_stat_args *args,
			const lwfs_rma *data_addr,
			lwfs_stat_data *res); 

#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

