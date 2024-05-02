INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

noinst_LTLIBRARIES =  libsysmon.la

libsysmon_la_SOURCES = sysmon.c meminfo.c

noinst_HEADERS = sysmon.h

if HAVE_GENGETOPT
cmdline.c: sysmon_opts.ggo sysmon_opts.h
	$(GENGETOPT) -C -S -i sysmon_opts.ggo
endif	

CLEANFILES = *~ core.*
