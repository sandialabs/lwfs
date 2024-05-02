dnl ----------------- LWFS config file ------------------
dnl   This file contains the autoconf configuration script
dnl   used to build the LWFS client and server libraries and
dnl   the LWFS core services.  
dnl ----------------------------------------------------

AC_INIT(configure.ac)
AC_CONFIG_AUX_DIR(config)
AC_REVISION($Revision: 1565 $)

dnl AC_CONFIG_HEADERS([src/lwfs_config.h:defs.pre])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE(lwfs, 0.1)


dnl  ---------------- CONFIGURE ARGUMENTS ----------------------------

ac_server_disabled=no
ac_server_ok=yes
ac_client_ok=yes
ac_testing_ok=yes
ac_docs_ok=yes


dnl Build the client? 
AC_ARG_ENABLE(client, 
	[  --enable-client     Configure and build LWFS client libraries], 
	[case "${enableval}" in 
	    yes) ac_client_ok=yes ;; 
	    no)  ac_client_ok=no ;; 
	    *) AC_MSG_ERROR(bad value ${enableval} for --enable-client) ;;
	 esac],
	[ac_client_ok=yes])

dnl Build the server? 
AC_ARG_ENABLE(server, 
	[  --enable-server     Configure and build LWFS servers], 
	[case "${enableval}" in 
		yes) ac_server_ok=yes ;; 
		no)  ac_server_ok=no ;; 
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-server) ;;
	esac],
	[ac_server_ok=yes])

if test x$ac_server_ok = xno; then
    ac_server_disabled=yes;
    AC_MSG_WARN(server disabled)
else
    ac_server_disabled=no;
    AC_MSG_WARN(server enabled)
fi

dnl Enable testing
AC_ARG_ENABLE(testing, 
	[  --enable-testing     Configure and enable testing (default=enable)], 
	[case "${enableval}" in 
		yes) ac_testing_ok=yes ;; 
		no)  ac_testing_ok=no ;; 
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-testing) ;;
	esac],
	[ac_testing_ok=yes])


dnl Build documents
AC_ARG_ENABLE(docs, 
	[  --enable-docs     Configure and build LWFS documents (default=disable)], 
	[case "${enableval}" in 
		yes) ac_docs_ok=yes ;; 
		no)  ac_docs_ok=no ;; 
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-docs) ;;
     esac],[ac_docs_ok=no])


dnl  ---------------- PATHS TO SOFTWARE CHECKS  ----------------------------


dnl AC_ARG_WITH(krb5, [  --with-krb5=<dir>	Location of Kerberos v5 distribution],
dnl 	    [LDFLAGS="${LDFLAGS} -L$withval/lib"; CPPFLAGS="${CPPFLAGS} -I$withval/include";])

dnl 
dnl 

dnl  ------- this is required to allow the UINT*_C macros in C++ -----
CXXFLAGS="${CXXFLAGS} -D__STDC_CONSTANT_MACROS"


