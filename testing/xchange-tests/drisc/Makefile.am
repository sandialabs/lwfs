## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies foreign

SUBDIRS = . tests vtests

if !KERNEL
BUILT_SOURCES = drisc.h
if BUILD_LIBRARY
lib_LTLIBRARIES = libdrisc.la
include_HEADERS = drisc.h
endif BUILD_LIBRARY

all-local:
check-local:
install-exec-local:

else KERNEL

BUILT_SOURCES = drisc.h
all-local: drisc_module.o
check-local: kernel_check_target
MOSTLYCLEANFILES = drisc_module.o
install-exec-local: drisc_module.o
		install -d /lib/modules/@KERNEL_VERSION@/kernel/misc
		install -c drisc_module.o /lib/modules/@KERNEL_VERSION@/kernel/misc

KERNEL_OBJS= kdrisc.lo env.lo qsort.lo libopcodefixup.lo

endif KERNEL

noinst_HEADERS = config.h drisc_internal.h

if MULTI_TARGET
libdrisc_la_SOURCES = drisc_sparc.c drisc_util.c sparc.c drisc_x86.c x86.c \
		drisc_arm5.c arm5.c drisc_virtual.c drisc_x86_64.c x86_64.c \
		virtual.c drisc.c vm.c
else !MULTI_TARGET
if NATIVE_CG
libdrisc_la_SOURCES = drisc_util.c  drisc_virtual.c virtual.c drisc.c vm.c drisc_${ARCH_FILE}.c ${ARCH_FILE}.c
else !NATIVE_CG
libdrisc_la_SOURCES = drisc_util.c  drisc_virtual.c virtual.c drisc.c vm.c
endif !NATIVE_CG
endif !MULTI_TARGET

libdrisc_la_DEPENDENCIES = ${KERNEL_OBJS}
libdrisc_la_LIBADD =   ${KERNEL_OBJS} @DIS_LIBS@ 
libdrisc_la_LDFLAGS =

CLEANFILES = drisc_sparc.c drisc.h drisc_x86.c drisc_x86_64.c drisc_arm5.c drisc_virtual.c

vm.lo: vm_include.c

drisc_sparc.c: sparc.ops
	$(PERL) $(srcdir)/sparc.ops

drisc_virtual.c: virtual.ops
	$(PERL) $(srcdir)/virtual.ops

drisc_x86.c: x86.ops
	$(PERL) $(srcdir)/x86.ops

drisc_x86_64.c: x86_64.ops
	$(PERL) $(srcdir)/x86_64.ops

drisc_ia64.c: ia64.ops
	$(PERL) $(srcdir)/ia64.ops

drisc_arm5.c: arm5.ops
	$(PERL) $(srcdir)/arm5.ops

drisc.h: base.ops
	$(PERL) $(srcdir)/base.ops

$(libdrisc_la_OBJECTS) : drisc.h

#
# Below this point are targets used for linux kernel build.
#
kernel_check_target: drisc_module.o kernel/undefined_syms
	nm drisc_module.o | grep U > /tmp/cur_undefined_syms
	cmp /tmp/cur_undefined_syms kernel/undefined_syms
	rm /tmp/cur_undefined_syms

drisc_module.o:	libdrisc.la
	rm -fr /tmp/drisc_build
	mkdir /tmp/drisc_build
	cp .libs/libdrisc.a /tmp/drisc_build
	cd /tmp/drisc_build ; ar x libdrisc.a
	cd /tmp/drisc_build ; rm libdrisc.a
	cd /tmp/drisc_build ; ld -r *.o -o $@ @LDFLAGS@ @DIS_LIBS@
	cp /tmp/drisc_build/drisc_module.o .
	rm -fr /tmp/drisc_build 

env.c: 
	rm -f env.c
	$(LN_S) kernel/env.c env.c

qsort.c: 
	rm -f qsort.c
	$(LN_S) kernel/qsort.c qsort.c

kdrisc.c:
	rm -f kdrisc.c
	$(LN_S) kernel/kdrisc.c kdrisc.c

libopcodefixup.c:
	rm -f libopcodefixup.c
	$(LN_S) kernel/libopcodefixup.c libopcodefixup.c

