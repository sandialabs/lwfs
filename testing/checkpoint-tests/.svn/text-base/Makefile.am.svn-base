
INCLUDES =  $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src
INCLUDES += -I$(top_builddir)/checkpoint-tests


AM_CPPFLAGS = -I${MPI_INCDIR} -DMPI -DPARANOID -O -Wall

CCLD = --tag=CC $(MPICC)
CC=$(MPICC) --static
LIBS += $(MPILIBS)

if HAVE_MPI

noinst_PROGRAMS = 
bin_PROGRAMS = 

#noinst_PROGRAMS = mpiio-nto1-write
#mpiio_nton_write_SOURCES = mpiio-nton-write.c
#mpiio_nton_write_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}

#noinst_PROGRAMS += mpiio-nton-write
#mpiio_nto1_write_SOURCES = mpiio-nto1-write.c
#mpiio_nto1_write_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}

noinst_PROGRAMS += posix-nton-write
posix_nton_write_SOURCES = posix-opts.c posix-nton-write.c
posix_nton_write_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}

noinst_PROGRAMS += posix-nto1-write
posix_nto1_write_SOURCES = posix-opts.c posix-nto1-write.c
posix_nto1_write_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}

noinst_PROGRAMS += posix-nton-create
posix_nton_create_SOURCES = posix-opts.c posix-nton-create.c
posix_nton_create_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}

bin_PROGRAMS += lwfs-nton-create
lwfs_nton_create_SOURCES = lwfs-opts.c lwfs-nton-create.c
lwfs_nton_create_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}
lwfs_nton_create_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
lwfs_nton_create_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la
lwfs_nton_create_LDADD += $(LWFS_BUILDDIR)/src/common/libcommon.la

bin_PROGRAMS += lwfs-nton-write
lwfs_nton_write_SOURCES = lwfs-opts.c lwfs-nton-write.c
lwfs_nton_write_LDFLAGS = 
lwfs_nton_write_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
lwfs_nton_write_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la

if HAVE_GENGETOPT

lwfs-opts.c: $(srcdir)/posix-opts.ggo $(srcdir)/lwfs-opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/posix-opts.ggo $(srcdir)/lwfs-opts.ggo \
		$(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo \
		$(LWFS_SRCDIR)/src/client/authr_client/authr_client_opts.ggo \
		$(LWFS_SRCDIR)/src/client/storage_client/storage_client_opts.ggo \
		| $(GENGETOPT) -S --set-package="$(PACKAGE)" --set-version="$(VERSION)" \
		--file-name=lwfs-opts 

posix-opts.c: $(srcdir)/posix-opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/posix-opts.ggo \
		| $(GENGETOPT) -S --set-package="$(PACKAGE)" \
		--set-version="$(VERSION)" \
		--file-name=posix-opts
endif

#noinst_PROGRAMS += lwfs-nton-stripe
#lwfs_nton_stripe_SOURCES = lwfs-nton-stripe.c
#lwfs_nton_stripe_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}
#lwfs_nton_stripe_LDADD =  $(LWFS_BUILDDIR)/support/util/liblwfs_util.la
#lwfs_nton_stripe_LDADD += $(LWFS_BUILDDIR)/src/storage/liblwfs_ss.la


#noinst_PROGRAMS += mpi-test
#mpi_test_SOURCES = test.c
#mpi_test_LDFLAGS = ${MPI_LIBDIR} ${GM_LIBDIR}
#mpi_test_LDADD = $(LWFS_BUILDDIR)/src/types/liblwfs_types.la
#mpi_test_LDADD += $(LWFS_BUILDDIR)/support/logger/liblogger.la

images: \
	images/posix-nton-write_client-scaling.eps \
	images/posix-nton-write_ost-scaling.eps

images/posix-nton-write_client-scaling.eps:
	test ! -d images && mkdir images;\
	test ! -d data && mkdir data;\
	scripts/posix-nton-write_client-scaling.pl \
	    --imagedir=images \
	    --datadir=data \
	    --scriptdir=scripts \
	    --resultdir=results

images/posix-nton-write_ost-scaling.eps:
	test ! -d images && mkdir images; \
	test ! -d data && mkdir data; \
	scripts/posix-nton-write_ost-scaling.pl \
	    --imagedir=images \
	    --datadir=data \
	    --imagedir=images \
	    --scriptdir=scripts \
	    --resultdir=results


CLEANFILES = lwfs-opts.c lwfs-opts.h
CLEANFILES += posix-opts.c posix-opts.h
CLEANFILES += images/* data/* *.dat

endif

