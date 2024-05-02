
#ifdef HAVE_GENGETOPT
#include "cmdline.h"
#else
#include "cmdline_default.h"
#endif

#ifndef _STORAGE_SERVER_OPTS_H_
#define _STORAGE_SERVER_OPTS_H_

/**
 * @brief Output the storage server options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_storage_server_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix)
{
	fprintf(fp, "%s ------------ Storage Server Options ----------\n", prefix);
	fprintf(fp, "%s \tss-pid = %llu\n", prefix, 
			(unsigned long long)args_info->ss_pid_arg);
	fprintf(fp, "%s \tss-root = %s\n", prefix, args_info->ss_root_arg);
	fprintf(fp, "%s \tss-iolib = %s\n", prefix, args_info->ss_iolib_arg);
}

#endif
