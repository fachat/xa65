#
# Makefile for tests
#

XA=../../xa

CA65=ca65
LD65=ld65

tests: unnamed1 unnamed2 escape1 escape2 clean


unnamed1: unnamed1.a65
	#${CA65} $<; ${LD65} -t none -o unnamed1.ca65 unnamed1.o; rm unnamed1.o
	${XA} -XCA65 $<
	cmp unnamed1.ca65 a.o65

unnamed2: unnamed2.a65
	#${CA65} $<; ${LD65} -t none -o unnamed2.ca65 unnamed2.o; rm unnamed2.o
	${XA} -XCA65 $< 2>a.err || true
	cmp unnamed2.err a.err

escape1: escape1.a65
	${XA} $<
	cmp escape1.out a.o65

escape2: escape2.a65
	#${CA65} $<; ${LD65} -t none -o escape2.ca65 escape2.o; rm escape2.o
	${XA} -XCA65 $< 2>a.err || true
	cmp escape2.ca65 a.o65

clean:
	rm -f a.err a.o65

