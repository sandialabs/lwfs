/**  
 *   @file storage_client_sync.h
 * 
 *   @brief Protype definitions for the synchronous storage server API. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */


#include "client/storage_client/storage_client.h"

#ifndef _SS_CLNT_SYNC_H_
#define _SS_CLNT_SYNC_H_


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)


	/** 
	 * @brief Create a new object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_create_obj</tt> function creates a new object 
	 * on the specified storage server.
	 *
	 * @param svc @input_type Points to the service structure for the target 
	 *                        storage server. 
	 * @param txn @input_type Points to the transaction (NULL if no transaction).
	 * @param cid @input_type The container ID to use for the new object. 
	 * @param cap @input_type Points to the capability that allows creation of an 
	 *                        object on the storage server. 
	 * @param result @output_type If successful, points to a new object 
	 *                            reference. Undefined otherwise. 
	 * @param req @output_type Points to the request structure. 
	 *
	 * @remark The container ID argument is redundant because the 
	 *       capability structure has the container ID encoded in it. 
	 *       Should we remove it from the parameter list. 
	 *       
	 *
     * @return <b>\ref LWFS_OK</b> Success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the 
	 *                                  communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NOENT</b> The container id does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_create_obj_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap);


	/** 
	 * @brief Remove an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_remove_obj</tt> function removes a remote object
	 * from a specified storage server. 
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction).
	 * @param obj @input_type Points to the object to remove. 
	 * @param cap @input_type Points to the capability that allows the holder to remove
	 *            the specified object from the storage server. 
	 * @param req @output_type Points to the request structure. 
	 *
     * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the 
	 *                                  communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_remove_obj_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap);

	/** 
	 * @brief Read from an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_read</tt> function reads a range of 
	 * bytes from an object on a remote storage server. 
	 *
	 * @param txn  @input_type Points to the transaction ID (NULL if no transaction).
	 * @param src_obj @input_type Points to the source object. 
	 * @param src_offset @input_type Where to start reading. 
	 * @param buf @input_type Points to the local memory reserved for the data. 
	 * @param len @input_type Indicates the number of bytes to try to read. 
	 * @param cap @input_type Points to the capability that allows the holder to read
	 *                   from the specified remote object.
	 * @param result @output_type Indicates the number of bytes actually read. 
	 * @param req @output_type Points to the request handle (used to test for completion). 
	 *
	 *
     * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the 
	 *                                  communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The source object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_read_sync(
			const lwfs_txn *txn,
			const lwfs_obj *src_obj, 
			const lwfs_size src_offset, 
			void *buf, 
			const lwfs_size len, 
			const lwfs_cap *cap, 
			lwfs_size *result);

	/** 
	 * @brief Write to an object. 
	 *
	 * @ingroup ss_api 
	 *  
	 * The <tt>\ref lwfs_write</tt> function writes a contiguous array of bytes 
	 * to a remote object on a storage server. 
	 *
	 * @param txn  @input_type Points to the transaction ID (NULL if no transaction).
	 * @param dest_obj @input_type Points to the destination object. 
	 * @param dest_offset @input_type Indicates the initial offset on the remote object (i.e., 
	 *                           where to start writing). 
	 * @param buf the @input_type Points to the local buffer that holds the data to be 
	 *                       written.  This buffer must not be modified until the 
	 *                       operation completes (i.e., until the \ref lwfs_wait 
	 *                       function returns \ref LWFS_OK). 
	 * @param len the @input_type Indicates the number of bytes to write. 
	 * @param cap the @input_type Points to the capability that allows the holder to write
	 *                       to the specified storage server object. 
	 * @param req the @output_type Points to the request handle (used to test for completion). 
	 *
     * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The destination object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_write_sync(
			const lwfs_txn *txn,
			const lwfs_obj *dest_obj, 
			const lwfs_size dest_offset, 
			const void *buf, 
			const lwfs_size len, 
			const lwfs_cap *cap);


	/** 
	 * @brief Sync to an object. 
	 *
	 * @ingroup ss_api 
	 *  
	 * The <tt>\ref lwfs_fsync</tt> function is similar to the UNIX command ``fsync''. 
	 * It completes when the storage server flushes all data waiting to be written to 
	 * stable storage.  
	 *
	 * @param txn  @input_type   Points to the transaction ID (NULL if no transaction).
	 * @param obj     @input_type   Points to the object of interest. 
	 * @param cap     @input_type   Points to the capability that allows the holder to perform
	 *                              to the fsync operation. 
	 * @param req     @output_type  Points to the request handle (used to test for completion). 
	 *
     * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_fsync_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap);

	/** 
	 * @brief Get the names of the extended attributes of an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_listattrs</tt> function fetches the names of the attributes
	 * (defined in \ref lwfs_obj_attr) of a specified storage server object.
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   get the attributes for objects in a specified container. 
	 * @param result @output_type Points to the array of attribute names. 
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_listattrs_sync(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_name_array *res); 


	/** 
	 * @brief Get the named extended attributes of an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_getattrs</tt> function fetches the attributes
	 * (defined in \ref lwfs_obj_attr) of a specified storage server object.
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param names @input_type The names of the attributes to get. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   get the attributes for objects in a specified container. 
	 * @param result @output_type Points to an array of attributes. 
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_getattrs_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name_array *names, 
			const lwfs_cap *cap, 
			lwfs_attr_array *result); 


	/** 
	 * @brief Set extended attributes of an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_setattrs</tt> associates an extended attribute to
	 * an object. 
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param attrs @input_type The array of attributes. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   set the attributes for objects in a specified container. 
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_setattrs_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_attr_array *attrs, 
			const lwfs_cap *cap); 


	/** 
	 * @brief Remove extended attributes from an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_rmattrs</tt> function removes an attribute
	 * from a storage server object.
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param names @input_type Array of names of the attributes to remove. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   remove the attributes for objects in a specified container. 
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_rmattrs_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name_array *names, 
			const lwfs_cap *cap); 

	/** 
	 * @brief Get a named attribute of an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The \b lwfs_getattr method fetches a named attribute
	 * (defined in \ref lwfs_obj_attr) of a 
	 * specified storage server object.
	 *
	 * @param txn_id @input transaction ID.
	 * @param obj @input the object. 
	 * @param name @input the name of the attr to get.
	 * @param cap @input the capability that allows the operation.
	 *
	 * @returns the object attribute in the result field.
	 */
	extern int lwfs_getattr_sync(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj, 
			const lwfs_name name, 
			const lwfs_cap *cap, 
			lwfs_attr *attr);

	/** 
	 * @brief Set extended attribute of an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_setattr</tt> associates an extended attribute to
	 * an object. 
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param attr @input_type The attribute. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   set the attributes for objects in a specified container. 
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_setattr_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name name,
			const void *value,
			const int size, 
			const lwfs_cap *cap); 


	/** 
	 * @brief Remove extended attribute from an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_rmattr</tt> function removes an attribute
	 * from a storage server object.
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param name @input_type Name of the attribute to remove. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   remove the attributes for objects in a specified container. 
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_rmattr_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name name, 
			const lwfs_cap *cap); 

	/** 
	 * @brief Get information about an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_stat</tt> returns information about an 
	 * object--similar to the UNIX stat() call. 
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   stat the object. 
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_stat_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_stat_data *data);

	/** 
	 * @brief Set the size of an object. 
	 *  
	 * The \b lwfs_set_size method fixes the size of an 
	 * object to exactly \em size bytes.  If the new size 
	 * is greater than the current size, the remaning bytes
	 * are filled with zeros. 
	 *
	 * @param txn @input_type transaction ID.
	 * @param obj @input_type the object. 
	 * @param size @input_type the new size of the object. 
	 * @param cap @input_type the capability that allows the operation.
	 * @param req @output_type the request handle (used to test for completion)
	 *
         * @return <b>\ref LWFS_OK</b> Indicates success. 
	 * @return <b>\ref LWFS_ERR_RPC</b> Indicates an failure in the 
	 *                                  communication library. 
	 */
	extern int lwfs_truncate_sync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_ssize size, 
			const lwfs_cap *cap);

#else /* K&R C */

#endif



#ifdef __cplusplus
}
#endif

#endif 

