
XA=../../xa

all: test1 test2 clean

clean:
	rm a.o65

%:%.a65
	${XA} -XC $<
	cmp a.o65 $@.o65


