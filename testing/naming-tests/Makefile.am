AM_CFLAGS = -Wall


AUTHR_PID = 124
SS_PID = 122
NAMING_PID = 126

INCLUDES = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

METASOURCES = AUTO

#bin_PROGRAMS =  naming-test naming-perf-tests mpi-hostname
bin_PROGRAMS =  naming-tests

#check_naming_SOURCES =  cmdline.c check-naming.c perms.c
#check_naming_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la

naming_tests_SOURCES =  cmdline.c naming-tests.c perms.c
naming_tests_LDADD = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
naming_tests_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la


if HAVE_GENGETOPT
cmdline.c: local_opts.ggo Makefile.am
	cat local_opts.ggo \
		$(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo \
		$(LWFS_SRCDIR)/src/client/authr_client/authr_client_opts.ggo \
		$(LWFS_SRCDIR)/src/client/storage_client/storage_client_opts.ggo \
		$(LWFS_SRCDIR)/src/client/naming_client/naming_client_opts.ggo \
		| $(GENGETOPT) -S --set-package="$(PACKAGE)" --set-version="$(VERSION)" 
endif

#CCLD = --tag=CC $(MPICC)


# This tests that the ss-perf client can connect to a local
# authorization server and storage server. 
testing : naming-tests
	@echo; echo "============= STARTING AUTHR SERVER ============"; echo
	$(LWFS_BUILDDIR)/src/server/authr_server/lwfs-authr \
		--verbose=2 --daemon --authr-db-clear 
	@sleep 3 
	@echo; echo "============= STARTING STORAGE SERVER =========="; echo
	$(LWFS_BUILDDIR)/src/server/storage_server/lwfs-ss \
		--verbose=2 --daemon
	@sleep 3 
	@echo; echo "============= STARTING NAMING SERVER =========="; echo
	$(LWFS_BUILDDIR)/src/server/naming_server/lwfs-naming-server \
		--verbose=2 --daemon --naming-db-clear
	@sleep 3 
	@echo; echo "============= STARTING NAMING TESTS ============="; echo
	@naming-tests --verbose=2
	@echo; echo "============= KILLING STORAGE SERVER ============="; echo
	$(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill --verbose=2 --server-pid=$(SS_PID)
	@echo; echo "============= KILLING AUTHR SERVER ==============="; echo
	$(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill --verbose=2 --server-pid=$(AUTHR_PID)
	@echo; echo "============= KILLING NAMING SERVER =============="; echo
	$(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill --verbose=2 --server-pid=$(NAMING_PID)
	@echo; echo "============= FINISHED ========================="; echo


CLEANFILES=core.* *~ *.db cmdline.*
