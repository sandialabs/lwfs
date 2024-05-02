INCLUDES =  $(all_includes)
INCLUDES +=  -I$(LWFS_SRCDIR)/src

METASOURCES = AUTO

AM_CFLAGS = -Wall -g

noinst_PROGRAMS =  tcp-xfer 

noinst_HEADERS = 

tcp_xfer_SOURCES = tcp-xfer.c
tcp_xfer_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
tcp_xfer_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la

#CLEANFILES = 
