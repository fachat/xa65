#
# Makefile for tests
#

XA=../../xa

CA65=ca65
LD65=ld65

tests: assertx.out assertx.html linebreakx.out include1x.out listblocksx.html listca65x.html listca65_2x.html

assertx.out: assert.a65
	${XA} -P- -o assertx.o65 $< > $@
	../hextool -cmp=assert.out < $@
	../hextool -cmp=assert.o65 < assertx.o65

assertx.html: assert.a65
	${XA} -P- -Fhtml -o assertx.o65 $< > $@
	../hextool -cmp=assert.html < $@

include1x.out: include1.a65
	${XA} -P- $< > $@
	../hextool -cmp=include1.out < $@

linebreakx.out: linebreak.a65
	${XA} -P- $< > $@
	../hextool -cmp=linebreak.out < $@

listblocksx.html: listblocks.a65
	${XA} -P- -Fhtml listblocks.a65 > $@
	../hextool -cmp=listblocks.html < $@

listca65x.html: listca65.a65
	${XA} -XCA65 -P- -Fhtml -o listca65x.o65 listca65.a65 > $@
	#${CA65} $<; ${LD65} -t none -o listca65.ca65 listca65.o; rm listca65.o
	../hextool -cmp=listca65.html < $@
	../hextool -cmp=listca65.ca65 < listca65x.o65

listca65_2x.html: listca65_2.a65
	${XA} -XCA65 -P- -Fhtml -o listca65_2x.o65 listca65_2.a65 > $@
	#${CA65} $<; ${LD65} -t none -o listca65_2.ca65 listca65_2.o; rm listca65_2.o
	../hextool -cmp=listca65_2.html < $@
	../hextool -cmp=listca65_2.ca65 < listca65_2x.o65

clean:
	rm -f *.err *x.out *x.html *x.o65

