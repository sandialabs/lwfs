/**  @file $RCSfile$
 *   
 *   @brief Insert short description of file here. 
 *   
 *   Insert more detailed description of file here (or remove this line). 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

#include "xdr_types.h"

#include "config.h"

#if STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#include <stdio.h>

const lwfs_oid LWFS_OID_ANY = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
const lwfs_oid LWFS_OID_ZERO = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


/**
 * @brief Release data structures in an lwfs_acl
 *
 * @param acl     @input_type Access-control list.
 */
void lwfs_acl_free(lwfs_uid_array *acl)
{
    xdr_free((xdrproc_t)xdr_lwfs_uid_array, (char *)acl);
    acl->lwfs_uid_array_len=0;
    acl->lwfs_uid_array_val=NULL;
}

int lwfs_is_oid_any(const lwfs_oid oid)
{
	if (!memcmp(oid, LWFS_OID_ANY, sizeof(lwfs_oid))) {
		return(TRUE);
	}
	
	return(FALSE);
}

int lwfs_is_oid_zero(const lwfs_oid oid)
{
	if (!memcmp(oid, LWFS_OID_ZERO, sizeof(lwfs_oid))) {
		return(TRUE);
	}
	
	return(FALSE);
}

void lwfs_set_oid_any(lwfs_oid oid)
{
	memcpy(oid, LWFS_OID_ANY, sizeof(lwfs_oid));
}

void lwfs_clear_oid(lwfs_oid oid)
{
	memcpy(oid, LWFS_OID_ZERO, sizeof(lwfs_oid));
}

char *lwfs_oid_to_string(const lwfs_oid oid, char *ostr)
{
	uint32_t oid_int1, oid_int2, oid_int3, oid_int4;
	const char *oid_char = (const char*)oid;

	memcpy(&oid_int1, &(oid_char[0]), sizeof(uint32_t));
	memcpy(&oid_int2, &(oid_char[4]), sizeof(uint32_t));
	memcpy(&oid_int3, &(oid_char[8]), sizeof(uint32_t));
	memcpy(&oid_int4, &(oid_char[12]), sizeof(uint32_t));
	sprintf(ostr, "%08X%08X%08X%08X", oid_int1, oid_int2, oid_int3, oid_int4);
	
	return(ostr);
}

void lwfs_string_to_oid(const char *ostr, lwfs_oid oid)
{
	char *oid_char = (char*)oid;

	sscanf(ostr, "%08X%08X%08X%08X", 
		(uint32_t *)&(oid_char[0]), (uint32_t *)&(oid_char[4]), (uint32_t *)&(oid_char[8]), (uint32_t *)&(oid_char[12]));

	return;
}
