/**
    @file mds_db_access.c
    @brief The functions in this file interact with the database
	    that stores the metadata.

    @author Rolf Riesen (rolf\@cs.sandia.gov)
    $Revision: 1441 $
    $Date: 2007-05-21 08:15:36 -0600 (Mon, 21 May 2007) $

*/

#include <stdio.h>
#include <limits.h>
#include <db.h>
#include "logger/logger.h"
#include "lwfs_xdr.h"
#include "mds/mds_xdr.h"
#include "mds/mds_db.h"
#include "mds/mds_db_access.h"
#include "mds/mds_debug.h"


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/*
** Some definitions needed in this file
*/
#define GLOBAL_DB_KEY			"The latest generated ID!"
#define SECONDARY_NAME_EXTENSION	".2nd"
#define TERTIARY_NAME_EXTENSION		".3rd"


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/*
** These are the data structures we use to access the database
*/

/**
    @struct dbentry_t
    @brief This is the structure used for every entry in the database

*/
typedef struct dbentry_t   {
    char name[LWFS_NAME_LEN];	/**< The name of the object. */
    lwfs_did_t parent_ID;		/**< The unique database key for the directory
				    this object is in. */
    lwfs_did_t my_ID;		/**< The unique database key for this object.
				    We need it here, so we can translate a
				    primary key/entry pair into a key for the
				    seocondary database. */
    mds_otype_t otype;		/**< This entry is either a directory (MDS_DIR),
				    or an object (MDS_REG). */
    lwfs_did_t link;		/**< If this is a link, the node it points to. */
    int link_cnt;		/**< How many links point to us */
    int deleted;		/**< Is this node deleted (but still linked to)? */
    lwfs_obj_ref_t dir;		/**< stores things like the ssid, vid, etc. */
    lwfs_cap_t cap;		/**< @todo We may not need to store the capability here. */
    char attr[DB_ATTR_SIZE];	/**< User defined attributes */
} dbentry_t;


/**
    @struct key1_t
    @brief Primary key structure

    This is the key we use to access the primary database. It is of the
    form <parent ID><name>, e.g. 00000000000000000000000000000001/"object name"

*/
typedef struct key1_t   {
    lwfs_did_t ID;
    char entry_name[LWFS_NAME_LEN];
} key1_t;


/**
    @struct key2_t
    @brief Secondary key structure

    This is the key we use to access the secondary database. It is of the form
    <object ID>, e.g. 00000000000000000000000000000002
    This is the unique, unchanging ID of an object. We can always find it
    using this ID. Key1_t changes when an object is renamed, and key_3_t
    changes when an object is moved to another directory.

*/
typedef struct key2_t   {
    uint32_t ID_p1;
    uint32_t ID_p2;
    uint32_t ID_p3;
    uint32_t ID_p4;
} key2_t;


