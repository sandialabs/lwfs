/**  
 *   @file authr_db.h
 * 
 *   @brief Prototypes for the database access methods used 
 *          by the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 */

#include "common/types/types.h"
#include "common/authr_common/authr_args.h"

#ifndef _LWFS_AUTH_DB_H_
#define _LWFS_AUTH_DB_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	/**
	 * @brief Initialize the database.
	 *
	 * This function initializes the sleepycat database used to store 
	 * access-control lists for the authorization server. 
	 *
	 * @param acl_db_fname @input_type path to the database file. 
	 * @param dbclear @input_type  flag to signal a fresh start.
	 * @param dbrecover @input_type flag to signal recovery from crash.
	 */
	extern int authr_db_init(
			const char *acl_db_fname, 
			const lwfs_bool dbclear, 
			const lwfs_bool dbrecover); 

	extern int authr_db_fini();

	/**
	 * @brief Generate a unique container ID. 
	 *
	 * This function generates a system-wide unique container ID. 
	 *
	 * This particular implementation increments and returns an integer
	 * variable stored in the acl database. 
	 */
	extern int authr_db_create_container(lwfs_cid *cid); 

	extern int authr_db_remove_container(const lwfs_cid cid); 

	extern int authr_db_put_acl(
			const lwfs_cid cid, 
			const lwfs_opcode opcode, 
			const lwfs_uid_array *acl);

	extern int authr_db_get_acl(
			const lwfs_cid cid, 
			const lwfs_opcode opcode,
			lwfs_uid_array *acl); 

	extern int authr_db_del_acl(
			const lwfs_cid cid, 
			const lwfs_opcode opcode);

#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 
