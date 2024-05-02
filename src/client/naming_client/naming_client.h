/**  
 *   @file naming_clnt.h
 * 
 *   @brief Prototype definitions for the LWFS naming service.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   @version $Revision: 1359 $
 *   @date $Date: 2007-04-24 11:30:48 -0600 (Tue, 24 Apr 2007) $
 */
#include "config.h"

/*#include "portals3.h"*/

#include "common/types/types.h"
#include "common/naming_common/naming_args.h"
#include "common/naming_common/naming_debug.h"
#include "common/naming_common/naming_xdr.h"
#include "common/naming_common/naming_opcodes.h"

#include "client/rpc_client/rpc_client.h"
#include "client/authr_client/authr_client.h"
#include "client/storage_client/storage_client.h"

#ifndef _LWFS_NAMING_H_
#define _LWFS_NAMING_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	/**
	 *   @addtogroup naming_api
	 * 
	 *   The LWFS naming service provides methods that 
	 *   allow a client file system to organize storage server 
	 *   objects into a directory-based namespace. 
	 *   
	 *   The namespace consists of a three types of named 
	 *   \ref lwfs_entry "entries": directories, files, and links.
	 *
	 *   - A \em directory entry contains a list of other
	 *   entries (files, directories, and links). Directories 
	 *   represent the internal nodes of the tree structure that 
	 *   composes the LWFS namespace. 

	 *   - A \em file entry points to a single storage server 
	 *   object (file object) that the client file system uses 
	 *   to store any metadata, or data associated with the file.
	 *   The LWFS imposes no structure on the contents of the 
	 *   file object. 
	 *
	 *   - A \em link points to another entry in the namespace. 
	 *   
	 *   For access control, each entry belongs to a 
	 *   \ref lwfs_cid "container" 
	 *   and is subject to the access-control policies defined 
	 *   for that container. In the case of files, the container 
	 *   id of the file entry is the same as the container ID of 
	 *   the object pointed to by the file. Similarly, a link entry
	 *   has the same container ID as the entry it references. 
	 *
	 *   The API for the LWFS naming service includes convenience 
	 *   methods to extract the name and container ID from an entry, 
	 *   methods that allow a client to synchronize access to namespace 
	 *   entries, and methods to create, remove, lookup, and list 
	 *   entries in a particular directory of the namespace. 
	 *
	 *   \par Example implementations (to fill in later)
	 *      - create directory, 
	 *      - create file, 
	 *      - directory traversal, 
	 *      - ...
	 */

	/* ---------------------- Convenience functions --------------- */

	/** 
	 * @brief Return the container ID of an entry. 
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_get_cid method returns the 
	 * \ref lwfs_cid "container ID" of an entry. 
	 *
	 * @param obj @input_type the object to look up.
	 */
	extern lwfs_cid lwfs_get_cid(
			const lwfs_obj *obj);

	/** 
	 * @brief Return the entry type.
	 *
	 * @ingroup naming_api
	 *
	 * The \b lwfs_get_etype method returns the 
	 * \ref lwfs_entry_type "type" of an entry. 
	 *
	 * @param obj @input_type the object to look up.
	 */
	extern int lwfs_get_type(
			const lwfs_obj *obj);


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
	 * @param req    @output the request handle (used to test for completion)
	 *
	 * @remark <b>Todd (12/13/2006):</b>  Notice that creating a namespace 
	 * does not requires a capability.  The namespace is created in the 
	 * \ref lwfs_cid "container ID" given.  Normal access control policies 
	 * apply.
	 */
	extern int lwfs_create_namespace(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const char *name, 
			const lwfs_cid cid,
			lwfs_namespace *result,
			lwfs_request *req); 


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
	 * @param req    @output the request handle (used to test for completion). 
	 *
	 * @note There is no requirement that the target namespace be 
	 *       empty for the remove to complete successfully.  It is 
	 *       the responsibility of the file system implementation
	 *       to enforce such a policy. 
	 */
	extern int lwfs_remove_namespace(
			const lwfs_service *svc, 
			const lwfs_txn *txn_id,
			const char *name, 
			const lwfs_cap *cap, 
			lwfs_namespace *result,
			lwfs_request *req);


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
	 * @param req    @output the request handle (used to test for completion)
	 *
	 * @remark <b>Todd (12/13/2006):</b>  Notice that creating a namespace 
	 * does not requires a capability.  The namespace is created in the 
	 * \ref lwfs_cid "container ID" given.  Normal access control policies 
	 * apply.
	 */
	extern int lwfs_get_namespace(
			const lwfs_service *svc, 
			const char *name, 
			lwfs_namespace *result,
			lwfs_request *req); 


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
	 * @param req    @output_type the request handle (used to test for completion). 
	 * 
	 * @remark <b>Todd (12/14/2006):</b> This comment from \b lwfs_list_dir applies 
	 *         here as well - "I wonder if we should add another argument 
	 *         to filter to results on the server.  For example, only return 
	 *         results that match the provided regular expression". 
	 */
	extern int lwfs_list_namespaces(
			const lwfs_service *svc,
			lwfs_namespace_array *result,
			lwfs_request *req); 

	
	/** 
	 * @brief Create a new directory.
	 *
	 * @ingroup naming_api
	 *  
	 * The \b lwfs_create_dir method creates a new directory entry using the
	 * provided container ID and adds the new entry to the list 
	 * of entries in the parent directory. 
	 *
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type the parent directory. 
	 * @param name   @input_type the name of the new directory. 
	 * @param cid    @input_type the container ID to use for the directory.  
	 * @param cap    @input_type the capability that allows the operation.
	 * @param result @output_type handle to the new directory entry. 
	 * @param req    @output_type the request handle (used to test for completion)
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
	extern int lwfs_create_dir(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_cid cid,
			const lwfs_cap *cap, 
			lwfs_ns_entry *result,
			lwfs_request *req); 

	/** 
	 * @brief Remove a directory.
	 *
	 * @ingroup naming_api
	 * 
	 * The \b lwfs_remove_dir method removes an empty 
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
	 * 
	 * @remark <b>Ron (11/30/2004):</b> Is it enough to only require a capability 
	 *          that allows the client to modify the contents of the parent 
	 *          directory, or should we also have a capability that allows 
	 *          removal of a particular directory entry? 
	 * @remark  <b>Ron (11/30/2004):</b> The interfaces to remove files and dirs
	 *          reference the target file/dir by name.  Should we also have methods
	 *          to remove a file/dir by its \ref lwfs_ns_entry handle?  
	 *          This is essentially what Rolf did in the original implementation. 
	 */
	extern int lwfs_remove_dir(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_cap *cap, 
			lwfs_ns_entry *result,
			lwfs_request *req);


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
	 * @param svc    @input_type Points to the naming service descriptor. 
	 * @param txn_id @input_type transaction ID.
	 * @param parent @input_type the parent directory.
	 * @param name   @input_type the name of the new file. 
	 * @param obj    @input_type the object to associate with the file. 
	 * @param cap    @input_type the capability that allows the operation.
	 * @param result @output_type the resulting file entry. 
	 * @param req    @output_type the request handle (used to test for completion)
	 * 
	 */
	extern int lwfs_create_file(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_obj *obj,
			const lwfs_cap *cap, 
			lwfs_ns_entry *result, 
			lwfs_request *req);



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
	extern int lwfs_create_link(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_cap *cap, 
			const lwfs_ns_entry *target_parent,
			const char *target_name,
			const lwfs_cap *target_cap, 
			lwfs_ns_entry *result,
			lwfs_request *req);


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
	extern int lwfs_unlink(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_ns_entry *parent,
			const char *name, 
			const lwfs_cap *cap,
			lwfs_ns_entry *result,
			lwfs_request *req); 


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
	extern int lwfs_lookup(
			const lwfs_service *svc,
			const lwfs_txn *txn_id, 
			const lwfs_ns_entry *parent, 
			const char *name, 
			const lwfs_lock_type lock_type, 
			const lwfs_cap *cap, 
			lwfs_ns_entry *result,
			lwfs_request *req);


	/*
	 * @remarks <b>Ron (12/01/2004):</b> We've considered a much more complex 
	 *          version of this method that incorporates implicit locking and 
	 *          unlocking of the parent directory and the resulting entry. 
	 *          We considered the traversing a directory could require many . 
	 *          consider the code fragment associated with traversing 
	 * 
	 *    @code
	 *        // traverses from the root, returns the child entry
	 *        int traverse(txn_id *txn_id, 
	 *                     lwfs_obj *root, 
	 *                     char *path, 
	 *                     lwfs_credential *cred, 
	 *                     lwfs_obj *child, 
	 *                     lwfs_lock_type child_lock_type,
	 *                     lwfs_lock_id *child_lock) 
	 *        {
	 *            char **names; 
	 *            int len,i,rc; 
	 *            lwfs_obj *parent; 
	 *            lwfs_cid cid; 
	 *            static lwfs_op_list lookup_ops = {LWFS_LOOKUP, NULL};
	 *            lwfs_cap_list cap_list; 
	 *            lwfs_lock_id parent_lock, 
	 * 
	 *            // parse path into a list of strings
	 *            parse_path(path, &names, &len); 
	 *
	 *            parent = root; 

	 *            // lock parent so nobody removes it before we do our lookup
	 *            lwfs_lock_entry(txn_id, parent, LWFS_READ_LOCK, &parent_lock, &req);  
	 *            lwfs_wait(&req, &rc);
	 *
	 *            for (i=0; i<len; i++) {
	 *
	 *               // get the container ID of the parent (for the getcaps call)
	 *               cid = lwfs_get_cid(parent); 
	 *
	 *               // get cap that allow us to do the lookup
	 *               lwfs_getcaps(cid, &lookup_ops, cred, &cap_list, &req); 
	 *               lwfs_wait(&req, &rc); 
	 *
	 *               // lookup the next entry
	 *               lwfs_lookup(parent, name++, cap_list.cap, child, &req);
	 *               lwfs_wait(&req, &rc); 
	 *
	 *               // lock the child entry 
	 *               if (i < len-1) {   // if not finished traversing path
	 *               	lwfs_lock_entry(txn_id, child, LWFS_READ_LOCK); 
	 *               	lwfs_wait(&req, &rc); 
	 *               }
	 *               else {
	 *                  lwfs_lock_entry(txn_id, child, child_lock_type, child_lock, &req);
	 *               	lwfs_wait(&req, &rc); 
	 *               }
	 *               
	 *               // unlock parent directory
	 *               lwfs_unlock_entry(txn_id, parent_lock, &req); 
	 *               lwfs_wait(&req, &rc); 
	 *
	 *               // child becomes the parent for the next lookup (if needed)
	 *               parent = &child; 
	 *            }
	 *        }
	 *    @endcode
	 *
	 *    There is still a small window of opportunity where an outside entity
	 *    can modify or remove a looked-up entry before the client has the chance
	 *    to lock the entry.  
	 */


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
	extern int lwfs_list_dir(
			const lwfs_service *svc,
			const lwfs_ns_entry *parent,
			const lwfs_cap *cap,
			lwfs_ns_entry_array *result,
			lwfs_request *req); 

	
#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

