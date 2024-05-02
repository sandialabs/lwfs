INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/support 
INCLUDES += -I$(top_srcdir)/src 
INCLUDES += -I$(top_srcdir)/src/include 

METASOURCES = AUTO
noinst_LTLIBRARIES =  libsecurity.la

noinst_PROGRAMS =  security_test

libsecurity_la_SOURCES = security.c 

security_test_SOURCES = security_test.c
security_test_LDADD = libsecurity.la
security_test_LDADD += $(top_builddir)/support/logger/liblogger.la
security_test_LDADD += $(top_builddir)/src/include/lwfs/libtypes.la
