/**
 *   @file naming_srvr.c
 *
 *   @brief Implementation of the server-side methods for the LWFS naming service.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   @version $Revision: 1503 $
 *   @date $Date: 2007-08-08 10:48:35 -0600 (Wed, 08 Aug 2007) $
 */


#include <db.h>
#include <time.h>
#include "client/authr_client/authr_client_sync.h"
#include "client/authr_client/authr_client.h"
#include "support/trace/trace.h"
#include "common/naming_common/naming_trace.h"
#include "naming_server.h"
#include "naming_db.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

static lwfs_bool print_args = FALSE;


static const lwfs_oid ROOT_OID = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static lwfs_service authr_svc;
static lwfs_service naming_svc;
static struct timeval basetv;  /* initial time */

/* entry for the root directory */
static naming_db_entry root_entry;

/* entry for the orphan directory */
static naming_db_entry orphan_entry;

/**
 * @brief An array of svc operation descriptions.
 */
static const lwfs_svc_op naming_op_array[] = {
	{
		LWFS_OP_CREATE_DIR,                   /* opcode */
		(lwfs_rpc_proc)&naming_create_dir, /* func */
		sizeof(lwfs_create_dir_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_create_dir_args, /* decode args */
		sizeof(lwfs_ns_entry),                   /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry            /* encode res */
	},
	{
		LWFS_OP_REMOVE_DIR,                   /* opcode */
		(lwfs_rpc_proc)&naming_remove_dir, /* func */
		sizeof(lwfs_remove_dir_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_remove_dir_args, /* decode args */
		sizeof(lwfs_ns_entry),                   /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry            /* encode res */
	},
	{
		LWFS_OP_CREATE_FILE,                   /* opcode */
		(lwfs_rpc_proc)&naming_create_file, /* func */
		sizeof(lwfs_create_file_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_create_file_args, /* decode args */
		sizeof(lwfs_ns_entry),                    /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry             /* encode res */
	},
	{
		LWFS_OP_CREATE_LINK,                   /* opcode */
		(lwfs_rpc_proc)&naming_create_link, /* func */
		sizeof(lwfs_create_link_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_create_link_args, /* decode args */
		sizeof(lwfs_ns_entry),                    /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry             /* encode res */
	},
	{
		LWFS_OP_UNLINK,                   /* opcode */
		(lwfs_rpc_proc)&naming_unlink, /* func */
		sizeof(lwfs_unlink_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_unlink_args, /* decode args */
		sizeof(lwfs_ns_entry),                    /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry             /* encode res */
	},
	{
		LWFS_OP_LIST_DIR,                   /* opcode */
		(lwfs_rpc_proc)&naming_list_dir, /* func */
		sizeof(lwfs_list_dir_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_list_dir_args, /* decode args */
		sizeof(lwfs_ns_entry_array),            /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry_array     /* encode res */
	},
	{
		LWFS_OP_LOOKUP,                   /* opcode */
		(lwfs_rpc_proc)&naming_lookup, /* func */
		sizeof(lwfs_lookup_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_lookup_args, /* decode args */
		sizeof(lwfs_ns_entry),          /* sizeof res */
		(xdrproc_t)&xdr_lwfs_ns_entry   /* encode res */
	},
	{
		LWFS_OP_NAME_STAT,           	/* opcode */
		(lwfs_rpc_proc)&naming_stat, /* func */
		sizeof(lwfs_name_stat_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_name_stat_args, /* decode args */
		sizeof(lwfs_stat_data),          /* sizeof res */
		(xdrproc_t)&xdr_lwfs_stat_data   /* encode res */
	},
	{
		LWFS_OP_CREATE_NAMESPACE,           	/* opcode */
		(lwfs_rpc_proc)&naming_create_namespace, /* func */
		sizeof(lwfs_create_namespace_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_create_namespace_args, /* decode args */
		sizeof(lwfs_namespace),          /* sizeof res */
		(xdrproc_t)&xdr_lwfs_namespace   /* encode res */
	},
	{
		LWFS_OP_REMOVE_NAMESPACE,           	/* opcode */
		(lwfs_rpc_proc)&naming_remove_namespace, /* func */
		sizeof(lwfs_remove_namespace_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_remove_namespace_args, /* decode args */
		sizeof(lwfs_namespace),          /* sizeof res */
		(xdrproc_t)&xdr_lwfs_namespace   /* encode res */
	},
	{
		LWFS_OP_GET_NAMESPACE,           	/* opcode */
		(lwfs_rpc_proc)&naming_get_namespace, /* func */
		sizeof(lwfs_get_namespace_args),         /* sizeof args */
		(xdrproc_t)&xdr_lwfs_get_namespace_args, /* decode args */
		sizeof(lwfs_namespace),          /* sizeof res */
		(xdrproc_t)&xdr_lwfs_namespace   /* encode res */
	},
	{
		LWFS_OP_LIST_NAMESPACES,           	/* opcode */
		(lwfs_rpc_proc)&naming_list_namespaces, /* func */
		0,         /* sizeof args */
		(xdrproc_t)NULL, /* decode args */
		sizeof(lwfs_namespace_array),          /* sizeof res */
		(xdrproc_t)&xdr_lwfs_namespace_array   /* encode res */
	},
	{LWFS_OP_NULL}
};

/* ------ Support methods -------- */

/* ------ Private methods -------- */

/**
  * @brief Get the credential of the naming service. *
  *
  * TODO: actually implement this function.
  */
static int naming_get_cred(lwfs_cred *cred) {
	memset(cred, 0, sizeof(lwfs_cred));
	return LWFS_OK;
}


int update_time(lwfs_time *t)
{
	int rc = LWFS_OK;

	/*
	struct timeval tv;
        gettimeofday(&tv, 0);

        t->seconds = tv.tv_sec - basetv.tv_sec;
        t->seconds += (int)(basetv.tv_usec > tv.tv_usec);

        if (tv.tv_usec > basetv.tv_usec){
                t->nseconds = (tv.tv_usec - basetv.tv_usec)*1000;
        }
        else
        {
                t->nseconds = tv.tv_usec;
                t->nseconds += ones - basetv.tv_usec;
        }
	*/
	
	t->seconds = time(NULL);   /* time since Epoch */
	t->nseconds = 0;

	return rc;
}

static int check_perm(
	const lwfs_oid *oid,
	const lwfs_cap *cap,
	const lwfs_container_op container_op)
{
	int rc = LWFS_OK;

	naming_db_entry db_ent;

	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_cap(fp, "cap", "DEBUG\t", cap);
		fprint_lwfs_oid(fp, "oid", "DEBUG\t", oid);
	}
	
	/* Look up the entry */
	rc = naming_db_get_by_oid(oid, &db_ent);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup target entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_obj(fp, "db_ent.inode.entry_obj", "DEBUG\t", &db_ent.inode.entry_obj);
		fprint_lwfs_cap(fp, "cap", "DEBUG\t", cap);
	}
	
	log_debug(naming_debug_level, "------------------------------------------------");
	log_debug(naming_debug_level, "------------------------------------------------");
	log_debug(naming_debug_level, "------------------------------------------------");
	log_debug(naming_debug_level,
		  "db_ent.cid==0x%08x;cap.cid==0x%08x;LWFS_CID_ANY==0x%08x",
		  (unsigned long)db_ent.inode.entry_obj.cid, (unsigned long)cap->data.cid, LWFS_CID_ANY);
	log_debug(naming_debug_level, "------------------------------------------------");
	log_debug(naming_debug_level, "------------------------------------------------");
	log_debug(naming_debug_level, "------------------------------------------------");

	/* Verify that the cid matches the cap cid */
	if ((db_ent.inode.entry_obj.cid != LWFS_CID_ANY) &&
			(db_ent.inode.entry_obj.cid != cap->data.cid)) {
		log_error(naming_debug_level, "cid does not match cid of cap (db_ent.cid==0x%08x;cap.cid==0x%08x)",
			  (unsigned long)db_ent.inode.entry_obj.cid, (unsigned long)cap->data.cid);
		return LWFS_ERR_ACCESS;
	}

	log_debug(naming_debug_level, "container_op=%d, cap->data.container_op=%d",
		container_op, cap->data.container_op);

	/* Make sure bits in the container_op are set in the capability */
	if ((container_op & cap->data.container_op) != container_op) {
		log_error(naming_debug_level,
				"operation does not match cap container_op");
		return LWFS_ERR_ACCESS;
	}

	/* Verify the capability was generated by the authr server and
	 * that it has not changed.
	 */
	rc = lwfs_verify_caps_sync(&authr_svc, cap, 1);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not verify caps: %s",
				lwfs_err_str(rc));
		return rc;
	}

	return rc;
}


static lwfs_obj *get_file_obj(
	const naming_db_entry *db_entry)
{
	lwfs_obj *result = NULL;

	/* set the file object */
	if (db_entry->inode.file_obj_valid) {
		/* allocate space for file object */
		result = (lwfs_obj *)malloc(sizeof(lwfs_obj));
		memcpy(result, &db_entry->inode.file_obj, sizeof(lwfs_obj));
	}
	else if (db_entry->inode.entry_obj.type == LWFS_LINK_ENTRY) {
		int rc = LWFS_OK;

		naming_db_entry link_entry;

		/* get the link entry */
		rc = naming_db_get_by_oid(&db_entry->dirent.link, &link_entry);
		if (rc != LWFS_OK) {
			log_error(naming_debug_level, "could not get link: %s",
				lwfs_err_str(rc));
			return NULL;
		}

		/* recursive call to get object from link */
		result = get_file_obj(&link_entry);
	}

	return result;
}

static 
void copy_db_to_ns_entry(lwfs_ns_entry *ns_entry,
			 naming_db_entry *db_entry)
{
	memset(ns_entry, 0, sizeof(lwfs_ns_entry));
	strncpy(ns_entry->name, db_entry->dirent.name, LWFS_NAME_LEN);
	ns_entry->link_cnt = db_entry->inode.ref_cnt;
	memcpy(&ns_entry->dirent_oid, &db_entry->dirent.oid, sizeof(lwfs_oid));
	memcpy(&ns_entry->inode_oid, &db_entry->dirent.inode_oid, sizeof(lwfs_oid));
	memcpy(&ns_entry->parent_oid, &db_entry->dirent.parent_oid, sizeof(lwfs_oid));
	memcpy(&ns_entry->entry_obj, &db_entry->inode.entry_obj, sizeof(lwfs_obj));

	ns_entry->file_obj = get_file_obj(db_entry);
}

static 
void copy_db_to_namespace(lwfs_namespace *ns,
			  naming_db_entry *db_entry)
{
	memset(ns, 0, sizeof(lwfs_namespace));
	
	strncpy(ns->name, db_entry->dirent.name, LWFS_NAME_LEN);
	copy_db_to_ns_entry(&ns->ns_entry, db_entry);
}


/* ------ Implementation of the naming server API ------- */

/**
 * @brief Initialize a naming server.
 */
int naming_server_init(
		const char *db_path,
		const lwfs_bool db_clear,
		const lwfs_bool db_recover,
		const lwfs_service *a_svc,
		lwfs_service *n_svc)
{
	int rc = LWFS_OK;
	lwfs_cred naming_cred;

	log_debug(naming_debug_level, "entered naming_service_init");

	/* initialize the service to receive requests on a portal index */
	rc = lwfs_service_init(LWFS_NAMING_MATCH_BITS, LWFS_SHORT_REQUEST_SIZE, n_svc);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to init service: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* add naming service ops to our list of supported ops */
	rc = lwfs_service_add_ops(n_svc, lwfs_naming_op_array(), 12);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to add naming ops: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* initialize the global variable "naming_svc" */
	memcpy(&naming_svc, n_svc, sizeof(lwfs_service));
	memcpy(&authr_svc, a_svc, sizeof(lwfs_service));

	/* initialize the start time for the service */
	gettimeofday(&basetv, 0);

	/* make sure a container exists for the root container */
	rc = naming_get_cred(&naming_cred);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to get naming cred");
		return rc;
	}


	/* initialize the naming svc database */
	rc = naming_db_init(db_path, db_clear, db_recover,
			&root_entry, &orphan_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to initialize as authr_clnt: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* does the container for the root entry exist? */

	/* if it does not exist, create a new container for the root entry */

	log_debug(naming_debug_level, "finished naming_service_init");

	return LWFS_OK;
}


/**
 * @brief finalize the naming svc.
 */
int naming_server_fini(lwfs_service *n_svc)
{
	int rc = LWFS_OK;

	/* close the database */
	naming_db_fini();

	rc = lwfs_service_fini(n_svc);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to shutdown naming service");
		return rc;
	}

	return rc;
}

const lwfs_svc_op *lwfs_naming_op_array()
{
	return naming_op_array;
}


/* ------ Server-side stubs for the naming service -------- */

/**
 * @brief Create a new namespace.
 *
 * @param caller @input the process that called this function.
 * @param args   @input the arguments structure used by this function.
 * @param data_addr @input not used.
 * @param result  @output the resulting object structure.
 *
 * The \b lwfs_create_namespace method creates a new namespace using the
 * provided container ID.
 *
 * @remark <b>Todd (12/13/2006):</b>  Notice that creating a namespace
 * does not requires a capability.  The namespace is created in the
 * \ref lwfs_cid "container ID" given.  Normal access control policies
 * apply.
 */
int naming_create_namespace(
		const lwfs_remote_pid *caller,
		const lwfs_create_namespace_args *args,
		const lwfs_rma *data_addr,
		lwfs_namespace *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_name name = args->name;
	const lwfs_cid cid = args->cid;

	trace_event(TRACE_CREATE_NAMESPACE, 0, "create_namespace");

	log_debug(naming_debug_level, "starting lwfs_create_namespace");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_cid(fp, "args->cid", "DEBUG\t", &cid);
	}

	/* Initialize the database entry */
	memset(&db_entry, 0, sizeof(naming_db_entry));

	/* initialize the dirent object */
	strncpy(db_entry.dirent.name, name, LWFS_NAME_LEN);
	memcpy(&db_entry.dirent.parent_oid, &ROOT_OID, sizeof(lwfs_oid));
	naming_db_gen_oid(&db_entry.dirent.oid);  /* generate a new oid */

	/* initialize the inode object */
	memset(&db_entry.inode.entry_obj, 0, sizeof(lwfs_obj));
	memcpy(&db_entry.inode.entry_obj.svc, &naming_svc, sizeof(lwfs_service));
	db_entry.inode.entry_obj.type = LWFS_NS_OBJ;
	db_entry.inode.entry_obj.cid = cid;
	naming_db_gen_oid(&db_entry.inode.entry_obj.oid);  /* generate a new oid */
	db_entry.inode.ref_cnt = 1;
	memcpy(&db_entry.dirent.inode_oid, &db_entry.inode.entry_obj.oid, sizeof(lwfs_oid));
	db_entry.inode.file_obj_valid = FALSE;

	/* set the attributes */
	db_entry.inode.stat_data.size = 0;
	update_time(&db_entry.inode.stat_data.atime);
	memcpy(&db_entry.inode.stat_data.mtime, &db_entry.inode.stat_data.atime, sizeof(lwfs_time));
	memcpy(&db_entry.inode.stat_data.ctime, &db_entry.inode.stat_data.atime, sizeof(lwfs_time));

	/* Insert the entry into the database */
	rc = naming_db_put(&db_entry, DB_NOOVERWRITE);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level, "could not put entry in db: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	copy_db_to_namespace(result, &db_entry);

	if (logging_debug(naming_debug_level))
		fprint_lwfs_namespace(logger_get_file(), "result", "naming_create_namespace created ->", result);

cleanup:
	if (rc != LWFS_OK) {
		/* zero out the result */
		memset(result, 0, sizeof(lwfs_namespace));
	}

	log_debug(naming_debug_level, "finished lwfs_create_namespace");
	return rc;
}

int namespace_lookup(char *name,
		     lwfs_namespace *ns)
{
	int rc=LWFS_OK;
	naming_db_entry db_entry;
	char ostr[33];

	/* initialize the result */
	memset(ns, 0, sizeof(lwfs_namespace));

	/* fetch the entry */
	log_debug(naming_debug_level, "naming_db_get_by_name(%s, %s, ...)",
		lwfs_oid_to_string(ROOT_OID, ostr), name);
	rc = naming_db_get_by_name(&ROOT_OID, name, &db_entry);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level,
			"could not get entry for oid=%s, key=\"%s\": %s",
			lwfs_oid_to_string(ROOT_OID, ostr), name, lwfs_err_str(rc));
		return rc;
	}

	copy_db_to_namespace(ns, &db_entry);

	return rc;
}

/**
 * @brief Remove a namespace.
 *
 * @param caller @input the process that called this function.
 * @param args   @input the arguments structure used by this function.
 * @param data_addr @input not used.
 * @param result  @output the resulting object structure.
 *
 * The \b lwfs_remove_namespace method removes an empty namespace.
 *
 * @note There is no requirement that the target namespace be
 *       empty for the remove to complete successfully.  It is
 *       the responsibility of the file system implementation
 *       to enforce such a policy.
 */
int naming_remove_namespace(
		const lwfs_remote_pid *caller,
		const lwfs_remove_namespace_args *args,
		const lwfs_rma *data_addr,
		lwfs_namespace *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_name name = args->name;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_REMOVE_NAMESPACE, 0, "create_namespace");

	log_debug(naming_debug_level, "starting lwfs_remove_namespace");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* lookup the namespace by name */
	rc = namespace_lookup(name, result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "couldn't find a namespace named \"%s\"", name);
		return LWFS_ERR_NOENT;
	}

	/* Check permissions. The caller needs to have the capability
	 * to WRITE to the namespace.
	 */
	rc = check_perm((const lwfs_oid *)&result->ns_entry.dirent_oid, cap, LWFS_CONTAINER_WRITE);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		return rc;
	}


	/* remove the entry */
	rc = naming_db_del((const lwfs_oid *)&result->ns_entry.parent_oid, name, &db_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not remove %s", name);
		return LWFS_ERR_NAMING;
	}

	copy_db_to_namespace(result, &db_entry);

	log_debug(naming_debug_level, "finished lwfs_remove_namespace");
	return rc;
}

