/**  
 *   @file config_parser.h
 *   
 *   @brief  Parse the lwfs configuration XML file. 
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1.23 $
 *   $Date: 2005/11/09 20:15:51 $
 *
 */

#ifndef _CONFIG_PARSER_H_
#define _CONFIG_PARSER_H_

#include "common/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern log_level config_debug_level; 

    /**
     * @brief A structure to represent the configuration of
     * LWFS core services.
     */
    struct lwfs_config {

	/** @brief Authorization service ID */
	lwfs_remote_pid authr_id;

	/* @brief Naming service ID */
	lwfs_remote_pid naming_id;

	/** @brief Namespace */
	char namespace_name[LWFS_NAME_LEN];

	/** @brief Number of available storage servers */
	int ss_num_servers; 

	/** @brief storage service IDs */
	lwfs_remote_pid *ss_server_ids;

	/** @brief Storage service chunksize */
	int ss_chunksize;
	
	/** @brief Number of fake i/o patterns */
	int ss_num_fake_io_patterns; 

	/** @brief if a filename matches one of these patterns, then 
	 *  writes are ignored and reads are faked by returning the 
	 *  requested number of bytes filled with a predefined pattern. 
	 */
	char **ss_fake_io_patterns;
    };



#if defined(__STDC__) || defined(__cplusplus)

    extern int parse_lwfs_config_file(const char *fname, 
	    struct lwfs_config *config);

    extern void lwfs_config_free(
	    struct lwfs_config *config);

#endif

#ifdef __cplusplus
}
#endif

#endif
