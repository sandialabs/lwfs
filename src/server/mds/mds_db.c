/**
    @file mds_db.c
    @brief Functions to enter, update, and find objects in the database.

    The functions in this file implement the basic file system
    primitives, such as inserting an object into a directory,
    finding it again, and removing it. They are kept fairly general;
    i.e. they call other functions (in mds_db_access.c) to do the
    actual database accesses.

    The functions here do some basic checks. For example, they make
    sure an object to be removed is not a directory, or that the
    name of a new object does not already exist, etc.

    @author Rolf Riesen (rolf\@cs.sandia.gov)
    $Revision: 791 $
    $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $

*/

#include <stdio.h>
#include "logger/logger.h"
#include "lwfs_xdr.h"
#include "mds/mds_xdr.h"
#include "mds/mds_db.h"
#include "mds/mds_db_access.h"
#include "mds/mds_debug.h"


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/*
** A helper function to print IDs
*/
char *
ID2str(lwfs_did_t *ID)
{

static int i= 0;
static char str1[LWFS_NAME_LEN];
static char str2[LWFS_NAME_LEN];
static char str3[LWFS_NAME_LEN];
static char str4[LWFS_NAME_LEN];


    i= (i + 1) % 4;
    if (i == 0)    {
	sprintf(str1, "%08x%08x%08x%08x", (unsigned int)ID->part1,
	    (unsigned int)ID->part2, (unsigned int)ID->part3, (unsigned int)ID->part4);
	return str1;
    } else if (i == 1)    {
	sprintf(str2, "%08x%08x%08x%08x", (unsigned int)ID->part1,
	    (unsigned int)ID->part2, (unsigned int)ID->part3, (unsigned int)ID->part4);
	return str2;
    } else if (i == 2)    {
	sprintf(str3, "%08x%08x%08x%08x", (unsigned int)ID->part1,
	    (unsigned int)ID->part2, (unsigned int)ID->part3, (unsigned int)ID->part4);
	return str3;
    } else if (i == 3)    {
	sprintf(str4, "%08x%08x%08x%08x", (unsigned int)ID->part1,
	    (unsigned int)ID->part2, (unsigned int)ID->part3, (unsigned int)ID->part4);
	return str4;
    }

    return NULL;

}  /* end of ID2str() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Create a new directory or object in the database.

    @param args		the structure containing the parent directory ID, and
			the new object name.
    @param otype	either MDS_REG or MDS_DIR to create a regular object
			or a directory. Links are created with mds_link_db().
    @param result	the new object's ID will be returned here.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_EXIST otherwise

    The function makes sure the parent directory exists and then
    inserts the new object into the database. If that name already
    exisits in this subdirectory, then db_enter_new() will return
    an error when it tries to insert it into the database,

    If otype is MDS_DIR, then the new object will be a directory. If
    otype is MDS_REG, then the new object will be a regular entry.

*/
lwfs_return_code_t
mds_enter_db(mds_args_Nn_t *args, mds_otype_t otype, mds_res_N_rc_t *result)
{

struct lwfs_did_t parent_ID;
struct lwfs_did_t notused = {0, 0, 0, 0};
int rc;
int isdir;


    /* FIXME: At least some of this will have to be a single transaction */

    log_debug(mds_debug_level, " DB mds_enter_db(%s) ------------------------------------",
	*(args->name));

    /* The parent directory should already exist in the database.  */
    parent_ID= args->node.ID->dbkey;

    if (db_obj_exists(&parent_ID, &isdir) != 0)   {
	log_error(mds_debug_level, " DB parent of \"%s\" doesn't exist", *(args->name));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    if (!isdir)   {
	log_error(mds_debug_level, " DB parent of \"%s\" is not a dir", *(args->name));
	/* FIXME: This may not be the correct code for this case! */
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    /* Create a new database entry for this object */
    rc= db_enter_new(args, otype, notused, &(result->node.dbkey));
    if (rc != 0)   {
	log_error(mds_debug_level, " DB %s can't be entered", *(args->name));
	/* FIXME: This may not be the correct code in all cases! */
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    return result->ret= LWFS_OK;

}  /* end of mds_enter_db() */


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Find an object in the database using its database ID.

    @param args		the structure containng the object ID.
    @param result	contains the found information about the object.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_EXIST otherwise

    Find an object in the database and return some information
    about it.  Since we only have the object's unique ID, we do
    the lookup in the secondary database.

*/
lwfs_return_code_t
mds_lookup_db(mds_args_N_t *args, mds_res_note_rc_t *result)
{

lwfs_return_code_t rc;
mds_res_N_rc_t res;


    log_debug(mds_debug_level, " DB mds_lookup_db(ID %s) ----------------------",
	ID2str(&(args->node.ID->dbkey)));

    if (db_find(&(args->node.ID->dbkey), &res) != 0)   {
	log_warn(mds_debug_level, " DB Can't find entry %s",
	    ID2str(&(args->node.ID->dbkey)));
	rc= LWFS_ERR_MDS_EXIST;
    } else   {
	rc= LWFS_OK;
    }

    return result->ret= rc;

}  /* end of mds_lookup_db() */


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Find an object in the database using its name and its parent's ID.

    @param args		the structure containing the parent directory ID, and
			the object's name.
    @param result	contains the found information about the object.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_EXIST otherwise

    Find an object in the database and return some information
    about it.  We have the ID of the parent directory, and the
    object's name. Therefore, we look in the primary database.

*/
lwfs_return_code_t
mds_lookup_name_db(mds_args_Nn_t *args, mds_res_N_rc_t *result)
{

lwfs_return_code_t rc;


    log_debug(mds_debug_level, " DB mds_lookup_name_db(%s) ------------------------------------",
	*(args->name));

    if (db_find_name(&(args->node.ID->dbkey), *(args->name), result) != 0)   {
	log_warn(mds_debug_level, " DB Can't find entry \"%s\"",
	    *(args->name));
	rc= LWFS_ERR_MDS_EXIST;
    } else   {
	rc= LWFS_OK;
    }

    return result->ret= rc;

}  /* end of mds_lookup_name_db() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Remove an object from the database.

    @param args		the structure containing the object ID.
    @param result	The same result code as returned by the function.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_EXIST otherwise.

    Remove an object from the database. The function makes sure
    the object is not a directory.

*/
lwfs_return_code_t
mds_delobj_db(mds_node_args_t *args, mds_res_note_rc_t *result)
{

lwfs_did_t obj_ID;
int rc;
int isdir;


    log_debug(mds_debug_level, " DB mds_delobj_db() ------------------------------------");

    /* Make sure the object itself is not a directory */
    obj_ID= args->ID->dbkey;
    if (db_obj_exists(&obj_ID, &isdir) != 0)   {
	log_error(mds_debug_level, " DB object doesn't exist!");
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    if (isdir)   {
	log_error(mds_debug_level, " DB object is a directory!");
	return result->ret= LWFS_ERR_MDS_ISDIR;
    }


    /* Now remove the object from the database */
    rc= db_delobj(&obj_ID);
    if (rc != 0)   {
	log_error(mds_debug_level, " DB object can't be removed!");
	return result->ret= LWFS_ERR_MDS_ACCES;
    }

    return result->ret= LWFS_OK;

}  /* end of mds_delobj_db() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Remove a directory from the database.

    @param args		the structure containing the directory ID.
    @param result	The same result code as returned by the function.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_EXIST otherwise.

    Remove a directory from the database. The function makes sure
    the object is a directory and is empty.

*/
lwfs_return_code_t
mds_rmdir_db(mds_args_N_t *args, mds_res_note_rc_t *result)
{

lwfs_did_t obj_ID;
int rc;
int isdir;


    log_debug(mds_debug_level, " DB mds_rmdir_db() ------------------------------------");

    /* Make sure it is a directory */
    obj_ID= args->node.ID->dbkey;
    if (db_obj_exists(&obj_ID, &isdir) != 0)   {
	log_error(mds_debug_level, " DB object doesn't exist!");
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    if (!isdir)   {
	log_error(mds_debug_level, " DB object is not a directory!");
	return result->ret= LWFS_ERR_MDS_ISDIR;
    }


    /*
    ** Make sure the directory is empty.
    ** The easiest way to do that, is to look up its ID in the tertiary database.
    ** If any object still resides in this directory, it will be in the tertiary
    ** database with this directorys ID as key.
    */
    rc= is_dir_empty(&obj_ID);
    if (rc != 0)   {
	log_error(mds_debug_level, " DB directory is not empty");
	/* FIXME: This needs to be a different error code. */
	return result->ret= LWFS_ERR_NOTSUPP;
    }


    /* Now remove the directory from the database */
    rc= db_delobj(&obj_ID);
    if (rc != 0)   {
	log_error(mds_debug_level, " DB directory can't be removed");
	return result->ret= LWFS_ERR_MDS_ACCES;
    }

    return result->ret= LWFS_OK;

}  /* end of mds_rmdir_db() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Rename an object

    @param args		the structure containing the source ID and target directory
			ID and new object name..
    @param result	The same result code as returned by the function.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_EXIST otherwise.

*/
lwfs_return_code_t
mds_rename_db(mds_args_NNn_t *args, mds_res_N_rc_t *result)
{

mds_res_N_rc_t res;
lwfs_did_t new_dir_ID;
int isdir;


    log_debug(mds_debug_level, " DB mds_rename_db() ------------------------------------");


    /*
    ** This is a rather inefficient implementation. It requires several access to
    ** the database and the removal and insertion of the object into the database
    ** twice.
    */


    /*
    ** Check if the object exists
    */
    if (db_find(&(args->node1.ID->dbkey), result) != 0)   {
	log_warn(mds_debug_level, "DB mds_rename() Can't find entry %s",
	    ID2str(&(args->node1.ID->dbkey)));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    /*
    ** Make sure the destination exists
    */
    new_dir_ID= args->node2.ID->dbkey;
    if (db_obj_exists(&new_dir_ID, &isdir) != 0)   {
	log_error(mds_debug_level, "DB mds_rename() destination doesn't exist");
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    if (!isdir)   {
	log_error(mds_debug_level, "DB mds_rename() destination is not a directory");
	return result->ret= LWFS_ERR_MDS_EXIST;
    }



    /*
    ** Check if the new name in the new directory is valid
    */
    if (db_find_name(&new_dir_ID, *(args->name), &res) == 0)   {
	log_warn(mds_debug_level, "DB mds_rename() new name \"%s\" already exists",
	    *(args->name));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    /*
    ** Make sure the source is not part of the destination; e.g.,
    ** don't permit mv dir2 dir2/dir3/dir4. Programs like mv prevent this from
    ** happening, but it doesn't hurt to check here: One way is to traverse
    ** destination up to the root directory and make sure we don't see the
    ** source node on the way.
    
       FIXME: Well, do it!!!!!! ;-)
    */



    /*
    ** Perform the rename
    */
    if (db_update_name(&(args->node1.ID->dbkey), *(args->name)) != 0)   {
	log_warn(mds_debug_level, "DB mds_rename() rename to \"%s\" failed",
	    *(args->name));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    /*
    ** Move the object to the new directory, if necessary
    */
    if (db_move_obj(&(args->node1.ID->dbkey), &new_dir_ID) != 0)   {
	log_warn(mds_debug_level, "DB mds_rename() move of \"%s\" failed",
	    *(args->name));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    return result->ret= LWFS_OK;

}  /* end of mds_rename_db() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Link an object

    @param args		node1 is the target we link to, node2 is the directory we
			create the link in, and "name" is the name of the link.
    @param result	The same result code as returned by the function.
    @return		LWFS_OK if successfull

*/
lwfs_return_code_t
mds_link_db(mds_args_NNn_t *args, mds_res_N_rc_t *result)
{

int rc;
mds_res_N_rc_t res;
lwfs_did_t new_dir_ID;
int isdir;


    log_debug(mds_debug_level, " DB mds_link_db() ------------------------------------");


    /*
    ** Check if the node to link to exists
    */
    if (db_find(&(args->node1.ID->dbkey), &res) != 0)   {
	log_warn(mds_debug_level, "DB mds_link_db() Can't find entry %s to link to",
	    ID2str(&(args->node1.ID->dbkey)));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    /*
    ** Make sure the directory exists
    */
    new_dir_ID= args->node2.ID->dbkey;
    if (db_obj_exists(&new_dir_ID, &isdir) != 0)   {
	log_error(mds_debug_level, "DB mds_link_db() destination dir doesn't exist");
	return result->ret= LWFS_ERR_MDS_EXIST;
    }

    if (!isdir)   {
	log_error(mds_debug_level, "DB mds_link_db() destination is not a directory");
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    /*
    ** Check if the new name in the new directory is valid
    */
    if (db_find_name(&new_dir_ID, *(args->name), &res) == 0)   {
	log_warn(mds_debug_level, "DB mds_link_db() link name \"%s\" already exists",
	    *(args->name));
	return result->ret= LWFS_ERR_MDS_EXIST;
    }


    /*
    ** Create the link
    */
    mds_args_Nn_t args2;

    args2.node= args->node2;
    args2.name= args->name;
    rc= db_enter_new(&args2, MDS_LNK, args->node1.ID->dbkey, &(result->node.dbkey));
    if (rc != 0)   {
	log_error(mds_debug_level, " DB %s can't be entered", *(args->name));
	/* FIXME: This may not be the correct code in all cases! */
	return result->ret= LWFS_ERR_MDS_EXIST;
    }



    /*
    ** Increment the link counter
    */
    update_link_cnt(&(args->node1.ID->dbkey), 1);


    return result->ret= LWFS_OK;

}  /* end of mds_link_db() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Get the attributes of an object

    @param args		the structure containing the object ID.
    @param result	the attributes.
    @return		LWFS_OK if successfull, LWFS_ERR_MDS_NOENT otherwise.

    This function returns the user defined aread of the entry.

*/
lwfs_return_code_t
mds_getattr_db(mds_node_args_t *args, mds_res_note_rc_t *result)
{

lwfs_did_t obj_ID;


    log_debug(mds_debug_level, " DB mds_getattr_db() ------------------------------------");

    /* Get the user data */
    if (db_getuser(&obj_ID, result) != 0)   {
	log_error(mds_debug_level, " DB can't get user data!");
	return result->ret= LWFS_ERR_MDS_NOENT;
    }

    return result->ret= LWFS_OK;

}  /* end of mds_getattr_db() */



/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Read a directory

    @param args		the structure containing the directory ID.
    @param result	The same result code as returned by the function.
    @return		LWFS_OK if successfull.

    Read the contents of a directory

*/
lwfs_return_code_t
mds_readdir_db(mds_args_N_t *args, mds_readdir_res_t *result)
{

lwfs_did_t obj_ID;
int isdir;
int rc;


    log_debug(mds_debug_level, " DB mds_readdir_db() ------------------------------------");

    /* Make sure it is a directory */
    obj_ID= args->node.ID->dbkey;
    if (db_obj_exists(&obj_ID, &isdir) != 0)   {
	log_error(mds_debug_level, " DB object doesn't exist!");
	result->start= NULL;
	return result->return_code= LWFS_ERR_MDS_EXIST;
    }

    if (!isdir)   {
	log_error(mds_debug_level, " DB object is not a directory!");
	result->start= NULL;
	return result->return_code= LWFS_ERR_MDS_ISDIR;
    }

    rc= db_build_dir_list(&obj_ID, &(result->start));
    if (rc == 0)   {
	result->return_code= LWFS_OK;
    } else   {
	result->return_code= LWFS_MDS_INT_DB;
    }

    return result->return_code;

}  /* end of mds_readdir_db() */