/**
 * @brief Find an entry in a directory.
 *
 * The \b lwfs_lookup method finds an entry by name and acquires
 * a lock for the entry. We incorporated an implicit lock into
 * the method to remove a potential race condition where an outside
 * process could modify the entry between the time the lookup call
 * found the entry and the client performs an operation on the entry.
 */
int naming_get_namespace(
		const lwfs_remote_pid *caller,
		const lwfs_get_namespace_args *args,
		const lwfs_rma *data_addr,
		lwfs_namespace *result)
{
	int rc = LWFS_OK;

	/* copy arguments */
	const lwfs_name name = args->name;

	trace_event(TRACE_GET_NAMESPACE, 0, "create_namespace");

	log_debug(naming_debug_level, "starting lwfs_get_namespace");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
	}

	/* lookup the namespace by name */
	rc = namespace_lookup(name, result);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "couldn't find a namespace named \"%s\"", name);
		return LWFS_ERR_NOENT;
	}
	strncpy(result->name, args->name, LWFS_NAME_LEN);
	if (logging_debug(naming_debug_level))
		fprint_lwfs_namespace(logger_get_file(), "namespace", "lwfs_get_namespace found ->", result);
	
	log_debug(naming_debug_level, "finished lwfs_lookup");
	return rc;
}

