/*-------------------------------------------------------------------------*/
/**  
 *   @file xdr-tests.h
 *   
 *   @brief Method prototypes for testing XDR. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1073 $
 *   $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
 *
 */

#ifndef _XDR_TESTS_H_
#define _XDR_TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

extern log_level xdr_debug_level; 

/*------------- Method prototypes ---------------*/


#if defined(__STDC__) || defined(__cplusplus)

extern int xdr_test_init(void);

extern int xdr_test_encode(XDR *xdrs); 

extern int xdr_test_decode(XDR *xdrs); 

extern int xdr_test_size(void);

extern int xdr_test_fini(void);

#else /* K&R C */

extern int xdr_test_init() 

extern int xdr_test_encode();

extern int xdr_test_decode();

extern int xdr_test_size();

extern int xdr_test_fini();

#endif /* K&R C */


#ifdef __cplusplus
}
#endif

#endif /* !_MESSAGING_H_ */
