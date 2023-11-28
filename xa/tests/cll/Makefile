
XA=../../xa

all: test1

%: %.a65
	${XA} -XCA65 -o $@ $<

clean:
	rm -f *.o65
	rm -f test1

