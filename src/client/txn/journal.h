/*-------------------------------------------------------------------------*/
/**  
 *   @file journal.h
 * 
 *   @brief Methods that operate on journals.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Riesen (rolf\@cs.sandia.gov)
 *   $Revision: 1081 $
 *   $Date: 2007-01-24 15:10:51 -0700 (Wed, 24 Jan 2007) $
 */

/**
 * @addtogroup journal_api
 * 
 * An LWFS journal stores a sequence of LWFS operations that 
 * execute inside a distributed transaction.  Note that LWFS 
 * transactions, by themselves, do not preserve all of the 
 * ACID properties typically required for distributed 
 * transactions. The client must explicitely use 
 * separate mechanisms (e.g., \ref lock_api "locks") 
 * to ensure isolation and consistency within a transaction. 
 * 
 * In the event of a call to 
 * <tt>\ref lwfs_journal_abort "lwfs_journal_abort()"</tt>, the 
 * transaction manager (usually the client) is responsible 
 * for returning the system to its previous state.  If the 
 * client fails in the middle of a transaction.  The 
 * storage server managing the journal becomes 
 * the transaction manager and can either complete or 
 * abort the transaction on behalf of the client. 
 *
 * All functions in the Journal API are asynchronous. The client 
 * checks for completion by using the functions described in 
 * @latexonly Section~\ref{group__async__api}@endlatexonly.
 *
 */

#ifndef _LWFS_JOURNAL_H_
#define _LWFS_JOURNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common/types/types.h"
#include "common/types/fprint_types.h"

