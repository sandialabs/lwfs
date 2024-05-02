INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

#METASOURCES = AUTO

AM_CPPFLAGS="-Wall"

noinst_LTLIBRARIES =

noinst_LTLIBRARIES +=  libhashtable.la
libhashtable_la_CPPFLAGS = 
libhashtable_la_SOURCES = hash_funcs.c
libhashtable_la_SOURCES += hashtable_itr.c
libhashtable_la_SOURCES += hashtable.c
libhashtable_la_LIBADD = -lm

noinst_PROGRAMS = hashtable_tester
hashtable_tester_SOURCES = hashtable_tester.c
hashtable_tester_LDADD = libhashtable.la 


if HAVE_PTHREAD

noinst_LTLIBRARIES += libmt_hashtable.la
libmt_hashtable_la_SOURCES = mt_hashtable.c
libmt_hashtable_la_LIBADD = -lpthread


noinst_PROGRAMS += mt_hashtable_tester
mt_hashtable_tester_SOURCES = mt_hashtable_tester.c
mt_hashtable_tester_LDADD  = libmt_hashtable.la
mt_hashtable_tester_LDADD += libhashtable.la

endif

noinst_PROGRAMS += hash_test
hash_test_SOURCES = hash_test.c
hash_test_LDADD = libhashtable.la

CLEANFILES=core.* *~  *.loT
