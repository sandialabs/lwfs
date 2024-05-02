INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable -D_GNU_SOURCE

noinst_LTLIBRARIES = librpc_client.la

librpc_client_la_SOURCES = 
librpc_client_la_SOURCES += rpc_client.c

librpc_client_la_LIBADD = $(PORTALS_LIBS)

noinst_HEADERS = rpc_client.h


CLEANFILES = 
