/**
 * @file perms.h
 * 
 * @author Ron Oldfield (raoldfi\@sandia.gov)
 * $Revision: 474 $
 * $Date: 2005-11-03 16:04:16 -0700 (Thu, 03 Nov 2005) $
 */

#include "common/types/types.h"
#include <sys/time.h>

#ifndef _PERMS_H_
#define _PERMS_H_


/* the maximum lenth of the object array */
#define MAX_LEN 100

#ifdef __cplusplus
extern "C" {
#endif

	int get_perms(
			const lwfs_service *authr_svc, 
			const lwfs_cred *cred,
			lwfs_cid cid, 
			const lwfs_opcode opcodes,
			lwfs_cap *caps);

#ifdef __cplusplus
}
#endif

#endif
