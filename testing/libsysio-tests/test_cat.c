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

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/queue.h>

#if defined(SYSIO_LABEL_NAMES)
#include "sysio.h"
#endif
#include "xtio.h"

/*
 * Copy one file to another.
 *
 * Usage: test_copy [-o] <src> <dest>
 *
 * Destination will not be overwritten if it already exist.
 */

static int overwrite = 0;				/* over-write? */

void	usage(void);
int	cat_file(const char *spath);

int
main(int argc, char * const argv[])
{
	int	i;
	int	err;
	const char *spath; 

	/*
	 * Parse command-line args.
	 */
	while ((i = getopt(argc,
			   argv,
			   "o"
			   )) != -1)
		switch (i) {

		case 'o':
			overwrite = 1;
			break;
		default:
			usage();
		}

	if (!(argc - optind))
		usage();

	/*
	 * Source
	 */
	spath = argv[optind++];

	err = cat_file(spath);

	return err;
}

void
usage()
{

	(void )fprintf(stderr,
		       "Usage: test_cat "
		       " source \n");
	exit(1);
}

int
open_file(const char *path, int flags, mode_t mode)
{
	int	fd;

	/*fd = SYSIO_INTERFACE_NAME(open)(path, flags, mode);*/
	fd = open(path, flags, mode);
	if (fd < 0)
		perror(path);

	return fd;
}

int
cat_file(const char *spath)
{
	int	sfd, dfd;
	int	flags;
	int	rtn;
	struct stat stat;
	char	*buf;
	size_t	bufsiz;
	ssize_t	cc, wcc;

	sfd = dfd = -1;
	rtn = -1;
	buf = NULL;

	sfd = open_file(spath, O_RDONLY, 0);
	if (sfd < 0)
		goto out;
	flags = O_CREAT|O_WRONLY;
	if (!overwrite)
		flags |= O_EXCL;

	/* dfd points to stdout (fd=1) */
	dfd = 1;

	/*rtn = SYSIO_INTERFACE_NAME(fstat)(dfd, &stat);*/
	rtn = fstat(dfd, &stat);
	if (rtn != 0) {
		goto out;
	}
	bufsiz = stat.st_blksize;
	if (bufsiz < (64 * 1024))
		bufsiz =
		    (((64 * 1024) / stat.st_blksize - 1) + 1) * (64 * 1024);
	buf = malloc(bufsiz);
	if (!buf) {
		goto out;
	}
	while (1) {
		/*cc = SYSIO_INTERFACE_NAME(read)(sfd, buf, bufsiz);*/
		cc = read(sfd, buf, bufsiz);
		if (cc <= 0) break;
		/*if ((wcc = SYSIO_INTERFACE_NAME(write)(dfd, buf, cc)) != cc) {*/
		if ((wcc = write(dfd, buf, cc)) != cc) {
			if (wcc < 0) {
				break;
			}
			(void )fprintf(stderr,
				       ": short write (%u/%u)\n",
				       (unsigned )wcc,
				       (unsigned )cc);
			break;
		}
	}
	if (cc < 0) {
		perror(spath);
		rtn = -1;
	}

out:
	if (buf)
		free(buf);
	if (sfd >= 0 && close(sfd) != 0)
		perror(spath);

	return rtn;
}
