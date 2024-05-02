METASOURCES = AUTO

INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

AM_CPPFLAGS = 

AM_CFLAGS = -I$(top_srcdir)/src

noinst_LTLIBRARIES = libconfig_parser.la
libconfig_parser_la_SOURCES = config_parser.c
#libconfig_parser_la_LIBADD = $(top_builddir)/src/common/types/libtypes.la
#libconfig_parser_la_LIBADD += $(top_builddir)/src/support/logger/liblogger.la
#libconfig_parser_la_LIBADD += $(top_builddir)/src/support/ezxml/libezxml.la

noinst_PROGRAMS = parse_test
parse_test_SOURCES = parse_test.c
parse_test_LDADD = libconfig_parser.la
parse_test_LDADD += $(top_builddir)/src/common/types/libtypes.la
parse_test_LDADD += $(top_builddir)/src/support/logger/liblogger.la
parse_test_LDADD += $(top_builddir)/src/support/ezxml/libezxml.la
