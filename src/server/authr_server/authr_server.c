/**  
 *   @file authr_srvr.c
 * 
 *   @brief Implementation of the server-side methods for accessing 
 *   the authorization service. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1563 $
 *   $Date: 2007-09-11 18:31:41 -0600 (Tue, 11 Sep 2007) $
 */

#include <db.h>
#include <assert.h>
#include <math.h>


#include "server/rpc_server/rpc_server.h"

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/authr_common/authr_args.h"
#include "common/authr_common/authr_debug.h"
#include "common/authr_common/authr_xdr.h"
#include "common/authr_common/authr_opcodes.h"

#include "support/trace/trace.h"

#include "cap.h"
#include "authr_server.h"
#include "authr_db.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif 

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* --------- type definitions ------------ */
typedef int (*compare_proc)(const void *, const void *);

/* ----------------- global variables -------------------------------*/

/** @brief The key used to generate/verify caps. */
static lwfs_key authr_svc_key; 

/** @brief Flags to enable/disable some functionality. */
static lwfs_bool acl_db_enabled = TRUE;
static lwfs_bool caps_enabled = TRUE; 
static lwfs_bool creds_enabled = FALSE; 

struct authr_counter {
    long create; 
    long remove;
    long getacl;
    long modacl;
    long getcap;
    long verify;
};

static struct authr_counter authr_counter;

/**
  * @brief array of supported operation descriptions. 
  */
static const lwfs_svc_op op_array[] = {

	/*  create container  */
	{
		LWFS_OP_CREATE_CONTAINER,
		(lwfs_rpc_proc)&create_container, 
		sizeof(lwfs_create_container_args), 
		(xdrproc_t)&xdr_lwfs_create_container_args, 
		sizeof(lwfs_cap),
		(xdrproc_t)&xdr_lwfs_cap 
	},

	/* remove container */
	{
		LWFS_OP_REMOVE_CONTAINER,
		(lwfs_rpc_proc)&remove_container, 
		sizeof(lwfs_remove_container_args), 
		(xdrproc_t)&xdr_lwfs_remove_container_args, 
		sizeof(void),
		(xdrproc_t)&xdr_void 
	},

	/* get acl */
	{
		LWFS_OP_GET_ACL, 
		(lwfs_rpc_proc)&get_acl, 
		sizeof(lwfs_get_acl_args), 
		(xdrproc_t)&xdr_lwfs_get_acl_args, 
		sizeof(lwfs_uid_array), 
		(xdrproc_t)&xdr_lwfs_uid_array 
	},

	/* mod acl */
	{
		LWFS_OP_MOD_ACL, 
		(lwfs_rpc_proc)&mod_acl, 
		sizeof(lwfs_mod_acl_args), 
		(xdrproc_t)&xdr_lwfs_mod_acl_args, 
		sizeof(void), 
		(xdrproc_t)&xdr_void 
	},

	/* get caps */
	{
		LWFS_OP_GET_CAP,
		(lwfs_rpc_proc)&get_cap,
		sizeof(lwfs_get_cap_args),
		(xdrproc_t)&xdr_lwfs_get_cap_args,
		sizeof(lwfs_cap),
		(xdrproc_t)&xdr_lwfs_cap
	},

	/* verify caps */
	{
		LWFS_OP_VERIFY_CAPS,
		(lwfs_rpc_proc)&verify_caps,
		sizeof(lwfs_verify_caps_args),
		(xdrproc_t)&xdr_lwfs_verify_caps_args,
		sizeof(void),
		(xdrproc_t)&xdr_void
	},

	/* always ends with a null op */
	{LWFS_OP_NULL}
};

const lwfs_svc_op *lwfs_authr_op_array()
{
	return op_array; 
}

/* ----------------- private methods --------------------------------*/

/** 
 * @brief A function to compare two uids.
 * 
 * This function returns a positive value if a > b, 
 * a negative value if a < b, and zero if they
 * are equal.  
 *
 * @param uid1  @input_type  the first uid.
 * @param uid2  @input_type  the second uid.
 */
/** 
 * @brief A function to compare two uids.
 *
 * @param uid1  @input_type  the first uid.
 * @param uid2  @input_type  the second uid.
 */
static int compare_uids(const void *a, const void *b) 
{
	const lwfs_uuid *uuid_a_ptr = (const lwfs_uuid *)a; 
	const lwfs_uuid *uuid_b_ptr = (const lwfs_uuid *)b; 

	/* a and b are char[16] */
	const char *uid_a = (const char *)(*uuid_a_ptr); 
	const char *uid_b = (const char *)(*uuid_b_ptr); 

	int i; 
	long res=0 ; 
	uint32_t inta[4], intb[4]; 

	/*
	fprintf(logger_get_file(), 
	    "uid_a[0]=%d, uid_b[0]=%d\n",(int)uid_a[0], (int)uid_b[0]);
	*/

	memcpy(&inta[0], &uid_a[0], sizeof(uint32_t));
	memcpy(&inta[1], &uid_a[4], sizeof(uint32_t));
	memcpy(&inta[2], &uid_a[8], sizeof(uint32_t));
	memcpy(&inta[3], &uid_a[12], sizeof(uint32_t));

	memcpy(&intb[0], &uid_b[0], sizeof(uint32_t));
	memcpy(&intb[1], &uid_b[4], sizeof(uint32_t));
	memcpy(&intb[2], &uid_b[8], sizeof(uint32_t));
	memcpy(&intb[3], &uid_b[12], sizeof(uint32_t));

	/*
	fprint_lwfs_uuid(logger_get_file(), "a", "", a);
	fprint_lwfs_uuid(logger_get_file(), "b", "", b);
	*/

	for (i=3; i>=0; i--) {
		res = intb[i] - inta[i]; 
		if (res != 0)
			break;
	}

	if (res > 0) 
		return 1; 
	else if (res < 0)
		return -1;
	else 
		return 0; 
}

