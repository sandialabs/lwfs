METASOURCES = AUTO

INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I$(top_srcdir)/../libsysio/include

AM_CPPFLAGS = -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 
AM_CPPFLAGS+= -D_LARGEFILE64_SOURCE=1 -DHAVE_ASM_WEAK_DIRECTIVE=1

AM_CFLAGS = -I$(top_srcdir)/../libsysio/include
AM_CFLAGS+= -Wall -Wno-unused-variable

noinst_LTLIBRARIES = libnative_driver.la

libnative_driver_la_SOURCES = fs_native.c

