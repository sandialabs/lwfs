METASOURCES = AUTO

if HAVE_LIBSYSIO

INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I/usr/include/libxml2

AM_CPPFLAGS = -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 
AM_CPPFLAGS+= -D_LARGEFILE64_SOURCE=1 -DHAVE_ASM_WEAK_DIRECTIVE=1
AM_CPPFLAGS+= $(LIBSYSIO_CPPFLAGS)

AM_CFLAGS = -I$(top_srcdir)/src
AM_CFLAGS+= -Wall -Wno-unused-variable

noinst_LTLIBRARIES = liblwfs_driver.la
liblwfs_driver_la_SOURCES = fs_lwfs.c


# Extra stuff needed to bootstrap libsysio
lib_LTLIBRARIES = liblwfs_sysio.la
liblwfs_sysio_la_SOURCES = startup.c drv_init_all.c drv_data.c 
liblwfs_sysio_la_LIBADD = liblwfs_driver.la
liblwfs_sysio_la_LIBADD += $(top_builddir)/src/support/libsupport.la
liblwfs_sysio_la_LIBADD += $(top_builddir)/src/common/libcommon.la
liblwfs_sysio_la_LIBADD += $(LIBSYSIO_LIBS)

endif
