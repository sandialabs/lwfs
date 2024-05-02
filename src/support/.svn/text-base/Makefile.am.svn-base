# set the include path found by configure
INCLUDES= -I$(top_srcdir)/src -I$(top_srcdir) $(all_includes) 

SUBDIRS = hashtable list timer logger trace signal threadpool stats ezxml sysmon ptl_uuid

METASOURCES = AUTO

noinst_LTLIBRARIES = libsupport.la

libsupport_la_SOURCES = 

libsupport_la_LIBADD  = hashtable/libhashtable.la
libsupport_la_LIBADD += list/liblist.la
libsupport_la_LIBADD += timer/libtimer.la
libsupport_la_LIBADD += logger/liblogger.la
#libsupport_la_LIBADD += security/libsecurity.la
libsupport_la_LIBADD += signal/libsignal.la
libsupport_la_LIBADD += stats/libstats.la
libsupport_la_LIBADD += ezxml/libezxml.la
libsupport_la_LIBADD += trace/libtrace.la
libsupport_la_LIBADD += sysmon/libsysmon.la
libsupport_la_LIBADD += ptl_uuid/libptl_uuid.la

if HAVE_PTHREAD
libsupport_la_LIBADD += hashtable/libmt_hashtable.la
libsupport_la_LIBADD += threadpool/libthreadpool.la
endif
