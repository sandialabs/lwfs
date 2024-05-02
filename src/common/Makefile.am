# set the include path found by configure
INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

SUBDIRS = types
SUBDIRS += rpc_common
SUBDIRS += authr_common
SUBDIRS += storage_common
SUBDIRS += naming_common
SUBDIRS += config_parser

METASOURCES = AUTO

noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = 

libcommon_la_LIBADD  = authr_common/libauthr_common.la
libcommon_la_LIBADD += naming_common/libnaming_common.la
libcommon_la_LIBADD += rpc_common/librpc_common.la
libcommon_la_LIBADD += storage_common/libstorage_common.la
libcommon_la_LIBADD += types/libtypes.la
libcommon_la_LIBADD += config_parser/libconfig_parser.la


