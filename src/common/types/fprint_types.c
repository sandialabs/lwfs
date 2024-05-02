/**  
 *   @file fprint_types.c
 *   
 *   @brief Implementation of methods to initialize and finalize 
 *   client interaction with the LWFS metadata services and storage 
 *   servers. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1497 $.
 *   $Date: 2007-08-08 10:42:51 -0600 (Wed, 08 Aug 2007) $.
 *
 */

#include <stdio.h>

#include "config.h"

#if STDC_HEADERS
#include <string.h> /* find memcpy() */
#endif

#include "types.h"
#include "fprint_types.h"
#include "support/logger/logger.h"

#define _XOPEN_SOURCE
#include <unistd.h>



/* ----------- Utility functions to output types ---- */


/**
 * @brief Print out a return code.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The return code.
 */
void fprint_lwfs_return_code(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const int rc) 
{
	logger_mutex_lock();
	fprintf(fp, "%s %s = %s\n", prefix, name, lwfs_err_str(rc));
	logger_mutex_unlock();
}

static const char *myitoa(int val) {
	static char buf[32]; 

	snprintf(buf, sizeof(buf), "%d", val); 
	return buf; 
}

/**
 * @brief Output a string associated with a return code.
 * 
 * @param rc @input the return code. 
 * 
 * @returns A string associated with the return code. 
 */
const char *lwfs_err_str(int rc) 
{
	switch (rc) {
		case LWFS_OK:
			return "LWFS_OK";

		case LWFS_ERR:
			return "LWFS_ERR";

		case LWFS_ERR_NOTSUPP:
			return "LWFS_ERR_NOTSUPP";

		case LWFS_ERR_NOSPACE:
			return "LWFS_ERR_NOSPACE";


		case LWFS_ERR_STORAGE:
			return "LWFS_ERR_STORAGE";

		case LWFS_ERR_NO_OBJ:
			return "LWFS_ERR_NO_OBJ";

		case LWFS_ERR_NAMING:
			return "LWFS_ERR_NAMING";

		case LWFS_ERR_PERM:
			return "LWFS_ERR_PERM";

		case LWFS_ERR_NOENT:
			return "LWFS_ERR_NOENT";

		case LWFS_ERR_ACCESS:
			return "LWFS_ERR_ACCESS";

		case LWFS_ERR_EXIST:
			return "LWFS_ERR_EXIST";

		case LWFS_ERR_NOTDIR:
			return "LWFS_ERR_NOTDIR";

		case LWFS_ERR_ISDIR:
			return "LWFS_ERR_ISDIR";

		case LWFS_ERR_RPC:
			return "LWFS_ERR_RPC";

		case LWFS_ERR_ENCODE:
			return "LWFS_ERR_ENCODE";

		case LWFS_ERR_DECODE:
			return "LWFS_ERR_DECODE";

		case LWFS_ERR_SEC:
			return "LWFS_ERR_SEC";

		case LWFS_ERR_GENKEY:
			return "LWFS_ERR_GENKEY";

		case LWFS_ERR_VERIFYCRED:
			return "LWFS_ERR_VERIFYCRED";

		case LWFS_ERR_VERIFYCAP:
			return "LWFS_ERR_VERIFYCAP";

		default:
			return myitoa(rc);
	}
}

/**
 * @brief Print out a uuid.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The uuid.
 */
void fprint_lwfs_uuid(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_uuid *uuid) 
{
	uint32_t uid_int[4];
	char *uuid_char = (char*)uuid;

	memcpy(&uid_int[0], &uuid_char[0], sizeof(uint32_t));
	memcpy(&uid_int[1], &uuid_char[4], sizeof(uint32_t));
	memcpy(&uid_int[2], &uuid_char[8], sizeof(uint32_t));
	memcpy(&uid_int[3], &uuid_char[12], sizeof(uint32_t));

	logger_mutex_lock();

	fprintf(fp, "%s %s = {0x%08X, 0x%08X, 0x%08X, 0x%08X}\n", 
			prefix, name, uid_int[3], uid_int[2], 
			uid_int[1], uid_int[0]);
	
	/*
	   fprintf(fp, "%s", crypt((char *)uuid, "uu"));
	 */

	/* footer */
	fprintf(fp, "\n");
	logger_mutex_unlock();
}

/**
 * @brief Output the contents of an lwfs_uid_array
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param md      The remote memory descriptor.
 */
