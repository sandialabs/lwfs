/**  
 *   @file ss_args.x
 *   
 *   @brief XDR defintions for the storage service arguments.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1157 $.
 *   $Date: 2007-02-06 13:28:37 -0700 (Tue, 06 Feb 2007) $.
 *
 */

#ifdef RPC_HDR
%#include "common/types/types.h"
#endif


struct ss_create_obj_args {
	lwfs_txn *txn_id;
	lwfs_obj *obj; 
	lwfs_cap *cap; 
};

struct ss_remove_obj_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_cap *cap; 
};

struct ss_read_args {
	lwfs_txn *txn_id;
	lwfs_obj *src_obj;
	lwfs_size src_offset;
	lwfs_size len; 
	lwfs_cap *cap; 
};

struct ss_fsync_args {
    lwfs_txn *txn_id;
    lwfs_obj *obj;
	lwfs_cap *cap; 
};

struct ss_write_args {
	lwfs_txn *txn_id;
	lwfs_obj *dest_obj;
	lwfs_size dest_offset;
	lwfs_size len;
	lwfs_cap *cap;
};

struct ss_stat_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_cap *cap; 
};


struct ss_listattrs_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_cap *cap; 
};

struct ss_getattrs_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_name_array *names;
	 lwfs_cap *cap; 
};

struct ss_setattrs_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_attr_array *attrs;
	 lwfs_cap *cap; 
};

struct ss_rmattrs_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_name_array *names;
	 lwfs_cap *cap; 
};

struct ss_getattr_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_name name;
	 lwfs_cap *cap; 
};

struct ss_setattr_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_attr *attr;
	 lwfs_cap *cap; 
};

struct ss_rmattr_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_name name;
	 lwfs_cap *cap; 
};

struct ss_truncate_args {
	 lwfs_txn *txn_id;
	 lwfs_obj *obj; 
	 lwfs_ssize size; 
	 lwfs_cap *cap; 
};
