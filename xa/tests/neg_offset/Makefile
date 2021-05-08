default: test0 test1 test2 test3 test4 test5 test6 test7 test8 

test0:
	# this test must fail.
	../../xa -o test0.o test0.s || exit 0 && exit 1

test1:
	../../xa -R -LLIB6502 test1.s -o test1.o
	../hextool -cmp=ok1 < test1.o

test2:
	../../xa -R -LLIB6502 test2.s -o test2.o
	../hextool -cmp=ok2 < test2.o

test3:
	../../xa -R -LLIB6502 test3.s -o test3.o
	../hextool -cmp=ok3 < test3.o

test4:
	../../xa -R -LLIB6502 test4.s -o test4.o
	../hextool -cmp=ok4 < test4.o

test5:
	../../xa -R -LLIB6502 test5.s -o test5.o
	../hextool -cmp=ok5 < test5.o

test6:
	# this test must fail.
	../../xa -R -LLIB6502 test6.s -o test6.o || exit 0 && exit 1

test7:
	# this test must fail.
	../../xa -R -LLIB6502 test7.s -o test7.o || exit 0 && exit 1

test8:
	# this test must fail.
	../../xa -R -LLIB6502 test8.s -o test8.o || exit 0 && exit 1

clean:
	rm -f *.o
