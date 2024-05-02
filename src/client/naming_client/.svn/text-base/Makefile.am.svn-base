INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -D_GNU_SOURCE


noinst_LTLIBRARIES = libnaming_client.la

libnaming_client_la_SOURCES = naming_client.c
libnaming_client_la_SOURCES += naming_client_sync.c

libnaming_client_la_LIBADD = $(PORTALS_LIBS)

noinst_HEADERS = naming_client.h naming_client_sync.h


CLEANFILES = 