static int compare_uids_reverse(const void *a, const void *b) 
{
	return -compare_uids(a, b);
}


/** 
 * @brief Remove duplicate uids from a UID array.
 */
static int remove_dups(void *vals, size_t nmemb, size_t size,
		int(*compar)(const void *, const void *))
{
	int i, count = 0; 
	void *a, *b; 

	/* remove the duplicates from the set list */
	if (nmemb > 0) {
		qsort(vals, nmemb, size, compar);
		count = 1; 
		a = vals; 

		for (i=1; i<nmemb; i++) {
			b = vals + i*size; 
			if (compar(a, b) != 0) {
				a = b;
				count++;
			}
			else {
				/* set the uid to inf_uid */
				/* fprintf(logger_get_file(), "found dup at i=%d, val[0]=%d\n", i, ((char *)b)[0]); */
				memset(b, -1, size);
			}
		}

		/* sort entries again, duplicates will go to end */
		qsort(vals, nmemb, size, compar);
	}

	return count;
}

/**
  * @brief Add entries from the toadd list to the uid_array. 
  */
int add_uids(
		lwfs_uid_array *orig, 
		const lwfs_uid_array *toadd)
{
	int rc = LWFS_OK;
	int num_orig = orig->lwfs_uid_array_len; 
	int num_add  = toadd->lwfs_uid_array_len; 
	int total; 

	assert(num_orig >= 0);
	assert(num_add >= 0);
	total = num_orig + num_add;
	
	log_debug(authr_debug_level, "num_orig=%d, num_add=%d, total=%d", num_orig, num_add, total); 

	/* allocate space for the new entries */
	orig->lwfs_uid_array_val = (lwfs_uid *)
		realloc(orig->lwfs_uid_array_val, total*sizeof(lwfs_uid));

	/* check for out of space error */
	if (!orig->lwfs_uid_array_val && (total > 0)) {
		log_error(authr_debug_level, "failed to allocate uid array");
		return LWFS_ERR_NOSPACE;
	}


	/* copy the uids from the toadd list to the extended original array */
	memcpy( &(orig->lwfs_uid_array_val[num_orig]), 
			toadd->lwfs_uid_array_val, 
			num_add*sizeof(lwfs_uid));

	if (logging_debug(authr_debug_level)) {
		log_debug(authr_debug_level, "printing uid_array after copy");
		fprint_lwfs_uid_array(logger_get_file(), "orig", "", orig);
	}

	/* remove duplicates */
	orig->lwfs_uid_array_len = remove_dups(
			orig->lwfs_uid_array_val,
			total, 
			sizeof(lwfs_uid),
#if defined(HAVE_CRAY_PORTALS) 
			&compare_uids_reverse);
#else
			&compare_uids);
#endif

	if (logging_debug(authr_debug_level)) {
		log_debug(authr_debug_level, "printing uid_array after dedup");
		fprint_lwfs_uid_array(logger_get_file(), "orig", "", orig);
	}

	return rc; 
}


/**
 * @brief Remove entries from a uid array. 
 */
int remove_uids(
		lwfs_uid_array *orig_array, 
		lwfs_uid_array *torm_array)
{
	int num_orig; 
	int num_rm; 
	lwfs_uid *orig; 
	lwfs_uid *torm; 
	int i; 
	int j; 
	int removed = 0; 

	/* remove duplicates from both arrays (also sorts them) */
	orig_array->lwfs_uid_array_len = remove_dups(
			orig_array->lwfs_uid_array_val,
			orig_array->lwfs_uid_array_len, 
			sizeof(lwfs_uid), 
#if defined(HAVE_CRAY_PORTALS) 
			&compare_uids_reverse);
#else
			&compare_uids);
#endif

	torm_array->lwfs_uid_array_len = remove_dups(
			torm_array->lwfs_uid_array_val,
			torm_array->lwfs_uid_array_len, 
			sizeof(lwfs_uid), 
#if defined(HAVE_CRAY_PORTALS) 
			&compare_uids_reverse);
#else
			&compare_uids);
