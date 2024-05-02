/**  @file ebofs-tests.c
 *   
 *   @brief Test the ebofs object-based file system.
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 830 $.
 *   $Date: 2006-07-13 13:46:30 -0600 (Thu, 13 Jul 2006) $.
 */

#include "cmdline.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "support/logger/logger.h"
#include "support/logger/logger_opts.h"
#include "ebofs/ebofs.h"



enum testid {
	EXISTS=1,
	CREATE,
	REMOVE,
	READ,
	WRITE,
	SETATTR,
	GETATTR,
	RMATTR,
	LISTATTR
};
	

static void print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
    print_logger_opts(fp, args_info, prefix); 

    fprintf(fp, "%s devfs = %s\n", prefix, args_info->devfs_arg);
    fprintf(fp, "%s oid = %x\n", prefix, args_info->oid_arg);

    fprintf(fp, "%s testid = %d ", prefix, args_info->testid_arg);
    switch (args_info->testid_arg) {
	case EXISTS:
	    fprintf(fp, "(EXISTS)\n");
	    break;

	case CREATE:
	    fprintf(fp, "(CREATE)\n");
	    break;

	case REMOVE:
	    fprintf(fp, "(REMOVE)\n");
	    break;

	case READ:
	    fprintf(fp, "(READ)\n");
	    fprintf(fp, "%s offset = %d ", prefix, args_info->offset_arg);
	    break;

	case WRITE:
	    fprintf(fp, "(WRITE)\n");
	    fprintf(fp, "%s offset = %d ", prefix, args_info->offset_arg);
	    fprintf(fp, "%s data = %s ", prefix, args_info->data_arg);
	    break;

	case SETATTR:
	    fprintf(fp, "(SETATTR)\n");
	    fprintf(fp, "%s attr-name = %s ", prefix, args_info->attr_name_arg);
	    fprintf(fp, "%s attr-val = %d ", prefix, args_info->attr_val_arg);
	    break;

	case GETATTR:
	    fprintf(fp, "(GETATTR)\n");
	    fprintf(fp, "%s attr-name = %s ", prefix, args_info->attr_name_arg);
	    break;

	case RMATTR:
	    fprintf(fp, "(RMATTR)\n");
	    fprintf(fp, "%s attr-name = %s ", prefix, args_info->attr_name_arg);
	    break;

	case LISTATTR:
	    fprintf(fp, "(LISTATTR)\n");
	    break;

	default:
	    fprintf(fp, "(UNKNOWN)\n");
	    break;
    }
}


/* --------------------------------------------- */

