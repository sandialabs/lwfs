INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable

#CFLAGS += -Wno-unused-variable

noinst_LTLIBRARIES =  libtypes.la
libtypes_la_SOURCES = xdr_types.c types.c fprint_types.c lwfs_int.h

# Do not include warnings for unused variables in generated code
xdr_types.lo: xdr_types.c $(srcdir)/lwfs_int.h
	$(LTCOMPILE) -Wno-unused-variable -c $<

$(srcdir)/xdr_types.c : $(srcdir)/xdr_types.h
	rm -f $(srcdir)/xdr_types.c
	rpcgen -c $(srcdir)/xdr_types.x -o $(srcdir)/xdr_types.c

$(srcdir)/xdr_types.h: $(srcdir)/xdr_types.x
	rm -f $(srcdir)/xdr_types.h
	rpcgen -h $(srcdir)/xdr_types.x -o $(srcdir)/xdr_types.h


# This is to make sure we use the latest configure-generated lwfs_int.h
$(srcdir)/lwfs_int.h: $(top_builddir)/src/common/types/lwfs_int.h
	cp $(top_builddir)/src/common/types/lwfs_int.h $(srcdir)

CLEANFILES=core.* *~  $(srcdir)/xdr_types.c $(srcdir)/xdr_types.h
