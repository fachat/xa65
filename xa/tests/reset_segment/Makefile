default: test1 

test1:
	../../xa -R -LLIB6502 test1.s -o test1.o
	../hextool -cmp=ok < test1.o

clean:
	rm -f *.o
