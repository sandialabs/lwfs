#
#
# AC_PROG_XSLTPROC
#
# Test for xsltproc
# and set $xsltproc to the correct value.
#
#
dnl @synopsis AC_PROG_XSLTPROC
dnl
dnl This macro test if dvipdf is installed. If dvipdf
dnl is installed, it set $dvipdf to the right value
dnl
dnl @version 1.3
dnl @author Mathieu Boretti boretti@eig.unige.ch
dnl
AC_DEFUN([AC_PROG_XSLTPROC],[
AC_CHECK_PROGS(xsltproc,xsltproc,no)
export xsltproc;
if test $xsltproc = "no" ;
then
	AC_MSG_ERROR([Unable to find a xsltproc application]);
fi;
AC_SUBST(xsltproc)
])