/**
 * @brief Get a list of namespaces that exist on this naming server.
 *
 * @ingroup naming_api
 *
 * The \b lwfs_list_namespaces method returns the list of namespaces available
 * on this naming server.
 *
 * @remark <b>Todd (12/14/2006):</b> This comment from \b lwfs_list_dir applies
 *         here as well - "I wonder if we should add another argument
 *         to filter to results on the server.  For example, only return
 *         results that match the provided regular expression".
 */
int naming_list_namespaces(
		const lwfs_service *svc,
		lwfs_namespace_array *result)
{
	int rc = LWFS_OK;
	int i;
	naming_db_entry *db_ents = NULL;
	lwfs_namespace *namespaces = NULL;
	db_recno_t count;

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_namespace_array));

	log_debug(naming_debug_level, "starting lwfs_list_namespaces");

	/* count the entries in the directory */
	rc = naming_db_get_size(&ROOT_OID, &count);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not get namespace count: %s",
				lwfs_err_str(rc));
		return rc;
	}

	if (count == 0) {
		result->lwfs_namespace_array_len = 0;
		result->lwfs_namespace_array_val = NULL;
		return LWFS_OK;
	}

	/* allocate space for the entries */
	db_ents = (naming_db_entry *)malloc(count*sizeof(naming_db_entry));
	if (db_ents == NULL) {
		log_error(naming_debug_level, "could not allocate entries");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	memset(db_ents, 0, count*sizeof(naming_db_entry));

	/* allocate space for the ns entries */
	namespaces = (lwfs_namespace *)malloc(count*sizeof(lwfs_ns_entry));
	if (namespaces == NULL) {
		log_error(naming_debug_level, "could not allocate entries");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}

	log_debug(naming_debug_level, "getting entries");

	/* fetch the namespaces */
	rc = naming_db_get_all_by_parent(&ROOT_OID, db_ents, count+1);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not get entries: %s",
				lwfs_err_str(rc));
		return rc;
	}

	log_debug(naming_debug_level, "setting namespaces");

	/* copy the objects in the entries to the result array */
	for (i=0; i<count; i++) {
		copy_db_to_namespace(&namespaces[i], &db_ents[i]);
	}

	log_debug(naming_debug_level, "setting result");

	/* set the result */
	result->lwfs_namespace_array_len = count;
	result->lwfs_namespace_array_val = namespaces;

