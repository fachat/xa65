default: test1 test2

test1:
	../../xa -R -LLIB6502 test1.s -o test1.o
	../hextool -cmp=ok < test1.o

test2:
	../../xa -R -LLIB6502 test2.s -o test2.o
	../hextool -cmp=ok < test2.o

clean:
	rm -f *.o
