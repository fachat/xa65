#
# Makefile for tests
#

XA=../../xa

tests: linebreak include1 clean


include1: include1.a65
	${XA} -P- $< > a.out
	cmp include1.out a.out

linebreak: linebreak.a65
	${XA} -P- $< > a.out
	cmp linebreak.out a.out

clean:
	rm -f a.err a.o65 a.out

