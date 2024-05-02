dnl @synopsis AC_DISKSIM([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro tries to find out how to compile programs that
dnl use the disksim library and API. 
dnl
dnl On success, it defines HAVE_DISKSIM and sets DISKSIM_LIBS 
dnl to any libraries that are needed for linking DISKSIM. 
dnl
dnl If you want to compile everything with DISKSIM, you should set:
dnl
dnl     LIBS="$DISKSIM_LIBS $LIBS"
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if the DISKSIM
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands
dnl to run it if it is not found.  If ACTION-IF-FOUND is not specified,
dnl the default action will define HAVE_DISKSIM.
dnl
dnl @version $Id: acx_mpi.m4 676 2006-05-16 20:44:08Z raoldfi $
dnl @author Ron A. Oldfield <raoldfi@sandia.gov>

AC_DEFUN([AC_DISKSIM], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_REQUIRE([ACX_PTHREAD])
AC_LANG_SAVE
AC_LANG_C
ac_disksim_ok=yes
ac_disksim_path=""

AM_CONDITIONAL(HAVE_DISKSIM,true)

DISKSIM_CPPFLAGS=""
DISKSIM_LIBS="-ldisksim -lparam -ldiskmodel -lm"

AC_ARG_WITH(disksim,
	[  --with-disksim=DIR      Location of DISKSIM],
	[DISKSIM_CPPFLAGS="-I$withval/include";
	 DISKSIM_LDFLAGS="-L$withval/lib"; 
	 ac_disksim_path=$withval])

dnl Look for a header file
save_CPPFLAGS=${CPPFLAGS}
CPPFLAGS="${CPPFLAGS} ${DISKSIM_CPPFLAGS}"
if test x"$ac_disksim_ok" = xyes; then
	dnl AC_MSG_NOTICE([searching for includes in $CPPFLAGS])
	AC_CHECK_HEADER(disksim_iosim.h, 
		[], 
		[ac_disksim_ok=no])
fi
CPPFLAGS=$save_CPPFLAGS;


ac_disksim_flags="none ${PTHREAD_CFLAGS} ${PTHREAD_LIBS}"

if test x"$ac_disksim_ok" = xyes; then
for flag in $ac_disksim_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether disksim works without any additional flags])
                ;;

                -l*)
                AC_MSG_CHECKING([whether disksim works with $flag])
                EXTRA_LIBS="$flag"
                ;;

                -*)
                AC_MSG_CHECKING([whether disksim works with -l$flag])
                EXTRA_CFLAGS="$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
	save_LDFLAGS="$LDFLAGS"
        LIBS="$LIBS $DISKSIM_LIBS $EXTRA_LIBS"
        CFLAGS="$CFLAGS $DISKSIM_CFLAGS $EXTRA_FLAGS"
        LDFLAGS="$LDFLAGS $DISKSIM_LDFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK_FUNC([disksim_loadparams],
		    [ac_disksim_ok=yes],
                    [ac_disksim_ok=no])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
	LDFLAGS="$save_LDFLAGS"

        AC_MSG_RESULT($ac_disksim_ok)
        if test "x$ac_disksim_ok" = xyes; then
		DISKSIM_LIBS="$DISKSIM_LIBS $EXTRA_LIBS";
		DISKSIM_CFLAGS="$DISKSIM_CFLAGS $EXTRA_CFLAGS";
                break;
        fi

        EXTRA_LIBS=""
        EXTRA_CFLAGS=""
done
fi

if test x"$ac_disksim_ok" = xyes; then
	AM_CONDITIONAL(HAVE_DISKSIM,true)
else
	AM_CONDITIONAL(HAVE_DISKSIM,false)
fi


AC_SUBST(DISKSIM_LDFLAGS)
AC_SUBST(DISKSIM_LIBS)
AC_SUBST(DISKSIM_CFLAGS)
AC_SUBST(DISKSIM_CPPFLAGS)


# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$ac_disksim_ok" = xyes; then
        ifelse([$1],,[AC_DEFINE(HAVE_DISKSIM,1,[Define if you have DISKSIM.])],[$1])
        :
else
        $2
        :
fi
AC_LANG_RESTORE
])dnl AC_DISKSIM