dnl  ------- Various required flags for Mac OSX -------
dnl    way up here because AC_PROG_LIBTOOL is dependent
if test `uname -s` = "Darwin"; then
  AC_DEFINE(BROKEN_DARWIN_RPCGEN, [1], [Darwin's rpcgen is broken in several painful ways])
  AM_CONDITIONAL(NEED_DARWIN_SINGLE_MODULE, true)
  MACOSX_DEPLOYMENT_TARGET=10.4
fi

dnl  ---------------- COMPILER CHECKS  ----------------------------

AC_LANG_C
AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_HEADER(string.h, [], [])
AC_CHECK_HEADER(strings.h, [], [])
AC_CHECK_HEADERS([malloc.h])
AX_CREATE_STDINT_H(src/common/types/lwfs_int.h)

dnl  ---------------- HEADER CHECKS ------------------------
AC_HEADER_STDC
AC_CHECK_HEADERS([argp.h])


dnl Find working version of gengetopt
AC_PROG_GENGETOPT

dnl ------ TEST LIBS REQUIRED BY SERVERS ----

if test x$ac_server_disabled = xyes; then
    ac_server_ok=no
    AC_MSG_WARN(servers disabled)
    AM_CONDITIONAL(HAVE_BDB,false)
    AM_CONDITIONAL(HAVE_PTHREAD,false)
    AM_CONDITIONAL(HAVE_PABLO,false)
    AM_CONDITIONAL(HAVE_DISKSIM,false)
    AM_CONDITIONAL(HAVE_EBOFS,false)
    AM_CONDITIONAL(HAVE_OPENSSL,false)
else

AX_BERKELEY_DB([4.2],
	[AM_CONDITIONAL(HAVE_BDB,true)],
	[AM_CONDITIONAL(HAVE_BDB,false)])


dnl Find working version of berkeley  db
dnl AX_PATH_BDB([4.2],
dnl 	[AM_CONDITIONAL(HAVE_BDB,true)],
dnl 	[AC_MSG_WARN("missing berkeley db ... LWFS servers disabled");
dnl 	ac_server_ok=no; AM_CONDITIONAL(HAVE_BDB,false)])

dnl Find working version of ssl and crypto library (defines HAVE_SSL)
AC_OPENSSL([],
	[AC_MSG_WARN("missing openssl ... LWFS servers disabled");
	ac_server_ok=no])

dnl -- Pthreads (used by server-side RPC stubs)
ACX_PTHREAD([AM_CONDITIONAL(HAVE_PTHREAD,true)],
	[AC_MSG_WARN("missing pthreads library ... LWFS servers disabled");
	ac_server_ok=no;
	AM_CONDITIONAL(HAVE_PTHREAD,false)])

dnl -- Pablo required for tracing
AC_PABLO([], [AC_MSG_WARN("missing Pablo ... tracing disabled")])

dnl -- Check for libebofs
AC_EBOFS([], [AC_MSG_WARN("missing ebofs")])


dnl -- Check for disksim
AC_DISKSIM([], [AC_MSG_WARN("missing disksim ... simio disabled")])

fi


dnl ------ TEST LIBS REQUIRED BY SERVERS AND CLIENTS ----


dnl -- Portals required for networking
AC_PORTALS([], 
	[ac_server_ok=no; ac_client_ok=no;
	AC_MSG_WARN("missing Portals ... LWFS disabled")])

dnl -- real-time library required for asynchronous I/O library
dnl -- (defines HAVE_RT, RT_{CPPFLAGS,CFLAGS,LDFLAGS,LIBS})
AC_RT([], [AC_MSG_WARN("missing rtlib ... aio disabled")])

dnl -- Check for the O_DSYNC flag for use with aio_fsync()
	AH_TEMPLATE([HAVE_FLAG_O_DSYNC], [have the O_DSYNC flag])
	AC_MSG_CHECKING(if the O_DSYNC flag is supported for aio_fsync())
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		#include <stdlib.h>
		#include <stdio.h>
		#include <time.h>
		#include <sys/types.h>
		#include <sys/stat.h>
		#include <unistd.h>
		#include <fcntl.h>
		#include <errno.h>
		#include <pthread.h>
		#include <signal.h>
		#include <aio.h>
		]],[[
		struct aiocb tt;
		aio_fsync(O_DSYNC,&tt);
		]])],
		[ AC_DEFINE(HAVE_FLAG_O_DSYNC) 
		AC_MSG_RESULT([yes])
		],[
		AC_MSG_RESULT([no, use O_SYNC instead])
		]
	)


dnl -- Check for Lee's libsysio
AC_LIBSYSIO([], [AC_MSG_WARN("missing libsysio")])


dnl -- Check's required for the uuid support library
UUID_CHECK_ALL



dnl -- MacOS's RFO (Real F'ing Old) XDR implementation doesn't have xdr_sizeof.
dnl -- Find out if we need to compile the one we bring along.
AC_CHECK_FUNCS([xdr_sizeof], 
		[AM_CONDITIONAL(NEED_LWFS_XDR_SIZEOF, false)], 
		[AM_CONDITIONAL(NEED_LWFS_XDR_SIZEOF, true)]
		)

dnl  ---------------- FUNCTION CHECKS  -----------------------
AC_CHECK_FUNCS(clock_gettime gettimeofday fdatasync)

