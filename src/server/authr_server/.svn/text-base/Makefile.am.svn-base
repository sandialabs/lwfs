INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src


METASOURCES = AUTO
CLEANFILES = 

AM_CFLAGS = $(CFLAGS) $(OPENSSL_CFLAGS)  $(BDB_CFLAGS)
AM_LDFLAGS = $(OPENSSL_LDFLAGS) $(BDB_LDFLAGS) $(PORTALS_LDFLAGS) $(PABLO_LDFLAGS)
AM_CPPFLAGS  = -Wall -Wno-unused-variable $(OPENSSL_CPPFLAGS) $(BDB_CPPFLAGS) -DHAVE_CRAY_PORTALS

noinst_LTLIBRARIES = libauthr_server.la

libauthr_server_la_SOURCES = authr_server.c
libauthr_server_la_SOURCES += authr_db.c
libauthr_server_la_SOURCES += cap.c
libauthr_server_la_LDFLAGS = $(BDB_LDFLAGS) $(OPENSSL_LDFLAGS) $(PABLO_LDFLAGS)
libauthr_server_la_LIBADD = $(BDB_LIBS) $(OPENSSL_LIBS) $(PABLO_LIBS)


bin_PROGRAMS = lwfs-authr
lwfs_authr_SOURCES = 
lwfs_authr_LDFLAGS =
lwfs_authr_LDADD = 


# If we have gengetopt, use it
if HAVE_GENGETOPT
cmdline.c: $(srcdir)/authr_server_opts.ggo Makefile.am
	cat $(srcdir)/authr_server_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		$(top_srcdir)/src/support/threadpool/threadpool_opts.ggo \
		$(top_srcdir)/src/support/sysmon/sysmon_opts.ggo \
		| $(GENGETOPT) -S --set-package="authr-server" --set-version=$(VERSION)

# generate cmdline_default only if the ggo file changed
$(srcdir)/cmdline_default.c: $(srcdir)/authr_server_opts.ggo
	cat $(srcdir)/authr_server_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		$(top_srcdir)/src/support/threadpool/threadpool_opts.ggo \
		$(top_srcdir)/src/support/sysmon/sysmon_opts.ggo \
		| $(GENGETOPT) -S --set-package="authr-server" \
		--set-version=$(VERSION) -F cmdline_default --output-dir=$(srcdir)

CLEANFILES += cmdline.c cmdline.h

lwfs_authr_SOURCES += cmdline.c 

else 
# use the default command-line code
lwfs_authr_SOURCES += cmdline_default.c
endif

lwfs_authr_SOURCES += main.c
lwfs_authr_LDADD  += libauthr_server.la
lwfs_authr_LDADD += $(top_builddir)/src/server/rpc_server/librpc_server.la
lwfs_authr_LDADD += $(top_builddir)/src/common/libcommon.la
lwfs_authr_LDADD += $(top_builddir)/src/support/libsupport.la
#lwfs_authr_LDADD += $(PABLO_LIBS)
#lwfs_authr_LDADD += $(top_builddir)/src/support/trace/libtrace.la


CLEANFILES += acls.db *.sddf  authr.db