#if defined(__STDC__) || defined(__cplusplus)

	/** 
	 * @brief Begin logging LWFS operations to a journal. 
	 * @ingroup journal_api
	 * 
	 * The <tt> \ref lwfs_journal_begin </tt> function creates a new 
	 * transaction ID that LWFS functions reference when 
	 * logging operations to a persistent journal.  
	 * 
	 * @param parent_txn_id  @input_type Points to the parent transaction (NULL 
	 *                               if creating a top-level transaction).
	 * @param ss_id          @input_type Points to the process ID of the storage server 
	 *                               that manages the journal. 
	 * @param server_list    @input_type Points to the list of servers involved in 
	 *                               the transaction. 
	 * @param cap            @input_type Points to a capability that allows the client to 
	 *                               begin logging operations to a journal. 
	 * @param result         @output_type Points to a new transaction ID.  
	 * @param req            @output_type  Points to the data structure that holds information
	 *                               about the pending request. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success of encoding and transferring the request.
     * @return <b>\ref LWFS_ERR_RPC</b> Indicates an error in the underlying transport library
	 *                                  while transferring the request to the remote server. 
	 * @return <b>\ref LWFS_ERR_TXN</b> Indicates that parent transaction ID is invalid or the
	 *                                 journal object does not exist. 
	 * @return <b>\ref LWFS_ERR_NOENT</b> Indicates invalid server IDs for the parent 
	 *                                    or the server list. 
	 * @return <b>\ref LWFS_ERR_ACCESS</b> Indicates that the capabilty does not enable
	 *                                     the right kind of access authorization. 
	 */
	extern int lwfs_journal_begin(
			lwfs_txn *parent_txn_id,
			lwfs_remote_pid *ss_id, 
			lwfs_remote_pid_list *server_list, 
			lwfs_cap *cap, 
			lwfs_txn *result,
			lwfs_request *req);

	/** 
	 * @brief Add participating servers to a transaction. 
	 * @ingroup journal_api
	 * 
	 * The <tt>\ref lwfs_journal_add_servers</tt> function adds 
	 * a list of participating servers to a transaction. The list 
	 * supplements the list of participating servers provided in 
	 * the <tt>\ref lwfs_journal_begin "lwfs_journal_begin()"</tt>  function. 
	 * 
	 * @param txn_id     @input_type Points to the transaction structure. 
	 * @param to_add     @input_type Points to the list of servers to add to a transaction. 
	 * @param cap        @input_type Points to the capability that allows the caller to 
	 *                          modify the journal.
	 * @param req        @output_type Points to the data structure that holds information
	 *                           about the pending request. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success of encoding and transferring the request.
     * @return <b>\ref LWFS_ERR_RPC</b> Indicates an error in the underlying transport library
	 *                                  while transferring the request to the remote server. 
	 * @return <b>\ref LWFS_ERR_TXN</b> Indicates that the journal object associated with
	 *                                     the transaction ID does not exist. 
	 * @return <b>\ref LWFS_ERR_NOENT</b> Indicates invalid server IDs. 
	 * @return <b>\ref LWFS_ERR_ACCESS</b> Indicates that the capability does not enable
	 *                                     the right kind of access authorization. 
	 */
	extern int lwfs_journal_add_servers(
			lwfs_txn *txn_id, 
			lwfs_remote_pid_list *to_add,
			lwfs_cap *cap, 
			lwfs_request *req);


	/** 
	 * @brief Prepare to commit a transaction. 
	 * @ingroup journal_api
	 * 
	 * The <tt>\ref lwfs_journal_prepare_commit</tt> function executes the first 
	 * phase of a two-phase transaction commit. It tells all participating servers 
	 * to expect a commit call.  The operation completes when 
	 * all participating servers acknowledge the prepare with a 
	 * ``yes'' or a ``no''.  If any server responds ``no'', the 
	 * transaction aborts.  
	 *
	 * @param txn_id   @input_type Points to the transaction structure. 
	 * @param cap      @input_type Points to the capability that allows the caller 
	 *                         to prepare to commit.
	 * @param req      @output_type Points to the data structure that holds information
	 *                         about the pending request. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success of encoding and transferring the request.
     * @return <b>\ref LWFS_ERR_RPC</b> Indicates an error in the underlying transport library
	 *                                  while transferring the request to the remote server. 
	 * @return <b>\ref LWFS_ERR_TXN</b> Indicates an invalid transaction. 
	 * @return <b>\ref LWFS_ERR_ACCESS</b> Indicates that the capability does not enable
	 *                                     the right kind of access authorization. 
	 */
	extern int lwfs_journal_prepare_commit(
			lwfs_txn *txn_id,
			lwfs_cap *cap, 
			lwfs_request *req);

	/** 
	 * @brief Commit a transaction. 
	 * @ingroup journal_api
	 * 
	 * The <tt>\ref lwfs_journal_commit</tt> function executes the 
	 * second phase of a two-phase transaction commit. 
	 * It tells all participating servers to go ahead and commit. This function 
	 * completes when all participating servers acknowledge commiting their 
	 * changes.  If the client fails before all servers report success, the 
	 * storage server managing the journal can continue the operation.  Upon
	 * completion, the transaction manager also releases any locks associated 
	 * with the transaction. 
	 *
	 * @param txn_id   @input_type Points to the transaction structure. 
	 * @param cap      @input_type Points to the capability that allows the caller 
	 *                         to commit the transaction. 
	 * @param req      @output_type Points to the data structure that holds information
	 *                         about the pending request. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success of encoding and transferring the request.
     * @return <b>\ref LWFS_ERR_RPC</b> Indicates an error in the underlying transport library
	 *                                  while transferring the request to the remote server. 
	 * @return <b>\ref LWFS_ERR_TXN</b> Indicates an invalid transaction. 
	 * @return <b>\ref LWFS_ERR_ACCESS</b> Indicates that the capability does not enable
	 *                                     the right kind of access authorization. 
	 */
	extern int lwfs_journal_commit(
			lwfs_txn *txn_id,
			lwfs_cap *cap, 
			lwfs_request *req);

	/** 
	 * @brief Abort a transaction. 
	 * @ingroup journal_api
	 * 
	 * The <tt>\ref lwfs_journal_abort</tt> function tells all servers 
	 * participating in a transaction to abort the transaction. This 
	 * functional also releases any locks associated with the transaction. 
	 *
	 * @note It is the responsibility of the transaction manager (in 
	 *       most cases the client) to recover the system after an abort. 
	 * 
	 * @param txn_id   @input_type Points to the transaction structure. 
	 * @param cap      @input_type Points to the capability that allows the caller 
	 *                         to abort the transaction. 
	 * @param req      @output_type Points to the data structure that holds information
	 *                         about the pending request. 
	 *
	 * @return <b>\ref LWFS_OK</b> Indicates success of encoding and transferring the request.
     * @return <b>\ref LWFS_ERR_RPC</b> Indicates an error in the underlying transport library
	 *                                  while transferring the request to the remote server. 
	 * @return <b>\ref LWFS_ERR_TXN</b> Indicates an invalid transaction. 
	 * @return <b>\ref LWFS_ERR_ACCESS</b> Indicates that the capability does not enable
	 *                                     the right kind of access authorization. 
	 */
	extern int lwfs_journal_abort(
			lwfs_txn *txn_id,
			lwfs_cap *cap, 
			lwfs_request *req);

  
#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