/**
    @struct key3_t
    @brief Tertiary key structure

    This is the key we use to access the tertiary database. It is of the form
    <parent ID>, e.g. 00000000000000000000000000000001. Note that this is the
    same format as used in key2_t. However, here we list the directory ID in
    which this entry resides. Therefore, keys can be duplicated in the tertiary
    database, since more than one object can reside inside the same directory.

    Searching the tertiary database for a given directory ID, will gives us
    all the objects that reside in that directory.
*/
typedef struct key3_t   {
    uint32_t parent_p1;
    uint32_t parent_p2;
    uint32_t parent_p3;
    uint32_t parent_p4;
} key3_t;


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    Local "globals"
*/
static DB *dbp1;	/**< The primary database pointer */
static DB *dbp2;	/**< The secondary database pointer */
static DB *dbp3;	/**< The tertiary database pointer */
static lwfs_did_t link_orphan_ID;	/**< The node ID of the directory where we keep the link orphans */


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/*
** Local functions
*/
static int pto2nd(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey);
static int pto3rd(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey);
static void new_dbkey(lwfs_did_t *ID);
static int put_db_entry(key1_t *key1, dbentry_t *dbentry, u_int32_t flags);
static int get_db_entry1(key1_t *key1, dbentry_t **dbentry);
static int get_db_entry2(key2_t *key2, dbentry_t **dbentry);


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @addtogroup db_associate Associating three databases with each other
    The functions pto2nd() and pto3rd() are needed by the sleepycat database
    to maintain the secondary and tertiary database when we make accesses to
    the primary database.
    @{
*/

/**
    @brief Create a secondary key from a primary key/data pair.

    @param dbp		The primary database pointer.
    @param pkey		The key for the primary database.
    @param pdata	The data accessed by the primary key.
    @param skey		The calculated key into the secondary database.
    @return		0 if successful, and DB_DONOTINDEX if the
			primary key is for the entry where we store
			the last generated unqique database key.

    This is a mapping from the primary database into the second one.
    Sleepycat calls it with the primary key and data item and
    expects this function to return a key into the second database.

    Each entry contains its own unique ID. All this function does,
    is retrieve that ID. It is what is needed as a key into the
    second database.

    We generate a unique key for each entry in the database. We
    store the most recently generated key in the database itself.
    When this function gets called with that particular entry,
    it returns DB_DONOTINDEX, because we do not need to store it
    in the secondary or tertiary database.
*/
static int
pto2nd(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{

    memset(skey, 0, sizeof(DBT));

    if (strcmp(pkey->data, GLOBAL_DB_KEY) == 0)   {
	/* The current ID is not in the secondary database */
	return DB_DONOTINDEX;
    }

    skey->data= &((dbentry_t *)pdata->data)->my_ID;
    skey->size= sizeof(lwfs_did_t);

    {
    key1_t *key1= pkey->data;
    key2_t *key2= skey->data;

	log_debug(mds_debug_level, " ===> %s/\"%s\" -> %s",
	    ID2str(&(key1->ID)), key1->entry_name, ID2str((lwfs_did_t *)key2));
    }

    return 0;

}  /* end of pto2nd() */


/**
    @brief Create a tertiary key from a primary key/data pair.

    @param dbp		The primary database pointer.
    @param pkey		The key for the primary database.
    @param pdata	The data accessed by the primary key.
    @param skey		The calculated key into the secondary database.
    @return		0 if successful, and DB_DONOTINDEX if the
			primary key is for the entry where we store
			the last generated unqique database key.

    This is a mapping from the primary database into the tertiary
    one.  Sleepycat calls it with the primary key and data item and
    expects this function to return a key into the tertiary database.

    Each entry contains the ID of its parent; the directory the
    object is in.  All this function does, is retrieve that ID. It
    is what is needed as a key into the tertirary database.

    We generate a unique key for each entry in the database. We
    store the most recently generated key in the database itself.
    When this function gets called with that particular entry,
    it returns DB_DONOTINDEX, because we do not need to store it
    in the secondary or tertiary database.
*/
static int
pto3rd(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{

    memset(skey, 0, sizeof(DBT));

    if (strcmp(pkey->data, GLOBAL_DB_KEY) == 0)   {
	/* The current ID is not in the tertiary database */
	return DB_DONOTINDEX;
    }

    skey->data= &((dbentry_t *)pdata->data)->parent_ID;
    skey->size= sizeof(lwfs_did_t);

    {
    key1_t *key1= pkey->data;
    key3_t *key3= skey->data;

	log_debug(mds_debug_level, " ===> %s/\"%s\" -> %s",
	    ID2str(&(key1->ID)), key1->entry_name, ID2str((lwfs_did_t *)key3));
    }

    return 0;

}  /* end of pto3rd() */
/** @} */ /* and of group db_associate */


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @brief Initialize the database.
    
    If dbclear is set, then the database is erased and only an
    entry for "/", the top level root directory, will be present.
    If dbrecover is set, and attempt is made to recover a database
    that has crashed.

*/
lwfs_return_code_t
mds_init_db(char *db_file_name, int dbclear, int dbrecover)
{

int rc;
char *db_file_name2;
char *db_file_name3;
struct lwfs_did_t notused;


    log_debug(mds_debug_level, " ===> mds_init_db(\"%s\", clear= %d, recover= %d)"
	" ------------------------------------", db_file_name, dbclear,
	dbrecover);

    /* Create the file name for the secondary DB */
    db_file_name2= malloc(strlen(db_file_name) + strlen(SECONDARY_NAME_EXTENSION) + 1);
    if (db_file_name2 == NULL)   {
	log_fatal(mds_debug_level, " ===> Out of memory");
	return LWFS_MDS_INT_DB;
    }
    strcpy(db_file_name2, db_file_name);
    strcat(db_file_name2, SECONDARY_NAME_EXTENSION);


    /* Create the file name for the tertiary DB */
    db_file_name3= malloc(strlen(db_file_name) + strlen(TERTIARY_NAME_EXTENSION) + 1);
    if (db_file_name3 == NULL)   {
	log_fatal(mds_debug_level, " ===> Out of memory");
	free(db_file_name2);
	return LWFS_MDS_INT_DB;
    }
    strcpy(db_file_name3, db_file_name);
    strcat(db_file_name3, TERTIARY_NAME_EXTENSION);


    if (dbclear)   {
	if (dbrecover)   {
	    log_debug(mds_debug_level, " ===> mds_init_db(\"%s\", clear, "
		"recover)", db_file_name);
	    /*
	    ** Recovering a database that we are about to erase doesn't
	    ** make much sense...
	    */
	    log_error(mds_debug_level, " ===> Cannot specify -dbclear and "
		"-dbrecover together!");
	    free(db_file_name2);
	    free(db_file_name3);
	    return LWFS_MDS_INT_DB;
	} else   {
	    log_debug(mds_debug_level, " ===> mds_init_db(\"%s\", clear, "
		"norecover)", db_file_name);
	}
	remove(db_file_name);
	remove(db_file_name2);
	remove(db_file_name3);
    } else   {
	if (dbrecover)   {
	    log_debug(mds_debug_level, " ===> mds_init_db(\"%s\", noclear, "
		"recover)", db_file_name);
	} else   {
	    log_debug(mds_debug_level, " ===> mds_init_db(\"%s\", noclear, "
		"norecover)", db_file_name);
	}
    }


    /*
    ** Open the primary database
    */
    rc= db_create(&dbp1, NULL, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> db_create() primary DB %s",
	    db_strerror(rc));
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp1->set_pagesize(dbp1, 4 * 1024);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_pagesize() primary DB %s",
	    db_strerror(rc));
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp1->set_cachesize(dbp1, 0, 32 * 1024, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_cachesize() primary DB %s",
	    db_strerror(rc));
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp1->open(dbp1, NULL, db_file_name, NULL, DB_HASH, DB_CREATE, 0664);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> open() primary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }


    /*
    ** Open the secondary database
    */
    rc= db_create(&dbp2, NULL, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> db_create() secondary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp2->set_pagesize(dbp2, 4 * 1024);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_pagesize() secondary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp2->set_cachesize(dbp2, 0, 32 * 1024, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_cachesize() secondary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp2->open(dbp2, NULL, db_file_name2, NULL, DB_HASH, DB_CREATE, 0664);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> open() secondary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }


    /*
    ** Open the tertiary database
    */
    rc= db_create(&dbp3, NULL, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> db_create() tertiary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp3->set_pagesize(dbp3, 4 * 1024);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_pagesize() tertiary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp3->set_cachesize(dbp3, 0, 32 * 1024, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_cachesize() tertiary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    /*
    ** Keys in the tertiary database may be duplicated. The keys here are
    ** the IDs of parent directories. Many objects may share a parent.
    */
    rc= dbp3->set_flags(dbp3, DB_DUP | DB_DUPSORT);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> set_flags() tertiary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }

    rc= dbp3->open(dbp3, NULL, db_file_name3, NULL, DB_HASH, DB_CREATE, 0664);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> open() tertiary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	dbp3->close(dbp3, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }


    /* Associate the secondary DB with the primary one */
    rc= dbp1->associate(dbp1, NULL, dbp2, pto2nd, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> associate() secondary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	dbp3->close(dbp3, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }


    /* Associate the tertiary DB with the primary one */
    rc= dbp1->associate(dbp1, NULL, dbp3, pto3rd, 0);
    if (rc != 0)   {
	log_fatal(mds_debug_level, " ===> open() tertiary DB %s",
	    db_strerror(rc));
	dbp1->close(dbp1, 0);
	dbp2->close(dbp2, 0);
	dbp3->close(dbp3, 0);
	free(db_file_name2);
	free(db_file_name3);
	return LWFS_MDS_INT_DB;
    }



    if (dbclear)   {
	/**
	** Initialize the current ID and store it in the database. This entry
	** is used by new_dbkey() to create unique IDs for each object. The
	** initial ID has all bits set, so it becomes 00000000000000000000000000000000
	** when we call new_dbkey() the first time to enter "/" into the
	** database.
	*/
	lwfs_did_t dbkey= {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
	DBT key, data;

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data= GLOBAL_DB_KEY;
	key.size= strlen(key.data);
	data.data= (void *)&dbkey;;
	data.size= sizeof(dbkey);

	rc= dbp1->put(dbp1, NULL, &key, &data, DB_NOOVERWRITE);

	if (rc != 0)   {
	    log_error(mds_debug_level, " ===> mds_init_db() %s", db_strerror(rc));
	    free(db_file_name2);
	    free(db_file_name3);
	    return LWFS_MDS_INT_DB; 
	}


	/**
	** Create the root entry "/". The key for root is always
	** 00000000000000000000000000000000. Since this will be the first time
	** we call new_dbkey(), it will return that special ID.
	** There is no parent, so we always set that to
	** 00000000000000000000000000000000 as well.
	*/
	lwfs_did_t root_ID= {0x00000000, 0x00000000, 0x00000000, 0x00000000};
	mds_args_Nn_t args;
	lwfs_obj_ref_t dir;
	mds_name_t mname= "/";
	lwfs_did_t res;

	args.name= &mname;
	args.node.ID= &dir;
	args.node.ID->dbkey= root_ID;
	rc= db_enter_new(&args, MDS_DIR, notused, &res);

	if (rc != 0)   {
	    log_error(mds_debug_level, " ===> mds_init_db() %s", db_strerror(rc));
	    /* FIXME: This may not be the correct code in all cases! */
	    free(db_file_name2);
	    free(db_file_name3);
	    return LWFS_ERR_MDS_EXIST; 
	}



	/**
	** Now create the link orphan directory. This is where we "hide" all
	** the nodes that have been deleted, but whose link count has not
	** reached zero yet.
	*/
	lwfs_did_t orphan_ID= {0x00000000, 0x00000000, 0x00000000, 0x00000001};
	mds_args_Nn_t orphanargs;
	lwfs_obj_ref_t orphandir;
	mds_name_t orphanname= "__link_orphan_directory__";

	orphanargs.name= &orphanname;
	orphanargs.node.ID= &orphandir;
	orphanargs.node.ID->dbkey= orphan_ID;
	rc= db_enter_new(&orphanargs, MDS_DIR, notused, &link_orphan_ID);

	if (rc != 0)   {
	    log_error(mds_debug_level, " ===> mds_init_db() mkdir link orphan "
		"failed %s", db_strerror(rc));
	    /* FIXME: This may not be the correct code in all cases! */
	    free(db_file_name2);
	    free(db_file_name3);
	    return LWFS_ERR_MDS_EXIST; 
	}

    }

    free(db_file_name2);
    free(db_file_name3);
    return LWFS_OK;

}  /* end of mds_init_db() */


/**
    @brief This function closes the three databases properly.
*/
void
mds_exit_db(void)
{

    log_debug(mds_debug_level, " ===> mds_db_exit() ------------------------------------");
    dbp1->close(dbp1, 0);
    dbp2->close(dbp2, 0);
    dbp3->close(dbp3, 0);

}  /* end of mds_exit_db() */


/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/**
    @addtogroup db_access Database access functions

    These functions are the ones that actually access the
    database. Except for mds_init_db() and mds_exit_db(), these
    functions are called from the more general functions in mds_db.c.

    @{
*/

/**
    @brief Create a new node and enter into the database.
    @return 0, if successful; -1 otherwise.
*/
int
db_enter_new(mds_args_Nn_t *args, mds_otype_t otype, lwfs_did_t link, lwfs_did_t *res)
{

int rc;
key1_t key1;
dbentry_t new;


    /*
    ** Build a database entry named "new" and fill it with the
    ** data passed in.
    */
    strncpy(new.name, *(args->name), LWFS_NAME_LEN);

    /* FIXME: We need to set these to the correct values */
    memset(&new.cap, 0, sizeof(lwfs_cap_t));
    memset(&new.dir.ssid, 0, sizeof(lwfs_ssid_t));
    memset(&new.dir.vid, 0, sizeof(lwfs_vid_t));
    memset(&new.attr, 0, DB_ATTR_SIZE);
    lwfs_set_oid_zero(&new.dir.oid);

    new_dbkey(&new.my_ID);
    new.dir.dbkey= new.my_ID;
    new.parent_ID= args->node.ID->dbkey;
    new.otype= otype;
    new.link= link;
    new.link_cnt= 0;
    new.deleted= 0;

    /*
    ** Setup the key. The memset is important to make sure that all
    ** bytes after the name are zeroed out. Garbage after the \0 still
    ** counts as part of the key into the database,
    */
    memset(&key1, 0, sizeof(key1_t));
    strncpy(key1.entry_name, *(args->name), LWFS_NAME_LEN);
    key1.ID= args->node.ID->dbkey;
    log_info(mds_debug_level, " ===> db_enter_new() key1 is %s/\"%s\"",
	ID2str((lwfs_did_t *)&(key1.ID)), key1.entry_name);

    /*
    ** The following put adds the entry in all three databases, which
    ** means we can get to the data through the primary database and
    ** a <parent ID><name> key, and its own ID in the secondary database.
    ** By adding an entry to the tertiary database, we automatically
    ** entered this object in its parent directory.
    */
    rc= put_db_entry(&key1, &new, DB_NOOVERWRITE);

    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_enter_new(%s) put new failed! %s",
	    *(args->name), db_strerror(rc));
	return -1; 
    }

    *res= new.my_ID;

    return 0;

}  /* end of db_enter_new() */


/**
    @brief Find an entry by its secondary database ID

    Given an ID, find the corresponding object. If found, fill in
    the dbentry structure.

    @return 0 if the object exists, -1 otherwise.
*/
int
db_find(lwfs_did_t *ID, mds_res_N_rc_t *result)
{

int rc;
dbentry_t *dbentry;


    log_info(mds_debug_level, " ===> db_find() Looking up key %s", ID2str(ID));

    rc= get_db_entry2((key2_t *)ID, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_find() %s", db_strerror(rc));
	return -1;
    }


    /* FIXME: We need to add all these fields to dbentry_t and fill them in */
    /*
    result->oref.ssid= dbentry->ssid;
    result->oref.vid= dbentry->vid;
    result->oref.oid= dbentry->oid;
    */
    result->node.type= dbentry->otype;
    result->node.links= dbentry->link_cnt;
    result->node.linkto= dbentry->link;
    result->node.dbkey= dbentry->my_ID;
    log_info(mds_debug_level, " ===> db_find() name = %s", dbentry->name);

    free(dbentry);
    return 0;

}  /* end of db_find() */


/**
    @brief Find an entry by its name and the database ID of its parent

    Given a name and a parent ID, find the corresponding object. If
    found, fill in the dbentry structure.

    @return 0 if the object exists, -1 otherwise.
*/
int
db_find_name(lwfs_did_t *parent_ID, char *name, mds_res_N_rc_t *result)
{

int rc;
dbentry_t *dbentry;
key1_t key1;


    log_info(mds_debug_level, " ===> db_find_name() Looking up name \"%s\"",
	name);

    memset(&key1, 0, sizeof(key1_t));
    strncpy(key1.entry_name, name, LWFS_NAME_LEN);
    key1.ID= *parent_ID;
    rc= get_db_entry1(&key1, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_find_name() %s", db_strerror(rc));
	return -1;
    }



    /* FIXME: We need to add all these fields to dbentry_t and fill them in */
    /*
    result->oref.ssid= dbentry->ssid;
    result->oref.vid= dbentry->vid;
    result->oref.oid= dbentry->oid;
    */
    result->node.type= dbentry->otype;
    result->node.links= dbentry->link_cnt;
    result->node.linkto= dbentry->link;
    result->node.dbkey= dbentry->my_ID;
    log_info(mds_debug_level, " ===> db_find_name() name = %s", dbentry->name);

    free(dbentry);
    return 0;

}  /* end of db_find_name() */



/**
    @brief Change the name of an object

    Given an ID, retrieve and delete an object, update its name,
    and re-enter it in the database. This changes the primary key
    for this object.

    @return 0 if successful, -1 otherwise.
*/
int
db_update_name(lwfs_did_t *ID, char *name)
{

int rc;
dbentry_t *dbentry;
key1_t key1;


    log_info(mds_debug_level, " ===> db_update_name() for key %s to "
	"name \"%s\"", ID2str(ID), name);

    /* Get it */
    rc= get_db_entry2((key2_t *)ID, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_update_name() get: %s",
	    db_strerror(rc));
	return -1;
    }

    /* Delete it */
    if (db_delobj(ID) != 0)   {
	log_error(mds_debug_level, " ===> db_update_name() del: %s",
	    db_strerror(rc));
	free(dbentry);
	return -1;
    }

    /* Change its name */
    strncpy(dbentry->name, name, LWFS_NAME_LEN);

    /* Insert it */
    strncpy(key1.entry_name, name, LWFS_NAME_LEN);
    key1.ID= dbentry->parent_ID;
    rc= put_db_entry(&key1, dbentry, DB_NOOVERWRITE);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_update_name() put: %s",
	    db_strerror(rc));
	free(dbentry);
	return -1;
    }

    free(dbentry);
    return 0;

}  /* end of db_update_name() */



