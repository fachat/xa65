#
# Makefile for tests
#

XA=../../xa

CA65=ca65
LD65=ld65
OBJS=unnamed1 unnamed2 escape1 escape2 lll

# escape1 test only relevant if xa23 mode is on
#tests: unnamed1 unnamed2 escape1 escape2 clean
tests: $(OBJS)

# BSD make won't populate $< in these and GNU make doesn't like $>

unnamed1: unnamed1.a65
	#${CA65} unnamed1.a65; ${LD65} -t none -o unnamed1.ca65 unnamed1.o; rm unnamed1.o
	${XA} -XCA65 unnamed1.a65 -o $@
	../hextool -cmp=unnamed1.ca65 < $@
	${XA} -a unnamed1.a65 -o $@
	../hextool -cmp=unnamed1.ca65 < $@

unnamed2: unnamed2.a65
	#${CA65} unnamed2.a65; ${LD65} -t none -o unnamed2.ca65 unnamed2.o; rm unnamed2.o
	${XA} -XCA65 unnamed2.a65 -o $@ 2>a.err || true
	../hextool -cmp=unnamed2.ca65 < $@
	${XA} -a unnamed2.a65 -o $@ 2>a.err || true
	../hextool -cmp=unnamed2.ca65 < $@

# add -XXA23 to actually test this
escape1: escape1.a65
	# shouldn't work by default
	${XA} escape1.a65 -o $@ || exit 0 && exit 1
	${XA} -XXA23 escape1.a65 -o $@
	../hextool -cmp=escape1.out < $@
	${XA} -k escape1.a65 -o $@
	../hextool -cmp=escape1.out < $@

escape2: escape2.a65
	#${CA65} escape2.a65; ${LD65} -t none -o escape2.ca65 escape2.o; rm escape2.o
	${XA} -XCA65 escape2.a65 -o $@ 2>a.err || true
	../hextool -cmp=escape2.ca65 < $@

lll: lll.a65
	# shouldn't work
	${XA} lll.a65 -o $@ || exit 0 && exit 1
	${XA} -a lll.a65 -o $@ || exit 0 && exit 1
	${XA} -w lll.a65 -o $@ || exit 0 && exit 1
	# should work
	${XA} -XCA65 lll.a65 -o $@
	../hextool -cmp=lll.out < $@

clean:
	rm -f *.err a.o65 $(OBJS)

