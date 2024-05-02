INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -D_GNU_SOURCE


noinst_LTLIBRARIES = libstorage_client.la

libstorage_client_la_SOURCES = storage_client.c storage_client_sync.c
libstorage_client_la_CPPFLAGS = $(PORTALS_CPPFLAGS)

noinst_HEADERS = storage_client.h storage_client_sync.h


CLEANFILES = 
