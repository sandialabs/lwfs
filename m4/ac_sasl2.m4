dnl sasl2.m4--sasl2 libraries and includes
dnl Rob Siemborski
dnl $Id: sasl2.m4,v 1.5 2005/03/05 00:36:14 dasenbro Exp $

AC_DEFUN([SASL_GSSAPI_CHK],[
 AC_ARG_ENABLE(gssapi, [  --enable-gssapi=<DIR>   enable GSSAPI authentication [yes] ],
    gssapi=$enableval,
    gssapi=yes)
 AC_ARG_WITH(gss_impl, [  --with-gss_impl={heimdal|mit|cybersafe|seam|auto}                                                       choose specific GSSAPI implementation [[auto]] ],
    gss_impl=$withval,
    gss_impl=auto)
 AC_REQUIRE([SASL2_CRYPT_CHK])
 AC_REQUIRE([CMU_SOCKETS])

 if test "$gssapi" != no; then
    platform=
    case "${host}" in
      *-*-linux*)
        platform=__linux
        ;;
      *-*-hpux*)
        platform=__hpux
        ;;
      *-*-irix*)
        platform=__irix
        ;;
      *-*-solaris2*)
# When should we use __sunos?
        platform=__solaris
        ;;
      *-*-aix*)
###_AIX
        platform=__aix
        ;;
      *)
        AC_WARN([The system type is not recognized. If you believe that CyberSafe GSSAPI works on this platform, please update the configure script])
	if test "$gss_impl" = "cybersafe"; then
	   AC_ERROR([CyberSafe was forced, cannot continue as platform is not supported])
	fi
        ;;
    esac

    cmu_saved_CPPFLAGS=$CPPFLAGS

    if test -d ${gssapi}; then
       CPPFLAGS="$CPPFLAGS -I$gssapi/include"
# We want to keep -I in our CPPFLAGS, but only if we succeed
       cmu_saved_CPPFLAGS=$CPPFLAGS