void fprint_lwfs_uid_array(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_uid_array *acl)
{
	int i;
	char subprefix[100]; 
	char index[100];

	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	if (acl == NULL) {
		fprintf(fp, "%s %s = NULL\n", prefix, name);
		return; 
	}

	fprintf(fp, "%s %s = {\n", prefix, name);



	/* contents */
	for (i=0; i<acl->lwfs_uid_array_len; i++) {
		snprintf(index, 100, "[%d]", i); 
		fprint_lwfs_uuid(fp, 
				 subprefix, 
				 index,
				 (const lwfs_uuid *)(acl->lwfs_uid_array_val[i]));
	}

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}


/**
 * @brief Output the contents of a process ID
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param md      The remote memory descriptor.
 */
void fprint_lwfs_remote_pid(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_remote_pid *id)
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprintf(fp, "%s    nid = %llu,\n", subprefix, (unsigned long long)id->nid);
	fprintf(fp, "%s    pid = %llu,\n", subprefix, (unsigned long long)id->pid);

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}


/**
 * @brief Output the contents of a Portals remote memory descriptor.
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param addr    The remote memory address.
 */
void fprint_lwfs_rma(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_rma *addr)
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	if (addr == NULL) {
		fprintf(fp, "%s %s = NULL\n", prefix, name);
		return; 
	}

	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprint_lwfs_remote_pid(fp, "match_id", subprefix, &addr->match_id);
	fprintf(fp, "%s    buffer_id = %d,\n", subprefix, addr->buffer_id);
	fprintf(fp, "%s    offset = %ld,\n", subprefix, (long)addr->offset);
	fprintf(fp, "%s    match_bits = %llu,\n", subprefix, (unsigned long long)addr->match_bits);
	fprintf(fp, "%s    len = %ld,\n", subprefix, (long)addr->len);

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}

/**
 * @brief Output the contents of a request header.
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param hdr     The request header.
 */
void fprint_lwfs_request_header(
		FILE *fp, 
		const char *name, 
		const char *prefix,
		const lwfs_request_header *hdr)
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprintf(fp, "%s    id = %lu,\n", subprefix, hdr->id);
	fprintf(fp, "%s    opcode = %u,\n", subprefix, hdr->opcode);
	fprintf(fp, "%s    fetch_args = %d,\n", subprefix, hdr->fetch_args);
	fprint_lwfs_rma(fp, "args_addr", subprefix, &(hdr->args_addr));
	fprint_lwfs_rma(fp, "data_addr", subprefix, &hdr->data_addr);
	fprint_lwfs_rma(fp, "res_addr", subprefix, &(hdr->res_addr));

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}

/**
 * @brief Output the contents of a result header.
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param hdr     The result header.
 */
void fprint_lwfs_result_header(
		FILE *fp, 
		const char *name, 
		const char *prefix,
		const lwfs_result_header *hdr)
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprintf(fp, "%s    id = %lu,\n", subprefix, hdr->id);
	fprintf(fp, "%s    fetch_result = %d,\n", subprefix, hdr->fetch_result);
	fprint_lwfs_rma(fp, "res_addr", subprefix, &hdr->result_addr);
	fprintf(fp, "%s    rc = %d,\n", subprefix, hdr->rc);

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}


/**
 * @brief Print out a message-authentication code.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The credential data.
 */
void fprint_lwfs_mac(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_mac *mac) 
{
	uint32_t mac_int;
	char *mac_char = (char*)mac;

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = 0x", prefix, name);
	memcpy(&mac_int, &(mac_char[0]), sizeof(mac_int));
	fprintf(fp, "%08X\n", mac_int);
	fprintf(fp, "%s %s = 0x", prefix, name);
	memcpy(&mac_int, &(mac_char[4]), sizeof(mac_int));
	fprintf(fp, "%08X\n", mac_int);
	fprintf(fp, "%s %s = 0x", prefix, name);
	memcpy(&mac_int, &(mac_char[8]), sizeof(mac_int));
	fprintf(fp, "%08X\n", mac_int);
	fprintf(fp, "%s %s = 0x", prefix, name);
	memcpy(&mac_int, &(mac_char[12]), sizeof(mac_int));
	fprintf(fp, "%08X\n", mac_int);
	fprintf(fp, "%s %s = 0x", prefix, name);
	memcpy(&mac_int, &(mac_char[16]), sizeof(mac_int));
	fprintf(fp, "%08X\n", mac_int);
	
	fprintf(fp, "%s %s = 0x", prefix, name);

	/* contents */
	/*
	   fprintf(fp, "%s", crypt((char *)mac, "mc"));
	 */

	/* footer */
	fprintf(fp, "\n");
	logger_mutex_unlock();
}

