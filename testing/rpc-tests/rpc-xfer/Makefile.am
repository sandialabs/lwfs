INCLUDES =  $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

METASOURCES = AUTO

noinst_PROGRAMS =  rpc-xfer rpc-xfer-server

noinst_HEADERS = 

rpc_xfer_SOURCES = xfer_xdr.c xfer_client.c xfer_clnt.c
rpc_xfer_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
rpc_xfer_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la

rpc_xfer_server_SOURCES = xfer_server.c xfer_svc.c xfer_xdr.c

xfer_xdr.lo: xfer_xdr.c
	$(LTCOMPILE) -c -Wno-unused-variable $<

xfer_xdr.o: xfer_xdr.c
	$(COMPILE) -c -Wno-unused-variable $<

$(srcdir)/xfer_xdr.c: $(srcdir)/xfer.h
	rm -f $(srcdir)/xfer_xdr.c
	rpcgen -c $(srcdir)/xfer.x -o $(srcdir)/xfer_xdr.c 

$(srcdir)/xfer.h: $(srcdir)/xfer.x
	rm -f $(srcdir)/xfer.h
	rpcgen -h $(srcdir)/xfer.x -o $(srcdir)/xfer.h

CLEANFILES = $(srcdir)/xfer.h $(srcdir)/xfer_xdr.c 
