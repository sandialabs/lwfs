INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

bin_PROGRAMS = 
noinst_LTLIBRARIES = 

#noinst_LTLIBRARIES += libpablo_interface.la
#libpablo_interface_la_SOURCES = pablo_interface.C
#libpablo_interface_la_LDFLAGS = $(PABLO_LDFLAGS)
#libpablo_interface_la_LIBADD = $(PABLO_LIBS)

noinst_LTLIBRARIES += libtrace.la
libtrace_la_SOURCES = trace.cc
if HAVE_PABLO
libtrace_la_SOURCES += pablo_interface.C
libtrace_la_LDFLAGS = $(PABLO_LDFLAGS)
libtrace_la_LIBADD = $(PABLO_LIBS)
else
libtrace_la_SOURCES += pablo_empty_interface.c
endif

#libtrace_la_LIBADD += $(top_builddir)/src/support/timer/libtimer.la
#libtrace_la_LIBADD += $(top_builddir)/src/support/hashtable/libhashtable.la
#if HAVE_PTHREAD
#libtrace_la_LIBADD += $(top_builddir)/src/support/hashtable/libmt_hashtable.la
#endif
#endif

CLEANFILES = *~ core.*