dnl  ---------------- TYPE AND VARIABLE CHECKS  ----------------------------
AC_CHECK_TYPES([uint32_t, uint64_t])
dnl AC_CHECK_SIZEOF(void *, 8)
AC_CHECK_TYPES(sigval_t,,,[#include <signal.h>])

dnl  ---------------- FINAL TESTS  ----------------------------


dnl ----------------- CONFIGURE SUBDIRS --------------------------

AM_CONDITIONAL(BUILD_SERVER, test x$ac_server_ok = xyes)
AM_CONDITIONAL(BUILD_CLIENT, test x$ac_client_ok = xyes)
AM_CONDITIONAL(BUILD_DOCS,   test x$ac_docs_ok = xyes)
AM_CONDITIONAL(BUILD_TESTS,  test x$ac_testing_ok = xyes)

dnl AC_CONFIG_SUBDIRS(doc)


test x$ac_docs_ok = xyes && \
	AC_CONFIG_SUBDIRS(doc)

if test x$ac_server_ok = xyes; then
 	ac_configure_args="$ac_configure_args --enable-testing-servers"
fi

test x$ac_testing_ok = xyes && \
 	AC_CONFIG_SUBDIRS(testing)


AC_CONFIG_FILES([Makefile
		src/Makefile
		src/support/Makefile
		src/support/hashtable/Makefile
		src/support/list/Makefile
		src/support/logger/Makefile
		src/support/signal/Makefile
		src/support/stats/Makefile
		src/support/threadpool/Makefile
		src/support/timer/Makefile
		src/support/trace/Makefile
		src/support/ezxml/Makefile
		src/support/sysmon/Makefile
		src/support/ptl_uuid/Makefile
		src/common/Makefile
		src/common/types/Makefile
		src/common/config_parser/Makefile
		src/common/rpc_common/Makefile
		src/common/authr_common/Makefile
		src/common/storage_common/Makefile
		src/common/naming_common/Makefile
		src/server/Makefile
		src/server/rpc_server/Makefile
		src/server/authr_server/Makefile
		src/server/storage_server/Makefile
		src/server/naming_server/Makefile
		src/client/Makefile
		src/client/rpc_client/Makefile
		src/client/authr_client/Makefile
		src/client/storage_client/Makefile
		src/client/naming_client/Makefile
		src/client/txn/Makefile
		src/client/sysio_client/Makefile
                src/progs/Makefile 
                src/progs/lwfs-kill/Makefile 
                src/progs/lwfs-ping/Makefile 
                src/progs/lwfs-nid/Makefile ])

# Add this later. 
#src/progs/lwfs-ops/Makefile

AC_OUTPUT


echo
echo "-----------------------------------------
$PACKAGE configured as follows:
-----------------------------------------"

echo
echo "  - General Compile FLAGS";
echo "      - CPPFLAGS = $CPPFLAGS";
echo "      - CFLAGS = $CFLAGS";
echo "      - LDLAGS = $LDFLAGS";
echo "      - LIBS = $LIBS";
echo
if test -z "${HAVE_PORTALS_TRUE}"; then
	echo "  - PORTALS";
	echo "      - PORTALS_CFLAGS = $PORTALS_CFLAGS";
	echo "      - PORTALS_CPPFLAGS = $PORTALS_CPPFLAGS";
	echo "      - PORTALS_LDFLAGS = $PORTALS_LDFLAGS";
	echo "      - PORTALS_LIBS = $PORTALS_LIBS";
fi
echo
if test -z "${HAVE_PTHREAD_TRUE}"; then
	echo "  - PTHREADS";
	echo "      - PTHREAD_CC = $PTHREAD_CC";
	echo "      - PTHREAD_CFLAGS = $PTHREAD_CFLAGS";
	echo "      - PTHREAD_LIBS = $PTHREAD_LIBS";
fi
echo
if test -z "${HAVE_RT_TRUE}"; then
	echo "  - RT Library";
	echo "      - RT_CFLAGS = $RT_CFLAGS";
	echo "      - RT_CPPFLAGS = $RT_CPPFLAGS";
	echo "      - RT_LDFLAGS = $RT_LDFLAGS";
	echo "      - RT_LIBS = $RT_LIBS";
fi
echo
dnl if test -z "${HAVE_MPI_TRUE}"; then
dnl 	echo "  - MPI";
dnl 	echo "      - MPICC = $MPICC";
dnl 	echo "      - MPILIBS = $MPILIBS";
dnl fi
dnl echo
if test -z "${HAVE_LIBSYSIO_TRUE}"; then
	echo "  - LIBSYSIO";
	echo "      - LIBSYSIO_CFLAGS = $LIBSYSIO_CFLAGS";
	echo "      - LIBSYSIO_CPPFLAGS = $LIBSYSIO_CPPFLAGS";
	echo "      - LIBSYSIO_LDFLAGS = $LIBSYSIO_LDFLAGS";
	echo "      - LIBSYSIO_LIBS = $LIBSYSIO_LIBS";
fi
echo
if test -z "${HAVE_DISKSIM_TRUE}"; then
	echo "  - DISKSIM";
	echo "      - DISKSIM_CFLAGS = $DISKSIM_CFLAGS";
	echo "      - DISKSIM_CPPFLAGS = $DISKSIM_CPPFLAGS";
	echo "      - DISKSIM_LDFLAGS = $DISKSIM_LDFLAGS";
	echo "      - DISKSIM_LIBS = $DISKSIM_LIBS";
fi
echo
if test -z "${HAVE_BDB_TRUE}"; then
	echo "  - Berkeley DB";
	echo "      - BDB_CFLAGS = $BDB_CFLAGS";
	echo "      - BDB_CPPFLAGS = $BDB_CPPFLAGS";
	echo "      - BDB_LDFLAGS = $BDB_LDFLAGS";
	echo "      - BDB_LIBS = $BDB_LIBS";
fi
echo
if test -z "${HAVE_EBOFS_TRUE}"; then
	echo "  - EBOFS";
	echo "      - EBOFS_CFLAGS = $EBOFS_CFLAGS";
	echo "      - EBOFS_CPPFLAGS = $EBOFS_CPPFLAGS";
	echo "      - EBOFS_LDFLAGS = $EBOFS_LDFLAGS";
	echo "      - EBOFS_LIBS = $EBOFS_LIBS";
fi
echo


if test x$ac_server_ok = xyes; then
	echo "  - Configured to build LWFS servers"
	test -z "${HAVE_PABLO_TRUE}" && \
		echo "      - Pablo tracing available"
	test -z "${HAVE_PABLO_FALSE}" && \
		echo "      - Pablo tracing not available"
	test -z "${HAVE_RT_TRUE}" && \
		echo "      - AIO library available";
	test -z "${HAVE_RT_FALSE}" && \
		echo "      - AIO library not available";
	test -z "${HAVE_EBOFS_TRUE}" && \
		echo "      - EBOFS library available";
	test -z "${HAVE_EBOFS_FALSE}" && \
		echo "      - EBOFS library not available";
else
	echo "  - Not configured to build LWFS servers"
	if test x$ac_server_disabled=xyes; then
	    echo "      - Servers disabled";
	else
	    test -z "${HAVE_PORTALS_FALSE}" && \
		echo "      - Portals is missing or does not work";
	    test -z "${HAVE_DB4_FALSE}" && \
		echo "      - Missing db-4.{3,2} library";
	    test -z "${HAVE_CRYPT_FALSE}" && \
		echo "      - Missing crypt library";
	    test -z "${HAVE_SSL_FALSE}" && \
		echo "      - Missing ssl library";
	    test -z "${HAVE_RT_FALSE}" && \
		echo "      - Missing rt library (needed for async I/O)";
	fi
fi

echo
if test x$ac_client_ok = xyes; then
	echo "  - Configured to build LWFS client libraries"
else
	echo "  - Not configured to build LWFS client libraries"
	if test -z "${HAVE_PORTALS_FALSE}"; then
		echo "      - Portals is missing or does not work";
	fi
fi

echo

if test -z "${BUILD_TESTS_TRUE}"; then
	echo "  - Configured to build LWFS tests"
else
	echo "  - Not configured to build LWFS tests"
fi

echo
if test -z "${BUILD_DOCS_TRUE}"; then
	echo "  - Configured to build LWFS docs"
else
	echo "  - Not configured to build LWFS docs"
fi

echo
