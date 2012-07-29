#
# Makefile for tests
#

XA=../../xa

tests: linebreak clean


linebreak: linebreak.a65
	${XA} -P- $< > a.out
	cmp linebreak.out a.out

clean:
	rm -f a.err a.o65 a.out

