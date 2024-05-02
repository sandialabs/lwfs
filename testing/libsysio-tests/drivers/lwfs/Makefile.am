METASOURCES = AUTO

INCLUDES  = $(all_includes)
INCLUDES += -I$(LWFS_ROOT)/src
INCLUDES += -I/usr/include/libxml2

if HAVE_LIBSYSIO_RULES
AM_CFLAGS = $(LIBSYSIO_CFLAGS)
AM_CPPFLAGS = $(LIBSYSIO_CPPFLAGS)
AM_LDFLAGS = $(LIBSYSIO_LDFLAGS)
else
# We have to guess
AM_CPPFLAGS = -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 
AM_CPPFLAGS+= -D_LARGEFILE64_SOURCE=1 -DHAVE_ASM_WEAK_DIRECTIVE=1
endif


AM_CFLAGS = -I$(LWFS_ROOT)/src
AM_CFLAGS+= -Wall -Wno-unused-variable

noinst_LTLIBRARIES = liblwfs_driver.la
liblwfs_driver_la_SOURCES = fs_lwfs.c
liblwfs_driver_la_SOURCES += config_parser.c
liblwfs_driver_la_LIBADD = $(LWFS_ROOT)/src/client/liblwfs_client.la