/**
    @brief Move an object from one directory to another

    Given an ID, retrieve and delete an object, update its parent ID,
    and re-enter it in the database.
    If the object is already in the correct directory, the deletion
    and reinsertion is not done. If it is done, then the primary and
    tertiary keys of the object are changed.

    @return 0 if successful, -1 otherwise.
*/
int
db_move_obj(lwfs_did_t *obj, lwfs_did_t *dir)
{

int rc;
dbentry_t *dbentry;
key1_t key1;
key3_t key3;


    log_info(mds_debug_level, " ===> db_move_obj() for key %s to key %s",
	ID2str(obj), ID2str(dir));

    /* Get it */
    rc= get_db_entry2((key2_t *)obj, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_move_obj() get: %s",
	    db_strerror(rc));
	return -1;
    }

    if ((dbentry->parent_ID.part1 == dir->part1) &&
	    (dbentry->parent_ID.part2 == dir->part2) &&
	    (dbentry->parent_ID.part3 == dir->part3) &&
	    (dbentry->parent_ID.part4 == dir->part4))   {
	/* No need to move it */
	log_info(mds_debug_level, " ===> db_move_obj() key %s is already in "
	    "correct directory.", ID2str(obj));
	free(dbentry);
	return 0;
    }

    /* Delete it */
    if (db_delobj(obj) != 0)   {
	log_error(mds_debug_level, " ===> db_move_obj() del: %s",
	    db_strerror(rc));
	free(dbentry);
	return -1;
    }

    /* Change its parent ID */
    dbentry->parent_ID= *dir;

    /* Insert it */
    strncpy(key1.entry_name, dbentry->name, LWFS_NAME_LEN);
    key1.ID= dbentry->parent_ID;
    key3.parent_p1= dbentry->parent_ID.part1;
    key3.parent_p2= dbentry->parent_ID.part2;
    key3.parent_p3= dbentry->parent_ID.part3;
    key3.parent_p4= dbentry->parent_ID.part4;
    rc= put_db_entry(&key1, dbentry, DB_NOOVERWRITE);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_move_obj() put: %s",
	    db_strerror(rc));
	free(dbentry);
	return -1;
    }

    free(dbentry);
    return 0;

}  /* end of db_move_obj() */