#endif


	orig = orig_array->lwfs_uid_array_val; 
	num_orig = orig_array->lwfs_uid_array_len; 

	torm = torm_array->lwfs_uid_array_val; 
	num_rm = torm_array->lwfs_uid_array_len;

	i = j = 0;

	/* Set the uids that need to be removed to infinity. 
	 * Both lists are sorted, so this loop only requires 
	 * O(max(unset_size, size)) ops. 
	 */
	while ((i<num_orig) && (j<num_rm)) {

		int comp_res = compare_uids(orig[i], torm[j]); 

		/* null out the orig uid if it is in the delete list */
		if (comp_res == 0) {
			memset(&orig[i], -1, sizeof(lwfs_uid));
			removed++;
			j++;
			i++; 
		}

		else if (comp_res < 0) {
			i++;
		}
		else if (comp_res > 0) {
			j++;
		}
	}


	/* sort the original list to move the removed items to the end */
	qsort(orig, 
	      num_orig, 
	      sizeof(lwfs_uid), 
#if defined(HAVE_CRAY_PORTALS) 
	      &compare_uids_reverse);
#else
	      &compare_uids);
#endif


	/* update the number of items */
	orig_array->lwfs_uid_array_len = num_orig - removed; 

	return LWFS_OK;
}



/**
 * @brief Check for a uid in an acl.
 *
 * This function checks to see if the provide uid is in the 
 * acl for the cid/container_op pair. 
 *
 * @param cid    @input_type The container ID. 
 * @param container_op @input_type The operation to check. 
 * @param uid    @input_type The user ID. 
 *
 * @returns This function returns
 *     - \ref LWFS_OK if the \em uid is in the ACL, or
 *     - \ref LWFS_ERR_ACCESS if the \em uid is not in the ACL. 
 */
static int check_perm(
		const lwfs_cid cid,
		const lwfs_container_op container_op, 
		const lwfs_uid uid)
{
    int rc = LWFS_OK;

    /* only run this check if the acl database is enabled */
    if (acl_db_enabled) {
	int i;
	lwfs_uid_array acl; 
	lwfs_uid *found = NULL;
	lwfs_container_op testop = 1; 


	if (logging_debug(authr_debug_level)) {
	    fprint_lwfs_uuid(logger_get_file(), 
		    "searching for this uid", "", (const lwfs_uuid *)uid);
	}

	/* Get acls for each operation in the container op */
	for (i=0; i<NUM_CONTAINER_OPS; i++) {

	    if (testop == LWFS_CONTAINER_CREATE) {
		/* everyone is allowed to create containers */
	    }

	    else if (container_op & testop) {

		/* get the acl "cid:container_op" (db4 allocates the acl) */
		rc = authr_db_get_acl(cid, testop, &acl); 
		if (rc != LWFS_OK) {
		    log_warn(authr_debug_level, "acl not found");
		    return rc;
		}
		if (logging_debug(authr_debug_level)) {
		    fprint_lwfs_uid_array(logger_get_file(), 
			    "container acl", "container acl", &acl);
		}

		/* Find the uid in the acl. This is an O(log(n) search,
		 * where n is the number of uids in the acl.  */
		found = bsearch(uid, acl.lwfs_uid_array_val, 
			        acl.lwfs_uid_array_len,
			        sizeof(lwfs_uid), 
#if defined(HAVE_CRAY_PORTALS) 
			        &compare_uids_reverse);
#else
			        &compare_uids);
#endif

		if (found == NULL) {
		    log_warn(authr_debug_level, "uid not in acl");
		    rc = LWFS_ERR_ACCESS; 
		}
		else {
		    log_debug(authr_debug_level, "uid found in acl");
		    rc = LWFS_OK;
		}

		/* free the memory allocated by db4 */
		free(acl.lwfs_uid_array_val);
		if (rc != LWFS_OK) {
		    return rc; 
		}
	    }

	    /* shift the bit one spot to the left */
	    testop = testop << 1; 
	}
    }

    return rc; 
}


/**
 * @brief Check a credential. 
 *
 * This function validates a credential by looking in a cache of previously
 * validated credentials.  If the credential is not in the cache, we contact
 * the authentication server an request validation and store the result in the 
 * local cache. 
 *
 * @param cred    @input_type The credential to check. 
 *
 * @returns This function returns
 *     - \ref LWFS_OK if the \em uid is in the ACL, or
 *     - \ref LWFS_ERR_ACCESS if the \em uid is not in the ACL. 
 */
static int check_cred(const lwfs_cred *cred)
{
	int rc = LWFS_OK; 

	/* only run this check if creds are enabled */
	if (creds_enabled) {
		/* check the local cache of previously validated credentials */

		/* call the authentication server to validate the credential */

		/* store the result in the cache */

		/* this function is currently not supported */
		return LWFS_ERR_NOTSUPP;
	}

	return rc; 
}

/**
 * @brief Check a capability. 
 *
 * This function verifies that the MAC embedded in the cap was generated 
 * with the right key.
 *
 * @param cap    @input_type The capability to check. 
 *
 * @returns This function returns
 *     - \ref LWFS_OK if the \em uid is in the ACL, or
 *     - \ref LWFS_ERR_ACCESS if the \em uid is not in the ACL. 
 */
static int check_cap(const lwfs_cap *cap)
{
	int rc = LWFS_OK; 

	/* only run this check if caps are enabled */
	if (caps_enabled) {
		/* the verify_capability method is part of the security library (in cap.c) */
		rc = verify_cap((const lwfs_key *)(&authr_svc_key), cap); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "could not verify cap: %s",
					lwfs_err_str(rc));
			return rc; 
		}
	}

	return rc; 
}


