/**  
 *   @file authr_db.c
 * 
 *   @brief Database functions used by the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1564 $
 *   $Date: 2007-09-11 18:32:08 -0600 (Tue, 11 Sep 2007) $
 */

#include <db.h>
#include <errno.h>

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/authr_common/authr_args.h"
#include "common/authr_common/authr_debug.h"

#include "authr_db.h"
#include "authr_server.h"

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

/* ----------------- global variables --------------------------------*/
static DB *acl_db;

/* ----------------- private methods --------------------------------*/

static int make_keystr(
		const lwfs_cid cid, 
		const lwfs_opcode opcode, 
		char *key_data, 
		const uint32_t maxlen)
{
	memset(key_data, 0, maxlen);
	snprintf(key_data, maxlen, "%08d:%04d", (unsigned int)cid, opcode);
	return LWFS_OK;
}

/**
 * @brief Generate a unique container ID. 
 *
 * This function generates a system-wide unique container ID. 
 *
 * This particular implementation increments and returns an integer
 * variable stored in the acl database. 
 */
static int create_cid(lwfs_cid *cid) 
{
	int rc; 
	DBT key, data;
	char key_data[16];

	log_debug(authr_debug_level, "create_cid: start");

	/* initialize the container ID */
	*cid = 0;

	/* initialize the key data */
	sprintf(key_data, "cid");

	/* initialize the key and data for the database */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.size = strlen(key_data); 
	key.data = key_data; 

	data.flags = DB_DBT_MALLOC;  /* db4 will use the provided memory */

	log_debug(authr_debug_level, "fetching cid from database");

	if ((rc = acl_db->get(acl_db, NULL, &key, &data, 0)) != 0) {
		log_warn(authr_debug_level, "unable to get cid (resetting to 0): %s",
				key_data, db_strerror(rc));
		*cid = 0;
	}
	else {
		*cid = *((lwfs_cid *)data.data); 
		free(data.data);
	}

	log_debug(authr_debug_level, "retrieved cid=%d from database",*cid);

	/* increment cid */
	(*cid)++; 

	data.size = sizeof(lwfs_cid); 
	data.data = cid; 

	/* put the result back in the database */
	if ((rc = acl_db->put(acl_db, NULL, &key, &data, 0)) != 0) {
		log_error(authr_debug_level, "unable to add cid to database: %s",
				db_strerror(rc));
		switch (rc) {
			case DB_KEYEXIST:
				rc = LWFS_ERR_EXIST;
				break;
			default:
				rc = LWFS_ERR_SEC;
				break;
		}
		return rc; 
	}
	log_debug(authr_debug_level, "added cid=%d to database.", *cid); 

	return rc; 
}

/* ----------------- The authr_db API -----------------*/

/**
 * @brief Initialize the database.
 *
 * This function initializes the sleepycat database used to store 
 * access-control lists for the authorization server. 
 *
 * @param acl_db_fname @input path to the database file. 
 * @param dbclear @input  flag to signal a fresh start.
 * @param dbrecover @input flag to signal recovery from crash.
 */
int authr_db_init(
	const char *acl_db_fname, 
	const lwfs_bool dbclear, 
	const lwfs_bool dbrecover)
{
	int rc = LWFS_OK; 
	int rc2 = LWFS_OK;

	/* create the acl db */
	rc = db_create(&acl_db, NULL, 0); 
	if (rc != 0) {
		log_error(authr_debug_level, "unable to open database: %s",db_strerror(rc));
		goto cleanup;
	}

	/* if we are supposed to start from scratch, we remove any existing files */
	if (dbclear) {
		log_debug(authr_debug_level, "about to clear database (%s)", acl_db_fname);
		rc = remove(acl_db_fname);
		if (rc != 0) {
			/* ignore the no-entry case */
			if (errno != ENOENT) {
				log_error(authr_debug_level, "unable to remove existing db file: %s",
						strerror(rc));
				goto cleanup; 
			}
		}
	}

	/* open the acl db */
	log_debug(authr_debug_level, "about to open database (%s)", acl_db_fname);
	rc = acl_db->open(acl_db, NULL, acl_db_fname, NULL, DB_HASH, DB_CREATE|DB_THREAD, 0664);
	if (rc != 0) {
		log_error(authr_debug_level, "unable to open database file \"%s\": %s",
			acl_db_fname, db_strerror(rc));
		goto cleanup;
	}

	/* if we made it here, everything worked */
	rc = LWFS_OK;
	return rc; 

cleanup:  /* only executes on error */

	if ((rc2 = acl_db->close(acl_db, 0)) != 0 && rc == LWFS_OK) {
		rc = rc2;
	}

	return rc; 
}

int authr_db_fini() 
{
	int rc = LWFS_OK; 

	log_debug(authr_debug_level, "************************************");
	log_debug(authr_debug_level, "          closing database          ");
	log_debug(authr_debug_level, "************************************");

	if ((rc = acl_db->close(acl_db, 0)) != 0) {
		rc = LWFS_ERR_SEC;
	}

	return rc; 
}

lwfs_bool authr_db_container_exists(const lwfs_cid cid) {

	int rc = LWFS_OK;
	lwfs_uid_array acl; 
	
	rc = authr_db_get_acl(cid, -1, &acl); 

	return rc == LWFS_OK; 
}

/**
 * 
 */
