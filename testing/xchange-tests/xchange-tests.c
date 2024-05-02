/**
 * Test file for XChange integration
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cmdline.h"
#include "client/authr_client/authr_client.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/authr_client/authr_client_opts.h"
#include "client/naming_client/naming_client.h"
#include "client/naming_client/naming_client_sync.h"
#include "client/naming_client/naming_client_opts.h"
#include "client/storage_client/storage_client.h"
#include "client/storage_client/storage_client_sync.h"
#include "client/storage_client/storage_client_opts.h"
#include "common/naming_common/naming_options.h"

#include "support/logger/logger_opts.h"

#include "ecl.h"

int test_result (FILE *fp, const char* func_name, int rc, int expected);
int test_int (FILE *fp, const char* name, int val, int expected);
int test_str (FILE * fp, const char * name, const char * val, const char * expected);
int test_equiv (FILE * fp, const char * name, void * val, void * expected, lwfs_size size);

/* these 4 decls are the type declarations for the test_ecl code gen example.
 * they can be replaced by dynamic generation using the XMIT
 * tool Patrick wrote, but it has to be extracted from PBIO
 * into a pure parsing tool for ECL (Jay's ToDo).
 */
struct input
{
	int one;
	int two;
};

struct output
{
	int average;
};

IOField input_fields [] =
{
	 {"one", "integer", sizeof (int), IOOffset (struct input *, one)}
	,{"two", "integer", sizeof (int), IOOffset (struct input *, two)}
	,{NULL, NULL, 0, 0}
};

IOField output_fields [] =
{
	 {"average", "integer", sizeof (int), IOOffset (struct output *, average)}
	,{NULL, NULL, 0, 0}
};

const char * test_ecl_code_string =
"{\n"
"    out.average = (in.one + in.two) / 2;\n"
"    return out.average;\n"
"}";

/* these 4 decls are for the dummied type info used in the file_read command */
struct input_v
{
	int x [10];
};

struct output_v
{
	int y [10];
};

IOField input_v_fields [] =
{
	 {"x", "integer[10]", sizeof (int), IOOffset (struct input_v *, x)}
	,{NULL, NULL, 0, 0}
};

IOField output_v_fields [] =
{
	 {"y", "integer[10]", sizeof (int), IOOffset (struct output_v *, y)}
	,{NULL, NULL, 0, 0}
};

const char * file_read_code_string =
"{\n"
"	int i;\n"
"	for (i = 0; i < 10; i++)\n"
"		out.y [i] = 2 * in.x [i];\n"
"	return 0;\n"
"}";

const char * DIR_STR = "tmp";
const lwfs_name NS_NAME = "namespace_dt";

typedef long (* TRANSFORM_FN) (void *, void *);

/* A collection of the things needed for file operations.
 * This is created during the file_init and cleaned up
 * during the file_shutdown.
 */
typedef struct
{
	lwfs_cred cred;
	lwfs_service authr_svc;
	lwfs_service naming_svc;
	lwfs_service * storage_svc;
} LWFS_CREDENTIALS;

/* These are the things necessary to access a file
 * for reading/writing once it has been identified
 * and accessed
 */
typedef struct
{
	lwfs_cap cap;
	lwfs_uid_array uid_array;
	lwfs_cid cid;
	lwfs_txn * txn;
	lwfs_namespace namespace;
	lwfs_ns_entry dir;
	lwfs_ns_entry file;
	lwfs_obj obj;
	ecl_parse_context context; /* used for code gen */
	ecl_code gen_code; /* used for code gen */
	TRANSFORM_FN fn; /* transform fn */
} LWFS_FILE;

/* The various new API functions for a proposed library layer file API */
int file_init (struct gengetopt_args_info * args_info, lwfs_cred * cred, LWFS_CREDENTIALS * user);
LWFS_FILE * file_open (LWFS_CREDENTIALS * user, const char * filename, lwfs_opcode mode);
LWFS_FILE * file_open_transformed (LWFS_CREDENTIALS * user, const char * filename, lwfs_opcode mode, const char * transform);
int file_link_transformed (LWFS_CREDENTIALS * user, const char * orig_file, const char * new_file, const char * transform);
int file_read (LWFS_CREDENTIALS * user, LWFS_FILE * file, lwfs_size offset, char * buffer, int len, lwfs_size * bytes_read);
int file_write (LWFS_CREDENTIALS * USER, LWFS_FILE * file, lwfs_size offset, char * buffer, int len);
int file_close (LWFS_CREDENTIALS * user, LWFS_FILE * file);
int file_delete (LWFS_CREDENTIALS * user ,const char * filename);
int file_shutdown (LWFS_CREDENTIALS * user);

