
XA=../../xa

all: test1 clean

clean:
	rm a.o65

%:%.a65
	${XA} $<
	cmp a.o65 $@.o65


