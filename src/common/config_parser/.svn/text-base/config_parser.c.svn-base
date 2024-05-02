
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "support/ezxml/ezxml.h"
#include "support/logger/logger.h"

#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "config_parser.h"


/* debug level for the configuration parser */
log_level config_debug_level = LOG_UNDEFINED; 


/* The config parser parses an LWFS config file (xml)
 * and returns the lwfs_config data structure.
 */
static int 
parse_service(ezxml_t node, lwfs_remote_pid *id)
{
    int rc = LWFS_OK;
    const char *attr; 

    /* get the nid attribute */
    attr = ezxml_attr(node, "nid");
    id->nid = (lwfs_pid)atoll(attr); 

    /* get the pid attribute */
    attr = ezxml_attr(node, "pid");
    id->pid = (lwfs_pid)atoll(attr);

    log_debug(config_debug_level, "service (nid=%lld, pid=%llu)\n",
	    (long long)id->nid, (long long)id->pid);

    /* get the service */
    /*
    if (svc != NULL) {
	rc = lwfs_get_service(id, svc);
	if (rc != LWFS_OK) {
	    log_error(config_debug_level, "getting service "
		    "description: %s", lwfs_err_str(rc));
	}
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
	struct lwfs_config *config)
{
    int rc = LWFS_OK;
    ezxml_t server; 

    server = ezxml_child(authr, "server-id");
    rc = parse_service(server, &config->authr_id); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"error parsing authr ID: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}

static int 
parse_naming(
	ezxml_t naming, 
	struct lwfs_config *config) 
{
    int rc = LWFS_OK; 
    ezxml_t service, namespace_name;

    service = ezxml_child(naming, "server-id");
    rc = parse_service(service, &config->naming_id);
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"error parsing naming svc: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    namespace_name = ezxml_child(naming, "namespace");
    rc = parse_namespace(namespace_name, config->namespace_name);
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"error parsing namespace name: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


static int 
parse_serverlist(
	ezxml_t list, 
	struct lwfs_config *config) 
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

    config->ss_num_servers = count; 

    config->ss_server_ids = 
	(lwfs_remote_pid *) calloc(count, sizeof(lwfs_remote_pid));

    if (!config->ss_server_ids) {
	log_error(config_debug_level, 
		"could not allocate storage services");
	return LWFS_ERR_NOSPACE;
    }

    /* initialize count again */
    count = 0;

    /* parse the service list */
    for (service = ezxml_child(list, "server-id"); 
	    service; 
	    service = service->next)
    {
	rc = parse_service(service, 
		&config->ss_server_ids[count++]);
	if (rc != LWFS_OK) {
	    log_error(config_debug_level, "error parsing service: %s", 
		    lwfs_err_str(rc));
	    return rc; 
	}
    }

    return rc;
}

static int 
parse_chunksize(ezxml_t node,  int *chunksize)
{
    int rc = LWFS_OK;
    const char *attr;

    attr = ezxml_attr(node, "default");
    *chunksize = (int)atoll(attr);

    return rc;
}

static int 
parse_fake_io_pattern_list(
	ezxml_t list, 
	struct lwfs_config *config) 
{
    int rc = LWFS_OK;
    ezxml_t pattern; 
    const char* attr;
    int count=0; 

    log_debug(config_debug_level, "entered parse_fake_io_pattern_list");

    /* first count the children */
    for (pattern = ezxml_child(list, "fake-io-pattern"); 
	    pattern; 
	    pattern = pattern->next)
    {
	count++; 
    }

    config->ss_num_fake_io_patterns = count; 

    log_debug(config_debug_level, "config->ss_num_fake_io_patterns == %d", config->ss_num_fake_io_patterns);

    config->ss_fake_io_patterns = 
	(char **) calloc(count, sizeof(char *));
    if (!config->ss_fake_io_patterns) {
	log_error(config_debug_level, 
		"could not allocate fake i/o patterns");
	return LWFS_ERR_NOSPACE;
    }
    
    /* initialize count again */
    count = 0;
    /* parse the service list */
    for (pattern = ezxml_child(list, "fake-io-pattern"); 
	    pattern; 
	    pattern = pattern->next, count++)
    {
	attr = ezxml_attr(pattern, "pattern");
	config->ss_fake_io_patterns[count] = strdup(attr);
        log_debug(config_debug_level, "config->ss_fake_io_patterns[%d] == %s", count, config->ss_fake_io_patterns[count]);
    }

    return rc;
}

static int 
parse_storage(
	ezxml_t node, 
	struct lwfs_config *config) 
{
    int rc = LWFS_OK;
    ezxml_t server_list, chunk_size, fake_io_pattern_list;

    /* server list */
    server_list = ezxml_child(node, "server-list"); 
    rc = parse_serverlist(server_list, config);
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"error parsing serverlist: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    chunk_size = ezxml_child(node, "chunk-size");
    rc = parse_chunksize(chunk_size, &config->ss_chunksize); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"error parsing chunksize: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    fake_io_pattern_list = ezxml_child(node, "fake-io-pattern-list");
    rc = parse_fake_io_pattern_list(fake_io_pattern_list, config); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"error parsing fake i/o pattern list: %s", 
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}



static int 
parse_config(
	ezxml_t node, 
	struct lwfs_config *config) 
{
    int rc;
    ezxml_t authr, naming, storage;

    authr = ezxml_child(node, "authr"); 
    rc = parse_authr(authr, config); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"could not parse authr: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    naming = ezxml_child(node, "naming"); 
    rc = parse_naming(naming, config); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"could not parse naming: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    storage = ezxml_child(node, "storage"); 
    rc = parse_storage(storage, config); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"could not parse naming: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    return rc;
}


/*------------------ EXTERNAL APIs ------------------ */

int
parse_lwfs_config_file(
	const char *docname, 
	struct lwfs_config *lwfs_cfg) 
{
    int rc = LWFS_OK;
    int fd = 0;
    ezxml_t doc, config; 

    log_debug(config_debug_level, "entered parse_config_file");

    if (!docname) {
	rc = LWFS_ERR_NOTFILE;
	return rc;
    }

    fd = open(docname, O_RDONLY, 0);
    if (fd < 0) {
	log_error(config_debug_level, "failed to open config file (%s): %s", docname, strerror(errno));
	return LWFS_ERR;
    }
    doc = ezxml_parse_fd(fd); 
    if (doc == NULL) {
	log_error(config_debug_level, "failed to parse config file (%s)", docname);
	return LWFS_ERR;
    }

    config = ezxml_child(doc, "config");
    rc = parse_config(config, lwfs_cfg); 
    if (rc != LWFS_OK) {
	log_error(config_debug_level, 
		"could not parse config file: %s",
		lwfs_err_str(rc));
	return rc; 
    }

    ezxml_free(doc);

    log_debug(config_debug_level, "finished parse_config_file");

    return rc;
}


void lwfs_config_free(struct lwfs_config *lwfs_cfg)
{
    /* release the space allocated for the ss_server_ids */
    free(lwfs_cfg->ss_server_ids); 
}