/**
 * @brief Print out credential data.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The credential data.
 */
void fprint_lwfs_cred_data(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_cred_data *cred_data) 
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprint_lwfs_uuid(fp, "uid", subprefix, &(cred_data->uid));

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}


/**
 * @brief Print out a credential.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The capability list. 
 */
void fprint_lwfs_cred(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_cred *cred)
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprint_lwfs_cred_data(fp, "data", subprefix, &(cred->data));
	fprint_lwfs_mac(fp, "mac", subprefix, &(cred->mac));

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}



/**
 * @brief Print out capability data.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The data. 
 */
void fprint_lwfs_cap_data(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_cap_data *cap_data)
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	/*
	   fprint_lwfs_uuid(fp, "cid", subprefix, &(cap_data->cid));
	 */
	fprintf(fp, "%s %s = 0x%08x\n", subprefix, "cid", (unsigned int)cap_data->cid);

	/* operation code */
	fprintf(fp, "%s %s = 0x%08x\n", subprefix, "container_op", 
			(unsigned int)cap_data->container_op);

	/* credential */
	fprint_lwfs_cred(fp, "cred", subprefix, &cap_data->cred);

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}


/**
 * @brief Print out a capability list.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The capability list. 
 */
void fprint_lwfs_cap(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_cap *cap) 
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprint_lwfs_cap_data(fp, "data", subprefix, &(cap->data));
	fprint_lwfs_mac(fp, "mac", subprefix, &(cap->mac));

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}

/**
 * @brief Print out a capability list.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The capability list. 
 */
void fprint_lwfs_cap_list(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_cap_list *cap_list)
{
	char subprefix[100]; 
	char subname[10]; 

	int count = 0;
	lwfs_cap_list *cur = (lwfs_cap_list *)cap_list; 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);
	snprintf(subprefix, 100, "%s\t", prefix); 

	/* contents */
	while (cur != NULL) {
		snprintf(subname, 10, "%s[%d]", name, count++); 
		fprint_lwfs_cap(fp, subname, subprefix, &(cur->cap));
		cur = cur->next; 
	}

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}




/**
 * @brief Print out lwfs_rpc_encode.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The encoding.
 */
void fprint_lwfs_rpc_encode(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_rpc_encode *rpc_encode) 
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s", prefix); 

	/* contents */
	switch (*rpc_encode) {
		case LWFS_RPC_XDR:
			fprintf(fp, "%s    %s = LWFS_RPC_XDR,\n", subprefix, name);
			break;

		default:
			fprintf(fp, "%s    %s = UNDEFINED,\n", subprefix, name);
			break;
	}
}




/**
 * @brief Print out an lwfs service descriptor.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The service.
 */
void fprint_lwfs_service(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_service *svc) 
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprint_lwfs_rpc_encode(fp, "rpc_encode", subprefix, &(svc->rpc_encode));
	fprint_lwfs_rma(fp, "req_addr", subprefix, &(svc->req_addr));
	fprintf(fp, "%s    max_reqs = %u,\n", subprefix, svc->max_reqs);

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}


/**
 * @brief Print out an ssize. 
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The oid.
 */
void fprint_lwfs_ssize(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_ssize *ssize) 
{
	char subprefix[100]; 

	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* contents */
	fprintf(fp, "%s %s = %lu\n", prefix, name, (unsigned long)*ssize);
	logger_mutex_unlock();
}

/**
 * @brief Print out an lwfs_time struct. 
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The time struct.
 */
void fprint_lwfs_time(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_time *time) 
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprintf(fp, "%s     seconds  = %u\n", prefix, time->seconds);
	fprintf(fp, "%s     nseconds = %u\n", prefix, time->nseconds);

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}

/**
 * @brief Print out an obj attributes. 
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The object attributes.
 */
void fprint_lwfs_stat_data(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_stat_data *data) 
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	fprint_lwfs_time(fp, "atime", subprefix, &(data->atime));
	fprint_lwfs_time(fp, "mtime", subprefix, &(data->atime));
	fprint_lwfs_time(fp, "ctime", subprefix, &(data->atime));
	fprint_lwfs_ssize(fp, "size", subprefix, &(data->size));

	/* footer */
	fprintf(fp, "%s }\n", prefix);
	logger_mutex_unlock();
}


/**
 * @brief Print out an lwfs container ID. 
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The service.
 */
