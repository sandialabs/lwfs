# portals libraries should be included by configure

#AUTOMAKE_OPTIONS = dejagnu

INCLUDES = $(all_includes)
INCLUDES += -I$(LWFS_SRCDIR)/src

METASOURCES = AUTO

# This stuff only compiles if we found the ebofs
if HAVE_EBOFS



AM_CPPFLAGS = -Wall -Wno-sign-compare

TESTS =  create-test 
TESTS += write-test
TESTS += exists-test
TESTS += read-test
TESTS += setattr-test
TESTS += getattr-test
TESTS += listattr-test
TESTS += rmattr-test
TESTS += remove-test

XFAIL_TESTS = 

noinst_PROGRAMS = ebofs-tests

ebofs_tests_SOURCES = cmdline.c ebofs-tests.c
ebofs_tests_LDADD = $(EBOFS_LIBS)
ebofs_tests_LDADD += $(LWFS_BUILDDIR)/src/support/logger/liblogger.la

if HAVE_GENGETOPT
$(srcdir)/cmdline.c: $(srcdir)/local-opts.ggo $(srcdir)/Makefile.am
	cat $(srcdir)/local-opts.ggo \
		$(LWFS_SRCDIR)/src/support/logger/logger_opts.ggo \
		| $(GENGETOPT) -S --set-package="$(PACKAGE)" \
		--set-version="$(VERSION)" --output-dir=$(srcdir)
endif

exists-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > exists-test
	@echo "ebofs-tests --testid=1 > exists.log" >> exists-test
	@chmod +x exists-test

create-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > create-test
	@echo "ebofs-tests --testid=2 > create.log" >> create-test
	@chmod +x create-test

remove-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > remove-test
	@echo "ebofs-tests --testid=3 > remove.log" >> remove-test
	@chmod +x remove-test

read-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > read-test
	@echo "ebofs-tests --testid=4 > read.log" >> read-test
	@chmod +x read-test

write-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > write-test
	@echo "ebofs-tests --testid=5 > write.log" >> write-test
	@chmod +x write-test

setattr-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > setattr-test
	@echo "ebofs-tests --testid=6 > setattr.log" >> setattr-test
	@chmod +x setattr-test

getattr-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > getattr-test
	@echo "ebofs-tests --testid=7 > getattr.log" >> getattr-test
	@chmod +x getattr-test

rmattr-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > rmattr-test
	@echo "ebofs-tests --testid=8 > rmattr.log" >> rmattr-test
	@chmod +x rmattr-test

listattr-test: ebofs-tests ebofs.disk
	@echo "#!/bin/sh" > listattr-test
	@echo "ebofs-tests --testid=9 > listattr.log" >> listattr-test
	@chmod +x listattr-test


# Creates a 1 MB file full of zeros, then formats it as an EBOFS FS
ebofs.disk: 
	dd if=/dev/zero of=ebofs.disk bs=1k count=100000
	$(MKFS_EBOFS) ebofs.disk


#check-local: ebofs-tests
#	ebofs-tests --dev=ebofs.disk

endif

CLEANFILES = ebofs.disk $(TESTS) *.log cmdline.c cmdline.h
