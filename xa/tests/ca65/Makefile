#
# Makefile for tests
#

XA=../../xa

CA65=ca65
LD65=ld65
OBJS=unnamed1 unnamed2 escape2

# escape1 test only relevant if xa23 mode is on
#tests: unnamed1 unnamed2 escape1 escape2 clean
tests: $(OBJS)


unnamed1: unnamed1.a65
	#${CA65} $<; ${LD65} -t none -o unnamed1.ca65 unnamed1.o; rm unnamed1.o
	${XA} -XCA65 $< -o $@
	../hextool -cmp=unnamed1.ca65 < $@

unnamed2: unnamed2.a65
	#${CA65} $<; ${LD65} -t none -o unnamed2.ca65 unnamed2.o; rm unnamed2.o
	${XA} -XCA65 $< -o $@ 2>a.err || true
	../hextool -cmp=unnamed2.ca65 < $@

# add -XXA23 to actually test this
escape1: escape1.a65
	${XA} $< -o $@
	../hextool -cmp=escape1.out < $@

escape2: escape2.a65
	#${CA65} $<; ${LD65} -t none -o escape2.ca65 escape2.o; rm escape2.o
	${XA} -XCA65 $< -o $@ 2>a.err || true
	../hextool -cmp=escape2.ca65 < $@

clean:
	rm -f a.err a.o65 $(OBJS)

