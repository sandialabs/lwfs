/*
 *
 *  $Id$
 *
 */

#include "config.h"

#include <pthread.h>

#include "locks.h"



int
lwfs_lock( lwfs_txn *txn_id,
	   lwfs_obj *obj,
	   lwfs_lock_type lock_type,
	   lwfs_lock_id *result,
	   lwfs_request *req )

{
  return -1;
}


int
lwfs_unlock( lwfs_txn *txn_id,
	     lwfs_lock_id *lock_id,
	     lwfs_request *req )
{
  return -1;
}









