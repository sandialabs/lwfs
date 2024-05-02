

#ifndef _AUTHR_CLIENT_OPTS_H_
#define _AUTHR_CLIENT_OPTS_H_

/**
 * @brief Output the authorization options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_authr_client_opts(
		FILE *fp, 
		const struct gengetopt_args_info *args_info, 
		const char *prefix)
{
	fprintf(fp, "%s ------------ Authorization Options -----------\n", prefix);
	fprintf(fp, "%s \tcache_caps = %s\n", prefix, ((args_info->authr_cache_caps_flag)? "true" : "false"));
	fprintf(fp, "%s \tnid = %lu\n", prefix, args_info->authr_nid_arg);
	fprintf(fp, "%s \tpid = %lu\n", prefix, args_info->authr_pid_arg);
}

#endif
