
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "fs_lwfs.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "client/rpc_client/rpc_client.h"
#include "support/logger/logger.h"

extern log_level sysio_debug_level; 

static int 
parseServerID(xmlDocPtr doc, xmlNodePtr cur, lwfs_service *svc)
{
	int rc = LWFS_OK;
	xmlChar *str; 
	lwfs_remote_pid id; 

	str = xmlGetProp(cur,(const xmlChar*)"nid");
	id.nid = (lwfs_pid)atoll((const char *)str);
	xmlFree(str); 

	str = xmlGetProp(cur,(const xmlChar*)"pid");
	id.pid = (lwfs_nid)atoll((const char *)str);
	xmlFree(str); 

	/* get the service */
	rc = lwfs_get_service(id, svc);
	if (rc != LWFS_OK) {
		log_error(sysio_debug_level, "getting service "
				"description: %s", lwfs_err_str(rc));
	}

	return rc;
}


static int 
parseNamespace(xmlDocPtr doc, xmlNodePtr cur, char *name)
{
	int rc = LWFS_OK;
	xmlChar *str; 

	str = xmlGetProp(cur,(const xmlChar*)"name");
	strcpy(name, (const char *)str);
	xmlFree(str); 

	return rc;
}


static int 
parseAuthr(xmlDocPtr doc, xmlNodePtr cur, lwfs_filesystem *lwfs_fs) {
	int rc = LWFS_OK;
	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {

		/* options for the authorization service */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"server-id"))) {
			rc = parseServerID(doc, cur, &lwfs_fs->authr_svc);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "error parsing server ID: %s", 
						lwfs_err_str(rc));
				return rc; 
			}
		}

		cur = cur->next;
	}
	return rc;
}

static int 
parseNaming(
		xmlDocPtr doc, 
		xmlNodePtr cur, 
		lwfs_filesystem *lwfs_fs) 
{
	int rc = LWFS_OK; 
	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {

		/* options for the naming service */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"server-id"))) {
			rc = parseServerID(doc, cur, &lwfs_fs->naming_svc);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "error parsing server ID: %s", 
						lwfs_err_str(rc));
				return rc; 
			}
		}

		if ((!xmlStrcmp(cur->name, (const xmlChar *)"namespace"))) {
			rc = parseNamespace(doc, cur, lwfs_fs->namespace.name);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "error parsing namespace name: %s", 
						lwfs_err_str(rc));
				return rc; 
			}
		}

		cur = cur->next;
	}
	
	return rc;
}


static int 
parseServerList(
		xmlDocPtr doc, 
		xmlNodePtr cur, 
		lwfs_filesystem *lwfs_fs) 
{
	int rc = LWFS_OK;
	xmlChar *key;
	xmlNodePtr head = cur->xmlChildrenNode; 

	cur = head;
	int count = 0;

	/* count the number of children */
	while (cur != NULL) {
		/* options for the storage service */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"server-id"))) {
			count++;
		}
		cur = cur->next;
	}

	/* allocate the service descriptions */
	lwfs_fs->num_servers = count;
	lwfs_fs->storage_svc = (lwfs_service *)calloc(count, sizeof(lwfs_service));
	if (!lwfs_fs->storage_svc) {
		log_error(sysio_debug_level, "could not allocate storage services");
		return LWFS_ERR_NOSPACE;
	}

	cur = head;
	count = 0;
	while (cur != NULL) {
		/* options for the storage service */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"server-id"))) {
			rc = parseServerID(doc, cur, &lwfs_fs->storage_svc[count++]);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "error parsing server ID: %s", 
						lwfs_err_str(rc));
				return rc; 
			}
		}
		cur = cur->next;
	}
	return rc;
}

static int 
parseChunkSize(xmlDocPtr doc, xmlNodePtr cur, lwfs_filesystem *lwfs_fs)
{
	int rc = LWFS_OK;
	xmlChar *str; 

	str = xmlGetProp(cur,(const xmlChar*)"default");
	lwfs_fs->default_chunk_size = (int)atoll((const char *)str);
	xmlFree(str); 

	return rc;
}

static int 
parseStorage(
		xmlDocPtr doc, 
		xmlNodePtr cur, 
		lwfs_filesystem *lwfs_fs) 
{
	int rc = LWFS_OK;
	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {

		/* options for the storage service */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"server-list"))) {
			parseServerList(doc, cur, lwfs_fs);
		}
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"chunk-size"))) {
			parseChunkSize(doc, cur, lwfs_fs);
		}

		cur = cur->next;
	}

	return rc;
}



static int 
parseConfig(
		xmlDocPtr doc, 
		xmlNodePtr cur, 
		lwfs_filesystem *lwfs_fs) 
{
	int rc=LWFS_OK;
	xmlChar *key;
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {

		/* Parse the authr options */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"authr"))) {
			rc = parseAuthr(doc, cur, lwfs_fs);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "could not parse: %s",
						lwfs_err_str(rc));
				return rc; 
			}
		}

		/* Parse the naming options */
		else if ((!xmlStrcmp(cur->name, (const xmlChar *)"naming"))) {
			rc = parseNaming(doc, cur, lwfs_fs);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "could not parse: %s",
						lwfs_err_str(rc));
				return rc; 
			}
		}

		/* Parse the storage options */
		else if ((!xmlStrcmp(cur->name, (const xmlChar *)"storage"))) {
			rc = parseStorage(doc, cur, lwfs_fs);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "could not parse: %s",
						lwfs_err_str(rc));
				return rc; 
			}
		}

		cur = cur->next;
	}
	return rc;
}


int
parse_config_file(
		const char *docname, 
		lwfs_filesystem *lwfs_fs) 
{
	int rc = LWFS_OK;
	xmlDocPtr doc;
	xmlNodePtr cur;

	log_debug(sysio_debug_level, "entered parse_config_file");

	doc = xmlParseFile(docname);

	if (doc == NULL ) {
		log_error(sysio_debug_level,"Document not parsed successfully.");
		return LWFS_ERR;
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		log_error(sysio_debug_level,"empty document");
		xmlFreeDoc(doc);
		return LWFS_ERR;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "lwfs")) {
		log_error(sysio_debug_level,
				"document of the wrong type, root node != lwfs");
		xmlFreeDoc(doc);
		return LWFS_ERR;
	}

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"config"))){
			rc = parseConfig (doc, cur, lwfs_fs);
			if (rc != LWFS_OK) {
				log_error(sysio_debug_level, "could not parse "
						"configuration file: %s", lwfs_err_str(rc));
				return rc; 
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);

	log_debug(sysio_debug_level, "finished parse_config_file");

	return rc;

}

/*
int
main(int argc, char **argv) {

        char *docname;
                
        if (argc <= 1) {
                printf("Usage: %s docname\n", argv[0]);
                return(0);
        }

        docname = argv[1];
        parseDoc (docname);

        return (1);
}
*/
