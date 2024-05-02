METASOURCES = AUTO

INCLUDES  = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

if HAVE_LIBSYSIO

AM_CPPFLAGS = -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 
AM_CPPFLAGS+= -D_LARGEFILE64_SOURCE=1 -DHAVE_ASM_WEAK_DIRECTIVE=1
AM_CPPFLAGS+= -D_POSIX_C_SOURCE=199506L -D_XOPEN_SOURCE=600 
AM_CPPFLAGS+= -DSYSIO_TRACING=1 -DAUTOMOUNT_FILE_NAME=\".mount\" 
AM_CPPFLAGS+= -DZERO_SUM_MEMORY=1 -DSTDFD_DEV=1 
AM_CPPFLAGS+= -DWITH_SOCKETS=1 -DDEFER_INIT_CWD=0 
#AM_CPPFLAGS+= -DSYSIO_LABEL_NAMES="sysio_"
AM_CPPFLAGS+= -DSYSIO_LABEL_NAMES=""
#AM_CPPFLAGS+= -DSYSIO_TRACING=""
AM_CPPFLAGS+= $(LIBSYSIO_CPPFLAGS)

AM_CFLAGS = -I$(LWFS_SRCDIR)/src
AM_CFLAGS+= -Wall -Wno-unused-variable

# We have to do this to force the linker to link in the 
# premain/postmain code in the lwfs_sysio library. There 
# must be a better way to do this.
AM_LDFLAGS = -u _lwfs_premain

noinst_PROGRAMS  = test_copy
noinst_PROGRAMS += test_getcwd
noinst_PROGRAMS += test_link
noinst_PROGRAMS += test_list
noinst_PROGRAMS += test_mkdir
noinst_PROGRAMS += test_path
noinst_PROGRAMS += test_regions
noinst_PROGRAMS += test_rename
noinst_PROGRAMS += test_rmdir
noinst_PROGRAMS += test_stats 
noinst_PROGRAMS += test_stddir
noinst_PROGRAMS += test_unlink
#noinst_PROGRAMS += test_read
#noinst_PROGRAMS += test_write


#
# libs needed to link the test programs
#
test_libs = 
test_libs += $(LWFS_BUILDDIR)/src/client/sysio_client/liblwfs_sysio.la
test_libs += $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
#test_libs += $(LWFS_BUILDDIR)/../software/src/libsysio/lib/libsysio.a -lstdarg
test_libs += -L/home/thkorde/projects/lwfs/install/cnos/lib 


test_copy_SOURCES = test_copy.c $(CMNSRC)
test_copy_LDADD = $(test_libs)

test_getcwd_SOURCES = test_getcwd.c $(CMNSRC)
test_getcwd_LDADD = $(test_libs)

test_link_SOURCES = test_link.c $(CMNSRC)
test_link_LDADD = $(test_libs)

test_list_SOURCES = test_list.c $(CMNSRC)
test_list_LDADD = $(test_libs)
test_list_LDFLAGS = -u _lwfs_premain

test_mkdir_SOURCES = test_mkdir.c $(CMNSRC)
test_mkdir_LDADD = $(test_libs)

test_path_SOURCES = test_path.c $(CMNSRC)
test_path_LDADD = $(test_libs)

test_regions_SOURCES = test_regions.c $(CMNSRC)
test_regions_LDADD = $(test_libs)

test_rename_SOURCES = test_rename.c $(CMNSRC)
test_rename_LDADD = $(test_libs)

test_rmdir_SOURCES = test_rmdir.c $(CMNSRC)
test_rmdir_LDADD = $(test_libs)

test_stats_SOURCES = test_stats.c $(CMNSRC)
test_stats_LDADD = $(test_libs)

test_stddir_SOURCES = test_stddir.c $(CMNSRC)
test_stddir_LDADD = $(test_libs)

test_unlink_SOURCES = test_unlink.c $(CMNSRC)
test_unlink_LDADD = $(test_libs)

#test_read_SOURCES = test_read.c $(CMNSRC)
#test_read_LDADD = $(test_libs)

#test_write_SOURCES = test_write.c $(CMNSRC)
#test_write_LDADD = $(test_libs)

CLEANFILES = $(noinst_PROGRAMS)

endif
