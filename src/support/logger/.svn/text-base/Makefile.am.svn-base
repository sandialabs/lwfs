INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS=-D_GNU_SOURCE


noinst_LTLIBRARIES =  liblogger.la
liblogger_la_SOURCES = logger.c
liblogger_la_DEPENDENCIES = logger.h

noinst_PROGRAMS = logger_test

logger_test_SOURCES = cmdline.c logger_test.c
logger_test_LDADD = liblogger.la

TESTS = logger_test

if HAVE_GENGETOPT
cmdline.c: logger_opts.ggo logger_opts.h
	$(GENGETOPT) -C -S -i logger_opts.ggo
endif	

#include_HEADERS =  logger.h

CLEANFILES = *~ core.* 
noinst_HEADERS =  logger.h
