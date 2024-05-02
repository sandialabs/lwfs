dnl @synopsis AC_EBOFS([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro tries to find out how to compile programs that
dnl use the EBOFS API. 
dnl
dnl On success, it defines HAVE_EBOFS and sets EBOFS_LIBS 
dnl to any libraries that are needed for linking 
dnl EBOFS (e.g. -lp3utcp, -lp3lib,...). 
dnl
dnl If you want to compile everything with EBOFS, you should set:
dnl
dnl     LIBS="$EBOFS_LIBS $LIBS"
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a EBOFS
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands
dnl to run it if it is not found.  If ACTION-IF-FOUND is not specified,
dnl the default action will define HAVE_EBOFS.
dnl
dnl @version $Id: acx_mpi.m4 676 2006-05-16 20:44:08Z raoldfi $
dnl @author Ron A. Oldfield <raoldfi@sandia.gov>

AC_DEFUN([AC_EBOFS], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_REQUIRE([ACX_PTHREAD])
AC_LANG_SAVE
AC_LANG_C

AC_LANG_PUSH([C++])
ac_ebofs_ok=yes
ac_ebofs_path="."


AC_ARG_WITH(ebofs,
	[  --with-ebofs=DIR      Location of EBOFS],
	[ac_ebofs_path=$withval;
	 EBOFS_LDFLAGS="$-L$withval/lib"; 
	 EBOFS_CPPFLAGS="-I$withval/include";])


dnl if test -z "${HAVE_EBOFS_TRUE}"; then
dnl 	AC_CHECK_LIB(ebofs, main, 
dnl 		[EBOFS_LIBS="-lebofs"],
dnl 		[AM_CONDITIONAL(HAVE_EBOFS,false)])
dnl fi


if test x"$ac_ebofs_ok" = xyes; then
	save_PATH=$PATH
	export PATH="$PATH:$ac_ebofs_path/bin";
	AC_PATH_PROG(MKFS_EBOFS, mkfs.ebofs, 
		[not_found])
	export PATH=$save_PATH; 
	if test x"$MKFS_EBOFS" = xnot_found; then
	    ac_ebofs_ok=no
	fi
fi


if test x"$ac_ebofs_ok" = xyes; then
	AC_CHECK_HEADER(ebofs/ebofs.h, 
		[], 
		[ac_ebofs_ok=no])
fi

EBOFS_LIBS="-lebofs"


ac_ebofs_flags="none ${PTHREAD_CFLAGS} ${PTHREAD_LIBS}"

if test x"$ac_ebofs_ok" = xyes; then
ac_ebofs_ok=no
for flag in $ac_ebofs_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether ebofs works without any additional flags])
                ;;

                -l*)
                AC_MSG_CHECKING([whether ebofs works with library $flag])
                EXTRA_LIBS="$flag"
                ;;

                -*)
                AC_MSG_CHECKING([whether ebofs works with library -l$flag])
                EXTRA_CFLAGS="$flag"
                ;;
        esac


        save_LDFLAGS="$LDFLAGS"
        save_LIBS="$LIBS"
        save_CPPFLAGS="$CPPFLAGS"
        save_CFLAGS="$CFLAGS"
	LDFLAGS="$LDFLAGS $EBOFS_LDFLAGS"
        LIBS="$LIBS $EBOFS_LIBS $EXTRA_LIBS"
	CPPFLAGS="$CPPFLAGS $EBOFS_CPPFLAGS"
        CFLAGS="$CFLAGS $EBOFS_CFLAGS $EXTRA_FLAGS"

	dnl AC_MSG_CHECKING([CPPFLAGS=$CPPFLAGS])
	dnl AC_MSG_CHECKING([LDFLAGS=$LDFLAGS])
	dnl AC_MSG_CHECKING([LIBS=$LIBS])

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.

	AC_LINK_IFELSE(AC_LANG_PROGRAM(
		    [[#include "ebofs/ebofs.h"]], 
		    [[ebofs_mount(NULL);]]),
		[ac_ebofs_ok=yes;],
		[ac_ebofs_ok=no;])
		
		
		dnl AC_LANG_CALL([#include "ebofs/ebofs.h"],[obj_exists]),
		dnl [ac_ebofs_ok=yes;],
		dnl [ac_ebofs_ok=no;])

        LDFLAGS="$save_LDFLAGS"
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        CPPFLAGS="$save_CPPFLAGS"

        AC_MSG_RESULT($ac_ebofs_ok)
        if test "x$ac_ebofs_ok" = xyes; then
		EBOFS_LIBS="$EBOFS_LIBS $EXTRA_LIBS";
		EBOFS_CFLAGS="$EBOFS_CFLAGS $EXTRA_CFLAGS";
                break;
        fi

        EXTRA_LIBS=""
        EXTRA_CFLAGS=""
done
fi

AM_CONDITIONAL(HAVE_EBOFS, test x$ac_ebofs_ok = xyes)
	
AC_SUBST(EBOFS_LIBS)
AC_SUBST(EBOFS_CFLAGS)
AC_SUBST(EBOFS_CPPFLAGS)
AC_SUBST(EBOFS_LDFLAGS)


# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$ac_ebofs_ok" = xyes; then
        ifelse([$1],,[AC_DEFINE(HAVE_EBOFS,1,[Define if you have EBOFS.])],[$1])
        :
else
        $2
        :
fi

AC_LANG_POP([C++])

AC_LANG_RESTORE
])dnl AC_EBOFS
