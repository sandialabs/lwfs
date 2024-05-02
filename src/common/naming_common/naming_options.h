/**  
 *   @file naming_options.h
 * 
 *   @brief Options for the naming service.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 847 $
 *   $Date: 2006-07-20 10:40:33 -0600 (Thu, 20 Jul 2006) $
 */

#include "common/types/types.h"

#ifndef _NAMING_OPTIONS_H_
#define _NAMING_OPTIONS_H_

#define NAMING_OPTIONS \
	{"naming-pid",  NAMING_PID_OPT, "<val>", 0, \
		"Portals PID of the naming server." }, \
	{"naming-db-path", NAMING_DB_PATH_OPT, "<FILE>", 0,  \
		"path to the naming service database" }, \
	{"naming-db-clear",    NAMING_DB_CLEAR_OPT, 0, 0,  \
		"clear the database before use"}, \
	{"naming-db-recover",  NAMING_DB_RECOVER_OPT, 0, 0,  \
		"recover database after a crash"}

#ifdef __cplusplus
extern "C" {
#endif

	struct naming_options {

		/** @brief Flag to run the naming service locally. */
		lwfs_bool local;

		/** @brief Process ID of the naming service. */
		lwfs_remote_pid id; 

		/** @brief Clear the database. */
		lwfs_bool db_clear; 

		/** @brief Recover from a crash. */
		lwfs_bool db_recover; 

		/** @brief Path to the database file. */
		char *db_path; 
	};

	enum naming_option_ids {
		NAMING_LOCAL_OPT = 40,
		NAMING_PID_OPT = 41,
		NAMING_NID_OPT = 42,
		NAMING_DB_CLEAR_OPT = 43,
		NAMING_DB_RECOVER_OPT = 44,
		NAMING_DB_PATH_OPT = 45
	};

#if defined(__STDC__) || defined(__cplusplus)

	extern int load_default_naming_opts(
			struct naming_options *opts);

	extern void print_naming_opts(
			FILE *fp, 
			struct naming_options *opts, 
			const char *prefix);

	extern int parse_naming_opt(
			int key, 
			char *arg,
			struct naming_options *naming_opts);
#else

#endif

#ifdef __cplusplus
	}
#endif

#endif 

