
#ifndef _NAMING_CLIENT_OPTS_H_
#define _NAMING_CLIENT_OPTS_H_

/**
 * @brief Output the authorization options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_naming_client_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix)
{
	fprintf(fp, "%s ------------ Naming Client Options -----------\n", prefix);
	fprintf(fp, "%s \tnaming-nid = %lu\n", prefix, args_info->naming_nid_arg);
	fprintf(fp, "%s \tnaming-pid = %lu\n", prefix, args_info->naming_pid_arg);
}

#endif
