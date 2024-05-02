# set the include path found by configure
INCLUDES= -I$(top_srcdir)/src  $(all_includes) 

SUBDIRS  = rpc_server
SUBDIRS += authr_server
SUBDIRS += storage_server
SUBDIRS += naming_server

METASOURCES = AUTO

if NEED_DARWIN_SINGLE_MODULE
AM_LDFLAGS = -Wl,-single_module
endif

lib_LTLIBRARIES = liblwfs_server.la

liblwfs_server_la_SOURCES = 
liblwfs_server_la_LDFLAGS = $(PORTALS_LDFLAGS)
liblwfs_server_la_LIBADD = rpc_server/librpc_server.la
liblwfs_server_la_LIBADD += storage_server/libstorage_server.la
liblwfs_server_la_LIBADD += authr_server/libauthr_server.la
liblwfs_server_la_LIBADD += naming_server/libnaming_server.la
liblwfs_server_la_LIBADD += $(top_builddir)/src/common/libcommon.la 
liblwfs_server_la_LIBADD += $(top_builddir)/src/support/libsupport.la 
liblwfs_server_la_LIBADD += $(top_builddir)/src/client/liblwfs_client.la 
