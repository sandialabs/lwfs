INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -D_GNU_SOURCE


noinst_LTLIBRARIES = libauthr_client.la

libauthr_client_la_SOURCES = authr_client.c
libauthr_client_la_SOURCES += authr_client_sync.c

libauthr_client_la_LIBADD = $(PORTALS_LIBS)

CLEANFILES = 
