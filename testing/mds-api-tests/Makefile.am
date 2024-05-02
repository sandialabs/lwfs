AM_CFLAGS = -Wall

INCLUDES = -I$(top_srcdir)/src $(all_includes)

METASOURCES = AUTO

bin_PROGRAMS = mds_create mds_stress_dir

mds_create_SOURCES =  mds_create.c
mds_create_LDADD = $(top_builddir)/src/liblwfs.la

mds_stress_dir_SOURCES =  mds_stress_dir.c
mds_stress_dir_LDADD = $(top_builddir)/src/liblwfs.la

$(top_builddir)/src/liblwfs.la:
	pushd .; cd $(top_builddir)/src; make liblwfs.la; popd;

$(top_srcdir)/src/lwfs_xdr.h: $(top_srcdir)/src/lwfs_xdr.x
	pushd .; cd $(top_builddir)/src; make lwfs_xdr.h; popd;

$(top_srcdir)/src/comm/comm_xdr.h: $(top_srcdir)/src/comm/comm_xdr.x
	pushd .; cd $(top_builddir)/src/comm; make comm_xdr.h; popd;

$(top_srcdir)/src/security/security_xdr.h: $(top_srcdir)/src/security/security_xdr.x
	pushd .; cd $(top_builddir)/src/security; make security_xdr.h; popd;

$(top_srcdir)/src/mds/mds_xdr.h: $(top_srcdir)/src/mds/mds_xdr.x
	pushd .; cd $(top_builddir)/src/mds; make mds_xdr.h; popd;
	
mds_create.c: $(top_srcdir)/src/lwfs_xdr.h

CLEANFILES=core.* *~
