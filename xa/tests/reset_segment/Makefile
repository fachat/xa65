default: test1 

test1:
	../../xa -R -LLIB6502 test1.s -o test1.o
	../hextool -cmp=ok1 < test1.o
	../../xa -R test2.s -o test2.o
	../hextool -cmp=ok2 < test2.o

clean:
	rm -f *.o