/* the three testing harness entry points */
int test_api (struct gengetopt_args_info * args_info);
int test_raw (struct gengetopt_args_info * args_info);
int test_transform_api (struct gengetopt_args_info * args_info);

/* globals until the authr service gets the root dir cap fixes done */
lwfs_cid cid;
lwfs_cap cap;


static void print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
	print_logger_opts(fp, args_info, prefix); 
	print_authr_client_opts(fp, args_info, prefix); 
	print_naming_client_opts(fp, args_info, prefix); 
	print_storage_client_opts(fp, args_info, prefix); 
}



/* Initialize the file system by getting the servers and credentials
 * identified and stored for all other file operations
 */
int file_init (
		struct gengetopt_args_info * args_info, 
		lwfs_cred * cred, 
		LWFS_CREDENTIALS * user)
{
	int rc = LWFS_OK;
	lwfs_remote_pid authr_id; 
	lwfs_remote_pid naming_id; 
	lwfs_remote_pid *ss_ids = NULL; 

	memset (user, 0, sizeof (LWFS_CREDENTIALS));
	memcpy (&user->cred, cred, sizeof (lwfs_cred));

	/* get the descriptor for the authorization service */
	authr_id.pid = args_info->authr_pid_arg; 
	authr_id.nid = args_info->authr_nid_arg; 
	memset (&user->authr_svc, 0, sizeof (lwfs_service));
	rc = lwfs_get_service (authr_id, &user->authr_svc);
	if (!test_result (stdout, "lwfs_get_service (authr)", LWFS_OK, rc))
		goto cleanup;

	/* get the storage service descriptors */
	ss_ids = (lwfs_remote_pid *) 
		calloc(args_info->ss_num_servers_arg, sizeof(lwfs_remote_pid));
	user->storage_svc = (lwfs_service *) 
		calloc (args_info->ss_num_servers_arg, sizeof (lwfs_service));

	/* read the storage server file to get service ids */
	rc = read_ss_file(
			args_info->ss_server_file_arg,
			args_info->ss_num_servers_arg,
			ss_ids); 
	if (rc != LWFS_OK) {
		log_error(args_info->verbose_arg, "could ont read ss file");
		goto cleanup;
	}

	/* get service descriptions for each storage server */
	rc = lwfs_get_services(ss_ids, 
			args_info->ss_num_servers_arg, 
			user->storage_svc); 
	if (rc != LWFS_OK) {
		log_error(args_info->verbose_arg, "could not contact ss services");
		goto cleanup;
	}

	/* get the naming service */
	naming_id.nid = args_info->naming_nid_arg; 
	naming_id.pid = args_info->naming_pid_arg; 
	memset (&user->naming_svc, 0, sizeof (lwfs_service));
	rc = lwfs_get_service (naming_id, &user->naming_svc);
	if (rc != LWFS_OK) {
		log_error(args_info->verbose_arg, "could not contact naming service");
		goto cleanup;
	}

cleanup:
	free(ss_ids); 
	return rc;
}

/* Open a file.  This works similar to the standard fopen (), but uses an lwfs
 * storage backend.  Eventually this will be smart enough to realize that the
 * file being opened is a transform on another file and do the proper fixup.
 * We need attributes on objects before we can effectively do that.
 */
