INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)/src

METASOURCES = AUTO

AM_CPPFLAGS=-Wall -D_GNU_SOURCE

if HAVE_PTHREAD

noinst_LTLIBRARIES =  libthreadpool.la
libthreadpool_la_SOURCES = handler_thread.c
libthreadpool_la_SOURCES += handler_threads_pool.c
libthreadpool_la_SOURCES += requests_queue.c
libthreadpool_la_SOURCES += thread_pool.c
libthreadpool_la_SOURCES += thread_pool_options.c
libthreadpool_la_SOURCES += thread_pool_debug.c
libthreadpool_la_LIBADD = -lpthread

noinst_PROGRAMS = thread_pool_server
thread_pool_server_SOURCES = thread_pool_server.c
thread_pool_server_LDFLAGS = $(PABLO_LDFLAGS)
thread_pool_server_LDADD = libthreadpool.la
thread_pool_server_LDADD += $(top_builddir)/src/support/logger/liblogger.la
thread_pool_server_LDADD += $(top_builddir)/src/support/signal/libsignal.la
thread_pool_server_LDADD += $(top_builddir)/src/support/trace/libtrace.la
thread_pool_server_LDADD += $(top_builddir)/src/support/timer/libtimer.la
thread_pool_server_LDADD += $(top_builddir)/src/support/hashtable/libmt_hashtable.la
thread_pool_server_LDADD += $(top_builddir)/src/support/hashtable/libhashtable.la
thread_pool_server_LDADD += $(PABLO_LIBS)


endif

CLEANFILES=core.* *~
