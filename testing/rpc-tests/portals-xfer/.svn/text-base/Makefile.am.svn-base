# portals libraries should be included by configure

INCLUDES = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

METASOURCES = AUTO

#AM_CPPFLAGS = -DP3_NAL=\<p3nal\_utcp\.h\> -Wall

AM_CFLAGS = $(PORTALS_CFLAGS)
AM_CPPFLAGS = $(PORTALS_CPPFLAGS)

#CCLD = --tag=CC $(MPICC)
#CC=$(MPICC) --static
#LIBS += $(MPILIBS)

noinst_PROGRAMS = portals-xfer

portals_xfer_SOURCES = portals-xfer-opts.c portals-xfer.c
portals_xfer_LDFLAGS = $(PORTALS_LDFLAGS)
portals_xfer_LDADD = -lrt -lm $(PORTALS_LIBS) 
portals_xfer_LDADD += $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
portals_xfer_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la

if HAVE_GENGETOPT
$(srcdir)/portals-xfer-opts.c: $(srcdir)/portals-xfer-opts.ggo
	$(GENGETOPT) -F $* -S --set-package=$(PACKAGE) \
	    --set-version=$(VERSION) < $*.ggo
endif

