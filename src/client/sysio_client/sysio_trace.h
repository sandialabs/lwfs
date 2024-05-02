/*-------------------------------------------------------------------------*/
/**  
 *   @file sysio_trace.h
 *   
 *   @author Todd Kordenbrock (thkorde\@sandia.gov)
 *   $Revision: 231 $
 *   $Date: 2005-01-13 00:28:37 -0700 (Thu, 13 Jan 2005) $
 *
 */

#ifndef _SYSIO_TRACE_H_
#define _SYSIO_TRACE_H_

#include "support/logger/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern int sysio_trace_level;

	enum SYSIO_TRACE_IDS {
		TRACE_SYSIO_INO_LOOKUP = 401,
		TRACE_SYSIO_INO_GETATTR,
		TRACE_SYSIO_INO_SETATTR,
		TRACE_SYSIO_INO_FILLDIRENTRIES,
		TRACE_SYSIO_INO_MKDIR,
		TRACE_SYSIO_INO_RMDIR,
		TRACE_SYSIO_INO_SYMLINK,
		TRACE_SYSIO_INO_READLINK,
		TRACE_SYSIO_INO_OPEN,
		TRACE_SYSIO_INO_CLOSE,
		TRACE_SYSIO_INO_LINK,
		TRACE_SYSIO_INO_UNLINK,
		TRACE_SYSIO_INO_RENAME,
		TRACE_SYSIO_INO_READ,
		TRACE_SYSIO_INO_WRITE,
		TRACE_SYSIO_INO_POS,
		TRACE_SYSIO_INO_IODONE,
		TRACE_SYSIO_INO_FCNTL,
		TRACE_SYSIO_INO_SYNC,
		TRACE_SYSIO_INO_DATASYNC,
		TRACE_SYSIO_INO_IOCTL,
		TRACE_SYSIO_INO_MKNOD,
		TRACE_SYSIO_INO_STATVFS,
		TRACE_SYSIO_INO_GONE,
		TRACE_SYSIO_FS_MOUNT,
		TRACE_SYSIO_FS_GONE,
	};

#if defined(__STDC__) || defined(__cplusplus)

#else /* K&R C */

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif 