/**
 * @brief Test to see if the capability enables the operation (\em container_op)
 *        on the container (\em cid). 
 *
 * We should do some evaluation to find out if it is faster to 
 * cryptographically validate a capability than it is to lookup 
 * the permissions in the database.  The faster operation
 * should always be performed first. 
 *
 * @param cid    @input_type ID of the container the user wants to access.
 * @param container_op @input_type The container operation to authorize.
 * @param cap    @input_type The capability that (possibly) enables the access. 
 *
 * @returns 
 *      - \ref LWFS_OK if the capability and the access-control policies allow the operation.
 *      - \ref LWFS_ERR_VERIFYCAP if the capability is not valid.
 *      - \ref LWFS_ERR_VERIFYCRED if the credential (in the cap) is not valid. 
 *      - \ref LWFS_ERR_ACCESS if the capability does not enable the requested operation.
 * 
 */
static int check_authorization(
		const lwfs_cid cid, 
		const lwfs_container_op container_op, 
		const lwfs_cap *cap)
{
	int rc; 
	const lwfs_cred *cred = &cap->data.cred;
	const lwfs_uid *uid = &cred->data.uid; 

	/* verify that the cid matches cap->data.cid */
	if (cap->data.cid != cid) {
		log_error(authr_debug_level, "cid=%d do not match cap->data.cid=%d",
				cid, cap->data.cid);
		if (logging_debug(authr_debug_level)) {
			log_debug(authr_debug_level, "printing cap");
			fprint_lwfs_cap(logger_get_file(), "cap", "DEBUG", cap);
		}
		return LWFS_ERR_VERIFYCAP; 
	}

	/* verify that the specified container op is in the cap */
	if (!(container_op & cap->data.container_op)) {
		log_error(authr_debug_level, "container_op=%x is not in "
				"the caps container_op.", container_op);
		if (logging_debug(authr_debug_level)) {
			log_debug(authr_debug_level, "printing cap");
			fprint_lwfs_cap(logger_get_file(), "cap", "DEBUG", cap);
		}
		return LWFS_ERR_VERIFYCAP;
	}


	/* verify the credential */
	rc = check_cred(cred); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to verify the credential: %s",
				lwfs_err_str(rc));
		return rc;
	}

	/* verify the capability */
	rc = check_cap(cap); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to verify the capability: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* verify the permissions from the ACL table */
	rc = check_perm(cid, container_op, *uid); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "error checking permissions: %s",
				lwfs_err_str(rc));
		return rc; 
	}
	/* if we get to this point, return TRUE */
	return LWFS_OK; 
}


/* -------- Methods used by the server -------------- */

int lwfs_authr_srvr_init(
		const lwfs_bool verify_caps, 
		const char *db_path,
		const lwfs_bool db_clear,
		const lwfs_bool db_recover,
		lwfs_service *svc) 
{
	int rc = LWFS_OK; 

	acl_db_enabled = verify_caps; 
	caps_enabled = verify_caps;
	creds_enabled = FALSE;

	/* initialize the counters */
	memset(&authr_counter, 0, sizeof(struct authr_counter));

	trace_reset_count(TRACE_AUTHR_CREATE_CID, 0, "init create cid");
	trace_reset_count(TRACE_AUTHR_REMOVE_CID, 0, "init remove cid");
	trace_reset_count(TRACE_AUTHR_CREATE_ACL, 0, "init create acl");
	trace_reset_count(TRACE_AUTHR_MODACL, 0, "init modacl");
	trace_reset_count(TRACE_AUTHR_GETACL, 0, "init getacl");
	trace_reset_count(TRACE_AUTHR_GETCAP, 0, "init getcap");
	trace_reset_count(TRACE_AUTHR_VERIFY, 0, "init verify cap");

	/* register the message encoding scheme for the authr service */
	register_authr_encodings();

	/* initialize the rpc library */
	rc = lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);   
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not init rpc: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* initialize the service to handle requests */
	rc = lwfs_service_init(LWFS_AUTHR_MATCH_BITS, LWFS_SHORT_REQUEST_SIZE, svc);   
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not init authr service: %s",
				lwfs_err_str(rc));
		return rc; 
	}

	/* add the 7 authorization operations to the list of supported ops */
	rc = lwfs_service_add_ops(svc, lwfs_authr_op_array(), 7);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not add authorization ops");
		return rc; 
	}



	/* initialize the security key used to create/verify caps */
	rc = generate_cap_key(&authr_svc_key);   /* part of security library */
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not generate key");
		return rc; 
	}

	/* initialize the auth svc database */
	if (acl_db_enabled) {
		log_debug(authr_debug_level, "about to initialize database (%s)", db_path);
		rc = authr_db_init(db_path, db_clear, db_recover); 
		if (rc != LWFS_OK) {
			log_error(authr_debug_level, "could not initialize database");
			return rc; 
		}
	}

	return LWFS_OK;
}


