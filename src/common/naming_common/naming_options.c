
#include <argp.h>
#include "naming_options.h"

/**
 * @brief Output the naming options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_naming_opts(FILE *fp, struct naming_options *opts, const char *prefix)
{
	fprintf(fp, "%s ------------ Naming Options ------------------\n", prefix);
	fprintf(fp, "%s \tlocal = %s\n", prefix, ((opts->local)? "true" : "false"));
	fprintf(fp, "%s \tnid = %u\n", prefix, opts->id.nid);
	fprintf(fp, "%s \tpid = %u\n", prefix, opts->id.pid);
	if (opts->local) {
		fprintf(fp, "%s \tdb_path = %s\n", prefix, opts->db_path);
		fprintf(fp, "%s \tdb_clear = %s\n", prefix, 
				((opts->db_clear)? "true" : "false"));
		fprintf(fp, "%s \tdb_recover = %s\n", prefix, 
				((opts->db_recover)? "true" : "false"));
	}
}


int load_default_naming_opts(
		struct naming_options *naming_opts)
{
	int rc = LWFS_OK;
	
	memset(naming_opts, 0, sizeof(struct naming_options));

	naming_opts->local = FALSE; 
	naming_opts->id.pid = LWFS_NAMING_PID;
	naming_opts->id.nid = 0;
	naming_opts->db_path = "naming.db";
	naming_opts->db_clear = FALSE; 
	naming_opts->db_recover = FALSE; 

	return rc; 
}

int parse_naming_opt(
		int key, 
		char *arg,
		struct naming_options *naming_opts)
{
	switch (key) {
		case NAMING_PID_OPT: 
			naming_opts->id.pid = (lwfs_pid)atoll(arg);
			break;

		case NAMING_DB_CLEAR_OPT: 
			naming_opts->db_clear = TRUE;
			break;

		case NAMING_DB_RECOVER_OPT: /* naming_db_recover */
			naming_opts->db_recover = TRUE;
			break;

		case NAMING_DB_PATH_OPT: /* naming_db_recover */
			naming_opts->db_path = arg;
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}
