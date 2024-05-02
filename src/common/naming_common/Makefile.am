INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

#AM_LIBS = $(CLIENT_LIBS)
#AM_CFLAGS = $(CLIENT_CFLAGS)
#AM_CPPFLAGS = -Wall -Wno-unused-variable -fno-strict-aliasing \
#	      -D_GNU_SOURCE $(CLIENT_CPPFLAGS)

noinst_LTLIBRARIES = libnaming_common.la

libnaming_common_la_SOURCES = naming_args.c
libnaming_common_la_SOURCES += naming_xdr.c
libnaming_common_la_SOURCES += naming_debug.c

noinst_HEADERS = naming_args.h
noinst_HEADERS += naming_xdr.h
noinst_HEADERS += naming_debug.h

naming_args.lo: naming_args.c
	$(LTCOMPILE) -Wno-unused-variable -c $<

$(srcdir)/naming_args.c : $(srcdir)/naming_args.h
	rm -f $(srcdir)/naming_args.c
	rpcgen -c $(srcdir)/naming_args.x > $(srcdir)/naming_args.c

$(srcdir)/naming_args.h: $(srcdir)/naming_args.x
	rm -f $(srcdir)/naming_args.h
	rpcgen -h $(srcdir)/naming_args.x -o $(srcdir)/naming_args.h

CLEANFILES = $(srcdir)/naming_args.c $(srcdir)/naming_args.h