cleanup:
	free(db_ents);

	log_debug(naming_debug_level, "finished lwfs_list_dir");

	return rc;
}

/**
 * @brief Create a new directory.
 *
 * @param caller @input the process that called this function.
 * @param args   @input the arguments structure used by this function.
 * @param data_addr @input not used.
 * @param result  @output the resulting object structure.
 *
 * The \b lwfs_create_dir method creates a new directory entry using the
 * provided container ID and adds the new entry to the list
 * of entries in the parent directory.
 *
 * @remark <b>Ron (11/30/2004):</b>  I'm not sure we want the client to supply
 *         the container ID for the directory.  If the naming service generates
 *         the container ID, it can have some control over the access permissions
 *         authorized to the client.  For example, the naming service may allow
 *         clients to perform an add_file operation (through the nameservice API),
 *         but not grant generic permission to write to the directory object.
 *
 * @remark <b>Ron (12/04/2004):</b> Barney addressed the above concern by
 *         mentioning that assigning a container ID to an entry does not
 *         necessarily mean that the data associated with the entry (e.g.,
 *         the list of other entries in a directory) will be created using the
 *         provided container.  The naming service could use a private container
 *         ID for the data.  That way, we can use the provided container ID
 *         for the access policy for namespace entries, but at the same
 *         time, only allow access to the entry data through the naming API.
 */
