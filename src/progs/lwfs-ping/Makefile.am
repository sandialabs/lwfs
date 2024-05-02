INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I$(top_builddir)/src
INCLUDES += -I$(top_builddir)

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable -D_GNU_SOURCE 
AM_LDFLAGS = $(PORTALS_LDFLAGS)
CLEANFILES = 
bin_PROGRAMS = 

bin_PROGRAMS += lwfs-ping

lwfs_ping_SOURCES = ping_opts.c lwfs-ping.c
lwfs_ping_LDADD  = $(top_builddir)/src/common/libcommon.la 
lwfs_ping_LDADD  += $(top_builddir)/src/support/libsupport.la 
lwfs_ping_LDADD  += $(top_builddir)/src/client/liblwfs_client.la 

if HAVE_GENGETOPT
ping_opts.c: $(srcdir)/service_opts.ggo
	cat $(srcdir)/service_opts.ggo \
		$(top_srcdir)/src/support/logger/logger_opts.ggo \
		| $(GENGETOPT) -S --set-package=$(PACKAGE) \
		--set-version=$(VERSION) -F ping_opts

CLEANFILES += ping_opts.c ping_opts.h
endif