int main(int argc, char **argv)
{
	int rc; 
	
	struct Ebofs *ebofs = NULL;
	int bytes=0; 
	struct gengetopt_args_info args_info; 
	log_level debug_level = LOG_UNDEFINED;


	const int PASSED = 0; 
	const int FAILED = -1;

	/* Parse the command-line arguments */
	if (cmdline_parser(argc, argv, &args_info) != 0)
		return (1); 

	/* initialize the logger */
	logger_init(args_info.verbose_arg, args_info.logfile_arg); 


	/* mount the ebofs FS file */
	ebofs = ebofs_mount(args_info.devfs_arg); 
	if (ebofs == NULL) {
		log_fatal(debug_level, "Could not mount EBOFS filesystem to \"%s\"", 
				args_info.devfs_arg);
		return -1;
	}


	/* default */
	rc = PASSED; 

	switch (args_info.testid_arg) {
		case EXISTS:
			log_debug(debug_level, "calling obj_exists(%d)", args_info.oid_arg);
			rc = obj_exists(ebofs, args_info.oid_arg); 
			if (rc != TRUE) 
				rc = FAILED; 
			else 
				rc = PASSED;
			break;

		case CREATE:
			log_debug(debug_level, "calling obj_create(%d)", args_info.oid_arg);
			rc = obj_create(ebofs, args_info.oid_arg); 
			if (rc < 0)
				rc = FAILED; 
			else 
				rc = PASSED;
			break;

		case REMOVE:
			log_debug(debug_level, "calling obj_remove(%d)", args_info.oid_arg);
			rc = obj_remove(ebofs, args_info.oid_arg); 
			if (rc < 0) 
				rc = FAILED; 
			else 
				rc = PASSED;
			break;

		case READ:
			{
				char input[256]; 
				memset(input, 0, sizeof(input));
				log_debug(debug_level, "calling obj_read(oid=0x%08x,off=%d,len=%d)",
						args_info.oid_arg, 
						args_info.offset_arg, 
						(int)sizeof(input));
				bytes = obj_read(ebofs, 
						args_info.oid_arg, 
						args_info.offset_arg, 
						sizeof(input), 
						input);
				rc = PASSED; 
				if (bytes != strlen(args_info.data_arg)) {
					log_error(debug_level, "read %d bytes, should have read %d\n",
							bytes, strlen(args_info.data_arg));
					rc = FAILED; 
					break; 
				}

				if (strcmp(args_info.data_arg, input) != 0) {
					log_error(debug_level, "expected \"%s\", read \"%s\"",
							args_info.data_arg, input);
					rc = FAILED; 
				}
			}
			break;

		case WRITE:
			log_debug(debug_level, 
					"calling obj_write(oid=0x%8x,off=%d,len=%d,data=%s)", 
					args_info.oid_arg, (int)args_info.offset_arg, 
					(int)strlen(args_info.data_arg),  args_info.data_arg);
			bytes = obj_write(ebofs, args_info.oid_arg, 
					args_info.offset_arg, 
					strlen(args_info.data_arg), 
					args_info.data_arg);
			log_debug(debug_level, "obj_write() returned %d",rc);
			if (bytes != strlen(args_info.data_arg)) 
				rc = FAILED; 
			else 
				rc = PASSED;
			break;


		case SETATTR:
			log_debug(debug_level, "calling "
					"obj_setattr(oid=%x,attr_name=%s,attr_val=%d)", 
					args_info.oid_arg, args_info.attr_name_arg, 
					args_info.attr_val_arg);
			rc = obj_setattr(ebofs, args_info.oid_arg, 
					args_info.attr_name_arg, 
					&args_info.attr_val_arg, 
					sizeof(args_info.attr_val_arg));
			if (rc < 0) 
				rc = FAILED; 
			else 
				rc = PASSED;
			break;


		case GETATTR:
			{
				int val; 
				log_debug(debug_level, "calling "
						"obj_getattr(oid=%x,attr_name=%s)", 
						args_info.oid_arg, args_info.attr_name_arg);
				rc = obj_getattr(ebofs, args_info.oid_arg, 
						args_info.attr_name_arg, 
						&val, 
						sizeof(val));
				if ((rc<0) || (val != args_info.attr_val_arg))
					rc = FAILED; 
				else 
					rc = PASSED;
			}
			break;


		case RMATTR:
			log_debug(debug_level, "calling obj_rmattr(oid=%x,attr=%s)", 
					args_info.oid_arg, args_info.attr_name_arg);
			rc = obj_rmattr(ebofs, args_info.oid_arg, args_info.attr_name_arg);
			if (rc < 0) 
				rc = FAILED; 
			else 
				rc = PASSED;
			break;


		case LISTATTR:
			{
				char **list = NULL; 
				int len=0; 
				int i; 

				rc = obj_listattr(ebofs, 
						args_info.oid_arg, &list, &len);
				if (rc < 0) {
					rc = FAILED; 
				}
				else {
					for (i=0; i<len; i++) {
						fprintf(stdout, "attr[%d] = %s\n", i, list[i]);
						free(list[i]);
						list[i] = NULL;
					}
					free(list);
					list = NULL;
				}
			}
			break;

		default:
			print_args(stdout, &args_info, "");
			rc = FAILED;
	}

	/* unmount the ebofs FS file */
	ebofs_umount(ebofs); 

	/* release space allocated by the parser */
	cmdline_parser_free(&args_info);

	if (rc == PASSED) 
	    fprintf(stdout, "PASSED\n");
	else 
	    fprintf(stdout, "FAILED\n");

	return rc; 
}
