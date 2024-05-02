AM_CFLAGS = -Wall

INCLUDES = $(all_includes)
INCLUDES += -I$(top_srcdir)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I$(top_srcdir)/support
INCLUDES += -I/usr/tmp/raoldfi/include
INCLUDES += -I/home/raoldfi/software/include

AM_CPPFLAGS = -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=22

bin_PROGRAMS =  fuse-lwfs 
noinst_PROGRAMS = null hello fusexmp

null_SOURCES = null.c 
null_LDADD   = -L/usr/tmp/raoldfi/lib
null_LDADD  += -L/usr/tmp/raoldfi/lib -lfuse -lpthread

hello_SOURCES = hello.c 
hello_LDADD   = -L/usr/tmp/raoldfi/lib 
hello_LDADD  += -lfuse -lpthread

fusexmp_SOURCES =  fusexmp.c
fusexmp_LDADD   = -L/usr/tmp/raoldfi/lib
fusexmp_LDADD  += -lfuse -lpthread

fuse_lwfs_SOURCES =  fuse-lwfs.c fuse-callbacks.c fuse-debug.c
fuse_lwfs_LDADD   = $(top_builddir)/src/server/liblwfs_server.la 
fuse_lwfs_LDADD  += -L/usr/tmp/raoldfi/lib 
fuse_lwfs_LDADD  += -L/home/raoldfi/software/fuse
fuse_lwfs_LDADD  += -lfuse -lpthread

CLEANFILES=core.* *~ ss_root_dir *.db.* *.db

