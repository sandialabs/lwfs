AC_DEFUN([AC_ADD_PIC_ARGS],
[position_independent_code='false'
AC_SUBST(SOFLAGS)
AC_SUBST(SOLIBS)
if test -n "$GCC"; then
  CFLAGS="-fPIC $CFLAGS"
  SOFLAGS="-shared"
  SOLIBS="-lstdc++ -lgcc"
else
  case "$ARCH" in
    irix6* ) SOFLAGS="-shared" ;;
    sun4u* ) CFLAGS="-KPIC $CFLAGS"
             SOFLAGS="-G" ;;
  esac
fi
if test -n "$GXX"; then
  CXXFLAGS="-fPIC $CXXFLAGS"
else
  case "$ARCH" in
    irix6* ) ;;
    sun4u* ) CXXFLAGS="-KPIC $CXXFLAGS";;
  esac
fi])
dnl
