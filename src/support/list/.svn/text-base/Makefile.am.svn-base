INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src 

METASOURCES = AUTO

AM_CPPFLAGS="-Wall"

noinst_LTLIBRARIES = liblist.la
liblist_la_SOURCES = list.c

liblist_la_LIBADD = -lm


#noinst_PROGRAMS = hashtable_tester
#hashtable_tester_SOURCES = hashtable_tester.c
#hashtable_tester_LDADD = libhashtable.la


CLEANFILES=core.* *~  
