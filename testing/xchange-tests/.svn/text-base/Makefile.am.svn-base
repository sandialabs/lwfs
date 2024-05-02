AM_CFLAGS = -Wall

INCLUDES = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src
INCLUDES += -Iecl -Idrisc

CCLD = --tag=CC $(MPICC)
CC=$(MPICC)

AUTHR_PID = 124
SS_PID = 122
NAMING_PID = 126

LIBS += $(MPILIBS)
AM_LDFLAGS = -Lecl/.libs -lecl -Ldrisc/.libs -ldrisc

METASOURCES = AUTO

bin_PROGRAMS = xchange-tests

xchange_tests_SOURCES =  cmdline.c xchange-tests.c
xchange_tests_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la

if HAVE_GENGETOPT
cmdline.c: local-opts.ggo Makefile.am $(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo $(LWFS_SRCDIR)/src/client/authr_client/authr_client_opts.ggo $(LWFS_SRCDIR)/src/client/naming_client/naming_client_opts.ggo $(LWFS_SRCDIR)/src/client/storage_client/storage_client_opts.ggo
	cat local-opts.ggo \
		$(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo \
		$(LWFS_SRCDIR)/src/client/authr_client/authr_client_opts.ggo \
		$(LWFS_SRCDIR)/src/client/naming_client/naming_client_opts.ggo \
		$(LWFS_SRCDIR)/src/client/storage_client/storage_client_opts.ggo \
		| $(GENGETOPT) -S --set-package="$(PACKAGE)" --set-version="$(VERSION)"
endif


# This tests that the ss-perf client can connect to a local
# authorization server and storage server. 
test: xchange-tests
	xchange-tests --authr-pid=$(AUTHR_PID) --verbose=3 \
		--ss-pid=$(SS_PID)
#		--ss-num-servers=1 --ss-server-file=storage_servers.in


testing : xchange-tests
	@echo; echo "============= STARTING AUTHR SERVER ============"; echo
	$(LWFS_BUILDDIR)/src/server/authr_server/lwfs-authr \
		--verbose=6 --daemon --authr-db-clear --authr-pid=$(AUTHR_PID)
	@sleep 3 

	@echo; echo "============= STARTING STORAGE SERVER =========="; echo
	$(LWFS_BUILDDIR)/src/server/storage_server/lwfs-ss \
		--verbose=6 --daemon --authr-pid=$(AUTHR_PID) --ss-pid=$(SS_PID)
	@sleep 3 

	@echo; echo "============= STARTING NAMING SERVER ============="; echo
	$(LWFS_BUILDDIR)/src/server/naming_server/lwfs-naming-server \
		--verbose=6 --daemon --naming-db-clear
	@sleep 3 

	@echo; echo "============= STARTING XCHANGE TESTS ============="; echo
	@xchange-tests --verbose=0

	@echo; echo "============= KILLING NAMING SERVER ============="; echo
	$(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill --verbose=2 --server-pid=$(NAMING_PID)

	@echo; echo "============= KILLING STORAGE SERVER ============="; echo
	$(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill --verbose=2 --server-pid=$(SS_PID)

	@echo; echo "============= KILLING AUTHR SERVER ============="; echo
	$(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill --verbose=2 --server-pid=$(AUTHR_PID)

	@echo; echo "============= FINISHED ========================="; echo

CLEANFILES=core.* *~ *.db *.db.* ss-root acls.db cmdline.*
