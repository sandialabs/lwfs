INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall $(BDB_CPPFLAGS)
AM_LDFLAGS = $(PORTALS_LDFLAGS) $(BDB_LDFLAGS) $(PABLO_LDFLAGS)
CLEANFILES = 


noinst_LTLIBRARIES = libnaming_server.la

libnaming_server_la_SOURCES = naming_server.c 
libnaming_server_la_SOURCES += naming_db.c



bin_PROGRAMS = lwfs-naming-server
lwfs_naming_server_SOURCES = 
lwfs_naming_server_LDADD = 
lwfs_naming_server_LDFLAGS = 
lwfs_naming_server_CPPFLAGS = 

if HAVE_GENGETOPT
cmdline.c: $(srcdir)/local_opts.ggo $(srcdir)/naming_server_opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/local_opts.ggo \
		$(srcdir)/naming_server_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		$(top_srcdir)/src/support/threadpool/threadpool_opts.ggo \
		$(top_srcdir)/src/support/sysmon/sysmon_opts.ggo \
		$(top_srcdir)/src/client/authr_client/authr_client_opts.ggo \
		| $(GENGETOPT) -S --set-package=$(PACKAGE) \
		--set-version=$(VERSION) 

# only change this if the options have changed
$(srcdir)/cmdline_default.c: $(srcdir)/local_opts.ggo $(srcdir)/naming_server_opts.ggo
	cat $(srcdir)/local_opts.ggo \
		$(srcdir)/naming_server_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		$(top_srcdir)/src/support/threadpool/threadpool_opts.ggo \
		$(top_srcdir)/src/support/sysmon/sysmon_opts.ggo \
		$(top_srcdir)/src/client/authr_client/authr_client_opts.ggo \
		| $(GENGETOPT) -S --set-package=$(PACKAGE) \
		--set-version=$(VERSION) --output-dir=$(srcdir) -F cmdline_default

CLEANFILES += cmdline.c cmdline.h
lwfs_naming_server_SOURCES += cmdline.c

else
lwfs_naming_server_SOURCES += cmdline_default.c
endif


lwfs_naming_server_SOURCES += main.c
lwfs_naming_server_LDADD += libnaming_server.la
lwfs_naming_server_LDADD += $(top_builddir)/src/server/rpc_server/librpc_server.la
lwfs_naming_server_LDADD += $(top_builddir)/src/client/liblwfs_client.la
lwfs_naming_server_LDADD += $(top_builddir)/src/common/libcommon.la
lwfs_naming_server_LDADD += $(top_builddir)/src/support/libsupport.la
lwfs_naming_server_LDADD += $(BDB_LIBS) $(PABLO_LIBS)


CLEANFILES += naming.db naming.db.2nd naming.db.3rd naming.db.inode *~
