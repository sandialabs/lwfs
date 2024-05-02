INCLUDES =  $(all_includes)

METASOURCES = AUTO

AM_CPPFLAGS = -I${MPI_INCDIR} -DMPI -DPARANOID -O -Wall

CCLD = --tag=CC $(MPICC)
CC=$(MPICC) --static
LIBS += $(MPILIBS)


bin_PROGRAMS = mpi-xfer

noinst_HEADERS = 


mpi_xfer_SOURCES = mpi-xfer.c
mpi_xfer_LDADD = -lm 

#mpi_xfer : mpi-xfer.c
#	mpicc mpi-xfer.c -I/cluster/rte/mpich/include/ \
#	-L/cluster/rte/mpich/lib -L/cluster/rte/gm/lib \
#	-o mpi-xfer -lm	

CLEANFILES = 
