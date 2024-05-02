INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I$(top_builddir)/src/common/authr_common

METASOURCES = AUTO

AM_LIBS = $(CLIENT_LIBS)
AM_CFLAGS = $(CLIENT_CFLAGS)

noinst_LTLIBRARIES = libauthr_common.la

libauthr_common_la_SOURCES = authr_args.c
libauthr_common_la_SOURCES += authr_xdr.c
libauthr_common_la_SOURCES += authr_debug.c

libauthr_common_la_LIBADD = $(PORTALS_LIBS)

noinst_HEADERS = authr_args.h 
noinst_HEADERS += authr_debug.h 
noinst_HEADERS += authr_opcodes.h 
noinst_HEADERS += authr_trace.h 
noinst_HEADERS += authr_xdr.h 

authr_args.lo: authr_args.c
	$(LTCOMPILE) -Wno-unused-variable -c $<

# This has to be put into the source dir because of 
# the way rpcgen inserts the include lines (argh!!!)
$(srcdir)/authr_args.c : $(srcdir)/authr_args.h $(srcdir)/Makefile.am
	rm -f $(srcdir)/authr_args.c
	rpcgen -c $(srcdir)/authr_args.x > $(srcdir)/authr_args.c

$(srcdir)/authr_args.h: $(srcdir)/authr_args.x
	rm -f $(srcdir)/authr_args.h
	rpcgen -h $(srcdir)/authr_args.x -o $(srcdir)/authr_args.h

CLEANFILES = $(srcdir)/authr_args.c $(srcdir)/authr_args.h
