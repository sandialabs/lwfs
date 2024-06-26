/*
 *    This Cplant(TM) source code is the property of Sandia National
 *    Laboratories.
 *
 *    This Cplant(TM) source code is copyrighted by Sandia National
 *    Laboratories.
 *
 *    The redistribution of this Cplant(TM) source code is subject to the
 *    terms of the GNU Lesser General Public License
 *    (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
 *
 *    Cplant(TM) Copyright 1998-2003 Sandia Corporation. 
 *    Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 *    license for use of this work by or on behalf of the US Government.
 *    Export of this program may require a license from the United States
 *    Government.
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Questions or comments about this library should be sent to:
 *
 * Lee Ward
 * Sandia National Laboratories, New Mexico
 * P.O. Box 5800
 * Albuquerque, NM 87185-1110
 *
 * lee@sandia.gov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/queue.h>
#include <dirent.h>

#if defined(SYSIO_LABEL_NAMES)
#include "sysio.h"
#endif
#include "xtio.h"
#include "mount.h"


/*
 * Test getcwd()
 *
 * Usage: test_cwd [<working-dir>...]
 *
 * Without any path arguments, the program reads from standard-in, dealing with
 * each line as an absolute or relative path until EOF.
 */

static int doit(const char *path);
static void usage(void);

int
main(int argc, char *const argv[])
{
	int	i;
	int	n;

	/*
	 * Parse command line arguments.
	 */
	while ((i = getopt(argc, argv, "")) != -1)
		switch (i) {

		default:
			usage();
		}

	n = argc - optind;

	/*
	 * Try path(s) listed on command-line.
	 */
	while (optind < argc) {
		const char *path;

		path = argv[optind++];
		(void )doit(path);
	}

	/*
	 * If no command-line arguments, read from stdin until EOF.
	 */
	if (!n) {
		int	doflush;
		static char buf[4096];
		size_t	len;
		char	*cp;
		char	c;

		doflush = 0;
		while (fgets(buf, sizeof(buf), stdin) != NULL) {
			len = strlen(buf);
			cp = buf + len - 1;
			c = *cp;
			*cp = '\0';
			if (!doflush)
				doit(buf);
			doflush = c == '\n' ? 0 : 1;
		}
	}

	return 0;
}

static int
doit(const char *path)
{
	char	*buf;

	if (SYSIO_INTERFACE_NAME(chdir)(path) != 0) {
		perror(path);
		return -1;
	}
	buf = SYSIO_INTERFACE_NAME(getcwd)(NULL, 0);
	if (!buf) {
		perror(path);
		return -1;
	}
	(void )printf("%s\n", buf);
	free(buf);
	return 0;
}

static void
usage()
{

	(void )fprintf(stderr,
		       "Usage: test_getcwd "
		       " [<path> ...\n]");

	exit(1);
}
