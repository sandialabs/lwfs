dnl ----------------- LWFS tests cofig file ----------------
dnl   This file contains the autoconf configuration script
dnl   used to build the LWFS tests. 
dnl ----------------------------------------------------

AC_INIT(configure.ac)
AC_CONFIG_AUX_DIR(config)
AC_REVISION($Revision:$)

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(lwfs-tests, 0.1)

ac_server_disabled=no

dnl ---------------- Command-line arguments -------------
AC_ARG_WITH(lwfs-srcdir,
	[--with-lwfs-srcdir[=DIR]],
	[LWFS_SRCDIR=$withval],
	[LWFS_SRCDIR=$pwd/../])

AC_SUBST(LWFS_SRCDIR)

AC_ARG_WITH(lwfs-builddir,
	[--with-lwfs-builddir[=DIR]],
	[LWFS_BUILDDIR=$withval],
	[LWFS_BUILDDIR=${PWD}/../])

AC_SUBST(LWFS_BUILDDIR)

AC_ARG_ENABLE(server, 
	[  --enable-server     Test the LWFS servers], 
	[case "${enableval}" in 
		yes) ac_server_disabled=no ;; 
		no)  ac_server_disabled=yes ;; 
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-server) ;;
	esac],
	[ac_server_ok=yes])


if test x$ac_server_disabled = xyes; then
    AC_MSG_WARN(servers disabled)
    AM_CONDITIONAL(TEST_SERVERS,false)
else
    AC_MSG_WARN(servers enabled)
    AM_CONDITIONAL(TEST_SERVERS,true)
fi

dnl  ------- this is required to allow the UINT*_C macros in C++ -----
CXXFLAGS="${CXXFLAGS} -D__STDC_CONSTANT_MACROS"

dnl  ---------------- COMPILER CHECKS  ----------------------------

AC_LANG_C
AC_PROG_CC
AC_PROG_LIBTOOL

ACX_MPI

AC_CHECK_HEADER(string.h, [], [])
AC_CHECK_HEADER(strings.h, [], [])

if test x$ac_server_disabled = xyes; then
    AM_CONDITIONAL(HAVE_EBOFS,false)
    AM_CONDITIONAL(HAVE_BDB,false)
    AM_CONDITIONAL(HAVE_PABLO,false)
    AM_CONDITIONAL(HAVE_OPENSSL,false)
    AM_CONDITIONAL(HAVE_GSSAPI,false)
    AM_CONDITIONAL(HAVE_RT,false)
else

dnl -- Check for libebofs
AC_EBOFS([], [AC_MSG_WARN("missing ebofs")])

dnl Find working version of berkeley  db
AX_BERKELEY_DB([4.2],
	[AM_CONDITIONAL(HAVE_BDB,true)],
	[AC_MSG_WARN("missing berkeley db");
	 AM_CONDITIONAL(HAVE_BDB,false)])

dnl -- Pablo required for tracing
AC_PABLO([], [AC_MSG_WARN("missing Pablo ... tracing disabled")])

dnl Find working version of ssl and crypto library (defines HAVE_SSL)
AC_OPENSSL([],
	[AC_MSG_WARN("missing openssl ... LWFS servers disabled");
	ac_server_ok=no])

dnl -- Check for gssapi
SASL_GSSAPI_CHK([],[AC_MSG_WARN("missing gssapi")])

dnl -- real-time library required for asynchronous I/O library
dnl -- (defines HAVE_RT, RT_{CPPFLAGS,CFLAGS,LDFLAGS,LIBS})
AC_RT([], [AC_MSG_WARN("missing rtlib ... aio disabled")])

fi

dnl -- Check for Lee's libsysio
AC_LIBSYSIO([], [AC_MSG_WARN("missing libsysio")])


dnl -- Portals required for networking
AC_PORTALS([], 
	[ac_server_ok=no; ac_client_ok=no;
	AC_MSG_WARN("missing Portals ... LWFS disabled")])


dnl AX_PATH_BDB([4.2],
dnl 	[AM_CONDITIONAL(HAVE_BDB,true)],
dnl 	[AC_MSG_WARN("missing berkeley db");
dnl 	 AM_CONDITIONAL(HAVE_BDB,false)])


dnl Find working version of gengetopt
AC_PROG_GENGETOPT

dnl  ---------------- FINAL TESTS  ----------------------------


dnl  ---------------- Generate Makefiles  ----------------------------

AC_CONFIG_FILES([Makefile
		checkpoint-tests/Makefile
		mds-api-tests/Makefile
		fuse-lwfs/Makefile
		db-tests/Makefile
		gss-api-tests/Makefile
		portals-tests/Makefile
		xdr-tests/Makefile
		rpc-tests/Makefile
		threadpool-tests/Makefile
		rpc-tests/tcp-xfer/Makefile
		rpc-tests/mpi-xfer/Makefile
		rpc-tests/rpc-xfer/Makefile
		rpc-tests/lwfs-xfer/Makefile
		rpc-tests/portals-xfer/Makefile
		ss-tests/Makefile
		authr-tests/Makefile
		naming-tests/Makefile
		naming-tests/results/Makefile
		tracing-tests/Makefile
		libsysio-tests/Makefile
		xchange-tests/Makefile
		ebofs-tests/Makefile])

AC_OUTPUT

