
AUTOMAKE_OPTIONS = no-dependencies foreign

if BUILD_LIBRARY
EXTRA_PROGRAMS = basic_call general t1

TESTS = general$(EXEEXT) t1$(EXEEXT)

check-local: basic_call.stamp
endif BUILD_LIBRARY

LDADD = ../libdrisc.la

general_LDADD = ../libdrisc.la ${MATH_LIB} 

NOOPT_COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(NOOPT_CFLAGS)

general.o: general.c
	$(NOOPT_COMPILE) -c `test -f '$<' || echo '$(srcdir)/'`$<

general.c:  general.ops
	perl $(srcdir)/general.ops

basic_call.stamp:  basic_call@EXEEXT@
	./basic_call | head -200 1>tmp.out  2>&1
	@grep -v "lib[vi]code.a" tmp.out > tmp1.out
	@mv tmp1.out tmp.out
	diff -w tmp.out $(srcdir)/basic_call.out && touch basic_call.stamp
	@rm tmp.out

# t2.stamp:  t2@EXEEXT@
# 	./t2 1>tmp.out 2>&1
# 	@grep -v "lib[vi]code.a" tmp.out > tmp1.out
# 	@mv tmp1.out tmp.out
# 	diff -w tmp.out t2.out && touch t2.stamp
# 	@rm tmp.out

clean:
	rm -f t4.stamp t2.stamp ${EXTRA_PROGRAMS} general.c

