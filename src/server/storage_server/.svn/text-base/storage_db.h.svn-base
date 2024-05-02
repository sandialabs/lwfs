/**  
 *   @file storage_db.h
 * 
 *   @brief Prototypes for the database access methods used 
 *          by the storage service.  
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @author Rolf Reisen (rolf\@sandia.gov)
 *   $Revision: 1065 $
 *   $Date: 2007-01-19 10:12:25 -0700 (Fri, 19 Jan 2007) $
 */

#include "common/types/types.h"

#ifndef _LWFS_NAMING_DB_H_
#define _LWFS_NAMING_DB_H_

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Structure that represents a directory entry in the database.
	 */
	typedef struct  {
		/** @brief The unique id of the object associated attribute */
		lwfs_oid oid;

		/** @brief The name of the attribute */
		char name[LWFS_NAME_LEN];

		/** @brief The oid for the parent.
		 *
		 *  If we later want to support a distributed namespace,
		 *  we would represent the parent with an \ref lwfs_obj.
		 */
		char value[LWFS_ATTR_SIZE];

		/** @brief Is this node deleted (but still linked to)? */
		lwfs_size size;

	} ss_db_attr_entry; 

#if defined(__STDC__) || defined(__cplusplus)

	/**
	 * @brief Initialize the database.
	 *
	 * This function initializes the sleepycat database used to store
	 * namespace entries for the storage server
	 *
	 * @param acl_db_fname @input_type path to the database file.
	 * @param dbclear @input_type  flag to signal a fresh start.
	 * @param dbrecover @input_type flag to signal recovery from crash.
	 */
	extern int ss_db_init(
			const char *acl_db_fname,
			const lwfs_bool dbclear,
			const lwfs_bool dbrecover);

	extern int ss_db_fini();

	extern int ss_db_gen_oid(lwfs_oid *result);

	extern int ss_db_get_all_attr_names_by_oid(
			const lwfs_oid *oid,
			char **result,
			unsigned int *max_results);

	extern int ss_db_get_attr_by_name(
			const lwfs_oid *oid,
			const char *name,
			ss_db_attr_entry *result);

	extern int ss_db_attr_put(
			const ss_db_attr_entry *entry,
			const uint32_t options);

	extern int ss_db_attr_del(
			const lwfs_oid *oid,
			const char *name,
			ss_db_attr_entry *result);

	extern lwfs_bool ss_db_exists(
			const ss_db_attr_entry *db_entry);

	extern int ss_db_get_attr_count(
			const lwfs_oid *oid,
			unsigned int *result);

	extern int ss_db_print_all();

#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif

#endif 

