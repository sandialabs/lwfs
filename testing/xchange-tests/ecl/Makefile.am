## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies foreign

SUBDIRS = . tests

if !KERNEL
include_HEADERS = ecl.h
all-local:
check-local:
install-exec-local:
lib_LTLIBRARIES = libecl.la

else KERNEL
all-local: ecl_module.o
check-local: kernel_check_target
MOSTLYCLEANFILES = ecl_module.o
install-exec-local: ecl_module.o
		install -d /lib/modules/@KERNEL_VERSION@/kernel/misc
		install -c ecl_module.o /lib/modules/@KERNEL_VERSION@/kernel/misc

SPECIAL_LEX_ARGS= -Skernel/kflex.skl 
KERNEL_OBJS= kecl.lo

endif KERNEL

noinst_HEADERS = assert.h config.h ecl.tab.h ecl_internal.h structs.h

DCG_LIBS=@DCG_LIBS@

libecl_la_SOURCES = ecl.c ecl_node.c cg.c standard.c
libecl_la_LIBADD = $(XLATE_O) $(DCG_LIBS) $(KERNEL_OBJS) 
libecl_la_DEPENDENCIES = $(XLATE_O) $(KERNEL_OBJS)
libecl_la_LDFLAGS =

EXTRA_libecl_la_SOURCES = xlate.c kecl.c

xlate.c: icode.spec
	@PERL@ @XLATE_PATH@/xlate.pl -spec icode.spec xlate.c

icode.spec: cg.c
	rm -f icode.spec
	grep "/* op_i_" cg.c | sed 's=.*/* op_i_==; s/ .*//' | sort -u > icode.spec

lex.yy.c: ecl.l
	@LEX@ $(SPECIAL_LEX_ARGS) ecl.l

ecl.tab.h ecl.c: ecl.y
	$(YACC) -d ecl.y
	mv y.tab.h ecl.tab.h
	mv y.tab.c ecl.c

structs.h : struct.pl ecl.structs
	@PERL@ struct.pl < ecl.structs

ecl_node.c : struct.pl ecl.structs

ecl.lo: structs.h lex.yy.c
ecl_node.lo: structs.h

LDADD = libecl.la

EXTRA_PROGRAMS = t1 t2 t3 t4 t5 t6

clean-local:
	rm -f xlate.c lex.yy.c ecl.c ecl_node.c structs.h icode.spec

internal_release:
	(cd /tmp ; cvs -Q -d /users/c/chaos/CVSmaster export -r HEAD tcc_cg )
	(cd /tmp ; cvs -Q -d /users/c/chaos/CVSmaster export -r HEAD ecl )
	(cd /tmp ; tar cvf ecl.tar tcc_cg ecl)
	(rm -fr /tmp/tcc_cg /tmp/ecl )
	(cd /tmp ; gzip -9 --stdout ecl.tar > ecl.tar.gz )
	(cd /tmp ; compress ecl.tar )
	(cd /tmp ; mv ecl.tar.gz /net/www2-int/systems/chaos/ )
	(cd /tmp ; mv ecl.tar.Z /net/www2-int/systems/chaos/ )

depend:
	makedepend -- $(CPPFLAGS) -- *.c
	etags cg.c ecl.y ecl_node.c xlate.c 

#
# Below this point are targets used for linux kernel build.
#
kernel_check_target: ecl_module.o kernel/undefined_syms
	nm ecl_module.o | grep U > /tmp/cur_undefined_syms
	cmp /tmp/cur_undefined_syms kernel/undefined_syms
	rm /tmp/cur_undefined_syms

ecl_module.o:	libecl.la
	rm -fr /tmp/ecl_build
	mkdir /tmp/ecl_build
	cp .libs/libecl.a /tmp/ecl_build
	cd /tmp/ecl_build ; ar x libecl.a
	cd /tmp/ecl_build ; rm libecl.a
	cd /tmp/ecl_build ; ld -r *.o -o $@
	cp /tmp/ecl_build/ecl_module.o .
	rm -fr /tmp/ecl_build 

kecl.c:
	rm -f kecl.c
	$(LN_S) kernel/kecl.c kecl.c
