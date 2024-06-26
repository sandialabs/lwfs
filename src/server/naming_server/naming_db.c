/**
 *   @file naming_db.c
 *
 *   @brief Database functions used by the naming service.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1558 $
 *   $Date: 2007-09-11 18:29:06 -0600 (Tue, 11 Sep 2007) $
 */
#include "config.h"

#include <db.h>
#include <unistd.h>

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif



#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/naming_common/naming_debug.h"
#include "support/ptl_uuid/ptl_uuid.h"

#include "naming_db.h"


/* ----------------- global variables and structs ------------------*/

static DB *dbp1;
static DB *dbp2;
static DB *dbp3;
static DB *dbp_inode;

static const char *GLOBAL_DB_KEY = "The latest generated ID";
static const char *DB2_NAME_EXTENSION = ".2nd";
static const char *DB3_NAME_EXTENSION = ".3rd";
static const char *INODE_NAME_EXTENSION = ".inode";

static lwfs_oid ORPHAN_OID;

/**
 * @brief Structure for the primary database key.
 */
typedef struct  {
	lwfs_oid parent_oid;
	char name[LWFS_NAME_LEN];
} db1_key;

/**
 * @brief Structure for the secondary database key.
 */
typedef struct  {
	lwfs_oid oid;
} db2_key;

/**
 * @brief Structure for the tertiary database key.
 */
typedef struct  {
	lwfs_oid parent_oid;
} db3_key;

/**
 * @brief Structure for the inode database key.
 */
typedef struct  {
	lwfs_oid inode_oid;
} inode_key;


/* ----------------- private method prototypes ----------------------*/

/* methods for all databases */
static int db_del(naming_db_entry *entry);

/* methods for db1 */

static const char *db1_keystr(
		const db1_key *key);

static int db1_put(
	const db1_key *key,
	const naming_db_dirent *dirent,
	const uint32_t flags);

static int db1_get(
	const db1_key *key,
	naming_db_dirent *dirent);


/* methods for db2 */

static int db2_keygen(
	const lwfs_oid *oid,
	db2_key *result);

static const char *db2_keystr(
		const db2_key *key);

static int db2_get(
	const db2_key *key,
	naming_db_dirent *dirent);

/* methods for db3 */

static const char *db3_keystr(
		const db3_key *key);

static int db3_get(
	const db3_key *key,
	naming_db_dirent *result,
	const int maxlen);

static int db3_size(
	const db3_key *key,
	db_recno_t *result);


/* methods for inode */

static int inode_keygen(
	const lwfs_oid *inode_oid,
	inode_key *result);

static const char *inode_keystr(
		const inode_key *key);

static int inode_del(
		naming_db_inode *inode);

static int inode_put(
	const inode_key *key,
	const naming_db_inode *inode,
	const uint32_t flags);

static int inode_get(
	const inode_key *key,
	naming_db_inode *result);


/* methods used to associate the databases */

static int pto2nd(
	DB *dbp,
	const DBT *pkey,
	const DBT *pdata,
	DBT *skey);

static int pto3rd(
	DB *dbp,
	const DBT *pkey,
	const DBT *pdata,
	DBT *skey);

/* util methods */

static int create_entry(const naming_db_dirent *dirent,
			const naming_db_inode  *inode,
			naming_db_entry        *entry);			


/* ----------------- private method definitions ----------------------*/

static const char *db1_keystr(
		const db1_key *key)
{
	char ostr[33];
	static char str[2*LWFS_NAME_LEN];

	sprintf(str, "%s:%s", lwfs_oid_to_string(key->parent_oid, ostr), key->name);
	return str;
}

static const char *db2_keystr(
		const db2_key *key)
{
	static char str[33]; /* 32 chars + NULL */

	lwfs_oid_to_string(key->oid, str);
	return str;
}

static const char *db3_keystr(
		const db3_key *key)
{
	static char str[33]; /* 32 chars + NULL */

	lwfs_oid_to_string(key->parent_oid, str);
	return str;
}

static const char *inode_keystr(
		const inode_key *key)
{
	static char str[33]; /* 32 chars + NULL */

	lwfs_oid_to_string(key->inode_oid, str);
	return str;
}

static int create_entry(const naming_db_dirent *dirent,
			const naming_db_inode  *inode,
			naming_db_entry        *entry)
{
	int rc = LWFS_OK;
	
	memset(entry, 0, sizeof(naming_db_entry));

	memcpy(&entry->dirent, dirent, sizeof(naming_db_dirent));
	memcpy(&entry->inode, inode, sizeof(naming_db_inode));
	
	return rc;
}


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
    @param pkey	 	The key for the primary database.
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
	/* the key for the secondary database is the oid */
	memset(skey, 0, sizeof(DBT));
	skey->data= &((naming_db_dirent *)pdata->data)->oid;
	skey->size= sizeof(db2_key);

#if 0
	if (logging_debug(naming_debug_level)) {
		db1_key *key1= (db1_key *)pkey->data;
		db2_key *key2= (db2_key *)skey->data;

		log_debug(naming_debug_level,
				"entry <%s> of db1 maps to <%s> of db2",
				db1_keystr(key1), db2_keystr(key2));
	}
#endif

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

	/* the key for the tertiary database is the parent ID */
	skey->data = &((naming_db_dirent *)pdata->data)->parent_oid;
	skey->size= sizeof(lwfs_oid);

#if 0
	if (logging_debug(naming_debug_level)) {
		db1_key *key1= (db1_key *)pkey->data;
		db3_key *key3= (db3_key *)skey->data;

		log_debug(naming_debug_level,
				"entry <%s> of db1 maps to <%s> of db3",
				db1_keystr(key1), db3_keystr(key3));
	}
#endif

	return 0;

}  /* end of pto3rd() */
/** @} */ /* and of group db_associate */




/* ----------- Methods for DB1 ------------- */


