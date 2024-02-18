
XA=../../xa

all: test1.o65

.SUFFIXES: .a65 .o65

.a65.o65:
	${XA} -XCA65 -o $@ $>

clean:
	rm -f *.o65
	rm -f test1

