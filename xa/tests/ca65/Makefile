#
# Makefile for tests
#

XA=../../xa

tests: unn1 unn2


unn1: unnamed1.a65
	${XA} -XCA65 $<
	cmp unnamed1.o a.o65

unn2: unnamed2.a65
	${XA} -XCA65 $< 2>a.err || true
	cmp unnamed2.err a.err

clean:
	rm -f a.err a.o65

