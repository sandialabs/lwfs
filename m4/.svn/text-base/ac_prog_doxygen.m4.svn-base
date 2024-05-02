#
#
# AC_PROG_DOXYGEN
#
# Test for doxygen 
# and set $doxygen to the correct value.
#
#
dnl @synopsis AC_PROG_DOXYGEN
dnl
dnl This macro test if doxygen is installed. If doxygen
dnl is installed, it set $doxygen to the right value
dnl
dnl @version 1.3
dnl @author Ron A. Oldfield raoldfi@sandia.gov
dnl
AC_DEFUN([AC_PROG_DOXYGEN],[
AC_CHECK_PROGS(doxygen,[doxygen],no)
export doxygen;
if test $doxygen = "no" ;
then
	AC_MSG_ERROR([Unable to find a doxygen application]);
fi
AC_SUBST(doxygen)
])