int lwfs_authr_srvr_fini(const lwfs_service *svc)
{
	int rc = LWFS_OK; 

	/* close the database */
	rc = authr_db_fini(); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not close database: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* shutdown the service */
	log_debug(authr_debug_level, "shutting down service library");
	rc = lwfs_service_fini(svc); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not finish service: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* shutdown the RPC library */
	rc = lwfs_rpc_fini(); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "could not finish rpc library: %s",
			lwfs_err_str(rc));
		return rc; 
	}

	/* print out statistics */
	fprintf(logger_get_file(), "--- Authr Service Stats ---\n");
	fprintf(logger_get_file(), "  Operation counts:\n");
	fprintf(logger_get_file(), "\tcreate_container = %ld\n",authr_counter.create);
	fprintf(logger_get_file(), "\tremove_container = %ld\n",authr_counter.remove);
	fprintf(logger_get_file(), "\tget_acl = %ld\n",authr_counter.getacl);
	fprintf(logger_get_file(), "\tmod_acl = %ld\n",authr_counter.modacl);
	fprintf(logger_get_file(), "\tget_cap = %ld\n",authr_counter.getcap);
	fprintf(logger_get_file(), "\tverify = %ld\n",authr_counter.verify);
	fprintf(logger_get_file(), "---------------------------\n");


	return rc; 
}


/* -------- Server-side stubs of the authentication service API --*/

/**
 * @brief Create a new container on the authorization server.
 *
 * This method creates an entry in both the container database 
 * and the acl database.  
 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to create the container
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type a capability for the new container
 */
int create_container(
		const lwfs_remote_pid *caller, 
		const lwfs_create_container_args *args,
		const lwfs_rma *data_addr,
		lwfs_cap *result)
{
	int rc = LWFS_OK; 
	int i;
	lwfs_cap_data cap_data; 
	lwfs_container_op testop = 0; 
	static const int maxlen=256;
	char newdata[maxlen];
	static int volatile num_reqs = 0; 
	int interval_id; 
	int thread_id  = lwfs_thread_pool_getrank(); 

	num_reqs++; 
	interval_id = num_reqs; 

	/* extract arguments */
	//const lwfs_txn_id txn_id = args->txn_id;
	lwfs_cid cid             = args->cid;
	const lwfs_cap *cap      = args->cap; 

	authr_counter.create++; 

	snprintf(newdata, maxlen, "create cid %llu",(long long unsigned int)cid); 
	trace_event(TRACE_AUTHR_CREATE_CID, 0, newdata);

	trace_start_interval(interval_id, thread_id); 

	log_debug(authr_debug_level, "printing cap");
	if (logging_debug(authr_debug_level)) {
		log_debug(authr_debug_level, "printing cap");
		fprint_lwfs_cap(logger_get_file(), "cap", "", cap);
	}

	/* extract convenience variables from arguments */
	const lwfs_cred *cred = &cap->data.cred; 

	log_debug(authr_debug_level, "printing cred");
	if (logging_debug(authr_debug_level)) {
		log_debug(authr_debug_level, "printing cred");
		fprint_lwfs_cred(logger_get_file(), "cred", "", cred);
	}

	if (cred == NULL) {
		log_error(authr_debug_level, "invalid capability -- no attached cred");
		rc = LWFS_ERR_SEC;
		goto cleanup;
	}


	/* check authorization to perform this op */
	/* for now, anyone can perform this op */
	/*
	   rc = check_authorization(ROOT_CID, "new_cont", cap);
	   if (rc != LWFS_OK) {
	   log_error(authr_debug_level, "not authorized to create container");
		goto cleanup;
	   }
	 */

	/* create a new container */
	rc = authr_db_create_container(&cid); 
	if (rc != LWFS_OK) {
		log_warn(authr_debug_level, "unable to create the container: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}


	/* generate is a cap that enables the user to modify the acl */
	cap_data.cid = cid; 
	cap_data.container_op = LWFS_CONTAINER_MODACL 
		| LWFS_CONTAINER_REMOVE
		| LWFS_CONTAINER_READ; 
	memcpy(&cap_data.cred, cred, sizeof(lwfs_cred));


	/* insert the acls into the database */
	if (acl_db_enabled) {

		lwfs_uid uid;
		lwfs_uid_array uid_array; 
		lwfs_uid_array empty_acl; 

		memset(&uid_array, 0, sizeof(uid_array));
		memset(&empty_acl, 0, sizeof(uid_array));

		log_debug(authr_debug_level, "copying uid to acl");
		memcpy(&uid, &cred->data.uid, sizeof(lwfs_uid));

		/* initialize uid_array with one entry (the owner) */
		uid_array.lwfs_uid_array_len = 1;
		uid_array.lwfs_uid_array_val = &uid; 

		if (logging_debug(authr_debug_level)) {
			log_debug(authr_debug_level, "printing uids");
			fprint_lwfs_uid_array(logger_get_file(), "uids", "", &uid_array);
		}


		/* put an acl that allows the user to remove the container */
		testop = 1; 
		for (i=0; i<NUM_CONTAINER_OPS; i++) {

			/* For these ops, the user is in the acl */
			if (testop & cap_data.container_op) {
				rc = authr_db_put_acl(cid, testop, &uid_array);
				if (rc != LWFS_OK) {
					log_error(authr_debug_level, "unable to add acl");
					goto cleanup;
				}
			}

			/* These ops have emtpy acls */
			else {
				rc = authr_db_put_acl(cid, testop, &empty_acl);
				if (rc != LWFS_OK) {
					log_error(authr_debug_level, "unable to add acl");
					goto cleanup;
				}
			}

			testop = testop << 1; 
		}
	}

	/* call the security library to generate the capability */
	rc = generate_cap((const lwfs_key *)&authr_svc_key, 
			&cap_data, result);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to generate cap: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}

	log_debug(authr_debug_level, "Created a new container (cid=%d)",
		result->data.cid);


cleanup:
	snprintf(newdata, maxlen, "create cid %llu", (long long unsigned int)cid); 
	trace_end_interval(interval_id, TRACE_AUTHR_CREATE_CID, 
		lwfs_thread_pool_getrank(), newdata);

	return rc;
}

/**
 * @brief Remove a container.
 *
 * The \b lwfs_remove_container method removes an 
 * existing container ID from the authorization server. 
 *
 * @note This operation does not check for "orphan" entities (i.e., 
 * existing objects, namespace entries, and so forth) before removing
 * the container ID. We leave this responsibility to the client. 
 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to remove the container
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type a capability for the new container
 */
int remove_container(
		const lwfs_remote_pid *caller, 
		const lwfs_remove_container_args *args,
		const lwfs_rma *data_addr,
		void *result)
{
	int rc = LWFS_OK;
	const lwfs_cid cid = args->cid;
	int testop = 1; 
	int i;
	static const int maxlen=256;
	char newdata[maxlen];

	int interval_id; 
	int thread_id = lwfs_thread_pool_getrank();
	
	authr_counter.remove++; 
	interval_id = authr_counter.remove; 
	snprintf(newdata, maxlen, "remove cid %llu",(long long unsigned int)cid); 
	trace_event(TRACE_AUTHR_REMOVE_CID, 0, newdata);

	trace_start_interval(interval_id, thread_id);

	log_debug(authr_debug_level, "remove a container (%d)",authr_counter.remove);

	/* is the caller authorized to remove the container? */
	rc = check_authorization(args->cid, LWFS_CONTAINER_REMOVE, args->cap);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to authorize access");
		goto cleanup; 
	}

	/* remove all ACLs */
	testop = 1; 
	for (i=0; i<NUM_CONTAINER_OPS; i++) {

		/* remove the acl for testop */
		rc = authr_db_del_acl(cid, testop); 

		switch (rc) {

			/* fine... we successfully removed an acl */
			case LWFS_OK: 
				break;

			/* fine... the acl did not exist */
			case LWFS_ERR_NOENT:
				break;

			/* we got an unexpected error */
			default:
				goto cleanup; 
		}

		testop = testop << 1; 
	}
	
	/* remove the remaining remnants of the container */
	rc = authr_db_remove_container(cid);

cleanup:
	snprintf(newdata, maxlen, "remove cid %llu",(long long unsigned int)cid); 
	trace_end_interval(interval_id, TRACE_AUTHR_REMOVE_CID, 
		thread_id, newdata);

	return rc; 
}