int naming_create_dir(
		const lwfs_remote_pid *caller,
		const lwfs_create_dir_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_name name = args->name;
	const lwfs_cid cid = args->cid;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_NAMING_MKDIR, 0, "mkdir");

	log_debug(naming_debug_level, "starting lwfs_create_dir");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_cid(fp, "args->cid", "DEBUG\t", &cid);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* make sure the parent is a directory or a namespace (root dir)*/
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory or namespace");
		rc = LWFS_ERR_NOTDIR;
		goto cleanup;
	}

	/* Check permissions. The caller needs to have the capability
	 * to modify (i.e., WRITE) to the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_WRITE);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* Initialize the database entry */
	memset(&db_entry, 0, sizeof(naming_db_entry));

	/* store info in the db_entry */
	strncpy(db_entry.dirent.name, name, LWFS_NAME_LEN);
	memcpy(&db_entry.dirent.parent_oid, &parent->dirent_oid, sizeof(lwfs_oid));
	naming_db_gen_oid(&db_entry.dirent.oid);  /* generate a new oid */

	/* initialize the entry object */
	memset(&db_entry.inode.entry_obj, 0, sizeof(lwfs_obj));
	memcpy(&db_entry.inode.entry_obj.svc, &naming_svc, sizeof(lwfs_service));
	db_entry.inode.entry_obj.type = LWFS_DIR_ENTRY;
	db_entry.inode.entry_obj.cid = cid;
	naming_db_gen_oid(&db_entry.inode.entry_obj.oid);  /* generate a new oid */
	db_entry.inode.ref_cnt = 1;

	memcpy(&db_entry.dirent.inode_oid, &db_entry.inode.entry_obj.oid, sizeof(lwfs_oid));

	/* set the attributes */
	db_entry.inode.stat_data.size = 0;
	update_time(&db_entry.inode.stat_data.atime);
	memcpy(&db_entry.inode.stat_data.mtime, &db_entry.inode.stat_data.atime, sizeof(lwfs_time));
	memcpy(&db_entry.inode.stat_data.ctime, &db_entry.inode.stat_data.atime, sizeof(lwfs_time));

	/* Insert the entry into the database */
	rc = naming_db_put(&db_entry, DB_NOOVERWRITE);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level, "could not put entry in db: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* set the result */
	copy_db_to_ns_entry(result, &db_entry);

cleanup:
	if (rc != LWFS_OK) {
		/* zero out the result */
		memset(result, 0, sizeof(lwfs_ns_entry));
	}

	log_debug(naming_debug_level, "finished lwfs_create_dir");
	return rc;
}

/**
 * @brief Remove a directory.
 *
 * The \b lwfs_remove_dir method removes an empty
 * directory entry from the parent directory.
 *
 * @note There is no requirement that the target directory be
 *       empty for the remove to complete successfully.  It is
 *       the responsibility of the file system implementation
 *       to enforce such a policy.
 *
 * @remark <b>Ron (11/30/2004):</b> Is it enough to only require a capability
 *          that allows the client to modify the contents of the parent
 *          directory, or should we also have a capability that allows
 *          removal of a particular directory entry?
 * @remark  <b>Ron (11/30/2004):</b> The interfaces to remove files and dirs
 *          reference the target file/dir by name.  Should we also have methods
 *          to remove a file/dir by its lwfs_obj handle?  This is essentially
 *          what Rolf did in the original implementation.
 */
