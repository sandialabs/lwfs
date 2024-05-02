INCLUDES = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src 

METASOURCES = AUTO

AM_CPPFLAGS = -Wall -Wno-unused-variable -Wno-strict-aliasing $(CLIENT_CPPFLAGS)
AM_LDFLAGS = 

noinst_PROGRAMS = stdio_reader \
	       stdio_writer \
	       xdrmem_writer 

stdio_reader_SOURCES = test_xdr.c 
stdio_reader_SOURCES += stdio_reader.c
stdio_reader_SOURCES += xdr_tests_common.c

stdio_reader_LDADD  =  $(LWFS_BUILDDIR)/src/client/liblwfs_client.la



stdio_writer_SOURCES = test_xdr.c
stdio_writer_SOURCES += stdio_writer.c
stdio_writer_SOURCES += xdr_tests_common.c

stdio_writer_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la


xdrmem_writer_SOURCES = test_xdr.c
xdrmem_writer_SOURCES += xdrmem_writer.c
xdrmem_writer_SOURCES += xdr_tests_common.c
xdrmem_writer_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la


noinst_HEADERS = test_xdr.h xdr_tests_common.h

test_xdr.o: test_xdr.c
	$(COMPILE) -c -Wno-unused-variable $<

test_xdr.lo: test_xdr.h
	$(LTCOMPILE) -c -Wno-unused-variable $<

$(srcdir)/test_xdr.h: $(srcdir)/test_xdr.x
	rm -f $(srcdir)/test_xdr.h
	rpcgen -h $(srcdir)/test_xdr.x > $(srcdir)/test_xdr.h

$(srcdir)/test_xdr.c: $(srcdir)/test_xdr.h 
	rm -f $(srcdir)/test_xdr.c
	rpcgen -c $(srcdir)/test_xdr.x > $(srcdir)/test_xdr.c


CLEANFILES= $(srcdir)/test_xdr.h $(srcdir)/test_xdr.c core.* *~

