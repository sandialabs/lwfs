/**
 *   @file storage_db.c
 *
 *   @brief Database functions used by the storage service.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1066 $
 *   $Date: 2007-01-19 10:21:48 -0700 (Fri, 19 Jan 2007) $
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif 

#include <db.h>
#include <unistd.h>

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/storage_common/ss_debug.h"

#include "storage_db.h"

/* ----------------- global variables and structs ------------------*/

static DB *dbp1;
static DB *dbp2;

/*static const char *GLOBAL_DB_KEY = "The latest generated ID";*/
static const char *DB2_NAME_EXTENSION = ".2nd";

/**
 * @brief Structure for the primary database key.
 */
typedef struct  {
	lwfs_oid oid;
	char name[LWFS_NAME_LEN];
} db1_key;

/**
 * @brief Structure for the secondary database key.
 */
typedef struct  {
	lwfs_oid oid;
} db2_key;


/* ----------------- private method prototypes ----------------------*/

/* methods for all databases */
static int db_del(ss_db_attr_entry *entry);

/* methods for db1 */

static const char *db1_keystr(
		const db1_key *key);

static int db1_put(
	const db1_key *key,
	const ss_db_attr_entry *dirent,
	const uint32_t flags);

static int db1_get(
	const db1_key *key,
	ss_db_attr_entry *dirent);


/* methods for db2 */

static int db2_keygen(
	const lwfs_oid *oid,
	db2_key *result);

static const char *db2_keystr(
		const db2_key *key);

static int db2_get(
		const db2_key *key2,
		char **result,
		const int maxlen);


/* methods used to associate the databases */

static int pto2nd(
	DB *dbp,
	const DBT *pkey,
	const DBT *pdata,
	DBT *skey);


/* ----------------- private method definitions ----------------------*/

static const char *db1_keystr(
		const db1_key *key)
{
	char ostr[33];
	static char str[2*LWFS_NAME_LEN];

	sprintf(str, "%s:%s", lwfs_oid_to_string(key->oid, ostr), key->name);
	return str;
}

static const char *db2_keystr(
		const db2_key *key)
{
	char ostr[33];
	static char str[2*LWFS_NAME_LEN];

	sprintf(str, "%s", lwfs_oid_to_string(key->oid, ostr));
	return str;
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
	skey->data= (void *)&((ss_db_attr_entry *)pdata->data)->oid;
	skey->size= sizeof(db2_key);

	if (logging_debug(ss_debug_level)) {
		db1_key *key1= (db1_key *)pkey->data;
		db2_key *key2= (db2_key *)skey->data;

		log_debug(ss_debug_level,
				"entry <%s> of db1 maps to <%s> of db2",
				db1_keystr(key1), db2_keystr(key2));
	}

	return 0;

}  /* end of pto2nd() */




/* ----------- Methods for DB1 ------------- */


/**
 * @brief Generate a hash key for the db1.
 *
 * @param oid @input the oid of the parent directory.
 * @param name       @input the name of the entry.
 * @param result     @output the generated key.
 */
static int db1_keygen(
	const lwfs_oid oid,
	const char *name,
	db1_key *result)
{
	int len = (strlen(name) < LWFS_NAME_LEN)? strlen(name) : LWFS_NAME_LEN;

	/* initialize the key */
	memset(result, 0, sizeof(db1_key));

	memcpy(result->oid, oid, sizeof(lwfs_oid));
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
			log_warn(ss_debug_level, "unable to get entry: %s",
					db_strerror(rc));
	}

	return FALSE;
}

/**
 * @brief Put a database entry in the primary database.
 */