/**
    @brief See if an object exists and return whether it is a directory or not

    Given an ID, find out whether the object exists and is a directory or not.

    @return 0 if the object exists, -1 otherwise.
*/
int
db_obj_exists(lwfs_did_t *ID, int *isdir)
{

dbentry_t *parent;
int rc;


    log_info(mds_debug_level, " ===> db_obj_exists() Looking up key %s",
	ID2str(ID));

    rc= get_db_entry2((key2_t *)ID, &parent);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_obj_exists() %s", db_strerror(rc));
	free(parent);
	return -1;
    }

    log_info(mds_debug_level, " ===> db_obj_exists() name = %s", parent->name);

    if (parent->otype == MDS_DIR)   {
	*isdir= TRUE;
    } else   {
	*isdir= FALSE;
    }

    free(parent);
    return 0;

}  /* end of db_obj_exists() */



/**
    @brief Delete an object in the database.

    Given an ID, remove the corresponding object from the database.

    @return 0 if successful, -1 otherwise.
*/
int
db_delobj(lwfs_did_t *ID)
{

int rc;
DBT key;
dbentry_t *dbentry;
key1_t key1;


    log_info(mds_debug_level, " ===> db_delobj() deleting key %s", ID2str(ID));

    /* Get the entry so we can check the link count */
    rc= get_db_entry2((key2_t *)ID, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_delobj() %s", db_strerror(rc));
	return -1;
    }


    /* Now delete it */
    memset(&key, 0, sizeof(DBT));
    key.data= ID;
    key.size= sizeof(key2_t);

    rc= dbp2->del(dbp2, NULL, &key, 0);
    log_info(mds_debug_level, " ===> db_delobj() deleted %s", ID2str(ID));
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_delobj() %s", db_strerror(rc));
	free(dbentry);
	return -1;
    }


    /* Check the link count and move it to orphans, if not 0 */
    if (dbentry->link_cnt > 0)   {
	log_info(mds_debug_level, " ===> db_delobj() moving %s to orphans",
	    ID2str(ID));
	strncpy(key1.entry_name, ID2str(ID), LWFS_NAME_LEN);
	key1.ID= link_orphan_ID;
	dbentry->deleted= 1;
	dbentry->parent_ID= link_orphan_ID;
	rc= put_db_entry(&key1, dbentry, DB_NOOVERWRITE);
	free(dbentry);
	if (rc != 0)   {
	    log_error(mds_debug_level, " ===> db_delobj() put to link_orphan "
		"failed: %s", db_strerror(rc));
	    return -1;
	}
	return 0;
    } else if ((dbentry->link.part1 != 0) || (dbentry->link.part2 != 0) ||
	    (dbentry->link.part3 != 0) || (dbentry->link.part4 != 0))   {
	/* If we are removing a link, decrement the target link count */
	log_info(mds_debug_level, " ===> db_delobj() %s is a link to %s",
	    ID2str(ID), ID2str(&(dbentry->link)));
	update_link_cnt(&(dbentry->link), -1);
    }

    free(dbentry);
    return rc;

}  /* end of db_delobj() */



