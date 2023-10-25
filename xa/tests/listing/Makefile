#
# Makefile for tests
#

XA=../../xa

CA65=ca65
LD65=ld65

tests: linebreak.out include1.out listblocks.html listca65.html listca65_2.html


include1.out: include1.a65
	${XA} -P- $< > a.out
	../hextool -cmp=include1.out < a.out

linebreak.out: linebreak.a65
	${XA} -P- $< > a.out
	../hextool -cmp=linebreak.out < a.out

listblocks.html: listblocks.a65
	${XA} -P- -Fhtml listblocks.a65 > a.out
	../hextool -cmp=listblocks.html < a.out

listca65.html: listca65.a65
	${XA} -XCA65 -P- -Fhtml listca65.a65 > a.out
	#${CA65} $<; ${LD65} -t none -o listca65.ca65 listca65.o; rm listca65.o
	../hextool -cmp=listca65.html < a.out
	../hextool -cmp=listca65.ca65 < a.o65

listca65_2.html: listca65_2.a65
	${XA} -XCA65 -P- -Fhtml listca65_2.a65 > a.out
	#${CA65} $<; ${LD65} -t none -o listca65_2.ca65 listca65_2.o; rm listca65_2.o
	../hextool -cmp=listca65_2.html < a.out
	../hextool -cmp=listca65_2.ca65 < a.o65

clean:
	rm -f a.err a.o65 a.out

