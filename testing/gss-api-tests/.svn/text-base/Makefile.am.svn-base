
INCLUDES = -I$(top_srcdir)/src $(all_includes) 

METASOURCES = AUTO

if HAVE_GSSAPI

noinst_PROGRAMS =  gss-server gss-client 
#bin_PROGRAMS += gss-server-ptls gss-client-ptls

gss_server_SOURCES =  gss-server.c gss-misc.c gss-misc.h
gss_server_LDADD   =  -lkrb5 -lgssapi_krb5

gss_client_SOURCES =  gss-client.c gss-misc.c gss-misc.h
gss_client_LDADD   =  -lkrb5 -lgssapi_krb5

#gss_server_ptls_SOURCES =  gss-xdr.c \
#			   gss-server-ptls.c \
#			   gss-misc-ptls.c \
#			   gss-misc-ptls.h 
#gss_server_ptls_LDADD    =  $(top_builddir)/src/liblwfs.la 

#gss_client_ptls_SOURCES =  gss-client-ptls.c \
#			   gss-misc-ptls.c \
#			   gss-misc-ptls.h \
#			   gss-xdr.c 
#gss_client_ptls_LDADD   = $(top_builddir)/src/liblwfs.la 

gss-xdr.h: gss-xdr.x
	rm -f gss-xdr.h
	rpcgen -h gss-xdr.x -o gss-xdr.h

gss-xdr.c : gss-xdr.h 
	rm -f gss-xdr.c
	rpcgen -c gss-xdr.x -o gss-xdr.c 

#gss-misc-ptls.c: gss-xdr.h

#gss-server-ptls: gss-xdr.h

#gss-client-ptls: gss-xdr.h

CLEANFILES=core.* *~ gss-xdr.c gss-xdr.h

endif