### I am not sure how useful is this (and whether this is required at all
### especially when we have to provide two -L flags for new CyberSafe
       LDFLAGS="$LDFLAGS -L$gssapi/lib"

       if test -n "$platform"; then
         if test "$gss_impl" = "auto" -o "$gss_impl" = "cybersafe"; then
           CPPFLAGS="$CPPFLAGS -D$platform"
           if test -d "${gssapi}/appsec-sdk/include"; then
             CPPFLAGS="$CPPFLAGS -I${gssapi}/appsec-sdk/include"
           fi
         fi
       fi
    fi
    AC_CHECK_HEADER(gssapi.h, AC_DEFINE(HAVE_GSSAPI_H,,[Define if you have the gssapi.h header file]), [
      AC_CHECK_HEADER(gssapi/gssapi.h,, AC_WARN(Disabling GSSAPI - no include files found); gssapi=no)])

    CPPFLAGS=$cmu_saved_CPPFLAGS

 fi

 if test "$gssapi" != no; then
  dnl We need to find out which gssapi implementation we are
  dnl using. Supported alternatives are: MIT Kerberos 5,
  dnl Heimdal Kerberos 5 (http://www.pdc.kth.se/heimdal),
  dnl CyberSafe Kerberos 5 (http://www.cybersafe.com/)
  dnl and Sun SEAM (http://wwws.sun.com/software/security/kerberos/)
  dnl
  dnl The choice is reflected in GSSAPIBASE_LIBS

  AC_CHECK_LIB(resolv,res_search)
  if test -d ${gssapi}; then
     gssapi_dir="${gssapi}/lib"
     GSSAPIBASE_LIBS="-L$gssapi_dir"
     GSSAPIBASE_STATIC_LIBS="-L$gssapi_dir"
  else
     dnl FIXME: This is only used for building cyrus, and then only as
     dnl a real hack.  it needs to be fixed.
     gssapi_dir="/usr/local/lib"
  fi

  # Check a full link against the Heimdal libraries.
  # If this fails, check a full link against the MIT libraries.
  # If this fails, check a full link against the CyberSafe libraries.
  # If this fails, check a full link against the Solaris 8 and up libgss.

  if test "$gss_impl" = "auto" -o "$gss_impl" = "heimdal"; then
    gss_failed=0
    AC_CHECK_LIB(gssapi,gss_unwrap,gss_impl="heimdal",gss_failed=1,$GSSAPIBASE_LIBS -lgssapi -lkrb5 -lasn1 -lroken ${LIB_CRYPT} ${LIB_DES} -lcom_err ${LIB_SOCKET})
    if test "$gss_impl" != "auto" -a "$gss_failed" = "1"; then
      gss_impl="failed"
    fi
  fi

  if test "$gss_impl" = "auto" -o "$gss_impl" = "mit"; then
    gss_failed=0
    AC_CHECK_LIB(gssapi_krb5,gss_unwrap,gss_impl="mit",gss_failed=1,$GSSAPIBASE_LIBS -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err ${LIB_SOCKET})
    if test "$gss_impl" != "auto" -a "$gss_failed" = "1"; then
      gss_impl="failed"
    fi
  fi

  # For Cybersafe one has to set a platform define in order to make compilation work
  if test "$gss_impl" = "auto" -o "$gss_impl" = "cybersafe"; then

    cmu_saved_CPPFLAGS=$CPPFLAGS
    cmu_saved_GSSAPIBASE_LIBS=$GSSAPIBASE_LIBS
# FIX ME - Note that the libraries are in .../lib/64 for 64bit kernels
    if test -d "${gssapi}/appsec-rt/lib"; then
      GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -L${gssapi}/appsec-rt/lib"
    fi
    CPPFLAGS="$CPPFLAGS -D$platform"
    if test -d "${gssapi}/appsec-sdk/include"; then
      CPPFLAGS="$CPPFLAGS -I${gssapi}/appsec-sdk/include"
    fi

    gss_failed=0

# Check for CyberSafe with two libraries first, than fall back to a single 
# library (older CyberSafe)

    unset ac_cv_lib_gss_csf_gss_acq_user
    AC_CHECK_LIB(gss,csf_gss_acq_user,gss_impl="cybersafe03",[
      unset ac_cv_lib_gss_csf_gss_acq_user;AC_CHECK_LIB(gss,csf_gss_acq_user,gss_impl="cybersafe",gss_failed=1,$GSSAPIBASE_LIBS -lgss)],$GSSAPIBASE_LIBS -lgss -lcstbk5)

    if test "$gss_failed" = "1"; then
# Restore variables
      GSSAPIBASE_LIBS=$cmu_saved_GSSAPIBASE_LIBS
      CPPFLAGS=$cmu_saved_CPPFLAGS

      if test "$gss_impl" != "auto"; then
        gss_impl="failed"
      fi
    fi
  fi

  if test "$gss_impl" = "auto" -o "$gss_impl" = "seam"; then
    gss_failed=0
    AC_CHECK_LIB(gss,gss_unwrap,gss_impl="seam",gss_failed=1,-lgss)
    if test "$gss_impl" != "auto" -a "$gss_failed" = "1"; then
      gss_impl="failed"
    fi
  fi

  if test "$gss_impl" = "mit"; then
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err"
    GSSAPIBASE_STATIC_LIBS="$GSSAPIBASE_LIBS $gssapi_dir/libgssapi_krb5.a $gssapi_dir/libkrb5.a $gssapi_dir/libk5crypto.a $gssapi_dir/libcom_err.a"
  elif test "$gss_impl" = "heimdal"; then
    CPPFLAGS="$CPPFLAGS -DKRB5_HEIMDAL"
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgssapi -lkrb5 -lasn1 -lroken ${LIB_CRYPT} ${LIB_DES} -lcom_err"
    GSSAPIBASE_STATIC_LIBS="$GSSAPIBASE_STATIC_LIBS $gssapi_dir/libgssapi.a $gssapi_dir/libkrb5.a $gssapi_dir/libasn1.a $gssapi_dir/libroken.a $gssapi_dir/libcom_err.a ${LIB_CRYPT}"
  elif test "$gss_impl" = "cybersafe03"; then
# Version of CyberSafe with two libraries
    CPPFLAGS="$CPPFLAGS -D$platform -I${gssapi}/appsec-sdk/include"
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgss -lcstbk5"
    # there is no static libgss for CyberSafe
    GSSAPIBASE_STATIC_LIBS=none
  elif test "$gss_impl" = "cybersafe"; then
    CPPFLAGS="$CPPFLAGS -D$platform -I${gssapi}/appsec-sdk/include"
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgss"
    # there is no static libgss for CyberSafe
    GSSAPIBASE_STATIC_LIBS=none
  elif test "$gss_impl" = "seam"; then
    GSSAPIBASE_LIBS=-lgss
    # there is no static libgss on Solaris 8 and up
    GSSAPIBASE_STATIC_LIBS=none
  elif test "$gss_impl" = "failed"; then
    gssapi="no"
    GSSAPIBASE_LIBS=
    GSSAPIBASE_STATIC_LIBS=
    AC_WARN(Disabling GSSAPI - specified library not found)
  else
    gssapi="no"
    GSSAPIBASE_LIBS=
    GSSAPIBASE_STATIC_LIBS=
    AC_WARN(Disabling GSSAPI - no library)
  fi
 fi

#
# Cybersafe defines both GSS_C_NT_HOSTBASED_SERVICE and GSS_C_NT_USER_NAME in gssapi\rfckrb5.h
#
 if test "$gssapi" != "no"; then
  if test "$gss_impl" = "cybersafe" -o "$gss_impl" = "cybersafe03"; then
   AC_EGREP_CPP(hostbased_service_gss_nt_yes,
    [#include <gssapi/gssapi.h>
     #ifdef GSS_C_NT_HOSTBASED_SERVICE
      hostbased_service_gss_nt_yes
     #endif
    ], AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,,[Define if your GSSAPI implimentation defines GSS_C_NT_HOSTBASED_SERVICE]), AC_WARN(Cybersafe define not found))

  elif test "$ac_cv_header_gssapi_h" = "yes"; then
   AC_EGREP_HEADER(GSS_C_NT_HOSTBASED_SERVICE, gssapi.h,
     AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,,[Define if your GSSAPI implimentation defines GSS_C_NT_HOSTBASED_SERVICE]))
  elif test "$ac_cv_header_gssapi_gssapi_h"; then

   AC_EGREP_HEADER(GSS_C_NT_HOSTBASED_SERVICE, gssapi/gssapi.h,
     AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,,[Define if your GSSAPI implimentation defines GSS_C_NT_HOSTBASED_SERVICE]))
  fi

  if test "$gss_impl" = "cybersafe" -o "$gss_impl" = "cybersafe03"; then
   AC_EGREP_CPP(user_name_yes_gss_nt,
    [#include <gssapi/gssapi.h>
     #ifdef GSS_C_NT_USER_NAME
      user_name_yes_gss_nt
     #endif
    ], AC_DEFINE(HAVE_GSS_C_NT_USER_NAME,,[Define if your GSSAPI implimentation defines GSS_C_NT_USER_NAME]), AC_WARN(Cybersafe define not found))
  elif test "$ac_cv_header_gssapi_h" = "yes"; then
   AC_EGREP_HEADER(GSS_C_NT_USER_NAME, gssapi.h,
    AC_DEFINE(HAVE_GSS_C_NT_USER_NAME,,[Define if your GSSAPI implimentation defines GSS_C_NT_USER_NAME]))
  elif test "$ac_cv_header_gssapi_gssapi_h"; then
   AC_EGREP_HEADER(GSS_C_NT_USER_NAME, gssapi/gssapi.h,
    AC_DEFINE(HAVE_GSS_C_NT_USER_NAME,,[Define if your GSSAPI implimentation defines GSS_C_NT_USER_NAME]))
  fi
 fi

 GSSAPI_LIBS=""
 AC_MSG_CHECKING(GSSAPI)
 if test "$gssapi" != no; then
  AC_MSG_RESULT(with implementation ${gss_impl})
  AC_CHECK_LIB(resolv,res_search,GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lresolv")
  SASL_MECHS="$SASL_MECHS libgssapiv2.la"
  SASL_STATIC_OBJS="$SASL_STATIC_OBJS ../plugins/gssapi.o"

  cmu_save_LIBS="$LIBS"
  LIBS="$LIBS $GSSAPIBASE_LIBS"
  AC_CHECK_FUNCS(gsskrb5_register_acceptor_identity)
  LIBS="$cmu_save_LIBS"
  AM_CONDITIONAL(HAVE_GSSAPI,true)
else
  AC_MSG_RESULT(disabled)
  AM_CONDITIONAL(HAVE_GSSAPI,false)
fi
AC_SUBST(GSSAPI_LIBS)
AC_SUBST(GSSAPIBASE_LIBS)
])

