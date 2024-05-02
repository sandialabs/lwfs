/**  
 *   @file ss_clnt.h
 * 
 *   @brief Protype definitions for the storage server API. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1492 $
 *   $Date: 2007-08-08 10:39:03 -0600 (Wed, 08 Aug 2007) $
 */
#include "config.h"

/*#include "portals3.h"*/

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/storage_common/ss_opcodes.h"
#include "common/storage_common/ss_args.h"
#include "common/storage_common/ss_debug.h"
#include "common/storage_common/ss_trace.h"
#include "common/storage_common/ss_xdr.h"

#include "client/rpc_client/rpc_client.h"
#include "client/authr_client/authr_client.h"

#ifndef _SS_CLNT_H_
#define _SS_CLNT_H_

/**
 * 
 *   @addtogroup ss_api
 *
 *   An LWFS storage server manages a single \em volume of objects,
 *   where a volume represents a logical collection of \em objects. 
 *   A storage server object is a generic ``blob'' of bytes that
 *   a client identifies with the 
 *   <tt>\ref lwfs_obj</tt> data structure. 
 *   The LWFS controls access to objects by assigning a single, 
 *   permanent, \ref lwfs_cid "container ID" to the object 
 *   during creation. Access to the object is restricted to the 
 *   access-control policies defined for the container. 
 *   See Section@latexonly~\ref{group__authr__api}@endlatexonly for 
 *   details on access controls for containers. 
 *
 *   The storage service API includes methods to create, remove, read,  
 *   write, and get attributes on objects on a storage server. All methods
 *   are asynchronous and require the client to use the functions described
 *   in @latexonly Section~\ref{group__rpc__api}@endlatexonly
 *   to test for completion of an operation. 
 *
 *   @latexonly \input{generated/structlwfs__obj} @endlatexonly
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	/**
	 * @brief Initialize the object data structure.
	 *
	 * The <tt>\rev lwfs_init_obj</tt> function initializes an object
	 * data structure.  It is a convenience function that should be 
	 * be called before the <tt>\ref lwfs_create_obj</tt> function. 
	 *
	 * @param svc @input_type The service that hosts the object. 
	 * @param type @input_type The type of object (user defined).
	 * @param cid  @input_type The container for the object (used for access control).
	 * @param oid  @input_type The object id on the storage server (can be LWFS_OID_ANY).
	 * @param obj  @output_type The resulting object data structure. 
	 *
	 * @returns \ref LWFS_OK on success. 
	 */
	int lwfs_init_obj(
			const lwfs_service *svc, 
			const int type,
			const lwfs_cid cid,
			const lwfs_oid oid,
			lwfs_obj *obj);

	/** 
	 * @brief Create a new object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_create_obj</tt> function creates a new object 
	 * on the specified storage server.
	 *
	 * @param svc @input_type The storage service that will host the object. 
	 * @param txn @input_type Points to the transaction (NULL if no transaction).
	 * @param cid @input_type The container ID to use for the new object. 
	 * @param cid @input_type The object ID to use for the new object 
	 *                        (use \ref LWFS_OID_ANY if not known). 
	 * @param cap @input_type The capability that allows creation of an 
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
	 * @return <b>\ref LWFS_ERR_EXISTS</b> The object id already exists.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_create_obj(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_request *req); 

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
	extern int lwfs_remove_obj(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_request *req); 

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
	extern int lwfs_read(
			const lwfs_txn *txn,
			const lwfs_obj *src_obj, 
			const lwfs_size src_offset, 
			void *buf, 
			const lwfs_size len, 
			const lwfs_cap *cap, 
			lwfs_size *result,
			lwfs_request *req);

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
	extern int lwfs_write(
			const lwfs_txn *txn,
			const lwfs_obj *dest_obj, 
			const lwfs_size dest_offset, 
			const void *buf, 
			const lwfs_size len, 
			const lwfs_cap *cap, 
			lwfs_request *req); 

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
	extern int lwfs_fsync(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_request *req); 

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
	extern int lwfs_stat(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_stat_data *data,
			lwfs_request *req); 

	/** 
	 * @brief Create an attribute array from the component parts.
	 *
	 * @ingroup ss_api
	 *  
	 * The \b lwfs_create_attr_array creates an lwfs_attr_array from 
	 * arrays of attr names, attr values and attr value lengths.
	 *
	 * @param names @input array of attribute names
	 * @param values @input array of attribute values
	 * @param sizes @input array of attribute value sizes
	 * @param num_attrs @input the number of attributes to create
	 * @param attrs @output the request handle (used to test for completion)
	 *
	 * @returns the names of the object attributes in the result field.
	 */
	extern int lwfs_create_attr_array(
			const lwfs_name *names,
			const void **values, 
			const int *sizes, 
			const int num_attrs,
			lwfs_attr_array *attrs);


	/** 
	 * @brief Create an attribute array from the component parts.
	 *
	 * @ingroup ss_api
	 *  
	 * The \b lwfs_create_attr_array creates an lwfs_attr_array from 
	 * arrays of attr names, attr values and attr value lengths.
	 *
	 * @param names @input array of attribute names
	 * @param values @input array of attribute values
	 * @param sizes @input array of attribute value sizes
	 * @param num_attrs @input the number of attributes to create
	 * @param attrs @output the request handle (used to test for completion)
	 *
	 * @returns the names of the object attributes in the result field.
	 */
	extern int lwfs_create_name_array(
			const lwfs_name names[],
			const int num_names,
			lwfs_name_array *name_array);


	/** 
	 * @brief Get the names of the extended attributes of an object. 
	 *
	 * @ingroup ss_api
	 *  
	 * The <tt>\ref lwfs_getattrs</tt> function fetches the attributes
	 * (defined in \ref lwfs_obj_attr) of a specified storage server object.
	 *
	 * @param txn @input_type Points to the transaction ID (NULL if no transaction). 
	 * @param obj @input_type Points to the remote object we want to query. 
	 * @param cap @input_type Points to the capability that allows the holder to 
	 *                   get the attributes for objects in a specified container. 
	 * @param result @output_type Points to the array of attribute names. 
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_listattrs(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj, 
			const lwfs_cap *cap, 
			lwfs_name_array *res, 
			lwfs_request *req);


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
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_getattrs(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj, 
			const lwfs_name_array *names, 
			const lwfs_cap *cap, 
			lwfs_attr_array *res,
			lwfs_request *req); 


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
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_setattrs(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_attr_array *attrs, 
			const lwfs_cap *cap, 
			lwfs_request *req); 


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
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_rmattrs(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name_array *names, 
			const lwfs_cap *cap, 
			lwfs_request *req); 

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
	 * @param req @output the request handle (used to test for completion)
	 *
	 * @returns the object attribute in the result field.
	 */
	extern int lwfs_getattr(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj, 
			const lwfs_name name, 
			const lwfs_cap *cap, 
			lwfs_attr *attr,
			lwfs_request *req);
		
	/** 
	 * @brief Set an extended attribute of an object. 
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
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_setattr(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name name,
			const void *value,
			const int size, 
			const lwfs_cap *cap, 
			lwfs_request *req); 


	/** 
	 * @brief Remove an extended attribute from an object. 
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
	 * @param req @output_type Points to the request handle (used to test for completion)
	 *
	 *
	 * @return <b>\ref LWFS_OK</b> Success.
	 * @return <b>\ref LWFS_ERR_RPC</b> Failure in the communication library. 
	 * @return <b>\ref LWFS_ERR_TXN</b> The transaction is invalid. 
	 * @return <b>\ref LWFS_ERR_NO_OBJ</b> The object does not exist.
	 * @return <b>\ref LWFS_ERR_ACCESS</b> The capability is invalid or inappropriate. 
	 */
	extern int lwfs_rmattr(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_name name, 
			const lwfs_cap *cap, 
			lwfs_request *req); 

	/** 
	 * @brief Set the size of an object. 
	 *
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
	extern int lwfs_truncate(
			const lwfs_txn *txn,
			const lwfs_obj *obj, 
			const lwfs_ssize size, 
			const lwfs_cap *cap, 
			lwfs_request *req);


#else /* K&R C */

#endif



#ifdef __cplusplus
}
#endif

#endif 