int authr_db_create_container(lwfs_cid *cid)
{
    int rc = LWFS_OK;
    lwfs_uid_array empty_acl; 

    memset(&empty_acl, 0, sizeof(lwfs_uid_array));

    if (*cid == LWFS_CID_ANY) {
	do {
	    rc = create_cid(cid);
	    if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not create new container ID: %s",
			lwfs_err_str(rc));
		return rc; 
	    }
	} while (authr_db_container_exists(*cid));
    }

    else if (authr_db_container_exists(*cid)) {
	return LWFS_ERR_EXIST; 
    }

    else {
	rc = authr_db_put_acl(*cid, -1, &empty_acl); 
	if (rc != LWFS_OK) {
	    log_error(authr_debug_level, "could not create container: %s",
		    lwfs_err_str(rc));
	    return rc; 
	}
    }

    return rc; 
}



int authr_db_remove_container(const lwfs_cid cid) 
{
	int rc = LWFS_OK;

	/* check for the existence of the container */
	if (!authr_db_container_exists(cid)) { 
		return LWFS_ERR_NOENT;
	}

	/* do we need to search for all acls on this container, or should
	 * we assume they are gone? 
	 */

	/* remove the ``existence'' acl */
	rc = authr_db_del_acl(cid, -1); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not remove container");
		return rc; 
	}

	/* make sure the container is gone */
	if (authr_db_container_exists(cid)) {
		return LWFS_ERR;
	}

	/* the ACLs should be gone by now, so nothing to do here */
	return (LWFS_OK);
}

int authr_db_put_acl(
		const lwfs_cid cid, 
		const lwfs_opcode opcode, 
		const lwfs_uid_array *acl)
{
	int rc; 
	DBT key, data; 
	char key_data[16];

	/* set the key */
	make_keystr(cid, opcode, key_data, sizeof(key_data)); 

	log_debug(authr_debug_level, "key=%s, cid=%d, op=%d",
			key_data, (unsigned int)cid, opcode);
	
	/* initialize the key and data for the database */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = key_data; 
	key.size = strlen(key_data); 
	data.data = acl->lwfs_uid_array_val; 
	data.size = acl->lwfs_uid_array_len*sizeof(lwfs_uid); 

	log_debug(authr_debug_level, "key.size=%d, data.size=%d", key.size, data.size);
	log_debug(authr_debug_level, "sizeof(acl)=%d, acl->lwfs_uid_array_len=%d", 
		sizeof(acl), acl->lwfs_uid_array_len);

	if (logging_debug(authr_debug_level)) {
		log_debug(authr_debug_level, "printing uids");
		fprint_lwfs_uid_array(logger_get_file(), "uids", "", acl);
	}

	/* put the entry in the database */
	if ((rc = acl_db->put(acl_db, NULL, &key, &data, DB_NOOVERWRITE)) != 0) {
		log_error(authr_debug_level, "unable to add acl (key=%s): %s",
				key_data, db_strerror(rc));
		switch (rc) {
			case DB_KEYEXIST:
				rc = LWFS_ERR_EXIST;
				break;
			default:
				rc = LWFS_ERR_SEC;
				break;
		}
		return rc; 
	}
	else {
		log_debug(authr_debug_level, "added acl with key=%s", key_data);
	}

	log_debug(authr_debug_level, "finished with put");

	return rc; 
}

int authr_db_get_acl(
		const lwfs_cid cid, 
		const lwfs_opcode opcode, 
		lwfs_uid_array *result)
{
	int rc; 
	DBT key, data;
	char key_data[256];

	/* initialize the key data */
	make_keystr(cid, opcode, key_data, sizeof(key_data)); 

	/* initialize the key and data for the database */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.size = strlen(key_data); 
	key.data = key_data; 
	data.flags = DB_DBT_MALLOC;  /* db4 will alloc memory */

	log_debug(authr_debug_level, "fetching data for key=%s",key_data);

	if ((rc = acl_db->get(acl_db, NULL, &key, &data, 0)) != 0) {
	    if (opcode != -1) {
		log_error(authr_debug_level, "unable to get acl (key=%s): %s",
			key_data, db_strerror(rc));
	    }
	    rc = LWFS_ERR_NOENT; 
	}
	else {
		result->lwfs_uid_array_len = data.size/sizeof(lwfs_uid);
		result->lwfs_uid_array_val = (lwfs_uid *)data.data;

		log_debug(authr_debug_level, "yeah! acl exists: len=%d", 
				result->lwfs_uid_array_len);
	}

	log_debug(authr_debug_level, "done");
	return rc; 
}

int authr_db_del_acl(
		const lwfs_cid cid, 
		const lwfs_opcode opcode)
{
	int rc; 
	DBT key; 
	char key_data[256];

	/* initialize the key data */
	make_keystr(cid, opcode, key_data, sizeof(key_data)); 

	/* initialize the key and data for the database */
	memset(&key, 0, sizeof(key));
	key.size = strlen(key_data); 
	key.data = key_data; 

	log_debug(authr_debug_level, "removing acl with key=%s",key_data);

	if ((rc = acl_db->del(acl_db, NULL, &key, 0)) != 0) {
		log_error(authr_debug_level, "unable to remove acl (key=%s): %s",
				key_data, db_strerror(rc));
		switch (rc) {
			case DB_NOTFOUND:
				rc = LWFS_ERR_NOENT;
				return rc;

			default:
				rc = LWFS_ERR_SEC; 
				return rc; 
		}
	}

	return rc; 
}
