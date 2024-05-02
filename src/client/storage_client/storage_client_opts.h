#ifndef _STORAGE_CLIENT_OPTS_H_
#define _STORAGE_CLIENT_OPTS_H_


#include "config.h"

#include <errno.h>
#include <stdio.h>

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif


/**
 * @brief Output the authorization options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_storage_client_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix)
{
	fprintf(fp, "%s ------------ Storage Client Options ----------\n", prefix);
	if (args_info->ss_server_file_arg != NULL) {
		fprintf(fp, "%s \tss-server-file = %s\n", prefix, args_info->ss_server_file_arg);
		fprintf(fp, "%s \tss-num-servers = %d\n", prefix, args_info->ss_num_servers_arg);
	}

	else {
		fprintf(fp, "%s \tss-nid = %lu\n", prefix, 
				args_info->ss_nid_arg);
		fprintf(fp, "%s \tss-pid = %lu\n", prefix, 
				(unsigned long)args_info->ss_pid_arg);
	}
}


/** 
 * @brief Read the storage server input file.
 */
int read_ss_file(
        const char *fname,
        int num_ss,
        lwfs_remote_pid *ss)
{
	int i; 
	int rc = LWFS_OK; 
	FILE *fp=NULL; 

	if (fname != NULL) {
		fp = fopen(fname, "r");
		if (!fp) {
			log_error(ss_debug_level, "unable to open ss_file \"%s\": %s", fname, strerror(errno));
			return LWFS_ERR; 
		}

		for (i=0; i<num_ss; i++) {
			rc = fscanf(fp, "%u %u", &ss[i].nid, &ss[i].pid);
			if (rc != 2) {
				log_error(ss_debug_level, "errors reading ss_file: line %d", i);
				rc = LWFS_ERR; 
				goto cleanup;
			}
			rc = LWFS_OK;
		}
	}

	else if ((ss != NULL) && (num_ss > 0)) {
		ss[0].nid = 0; 
		ss[0].pid = LWFS_SS_PID; 
	}

cleanup: 
	if (fp) fclose(fp); 
	return rc; 
}



#endif