void fprint_lwfs_cid(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_cid *cid) 
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* contents */
	fprintf(fp, "%s %s = 0x%08lx (%lu)\n", prefix, name, 
		 (unsigned long)*cid, (unsigned long)*cid);
	logger_mutex_unlock();
}

/**
 * @brief Print out an lwfs lock ID. 
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The lock ID.
 */
void fprint_lwfs_lock_id(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_lock_id *lock_id) 
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	/* contents */
	fprintf(fp, "%s %s = %u\n", prefix, name, *lock_id);
	logger_mutex_unlock();
}

/**
 * @brief Print out an lwfs object reference.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The obj.
 */
void fprint_lwfs_obj(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_obj *obj) 
{
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s\t", prefix); 

	logger_mutex_lock();
	if (obj == NULL) {
		fprintf(fp, "%s %s = NULL\n", prefix, name);

	}
	else {

		/* header */
		fprintf(fp, "%s %s = {\n", prefix, name);

		/* contents */
		fprint_lwfs_service(fp, "svc", subprefix, &(obj->svc));
		fprintf(fp, "%s type = %d,\n", subprefix, obj->type);
		fprint_lwfs_cid(fp, "cid", subprefix, &(obj->cid));
		fprint_lwfs_oid(fp, "oid", subprefix, &(obj->oid));
		fprint_lwfs_lock_id(fp, "lock_id", subprefix, &(obj->lock_id));

		/* footer */
		fprintf(fp, "%s }\n", prefix);
	}
	logger_mutex_unlock();
}


/**
 * @brief Print out an lwfs transaction.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The txn.
 */
void fprint_lwfs_txn(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_txn *txn_id) 
{
	logger_mutex_lock();
	if (txn_id == NULL) {
		fprintf(fp, "%s %s = NULL\n", prefix, name);
	}

	else {
		char subprefix[100]; 
		snprintf(subprefix, 100, "%s\t", prefix); 

		/* header */
		fprintf(fp, "%s %s = {\n", prefix, name);

		/* contents */
		fprint_lwfs_obj(fp, "journal", subprefix, &(txn_id->journal));

		/* footer */
		fprintf(fp, "%s }\n", prefix);
	}
	logger_mutex_unlock();
}


/**
 * @brief Print out an lwfs name.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The txn.
 */
void fprint_lwfs_name(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_name *nm) 
{
	/* contents */
	fprintf(fp, "%s %s = %s\n", prefix, name, *nm);
}

/**
 * @brief Print out an lwfs_lock_type.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The txn.
 */
void fprint_lwfs_lock_type(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_lock_type *type) 
{
	logger_mutex_lock();
	/* contents */
	switch (*type) {
		case LWFS_LOCK_NULL:
			fprintf(fp, "%s %s = LWFS_LOCK_NULL\n", prefix, name);
			break;
		case LWFS_WRITE_LOCK:
			fprintf(fp, "%s %s = LWFS_WRITE_LOCK\n", prefix, name);
			break;
		case LWFS_READ_LOCK:
			fprintf(fp, "%s %s = LWFS_READ_LOCK\n", prefix, name);
			break;
		default:
			fprintf(fp, "%s %s = UNDEFINED LOCK TYPE\n", prefix, name);
			break;
	}
	logger_mutex_unlock();
}

/**
 * @brief Output the contents of an lwfs_ns_entry.
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param md      The remote memory descriptor.
 */
void fprint_lwfs_distributed_obj(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_distributed_obj *dso)
{
	int i;
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	if (dso == NULL) {
		fprintf(fp, "%s %s = NULL\n", prefix, name);

	}
	else {
		/* header */
		fprintf(fp, "%s %s = {\n", prefix, name);
	
		fprintf(fp, "%s    chunk_size=%d, ss_obj_count=%d\n", subprefix, 
			(int)dso->chunk_size, (int)dso->ss_obj_count);
	
		for (i=0; i<dso->ss_obj_count; i++) {
			char obj_prefix[100];
			sprintf(obj_prefix, "%s        ", prefix);
			fprint_lwfs_obj(fp, "dso_obj", obj_prefix, &dso->ss_obj[i]);
		}
	
		/* footer */
		fprintf(fp, "%s }\n", subprefix);
	}
	logger_mutex_unlock();
}

/**
 * @brief Output the contents of an lwfs_ns_entry.
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param md      The remote memory descriptor.
 */
