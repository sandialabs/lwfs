METASOURCES = AUTO

INCLUDES  = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

AM_CPPFLAGS = -DSTDC_HEADERS=1 -DTIME_WITH_SYS_TIME=1 
AM_CPPFLAGS+= -D_LARGEFILE64_SOURCE=1 -DHAVE_ASM_WEAK_DIRECTIVE=1
AM_CPPFLAGS+= -D_POSIX_C_SOURCE=199506L -D_XOPEN_SOURCE=600 
AM_CPPFLAGS+= -DSYSIO_TRACING=1 -DAUTOMOUNT_FILE_NAME=\".mount\" 
AM_CPPFLAGS+= -DZERO_SUM_MEMORY=1 -DSTDFD_DEV=1 
AM_CPPFLAGS+= -DWITH_SOCKETS=1 -DDEFER_INIT_CWD=0 
#AM_CPPFLAGS+= -DSYSIO_LABEL_NAMES="sysio_"
AM_CPPFLAGS+= -DSYSIO_LABEL_NAMES=""
#AM_CPPFLAGS+= -DSYSIO_TRACING=""

AM_CFLAGS = -I$(LWFS_SRCDIR)/src
AM_CFLAGS+= -Wall -Wno-unused-variable

# We have to do this to force the linker to link in the 
# premain/postmain code in the lwfs_sysio library. There 
# must be a better way to do this.
AM_LDFLAGS = -u _lwfs_premain

bin_PROGRAMS  = lwfs_copy
bin_PROGRAMS += lwfs_getcwd
bin_PROGRAMS += lwfs_link
bin_PROGRAMS += lwfs_list
bin_PROGRAMS += lwfs_mkdir
bin_PROGRAMS += lwfs_path
bin_PROGRAMS += lwfs_rename
bin_PROGRAMS += lwfs_rmdir
bin_PROGRAMS += lwfs_stats 
bin_PROGRAMS += lwfs_stddir
bin_PROGRAMS += lwfs_unlink
bin_PROGRAMS += lwfs_read
bin_PROGRAMS += lwfs_write


#
# libs needed to link the test programs
#
lwfs_libs = 
lwfs_libs += $(top_builddir)/src/client/sysio_client/liblwfs_sysio.la
lwfs_libs += $(top_builddir)/src/client/liblwfs_client.la
#lwfs_libs += $(LWFS_BUILDDIR)/../software/src/libsysio/lib/libsysio.a -lstdarg


lwfs_copy_SOURCES = lwfs_copy.c $(CMNSRC)
lwfs_copy_LDADD = $(lwfs_libs)

lwfs_getcwd_SOURCES = lwfs_getcwd.c $(CMNSRC)
lwfs_getcwd_LDADD = $(lwfs_libs)

lwfs_link_SOURCES = lwfs_link.c $(CMNSRC)
lwfs_link_LDADD = $(lwfs_libs)

lwfs_list_SOURCES = lwfs_list.c $(CMNSRC)
lwfs_list_LDADD = $(lwfs_libs)

lwfs_mkdir_SOURCES = lwfs_mkdir.c $(CMNSRC)
lwfs_mkdir_LDADD = $(lwfs_libs)

lwfs_path_SOURCES = lwfs_path.c $(CMNSRC)
lwfs_path_LDADD = $(lwfs_libs)

lwfs_rename_SOURCES = lwfs_rename.c $(CMNSRC)
lwfs_rename_LDADD = $(lwfs_libs)

lwfs_rmdir_SOURCES = lwfs_rmdir.c $(CMNSRC)
lwfs_rmdir_LDADD = $(lwfs_libs)

lwfs_stats_SOURCES = lwfs_stats.c $(CMNSRC)
lwfs_stats_LDADD = $(lwfs_libs)

lwfs_stddir_SOURCES = lwfs_stddir.c $(CMNSRC)
lwfs_stddir_LDADD = $(lwfs_libs)

lwfs_unlink_SOURCES = lwfs_unlink.c $(CMNSRC)
lwfs_unlink_LDADD = $(lwfs_libs)

lwfs_read_SOURCES = lwfs_read.c $(CMNSRC)
lwfs_read_LDADD = $(lwfs_libs)

lwfs_write_SOURCES = lwfs_write.c $(CMNSRC)
lwfs_write_LDADD = $(lwfs_libs)

CLEANFILES = $(bin_PROGRAMS)