LWFS_FILE * file_open (LWFS_CREDENTIALS * user, const char * filename, lwfs_opcode mode)
{
	int rc = LWFS_OK;
	lwfs_cap create_cid_cap;
	lwfs_cap modacl_cap;
	LWFS_FILE * f = (LWFS_FILE *) malloc (sizeof (LWFS_FILE));

	memset (f, 0, sizeof (LWFS_FILE));

	f->uid_array.lwfs_uid_array_len = 1;
	f->uid_array.lwfs_uid_array_val = (lwfs_uid *) &user->cred.data.uid;
	f->txn = NULL; /* currently unused */

	/* To open a file, we need to do the following
	 * (assuming that we do not have existing files
	 * to worry about):
	 * 1. Get the capability (make a container)
	 * 2. Create an object
	 * 3. Create a namespace entry associated
	 *    with the object.
         */
#if 0
	/* initialize the container id used in the calls below */
	cid = lwfs_get_cid (lwfs_get_obj (LWFS_NAMING_ROOT));

	/* get the capabilities for the container */
	rc = lwfs_get_cap_sync (user->authr_svc, cid, mode, user->cred, &cap);
	if (!test_result (stdout, "lwfs_get_cap ()", rc, LWFS_OK))
		goto cleanup;

	/* get the user ids with these capabilities for this (cid) container */
	rc = lwfs_get_acl_sync (user->authr_svc, cid, mode, &cap, &uid_array);
	if (!test_result (stdout, "lwfs_get_acl ()", rc, LWFS_OK))
		goto cleanup;
#endif
	/* get a capability id by creating a container to represent it */
	rc = lwfs_create_container_sync (&user->authr_svc, f->txn, LWFS_CID_ANY, &create_cid_cap, &modacl_cap);
	if (!test_result (stdout, "lwfs_create_container ()", rc, LWFS_OK))
		goto cleanup;

	/* initialize the capability id used in the calls below */
	f->cid = modacl_cap.data.cid;
	/* save the cid until lwfs code is fixed */
	cid = modacl_cap.data.cid;

	/* create the ACLs */
	rc = lwfs_create_acl_sync (&user->authr_svc, f->txn, f->cid, mode, &f->uid_array, &modacl_cap);
	if (!test_result (stdout, "lwfs_create_acl ()", rc, LWFS_OK))
		goto cleanup;

	/* get the capability */
	rc = lwfs_get_cap_sync (&user->authr_svc, f->cid, mode, &user->cred, &f->cap);
	/* save the cap until the lwfs code is fixed */
	memcpy (&cap, &f->cap, sizeof (cap));
	if (!test_result (stdout, "lwfs_get_cap ()", rc, LWFS_OK))
		goto cleanup;

	/* create a namespace for the directory to live in */
	rc = lwfs_create_namespace_sync (&user->naming_svc, f->txn, NS_NAME, cid, &f->namespace);
	if (!test_result (stdout, "lwfs_create_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* get the newly created namespace */
	rc = lwfs_get_namespace_sync (&user->naming_svc, NS_NAME, &f->namespace);
	if (!test_result (stdout, "lwfs_get_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* create a directory in which to store the filename */
	rc = lwfs_create_dir_sync (&user->naming_svc, f->txn, &f->namespace.ns_entry, DIR_STR, f->cid, &f->cap, &f->dir);
	if (!test_result (stdout, "lwfs_create_dir ()", rc, LWFS_OK))
		goto cleanup;

	/* populate the object data structure */
	rc = lwfs_init_obj (user->storage_svc, LWFS_GENERIC_OBJ, f->cid, LWFS_OID_ANY, &f->obj);
	if (!test_result (stdout, "lwfs_init_obj ()", rc, LWFS_OK))
		goto cleanup;

	/* create the object */
	rc = lwfs_create_obj_sync (f->txn, &f->obj, &f->cap);
	if (!test_result (stdout, "lwfs_create_obj ()", rc, LWFS_OK))
		goto cleanup;

	/* create a filename for accessing the object */
	rc = lwfs_create_file_sync (&user->naming_svc, f->txn, &f->dir, filename, &f->obj, &f->cap, &f->file);
	if (!test_result (stdout, "lwfs_create_file ()", rc, LWFS_OK))
		goto cleanup;

	return f;

cleanup:
	free (f);

	return NULL;
}

/* Like file_open () except that an explicit transform is supplied */
LWFS_FILE * file_open_transformed (LWFS_CREDENTIALS * user
				  ,const char * filename
				  ,lwfs_opcode mode
				  ,const char * transform
				  )
{
	int rc = LWFS_OK;
	lwfs_cap create_cid_cap;
	lwfs_cap modacl_cap;
	LWFS_FILE * f = (LWFS_FILE *) malloc (sizeof (LWFS_FILE));

	memset (f, 0, sizeof (LWFS_FILE));

	f->uid_array.lwfs_uid_array_len = 1;
	f->uid_array.lwfs_uid_array_val = (lwfs_uid *) &user->cred.data.uid;
	f->txn = NULL; /* currently unused */

	/* To open a file, we need to do the following
	 * (assuming that we do not have existing files
	 * to worry about):
	 * 1. Get the capability (make a container)
	 * 2. Create an object
	 * 3. Create a namespace entry associated
	 *    with the object.
         */
#if 0
	/* initialize the container id used in the calls below */
	cid = lwfs_get_cid (lwfs_get_obj (namespace_root));

	/* get the capabilities for the container */
	rc = lwfs_get_cap_sync (user->authr_svc, cid, mode, user->cred, &cap);
	if (!test_result (stdout, "lwfs_get_cap ()", rc, LWFS_OK))
		goto cleanup;

	/* get the user ids with these capabilities for this (cid) container */
	rc = lwfs_get_acl_sync (user->authr_svc, cid, mode, &cap, &uid_array);
	if (!test_result (stdout, "lwfs_get_acl ()", rc, LWFS_OK))
		goto cleanup;
#endif
	/* get a capability id by creating a container to represent it */
	rc = lwfs_create_container_sync (&user->authr_svc, f->txn, LWFS_CID_ANY, &create_cid_cap, &modacl_cap);
	if (!test_result (stdout, "lwfs_create_container ()", rc, LWFS_OK))
		goto cleanup;

	/* initialize the capability id used in the calls below */
	f->cid = modacl_cap.data.cid;
	/* save the cid until lwfs code is fixed */
	cid = modacl_cap.data.cid;

	/* create the ACLs */
	rc = lwfs_create_acl_sync (&user->authr_svc, f->txn, f->cid, mode, &f->uid_array, &modacl_cap);
	if (!test_result (stdout, "lwfs_create_acl ()", rc, LWFS_OK))
		goto cleanup;

	/* get the capability */
	rc = lwfs_get_cap_sync (&user->authr_svc, f->cid, mode, &user->cred, &f->cap);
	/* save the cap until the lwfs code is fixed */
	memcpy (&cap, &f->cap, sizeof (cap));
	if (!test_result (stdout, "lwfs_get_cap ()", rc, LWFS_OK))
		goto cleanup;

	/* create a namespace for the directory to live in */
	rc = lwfs_create_namespace_sync (&user->naming_svc, f->txn, NS_NAME, cid, &f->namespace);
	if (!test_result (stdout, "lwfs_create_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* get the newly created namespace */
	rc = lwfs_get_namespace_sync (&user->naming_svc, NS_NAME, &f->namespace);
	if (!test_result (stdout, "lwfs_get_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* create a directory in which to store the filename */
	rc = lwfs_create_dir_sync (&user->naming_svc, f->txn, &f->namespace.ns_entry, DIR_STR, f->cid, &f->cap, &f->dir);
	if (!test_result (stdout, "lwfs_create_dir ()", rc, LWFS_OK))
		goto cleanup;

	/* populate the object data structure */
	rc = lwfs_init_obj (user->storage_svc, LWFS_GENERIC_OBJ, f->cid, LWFS_OID_ANY, &f->obj);
	if (!test_result (stdout, "lwfs_init_obj ()", rc, LWFS_OK))
		goto cleanup;

	/* create the object */
	rc = lwfs_create_obj_sync (f->txn, &f->obj, &f->cap);
	if (!test_result (stdout, "lwfs_create_obj ()", rc, LWFS_OK))
		goto cleanup;

	/* create a filename for accessing the object */
	rc = lwfs_create_file_sync (&user->naming_svc, f->txn, &f->dir, filename, &f->obj, &f->cap, &f->file);
	if (!test_result (stdout, "lwfs_create_file ()", rc, LWFS_OK))
		goto cleanup;

	/* work the transform into a code string here */
	/* need to parse the types into the proper input and
	 * output types for the transform function and generate
	 * the code string based on the transforms
	 */
	transform = transform; /* remove the warning for now */
	char * code_string = (char *) file_read_code_string;

	f->context = new_ecl_parse_context ();
	/* add the input parameter type */
	ecl_add_struct_type ("input", input_v_fields, f->context);
	/* add the output parameter type */
	ecl_add_struct_type ("output", output_v_fields, f->context);
	/* describe the calling conventions of the generated fn */
	ecl_subroutine_declaration ("long proc (input * in, output * out)", f->context);
	/* generate the code */
	f->gen_code = ecl_code_gen (code_string, f->context);
	f->fn = (TRANSFORM_FN) (long) f->gen_code->func;

	return f;

cleanup:
	free (f);

	return NULL;
}

/* This is used to create a link filename that connects with another file
 * except that when this is used, the contents will be transformed using
 * the supplied transform code.  This can't be implemented until we get
 * attributes.
 */
int file_link_transformed (LWFS_CREDENTIALS * user, const char * orig_file
			  ,const char * new_file, const char * transform
			  )
{
	int rc = LWFS_OK;

	/* */

	return rc;
}

/* Read the contents of a file (object).  If a transform should be applied, it is
 * recognized and applied before returning the data.  This assumes only one transform
 * is being applied at a time right now.  They could be linked, but it hasn't been done
 */
int file_read (LWFS_CREDENTIALS * user, LWFS_FILE * file, lwfs_size offset
	      ,char * buffer, int len, lwfs_size * bytes_read
	      )
{
	int rc = LWFS_OK;

	if (file->fn == NULL)
	{
		rc = lwfs_read_sync (file->txn, &file->obj, offset, buffer, len, &file->cap, bytes_read);
	}
	else
	{
		/* if we are performing on the client machine, do it this way */
		void * buf = malloc (len);
		rc = lwfs_read_sync (file->txn, &file->obj, offset, buf, len, &file->cap, bytes_read);
		if (rc == LWFS_OK)
		{
			rc = file->fn (buf, buffer);
			if (!rc)
				rc = LWFS_OK;
			else
				rc = LWFS_ERR;
		}
		
		free (buf);
		/* otherwise, we need to have it running on the server
		 * which means that it has to be installed there somehow
		 */
	}
	if (!test_result (stdout, "lwfs_read ()", rc, LWFS_OK))
		goto cleanup;

	return LWFS_OK;
cleanup:
	return rc;
}

/* write data to a file (object) */
int file_write (LWFS_CREDENTIALS * USER, LWFS_FILE * file, lwfs_size offset, char * buffer, int len)
{
	int rc = LWFS_OK;

	rc = lwfs_write_sync (file->txn, &file->obj, offset, buffer, len, &file->cap);
	if (!test_result (stdout, "lwfs_write ()", rc, LWFS_OK))
		goto cleanup;

	return LWFS_OK;
cleanup:
	return rc;
}

/* there isn't anthing to do to close an object except clean up the data structures */
int file_close (LWFS_CREDENTIALS * user, LWFS_FILE * file)
{
	if (file->gen_code)
		ecl_code_free (file->gen_code);
/* this causes a crash right now so skip it.
 *	if (file->context)
 *		ecl_free_parse_context (file->context);
 */

	free (file);

	return LWFS_OK;
}

/* remove an object */
int file_delete (LWFS_CREDENTIALS * user, const char * filename)
{
	int rc = LWFS_OK;
	lwfs_txn txn;
#if 0
	lwfs_cid cid;
	lwfs_opcode mode = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;
	lwfs_cap cap;
	lwfs_uid_array uid_array;
#endif
	lwfs_lock_type lock_type = LWFS_LOCK_NULL;
	lwfs_ns_entry dir;
	lwfs_ns_entry file;
	lwfs_namespace namespace;

	/* Once we get the capability from the credentials, we
	 * need to find the filename in the nameserver to get
	 * the object it refers to and then need to unlink the
	 * file object and remove the filename from the namespace
	 */
#if 0
	/* initialize the container id used in the calls below */
	cid = lwfs_get_cid (lwfs_get_obj (namespace_root));

	/* get the capabilities for the container */
	rc = lwfs_get_cap_sync (user->authr_svc, cid, mode, user->cred, &cap);
	if (!test_result (stdout, "lwfs_get_cap ()", rc, LWFS_OK))
		goto cleanup;

	/* get the user ids with these capabilities for this (cid) container */
	rc = lwfs_get_acl_sync (user->authr_svc, cid, mode, &cap, &uid_array);
	if (!test_result (stdout, "lwfs_get_acl ()", rc, LWFS_OK))
		goto cleanup;
#endif

	/* lookup the namespace */
	rc = lwfs_get_namespace_sync (&user->naming_svc, NS_NAME, &namespace);
	if (!test_result (stdout, "lwfs_get_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* lookup the directory */
	rc = lwfs_lookup_sync (&user->naming_svc, &txn, &namespace.ns_entry, DIR_STR, lock_type, &cap, &dir);
	if (!test_result (stdout, "lwfs_lookup ()", rc, LWFS_OK))
		goto cleanup;

	/* lookup the filename */
	rc = lwfs_lookup_sync (&user->naming_svc, &txn, &dir, filename, lock_type, &cap, &file);
	if (!test_result (stdout, "lwfs_lookup ()", rc, LWFS_OK))
		goto cleanup;

	/* remove the object */
	rc = lwfs_remove_obj_sync (&txn, file.file_obj, &cap);
	if (!test_result (stdout, "lwfs_remove ()", rc, LWFS_OK))
		goto cleanup;

	/* unlink the file */
	rc = lwfs_unlink_sync (&user->naming_svc, &txn, &dir, filename, &cap, &file);
	if (!test_result (stdout, "lwfs_unlink ()", rc, LWFS_OK))
		goto cleanup;

	/* remove the dir */
	rc = lwfs_remove_dir_sync (&user->naming_svc, &txn, &namespace.ns_entry, DIR_STR, &cap, &dir);
	if (!test_result (stdout, "lwfs_remove_dir ()", rc, LWFS_OK))
		goto cleanup;

	/* remove the namespace */
	rc = lwfs_remove_namespace_sync (&user->naming_svc, &txn, NS_NAME, &cap, &namespace);
	if (!test_result (stdout, "lwfs_remove_namespace ()", rc, LWFS_OK))
		goto cleanup;

	return 0;
cleanup:
	return rc;
}

/* clean up the system cache of storage information */
int file_shutdown (LWFS_CREDENTIALS * user)
{
	free (user->storage_svc);

	return LWFS_OK;
}

/* test the API without invoking any of the transform code */
int test_api (struct gengetopt_args_info * args_info)
{
	lwfs_cred cred;
	LWFS_CREDENTIALS user;
	LWFS_FILE * file;
	const char * FILENAME = "file";
	const char * TEST_STRING = "This is a test";
	char buf [1024];
	lwfs_size bytes_read;

	int rc = LWFS_OK;

	/* the credentials are just dummied right now */
	memset (&cred, 0, sizeof (lwfs_cred));
	log_info (args_info->verbose_arg, "*** BEGIN file_init ()\n");
	rc = file_init (args_info, &cred, &user);
	test_result (stdout, "file_init ()", rc, LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_init ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_open ()\n");
	file = file_open (&user, FILENAME, LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ);
	test_result (stdout, "file_open ()", (file ? LWFS_OK : LWFS_ERR), LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_open ()\n");

	strcpy (buf, TEST_STRING);

	log_info (args_info->verbose_arg, "*** BEGIN file_write ()\n");
	rc = file_write (&user, file, 0, buf, sizeof (buf));
	test_result (stdout, "file_write ()", rc, LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_write ()\n");

	memset (buf, 0, sizeof (buf));

	log_info (args_info->verbose_arg, "*** BEGIN file_read ()\n");
	rc = file_read (&user, file, 0, buf, sizeof (buf), &bytes_read);
	if (!test_result (stdout, "file_read ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_read ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_close ()\n");
	rc = file_close (&user, file);
	if (!test_result (stdout, "file_close ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_close ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_delete ()\n");
	rc = file_delete (&user, FILENAME);
	if (!test_result (stdout, "file_delete ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_delete ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_shutdown ()\n");
	rc = file_shutdown (&user);
	if (!test_result (stdout, "file_shutdown ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_shutdown ()\n");

cleanup:
	return rc;
}

/* test the raw lwfs api doing similar operations as those done in the other tests */
int test_raw (struct gengetopt_args_info * args_info)
{
	int rc = LWFS_OK;

	lwfs_opcode opcodes = LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ;
	lwfs_cred cred;
	lwfs_cap cap;
	lwfs_cap modacl_cap;
	lwfs_cap create_cid_cap;
	lwfs_txn * txn = NULL; /* currently unused */
	lwfs_uid_array uid_array;
	lwfs_obj obj;
	lwfs_cid cid; 
	lwfs_ns_entry dir;
	lwfs_ns_entry file;
	const char * dir_str ="tmp";
	const char * file_str = "file";
	LWFS_CREDENTIALS user;
	const lwfs_name ns_name = "namespace_dt";
	lwfs_namespace namespace;
	lwfs_ns_entry * namespace_root = NULL;

	const char * outbuf = "example buffer";
	char inbuf [256];
	lwfs_size bytes_read = 0;

	/* initialize the user data structures */
	rc = file_init (args_info, &cred, &user);
	test_result (stdout, "file_init ()", rc, LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_init ()\n");

	/* The following code must be done almost no matter
	 * what the operations
	 */
	/* BEGIN SETUP */
	/* initialize the uid array (manage credentials) */
	uid_array.lwfs_uid_array_len = 1;
	uid_array.lwfs_uid_array_val = (lwfs_uid *) &cred.data.uid;

	/* get a capability id by creating a container to represent it */
	rc = lwfs_create_container_sync (&user.authr_svc, txn, LWFS_CID_ANY, &create_cid_cap, &modacl_cap);
	if (!test_result (stdout, "lwfs_create_container ()", rc, LWFS_OK))
		goto cleanup;

	cid = modacl_cap.data.cid;

	/* create the ACLs */
	rc = lwfs_create_acl_sync (&user.authr_svc, txn, cid, opcodes, &uid_array, &modacl_cap);
	if (!test_result (stdout, "lwfs_create_acl ()", rc, LWFS_OK))
		goto cleanup;

	/* get the capability */
	rc = lwfs_get_cap_sync (&user.authr_svc, cid, opcodes, &cred, &cap);
	if (!test_result (stdout, "lwfs_get_cap ()", rc, LWFS_OK))
		goto cleanup;
	/* END SETUP */

	/* create a namespace for the directory to live in */
	rc = lwfs_create_namespace_sync (&user.naming_svc, txn, ns_name, cid, &namespace);
	if (!test_result (stdout, "lwfs_create_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* get the newly created namespace */
	rc = lwfs_get_namespace_sync (&user.naming_svc, ns_name, &namespace);
	if (!test_result (stdout, "lwfs_get_namespace ()", rc, LWFS_OK))
		goto cleanup;

	namespace_root = &namespace.ns_entry;

	/* create a database in which to store the filename */
	rc = lwfs_create_dir_sync (&user.naming_svc, txn, namespace_root, dir_str, cid, &cap, &dir);
	if (!test_result (stdout, "lwfs_create_dir ()", rc, LWFS_OK))
		goto cleanup;

	/* create the object */
	rc = lwfs_init_obj (user.storage_svc, LWFS_GENERIC_OBJ, cid, LWFS_OID_ANY, &obj);
	if (!test_result (stdout, "lwfs_init_obj ()", rc, LWFS_OK))
		goto cleanup;

	rc = lwfs_create_obj_sync (txn, &obj, &cap);
	if (!test_result (stdout, "lwfs_create_obj ()", rc, LWFS_OK))
		goto cleanup;

	/* write the data */
	rc = lwfs_write_sync (txn, &obj, 0, outbuf, strlen (outbuf), &cap);
	if (!test_result (stdout, "lwfs_write ()", rc, LWFS_OK))
		goto cleanup;

	/* flush to the storage */
	rc = lwfs_fsync_sync (txn, &obj, &cap);
	if (!test_result (stdout, "lwfs_fsync ()", rc, LWFS_OK))
		goto cleanup;

	/* create a filename for accessing the object */
	rc = lwfs_create_file_sync (&user.naming_svc, txn, &dir, file_str, &obj, &cap, &file);
	if (!test_result (stdout, "lwfs_create_file ()", rc, LWFS_OK))
		goto cleanup;

	/* read the data back to verify */
	rc = lwfs_read_sync (txn, &obj, 0, inbuf, 256, &cap, &bytes_read);
	if (!test_result (stdout, "lwfs_read ()", rc, LWFS_OK))
		goto cleanup;

	if (!test_int (stdout, "bytes_read", bytes_read, strlen (outbuf)))
		goto cleanup;

	/* remove the object */
	rc = lwfs_remove_obj_sync (txn, &obj, &cap);
	if (!test_result (stdout, "lwfs_remove ()", rc, LWFS_OK))
		goto cleanup;

	/* unlink the file */
	rc = lwfs_unlink_sync (&user.naming_svc, txn, &dir, file_str, &cap, &file);
	if (!test_result (stdout, "lwfs_unlink ()", rc, LWFS_OK))
		goto cleanup;

	/* remove the dir */
	rc = lwfs_remove_dir_sync (&user.naming_svc, txn, namespace_root, dir_str, &cap, &dir);
	if (!test_result (stdout, "lwfs_remove_dir ()", rc, LWFS_OK))
		goto cleanup;

	/* remove the namespace */
	rc = lwfs_remove_namespace_sync (&user.naming_svc, txn, ns_name, &cap, &namespace);
	if (!test_result (stdout, "lwfs_remove_namespace ()", rc, LWFS_OK))
		goto cleanup;

	/* remove the container */ /* this is currently non-functional */
	/*
	 *rc = lwfs_remove_container_sync (&user.authr_svc, txn, args->cid, &cap);
	 *if (!test_result (stdout, "lwfs_remove_container ()", rc, LWFS_OK))
	 *	goto cleanup;
	 */

	/* shutdown */
	return LWFS_OK;
cleanup:
	free (user.storage_svc);

	return rc;
}

/* similar to the test_api () except that it invokes the transforms */
int test_transform_api (struct gengetopt_args_info * args_info)
{
	lwfs_cred cred;
	LWFS_CREDENTIALS user;
	LWFS_FILE * file;
	const char * FILENAME = "file";
	int x [10];
	char buf [10 * sizeof (int)];
	lwfs_size bytes_read;

	int rc = LWFS_OK;

	/* setup the test data */
	int i;
	for (i = 0; i < 10; i++) x [i] = i;

	/* the credentials are just dummied right now */
	memset (&cred, 0, sizeof (lwfs_cred));
	log_info (args_info->verbose_arg, "*** BEGIN file_init ()\n");
	rc = file_init (args_info, &cred, &user);
	test_result (stdout, "file_init ()", rc, LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_init ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_open_transformed ()\n");
	file = file_open_transformed (&user, FILENAME, LWFS_CONTAINER_WRITE | LWFS_CONTAINER_READ, "");
	test_result (stdout, "file_open_transformed ()", (file ? LWFS_OK : LWFS_ERR), LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_open_transformed ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_write ()\n");
	rc = file_write (&user, file, 0, (void *) x, sizeof (buf));
	test_result (stdout, "file_write ()", rc, LWFS_OK);
	log_info (args_info->verbose_arg, "*** END file_write ()\n");

	memset (buf, 0, sizeof (buf));

	log_info (args_info->verbose_arg, "*** BEGIN file_read ()\n");
	rc = file_read (&user, file, 0, buf, sizeof (buf), &bytes_read);
	if (!test_result (stdout, "file_read ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_read ()\n");

	/* test the transform code */
	{
		int * y = (int *) buf;
		for (i = 0; i < 10; i++)
			if (x [i] * 2 != y [i])
				fprintf (stdout, "ECL test didn't work x: %d y: %d\n", x [i], y [i]);
	}

	log_info (args_info->verbose_arg, "*** BEGIN file_close ()\n");
	rc = file_close (&user, file);
	if (!test_result (stdout, "file_close ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_close ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_delete ()\n");
	rc = file_delete (&user, FILENAME);
	if (!test_result (stdout, "file_delete ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_delete ()\n");

	log_info (args_info->verbose_arg, "*** BEGIN file_shutdown ()\n");
	rc = file_shutdown (&user);
	if (!test_result (stdout, "file_shutdown ()", rc, LWFS_OK))
		goto cleanup;
	log_info (args_info->verbose_arg, "*** END file_shutdown ()\n");

cleanup:
	return rc;
}

/* a simple test to see if the ECL code generation is working */
int test_ecl ()
{
	char * code_string = (char *) test_ecl_code_string;

	ecl_parse_context context = new_ecl_parse_context ();
	ecl_code gen_code;
	TRANSFORM_FN fn;
	long result;

	/* add the input parameter type */
	ecl_add_struct_type ("input", input_fields, context);
	/* add the output parameter type */
	ecl_add_struct_type ("output", output_fields, context);
	/* describe the calling conventions of the generated fn */
	ecl_subroutine_declaration ("long proc (input * in, output * out)", context);
	/* generate the code */
	gen_code = ecl_code_gen (code_string, context);
	/* extract the function */
	fn = (TRANSFORM_FN) (long) gen_code->func;

	struct input in = {3, 9};
	struct output out;
	result = (fn) ((void *) &in, (void *) &out);
	test_int (stdout, "ecl result", result, 6);
	test_int (stdout, "ecl output", out.average, 6);

	ecl_code_free (gen_code);
/* this causes a crash right now so skip it.
 *	ecl_free_parse_context (context);
 */

	return 0;
}

int main (int argc, char ** argv)
{
	struct gengetopt_args_info args_info;		/* for command line arguments */

	int rc = LWFS_OK;

	/* Parse command line options to override defaults */
	if (cmdline_parser (argc, argv, &args_info) != 0)
	{
		exit (1);
	}

	/* initialize the logger */
	logger_init (args_info.verbose_arg, args_info.logfile_arg);

	/* initialize RPC */
	lwfs_rpc_init (LWFS_RPC_PTL, LWFS_RPC_XDR);


	/* if the debug level says so, print the args */
	if (args_info.verbose_arg > 2)
	{
		print_args (stdout, &args_info, "");
	}	

        /* DO MY WORK HERE */
	switch (args_info.testid_arg)
	{
		case 0:	/* try my new API */
			rc = test_api (&args_info);
			test_result (stdout, "testing private API", rc, LWFS_OK);
			break;

		case 1:	/* try the API with a transform */
			rc = test_transform_api (&args_info);
			test_result (stdout, "testing transform API ()", rc, LWFS_OK);
			rc = test_ecl ();
			test_result (stdout, "test_ecl ()", rc, LWFS_OK);
			break;

		case 2:	/* use the LWFS API directly */
			rc = test_raw (&args_info);
			test_result (stdout, "testing LWFS raw", rc, LWFS_OK);
			break;

		default:/* unknown */
			rc = LWFS_ERR;
			fprintf (stdout, "Unknown testing mode: %d\n", args_info.testid_arg);
			break;
	}

	return 0;
}

/* -------------- PRIVATE METHODS -------------- */
/* These were stolen from the other LWFS test programs */
int test_result(FILE * fp, const char * func_name, int rc, int expected)
{
        fprintf (fp, "testing  %-24s expecting rc=%-15s ... ", func_name, lwfs_err_str (expected));
        if (rc != expected)
	{
                fprintf (fp, "FAILED (rc=%s)\n", lwfs_err_str(rc));
        }
        else
	{
                fprintf (fp, "PASSED\n");
        }

        return rc == expected;
}

int test_int(FILE * fp, const char * name, int val, int expected)
{
        fprintf (fp, "   expecting %s=%d ... ", name, expected);
        if (val != expected)
	{
                fprintf (fp, "FAILED (%s=%d)\n", name, val);
        }
        else
	{
                fprintf (fp, "PASSED\n");
        }

        return val == expected;
}

int test_str (FILE * fp, const char * name, const char * val, const char * expected)
{
        fprintf (fp, "   expecting %s=\"%s\" ... ", name, expected);
        if (strcmp (val, expected) != 0)
	{
                fprintf (fp, "FAILED (%s=%s)\n", name, val);

                return 0;
        }
        else
	{
                fprintf (fp, "PASSED\n");

                return 1;
        }
}

int test_equiv (FILE * fp, const char * name, void * val, void * expected, lwfs_size size)
{
        fprintf (fp, "testing  %-24s ... ", name);
        if (memcmp (val, expected, size) != 0)
	{
                fprintf (fp, "FAILED\n");

                return FALSE;
        }
        else
	{
                fprintf (fp, "PASSED\n");

                return TRUE;
        }
}