/**
 * @brief Get the access-control list for an 
 *        container/op pair.
 *
 * The \b lwfs_get_acl method returns a list of users that have 
 * a particular type of access to a container. 
 *
 * The args structure contains: 
 * 	- cid      @input_type the container id.
 * 	- container_op   @input_type  the operation (i.e., LWFS_CONTAINER_READ, 
 *                         LWFS_CONTAINER_WRITE, ...)
 * 	- cap      @input_type  the capability that allows us to get the ACL.
 *
 * @returns an ACL in the \em result field. 
 *
 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
 *         id for this method because getting the access-control list 
 *         does not change the state of the system. 
 */
int get_acl(
		const lwfs_remote_pid *caller, 
		const lwfs_get_acl_args *args, 
		const lwfs_rma *data_addr,
		lwfs_uid_array *result)
{
	int rc; 
	int interval_id; 
	int thread_id = lwfs_thread_pool_getrank();
	
	authr_counter.getacl++; 
	interval_id = authr_counter.getacl; 

	trace_start_interval(interval_id, thread_id); 

	log_debug(authr_debug_level, "get acl cid=%d, container_op=%d",
			args->cid, args->container_op);

	/* is the caller authorized to look at acls? */
	rc = check_authorization(args->cid, LWFS_CONTAINER_READ, args->cap);
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to authorize access");
		goto cleanup; 
	}

	rc = authr_db_get_acl(args->cid, args->container_op, result); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "acl not found");
		goto cleanup;
	}

cleanup:

	trace_end_interval(interval_id, TRACE_AUTHR_GETACL, 
		thread_id, "getacl");

	return rc;
}

/**
 * @brief Modify the access-control list for an container/op pair.
 *
 * This method changes the access-control list for a particular
 * operation on an container of objects.  The arguments 
 * include a list of users that require access (set) and 
 * a list of users that no longer require access (unset).  
 * To resolve conflicts that occur when a user ID appears in both
 * lists, the implementation first grants access to the users 
 * in the set list, then removes access from users in the unset list. 
 *
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to modify the ACL
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param notused @output_type not used. 
 *
 */ 
