INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I$(top_builddir)/src
INCLUDES += -I$(top_builddir)

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable -D_GNU_SOURCE 
AM_LDFLAGS = $(PORTALS_LDFLAGS)
CLEANFILES = 
bin_PROGRAMS = 

bin_PROGRAMS += lwfs-kill

lwfs_kill_SOURCES = kill_opts.c lwfs-kill.c
lwfs_kill_LDADD  = $(top_builddir)/src/common/libcommon.la 
lwfs_kill_LDADD  += $(top_builddir)/src/support/libsupport.la 
lwfs_kill_LDADD  += $(top_builddir)/src/client/liblwfs_client.la 

if HAVE_GENGETOPT
kill_opts.c: $(srcdir)/service_opts.ggo
	cat $(srcdir)/service_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		| $(GENGETOPT) -S --set-package=$(PACKAGE) \
		--set-version=$(VERSION) -F kill_opts

CLEANFILES += kill_opts.c kill_opts.h
endif





bin_PROGRAMS += lwfs-tracing

lwfs_tracing_SOURCES = trace_opts.c lwfs-tracing.c
lwfs_tracing_LDADD  = $(top_builddir)/src/common/libcommon.la 
lwfs_tracing_LDADD  += $(top_builddir)/src/support/libsupport.la 
lwfs_tracing_LDADD  += $(top_builddir)/src/client/liblwfs_client.la 

if HAVE_GENGETOPT
trace_opts.c: $(srcdir)/service_opts.ggo $(srcdir)/trace_opts.ggo
	cat $(srcdir)/service_opts.ggo $(srcdir)/trace_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		| $(GENGETOPT) -S --set-package=$(PACKAGE) \
		--set-version=$(VERSION) -F trace_opts

CLEANFILES += trace_opts.c trace_opts.h
endif

