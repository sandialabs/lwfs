
#include <string.h>

/*
 * This file should be included in files that want 
 * to use command-line options for the logger.  We 
 * include the source code here because the definition
 * of gengetopt_args_info (generated by gengetopt) 
 * will change for each set of options generated by
 * the gengetopt program. 
 */

#ifndef _LOGGER_OPTS_H_
#define _LOGGER_OPTS_H_


/**
 * @brief Output the logger options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_logger_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix)
{
	fprintf(fp, "%s ------------ Logger Options -----------\n", prefix);
	fprintf(fp, "%s \tverbose = %d\n", prefix, args_info->verbose_arg);
	fprintf(fp, "%s \tlogfile = %s\n", prefix, args_info->logfile_arg);
}


/** 
  * @brief Load default command-line options for the logger. 
  */
int load_default_logger_opts(struct gengetopt_args_info *args_info)
{
	args_info->verbose_arg = LOG_ALL; 
	args_info->logfile_arg = NULL;
	return 0;
}


#endif 
