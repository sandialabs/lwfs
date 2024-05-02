
AUTOMAKE_OPTIONS = no-dependencies foreign

EXTRA_PROGRAMS = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 gray general mix atl_test 

syntax-tests:  t4.stamp

run_time_tests: t1@EXEEXT@ t2.stamp t3@EXEEXT@ t5@EXEEXT@ t6@EXEEXT@ t8@EXEEXT@ t9@EXEEXT@ t10@EXEEXT@ gray@EXEEXT@ mix@EXEEXT@ atl_test@EXEEXT@ @GENERAL_TEST@
	./t1
	./t3
	./t5
	./t6
	./t8
	./t9
	./t10
	./atl_test
	./gray
	@GENERAL_TEST@

check-local: syntax-tests @test_sub@

general_LDADD = ../libecl.la ${MATH_LIB} 

general.c:  general.ops
	perl general.ops

general.$(OBJEXT): general.c
	$(LIBTOOL) --mode=compile `echo $(COMPILE) | sed 's/-O[0-9]*//'`  -c $<

t4.stamp:  t4@EXEEXT@
	./t4 1>tmp.out  2>&1
	@grep -v "lib[vi]code.a" tmp.out > tmp1.out
	@grep -v "## Error.*error" tmp1.out > tmp.out
	@rm tmp1.out
	diff -w tmp.out t4.out && touch t4.stamp
	@rm tmp.out

t2.stamp:  t2@EXEEXT@
	./t2 1>tmp.out 2>&1
	@grep -v "lib[vi]code.a" tmp.out > tmp1.out
	@mv tmp1.out tmp.out
	diff -w tmp.out t2.out && touch t2.stamp
	@rm tmp.out

clean:
	rm -f t4.stamp t2.stamp ${EXTRA_PROGRAMS} general.c

LDADD = ../libecl.la
