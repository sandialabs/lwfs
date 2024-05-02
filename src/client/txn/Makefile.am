INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -D_GNU_SOURCE

noinst_LTLIBRARIES = libtxn.la

libtxn_la_SOURCES = locks.c journal.c

libtxn_la_LIBADD = $(PORTALS_LIBS)

CLEANFILES =
