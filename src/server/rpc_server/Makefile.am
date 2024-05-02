INCLUDES =  -I$(top_srcdir)
INCLUDES += -I$(top_srcdir)/support
INCLUDES += -I$(top_srcdir)/src
INCLUDES += $(all_includes)

METASOURCES = AUTO

AM_LIBS = $(SERVER_LIBS)
AM_CFLAGS = $(SERVER_CFLAGS)
AM_CPPFLAGS = -Wall -Wno-unused-variable -Wno-cast-align -D_GNU_SOURCE $(SERVER_CPPFLAGS)

noinst_LTLIBRARIES = librpc_server.la

librpc_server_la_SOURCES = rpc_server.c
librpc_server_la_LIBADD = $(PORTALS_LIBS)

noinst_HEADERS = rpc_server.h rpc_trace.h

CLEANFILES = 