/**
 * @brief Generate a hash key for the db1.
 *
 * @param parent_oid @input the oid of the parent directory.
 * @param name       @input the name of the entry.
 * @param result     @output the generated key.
 */
static int db1_keygen(
	const lwfs_oid *parent_oid,
	const char *name,
	db1_key *result)
{
	int len = (strlen(name) < LWFS_NAME_LEN)? strlen(name) : LWFS_NAME_LEN;

	/* initialize the key */
	memset(result, 0, sizeof(db1_key));

	memcpy(&result->parent_oid, parent_oid, sizeof(lwfs_oid));
	strncpy(result->name, name, len);

	return LWFS_OK;
}


/**
 * @brief Check for the existence of an entry in the primary database.
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static lwfs_bool db1_exists(
		const db1_key *key1)
{
	int rc;
	DBT key;
	DBT data;

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)key1;
	key.size = sizeof(db1_key);

	/* initialize the data */
	memset(&data, 0, sizeof(data));
	data.dlen = 0;
	data.flags = DB_DBT_PARTIAL;

	rc = dbp1->get(dbp1, NULL, &key, &data, 0);
	switch (rc) {
		case 0:
			/* entry found */
			return TRUE;

		case DB_NOTFOUND:
			/* the entry was not found */
			return FALSE;

		default:
			/* some other error */
			log_warn(naming_debug_level, "unable to get entry: %s",
					db_strerror(rc));
	}

	return FALSE;
}

/**
 * @brief Put a database entry in the primary database.
 */
static int db1_put(
	const db1_key *key1,
	const naming_db_dirent *dirent,
	const uint32_t flags)
{
    int rc = LWFS_OK;
    DBT key, data;


    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* The key for the primary database is the parent ID contatenated
     * with the entry name.
     */
    key.data = (void *)key1;
    key.size = sizeof(db1_key);

    /* The data is the entry */
    data.data = (void *)dirent;
    data.size = sizeof(naming_db_dirent);

    /* Put the entry in the primary database. */
    rc = dbp1->put(dbp1, NULL, &key, &data, flags);
    if (rc == DB_KEYEXIST) {
	/*
	   log_error(naming_debug_level, "could not put entry in db1: %s",
	   db_strerror(rc));
	 */
	return LWFS_ERR_EXIST;
    }
    if (rc != 0) {
	log_error(naming_debug_level, "could not put entry in db1: %s",
		db_strerror(rc));
	return LWFS_ERR_NAMING;
    }

    /* sync the database */
    /*rc = dbp1->sync(dbp1, 0);
     */


    return rc;
}


#if 0
/**
 * @brief Lookup all entries in the primary database.
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static int db1_print_all()
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	DBT key;
	DBT data;
	DBC *dbc;
	

	/* acquire a cursor for database 1 */
	rc = dbp1->cursor(dbp1, NULL, &dbc, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "could not get cursor for db1: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));


	/* Copy each entry into the result */
	while ((rc = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0) {

#if 0
		db1_key *key1 = (db1_key *)key.data;
		naming_db_dirent *dirent = (naming_db_dirent *)data.data;

		log_debug(naming_debug_level, "dirent_key(%s):name=%s",
			db1_keystr(key1), dirent->name);
#endif
	}

	/* if there is an error besides DB_NOTFOUND, we need to report it */
	if (rc != DB_NOTFOUND) {
		log_error(naming_debug_level, "error iterating through dir: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}
	else {
		/* reset the return code */
		rc = LWFS_OK;
	}

cleanup:
	/* close the database cursor */
	rc2 = dbc->c_close(dbc);
	if (rc2 != 0) {
		log_error(naming_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_NAMING;
	}

	return rc;
}
#endif


/**
 * @brief Lookup an entry in the primary database.
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static int db1_get(
		const db1_key *key1,
		naming_db_dirent *result)
{
	int rc = LWFS_OK;
	DBT key;
	DBT data;

#if 0
	if (logging_debug(naming_debug_level)) {
		db1_print_all();
	}
#endif
	
	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)key1;
	key.size = sizeof(db1_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = result;
	data.ulen = sizeof(naming_db_dirent);
	data.flags = DB_DBT_USERMEM;

	rc = dbp1->get(dbp1, NULL, &key, &data, 0);
	switch (rc) {
		case 0:
			/* entry found */
			break;

		case DB_NOTFOUND:
			/* entry not found */
			rc = LWFS_ERR_NOENT;
			break;

		default:
			/* some other error */
			log_warn(naming_debug_level, "unable to get entry: %s",
					db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			break;
	}

	return rc;
}


/* ----------- Methods for DB2 ------------- */

/**
 * @brief Generate a hash key for the db2.
 *
 * @param parent_oid @input the oid of the parent directory.
 * @param name       @input the name of the entry.
 * @param result     @output the generated key.
 */
static int db2_keygen(
	const lwfs_oid *oid,
	db2_key *result)
{
	log_debug(naming_debug_level, "entered db2_keygen");

	/* initialize the key */
	memset(result, 0, sizeof(db2_key));
	memcpy(&result->oid, oid, sizeof(lwfs_oid));

	log_debug(naming_debug_level, "finished db2_keygen");

	return LWFS_OK;
}

/**
 * @brief Lookup an entry in the second database.
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static int db2_get(
		const db2_key *key2,
		naming_db_dirent *result)
{
	int rc = LWFS_OK;
	DBT key;
	DBT data;

	/* initialize the result */
	memset(result, 0, sizeof(naming_db_dirent));

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	/* initialize the key */
	key.data = (void *)key2;
	key.size = sizeof(db2_key);

	/* initialize the data */
	data.data = result;
	data.ulen = sizeof(naming_db_dirent);
	data.flags = DB_DBT_USERMEM;

	rc = dbp2->get(dbp2, NULL, &key, &data, 0);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to get entry (%s): %s",
				db2_keystr(key2), db_strerror(rc));
		return LWFS_ERR_NAMING;
	}

	return rc;
}