int naming_remove_dir(
		const lwfs_remote_pid *caller,
		const lwfs_remove_dir_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_name name = args->name;
	const lwfs_cap *cap = args->cap;
	
	trace_event(TRACE_NAMING_RMDIR, 0, "rmdir");

	log_debug(naming_debug_level, "starting lwfs_remove_dir");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* make sure the parent is a directory */
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory");
		return LWFS_ERR_NOTDIR;
	}

	/* Check permissions. The caller needs to have the capability
	 * to modify (i.e., WRITE) to the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_WRITE);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		return rc;
	}


	/* remove the entry */
	rc = naming_db_del(&parent->dirent_oid, name, &db_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not remove %s", name);
		return LWFS_ERR_NAMING;
	}

	/* set the result */
	copy_db_to_ns_entry(result, &db_entry);

	log_debug(naming_debug_level, "finished lwfs_remove_dir");
	return rc;
}

/**
 * @brief Create a new file.
 *
 * The \b lwfs_create_file method creates a new file entry
 * in the specified parent directory. Associated
 * with the file is a single storage server object
 * that the client creates prior to calling
 * \b lwfs_create_file method.
 */
int naming_create_file(
		const lwfs_remote_pid *caller,
		const lwfs_create_file_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;
	naming_db_entry tmp_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_name name = args->name;
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_NAMING_CREAT, 0, "create_file");

	log_debug(naming_debug_level, "starting lwfs_create_file");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_obj(fp, "args->obj", "DEBUG\t", obj);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* make sure the parent is a directory */
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory");
		rc = LWFS_ERR_NOTDIR;
		goto cleanup;
	}

	/* Check permissions. The caller needs to have the capability
	 * to modify (i.e., WRITE) to the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_WRITE);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}

	/* store info in the db_entry */
	memset(&db_entry, 0, sizeof(naming_db_entry));
	strncpy(db_entry.dirent.name, name, LWFS_NAME_LEN);
	memcpy(&db_entry.dirent.parent_oid, &parent->dirent_oid, sizeof(lwfs_oid));
	naming_db_gen_oid(&db_entry.dirent.oid);  /* generate a new oid */

	/* initialize the entry object */
	memcpy(&db_entry.inode.entry_obj.svc, &naming_svc, sizeof(lwfs_service));
	db_entry.inode.entry_obj.type = LWFS_FILE_ENTRY;
	db_entry.inode.entry_obj.cid = obj->cid;
	naming_db_gen_oid(&db_entry.inode.entry_obj.oid);  /* generate a new oid */
	db_entry.inode.ref_cnt = 1;

	/* set the attributes */
	db_entry.inode.stat_data.size = 0;
	update_time(&db_entry.inode.stat_data.atime);
	memcpy(&db_entry.inode.stat_data.mtime, &db_entry.inode.stat_data.atime, sizeof(lwfs_time));
	memcpy(&db_entry.inode.stat_data.ctime, &db_entry.inode.stat_data.atime, sizeof(lwfs_time));

	memcpy(&db_entry.dirent.inode_oid, &db_entry.inode.entry_obj.oid, sizeof(lwfs_oid));

	/* copy the supplied object into the file object */
	db_entry.inode.file_obj_valid = TRUE;
	memcpy(&db_entry.inode.file_obj, obj, sizeof(lwfs_obj));

	/* Insert the entry into the database */
	rc = naming_db_put(&db_entry, DB_NOOVERWRITE);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level, "could not put entry in db: %s",
				lwfs_err_str(rc));
		goto cleanup;
	}

	/* sanity check (lookup the entry, use ddd to check obj vals */
	rc = naming_db_get_by_oid((const lwfs_oid *)&db_entry.dirent.oid, &tmp_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup target entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	/* initialize the result */
	copy_db_to_ns_entry(result, &db_entry);

cleanup:

	log_debug(naming_debug_level, "finished lwfs_create_file");
	return rc;
}

/**
 * @brief Create a link.
 *
 * The \b lwfs_create_link method creates a new link entry in the
 * parent directory and associates the link with an existing
 * namespace entry (file or directory) with the link.
 *
 */
