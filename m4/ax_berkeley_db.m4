AC_DEFUN([AX_BERKELEY_DB],
[

  AC_ARG_WITH(berkeley-db,
	[  --with-berkeley-db=DIR      Location of Berkeley DB],
	[BDB_CPPFLAGS="-I$withval/include";
	 BDB_LDFLAGS="-L$withval/lib"; 
	 ac_disksim_path=$withval])

  old_LIBS="$LIBS"
  old_LDFLAGS="$LDFLAGS"
  old_CPPFLAGS="$CPPFLAGS"

  CPPFLAGS="$CPPFLAGS $BDB_CPPFLAGS"
  LDFLAGS="$LDFLAGS $BDB_LDFLAGS"

  minversion=ifelse([$1], ,,$1)

  BDB_HEADER=""
  BDB_LIBS=""

  if test -z $minversion ; then
      minvermajor=0
      minverminor=0
      minverpatch=0
      AC_MSG_CHECKING([for Berkeley DB])
  else
      minvermajor=`echo $minversion | cut -d. -f1`
      minverminor=`echo $minversion | cut -d. -f2`
      minverpatch=`echo $minversion | cut -d. -f3`
      minvermajor=${minvermajor:-0}
      minverminor=${minverminor:-0}
      minverpatch=${minverpatch:-0}
      AC_MSG_CHECKING([for Berkeley DB >= $minversion])
  fi

  for version in "" 5.0 4.9 4.8 4.7 4.6 4.5 4.4 4.3 4.2 4.1 4.0 3.6 3.5 3.4 3.3 3.2 3.1 ; do

    if test -z $version ; then
        db_lib="-ldb"
        try_headers="db.h"
    else
        db_lib="-ldb-$version"
        try_headers="db$version/db.h db`echo $version | sed -e 's,\..*,,g'`/db.h"
    fi

    LIBS="$old_LIBS $db_lib"

    for db_hdr in $try_headers ; do
        if test -z $BDB_HEADER ; then
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM(
                    [
                        #include <${db_hdr}>
                    ],
                    [
                        #if !((DB_VERSION_MAJOR > (${minvermajor}) || \
                              (DB_VERSION_MAJOR == (${minvermajor}) && \
                                    DB_VERSION_MINOR > (${minverminor})) || \
                              (DB_VERSION_MAJOR == (${minvermajor}) && \
                                    DB_VERSION_MINOR == (${minverminor}) && \
                                    DB_VERSION_PATCH >= (${minverpatch}))))
                            #error "too old version"
                        #endif

                        DB *db;
                        db_create(&db, NULL, 0);
                    ])],
                [
                    AC_MSG_RESULT([header $db_hdr, library $db_lib])

                    BDB_HEADER="$db_hdr"
                    BDB_LIBS="$db_lib"
                ])
        fi
    done
  done

  LIBS="$old_LIBS"
  LDFLAGS="$old_LDFLAGS"
  CPPFLAGS="$old_CPPFLAGS"

  if test -z $BDB_HEADER ; then
    AC_MSG_RESULT([not found])
    ifelse([$3], , :, [$3])
  else
    AC_DEFINE_UNQUOTED([BDB_HEADER], ["$BDB_HEADER"], ["Path to db.h"])
    AC_SUBST(BDB_LIBS)
    AC_SUBST(BDB_LDFLAGS)
    AC_SUBST(BDB_CPPFLAGS)
    ifelse([$2], , :, [$2])
  fi
])