void fprint_lwfs_ns_entry(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_ns_entry *ns_entry)
{
	char ostr[33];
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	const lwfs_ns_entry *ns_ent = ns_entry;

	fprintf(fp, "%s    dirent_oid=%s, inode_oid=%s, name=\"%s\", link_cnt=%d, type=", subprefix, 
		lwfs_oid_to_string(ns_ent->dirent_oid, ostr), 
		lwfs_oid_to_string(ns_ent->inode_oid, ostr), 
		ns_ent->name, 
		ns_ent->link_cnt);

	switch (ns_ent->entry_obj.type) {
		case LWFS_GENERIC_OBJ:
			fprintf(fp, "generic\n");
			break; 

		case LWFS_DIR_ENTRY:
			fprintf(fp, "dir ent\n");
			break; 

		case LWFS_FILE_ENTRY:
			fprintf(fp, "file ent\n");
			break; 

		case LWFS_LINK_ENTRY:
			fprintf(fp, "link ent\n");
			break; 

		case LWFS_FILE_OBJ:
			fprintf(fp, "file obj\n");
			break; 

		case LWFS_NS_OBJ:
			fprintf(fp, "namespace obj\n");
			break; 

		default:
			fprintf(fp, "undefined\n");
			break; 
	}
	{
		char entry_obj_prefix[100];
		sprintf(entry_obj_prefix, "%s        ", prefix);
		fprint_lwfs_obj(fp, "entry_obj", entry_obj_prefix, &ns_entry->entry_obj);
	}
	{
		char file_obj_prefix[100];
		sprintf(file_obj_prefix, "%s        ", prefix);
		fprint_lwfs_obj(fp, "file_obj", file_obj_prefix, ns_entry->file_obj);
	}
	{
		char dso_prefix[100];
		sprintf(dso_prefix, "%s        ", prefix);
		fprint_lwfs_distributed_obj(fp, "dso", dso_prefix, ns_entry->d_obj);
	}

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}

/**
 * @brief Output the contents of an lwfs_ns_entry_array.
 *
 * @param fp      File pointer (where to send output)
 * @param prefix  Text to put on every line before the output.
 * @param md      The remote memory descriptor.
 */
void fprint_lwfs_ns_entry_array(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_ns_entry_array *array)
{
	int i;
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	for (i=0; i<array->lwfs_ns_entry_array_len; i++) {
		lwfs_ns_entry *ns_ent = &array->lwfs_ns_entry_array_val[i];

		fprint_lwfs_ns_entry(fp, name, prefix, ns_ent);
	}

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}

/**
 * @brief Print out an lwfs namespace.
 *
 * @param fp     The output file.
 * @param name   The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param        The namespace.
 */
void fprint_lwfs_namespace(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_namespace *ns) 
{
	/* contents */
	const lwfs_ns_entry *ns_ent = &ns->ns_entry;

	logger_mutex_lock();

	/* header */
	fprintf(fp, "%s %s = { %s }\n", prefix, name, ns->name);

	fprint_lwfs_ns_entry(fp, name, prefix, ns_ent);

	logger_mutex_unlock();
}

/**
 * @brief Output the contents of an lwfs_namespace_array.
 *
 * @param fp      File pointer (where to send output)
 * @param name    The name of the variable. 
 * @param prefix  Text to put on every line before the output.
 * @param array   The namespace array
 */
void fprint_lwfs_namespace_array(
		FILE *fp, 
		const char *name,
		const char *prefix,
		const lwfs_namespace_array *array)
{
	int i;
	char subprefix[100]; 
	snprintf(subprefix, 100, "%s   ", prefix); 

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s = {\n", prefix, name);

	/* contents */
	for (i=0; i<array->lwfs_namespace_array_len; i++) {
		lwfs_namespace *namespace = &array->lwfs_namespace_array_val[i];

		fprint_lwfs_namespace(fp, name, prefix, namespace);
	}

	/* footer */
	fprintf(fp, "%s }\n", subprefix);
	logger_mutex_unlock();
}

/**
 * @brief Print out a uuid.
 *
 * @param fp  The output file.
 * @param name The name of the variable. 
 * @param prefix Text that precedes the variable. 
 * @param The oid.
 */
void fprint_lwfs_oid(
		FILE *fp, 
		const char *name, 
		const char *prefix, 
		const lwfs_oid *oid) 
{
	char ostr[33];

	logger_mutex_lock();
	/* header */
	fprintf(fp, "%s %s[0] = 0x%s\n", prefix, name, lwfs_oid_to_string(*oid, ostr));

	/* footer */
	logger_mutex_unlock();
}