int naming_create_link(
		const lwfs_remote_pid *caller,
		const lwfs_create_link_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;
	naming_db_entry target_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_name name = args->name;
	const lwfs_cap *cap = args->cap;
	const lwfs_ns_entry *target_parent = args->target_parent;
	const lwfs_name target_name = args->target_name;
	const lwfs_cap *target_cap = args->target_cap;

	trace_event(TRACE_NAMING_LINK, 0, "create_link");

	log_debug(naming_debug_level, "starting lwfs_create_link");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
		fprint_lwfs_ns_entry(fp, "args->target_parent", "DEBUG\t", target_parent);
		fprint_lwfs_name(fp, "args->target_name", "DEBUG\t", &target_name);
		fprint_lwfs_cap(fp, "args->target_cap", "DEBUG\t", target_cap);
	}

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* make sure the parent is a directory */
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory");
		rc = LWFS_ERR_NOTDIR;
		return rc;
	}

	/* Check permissions. The caller needs to have the capability
	 * to modify (i.e., WRITE) to the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_WRITE);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* Check permissions. The caller needs to have the capability
	 * to access (i.e., READ) from the target directory.
	 */
	rc = check_perm(&target_parent->dirent_oid, target_cap, LWFS_CONTAINER_READ);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize op on target: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* lookup the target entry */
	rc = naming_db_get_by_name(&target_parent->dirent_oid, target_name, &target_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup target entry: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* Initialize the database entry for the link */
	memset(&db_entry, 0, sizeof(naming_db_entry));

	/* store info in the db_entry */
	strncpy(db_entry.dirent.name, name, LWFS_NAME_LEN);
	memcpy(&db_entry.dirent.parent_oid, &parent->dirent_oid, sizeof(lwfs_oid));
	memcpy(&db_entry.dirent.link, &target_entry.inode.entry_obj.oid, sizeof(lwfs_oid));
	naming_db_gen_oid(&db_entry.dirent.oid);  /* generate a new oid */
	memcpy(&db_entry.dirent.inode_oid, &target_entry.dirent.inode_oid, sizeof(lwfs_oid));
	/* indicate that the new entry is a link to an existing entry */
	memcpy(&db_entry.dirent.link, &target_entry.dirent.oid, sizeof(lwfs_oid));

	/* copy the inode object */
	memcpy(&db_entry.inode, &target_entry.inode, sizeof(naming_db_inode));
	/* overwrite the naming_svc */ 
	memcpy(&db_entry.inode.entry_obj.svc, &naming_svc, sizeof(lwfs_service));

	/* Insert the link entry into the database */
	rc = naming_db_put(&db_entry, DB_NOOVERWRITE);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level, "could not put entry in db: %s",
				lwfs_err_str(rc));
		return rc;
	}

	/* set the result */
	copy_db_to_ns_entry(result, &db_entry);


	log_debug(naming_debug_level, "finished lwfs_create_link");
	return rc;
}

/**
 * @brief Remove a link.
 *
 * This method removes a link entry from the specified parent
 * directory. It does not modify or remove the entry
 * referenced by the link.
 *
 */
int naming_unlink(
		const lwfs_remote_pid *caller,
		const lwfs_unlink_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_name name = args->name;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_NAMING_UNLINK, 0, "unlink");

	log_debug(naming_debug_level, "starting lwfs_unlink");
	if (logging_debug(naming_debug_level)) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* make sure the parent is a directory */
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory");
		return LWFS_ERR_NOTDIR;
	}

	/* Check permissions. The caller needs to have the capability
	 * to modify (i.e., WRITE) to the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_WRITE);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* now we can remove the entry */
	rc = naming_db_del(&parent->dirent_oid, name, &db_entry);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not remove %s", name);
		/* reset the result */
		memset(result, 0, sizeof(lwfs_ns_entry));
		return LWFS_ERR_NAMING;
	}
	
	/* set the result */
	copy_db_to_ns_entry(result, &db_entry);

	log_debug(naming_debug_level, "finished lwfs_unlink");
	return rc;
}


/**
 * @brief Find an entry in a directory.
 *
 * The \b lwfs_lookup method finds an entry by name and acquires
 * a lock for the entry. We incorporated an implicit lock into
 * the method to remove a potential race condition where an outside
 * process could modify the entry between the time the lookup call
 * found the entry and the client performs an operation on the entry.
 */
int naming_lookup(
		const lwfs_remote_pid *caller,
		const lwfs_lookup_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry *result)
{
	int rc = LWFS_OK;
	naming_db_entry db_entry;
	char ostr[33];

	/* copy arguments */
	const lwfs_txn *txn_id = args->txn_id;
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_name name = args->name;
	const lwfs_lock_type lock_type = args->lock_type;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_NAMING_LOOKUP, 0, "lookup");

	log_debug(naming_debug_level, "starting lwfs_lookup");
	if (logging_debug(naming_debug_level) && print_args) {
		FILE *fp = logger_get_file();
		fprint_lwfs_txn(fp, "args->txn_id", "DEBUG\t", txn_id);
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_name(fp, "args->name", "DEBUG\t", &name);
		fprint_lwfs_lock_type(fp, "args->lock_type", "DEBUG\t", &lock_type);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry));

	/* make sure the parent is a directory */
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory");
		return LWFS_ERR_NOTDIR;
	}

	/* Check permissions. The caller needs to have the capability
	 * to access (i.e., READ) from the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_READ);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* fetch the entry */
	log_debug(naming_debug_level, "naming_db_get_by_name(parent_oid=0x%s, name=%s, ...)",
		lwfs_oid_to_string(parent->dirent_oid, ostr), name);
	rc = naming_db_get_by_name(&parent->dirent_oid, name, &db_entry);
	if (rc != LWFS_OK) {
		log_warn(naming_debug_level,
			"could not get entry for oid=%s, key=\"%s\": %s",
			lwfs_oid_to_string(parent->dirent_oid, ostr), name, lwfs_err_str(rc));
		return rc;
	}

	/* set the result */
	copy_db_to_ns_entry(result, &db_entry);

	log_debug(naming_debug_level, "finished lwfs_lookup");
	return rc;
}


