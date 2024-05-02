dnl @synopsis AC_LIBSYSIO([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro tries to find the libsysio library.
dnl
dnl On success, it defines HAVE_LIBSYSIO and sets LIBSYSIO_LIBS 
dnl to any libraries that are needed for linking 
dnl with libsysio. 
dnl
dnl If you want to compile a code to use libsysio, 
dnl
dnl     LIBS="$LIBSYSIO_LIBS $LIBS"
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if the libsysio
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands
dnl to run it if it is not found.  If ACTION-IF-FOUND is not specified,
dnl the default action will define HAVE_LIBSYSIO.
dnl
dnl @version $Id: acx_mpi.m4 676 2006-05-16 20:44:08Z raoldfi $
dnl @author Ron A. Oldfield <raoldfi@sandia.gov>

AC_DEFUN([AC_LIBSYSIO], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
ac_libsysio_ok=yes

AM_CONDITIONAL(HAVE_LIBSYSIO,true)


AC_ARG_WITH(libsysio,
	[  --with-libsysio=DIR      Location of libsysio],
	[LIBSYSIO_LDFLAGS="-L$withval/lib"; 
	 LIBSYSIO_CPPFLAGS="-I$withval/include";])

dnl Check for library
if test x$ac_libsysio_ok = xyes; then
    save_LIBS=$LIBS;
    save_LDFLAGS=$LDFLAGS; 
    LDFLAGS="$LDFLAGS $LIBSYSIO_LDFLAGS"
    LIBS=""
    AC_SEARCH_LIBS(_sysio_do_mount, [sysio],
	[ac_libsysio_ok=yes],
	[ac_libsysio_ok=no],
	[$save_LIBS])
    LIBSYSIO_LIBS=$LIBS;
    LIBS=$save_LIBS;
    LDFLAGS=$save_LDFLAGS;
fi

dnl The libsysio distribution has no include files... just a lib

if test x"$ac_libsysio_ok" = xno; then
	AM_CONDITIONAL(HAVE_LIBSYSIO,false)
fi

	
AC_SUBST(LIBSYSIO_LIBS)
AC_SUBST(LIBSYSIO_LDFLAGS)
AC_SUBST(LIBSYSIO_CPPFLAGS)


# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$ac_libsysio_ok" = xyes; then
        ifelse([$1],,[AC_DEFINE(HAVE_LIBSYSIO,1,[Define if you have the libsysio.])],[$1])
        :
else
        $2
        :
fi
AC_LANG_RESTORE
])dnl AC_LIBSYSIO