/**
    @brief See if there are any entries left inside a directory
    @param ID		The directory ID

    @return 0 if the directory is empty, -1 otherwise.

    If we can find the directory ID in the tertiary database,
    then that means that some objects are still present inside
    this directory.

    We do not need to check the individual entries. If they
    were deleted, they would be in the link orphan directory,
    which is never empty because it points to itself.
*/
int
is_dir_empty(lwfs_did_t *ID)
{

DBT key;
DBT data;
int rc;


    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data= ID;
    key.size= sizeof(key3_t);

    log_debug(mds_debug_level, " ===> is_dir_empty() retrieving key \"%s\"",
	ID2str(ID));

    rc= dbp3->get(dbp3, NULL, &key, &data, 0);
    log_debug(mds_debug_level, " ===> is_dir_empty() rc= %d", rc);

    if (rc == 0)   {
	/* Found an entry. Directory is not empty! */
	/* FIXME: Why do we not have to free data.data? When we do, we
	** seg fault sometime. */
	rc= -1;
    } else   {
	/* Nothing found. Directory is empty. */
	rc= 0;
    }
    return rc;

}  /* end of is_dir_empty() */


/**
    @brief Update the link counter of a node
    @return new value of counter, or -1 if error

    If the link count reaches 0, then the node is removed.
*/
int
update_link_cnt(lwfs_did_t *ID, int value)
{
int rc;
int new_value;
dbentry_t *dbentry;
DBT key, data;


    log_info(mds_debug_level, " ===> update_link_cnt() adding %d to %s",
	value, ID2str(ID));

    rc= get_db_entry2((key2_t *)ID, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> update_link_cnt() %s", db_strerror(rc));
	return -1;
    }
    log_info(mds_debug_level, " ===> update_link_cnt() old value of %s is %d",
	ID2str(ID), dbentry->link_cnt);

    dbentry->link_cnt += value;
    if (dbentry->link_cnt < 0)   {
	dbentry->link_cnt= 0;
    }


    /* Delete the entry */
    memset(&key, 0, sizeof(DBT));
    key.data= ID;
    key.size= sizeof(key2_t);

    rc= dbp2->del(dbp2, NULL, &key, 0);
    if (rc != 0)   {
	log_info(mds_debug_level, " ===> update_link_cnt() delete %s failed",
	    ID2str(ID));
    } else   {
	log_info(mds_debug_level, " ===> update_link_cnt() deleted %s",
	    ID2str(ID));
    }


    /* Put the entry back in, if necessary */
    if ((dbentry->link_cnt > 0) || !dbentry->deleted)   {
	log_info(mds_debug_level, " ===> update_link_cnt() putting %s back",
	    ID2str(ID));
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key1_t key1;

	strncpy(key1.entry_name, dbentry->name, LWFS_NAME_LEN);
	key1.ID= dbentry->parent_ID;

	key.data= &key1;
	key.size= sizeof(key1_t);
	data.data= (void *)dbentry;
	data.size= sizeof(dbentry_t);
	rc= dbp1->put(dbp1, NULL, &key, &data, 0);
	if (rc != 0)   {
	    log_info(mds_debug_level, " ===> update_link_cnt() put %s failed",
		ID2str(ID));
	    return -1;
	}
    }

    new_value= dbentry->link_cnt;
    log_info(mds_debug_level, " ===> update_link_cnt() new value of %s is %d",
	ID2str(ID), new_value);
    free(dbentry);
    return new_value;

}  /* end of update_link_cnt() */



