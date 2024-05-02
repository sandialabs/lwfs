INCLUDES =  $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

METASOURCES = AUTO


# Compile with MPICC
CCLD = --tag=CC $(MPICC)
CC=$(MPICC) --static
LIBS += $(MPILIBS)

AM_CPPFLAGS = -I${MPI_INCDIR} -DMPI -DPARANOID -O -Wall


if HAVE_MPI

bin_PROGRAMS =  lwfs-xfer-server
lwfs_xfer_server_SOURCES = xfer-xdr.c xfer-server.c
lwfs_xfer_server_LDADD = 
lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/server/liblwfs_server.la
#lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/client/liblwfs_client.la 
#lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/server/rpc_server/librpc_server.la
#lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/common/rpc_common/librpc_common.la
#lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la
#lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/client/liblwfs_client.la 
lwfs_xfer_server_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la
lwfs_xfer_server_LDADD += $(BDB_LIBS) $(OPENSSL_LIBS)

bin_PROGRAMS +=  lwfs-xfer-client
lwfs_xfer_client_SOURCES = xfer-xdr.c xfer-client.c
lwfs_xfer_client_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la 
lwfs_xfer_client_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la




#noinst_DATA = lwfs-xfer-data-blk.dat
lwfs-xfer-data-blk.dat: 
	./calc-stats.pl -o $@ -xcol 4 -ycol 8 output/lwfs-xfer-data-blk-*.out

#noinst_DATA += lwfs-xfer-data-noblk.dat
lwfs-xfer-data-noblk.dat: 
	./calc-stats.pl -o $@ -xcol 4 -ycol 8 output/lwfs-xfer-data-noblk-*.out


# Compile the client with MPICC
#xfer-client.o: xfer-client.c xfer.h xfer-xdr.c
#	$(MPICC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
#		$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -o $@ $<

#lwfs-xfer-client: xfer-client.o xfer-xdr.o
#	$(LIBTOOL) --tag=CC --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
#	$(AM_LDFLAGS) $(LDFLAGS) -o $@

xfer-xdr.o: $(srcdir)/xfer-xdr.c
	$(COMPILE) -c -Wno-unused-variable $<

xfer-xdr.lo: $(srcdir)/xfer-xdr.c
	$(LTCOMPILE) -c -Wno-unused-variable $<

$(srcdir)/xfer-xdr.c: $(srcdir)/xfer.h
	rm -f $(srcdir)/xfer-xdr.c
	rpcgen -c $(srcdir)/xfer.x > $(srcdir)/xfer-xdr.c

$(srcdir)/xfer.h: $(srcdir)/xfer.x
	rm -f $(srcdir)/xfer.h
	rpcgen -h $(srcdir)/xfer.x > $(srcdir)/xfer.h

CLEANFILES = *~ $(srcdir)/xfer-xdr.c $(srcdir)/xfer.h

endif

