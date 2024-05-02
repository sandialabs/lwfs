/*-------------------------------------------------------------------------*/
/**  @file cap.h
 *   
 *   @brief Prototypes for the LWFS capability functions.
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

#ifndef _LWFS_SECURITY_H_
#define _LWFS_SECURITY_H_

#include "lwfs/types.h"

#ifdef __cplusplus
extern "C" {
#endif 

#if defined(__STDC__) || defined(__cplusplus)

extern int generate_cap_key(
		lwfs_key *); 

extern int generate_cap(
		const lwfs_key *,
		const lwfs_cap_data *,
		lwfs_cap *);

extern int verify_cap(
		const lwfs_key *,
		const lwfs_cap *);

#else /* K&R C */

extern int generate_cap_key();
extern int generate_cap();
extern int verify_cap();

#endif /* K&R C */


#ifdef __cplusplus
}	
#endif 


#endif  /* _LWFS_SECURITY_H_ */
