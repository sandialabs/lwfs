# set the include path found by configure
INCLUDES= -I$(top_srcdir)/src -I$(top_srcdir) $(all_includes) 
AM_CFLAGS = -Wall -Werror

SUBDIRS = 
lwfsincludedir = $(includedir)/lwfs
nobase_lwfsinclude_HEADERS = 
EXTRA_DIST = 


#------ SUPPORT LIBRARIES ----------#
SUBDIRS += support

EXTRA_DIST += support
EXTRA_DIST += support/logger
EXTRA_DIST += support/hashtable
EXTRA_DIST += support/list
EXTRA_DIST += support/signal
EXTRA_DIST += support/threadpool
EXTRA_DIST += support/timer
EXTRA_DIST += support/trace
EXTRA_DIST += support/sysmon

nobase_lwfsinclude_HEADERS += support/list/tc.h
nobase_lwfsinclude_HEADERS += support/list/tctypes.h
nobase_lwfsinclude_HEADERS += support/list/tclist.h
nobase_lwfsinclude_HEADERS += support/stats/stats-debug.h
nobase_lwfsinclude_HEADERS += support/stats/stats.h
nobase_lwfsinclude_HEADERS += support/timer/timer.h
nobase_lwfsinclude_HEADERS += support/trace/trace.h
nobase_lwfsinclude_HEADERS += support/hashtable/mt_hashtable.h
nobase_lwfsinclude_HEADERS += support/hashtable/hashtable.h
nobase_lwfsinclude_HEADERS += support/hashtable/hash_funcs.h
nobase_lwfsinclude_HEADERS += support/logger/logger.h
nobase_lwfsinclude_HEADERS += support/logger/logger_opts.h
nobase_lwfsinclude_HEADERS += support/signal/lwfs_signal.h
nobase_lwfsinclude_HEADERS += support/threadpool/thread_pool_debug.h
nobase_lwfsinclude_HEADERS += support/threadpool/thread_pool.h
nobase_lwfsinclude_HEADERS += support/sysmon/sysmon.h

EXTRA_DIST += support/threadpool/thread_pool_opts.ggo

#------ COMMON LIBRARIES ----------#
SUBDIRS += common

EXTRA_DIST += common
EXTRA_DIST += common/types
EXTRA_DIST += common/authr_common
EXTRA_DIST += common/naming_common
EXTRA_DIST += common/rpc_common
EXTRA_DIST += common/storage_common

nobase_lwfsinclude_HEADERS += common/naming_common/naming_args.h
nobase_lwfsinclude_HEADERS += common/naming_common/naming_xdr.h
nobase_lwfsinclude_HEADERS += common/naming_common/naming_debug.h
nobase_lwfsinclude_HEADERS += common/naming_common/naming_opcodes.h
nobase_lwfsinclude_HEADERS += common/authr_common/authr_trace.h
nobase_lwfsinclude_HEADERS += common/authr_common/authr_args.h
nobase_lwfsinclude_HEADERS += common/authr_common/authr_debug.h
nobase_lwfsinclude_HEADERS += common/authr_common/authr_xdr.h
nobase_lwfsinclude_HEADERS += common/authr_common/authr_opcodes.h
nobase_lwfsinclude_HEADERS += common/types/fprint_types.h
nobase_lwfsinclude_HEADERS += common/types/types.h
nobase_lwfsinclude_HEADERS += common/types/xdr_types.h
nobase_lwfsinclude_HEADERS += common/types/lwfs_int.h
nobase_lwfsinclude_HEADERS += common/config_parser/config_parser.h
nobase_lwfsinclude_HEADERS += common/rpc_common/lwfs_ptls.h
nobase_lwfsinclude_HEADERS += common/rpc_common/rpc_debug.h
nobase_lwfsinclude_HEADERS += common/rpc_common/ptl_wrap.h
nobase_lwfsinclude_HEADERS += common/rpc_common/rpc_common.h
nobase_lwfsinclude_HEADERS += common/rpc_common/rpc_xdr.h
nobase_lwfsinclude_HEADERS += common/storage_common/ss_debug.h
nobase_lwfsinclude_HEADERS += common/storage_common/ss_xdr.h
nobase_lwfsinclude_HEADERS += common/storage_common/ss_args.h
nobase_lwfsinclude_HEADERS += common/storage_common/ss_trace.h
nobase_lwfsinclude_HEADERS += common/storage_common/ss_opcodes.h


#------ CLIENT LIBRARIES ----------#
if BUILD_CLIENT
SUBDIRS += client

EXTRA_DIST += client
EXTRA_DIST += client/rpc_client
EXTRA_DIST += client/authr_client
EXTRA_DIST += client/storage_client
EXTRA_DIST += client/naming_client

nobase_lwfsinclude_HEADERS += client/rpc_client/rpc_client.h
nobase_lwfsinclude_HEADERS += client/authr_client/authr_client_sync.h
nobase_lwfsinclude_HEADERS += client/authr_client/authr_client_opts.ggo
nobase_lwfsinclude_HEADERS += client/authr_client/authr_client.h
nobase_lwfsinclude_HEADERS += client/storage_client/storage_client.h
nobase_lwfsinclude_HEADERS += client/storage_client/storage_client_sync.h
nobase_lwfsinclude_HEADERS += client/storage_client/storage_client_opts.h
nobase_lwfsinclude_HEADERS += client/naming_client/naming_client_sync.h
nobase_lwfsinclude_HEADERS += client/naming_client/naming_client.h

EXTRA_DIST += client/authr_client/authr_client_opts.ggo
EXTRA_DIST += client/naming_client/naming_client_opts.ggo
EXTRA_DIST += client/storage_client/storage_client_opts.ggo

endif

#------ SERVER LIBRARIES ----------#
if BUILD_SERVER
SUBDIRS += server

EXTRA_DIST += server
nobase_lwfsinclude_HEADERS += server/naming_server/naming_server.h
nobase_lwfsinclude_HEADERS += server/authr_server/authr_server.h
nobase_lwfsinclude_HEADERS += server/rpc_server/rpc_server.h
nobase_lwfsinclude_HEADERS += server/storage_server/storage_server.h
endif

#------ EXTRA PROGRAMS ----------#
if BUILD_SERVER
SUBDIRS += progs
endif

METASOURCES = AUTO

