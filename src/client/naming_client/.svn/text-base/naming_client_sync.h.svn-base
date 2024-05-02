/**  
 *   @file naming_clnt.h
 * 
 *   @brief Prototype definitions for the LWFS naming service.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   @version $Revision: 791 $
 *   @date $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#include "client/naming_client/naming_client.h"

#ifndef _LWFS_NAMING_SYNC_H_
#define _LWFS_NAMING_SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	/** 
	 * @brief Create a new namespace.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_create_namespace method creates a new namespace using the
	 * provided container ID. 
	 *
	 * @param svc    @input lwfs naming service.
	 * @param txn_id @input transaction ID.
	 * @param name   @input the name of the new namespace. 
	 * @param cid    @input the container ID to use for the namespace.  
	 * @param result @output handle to the new namespace. 
	 */
	extern int lwfs_create_namespace_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const char *name, 
			const lwfs_cid cid,
			lwfs_namespace *result);


	/** 
	 * @brief Remove a namespace.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_remove_namespace method removes an namespace.  
	 *
	 * @param txn_id @input transaction ID.
	 * @param name   @input the name of the namespace to remove.
	 * @param cap    @input the capability that allows the client to remove the namespace.
	 * @param result @output handle to the namespace that was removed. 
	 */
	extern int lwfs_remove_namespace_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const char *name, 
			const lwfs_cap *cap, 
			lwfs_namespace *result);


	/** 
	 * @brief Get a namespace.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_get_namespace method gets a namespace reference. 
	 *
	 * @param svc    @input lwfs naming service.
	 * @param name   @input the name of the namespace. 
	 * @param result @output handle to the new namespace. 
	 */
	extern int lwfs_get_namespace_sync(
			const lwfs_service *svc, 
			const char *name, 
			lwfs_namespace *result); 


	/** 
	 * @brief Get a list of namespaces that exist on the naming server.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_list_namespaces method returns the list of namespaces available 
	 * on the naming server. 
	 *
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param result @output_type space for the result.
	 */
	extern int lwfs_list_namespaces_sync(
			const lwfs_service *svc,
			lwfs_namespace_array *result); 

	
	/** 
	 * @brief Synchronous function to create a new 
	 * directory on a naming service.
	 *
	 * @ingroup naming_api
	 *  
	 * The \b lwfs_create_dir_sync method creates a new directory 
	 * entry using the provided container ID and adds the new 
	 * entry to the list of entries in the parent directory. 
	 *
	 * @param svc    @input_type Points to the naming service description.
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type the parent directory. 
	 * @param name   @input_type the name of the new directory. 
	 * @param cid    @input_type the container ID to use for the directory.  
	 * @param cap    @input_type the capability that allows the operation.
	 * @param result @output_type handle to the new directory entry. 
	 *
	 */
	extern int lwfs_create_dir_sync(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_cid cid,
			const lwfs_cap *cap, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Synchronous function to remove a directory 
	 *        from a naming service.
	 *
	 * @ingroup naming_api
	 * 
	 * The \b lwfs_remove_dir_sync method removes an empty 
	 * directory entry from the parent directory.  
	 *
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type parent of the dir to remove.
	 * @param name   @input_type the name of the directory to remove.
	 * @param cap    @input_type the capability that allows the client to modify 
	 *                      the parent directory.
	 * @param result @output_type handle to the directory entry that was removed. 
	 * @param req    @output_type the request handle (used to test for completion). 
	 *
	 * @note There is no requirement that the target directory be 
	 *       empty for the remove to complete successfully.  It is 
	 *       the responsibility of the file system implementation
	 *       to enforce such a policy. 
	 */

	extern int lwfs_remove_dir_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_cap *cap, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Create a new file.
	 * 
	 * @ingroup naming_api
	 *  
	 * The \b lwfs_create_file method creates a new file entry
	 * in the specified parent directory. Associated
	 * with the file is a single storage server object
	 * that the client creates prior to calling 
	 * \b lwfs_create_file method.  For example, the following
	 * code fragement illustrates how a file system implementation
	 * might use the \b lwfs_create_file method inside its own 
	 * file creation method (no error checking included): 
	 *
	 * @code
	 * int my_create(...) {
	 *     lwfs_process_id auth_id, ss_id;
	 *     lwfs_cap cid_cap, obj_cap, newfile_cap;
	 *     lwfs_cid new_cid;
	 *     lwfs_txn_id txn_id;
	 *     lwfs_obj dirent, filent;
	 * 
	 *     // initialize vars, get caps, etc.
	 *     ...
	 *
	 *     // create a new container id for the file 
	 *     lwfs_create_container(&auth_server, &cid_cap, &new_cid, &req); 
	 *     lwfs_wait(&req, &rc); 
	 *
	 *     // create a new object using the container id 
	 *     lwfs_create_object(&txn_id, &ss_id, &new_cid, &obj, &obj_cap, &filent, &req);
	 *     lwfs_wait(&req, &rc); 
	 *
	 *     // create a new file entry (associates obj with the file entry)
	 *     lwfs_create_file(&txn_id, &dirent, "myfile", &obj, &newfile_cap, 
	 *                      &filent, &req);
	 *     lwfs_wait(&req, &rc); 
	 * }
	 * @endcode
	 *
	 * @param svc    @input_type Points to the naming service decriptor. 
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type the parent directory.
	 * @param name   @input_type the name of the new file. 
	 * @param obj    @input_type the object to associate with the file. 
	 * @param cap    @input_type the capability that allows the operation.
	 * @param result @output_type the resulting file entry. 
	 * @param req    @output_type the request handle (used to test for completion)
	 * 
	 */
	extern int lwfs_create_file_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_obj *obj,
			const lwfs_cap *cap, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Create a link. 
	 * 
	 * @ingroup naming_api
	 *
	 * The \b lwfs_create_link method creates a new link entry in the 
	 * parent directory and associates the link with an existing 
	 * namespace entry (file or directory) with the link. 
	 * 
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type the parent directory. 
	 * @param name   @input_type the name of the new link.
	 * @param cap    @input_type the capability that allows the client 
	 *                      to modify the parent directory.
	 * @param target_parent @input_type the parent directory of the target. 
	 * @param target_name   @input_type the name of the target.
	 * @param target_cap    @input_type the capability that allows the client to access
	 *                      the target directory.
	 * @param result @output_type the new link entry. 
	 * @param req    @output_type the request handle (used to test for completion). 
	 * 
	 */
	extern int lwfs_create_link_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_cap *cap, 
			const lwfs_ns_entry *target_parent,
			const char *target_name,
			const lwfs_cap *target_cap, 
			lwfs_ns_entry *result);

	/** 
	 * @brief Remove a link.
	 *
	 * @ingroup naming_api
	 *
	 * This method removes a link entry from the specified parent
	 * directory. It does not modify or remove the entry 
	 * referenced by the link. 
	 *
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type the parent directory.
	 * @param name @input_type the name of the link entry.
	 * @param cap @input_type the capability that allows us to 
	 *            remove the link from the parent.
	 * @param result @output_type the removed entry. 
	 * @param req @output_type the request handle (used to test for completion). 
	 * 
	 */
	extern int lwfs_unlink_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent,
			const char *name, 
			const lwfs_cap *cap,
			lwfs_ns_entry *result);


	/** 
	 * @brief Find an entry in a directory.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_lookup method finds an entry by name and acquires 
	 * a lock for the entry. We incorporated an (optional) implicit lock into 
	 * the method to remove a potential race condition where an outside
	 * process could modify the entry between the time the lookup call
	 * found the entry and the client performs an operation on the entry. 
	 * The resulting lock_id is encoded in the \ref lwfs_obj
	 * data structure.
	 * 
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param txn_id @input_type the transaction ID (used only to acquire the lock). 
	 * @param parent @input_type the parent directory.
	 * @param name   @input_type the name of the entry to look up. 
	 * @param lock_type @input_type the type of lock to get on the entry. 
	 * @param cap    @input_type the capability that allows us read from the directory.
	 * @param result @output_type the entry.
	 * @param req @output_type the request handle (used to test for completion).
	 */
	extern int lwfs_lookup_sync(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id, 
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_lock_type lock_type, 
			const lwfs_cap *cap, 
			lwfs_ns_entry *result);


	/** 
	 * @brief Read the contents of a directory.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_list_dir method returns the list of entries in the 
	 * parent directory. 
	 *
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param parent @input_type the parent directory. 
	 * @param cap    @input_type the capability that allows us to read the contents of the parent.
	 * @param result @output_type space for the result.
	 * @param req    @output_type the request handle (used to test for completion). 
	 * 
	 * @remarks <b>Ron (12/01/2004):</b> I removed the transaction ID from this call
	 *          because it does not change the state of the system and we do 
	 *          not acquire locks on the entries returned. 
	 *
	 * @remark <b>Ron (12/7/2004):</b> I wonder if we should add another argument 
	 *         to filter to results on the server.  For example, only return 
	 *         results that match the provided regular expression. 
	 */
	extern int lwfs_list_dir_sync(
			const lwfs_service *svc, 
			const lwfs_ns_entry *parent,
			const lwfs_cap *cap,
			lwfs_ns_entry_array *result);


#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

