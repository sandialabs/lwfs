INCLUDES  = $(all_includes)
INCLUDES += -I$(top_srcdir)/src
INCLUDES += -I$(top_builddir)/src
INCLUDES += -I$(top_builddir)

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable -D_GNU_SOURCE 
AM_LDFLAGS = $(PORTALS_LDFLAGS)
CLEANFILES = 
bin_PROGRAMS = 

bin_PROGRAMS += lwfs-nid

lwfs_nid_SOURCES = lwfs-nid.c
lwfs_nid_LDADD  = $(top_builddir)/src/common/libcommon.la 
lwfs_nid_LDADD  += $(top_builddir)/src/support/libsupport.la 
lwfs_nid_LDADD  += $(top_builddir)/src/client/liblwfs_client.la 
