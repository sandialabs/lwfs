/*-------------------------------------------------------------------------*/
/**  @file mds_srvr.c
 *   
 *   @brief Implementation of the server-side mds api. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

#include <portals/p30.h>
#include "lwfs_xdr.h"
#include "logger/logger.h"
#include "mds/mds_srvr.h"
#include "comm/comm.h"
#include "mds/mds_xdr.h"
#include "mds/mds_db.h"
#include "mds/mds_debug.h"



lwfs_return_code_t mds_create_srvr(
	mds_args_Nn_t *args, 
	mds_res_N_rc_t *result)
{
	log_debug(mds_debug_level, "create an object with name=%s", *(args->name));
	return mds_enter_db(args, MDS_REG, result); 
}


lwfs_return_code_t mds_remove_srvr(
	mds_node_args_t *args, 
	mds_res_note_rc_t *result)
{
	log_debug(mds_debug_level, "remove an object");
	return mds_delobj_db(args, result); 
}


lwfs_return_code_t mds_lookup2_srvr(
	mds_args_N_t *args, 
	mds_res_note_rc_t *result)
{
	log_debug(mds_debug_level, "look up an object");
	return mds_lookup_db(args, result); 
}


lwfs_return_code_t mds_lookup_srvr(
	mds_args_Nn_t *args, 
	mds_res_N_rc_t *result)
{
	log_debug(mds_debug_level, "look up an object with name=%s", *(args->name));
	return mds_lookup_name_db(args, result); 
}


lwfs_return_code_t mds_mkdir_srvr(
	mds_args_Nn_t *args, 
	mds_res_N_rc_t *result)
{
	log_debug(mds_debug_level, "Create a directory");
	return mds_enter_db(args, MDS_DIR, result); 
}


lwfs_return_code_t mds_rmdir_srvr(
	mds_args_N_t *args, 
	mds_res_note_rc_t *result)
{
	log_debug(mds_debug_level, "Remove a directory");
	return mds_rmdir_db(args, result); 
}


lwfs_return_code_t mds_rename_srvr(
	mds_args_NNn_t *args, 
	mds_res_N_rc_t *result)
{
	log_debug(mds_debug_level, "Renaming an object");
	return mds_rename_db(args, result); 
}


lwfs_return_code_t mds_link_srvr(
	mds_args_NNn_t *args, 
	mds_res_N_rc_t *result)
{
	log_debug(mds_debug_level, "Linking an object");
	return mds_link_db(args, result); 
}


lwfs_return_code_t mds_getattr_srvr(
	mds_node_args_t *args,
	mds_res_note_rc_t *result)
{
	log_debug(mds_debug_level, "Getting attributes of an object");
	return mds_getattr_db(args, result); 
}


lwfs_return_code_t mds_readdir_srvr(
	mds_args_N_t *args,
	mds_readdir_res_t *result)
{
	log_debug(mds_debug_level, "Reading a directory");
	return mds_readdir_db(args, result);
}


/*-------------------------------------------------------------------------
 * Revision history
 *
 * $Log$
 * Revision 1.15  2004/09/13 15:18:07  raoldfi
 * added a "_t" to all lwfs defined types.
 *
 * Revision 1.14  2004/08/11 17:06:34  raoldfi
 * Made lots of minor changes:
 *   - removed the include directory (not needed),
 *   - fixed #include statements that pointed to the include directory,
 *   - fixed paths in Makefile.am to include the full path name.
 *
 * Revision 1.13  2004/08/06 23:07:40  rolf
 *
 *     - Added lookup2(). May go away again in the near future.
 *     - mds_lookup_db() used wrong arguments.
 *     - Return # of links and linksto in db_find.
 *     - Bug in delobj.
 *     - Make sure lists are NULL terminated even in case of error.
 *     - Simplified the mds_readdir_res struct.
 *
 * Revision 1.12  2004/07/29 16:58:00  rolf
 *
 *     Added readdir capability (we need that for ls ;-)
 *
 * Revision 1.11  2004/07/26 21:35:57  rolf
 *
 *         Some clean-up and code for hard links. Links seem to work,
 * 	but more debugging and clean-up is required.
 *
 * Revision 1.10  2004/07/25 23:07:01  rolf
 *
 *     - major reorganization and simplification of code in mds_clnt.c
 * 	and mds.c
 *
 *     - renamed some of the arguments and results passed between
 * 	client and MDS server to more closely reflect recent
 * 	nomenclature definitions.
 *
 * Revision 1.9  2004/06/23 22:43:48  rolf
 *
 *     Added code for mds_getattr() and changed the data that
 *     is passed between the client and the server. This is a
 *     major change and it doesn't work yet. (It compile, though ;-)
 *     Everything before this check-in is tagged BEFORE_MDS_XDR_CLEANUP
 *
 * Revision 1.8  2004/03/08 18:37:16  rolf
 *
 *     - Initial code for rename
 *     - always use long options (gets rid of -- and use - for options)
 *
 * Revision 1.7  2004/02/29 04:58:33  rolf
 *
 *     - Added the rmdir function
 *     - Lots more documentation
 *     - Fixed some typos
 *     - Not all directory operations need "name" as an argument.
 * 	Often we have the object ID, which is enough to find the
 * 	object, so passing along the name is superfluous. This
 * 	needs more work to be implemented properly...
 *
 * Revision 1.6  2004/02/26 19:26:19  rolf
 *
 *     Now we can create objects and directories, look up by name and
 *     object ID, and remove objects.
 *
 *     Expanded the little test program mds_create a little bit to
 *     try out all these functions.
 *
 * Revision 1.5  2004/02/23 17:05:07  raoldfi
 * Added an input log_level variable to the logging interface
 * and added function name, file name, and line number to the
 * output of a log message.
 *
 * Revision 1.4  2004/02/19 17:04:03  rolf
 *
 *     Major clean-up and code reorganization. Split functionality
 *     into two files: mds_db.c provides somewhat generic functions
 *     to look up objects, enter objects, etc. mds_db_access.c has
 *     the functions needed by mds_db.c to perform operations on the
 *     databases themselves.
 *
 *     Added functions to lookup an object by its ID, or by its name
 *     and the ID of its parent.
 *
 * Revision 1.3  2004/02/13 23:01:32  rolf
 *
 *     The files needed to link into the Berkeley database. This is
 *     just a start. Not much works yet, and I'll probably change how
 *     things are stored in the database.
 *
 *     mds now accepts command line options:
 *
 *     mds [--help] [--dbfname <name>] [--dbclear] [--dbrecover]
 * 	--help              This usage message
 * 	--dbfname <name>    specifies the database name. Default: /tmp/mds.db
 * 	--dbclear           clears (empties) the database before use
 * 			    WARNING: All metadata will be lost!
 * 	--dbrecover         Recovers the database after a crash
 *
 * Revision 1.2  2004/02/05 20:49:55  rolf
 *
 *     Fixed a typo.
 *
 * Revision 1.1  2004/02/03 18:07:07  raoldfi
 * *** empty log message ***
 *
 * Revision 1.2  2004/01/07 19:12:39  raoldfi
 * added code to print out the capid of the arguments for mds_create.
 *
 * Revision 1.4  2003/12/17 22:12:22  raoldfi
 * final commit before releasing to lwfs group.
 *
 *
 */
