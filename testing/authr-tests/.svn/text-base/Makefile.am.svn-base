#SHELL = /bin/bash

INCLUDES =  $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src
#INCLUDES += -I$(LWFS_SRCDIR)/include

START_SERVER=0

# To override values, set env variables and run 'make -e'
SERVER_EXEC =
#CLIENT_EXEC = "yod -sz 1"
CLIENT_EXEC = ""
AUTHR_PID = 124
#AUTHR_NID = 1032
AUTHR_NID = 0
#NID = `utcp_nid $(PTL_IFACE)`

METASOURCES = AUTO

AM_CPPFLAGS = 
AM_CFLAGS = -Wall 
AM_LIBS = 
AM_LDFLAGS = -L$(LWFS_BUILDDIR)/src/support/trace
AM_LDFLAGS += -L$(LWFS_BUILDDIR)/src/support
AM_LDFLAGS += -L$(LWFS_BUILDDIR)/src/client
AM_LDFLAGS += -L$(LWFS_BUILDDIR)/src/client/authr_client

noinst_PROGRAMS = authr-tests

authr_tests_SOURCES = cmdline.c authr-tests.c
authr_tests_LDADD = 
#authr_tests_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la
#authr_tests_LDADD += $(LWFS_BUILDDIR)/src/support/libsupport.la
authr_tests_LDADD += -llwfs_client

if HAVE_GENGETOPT
$(srcdir)/cmdline.c: $(srcdir)/local_opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/local_opts.ggo \
		$(LWFS_SRCDIR)/src/client/authr_client/authr_client_opts.ggo \
		$(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo \
		| $(GENGETOPT) -S --set-package="authr-opts" \
		--set-version=$(VERSION) --output-dir=$(srcdir)
endif

showenv:
if TEST_SERVERS
	@echo "TESTING SERVERS"
endif
	@echo "CLIENT_EXEC=$(CLIENT_EXEC)"
	@echo "AUTHR_NID=$(AUTHR_NID) AUTHR_PID=$(AUTHR_PID)"

testb:
	@export AUTHR_NID=10; export AUTHR_PID=11; $(MAKE) -e testa

TESTS = 
if TEST_SERVERS
TESTS += start-authr
endif
TESTS += create-cid
TESTS += modacl
TESTS += getacl
TESTS += getcap
TESTS += verify-cap
TESTS += remove-cid
if TEST_SERVERS
TESTS += kill-authr
endif


# ----- Start and kill the server ----------------------

start-authr: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(LWFS_BUILDDIR)/src/server/authr_server/lwfs-authr \
		--verbose=6 --logfile=$@.log --daemon \
		--authr-db-clear \
		--authr-pid=$(AUTHR_PID) >> $@
	@echo sleep 3 >> $@
	@chmod +x $@

kill-authr: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill \
		--logfile=$@.log \
		--server-pid=$(AUTHR_PID) \
		--server-nid=$(AUTHR_NID) \
		>> $@
	@chmod +x $@


# ----- Tests (all assume the servers are running) ------

exists: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=1" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

create-cid: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=2" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

remove-cid: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=3" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

getacl: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=4" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

modacl: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=5" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@


getcap: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=6" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

verify-cap: authr-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) authr-tests --testid=7" \
		"--authr-nid=$(AUTHR_NID)" \
		"--authr-pid=$(AUTHR_PID)" \
		"> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@


CLEANFILES = *~ $(srcdir)/simple_svc_xdr.c $(srcdir)/simple_svc_xdr.h *.out acls.db
CLEANFILES += $(TESTS) *.log
CLEANFILES += $(srcdir)/cmdline.* authr.db
