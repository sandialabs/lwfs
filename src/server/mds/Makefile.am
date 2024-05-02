AM_CFLAGS = -Wall

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src $(all_includes)

METASOURCES = AUTO

bin_PROGRAMS =  mds
noinst_LTLIBRARIES =  libmds.la
noinst_HEADERS = mds_xdr.h \
		 mds_clnt.h \
		 mds_srvr.h \
		 mds_db.h \
		 mds_db_access.h \
		 mds_debug.h

libmds_la_SOURCES = mds_xdr.c \
		    mds_clnt.c \
		    mds_srvr.c \
		    mds_db.c \
		    mds_db_access.c \
		    mds_debug.c 
libmds_la_DEPENDENCIES = \
	mds_xdr.h \
	mds_srvr.h \
	$(top_srcdir)/src/comm/comm_xdr.h \
	$(top_srcdir)/src/lwfs_xdr.h 

mds_SOURCES =  mds.c 
mds_LDADD = $(top_builddir)/src/liblwfs.la
mds_DEPENDENCIES = $(top_builddir)/src/liblwfs.la 


$(top_builddir)/src/liblwfs.la: 
	pushd .; cd $(top_builddir)/src; make liblwfs.la; popd;


$(srcdir)/mds_xdr.h: $(srcdir)/mds_xdr.x $(top_srcdir)/src/lwfs_xdr.h
	rm -f $(srcdir)/mds_xdr.h
	rpcgen -h $(srcdir)/mds_xdr.x -o $(srcdir)/mds_xdr.h

$(srcdir)/mds_xdr.c : $(srcdir)/mds_xdr.h $(top_srcdir)/src/lwfs_xdr.h
	rm -f $(srcdir)/mds_xdr.c;
	rpcgen -c $(srcdir)/mds_xdr.x -o $(srcdir)/mds_xdr.c; 

	
$(top_srcdir)/src/comm/comm_xdr.h: $(top_srcdir)/src/comm/comm_xdr.x 
	pushd .; cd $(top_builddir)/src/comm; make comm_xdr.h; popd;

$(top_srcdir)/src/security/security_xdr.h: $(top_srcdir)/src/security/security_xdr.x 
	pushd .; cd $(top_builddir)/src/security; make security_xdr.h; popd;

$(top_srcdir)/src/lwfs_xdr.h: 
	pushd .; cd $(top_builddir)/src; make lwfs_xdr.h; popd; 

	
	
	
	
$(srcdir)/mds.c: $(srcdir)/mds_xdr.h \
	$(top_srcdir)/src/comm/comm_xdr.h \
	$(top_srcdir)/src/lwfs_xdr.h 
	
$(srcdir)/mds_srvr.c: $(srcdir)/mds_xdr.h \
	mds_srvr.h \
	$(top_srcdir)/src/comm/comm_xdr.h \
	$(top_srcdir)/src/lwfs_xdr.h 

mds_clnt.c: mds_xdr.h mds_clnt.h \
	$(top_srcdir)/src/comm/comm_xdr.h \
	$(top_srcdir)/src/lwfs_xdr.h 

mds_db.c: mds_db.h

mds_db_access.c: mds_db_access.h


CLEANFILES=$(srcdir)/mds_xdr.h $(srcdir)/mds_xdr.c core.* *~
