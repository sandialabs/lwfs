#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/queue.h>

#if defined(SYSIO_LABEL_NAMES)
#include "sysio.h"
#endif
#include "xtio.h"
#include "test.h"

void sysio_init(void) __attribute__ ((constructor));
void sysio_fini(void) __attribute__ ((destructor));

void sysio_init(void)
{
    int err; 

    printf("initializing libsysio\n");
    err = _test_sysio_startup(); 
    if (err) {
	errno = -err; 
	perror("sysio startup");
	exit(1);
    }
}

void sysio_fini(void)
{
    printf("finalizing libsysio\n");
    _test_sysio_shutdown();
}