/**
 * @brief Remove an entry from the database.
 *
 * @param entry        @input the entry to remove.
 */
static int db_del(
		naming_db_entry *entry)
{
	db2_key key2;
	inode_key ikey;
	int rc = LWFS_OK;
	DBT key;
	DBT data;

	/* generate the key for the secondary database */
	db2_keygen((const lwfs_oid *)&entry->dirent.oid, &key2);

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = (void *)&key2;
	key.size = sizeof(db2_key);


	/* delete the entry from the database (may re-insert it as an orphan later) */
	rc = dbp2->del(dbp2, NULL, &key, 0);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to remove entry: %s",
				db_strerror(rc));
		return LWFS_ERR_NAMING;
	}
	
	entry->inode.ref_cnt--;
	
	if (entry->inode.ref_cnt == 0) {
		/* the dearly departed dirent was the last ref to this inode, remove the inode */
		inode_del(&entry->inode);
	} else if (entry->inode.ref_cnt > 0) {
		/* other inode refs exists.  update inode ref count. */
		inode_keygen((const lwfs_oid *)&entry->inode.entry_obj.oid, &ikey);
		inode_put(&ikey, &entry->inode, 0);
	} else {
		char ostr[33];
		/* negative ref count.  concurrency error.  abort. */
		log_fatal(naming_debug_level, "inode (%s) ref_cnt(%d) is negative", 
			  lwfs_oid_to_string(entry->inode.entry_obj.oid, ostr), 
			  entry->inode.ref_cnt);
		abort();
	}

	return rc;
}


/* ----------- Methods for DB3 ------------- */

/**
 * @brief Generate a key for the db3.
 *
 * @param parent_oid @input the oid of the parent directory.
 * @param name       @input the name of the entry.
 * @param result     @output the generated key.
 */
static int db3_keygen(
	const lwfs_oid *oid,
	db3_key *result)
{
	log_debug(naming_debug_level, "entered db3_keygen");

	/* initialize the key */
	memset(result, 0, sizeof(db3_key));
	memcpy(&result->parent_oid, oid, sizeof(lwfs_oid));

	log_debug(naming_debug_level, "finished db3_keygen");

	return LWFS_OK;
}