static int db1_put(
	const db1_key *key1,
	const ss_db_attr_entry *attr,
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
	data.data = (void *)attr;
	data.size = sizeof(ss_db_attr_entry);

	/* Put the entry in the primary database. */
	rc = dbp1->put(dbp1, NULL, &key, &data, flags);
	if (rc == DB_KEYEXIST) {
		log_error(ss_debug_level, "could not put entry in db1: %s",
				db_strerror(rc));
		return LWFS_ERR_EXIST;
	}
	if (rc != 0) {
		log_error(ss_debug_level, "could not put entry in db1: %s",
				db_strerror(rc));
		return LWFS_ERR_STORAGE;
	}

	/* sync the database */
	/*rc = dbp1->sync(dbp1, 0);
	*/
	

	return rc;
}

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
		log_error(ss_debug_level, "could not get cursor for db1: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));


	/* Copy each entry into the result */
	while ((rc = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0) {

		db1_key *key1 = (db1_key *)key.data;
		ss_db_attr_entry *attr = (ss_db_attr_entry *)data.data;

		log_debug(ss_debug_level, "attr_key(%s):name=%s:value=%s",
			db1_keystr(key1), attr->name, attr->value);
	}

	/* if there is an error besides DB_NOTFOUND, we need to report it */
	if (rc != DB_NOTFOUND) {
		log_error(ss_debug_level, "error iterating through dir: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
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
		log_error(ss_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_STORAGE;
	}

	return rc;
}


/**
 * @brief Lookup an entry in the primary database.
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static int db1_get(
		const db1_key *key1,
		ss_db_attr_entry *result)
{
	int rc = LWFS_OK;
	DBT key;
	DBT data;

#if 0
	if (logging_debug(ss_debug_level)) {
		db1_print_all();
	}
#endif
	
	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)key1;
	key.size = sizeof(db1_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = (void *)result;
	data.ulen = sizeof(ss_db_attr_entry);
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
			log_warn(ss_debug_level, "unable to get entry: %s",
					db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			break;
	}

	return rc;
}


/* ----------- Methods for DB2 ------------- */

/**
 * @brief Generate a hash key for the db2.
 *
 * @param oid @input the oid of the parent directory.
 * @param name       @input the name of the entry.
 * @param result     @output the generated key.
 */
static int db2_keygen(
	const lwfs_oid *oid,
	db2_key *result)
{
	log_debug(ss_debug_level, "entered db2_keygen");

	/* initialize the key */
	memset(result, 0, sizeof(db2_key));
	memcpy(&result->oid, oid, sizeof(lwfs_oid));

	log_debug(ss_debug_level, "finished db2_keygen");

	return LWFS_OK;
}

/**
 * @brief Lookup all entries in the third database that have
 *        the same key (i.e., the same parent directory).
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the entry associated with the key.
 */
static int db2_get(
		const db2_key *key2,
		char **result,
		const int maxlen)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	DBT key;
	DBT data;
	DBC *dbc;
	int i=0;
	ss_db_attr_entry attr;
	
#if 0
	if (logging_debug(ss_debug_level)) {
		db1_print_all();
	}
#endif

	log_debug(ss_debug_level, "getting entries for db2 - key(%s):max results(%d)",
			db2_keystr(key2), maxlen);

	/* acquire a cursor for database 3 */
	rc = dbp2->cursor(dbp2, NULL, &dbc, 0);
	if (rc != 0) {
		log_error(ss_debug_level, "could not get cursor in db2: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (void *)key2;
	key.size = sizeof(db2_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = (void *)&attr;
	data.ulen = sizeof(ss_db_attr_entry);
	data.flags = DB_DBT_USERMEM;

	memset(data.data, 0, data.ulen);

	/* set the cursor for a particular key */
	rc = dbc->c_get(dbc, &key, &data, DB_SET);
	if (rc == DB_NOTFOUND) {
		log_debug(ss_debug_level, "no matches found: %s",
				db_strerror(rc));
		rc = LWFS_ERR_NOENT;
		goto cleanup;
	} else if (rc != 0) {
		log_error(ss_debug_level, "unable to get first entry: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}
	strncpy(result[0], attr.name, LWFS_NAME_LEN);
	log_debug(ss_debug_level, "got result for <%s> name=%s",
		  db2_keystr(key2), result[0]);

	memset(data.data, 0, data.ulen);

	/* Copy each entry into the result */
	while ((rc = dbc->c_get(dbc, &key, &data, DB_NEXT_DUP)) == 0) {

		/* increment the index */
		i++;

		if (i >= maxlen) {
			
			break;
		}

		strncpy(result[i], attr.name, LWFS_NAME_LEN);
		log_debug(ss_debug_level, "got result for <%s> name=%s",
			  db2_keystr(key2), result[i]);

		memset(data.data, 0, data.ulen);
	}

	/* if there is an error besides DB_NOTFOUND, we need to report it */
	if ((rc != LWFS_OK) && (rc != DB_NOTFOUND)) {
		log_error(ss_debug_level, "error iterating through dir: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
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
		log_error(ss_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_STORAGE;
	}

	return rc;
}

/**
 * @brief Return the size (number of attributes).
 *
 * @param key        @input the key of the entry to look up.
 * @param result     @output the number of entries with the same parent.
 */
static int db2_size(
		const db2_key *key2,
		db_recno_t *result)
{
	int rc = LWFS_OK;
	int rc2 = LWFS_OK;
	DBT key;
	DBT data;
	DBC *dbc;
	ss_db_attr_entry db_ent;

	/* acquire a cursor for database 2 */
	rc = dbp2->cursor(dbp2, NULL, &dbc, 0);
	if (rc != 0) {
		log_error(ss_debug_level, "could not get cursor in db2: %s",
				db_strerror(rc));
		return LWFS_ERR_STORAGE;
	}

	/* initialize the key */
	memset(&key, 0, sizeof(DBT));
	key.data = (db2_key *)key2;
	key.size = sizeof(db2_key);

	/* initialize the data */
	memset(&data, 0, sizeof(DBT));
	data.data = &db_ent;
	data.ulen = sizeof(ss_db_attr_entry);
	data.flags = DB_DBT_USERMEM;

	rc = dbc->c_get(dbc, &key, &data, DB_SET);
	if (rc == DB_NOTFOUND) {
		*result = 0;
		rc = LWFS_OK;
		goto cleanup;
	}
	else if (rc != 0) {
		log_error(ss_debug_level, "unable to get first entry: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

	/* get a count of the number of duplicates */
	rc = dbc->c_count(dbc, result, 0);
	if (rc != 0) {
		log_error(ss_debug_level, "unable to count duplicates: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

	log_debug(ss_debug_level, "obj <%s> has %d attributes",
		db2_keystr(key2), *result);
		

cleanup:
	/* close the database cursor */
	rc2 = dbc->c_close(dbc);
	if (rc2 != 0) {
		log_error(ss_debug_level, "unable to close db cursor: %s",
				db_strerror(rc2));
		rc = LWFS_ERR_STORAGE;
	}

	return rc;
}


/**
 * @brief Remove an entry from the database.
 *
 * @param entry        @input the entry to remove.
 */
static int db_del(
		ss_db_attr_entry *attr)
{
	db1_key key1;
	int rc = LWFS_OK;
	DBT key;

	/* generate the key for the primary database */
	db1_keygen(attr->oid, attr->name, &key1);

	log_debug(ss_debug_level, "key1==(%s)", db1_keystr(&key1));

	memset(&key, 0, sizeof(DBT));

	key.data = (void *)&key1;
	key.size = sizeof(db1_key);


	/* delete the entry from the database (may re-insert it as an orphan later) */
	rc = dbp1->del(dbp1, NULL, &key, 0);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "unable to remove entry: %s",
				db_strerror(rc));
		return LWFS_ERR_STORAGE;
	}
	

	return rc;
}


/* ----------------- The storage_db API -----------------*/

/**
 * @brief Initialize the database.
 *
 * This function creates or opens three sleepycat databases to store
 * information used by the lwfs storage service.
 *
 * @param acl_db_fname @input path to the database file.
 * @param dbclear @input  flag to signal a fresh start.
 * @param dbrecover @input flag to signal recovery from crash.
 */
int ss_db_init(
	const char *db1_fname,
	const lwfs_bool dbclear,
	const lwfs_bool dbrecover)
{
	int rc = LWFS_OK;
	lwfs_bool newfile = FALSE;

	char *db2_fname = NULL;    /* fname for secondary DB */

	/* initialize the file name of the secondary DB */
	db2_fname = malloc(strlen(db1_fname) + strlen(DB2_NAME_EXTENSION)+1);
	if (db2_fname == NULL) {
		log_fatal(ss_debug_level, "could not allocate space for db2_fname");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	sprintf(db2_fname, "%s%s", db1_fname, DB2_NAME_EXTENSION);

	/* test for existence of the files */
	if (access(db1_fname, F_OK) != 0) {
		newfile = TRUE;
	}

	/* if we are supposed to start from scratch, we remove any existing files */
	if (dbclear && !newfile) {

		/* remove primary db */
		rc = remove(db1_fname);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to remove %s: %s",
					db1_fname, strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}

		/* remove secondary db */
		rc = remove(db2_fname);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to remove %s: %s",
					db2_fname, strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}
	}

	/* create and open the primary db */
	if (db1_fname != NULL) {

		/* create the database */
		rc = db_create(&dbp1, NULL, 0);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to create database: %s",
					db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}

		/* set the page size */
		rc = dbp1->set_pagesize(dbp1, 8*1024);
		if (rc != 0) {
			log_fatal(ss_debug_level, "unable to set page size: %s",
					db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}

		rc = dbp1->open(dbp1, NULL, db1_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to open database file \"%s\": %s",
					db1_fname, db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}
	}


	/* create and open the secondary db */
	if (db2_fname != NULL) {
		rc = db_create(&dbp2, NULL, 0);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to open database: %s",db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}

		/* Keys in the secondary database may be duplicated. The keys here are
		 * object IDs.  Many attributes may share an object.
		 */
		rc = dbp2->set_flags(dbp2, DB_DUP | DB_DUPSORT);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to set flags: %s",
					db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}

		rc = dbp2->open(dbp2, NULL, db2_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
		if (rc != 0) {
			log_error(ss_debug_level, "unable to open database file \"%s\": %s",
					db2_fname, db_strerror(rc));
			rc = LWFS_ERR_STORAGE;
			goto cleanup;
		}
	}


	/* associate the secondary DB with the primary DB */
	rc = dbp1->associate(dbp1, NULL, dbp2, pto2nd, 0);
	if (rc != 0) {
		log_error(ss_debug_level, "unable to associate dbp2 with dbp1: %s",
				db_strerror(rc));
		rc = LWFS_ERR_STORAGE;
		goto cleanup;
	}

cleanup:
	/* free the name buffers */
	free(db2_fname);

	/* close the databases if there was an error */
	if (rc != LWFS_OK) {
		dbp1->close(dbp1, 0);
		dbp2->close(dbp2, 0);
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

int ss_db_fini()
{
	DB_HASH_STAT stats;

	int rc = LWFS_OK;

	/* print statistics on the database */
	memset(&stats, 0, sizeof(DB_HASH_STAT));

	//dbp1->stat_print(dbp1, 0);
	//print_hash_stats(&stats);


	if ((dbp1 != NULL) && ((rc = dbp1->close(dbp1, 0)) != 0)) {
		rc = LWFS_ERR_STORAGE;
	}

	if ((dbp2 != NULL) && ((rc = dbp2->close(dbp2, 0)) != 0)) {
		rc = LWFS_ERR_STORAGE;
	}

	return rc;
}


/**
 * @brief Lookup and entry in the database by its parent oid and name.
 *
 * @brief  oid   @input oid of the parent directory.
 * @brief  name         @input the name to find.
 * @brief  result       @output the entry.
 *
 * This function looks up an existing entry in the primary database.
 */
int ss_db_get_attr_by_name(
	const lwfs_oid *oid,
	const char *name,
	ss_db_attr_entry *result)
{
	int rc = LWFS_OK;
	db1_key key1;

	rc = db1_keygen(*oid, name, &key1);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not gen db1_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	log_debug(ss_debug_level, "key1==(%s)", db1_keystr(&key1));
	
	if (db1_exists(&key1) == FALSE) {
		log_warn(ss_debug_level, "(%s) does not exist",
				db1_keystr(&key1));
	}

	/* lookup the dirent */
	rc = db1_get(&key1, result);
	if (rc != LWFS_OK) {
		log_warn(ss_debug_level, "could not lookup entry: %s",
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
int ss_db_get_all_attr_names_by_oid(
	const lwfs_oid *oid,
	char **result,
	unsigned int *max_results)
{
	int rc = LWFS_OK;
	db2_key key2;

	rc = db2_keygen(oid, &key2);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not gen db2_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* lookup the entry */
	rc = db2_get(&key2, result, *max_results);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not get attrs: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:

	return rc;
}

lwfs_bool ss_db_exists(const ss_db_attr_entry *db_entry)
{
	lwfs_oid oid;
	const char *name = db_entry->name;

	int rc = LWFS_OK;
	db1_key key1;
	
	memcpy(&oid, &db_entry->oid, sizeof(lwfs_oid));

	/* generate the hash key for the primary database */
	rc = db1_keygen(oid, name, &key1);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not gen db1_key: %s",
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
int ss_db_attr_put(
	const ss_db_attr_entry *db_entry,
	const uint32_t options)
{
	lwfs_oid oid;
	const char *name = db_entry->name;
	char ostr[33];

	int rc = LWFS_OK;
	db1_key key1;

	memcpy(oid, db_entry->oid, sizeof(lwfs_oid));

	log_debug(ss_debug_level, "ss_db_put(0x%s, %s, %s)",
		lwfs_oid_to_string(oid, ostr), name, db_entry->value);
	/* generate the hash key for the primary database */
	rc = db1_keygen(oid, name, &key1);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not gen db1_key: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}
	log_debug(ss_debug_level, "key1==(%s)", db1_keystr(&key1));
	log_debug(ss_debug_level, "attr.oid==(0x%s)", lwfs_oid_to_string(db_entry->oid, ostr));

	/* put the entry in the primary database */
	rc = db1_put(&key1, db_entry, options);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not put entry: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

cleanup:

	return rc;
}

/**
 * @brief Remove an entry from the database.
 *
 * @param oid @input the oid of the parent.
 * @param name       @input the name of the entry to remove.
 * @param result     @output the database entry that was removed.
 *
 */
int ss_db_attr_del(
	const lwfs_oid *oid,
	const char *name,
	ss_db_attr_entry *result)
{
	int rc = LWFS_OK;

	/* get the entry for the result */
	rc = ss_db_get_attr_by_name(oid, name, result);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		return rc;
	}
	
	/* remove the entry from the database */
	rc = db_del(result);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not remove entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	/* exit here if everything worked correctly */
	return rc;
}

int ss_db_get_attr_count(
	const lwfs_oid *oid,
	unsigned int *result)
{
	int rc;
	db2_key key2;

	/* generate a hash key for the primary database */
	rc = db2_keygen(oid, &key2);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not generate key");
		return rc;
	}

	/* get the array of entries associated with the parent */
	rc = db2_size(&key2, result);
	if (rc != LWFS_OK) {
		log_error(ss_debug_level, "could not lookup entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	return rc;
}


int ss_db_print_all() {
#if 0
	if (logging_debug(ss_debug_level)) {
		db1_print_all();
	}
#endif
	return LWFS_OK;
}
