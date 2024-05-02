/**
    @file mds_db_access.h
    @brief The functions in this file interact with the database
	that stores the metadata.

    @author Rolf (rolf\@cs.sandia.gov)
    $Revision: 791 $
    $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $

*/

#ifndef _MDS_DB_ACCESS_H
#define _MDS_DB_ACCESS_H

#include "mds_xdr.h"


    extern int db_enter_new(mds_args_Nn_t *args, mds_otype_t otype, lwfs_did_t link,
	lwfs_did_t *res);

    extern int db_find(lwfs_did_t *ID, mds_res_N_rc_t *result);

    extern int db_find_name(lwfs_did_t *parent_ID, char *name,
	mds_res_N_rc_t *result);

    extern int db_obj_exists(lwfs_did_t *ID, int *isdir);

    extern int db_delobj(lwfs_did_t *ID);

    extern int db_getuser(lwfs_did_t *ID, mds_res_note_rc_t *result);

    extern int is_dir_empty(lwfs_did_t *ID);

    extern int db_update_name(lwfs_did_t *ID, char *name);

    extern int db_move_obj(lwfs_did_t *obj, lwfs_did_t *dir);

    extern int update_link_cnt(lwfs_did_t *ID, int value);

    extern int db_build_dir_list(lwfs_did_t *ID, dlist *start);

#endif /* _MDS_DB_ACCESS_H */
