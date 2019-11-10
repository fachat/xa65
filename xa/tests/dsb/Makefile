default: test1 test2 

test1:
	../../xa -R -LLIB6502 test1.s -o test1.o
	../hextool -cmp=ok1 < test1.o

test2:
	# this test must fail.
	../../xa -R -LLIB6502 test2.s -o test2.o || exit 0 && exit 1

clean:
	rm -f *.o
