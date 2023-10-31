
all: test1 clean

test1: test1.s
	../../xa -R -o $@.o65 $<
	../hextool -cmp=$@.o65 < $@.ok

clean:
	rm -f *.o65