AC_DEFUN([SASL_SET_GSSAPI_LIBS], [
    SASL_GSSAPI_LIBS_SET="yes"
])

dnl What we want to do here is setup LIB_SASL with what one would
dnl generally want to have (e.g. if static is requested, make it that,
dnl otherwise make it dynamic.

dnl We also want to create LIB_DYN_SASL and DYNSASLFLAGS.

dnl Also sets using_static_sasl to "no" "static" or "staticonly"

AC_DEFUN([CMU_SASL2], [
	AC_REQUIRE([SASL_GSSAPI_CHK])

AC_ARG_WITH(sasl,
            [  --with-sasl=DIR         Compile with libsasl2 in <DIR>],
	    with_sasl="$withval",
            with_sasl="yes")

AC_ARG_WITH(staticsasl,
	    [  --with-staticsasl=DIR   Compile with staticly linked libsasl2 in <DIR>],
	    with_staticsasl="$withval";
	    if test $with_staticsasl != "no"; then
		using_static_sasl="static"
	    fi,
	    with_staticsasl="no"; using_static_sasl="no")

	SASLFLAGS=""
	LIB_SASL=""

	cmu_saved_CPPFLAGS=$CPPFLAGS
	cmu_saved_LDFLAGS=$LDFLAGS
	cmu_saved_LIBS=$LIBS

	if test ${with_staticsasl} != "no"; then
	  if test -d ${with_staticsasl}; then
	    if test -d ${with_staticsasl}/lib64 ; then
	      ac_cv_sasl_where_lib=${with_staticsasl}/lib64
	    else
	      ac_cv_sasl_where_lib=${with_staticsasl}/lib
	    fi
	    ac_cv_sasl_where_lib=${with_staticsasl}/lib
	    ac_cv_sasl_where_inc=${with_staticsasl}/include

	    SASLFLAGS="-I$ac_cv_sasl_where_inc"
	    LIB_SASL="-L$ac_cv_sasl_where_lib"
	    CPPFLAGS="${cmu_saved_CPPFLAGS} -I${ac_cv_sasl_where_inc}"
	    LDFLAGS="${cmu_saved_LDFLAGS} -L${ac_cv_sasl_where_lib}"
	  else
	    with_staticsasl="/usr"
	  fi

	  AC_CHECK_HEADER(sasl/sasl.h, [
	    AC_CHECK_HEADER(sasl/saslutil.h, [
	     for i42 in lib64 lib; do
	        if test -r ${with_staticsasl}/$i42/libsasl2.a; then
		  ac_cv_found_sasl=yes
		  AC_MSG_CHECKING(for static libsasl)
		  LIB_SASL="$LIB_SASL ${with_staticsasl}/$i42/libsasl2.a"
		fi
	     done
             if test ! "$ac_cv_found_sasl" = "yes"; then
	          AC_MSG_CHECKING(for static libsasl)
		  AC_ERROR([Could not find ${with_staticsasl}/lib*/libsasl2.a])
             fi
	    ])])

	  AC_MSG_RESULT(found)

          if test "x$SASL_GSSAPI_LIBS_SET" = "x"; then
	    LIB_SASL="$LIB_SASL $GSSAPIBASE_STATIC_LIBS"
	  else
	    SASL_GSSAPI_LIBS_SET=""
	    cmu_saved_LIBS="$GSSAPIBASE_STATIC_LIBS $cmu_saved_LIBS" 
	  fi
	fi

	if test -d ${with_sasl}; then
            ac_cv_sasl_where_lib=${with_sasl}/lib
            ac_cv_sasl_where_inc=${with_sasl}/include

	    DYNSASLFLAGS="-I$ac_cv_sasl_where_inc"
	    if test "$ac_cv_sasl_where_lib" != ""; then
		CMU_ADD_LIBPATH_TO($ac_cv_sasl_where_lib, LIB_DYN_SASL)
	    fi
	    LIB_DYN_SASL="$LIB_DYN_SASL -lsasl2"
	    CPPFLAGS="${cmu_saved_CPPFLAGS} -I${ac_cv_sasl_where_inc}"
	    LDFLAGS="${cmu_saved_LDFLAGS} -L${ac_cv_sasl_where_lib}"
	fi

	dnl be sure to check for a SASLv2 specific function
	AC_CHECK_HEADER(sasl/sasl.h, [
	    AC_CHECK_HEADER(sasl/saslutil.h, [
	      AC_CHECK_LIB(sasl2, prop_get, 
                           ac_cv_found_sasl=yes,
		           ac_cv_found_sasl=no)],
	                   ac_cv_found_sasl=no)], ac_cv_found_sasl=no)

	if test "$ac_cv_found_sasl" = "yes"; then
	    if test "$ac_cv_sasl_where_lib" != ""; then
	        CMU_ADD_LIBPATH_TO($ac_cv_sasl_where_lib, DYNLIB_SASL)
	    fi
	    DYNLIB_SASL="$DYNLIB_SASL -lsasl2"
	    if test "$using_static_sasl" != "static"; then
		LIB_SASL=$DYNLIB_SASL
		SASLFLAGS=$DYNSASLFLAGS
	    fi
	else
	    DYNLIB_SASL=""
	    DYNSASLFLAGS=""
	    using_static_sasl="staticonly"
	fi

        if test "x$SASL_GSSAPI_LIBS_SET" != "x"; then
	    SASL_GSSAPI_LIBS_SET=""
	    cmu_saved_LIBS="$GSSAPIBASE_LIBS $cmu_saved_LIBS" 
	fi

	LIBS="$cmu_saved_LIBS"
	LDFLAGS="$cmu_saved_LDFLAGS"
	CPPFLAGS="$cmu_saved_CPPFLAGS"

	AC_SUBST(LIB_DYN_SASL)
	AC_SUBST(DYNSASLFLAGS)
	AC_SUBST(LIB_SASL)
	AC_SUBST(SASLFLAGS)
	])

AC_DEFUN([CMU_SASL2_REQUIRED],
[AC_REQUIRE([CMU_SASL2])
if test "$ac_cv_found_sasl" != "yes"; then
        AC_ERROR([Cannot continue without libsasl2.
Get it from ftp://ftp.andrew.cmu.edu/pub/cyrus-mail/.])
fi])

AC_DEFUN([CMU_SASL2_REQUIRE_VER], [
	AC_REQUIRE([CMU_SASL2_REQUIRED])

	cmu_saved_CPPFLAGS=$CPPFLAGS
	CPPFLAGS="$CPPFLAGS $SASLFLAGS"

	AC_TRY_CPP([
#include <sasl/sasl.h>

#ifndef SASL_VERSION_MAJOR
#error SASL_VERSION_MAJOR not defined
#endif
#ifndef SASL_VERSION_MINOR
#error SASL_VERSION_MINOR not defined
#endif
#ifndef SASL_VERSION_STEP
#error SASL_VERSION_STEP not defined
#endif

#if SASL_VERSION_MAJOR < $1 || SASL_VERSION_MINOR < $2 || SASL_VERSION_STEP < $3
#error SASL version is less than $1.$2.$3
#endif
	],,AC_ERROR([Incorrect SASL headers found.  This package requires SASL $1.$2.$3 or newer.]))

	CPPFLAGS=$cmu_saved_CPPFLAGS
])

AC_DEFUN([CMU_SASL2_CHECKAPOP_REQUIRED], [
	AC_REQUIRE([CMU_SASL2_REQUIRED])

	cmu_saved_LDFLAGS=$LDFLAGS

	LDFLAGS="$LDFLAGS $LIB_SASL"

	AC_CHECK_LIB(sasl2, sasl_checkapop, AC_DEFINE(HAVE_APOP,[],[Does SASL support APOP?]),
		AC_MSG_ERROR([libsasl2 without working sasl_checkapop.  Cannot continue.]))

	LDFLAGS=$cmu_saved_LDFLAGS
])

AC_DEFUN([SASL2_CRYPT_CHK],[
 AC_CHECK_FUNC(crypt, cmu_have_crypt=yes, [
  AC_CHECK_LIB(crypt, crypt,
	       LIB_CRYPT="-lcrypt"; cmu_have_crypt=yes,
	       cmu_have_crypt=no)])
 AC_SUBST(LIB_CRYPT)
])
