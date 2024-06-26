/**
    @file mds_db.h
    @brief Functions to enter, update, and find objects in the database.
    
    The functions in this file implement the basic file system
    primitives, such as inserting an object into a directory.
    The actual access to the database occurs in mds_db_access.c

    @author Rolf Riesen (rolf\@cs.sandia.gov)
    $Revision: 791 $
    $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $

*/

#ifndef _MDS_DB_H
#define _MDS_DB_H

#include "mds_xdr.h"


    extern void mds_exit_db(void);

    extern lwfs_return_code_t mds_init_db(char *db_file_name, int dbclear,
	int dbrecover);

    extern lwfs_return_code_t mds_enter_db(mds_args_Nn_t *args, mds_otype_t otype,
	mds_res_N_rc_t *result);

    extern lwfs_return_code_t mds_lookup_db(mds_args_N_t *args,
	mds_res_note_rc_t *result);

    extern lwfs_return_code_t mds_lookup_name_db(mds_args_Nn_t *args,
	mds_res_N_rc_t *result);

    extern lwfs_return_code_t mds_delobj_db(mds_node_args_t *args,
	mds_res_note_rc_t *result);

    extern lwfs_return_code_t mds_rmdir_db(mds_args_N_t *args,
	mds_res_note_rc_t *result);

    extern lwfs_return_code_t mds_rename_db(mds_args_NNn_t *args,
	mds_res_N_rc_t *result);

    extern lwfs_return_code_t mds_link_db(mds_args_NNn_t *args, mds_res_N_rc_t *result);

    extern lwfs_return_code_t mds_getattr_db(mds_node_args_t *args,
	mds_res_note_rc_t *result);

    extern lwfs_return_code_t mds_readdir_db(mds_args_N_t *args, mds_readdir_res_t *result);

    extern char *ID2str(lwfs_did_t *ID);

#endif /* _MDS_DB_H */