/**
 * @brief Lookup all entries in the third database that have
 *        the same key (i.e., the same parent directory).
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static int db3_get(
		const db3_key *key3,
		naming_db_dirent *result,
		const int maxlen)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	DBT key;
	DBT data;
	DBC *dbc;
	int i=0;

	log_debug(naming_debug_level, "getting entries for db3 - key(%s):maxlen(%d)",
			db3_keystr(key3), maxlen);

	/* acquire a cursor for database 3 */
	rc = dbp3->cursor(dbp3, NULL, &dbc, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "could not get cursor in db3: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)key3;
	key.size = sizeof(db3_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = &result[0];
	data.ulen = sizeof(naming_db_dirent);
	data.flags = DB_DBT_USERMEM;

	/* set the cursor for a particular key */
	rc = dbc->c_get(dbc, &key, &data, DB_SET);
	if (rc != 0) {
		log_error(naming_debug_level, "unable to get first entry: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}
	log_debug(naming_debug_level, "got result for <%s> name=%s",
			db3_keystr(key3), result[0].name);

	data.data = &result[++i];

	/* Copy each entry into the result */
	while ((rc = dbc->c_get(dbc, &key, &data, DB_NEXT_DUP)) == 0) {

#if 0
		log_debug(naming_debug_level, "got entry for <%s> name=%s",
				db3_keystr(key3), result[i].name);
#endif

		/* increment the index */
		i++;

		if (i >= maxlen) {
			
			break;
		}

		data.data = &result[i];
	}

	/* if there is an error besides DB_NOTFOUND, we need to report it */
	if ((rc != LWFS_OK) && (rc != DB_NOTFOUND)) {
		log_error(naming_debug_level, "error iterating through dir: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}
	else {
		/* reset the return code */
		rc = LWFS_OK;
	}

cleanup:
	/* close the database cursor */
	rc2 = dbc->c_close(dbc);
	if (rc2 != 0) {
		log_error(naming_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_NAMING;
	}

	return rc;
}


/**
 * @brief Return the size (number of entries in the directory.
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the number of entries with the same parent.
 */
static int db3_size(
		const db3_key *key3,
		db_recno_t *result)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	DBT key;
	DBT data;
	DBC *dbc;
	naming_db_dirent db_ent;

	/* acquire a cursor for database 3 */
	rc = dbp3->cursor(dbp3, NULL, &dbc, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "could not get cursor in db3: %s",
				db_strerror(rc));
		return LWFS_ERR_NAMING;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (db3_key *)key3;
	key.size = sizeof(db3_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = &db_ent;
	data.ulen = sizeof(naming_db_dirent);
	data.flags = DB_DBT_USERMEM;

	rc = dbc->c_get(dbc, &key, &data, DB_SET);
	if (rc == DB_NOTFOUND) {
		*result = 0;
		rc = LWFS_OK;
		goto cleanup;
	}
	else if (rc != 0) {
		log_error(naming_debug_level, "unable to get first entry: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	/* get a count of the number of duplicates */
	rc = dbc->c_count(dbc, result, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "unable to count duplicates: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	log_debug(naming_debug_level, "directory <%s> has %d entries",
		db3_keystr(key3), *result);
		

cleanup:
	/* close the database cursor */
	rc2 = dbc->c_close(dbc);
	if (rc2 != 0) {
		log_error(naming_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_NAMING;
	}

	return rc;
}


/* ----------- Methods for inode ------------- */


/**
 * @brief Generate a hash key for the inode.
 *
 * @param inode_oid  @input the oid of the parent directory.
 * @param result     @output the generated key.
 */
static int inode_keygen(
	const lwfs_oid *inode_oid,
	inode_key *result)
{
	/* initialize the key */
	memset(result, 0, sizeof(inode_key));

	memcpy(&result->inode_oid, inode_oid, sizeof(lwfs_oid));

	return LWFS_OK;
}


/**
 * @brief Check for the existence of an inode in the database.
 *
 * @param key        @input the key of the inode to look up.
 * @param result     @output the inode associated with the key.
 */
#if 0
static lwfs_bool inode_exists(
		const inode_key *key1)
{
	int rc;
	DBT key;
	DBT data;

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)key1;
	key.size = sizeof(inode_key);

	/* initialize the data */
	memset(&data, 0, sizeof(data));
	data.dlen = 0;
	data.flags = DB_DBT_PARTIAL;

	rc = dbp_inode->get(dbp_inode, NULL, &key, &data, 0);
	switch (rc) {
		case 0:
			/* inode found */
			return TRUE;

		case DB_NOTFOUND:
			/* the inode was not found */
			return FALSE;

		default:
			/* some other error */
			log_warn(naming_debug_level, "unable to get inode: %s",
					db_strerror(rc));
	}

	return FALSE;
}
#endif

/**
 * @brief Remove an inode from the database.
 *
 * @param inode		@input the inode to remove.
 */
static int inode_del(
		naming_db_inode *inode)
{
	inode_key ikey;
	int rc = LWFS_OK;
	DBT key;
	DBT data;

	/* generate the key for the inode database */
	inode_keygen((const lwfs_oid *)&inode->entry_obj.oid, &ikey);

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	key.data = (void *)&ikey;
	key.size = sizeof(inode_key);


	/* delete the inode from the database */
	rc = dbp_inode->del(dbp_inode, NULL, &key, 0);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to remove inode: %s",
				db_strerror(rc));
		return LWFS_ERR_NAMING;
	}
	
	return rc;
}

/**
 * @brief Put a database inode in the primary database.
 */
static int inode_put(
	const inode_key *key1,
	const naming_db_inode *inode,
	const uint32_t flags)
{
	int rc = LWFS_OK;
	DBT key, data;


	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	/* The key for the primary database is the parent ID contatenated
	 * with the inode name.
	 */
	key.data = (void *)key1;
	key.size = sizeof(inode_key);

	/* The data is the inode */
	data.data = (void *)inode;
	data.size = sizeof(naming_db_inode);

	/* Put the inode in the inode database. */
	rc = dbp_inode->put(dbp_inode, NULL, &key, &data, flags);
	if (rc == DB_KEYEXIST) {
		log_error(naming_debug_level, "could not put inode in inode: %s",
				db_strerror(rc));
		return LWFS_ERR_EXIST;
	}
	if (rc != 0) {
		log_error(naming_debug_level, "could not put inode in inode: %s",
				db_strerror(rc));
		return LWFS_ERR_NAMING;
	}

	/* sync the database */
	/*rc = dbp_inode->sync(dbp_inode, 0);
	*/
	

	return rc;
}


#if 0
/**
 * @brief Lookup all entries in the primary database.
 *
 * @param key        @input the key of the inode to look up.
 * @param result     @output the inode associated with the key.
 */
static int inode_print_all()
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	DBT key;
	DBT data;
	DBC *dbc;

	/* acquire a cursor for database 1 */
	rc = dbp_inode->cursor(dbp_inode, NULL, &dbc, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "could not get cursor for inode: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));


	/* Copy each inode into the result */
	while ((rc = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0) {
#if 0
		char ostr[33];
		inode_key *key1 = (inode_key *)key.data;
		naming_db_inode *inode = (naming_db_inode *)data.data;

		log_debug(naming_debug_level, "inode_key(%s):inode_oid=%s",
			inode_keystr(key1), lwfs_oid_to_string(inode->entry_obj.oid, ostr));
#endif
	}

	/* if there is an error besides DB_NOTFOUND, we need to report it */
	if (rc != DB_NOTFOUND) {
		log_error(naming_debug_level, "error iterating through dir: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}
	else {
		/* reset the return code */
		rc = LWFS_OK;
	}

cleanup:
	/* close the database cursor */
	rc2 = dbc->c_close(dbc);
	if (rc2 != 0) {
		log_error(naming_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_NAMING;
	}

	return rc;
}
#endif


/**
 * @brief Lookup an inode in the primary database.
 *
 * @param key        @input the key of the inode to look up.
 * @param result     @output the inode associated with the key.
 */
static int inode_get_by_dirent(
		const naming_db_dirent *dirent,
		naming_db_inode *result)
{
	int rc = LWFS_OK;
	inode_key ikey;
	
	/* initialize the inode key */
	rc = inode_keygen(&dirent->inode_oid, &ikey);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not gen ikey: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	rc = inode_get(&ikey, result); 
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not get inode (ikey=%d): %s",
				inode_keystr(&ikey), lwfs_err_str(rc));
		goto cleanup;
	}
	
cleanup:
	
	return rc;
}


/**
 * @brief Lookup an inode in the primary database.
 *
 * @param key        @input the key of the inode to look up.
 * @param result     @output the inode associated with the key.
 */
static int inode_get(
		const inode_key *ikey,
		naming_db_inode *result)
{
	int rc = LWFS_OK;
	DBT key;
	DBT data;

#if 0
	if (logging_debug(naming_debug_level)) {
		inode_print_all();
	}
#endif
	
	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)ikey;
	key.size = sizeof(inode_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = result;
	data.ulen = sizeof(naming_db_inode);
	data.flags = DB_DBT_USERMEM;

	rc = dbp_inode->get(dbp_inode, NULL, &key, &data, 0);
	switch (rc) {
		case 0:
			/* inode found */
			break;

		case DB_NOTFOUND:
			/* inode not found */
			rc = LWFS_ERR_NOENT;
			break;

		default:
			/* some other error */
			log_warn(naming_debug_level, "unable to get inode: %s",
					db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			break;
	}

	return rc;
}


/**
 * @brief Lookup an inode in the primary database.
 *
 * @param key        @input the key of the inode to look up.
 * @param result     @output the inode associated with the key.
 */
static int inode_increment_refcnt(
		const inode_key *ikey)
{
	int rc = LWFS_OK;
	naming_db_inode inode;

#if 0
	if (logging_debug(naming_debug_level)) {
		inode_print_all();
	}
#endif

	rc = inode_get(ikey, &inode);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to get inode (%s): %s",
				inode_keystr(ikey), strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	inode.ref_cnt++;
	
	rc = inode_put(ikey, &inode, 0);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to put inode (%s): %s",
				inode_keystr(ikey), strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

cleanup:

	return rc;
}


/* ----------------- The naming_db API -----------------*/

/**
 * @brief Initialize the database.
 *
 * This function creates or opens three sleepycat databases to store
 * information used by the lwfs naming service.
 *
 * @param acl_db_fname @input path to the database file.
 * @param dbclear @input  flag to signal a fresh start.
 * @param dbrecover @input flag to signal recovery from crash.
 * @param root_entry @output the root of the directory.
 * @param orphan_entry @output the root of the orphan directory.
 */
int naming_db_init(
	const char *db1_fname,
	const lwfs_bool dbclear,
	const lwfs_bool dbrecover,
	naming_db_entry *root_entry,
	naming_db_entry *orphan_entry)
{
	int rc = LWFS_OK;
	lwfs_bool newfile = FALSE;

	char *db2_fname = NULL;    /* fname for secondary DB */
	char *db3_fname = NULL;    /* fname for tertiary DB */
	char *inode_fname = NULL;  /* fname for inode DB */

	/* initialize the file name of the secondary DB */
	db2_fname = malloc(strlen(db1_fname) + strlen(DB2_NAME_EXTENSION)+1);
	if (db2_fname == NULL) {
		log_fatal(naming_debug_level, "could not allocate space for db2_fname");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	sprintf(db2_fname, "%s%s", db1_fname, DB2_NAME_EXTENSION);

	/* initialize the file name of the tertiary DB */
	db3_fname = malloc(strlen(db1_fname) + strlen(DB3_NAME_EXTENSION)+1);
	if (db2_fname == NULL) {
		log_fatal(naming_debug_level, "could not allocate space for db2_fname");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	sprintf(db3_fname, "%s%s", db1_fname, DB3_NAME_EXTENSION);

	/* initialize the file name of the inode DB */
	inode_fname = malloc(strlen(db1_fname) + strlen(INODE_NAME_EXTENSION)+1);
	if (inode_fname == NULL) {
		log_fatal(naming_debug_level, "could not allocate space for inode_fname");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	sprintf(inode_fname, "%s%s", db1_fname, INODE_NAME_EXTENSION);

	/* test for existence of the files */
	if (access(db1_fname, F_OK) != 0) {
		newfile = TRUE;
	}

	/* if we are supposed to start from scratch, we remove any existing files */
	if (dbclear && !newfile) {

		/* remove primary db */
		rc = remove(db1_fname);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to remove %s: %s",
					db1_fname, strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		/* remove secondary db */
		rc = remove(db2_fname);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to remove %s: %s",
					db2_fname, strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		/* remove tertiary db */
		rc = remove(db3_fname);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to remove %s: %s",
					db3_fname, strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		/* remove inode db */
		rc = remove(inode_fname);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to remove %s: %s",
					inode_fname, strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}
	}

	/* create and open the primary db */
	if (db1_fname != NULL) {

		/* create the database */
		rc = db_create(&dbp1, NULL, 0);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to create database: %s",
					db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		/* set the page size */
		rc = dbp1->set_pagesize(dbp1, 8*1024);
		if (rc != 0) {
			log_fatal(naming_debug_level, "unable to set page size: %s",
					db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		rc = dbp1->open(dbp1, NULL, db1_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to open database file \"%s\": %s",
					db1_fname, db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}
	}


	/* create and open the secondary db */
	if (db2_fname != NULL) {

		/* create the database */
		rc = db_create(&dbp2, NULL, 0);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to create database: %s",
					db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		rc = dbp2->open(dbp2, NULL, db2_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to open database file \"%s\": %s",
					db2_fname, db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}
	}


	/* create and open the tertiary db */
	if (db3_fname != NULL) {
		rc = db_create(&dbp3, NULL, 0);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to open database: %s",db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		/* Keys in the tertiary database may be duplicated. The keys here are
		 * the IDs of the parent directories.  Many objects may share a parent.
		 */
		rc = dbp3->set_flags(dbp3, DB_DUP | DB_DUPSORT);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to set flags: %s",
					db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		rc = dbp3->open(dbp3, NULL, db3_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to open database file \"%s\": %s",
					db3_fname, db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}
	}


	/* create and open the inode db */
	if (inode_fname != NULL) {
		rc = db_create(&dbp_inode, NULL, 0);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to open database: %s",db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}

		rc = dbp_inode->open(dbp_inode, NULL, inode_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
		if (rc != 0) {
			log_error(naming_debug_level, "unable to open database file \"%s\": %s",
					inode_fname, db_strerror(rc));
			rc = LWFS_ERR_NAMING;
			goto cleanup;
		}
	}


	/* associate the secondary DB with the primary DB */
	rc = dbp1->associate(dbp1, NULL, dbp2, pto2nd, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "unable to associate dbp2 with dbp1: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}

	/* associate the tertiary DB with the primary DB */
	rc = dbp1->associate(dbp1, NULL, dbp3, pto3rd, 0);
	if (rc != 0) {
		log_error(naming_debug_level, "unable to associate dbp3 with dbp1: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NAMING;
		goto cleanup;
	}


	/* Initialize the current ID and store it in the database. This entry
	 * is used by the new_dbkey() to create unique IDs for each object.  The
	 * initial ID has all bits set, so it becomes 0000... when we call
	 * new_dbkey() the first time to enter "/" into the database.
	 */
	if (dbclear || newfile) {
		char ostr[33];
		db_recno_t count;
		naming_db_entry id_entry;
		naming_db_entry test_entry;

		/* initialize ID entry */
		memset(&id_entry, 0, sizeof(naming_db_entry));
		snprintf(id_entry.dirent.name, LWFS_NAME_LEN, "%s", GLOBAL_DB_KEY);
		lwfs_clear_oid(id_entry.dirent.parent_oid);
		lwfs_clear_oid(id_entry.inode.entry_obj.oid);
		id_entry.inode.entry_obj.type = 0;
		lwfs_clear_oid(id_entry.dirent.inode_oid);
		lwfs_clear_oid(id_entry.dirent.link);    /* use the link val for the current ID */
		id_entry.dirent.hide = TRUE;

		/* put the entry in the database */
		rc = naming_db_put(&id_entry, DB_NOOVERWRITE);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not add current OID: %s",
				lwfs_err_str(rc));
			goto cleanup;
		}

		/* SANITY CHECK: lookup the id in db1 */
		rc = naming_db_get_by_name((const lwfs_oid *)&id_entry.dirent.parent_oid, GLOBAL_DB_KEY, &test_entry);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "failed SANITY CHECK on db1: %s",
				lwfs_err_str(rc));
			goto cleanup;
		}

		/* SANITY CHECK: lookup the id in db2 */
		rc = naming_db_get_by_oid((const lwfs_oid *)&id_entry.dirent.oid, &test_entry);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "failed SANITY CHECK on db2: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}

		/* SANITY CHECK: lookup the count in db3 */
		rc = naming_db_get_size((const lwfs_oid *)&id_entry.dirent.parent_oid, &count);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "failed SANITY CHECK on db3: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}


		/* create the root entry */
		memset(root_entry, 0, sizeof(naming_db_entry));
		sprintf(root_entry->dirent.name, "/");
		lwfs_clear_oid(root_entry->dirent.parent_oid);
		root_entry->inode.entry_obj.type = LWFS_DIR_ENTRY;
		root_entry->inode.entry_obj.cid = LWFS_CID_ANY;  /* allow anyone to access root */

		/* generate a new oid for the root inode */
		naming_db_gen_oid(&root_entry->inode.entry_obj.oid);
		/* generate a new oid for the root dirent */
		naming_db_gen_oid(&root_entry->dirent.oid);
		memcpy(&root_entry->dirent.inode_oid, &root_entry->inode.entry_obj.oid, sizeof(lwfs_oid));

		/* put the root entry in the database */
		rc = naming_db_put(root_entry, DB_NOOVERWRITE);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to add root entry: %s",
					lwfs_err_str(rc));
			return rc;
		}

		/* SANITY CHECK: lookup the id in db1 */
		log_debug(naming_debug_level, "getting entry for oid=%s, name=%s",
			lwfs_oid_to_string(root_entry->dirent.parent_oid, ostr), "/");
		rc = naming_db_get_by_name((const lwfs_oid *)&root_entry->dirent.parent_oid, "/", &test_entry);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "failed SANITY CHECK on db1: %s",
				lwfs_err_str(rc));
			goto cleanup;
		}
		log_debug(naming_debug_level, "db1_get RESULTS: name==%s; parent_oid==%s; link==0x08%x; link_cnt==%d",
			  test_entry.dirent.name, 
			  lwfs_oid_to_string(test_entry.dirent.parent_oid, ostr), 
			  test_entry.dirent.link, 
			  test_entry.inode.ref_cnt);
		if (logging_debug(naming_debug_level)) {
			fprint_lwfs_obj(logger_get_file(), "test_entry.inode.entry_obj", "db1_get RESULTS", &test_entry.inode.entry_obj);
			fprint_lwfs_obj(logger_get_file(), "test_entry.inode.file_obj", "db1_get RESULTS", &test_entry.inode.file_obj);
		}

		/* SANITY CHECK: lookup the id in db2 */
		rc = naming_db_get_by_oid((const lwfs_oid *)&root_entry->dirent.oid, &test_entry);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "failed SANITY CHECK on db2: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}
		log_debug(naming_debug_level, "db2_get RESULTS: name==%s; parent_oid==%s; link==0x08%x; link_cnt==%d",
			  test_entry.dirent.name, 
			  lwfs_oid_to_string(test_entry.dirent.parent_oid, ostr), 
			  test_entry.dirent.link, 
			  test_entry.inode.ref_cnt);
		if (logging_debug(naming_debug_level)) {
			fprint_lwfs_obj(logger_get_file(), "test_entry.inode.entry_obj", "db2_get RESULTS", &test_entry.inode.entry_obj);
			fprint_lwfs_obj(logger_get_file(), "test_entry.inode.file_obj", "db2_get RESULTS", &test_entry.inode.file_obj);
		}

		/* SANITY CHECK: lookup the count in db3 */
		rc = naming_db_get_size((const lwfs_oid *)&root_entry->dirent.parent_oid, &count);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "failed SANITY CHECK on db3: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}
		log_debug(naming_debug_level, "db3_size RESULTS: count==%d", count);

		/* Now create the link orphan directory. This is where we "hide"
		 * nodes that have been deleted, but whose link count has not
		 * reached zero yet.
		 */
		memset(orphan_entry, 0, sizeof(naming_db_entry));
		sprintf(orphan_entry->dirent.name, "orphans");
		lwfs_clear_oid(orphan_entry->dirent.parent_oid);
		orphan_entry->inode.entry_obj.type = LWFS_DIR_ENTRY;
		orphan_entry->inode.entry_obj.cid = LWFS_CID_ANY;  /* allow anyone to access orphans */

		/* generate a new oid for the orphans inode */
		naming_db_gen_oid(&orphan_entry->inode.entry_obj.oid);
		memcpy(&ORPHAN_OID, orphan_entry->inode.entry_obj.oid, sizeof(lwfs_oid));
		/* generate a new oid for the orphans dirent */
		naming_db_gen_oid(&orphan_entry->dirent.oid);
		memcpy(&orphan_entry->dirent.inode_oid, &orphan_entry->inode.entry_obj.oid, sizeof(lwfs_oid));

		/* put the orphans entry in the database */
		rc = naming_db_put(orphan_entry, DB_NOOVERWRITE);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "unable to add orphan entry: %s",
					lwfs_err_str(rc));
			return rc;
		}
	}


cleanup:
	/* free the name buffers */
	free(db2_fname);
	free(db3_fname);
	free(inode_fname);



	/* close the databases if there was an error */
	if (rc != LWFS_OK) {
		dbp1->close(dbp1, 0);
		dbp2->close(dbp2, 0);
		dbp3->close(dbp3, 0);
		dbp_inode->close(dbp_inode, 0);
	}

	return rc;
}


/*
static void print_hash_stats(DB_HASH_STAT *stats)
{
	fprintf(stderr, "----- HASH STATS -----\n");
	fprintf(stderr, "number of unique keys = %d\n", stats->hash_nkeys);
	fprintf(stderr, "number key/data pairs = %d\n", stats->hash_ndata);
	fprintf(stderr, "pagesize = %d\n", stats->hash_pagesize);
	fprintf(stderr, "number of items per bucket = %d\n", stats->hash_ffactor);
	fprintf(stderr, "buckets = %d\n", stats->hash_buckets);
	fprintf(stderr, "pages on free list = %d\n", stats->hash_free);
	fprintf(stderr, "bytes free on bucket pages = %d\n", stats->hash_bfree);
	fprintf(stderr, "number of big key/data pages = %d\n", stats->hash_bigpages);
	fprintf(stderr, "number of bytes free on big pages = %d\n", stats->hash_big_bfree);
	fprintf(stderr, "number of overflow pages = %d\n", stats->hash_overflows);
	fprintf(stderr, "number of bytes free on overflow pages = %d\n", stats->hash_ovfl_free);
	fprintf(stderr, "number of duplicate pages = %d\n", stats->hash_dup);
	fprintf(stderr, "number of bytes free on duplicate pages = %d\n", stats->hash_dup_free);
}
*/

int naming_db_fini()
{
	DB_HASH_STAT stats;

	int rc = LWFS_OK;

	/* print statistics on the database */
	memset(&stats, 0, sizeof(DB_HASH_STAT));

	//dbp1->stat_print(dbp1, 0);
	//print_hash_stats(&stats);


	if ((dbp1 != NULL) && ((rc = dbp1->close(dbp1, 0)) != 0)) {
		rc = LWFS_ERR_NAMING;
	}

	if ((dbp2 != NULL) && ((rc = dbp2->close(dbp2, 0)) != 0)) {
		rc = LWFS_ERR_NAMING;
	}

	if ((dbp3 != NULL) && ((rc = dbp3->close(dbp3, 0)) != 0)) {
		rc = LWFS_ERR_NAMING;
	}

	if ((dbp_inode != NULL) && ((rc = dbp_inode->close(dbp_inode, 0)) != 0)) {
		rc = LWFS_ERR_NAMING;
	}

	return rc;
}


static void gen_unique_oid(lwfs_oid *oid)
{
    uuid_t *uuid;
    size_t oid_size = sizeof(lwfs_oid);
    
    uuid_create(&uuid);
    uuid_make(uuid, UUID_MAKE_V1);
    uuid_export(uuid, UUID_FMT_BIN, (void *)&oid, &oid_size);
    uuid_destroy(uuid);
    

    return; 
}

/**
 * @brief Generate a unique container ID.
 *
 * This function generates a system-wide unique container ID.
 *
 * This particular implementation increments and returns an integer
 * variable stored in the acl database.
 */
int naming_db_gen_oid(lwfs_oid *result)
{
	int rc = LWFS_OK;

	gen_unique_oid(result);

	return rc;
}


/**
 * @brief Lookup and entry in the database by its parent oid and name.
 *
 * @brief  parent_oid   @input oid of the parent directory.
 * @brief  name         @input the name to find.
 * @brief  result       @output the entry.
 *
 * This function looks up an existing entry in the primary database.
 */
int naming_db_get_by_name(
	const lwfs_oid *parent_oid,
	const char *name,
	naming_db_entry *result)
{
	int rc = LWFS_OK;
	db1_key key1;
	naming_db_dirent dirent;
	naming_db_inode  inode;

	rc = db1_keygen(parent_oid, name, &key1);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not gen db1_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	log_debug(naming_debug_level, "key1==(%s)", db1_keystr(&key1));
	
	if (db1_exists(&key1) == FALSE) {
		log_warn(naming_debug_level, "(%s) does not exist",
				db1_keystr(&key1));
	}

	/* lookup the dirent */
	rc = db1_get(&key1, &dirent);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	rc = inode_get_by_dirent(&dirent, &inode);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup inode by dirent: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	
	rc = create_entry(&dirent, &inode, result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not create naming_db_entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	
cleanup:

	return rc;
}

/**
 * @brief Lookup and entry by its oid.
 *
 * @brief  oid         @input oid of the entry.
 * @brief  result       @output the entry.
 *
 * This function looks up an existing entry in the primary database.
 */
int naming_db_get_by_oid(
	const lwfs_oid *oid,
	naming_db_entry *result)
{
	int rc = LWFS_OK;
	db2_key key2;
	naming_db_dirent dirent;
	naming_db_inode  inode;

	rc = db2_keygen(oid, &key2);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not gen db2_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* lookup the entry */
	rc = db2_get(&key2, &dirent);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	rc = inode_get_by_dirent(&dirent, &inode);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup inode by dirent: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	
	rc = create_entry(&dirent, &inode, result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not create naming_db_entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:

	return rc;
}

/**
 * @brief Remove an entry from the database.
 *
 * @param parent_oid @input the oid of the parent.
 * @param result     @output an array of database entries with the same parent.
 * @param result_len @output the number of entries in the result.
 */
int naming_db_get_all_by_parent(
	const lwfs_oid *parent_oid,
	naming_db_entry *result,
	const int maxlen)
{
	int rc = LWFS_OK;
	int i = 0;
	db3_key key3;
	naming_db_dirent *dirent;
	naming_db_inode  inode;

	log_debug(naming_debug_level, "entered naming_db_get_all_by_parent");

	/* generate a hash key for the primary database */
	rc = db3_keygen(parent_oid, &key3);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not generate key");
		goto cleanup;
	}
	
	dirent = (naming_db_dirent *)malloc(maxlen * sizeof(naming_db_dirent));
	if (dirent == NULL) {
		rc = LWFS_ERR_NOENT;
		goto cleanup;
	}

	/* get the array of entries associated with the parent */
	rc = db3_get(&key3, dirent, maxlen);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	for (i=0; i < maxlen; i++) {
#if 0
		char ostr[33];
		log_debug(naming_debug_level, "dirent[i].oid==(%s)", lwfs_oid_to_string(dirent[i].oid, ostr));
		log_debug(naming_debug_level, "dirent[i].inode_oid==(%s)", lwfs_oid_to_string(dirent[i].inode_oid, ostr));
#endif
		rc = inode_get_by_dirent(&(dirent[i]), &inode);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not lookup inode by dirent: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}
		
		rc = create_entry(&(dirent[i]), &inode, &(result[i]));
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not create naming_db_entry: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}
	}

cleanup:
	log_debug(naming_debug_level, "finished naming_db_get_all_by_parent");

	return rc;
}

int naming_db_get_size(
	const lwfs_oid *parent_oid,
	db_recno_t *result)
{
	int rc;
	db3_key key3;

	/* generate a hash key for the primary database */
	rc = db3_keygen(parent_oid, &key3);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not generate key");
		return rc;
	}

	/* get the array of entries associated with the parent */
	rc = db3_size(&key3, result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	return rc;
}


lwfs_bool naming_db_exists(const naming_db_entry *db_entry)
{
	const lwfs_oid parent_oid;
	const char *name = db_entry->dirent.name;

	int rc = LWFS_OK;
	db1_key key1;
	
	memcpy(&parent_oid, &db_entry->dirent.parent_oid, sizeof(lwfs_oid));

	/* generate the hash key for the primary database */
	rc = db1_keygen(&parent_oid, name, &key1);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not gen db1_key: %s",
				lwfs_err_str(rc));
		return FALSE;
	}

	/* check for existence of entry */
	return db1_exists(&key1);
}

/**
 * @brief Put a new entry in the database.
 *
 * @param  db_entry @input the entry to insert into the database.
 *
 * This function puts a
 */
int naming_db_put(
	const naming_db_entry *db_entry,
	const uint32_t options)
{
	const lwfs_oid parent_oid;
	const char *name = db_entry->dirent.name;
	const lwfs_oid inode_oid;
	
	char ostr[33];

	int rc = LWFS_OK;
	db1_key key1;
	inode_key ikey;

	memcpy(&parent_oid, &db_entry->dirent.parent_oid, sizeof(lwfs_oid));
	memcpy(&inode_oid, &db_entry->inode.entry_obj.oid, sizeof(lwfs_oid));

	log_debug(naming_debug_level, "naming_db_put(%s, %s, ...)",
		lwfs_oid_to_string(parent_oid, ostr), name);
	/* generate the hash key for the primary database */
	rc = db1_keygen(&parent_oid, name, &key1);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not gen db1_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	log_debug(naming_debug_level, "key1==(%s)", db1_keystr(&key1));
	log_debug(naming_debug_level, "dirent.oid==(%s)", lwfs_oid_to_string(db_entry->dirent.oid, ostr));
	log_debug(naming_debug_level, "dirent.inode_oid==(%s)", lwfs_oid_to_string(db_entry->dirent.inode_oid, ostr));

	/* put the entry in the primary database */
	rc = db1_put(&key1, &db_entry->dirent, options);
	if (rc != LWFS_OK) {
	    if (rc != LWFS_ERR_EXIST) {
		log_warn(naming_debug_level, "could not put entry: %s",
			lwfs_err_str(rc));
	    }
	    goto cleanup;
	}

	/* generate the hash key for the inode database */
	rc = inode_keygen(&inode_oid, &ikey);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not gen inode_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	log_debug(naming_debug_level, "ikey==(%s)", inode_keystr(&ikey));

	if (!lwfs_is_oid_zero(db_entry->dirent.link)) {
		/* dirent is a link to an existing inode, just increment refcnt */
		rc = inode_increment_refcnt(&ikey);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not increment refcnt (%s): %s",
					inode_keystr(&ikey), lwfs_err_str(rc));
			goto cleanup;
		}
	} else {
		/* new entry, insert inode */
		rc = inode_put(&ikey, &db_entry->inode, options);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not put entry: %s",
					lwfs_err_str(rc));
			goto cleanup;
		}
	}

cleanup:

	return rc;
}

/**
 * @brief Remove an entry from the database.
 *
 * @param parent_oid @input the oid of the parent.
 * @param name       @input the name of the entry to remove.
 * @param result     @output the database entry that was removed.
 *
 */
int naming_db_del(
	const lwfs_oid *parent_oid,
	const char *name,
	naming_db_entry *result)
{
	int rc = LWFS_OK;

	/* get the entry for the result */
	rc = naming_db_get_by_name(parent_oid, name, result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		return rc;
	}
	
	/* remove the entry from the database */
	rc = db_del(result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not remove entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	/* exit here if everything worked correctly */
	return rc;
}


int naming_db_print_all() {
#if 0
	if (logging_debug(naming_debug_level)) {
		db1_print_all();
	}
#endif
	
	return LWFS_OK;
}