/**
    @brief Retrieve the user data of an object

    @return 0 if the object exists, -1 otherwise.
*/
int
db_getuser(lwfs_did_t *ID, mds_res_note_rc_t *result)
{

int rc;
dbentry_t *dbentry;


    log_info(mds_debug_level, " ===> db_getuser() Getting attr of key %s",
	ID2str(ID));

    rc= get_db_entry2((key2_t *)ID, &dbentry);
    if (rc != 0)   {
	log_error(mds_debug_level, " ===> db_getuser() %s", db_strerror(rc));
	return -1;
    }

    memcpy(&(result->note.data), dbentry->attr, DB_ATTR_SIZE);

    free(dbentry);
    return 0;

}  /* end of db_getuser() */



/**
    @brief Build a list of directory entries
    @param ID		The directory ID

    @return 0 if successful, -1 otherwise

    "start" is NULL, if no entries were found, or points to a list of
    entries, if there are some.
*/
int
db_build_dir_list(lwfs_did_t *ID, dlist *start)
{

DBT key;
DBT data;
int rc;
dlist new= NULL;
dlist first= NULL;
dlist current= NULL;
dbentry_t *dbentry;
DBC *DBcursor;
int again;
int flag;


    log_debug(mds_debug_level, " ===> db_build_dir_list() looking for entries "
	"in \"%s\"", ID2str(ID));

    rc= dbp3->cursor(dbp3, NULL, &DBcursor, 0);
    if (rc != 0)   {
	log_debug(mds_debug_level, " ===> db_build_dir_list() DB->cursor "
	    "error, %s\n", db_strerror(rc));
	*start= NULL;
	return -1;
    }


    /* Get the first item */
    flag= DB_SET;
    do {
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	key.data= ID;
	key.size= sizeof(key3_t);
	data.flags= DB_DBT_MALLOC;

	rc= DBcursor->c_get(DBcursor, &key, &data, flag);
	if (rc != 0)   {
	    log_debug(mds_debug_level, " ===> db_build_dir_list() "
		"DBcursor->cget(DB_SET) returned %s\n", db_strerror(rc));
	    /* This just means that the directory is empty */
	    *start= NULL;
	    rc= DBcursor->c_close(DBcursor);
	    if (rc != 0)   {
		log_debug(mds_debug_level, " ===> db_build_dir_list() DBcursor->c_close "
		    "error, %s\n", db_strerror(rc));
		return -1;
	    }
	    return 0;
	}

	dbentry= (dbentry_t *)data.data;
	if (dbentry == NULL)   {
	    log_debug(mds_debug_level, " ===> db_build_dir_list() No data "
		"returned\n");
	    *start= NULL;
	    rc= DBcursor->c_close(DBcursor);
	    if (rc != 0)   {
		log_debug(mds_debug_level, " ===> db_build_dir_list() DBcursor->c_close "
		    "error, %s\n", db_strerror(rc));
	    }
	    return -1;;
	}

	again= 0;
	if ((dbentry->my_ID.part1 == 0) && (dbentry->my_ID.part2 == 0) &&
		(dbentry->my_ID.part3 == 0) && (dbentry->my_ID.part4 == 0))   {
	    /* This is the root entry. Don't return it */
	    again= 1;
	    flag= DB_NEXT_DUP;
	    free(dbentry);
	}
    } while (again);


    /* The memory allocated here, will be freed by the XDR routines. */
    new= (dlist)malloc(sizeof(struct mds_entry_t));
    if (new == NULL)   {
	log_debug(mds_debug_level, " ===> db_build_dir_list() Out of memory\n");
	*start= NULL;
	rc= DBcursor->c_close(DBcursor);
	if (rc != 0)   {
	    log_debug(mds_debug_level, " ===> db_build_dir_list() DBcursor->c_close "
		"error, %s\n", db_strerror(rc));
	}
	return -1;
    }
    first= new;
    current= new;
    current->oref.type= dbentry->otype;
    current->oref.links= dbentry->link_cnt;
    current->oref.linkto= dbentry->link;
    current->oref.dbkey= dbentry->my_ID;
    current->name= strdup(dbentry->name);
    log_debug(mds_debug_level, "      db_build_dir_list() name %s\n",
	dbentry->name);
    current->next= NULL;

    free(dbentry);



    /*
    ** Now get the others
    ** We're working on the tertiary database which holds duplicate keys
    ** for all the entries that share a parent directory.
    */
    while (1)   {
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	key.data= ID;
	key.size= sizeof(key3_t);
	data.flags= DB_DBT_MALLOC;

	rc= DBcursor->c_get(DBcursor, &key, &data, DB_NEXT_DUP );
	if (rc != 0)   {
	    log_debug(mds_debug_level, " ===> db_build_dir_list() DBcursor->cget "
		"returned %s\n", db_strerror(rc));
	    break;
	}

	dbentry= (dbentry_t *)data.data;
	if (dbentry == NULL)   {
	    log_debug(mds_debug_level, " ===> db_build_dir_list() No data "
		"returned\n");
	    break;
	}

	if ((dbentry->my_ID.part1 == 0) && (dbentry->my_ID.part2 == 0) &&
		(dbentry->my_ID.part3 == 0) && (dbentry->my_ID.part4 == 0))   {
	    /* This is the root entry. Don't return it */
	    free(dbentry);
	    continue;
	}

	/* Found an entry. */
	log_debug(mds_debug_level, "      db_build_dir_list() Found an entry\n");
	new= (dlist)malloc(sizeof(struct mds_entry_t));
	if (new == NULL)   {
	    *start= first;
	    log_debug(mds_debug_level, " ===> db_build_dir_list() Out of memory\n");
	    rc= DBcursor->c_close(DBcursor);
	    if (rc != 0)   {
		log_debug(mds_debug_level, " ===> db_build_dir_list() DBcursor->c_close "
		    "error, %s\n", db_strerror(rc));
	    }
	    return -1;
	}
	current->next= new;
	current= current->next;

	current->oref.type= dbentry->otype;
	current->oref.dbkey= dbentry->my_ID;
	current->name= strdup(dbentry->name);
	log_debug(mds_debug_level, "      db_build_dir_list() name %s\n",
	    dbentry->name);
	current->next= NULL;

	free(dbentry);
    }

    *start= first;
    rc= DBcursor->c_close(DBcursor);
    if (rc != 0)   {
	log_debug(mds_debug_level, " ===> db_build_dir_list() DBcursor->c_close "
	    "error, %s\n", db_strerror(rc));
	return -1;
    }

    log_debug(mds_debug_level, "      db_build_dir_list() start is %p\n", *start);
    return 0;

}  /* end of db_build_dir_list() */




