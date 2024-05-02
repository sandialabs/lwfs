/**  @file naming_args.x
 *   
 *   @brief XDR definitions for the argument structures for the 
 *   \ref naming_api "naming API" for the LWFS. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1045 $.
 *   $Date: 2007-01-19 09:17:33 -0700 (Fri, 19 Jan 2007) $.
 *
 */

/* include files for security_xdr.h */
#ifdef RPC_HDR
%#include "common/types/types.h"
#endif

/**
 * @brief Arguments for the \ref lwfs_create_namespace method that 
 * have to be passed to the naming server. 
 */
struct lwfs_create_namespace_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The name of the new namespace. */
	lwfs_name name;

	/** @brief The container ID of the new namespace. */
	lwfs_cid cid;
};


/**
 * @brief Arguments for the \ref lwfs_remove_namespace method that 
 * have to be passed to the naming server. 
 */
struct lwfs_remove_namespace_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The name of the namespace to remove. */
	lwfs_name name;

	/** @brief The capability that allows the remove. */
	lwfs_cap *cap;
};


/**
 * @brief Arguments for the \ref lwfs_create_namespace method that 
 * have to be passed to the naming server. 
 */
struct lwfs_get_namespace_args {

	/** @brief The name of the new namespace. */
	lwfs_name name;
};


/**
 * @brief Arguments for the \ref lwfs_create_dir method that 
 * have to be passed to the naming server. 
 */
struct lwfs_create_dir_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the new entry. */
	lwfs_name name;

	/** @brief The container ID of the new directory. */
	lwfs_cid cid;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};


/**
 * @brief Arguments for the \ref lwfs_remove_dir method that 
 * have to be passed to the naming server. 
 */
struct lwfs_remove_dir_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the directory to remove. */
	lwfs_name name;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/**
 * @brief Arguments for the \ref lwfs_create_file method that 
 * have to be passed to the naming server. 
 */
struct lwfs_create_file_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the new file. */
	lwfs_name name;

	/** @brief The object to associate with the file. */
	lwfs_obj *obj;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/**
 * @brief Arguments for the \ref lwfs_remove_file method that 
 * have to be passed to the naming server. 
 */
struct lwfs_remove_file_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the file to remove. */
	lwfs_name name;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};



/**
 * @brief Arguments for the \ref lwfs_create_link method that 
 * have to be passed to the naming server. 
 */
struct lwfs_create_link_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the new file. */
	lwfs_name name;

	/** @brief The capability that allows the client to  
	           modify the parent directory */
	lwfs_cap *cap;

	/** @brief The parent directory of the target entry.  */
	lwfs_ns_entry *target_parent;

	/** @brief The name of the target */
	lwfs_name target_name; 

	/** @brief The capability that allows the client to  
	           access the target directory */
	lwfs_cap *target_cap;


};

/**
 * @brief Arguments for the \ref lwfs_unlink method that 
 * have to be passed to the naming server. 
 */
struct lwfs_unlink_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the link to remove. */
	lwfs_name name;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};


/**
 * @brief Arguments for the \ref lwfs_lookup method that 
 * have to be passed to the naming server. 
 */
struct lwfs_list_dir_args {

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};

/**
 * @brief Arguments for the \ref lwfs_lookup method that 
 * have to be passed to the naming server. 
 */
struct lwfs_lookup_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_ns_entry *parent;

	/** @brief The name of the entry to find. */
	lwfs_name name;

	/** @brief The type of lock to get on the target entry.  */
	lwfs_lock_type lock_type;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};


/**
 * @brief Arguments for the \ref lwfs_name_stat method that 
 * have to be passed to the naming server. 
 */
struct lwfs_name_stat_args {

	/** @brief The transaction ID of the operation. */
	lwfs_txn *txn_id;

	/** @brief The parent directory. */
	lwfs_obj *obj;

	/** @brief The capability that allows the operation. */
	lwfs_cap *cap;
};