/**
 * @brief Read the contents of a directory.
 *
 * The \b lwfs_list_dir method returns the list of entries in the
 * parent directory.
 *
 * @remarks <b>Ron (12/01/2004):</b> I removed the transaction ID from this call
 *          because it does not change the state of the system and we do
 *          not acquire locks on the entries returned.
 *
 * @remark <b>Ron (12/7/2004):</b> I wonder if we should add another argument
 *         to filter to results on the server.  For example, only return
 *         results that match the provided regular expression.
 */
int naming_list_dir(
		const lwfs_remote_pid *caller,
		const lwfs_list_dir_args *args,
		const lwfs_rma *data_addr,
		lwfs_ns_entry_array *result)
{
	int rc = LWFS_OK;
	int i;
	//naming_db_entry db_ents[10];
	naming_db_entry *db_ents = NULL;
	lwfs_ns_entry *ns_ents = NULL;
	db_recno_t count;

	/* copy arguments */
	const lwfs_ns_entry *parent = args->parent;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_NAMING_LS, 0, "ls");

	/* initialize the result */
	memset(result, 0, sizeof(lwfs_ns_entry_array));

	log_debug(naming_debug_level, "starting lwfs_list_dir");
	if (logging_debug(naming_debug_level) ) {
		FILE *fp = logger_get_file();
		fprint_lwfs_ns_entry(fp, "args->parent", "DEBUG\t", parent);
		fprint_lwfs_cap(fp, "args->cap", "DEBUG\t", cap);
	}

	/* make sure the parent is a directory */
	if ((parent->entry_obj.type != LWFS_DIR_ENTRY) &&
	    (parent->entry_obj.type != LWFS_NS_OBJ)) {
		log_error(naming_debug_level, "parent is not a directory");
		rc = LWFS_ERR_NOTDIR;
		return rc;
	}

	/* Check permissions. The caller needs to have the capability
	 * to access (i.e., READ) from the parent directory.
	 */
	rc = check_perm(&parent->dirent_oid, cap, LWFS_CONTAINER_READ);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		goto cleanup;
	}

	/* count the entries in the directory */
	rc = naming_db_get_size(&parent->dirent_oid, &count);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not get dir size: %s",
				lwfs_err_str(rc));
		return rc;
	}

	if (count == 0) {
		result->lwfs_ns_entry_array_len = 0;
		result->lwfs_ns_entry_array_val = NULL;
		return LWFS_OK;
	}

	/* allocate space for the entries */
	db_ents = (naming_db_entry *)malloc(count*sizeof(naming_db_entry));
	if (db_ents == NULL) {
		log_error(naming_debug_level, "could not allocate entries");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}
	memset(db_ents, 0, count*sizeof(naming_db_entry));

	/* allocate space for the ns entries */
	ns_ents = (lwfs_ns_entry *)malloc(count*sizeof(lwfs_ns_entry));
	if (ns_ents == NULL) {
		log_error(naming_debug_level, "could not allocate entries");
		rc = LWFS_ERR_NOSPACE;
		goto cleanup;
	}

	log_debug(naming_debug_level, "getting entries");

	/* fetch the entries in the directory */
	rc = naming_db_get_all_by_parent(&parent->dirent_oid, db_ents, count);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not get entries: %s",
				lwfs_err_str(rc));
		return rc;
	}

	log_debug(naming_debug_level, "setting ns_ents");

	/* copy the objects in the entries to the result array */
	for (i=0; i<count; i++) {
		copy_db_to_ns_entry(&ns_ents[i], &db_ents[i]);
	}

	log_debug(naming_debug_level, "setting result");

	/* set the result */
	result->lwfs_ns_entry_array_len = count;
	result->lwfs_ns_entry_array_val = ns_ents;

cleanup:
	free(db_ents);

	log_debug(naming_debug_level, "finished lwfs_list_dir");

	return rc;
}

int naming_list_all() {
	return naming_db_print_all();
}


/**
 * @brief Stat the namespace entry.
 *
 */
int naming_stat(
	const lwfs_remote_pid *caller,
	const lwfs_name_stat_args *args,
	const lwfs_rma *data_addr,
	lwfs_stat_data *res)
{
	int rc = LWFS_OK;
	naming_db_entry db_ent;
	
	/* extract the arguments */
	//const lwfs_txn *txn_id = args->txn_id;
	const lwfs_obj *obj = args->obj;
	const lwfs_cap *cap = args->cap;

	trace_event(TRACE_NAMING_STAT, 0, "stat file");

	/* initialize the result */
	memset(res, 0, sizeof(lwfs_stat_data));

	/* Check permissions. The caller needs to have the capability
	 * to access (i.e., READ) from object.
	 */
	rc = check_perm(&obj->oid, cap, LWFS_CONTAINER_READ);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "unable to authorize operation: %s",
			lwfs_err_str(rc));
		return rc;
	}

	/* Look up the entry */
	rc = naming_db_get_by_oid(&obj->oid, &db_ent);
	if (rc != LWFS_OK) {
		log_error(naming_debug_level, "could not lookup target entry: %s",
				lwfs_err_str(rc));
		return rc;
	}

	/* copy the attributes from the entry to the result */
	memcpy(res, &db_ent.inode.stat_data, sizeof(lwfs_stat_data));

	return rc;
}
