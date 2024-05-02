# AM_CFLAGS = -Wall -pedantic

INCLUDES =  $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable 

CLEANFILES =


# need libss_clnt.la (for client to link with) 
# same for libstorage_srvr.la until we no longer just link
noinst_LTLIBRARIES = libstorage_server.la

libstorage_server_la_LDFLAGS = 
libstorage_server_la_LIBADD = 
libstorage_server_la_CPPFLAGS = 
libstorage_server_la_SOURCES  = 

# Sources for the storage library
libstorage_server_la_SOURCES += aio_obj.c 
libstorage_server_la_SOURCES += buffer_stack.c 
libstorage_server_la_SOURCES += queue.c 
libstorage_server_la_SOURCES += storage_server.c
libstorage_server_la_SOURCES += storage_db.c
libstorage_server_la_SOURCES += sysio_obj.c

# The storage library uses the rpc_server, client, BDB, and EBOFS libraries
#libstorage_server_la_LIBADD += $(top_builddir)/src/server/rpc_server/librpc_server.la
#libstorage_server_la_LIBADD += $(top_builddir)/src/client/liblwfs_client.la
if HAVE_EBOFS
libstorage_server_la_SOURCES += ebofs_obj.c
libstorage_server_la_CPPFLAGS += $(EBOFS_CPPFLAGS)
libstorage_server_la_LDFLAGS += $(EBOFS_LDFLAGS)
libstorage_server_la_LIBADD += $(EBOFS_LIBS)
endif
if HAVE_BDB
libstorage_server_la_CPPFLAGS += $(BDB_CPPFLAGS)
libstorage_server_la_LDFLAGS += $(BDB_LDFLAGS)
libstorage_server_la_LIBADD += $(BDB_LIBS)
endif
if HAVE_RT
libstorage_server_la_CPPFLAGS += $(RT_CPPFLAGS)
libstorage_server_la_LDFLAGS += $(RT_LDFLAGS)
libstorage_server_la_LIBADD += $(RT_LIBS)
endif
if HAVE_PABLO
libstorage_server_la_CPPFLAGS += $(PABLO_CPPFLAGS)
libstorage_server_la_LDFLAGS += $(PABLO_LDFLAGS)
libstorage_server_la_LIBADD += $(PABLO_LIBS)
endif

# need lwfs-ss (will be built with libss_srvr.la)
bin_PROGRAMS =  lwfs-ss
lwfs_ss_SOURCES = 
lwfs_ss_CPPFLAGS = 
lwfs_ss_LDFLAGS = 
lwfs_ss_LDADD = 

if HAVE_GENGETOPT
cmdline.c: $(srcdir)/local_opts.ggo $(srcdir)/storage_server_opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/local_opts.ggo $(srcdir)/storage_server_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		$(top_srcdir)/src/support/threadpool/threadpool_opts.ggo \
		$(top_srcdir)/src/support/sysmon/sysmon_opts.ggo \
		$(top_srcdir)/src/client/authr_client/authr_client_opts.ggo \
		| $(GENGETOPT) -S --set-package="authr-server" \
		--set-version=$(VERSION) 

$(srcdir)/cmdline_default.h: $(srcdir)/local_opts.ggo $(srcdir)/storage_server_opts.ggo
	cat $(srcdir)/local_opts.ggo $(srcdir)/storage_server_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		$(top_srcdir)/src/support/threadpool/threadpool_opts.ggo \
		$(top_srcdir)/src/support/sysmon/sysmon_opts.ggo \
		$(top_srcdir)/src/client/authr_client/authr_client_opts.ggo \
		| $(GENGETOPT) -S --set-package="authr-server" \
		--set-version=$(VERSION) -F cmdline_default --output-dir=$(srcdir)

lwfs_ss_SOURCES += cmdline.c

CLEANFILES += cmdline.c cmdline.h
else
lwfs_ss_SOURCES += cmdline_default.c
endif

lwfs_ss_SOURCES += main.c
lwfs_ss_LDADD  += libstorage_server.la
lwfs_ss_LDADD +=  $(top_builddir)/src/server/rpc_server/librpc_server.la
lwfs_ss_LDADD +=  $(top_builddir)/src/client/liblwfs_client.la
lwfs_ss_LDADD +=  $(top_builddir)/src/support/libsupport.la
lwfs_ss_LDADD +=  $(top_builddir)/src/common/libcommon.la


if HAVE_EBOFS
#lwfs_ss_LDADD += $(EBOFS_LIBS)
#lwfs_ss_LDFLAGS += $(EBOFS_LDFLAGS)

ebofs.disk: 
	dd if=/dev/zero of=ebofs.disk bs=1k count=100000 
	mkfs.ebofs ebofs.disk

CLEANFILES += ebofs.disk

endif

