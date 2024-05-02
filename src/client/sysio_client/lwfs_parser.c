
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "support/ezxml/ezxml.h"

#include "fs_lwfs.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "client/rpc_client/rpc_client.h"
#include "support/logger/logger.h"

log_level lwfs_debug_level; 
int dryrun = 1; 

static int 
parse_service(ezxml_t node, lwfs_service *svc)
{
    int rc = LWFS_OK;
    lwfs_remote_pid id; 
    const char *attr; 

    /* get the nid attribute */
    attr = ezxml_attr(node, "nid");
    id.nid = (lwfs_pid)atoll(attr); 

    /* get the pid attribute */
    attr = ezxml_attr(node, "nid");
    id.nid = (lwfs_pid)atoll(attr);

    fprintf(stdout, "service (nid=%lld, pid=%llu)\n",
	    (long long)id.nid, (long long)id.pid);

    /* get the service */
    /*
    rc = lwfs_get_service(id, svc);
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, "getting service "
		"description: %s", lwfs_err_str(rc));
    }
    */

    return rc;
}


static int 
parse_namespace(ezxml_t node, char *name)
{
	int rc = LWFS_OK;
	const char* attr; 

	attr = ezxml_attr(node, "name");
	strcpy(name, attr);

	return rc;
}


static int 
parse_authr(
	ezxml_t authr, 
	lwfs_filesystem *lwfs_fs) 
{
    int rc = LWFS_OK;
    ezxml_t server; 

    server = ezxml_child(authr, "server-id");
    rc = parse_service(server, &lwfs_fs->authr_svc); 
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"error parsing server ID: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}

static int 
parse_naming(
	ezxml_t naming, 
	lwfs_filesystem *lwfs_fs) 
{
    int rc = LWFS_OK; 
    ezxml_t service, namespace;

    service = ezxml_child(naming, "server-id");
    rc = parse_service(service, &lwfs_fs->naming_svc);
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"error parsing naming svc: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    namespace = ezxml_child(naming, "namespace");
    rc = parse_namespace(namespace, lwfs_fs->namespace.name);
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"error parsing namespace name: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


static int 
parse_serverlist(
	ezxml_t list, 
	lwfs_filesystem *lwfs_fs) 
{
    int rc = LWFS_OK;
    ezxml_t service; 
    int count=0; 

    /* first count the children */
    for (service = ezxml_child(list, "server-id"); 
	    service; 
	    service = service->next)
    {
	count++; 
    }

    lwfs_fs->num_servers = count; 

    /* first count the children */
    for (service = ezxml_child(list, "server-id"); 
	    service; 
	    service = service->next)
    {
	count++; 
    }

    lwfs_fs->storage_svc = (lwfs_service *) calloc(count, sizeof(lwfs_service));
    if (!lwfs_fs->storage_svc) {
	log_error(lwfs_debug_level, 
		"could not allocate storage services");
	return LWFS_ERR_NOSPACE;
    }

    /* parse the service list */
    for (service = ezxml_child(list, "server-id"); 
	    service; 
	    service = service->next)
    {
	rc = parse_service(service, &lwfs_fs->storage_svc[count++]);
	if (rc != LWFS_OK) {
	    log_error(lwfs_debug_level, "error parsing service: %s", 
		    lwfs_err_str(rc));
	    return rc; 
	}
    }

    return rc;
}

static int 
parse_chunksize(ezxml_t node,  lwfs_filesystem *lwfs_fs)
{
    int rc = LWFS_OK;
    const char *attr;

    attr = ezxml_attr(node, "default");
    lwfs_fs->default_chunk_size = (int)atoll(attr);

    return rc;
}

static int 
parse_storage(
	ezxml_t node, 
	lwfs_filesystem *lwfs_fs) 
{
    int rc = LWFS_OK;
    ezxml_t server_list, chunk_size;

    /* server list */
    server_list = ezxml_child(node, "server-list"); 
    rc = parse_serverlist(server_list, lwfs_fs);
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"error parsing serverlist: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    chunk_size = ezxml_child(node, "chunk-size");
    rc = parse_chunksize(chunk_size, lwfs_fs); 
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"error parsing chunksize: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}



static int 
parse_config(
	ezxml_t node, 
	lwfs_filesystem *lwfs_fs) 
{
    int rc;
    ezxml_t authr, naming, storage;

    authr = ezxml_child(node, "authr"); 
    rc = parse_authr(authr, lwfs_fs); 
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"could not parse authr: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    naming = ezxml_child(node, "naming"); 
    rc = parse_naming(naming, lwfs_fs); 
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"could not parse naming: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    storage = ezxml_child(node, "storage"); 
    rc = parse_storage(storage, lwfs_fs); 
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"could not parse naming: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


int
parse_config_file(
	const char *docname, 
	lwfs_filesystem *lwfs_fs) 
{
    int rc = LWFS_OK;
    ezxml_t doc, config; 

    log_debug(lwfs_debug_level, "entered parse_config_file");

    doc = ezxml_parse_file(docname); 

    config = ezxml_child(doc, "config");
    rc = parse_config(config, lwfs_fs); 
    if (rc != LWFS_OK) {
	log_error(lwfs_debug_level, 
		"could not parse config file: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    ezxml_free(doc);

    log_debug(lwfs_debug_level, "finished parse_config_file");

    return rc;
}

int
main(int argc, char **argv) {

        char *docname;
	lwfs_filesystem lwfs_fs; 

	memset(&lwfs_fs, 0, sizeof(lwfs_filesystem));
                
        if (argc <= 1) {
                printf("Usage: %s docname\n", argv[0]);
                return(0);
        }

        docname = argv[1];
        parse_config_file (docname, &lwfs_fs);

        return (1);
}
