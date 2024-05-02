INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

bin_PROGRAMS = 


noinst_LTLIBRARIES =  libtimer.la

libtimer_la_SOURCES = timer.c
libtimer_la_LIBADD = @RTLIBADD@

noinst_HEADERS = timer.h

CLEANFILES = *~ core.*

