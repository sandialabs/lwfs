INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_LIBS = 
AM_CFLAGS = $(CLIENT_CFLAGS)
AM_CPPFLAGS = -Wall -Wno-unused-variable -D_GNU_SOURCE $(CLIENT_CPPFLAGS)

noinst_LTLIBRARIES = librpc_common.la

librpc_common_la_SOURCES = service_args.c
librpc_common_la_SOURCES += lwfs_ptls.c
librpc_common_la_SOURCES += rpc_xdr.c
librpc_common_la_SOURCES += rpc_common.c
librpc_common_la_SOURCES += rpc_debug.c
librpc_common_la_SOURCES += ptl_wrap.c
if NEED_LWFS_XDR_SIZEOF
librpc_common_la_SOURCES += xdr_sizeof.c
endif


librpc_common_la_LIBADD = $(PORTALS_LIBS)

service_args.lo: service_args.c
	$(LTCOMPILE) -Wno-unused-variable -c $<

# This has to be put into the source dir because of 
# the way rpcgen inserts the include lines (argh!!!)
$(srcdir)/service_args.c : $(srcdir)/service_args.h $(srcdir)/Makefile.am
	rm -f $(srcdir)/service_args.c
	rpcgen -c $(srcdir)/service_args.x > $(srcdir)/service_args.c

$(srcdir)/service_args.h: $(srcdir)/service_args.x
	rm -f $(srcdir)/service_args.h
	rpcgen -h $(srcdir)/service_args.x -o $(srcdir)/service_args.h

CLEANFILES = $(srcdir)/service_args.c $(srcdir)/service_args.h