/* <<---->> <<---->> <<---->> <<---->> <-> <<---->> <<---->> <<---->> <<---->> */
/*

    @addtogroup db_access_local Locally used database access functions
    Local functions only used inside this file.
    @{
*/

/**
    @brief Create a new, unique database ID
    @param ID	The function returns the newly generated ID in this
		data structure.

    This function generates the next 128-bit key. We assign a unique
    key to each object stored in the MDS database. This function
    simply uses the last generated key and increments it by one to
    generate the new key.

    The last generated key is stored in the database itself.

    The function does not try to reuse IDs of entries that have
    been removed from the database.
*/
static void
new_dbkey(lwfs_did_t *ID)
{

DBT key, data;
lwfs_did_t *dbkey;
int rc;


    /* Get the previous key out of the database */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags= DB_DBT_MALLOC;
    key.data= GLOBAL_DB_KEY;
    key.size= strlen(key.data);

    rc= dbp1->get(dbp1, NULL, &key, &data, 0);
    dbkey= (lwfs_did_t *)data.data;

    log_debug(mds_debug_level, " ===> new_dbkey() current key \"%s\"",
	ID2str(dbkey));

    /* Increment the key by one */
    if (dbkey->part4 == UINT_MAX)   {
	dbkey->part4= 0;
	if (dbkey->part3 == UINT_MAX)   {
	    dbkey->part3= 0;
	    if (dbkey->part2 == UINT_MAX)   {
		dbkey->part2= 0;
		if (dbkey->part1 == UINT_MAX)   {
		    log_warn(mds_debug_level, " ===> new_dbkey() key wrap-around!");
		    dbkey->part1= 0;
		} else   {
		    dbkey->part1++;
		}
	    } else   {
		dbkey->part2++;
	    }
	} else   {
	    dbkey->part3++;
	}
    } else   {
	dbkey->part4++;
    }


    /* Give it to the caller */
    *ID= *dbkey;
    log_info(mds_debug_level, " ===> new_dbkey() new key \"%s\"",
	ID2str(dbkey));

    /* Store the new key in the database */
    dbp1->put(dbp1, NULL, &key, &data, 0);

    if (rc != 0)   {
	log_error(mds_debug_level, " ===> new_dbkey() %s", db_strerror(rc));
    }
    free(data.data);

} /* end of new_dbkey() */


