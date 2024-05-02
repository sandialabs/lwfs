#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/queue.h>

#include "test.h"

#include "sysio.h"
#include "xtio.h"

#include "support/logger/logger.h"

void
_lwfs_premain(void)
{
#if defined(__LIBCATAMOUNT__)
	_catamount_startup();
#elif defined(__linux__)
	_linux_startup();
#else
#warning could not determine system type for _lwfs_premain()
#endif
}

void
_lwfs_postmain(void)
{
#if defined(__LIBCATAMOUNT__)
	_catamount_shutdown();
#elif defined(__linux__)
	_linux_shutdown();
#else
#warning could not determine system type for _lwfs_postmain()
#endif
}

int
_catamount_startup()
{
        int     err;
        char    *arg;

	/* initialize extra drivers */
        err = drv_init_all();
        if (err) {
        	fprintf(stderr, "failed to init drivers\n");
                return err;
        }
        /*
         * catamount has already booted the sysio namespace from sysio_init.
         * we are just going to mount our extra filesystems. 
         */
	arg = getenv("SYSIO_NAMESPACE2");
	if (arg) {
		/*
		 * fprintf(stderr, "arg2=%s\n", arg);
		 */
		err = _sysio_boot("namespace", arg);
	        if (err) {
	       		fprintf(stderr, "failed to boot the SYSIO_NAMESPACE2\n");
                	return err;
        	}
	}

        return 0;
}

int
_linux_startup()
{
	int	err;
	char	*arg;

	err = _sysio_init();
	if (err)
		return err;
	err = drv_init_all();
	if (err)
		return err;
#if SYSIO_TRACING
	/*
	 * tracing
	 */
	arg = getenv("SYSIO_TRACING");
	err = _sysio_boot("trace", arg);
	if (err)
		return err;
#endif
	/*
	 * namespace
	 *
	 * The boot function takes an argument that 
	 * contains an {operation,dev,dir,?,data} 
	 * The data portion gets passed on to the 
	 * mount command for the driver. 
	 */

	arg = getenv("SYSIO_NAMESPACE");
	if (!(arg || (arg = getenv("SYSIO_MANUAL")))) {
		/*
		 * Assume a native mount at root with automounts enabled.
		 */
		arg = "{mnt,dev=\"native:/\",dir=/,fl=2}";
	}
	/*
	 * fprintf(stderr, "arg=%s\n", arg);
	 */
	err = _sysio_boot("namespace", arg);
	if (err) {
       		fprintf(stderr, "failed to boot the SYSIO_NAMESPACE\n");
		return err;
	}
	arg = getenv("SYSIO_NAMESPACE2");
	if (arg) {
		/*
		 * fprintf(stderr, "arg2=%s\n", arg);
		 */
		err = _sysio_boot("namespace", arg);
		if (err) {
	       		fprintf(stderr, "failed to boot the SYSIO_NAMESPACE2\n");
			return err;
		}
	}
#if DEFER_INIT_CWD
	/*
	 * Current working directory.
	 */
	arg = getenv("SYSIO_CWD");
	if (!arg)
		arg = "/";
	err = _sysio_boot("cwd", arg);
	if (err)
		return err;
#endif
	return 0;
}

void
_catamount_shutdown()
{
	/* no op */
	return;
}

void
_linux_shutdown()
{
	_sysio_shutdown();
}
