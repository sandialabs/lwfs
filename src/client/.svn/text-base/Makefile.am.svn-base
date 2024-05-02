# set the include path found by configure
INCLUDES= -I$(top_srcdir)/src  $(all_includes) 

METASOURCES = AUTO

SUBDIRS  = rpc_client
SUBDIRS += authr_client
SUBDIRS += storage_client
SUBDIRS += naming_client
SUBDIRS += txn
SUBDIRS += sysio_client

if NEED_DARWIN_SINGLE_MODULE
AM_LDFLAGS = -Wl,-single_module
endif

lib_LTLIBRARIES = liblwfs_client.la
liblwfs_client_la_LDFLAGS = $(PORTALS_LDFLAGS)

liblwfs_client_la_SOURCES = 
liblwfs_client_la_LIBADD =  naming_client/libnaming_client.la 
liblwfs_client_la_LIBADD += authr_client/libauthr_client.la 
liblwfs_client_la_LIBADD += rpc_client/librpc_client.la 
liblwfs_client_la_LIBADD += storage_client/libstorage_client.la 
liblwfs_client_la_LIBADD += txn/libtxn.la
liblwfs_client_la_LIBADD += $(top_builddir)/src/common/libcommon.la 
liblwfs_client_la_LIBADD += $(top_builddir)/src/support/libsupport.la 
