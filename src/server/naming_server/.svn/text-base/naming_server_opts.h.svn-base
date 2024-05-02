
#if HAVE_GENGETOPT
#include "cmdline.h"
#else
#include "cmdline_default.h"
#endif

#ifndef _NAMING_SERVER_OPTS_H_
#define _NAMING_SERVER_OPTS_H_

/**
 * @brief Output the storage server options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_naming_server_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix)
{
	fprintf(fp, "%s ------------ Naming Server Options -----------\n", prefix);
	fprintf(fp, "%s \tnaming-pid = %llu\n", prefix, 
			(unsigned long long)args_info->naming_pid_arg);
	fprintf(fp, "%s \tnaming-db-path = %s\n", prefix, args_info->naming_db_path_arg);
	fprintf(fp, "%s \tnaming-db-clear = %s\n", prefix, 
			((args_info->naming_db_clear_flag)?"true":"false"));
	fprintf(fp, "%s \tnaming-db-recover = %s\n", prefix, 
			((args_info->naming_db_recover_flag)?"true":"false"));
}

#endif