/**
    @brief Insert an entry into the database
    @param key1		The primary key for this entry
    @param dbentry	The data to be stored
    @param flags	passed to the database put operation;
    			e.g. DB_NOOVERWRITE

    Using a primary key, this function enters a new entry into the
    database. If the key already exists, the operation will fail.

*/
static int
put_db_entry(key1_t *key1, dbentry_t *dbentry, u_int32_t flags)
{

DBT key, data;


    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data= key1;
    key.size= sizeof(key1_t);
    log_debug(mds_debug_level, " ===> put_db_entry() entering key %s/\"%s\"",
	ID2str((lwfs_did_t *)&(key1->ID)), key1->entry_name);

    data.data= (void *)dbentry;
    data.size= sizeof(dbentry_t);
    log_debug(mds_debug_level, " ===> put_db_entry() data.size %d", data.size);

    return dbp1->put(dbp1, NULL, &key, &data, flags);

}  /* end of put_db_entry() */



/**
    @brief Get a database entry based on primary key
    @param key1		The primary key
    @param dbentry	The entry data will be returned in here

    Given a primary key, retrieve the entry through the primary
    database.
*/
static int
get_db_entry1(key1_t *key1, dbentry_t **dbentry)
{

DBT key;
DBT data;
int rc;


    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data= key1;
    key.size= sizeof(key1_t);
    data.flags= DB_DBT_MALLOC;

    log_debug(mds_debug_level, " ===> get_db_entry1() retrieving key \"%s\"",
	key1->entry_name);

    rc= dbp1->get(dbp1, NULL, &key, &data, 0);
    *dbentry= (dbentry_t *)data.data;

    log_debug(mds_debug_level, " ===> get_db_entry1() retrieved size %d", data.size);

    return rc;

}  /* end of get_db_entry1() */



/**
    @brief Get a database entry based on it's secondary key
    @param key2		The secondary key
    @param dbentry	The entry data will be returned in here

    Given a secondary key, retrieve the entry through the secondary
    database.
*/
static int
get_db_entry2(key2_t *key2, dbentry_t **dbentry)
{

DBT key;
DBT data;
int rc;


    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data= key2;
    key.size= sizeof(key2_t);
    data.flags= DB_DBT_MALLOC;

    log_debug(mds_debug_level, " ===> get_db_entry2() retrieving key \"%s\"",
	ID2str((lwfs_did_t *)key2));

    rc= dbp2->get(dbp2, NULL, &key, &data, 0);
    *dbentry= (dbentry_t *)data.data;

    log_debug(mds_debug_level, " ===> get_db_entry2() retrieved size %d",
	data.size);

    return rc;

}  /* end of get_db_entry2() */

/** @} */ /* and of group db_access_local */
