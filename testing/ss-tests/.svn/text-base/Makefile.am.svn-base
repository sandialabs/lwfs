AM_CFLAGS = -Wall

INCLUDES = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

CCLD = --tag=CC $(MPICC)
CC=$(MPICC)

#if HAVE_EBOFS
#SS_IOLIB = ebofs
#SS_ROOT = ebofs.disk
#else
SS_IOLIB = sysio
SS_ROOT = ss-root
#endif


CLIENT_EXEC=""

AUTHR_NID = 0
AUTHR_PID = 124

SS_NID = 0
SS_PID = 122

LIBS += $(MPILIBS)

METASOURCES = AUTO

noinst_PROGRAMS = storage-tests storage-attrs-test
storage_tests_SOURCES =  cmdline.c storage-tests.c perms.c
storage_tests_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la

storage_attrs_test_SOURCES =  cmdline.c storage-attrs-test.c perms.c
storage_attrs_test_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la

if HAVE_GENGETOPT
$(srcdir)/cmdline.c: $(srcdir)/local_opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/local_opts.ggo \
		$(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo \
		$(LWFS_SRCDIR)/src/client/authr_client/authr_client_opts.ggo \
		$(LWFS_SRCDIR)/src/client/storage_client/storage_client_opts.ggo \
		| $(GENGETOPT) -S --set-package="$(PACKAGE)" \
		--set-version="$(VERSION)" --output-dir=$(srcdir)
endif


showenv:
if TEST_SERVERS
	@echo "TESTING SERVERS"
endif
	@echo "AUTHR_NID=$(AUTHR_NID) AUTHR_PID=$(AUTHR_PID)"
	@echo "SS_NID=$(AUTHR_NID) SS_PID=$(AUTHR_PID)"


TESTS = 

if TEST_SERVERS
TESTS +=  start-authr 
TESTS +=  start-ss 
endif

TESTS += create-test 
TESTS += write-test
TESTS += read-test
TESTS += setattr-test
TESTS += getattr-test
TESTS += rmattr-test
TESTS += remove-test

if TEST_SERVERS
TESTS +=  kill-ss 
TESTS +=  kill-authr 
endif

XFAIL_TESTS = 


storage_server.pid: 


# ----- SERVERS ------
if HAVE_EBOFS
start-authr: Makefile.am ebofs.disk
else
start-authr: Makefile.am
endif
	@echo "#!/bin/sh" > $@
	@echo $(LWFS_BUILDDIR)/src/server/authr_server/lwfs-authr \
		--verbose=6 --logfile=$@.log --daemon --authr-db-clear \
		--authr-pid=$(AUTHR_PID) >> $@
	@echo sleep 3 >> $@
	@chmod +x $@

kill-authr: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill \
		--logfile=$@.log --server-pid=$(AUTHR_PID) >> $@
	@chmod +x $@


start-ss: Makefile.am 
	@echo "#!/bin/sh" > $@
	@echo rm -rf ss-root >> $@
	@echo $(LWFS_BUILDDIR)/src/server/storage_server/lwfs-ss \
		--verbose=6 --logfile=$@.log \
		--ss-iolib=$(SS_IOLIB) --ss-root=$(SS_ROOT)\
		--daemon >> $@
	@echo sleep 3 >> $@
	@chmod +x $@


kill-ss: Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(LWFS_BUILDDIR)/src/progs/lwfs-kill/lwfs-kill \
		--logfile=$@.log --server-pid=$(SS_PID) >> $@
	@chmod +x $@
# ----- END SERVERS ------



# ----- Tests (all assume the servers are running) ------

create-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=create " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@ 
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

remove-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=remove " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

read-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=read " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

write-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=write " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

trunc-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=trunc " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

stat-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=stat " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

setattr-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=setattr " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

getattr-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=getattr " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

rmattr-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=rmattr " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@

listattr-test: storage-tests
	@echo "#!/bin/sh" > $@
	@echo "$(CLIENT_EXEC) storage-tests " \
	    "--test=listattr " \
	    "--authr-nid=$(AUTHR_NID) --authr-pid=$(AUTHR_PID) " \
	    "--ss-pid=$(SS_PID) --ss-nid=$(SS_NID) " \
	    "> $@.log" >> $@
	@echo "if [ \"x\`tail -1 $@.log | tr -d '\n'\`\" == \"xPASSED\" ]; then exit 0; fi" >> $@
	@echo "exit 1" >> $@
	@chmod +x $@


if HAVE_EBOFS

# Creates a 1 MB file full of zeros, then formats it as an EBOFS FS
ebofs.disk: 
	dd if=/dev/zero of=ebofs.disk bs=1k count=100000
	$(MKFS_EBOFS) ebofs.disk

endif


#bin_PROGRAMS +=  ss-perf
#ss_perf_SOURCES = ss-perf.c perms.c
#ss_perf_LDADD  = $(LWFS_BUILDDIR)/src/client/liblwfs_client.la


CLEANFILES=core.* *~ *.db *.db.* acls.db ebofs.disk $(TESTS) *.log
CLEANFILES += start-servers kill-servers *.log $(srcdir)/cmdline.* 


