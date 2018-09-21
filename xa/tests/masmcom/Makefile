default:
	# compile with masm mode on.
	../../xa -M -o test.o test.s
	../hextool -cmp=okmasm < test.o
	# compile without
	../../xa -o test.o test.s
	../hextool -cmp=oknomasm < test.o

clean:
	rm -f *.o