int mod_acl(
	const lwfs_remote_pid *caller, 
	const lwfs_mod_acl_args *args,
	const lwfs_rma *data_addr,
	void *notused)
{
    int rc = LWFS_OK;

    authr_counter.modacl++; 
    int interval_id = authr_counter.modacl++;
    int thread_id = lwfs_thread_pool_getrank();

    trace_start_interval(interval_id, thread_id);

    /* extract arguments */
    //const lwfs_txn_id txn_id    = args->txn_id; 
    const lwfs_cid cid = args->cid; 
    const lwfs_container_op container_op = args->container_op; 
    const lwfs_uid_array *set   = args->set; 
    const lwfs_uid_array *unset = args->unset; 
    const lwfs_cap *modacl_cap  = args->cap; 

    /* extract convenience variables from arguments */

    /* make sure the caller is authorized to modify acls */
    rc = check_authorization(cid, LWFS_CONTAINER_MODACL, modacl_cap);
    if (rc != LWFS_OK) {
	log_error(authr_debug_level, "unable to authorize access");
	goto cleanup;
    }

    /* perform operations on the acl database */
    if (acl_db_enabled) {

	lwfs_container_op testop = 1; 
	int i;

	/* initialize inf_uid */
	//memset(inf_uid, 0xffff, sizeof(lwfs_uid));


	if (logging_debug(authr_debug_level)) {
	    log_debug(authr_debug_level, "printing uid_arrays");
	    if (set != NULL)
		fprint_lwfs_uid_array(logger_get_file(), "set", "", set);
	    if (unset != NULL)
		fprint_lwfs_uid_array(logger_get_file(), "unset", "", unset);
	}


	for (i=0; i<NUM_CONTAINER_OPS; i++) {


	    if (testop & container_op) {

		//lwfs_uid_array orig; 
		lwfs_uid_array result; 

		//memset(&orig, 0, sizeof(orig));
		memset(&result, 0, sizeof(result));


		/* get the original acl from the database */
		rc = authr_db_get_acl(cid, testop, &result); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "could not get orig acl");
		    goto cleanup;
		}

		if (logging_debug(authr_debug_level)) {
		    FILE *fp = logger_get_file();
		    log_debug(authr_debug_level, "printing uid_array orig");
		    fprint_lwfs_uid_array(fp, "set", "", set);
		    fprint_lwfs_uid_array(fp, "remove", "", unset);
		    fprint_lwfs_uid_array(fp, "result", "", &result);
		}

		/* Adds the uids from the set array to the orig array */
		if (set != NULL) {
		    rc = add_uids(&result, set); 
		    if (rc != LWFS_OK) {
			log_error(authr_debug_level, "failed to add new uids");
			goto cleanup;
		    }
		}

		if (logging_debug(authr_debug_level)) {
		    FILE *fp = logger_get_file();
		    log_debug(authr_debug_level, "printing uid_array after add_uids()");
		    fprint_lwfs_uid_array(fp, "set", "", set);
		    fprint_lwfs_uid_array(fp, "unset", "", unset);
		    fprint_lwfs_uid_array(fp, "result", "", &result);
		}

		/* remove the unset uids from the original array */
		if (unset != NULL) {
		    rc = remove_uids(&result, (lwfs_uid_array *)unset); 
		    if (rc != LWFS_OK) {
			log_error(authr_debug_level, "failed to add new uids");
			goto cleanup;
		    }
		}

		if (logging_debug(authr_debug_level)) {
		    FILE *fp = logger_get_file();
		    log_debug(authr_debug_level, "printing uid_array after remove_uids()");
		    fprint_lwfs_uid_array(fp, "set", "", set);
		    fprint_lwfs_uid_array(fp, "unset", "", unset);
		    fprint_lwfs_uid_array(fp, "result", "", &result);
		}

		/* TODO: THESE TWO OPERATIONS NEED TO BE IN A TRANSACTION */

		/* remove the original entry from the database */
		rc = authr_db_del_acl(cid, testop); 
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "could not remove orig acl");
		    goto cleanup;
		}

		/* put the acl in the database (causes error if key already exists) */
		rc = authr_db_put_acl(cid, testop, &result);
		if (rc != LWFS_OK) {
		    log_error(authr_debug_level, "unable to add acl: %s", lwfs_err_str(rc));
		    goto cleanup;
		}

		/* free memory allocated by the database */
		free(result.lwfs_uid_array_val); 
	    }

	    /* shift the bit one space to the left */
	    testop = testop << 1; 
	}
    }

cleanup:

    trace_end_interval(interval_id, TRACE_AUTHR_MODACL, thread_id, "modacl");

    return rc; 
}

/**
 *  @brief Get a list of capabilities associated with a container. 
 *
 *  This method grants capabilities to users 
 *  that have valid credentails and have appropriate permissions 
 *  (i.e., their user ID is in the appropriate ACL) to access 
 *  the container.  
 * 
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to get a capability
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type the capability for access to container with op
 *
 * @returns the list of caps in the \em result field. 
 * 
 *  @note This method does not return partial results. If the 
 *  authorization service fails to grant all requested capabilities,
 *  \b lwfs_getcaps returns NULL for the resulting list of caps and 
 *  reports the error in the return code embedded in the request handle. 
 *
 * @remark <b>Ron (12/07/2004):</b> There's no need for a transaction
 *         id for this method because getting the access-control list 
 *         does not change the state of the system. 
 */
