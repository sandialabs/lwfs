#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/queue.h>

#include "lwfs_sysio.h"

#include "sysio.h"
#include "xtio.h"

#include "support/logger/logger.h"

#define IGNORE_WHITE	    " \t\r\n"
#define COMMENT_INTRO '#'

void _lwfs_premain (void) __attribute__ ((constructor));
void _lwfs_postmain (void) __attribute__ ((destructor));


void
_lwfs_premain(void)
{
#if defined(__LIBCATAMOUNT__)
	/*fprintf(stderr,"starting lwfs\n");*/
	_catamount_startup();
#elif defined(__linux__)
	/*fprintf(stdout, "linux premain()\n");*/
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
	/*fprintf(stdout, "linux shutdown()\n");*/
	_linux_shutdown();
#else
#warning could not determine system type for _lwfs_postmain()
#endif
}

const char *
get_token(const char *buf,
	int accepts,
	const char *delim,
	const char *ignore,
	char *tbuf)
{
    char	c;
    int	escape, quote;

    /* 
     * Find the first occurance of delim, recording how many
     * characters lead up to it.  Ignore indicated characters.
     */
    escape = quote = 0;
    while ((c = *buf) != '\0') {
	buf++;
	if (!escape) {
	    if (c == '\\') {
		escape = 1;
		continue;
	    }
	    if (c == '\"') {
		quote ^= 1;
		continue;
	    }
	    if (!quote) {
		if (strchr(delim, c) != NULL) {
		    accepts = 1;
		    break;
		}
		if (strchr(ignore, c) != NULL)
		    continue;
	    }
	} else
	    escape = 0;
	*tbuf++ = c;
    }
    if (!accepts)
	return NULL;
    *tbuf = '\0';			/* NUL term */
    return buf;
}



static int
parse_arg(const char *arg)
{
    char	c, *tok;
    ssize_t len;
    int err;
    unsigned count;
    /*
     * Allocate token buffer.
     */
    len = strlen(arg);
    tok = malloc(len ? len : 1);
    if (!tok)
	return -ENOMEM;
    err = 0;
    count = 0;
    while (1) {
	/*
	 * Discard leading white space.
	 */
	while ((c = *arg) != '\0' &&
		!(c == '{' || strchr(IGNORE_WHITE, c) == NULL))
	    arg++;
	if (COMMENT_INTRO == c) {
	    /*
	     * Discard comment.
	     */
	    while (*arg && (*arg != '\n')) {
		++arg;
	    }
	    continue;
	}

	if (c == '\0')
	    break;
	if (c != '{') {
	    err = -EINVAL;
	    break;
	}
	/*
	 * Get the command.
	 */
	*tok = '\0';
	arg =
	    (char *)get_token(arg + 1,
				     0,
				     "}",
				     IGNORE_WHITE,
				     tok);
	if (!arg) {
	    err = -EINVAL;
	    break;
	}
	count++;
	/*
	 * Perform.
	 */
	fprintf(stderr, "do_command(%s)\n",tok);
    }
#if SYSIO_TRACING
    if (err)
	_sysio_cprintf("+NS init+ failed at expr %u (last = %s): %s\n", 
		count,
		tok && *tok ? tok : "NULL",
		strerror(-err));
#endif
    free(tok);
    return err;
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
		fprintf(stderr, "arg=%s\n", arg);
		parse_arg(arg);
		err = _sysio_boot("namespace", arg);
	        if (err) {
	       		fprintf(stderr, "failed to boot the SYSIO_NAMESPACE2: %d\n",err);
                	return err;
        	}
	}

	arg = getenv("LWFS_INIT_DIR");
	if (arg) {
		fprintf(stderr, "changing to dir(%s)\n", arg);
		err = chdir(arg);
	        if (err) {
	       		fprintf(stderr, "failed to chdir(%s): %d\n", arg, err);
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

	arg = getenv("LWFS_INIT_DIR");
	if (arg) {
		fprintf(stderr, "changing to dir(%s)\n", arg);
		err = chdir(arg);
	        if (err) {
	       		fprintf(stderr, "failed to chdir(%s): %d\n", arg, err);
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
