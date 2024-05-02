# AM_CFLAGS = -Wall -pedantic

INCLUDES =  $(all_includes)
INCLUDES += -I$(top_srcdir)/src 

METASOURCES = AUTO

#AM_CPPFLAGS = -Wall -Wno-unused-variable -fno-strict-aliasing


# need libss_clnt.la (for client to link with) 
# same for libstorage_srvr.la until we no longer just link
noinst_LTLIBRARIES = libstorage_common.la

libstorage_common_la_SOURCES = ss_args.c 
libstorage_common_la_SOURCES += ss_debug.c
libstorage_common_la_SOURCES += ss_xdr.c

# Generating the XDR files

ss_args.lo: ss_args.c
	$(LTCOMPILE) -Wno-unused-variable -c $<

$(srcdir)/ss_args.c : $(srcdir)/ss_args.h 
	rm -f $(srcdir)/ss_args.c 
	rpcgen -c $(srcdir)/ss_args.x > $(srcdir)/ss_args.c

$(srcdir)/ss_args.h: $(srcdir)/ss_args.x
	rm -f $(srcdir)/ss_args.h
	rpcgen -h $(srcdir)/ss_args.x -o $(srcdir)/ss_args.h

ss_args.o: ss_args.c
	$(CC) -c ss_args.c

CLEANFILES = $(srcdir)/ss_args.c $(srcdir)/ss_args.h
