/*
 * This file automatically generated by gendrvdata.sh. All changes
 * will be lost!
 */

#include <stdlib.h>

#include "test.h"

#if !defined(__LIBCATAMOUNT__)
extern int _sysio_native_init(void); 
extern int _sysio_incore_init(void);
#endif 
extern int _sysio_lwfs_init(void);

int (*drvinits[])(void) = {
	_sysio_lwfs_init,
#if !defined(__LIBCATAMOUNT__)
	_sysio_native_init, 
	_sysio_incore_init,
#endif
	NULL
};