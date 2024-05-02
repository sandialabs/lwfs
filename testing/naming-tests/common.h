/**
 * @file test_ops.h
 *
 * @author Ron Oldfield (raoldfi\@sandia.gov)
 * $Revision: 1073 $
 * $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
 */

#include "types/types.h"
#include <sys/time.h>

#ifndef _TEST_OPS_H_
#define _TEST_OPS_H_


/* the maximum lenth of the object array */
#define MAX_LEN 100

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief returns the difference *tv1 - *tv0 in microseconds.
	 */
	extern double tv_diff_usec(
			const struct timeval *tv0, 
			const struct timeval *tv1); 

	/**
	 * @brief returns the difference *tv1 - *tv0 in seconds.
	 */
	extern double tv_diff_sec(
			const struct timeval *tv0, 
			const struct timeval *tv1);

	int get_perms(
			const lwfs_cred *cred,
			lwfs_cid *cid, 
			const lwfs_opcode opcodes,
			lwfs_cap *cap);

	extern int test_create(
			const lwfs_service *svc,
			const lwfs_txn *txn_id,
			const lwfs_cid cid,
			const lwfs_cap *cap,
			lwfs_obj *res);

	extern int test_remove(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj,
			const lwfs_cap *cap);

	extern int test_read(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj,
			const lwfs_size offset,
			const lwfs_size len,
			const lwfs_cap *cap,
			void *buf);

	extern int test_write(
			const lwfs_txn *txn_id,
			const char *buf, 
			const int buflen,
                        const int offset,                        
			const lwfs_obj *obj,
			const lwfs_cap *cap);

	extern int test_getattr(
			const lwfs_txn *txn_id,
			const lwfs_obj *obj,
			const lwfs_cap *cap,
			lwfs_obj_attr *res);

	extern int create_objs(
			const lwfs_service *service, 
			const lwfs_txn *txn_id,
			const lwfs_cid cid,
			const lwfs_cap *cap,
			lwfs_obj *obj, 
			int len);

	extern int remove_objs(
			const lwfs_txn *txn_id,
			lwfs_obj *obj, 
			const lwfs_cap *cap,
			const int len);

	extern int write_objs(
			const lwfs_txn *txn_id,
			const char *buf, 
			const int buflen, 
			lwfs_obj *obj,
			const int num_objs,
			const lwfs_cap *cap);

	extern int read_objs(
			const lwfs_txn *txn_id,
			char *buf, 
			const int buflen, 
			lwfs_obj *obj,
			const int num_objs,
			const lwfs_cap *cap);

	int getattr_objs(
			const lwfs_txn *txn_id,
			lwfs_obj *obj, 
			const int num_objs, 
			const lwfs_cap *cap, 
			lwfs_obj_attr *res);

#ifdef __cplusplus
}
#endif

#endif