int get_cap(
		const lwfs_remote_pid *caller, 
		const lwfs_get_cap_args *args, 
		const lwfs_rma *data_addr,
		lwfs_cap *result)
{
	int rc = LWFS_OK;
	lwfs_cap_data cap_data;
	static const int maxlen=256;
	char newdata[maxlen];


	/* extract arguments */
	const lwfs_cid cid = args->cid; 
	const lwfs_container_op container_op = args->container_op; 
	const lwfs_cred *cred = args->cred;

	/* extract convenience variables from arguments */
	const lwfs_uid *uid = &cred->data.uid; 


	authr_counter.getcap++;
	int interval_id = authr_counter.getcap++; 
	int thread_id = lwfs_thread_pool_getrank(); 
	snprintf(newdata, maxlen, "getcap for cid=%llu", (long long unsigned int)cid);
	trace_event(TRACE_AUTHR_GETCAP, 0, newdata);


	trace_start_interval(interval_id, thread_id);

	/* verify the credential */
	rc = check_cred(cred); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "authentication failed: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* check for appropriate permissions */
	rc = check_perm(cid, container_op, *uid); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "permissions failed for container_op=%x: %s",
				container_op, lwfs_err_str(rc));
		goto cleanup;
	}

	/* if we made it this far, we can safely generate the capability */
	memset(result, 0, sizeof(lwfs_cap));
	memset(&cap_data, 0, sizeof(lwfs_cap_data));

	memcpy(&cap_data.cred, cred, sizeof(lwfs_cred));
	cap_data.cid = cid; 
	cap_data.container_op = container_op; 

	/* create the cap */
	rc = generate_cap((const lwfs_key *)(&authr_svc_key), 
			&cap_data, result); 
	if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to generate caps: %s",
				lwfs_err_str(rc));
		goto cleanup; 
	}


cleanup:
	snprintf(newdata, maxlen, "getcap for cid=%llu", (long long unsigned int)cid);
	trace_end_interval(interval_id, TRACE_AUTHR_CREATE_CID, thread_id, newdata);

	if (rc != LWFS_OK) {
	    /* exit with an error */
	    memset(result, 0, sizeof(lwfs_cap));
	}
	return rc; 
}


/**
 *  @brief Verify a list of capabilities. 
 *
 *  This method verifies that the provided capabilities were 
 *  generated by this authorization server and that they have
 *  not been modified. If all of the caps are valid, this method 
 *  also registers the requesting process (parameter \em pid) 
 *  as a user of the caps.  The authorization server
 *  uses the information in registration table to decide
 *  who to contact when revoking access to capabilities. 
 * 
	 * @param caller @input_type the client's PID
	 * @param args @input_type arguments needed to verify the container
	 * @param data_addr @input_type address at which the bulk data can be found
	 * @param result @output_type no result
 *
 * @returns This method has no result other than the return code. It returns
 * LWFS_OK if all caps are valid. 
 */
int verify_caps(
		const lwfs_remote_pid *caller, 
		const lwfs_verify_caps_args *args, 
		const lwfs_rma *data_addr,
		void *result)
{
	int rc = LWFS_OK; 
	int i; 
	result = NULL;
	static const int maxlen=256;
	char newdata[maxlen];

	authr_counter.verify++;
	int interval_id = authr_counter.verify; 
	int thread_id = lwfs_thread_pool_getrank();

	const lwfs_cap *caps = args->cap_array->lwfs_cap_array_val; 
	const int len = args->cap_array->lwfs_cap_array_len; 

	trace_start_interval(interval_id, thread_id); 


	log_debug(authr_debug_level, "start verifying caps");

	/* check each capability in the cap list */
	for (i=0; i<len; i++) {

	    const lwfs_cred *cred = &caps[i].data.cred;
	    lwfs_cid cid = caps[i].data.cid; 
	    lwfs_container_op container_op = caps[i].data.container_op; 

	    snprintf(newdata, maxlen, "verify cap with cid=%llu", (long long unsigned int)cid);
	    trace_event(TRACE_AUTHR_VERIFY, 0, newdata);

	    /* check the credential associated with the cap */
	    rc = check_cred(cred); 
	    if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to verify the credential: %s",
			lwfs_err_str(rc));
		goto cleanup;
	    }

	    /* Make sure that we generated the capablity */
	    rc = check_cap(&caps[i]); 
	    if (rc != LWFS_OK) {
		log_error(authr_debug_level, "unable to verify the cap[%d]: %s",
			i, lwfs_err_str(rc));
		goto cleanup;
	    }

	    /* make sure the permissions are still valid */
	    rc = check_perm(cid, container_op, cred->data.uid); 
	    if (rc != LWFS_OK) {
		log_error(authr_debug_level, "error checking permissions: %s",
			lwfs_err_str(rc));
		goto cleanup;
	    }


	    /* TODO: we need to register the calling process for revocation */


	}

cleanup:
	trace_end_interval(interval_id, TRACE_AUTHR_VERIFY, thread_id, "verify cap");

	log_debug(authr_debug_level, "finished verifying caps");

	return rc; 
}
