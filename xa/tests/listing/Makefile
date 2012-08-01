#
# Makefile for tests
#

XA=../../xa

CA65=ca65
LD65=ld65

tests: linebreak include1 listblocks listca65 listca65_2 clean


include1: include1.a65
	${XA} -P- $< > a.out
	cmp include1.out a.out

linebreak: linebreak.a65
	${XA} -P- $< > a.out
	cmp linebreak.out a.out

listblocks: listblocks.a65
	${XA} -P- -Fhtml listblocks.a65 > a.out
	cmp listblocks.html a.out

listca65: listca65.a65
	${XA} -XCA65 -P- -Fhtml listca65.a65 > a.out
	#${CA65} $<; ${LD65} -t none -o listca65.ca65 listca65.o; rm listca65.o
	cmp listca65.html a.out
	cmp listca65.ca65 a.o65

listca65_2: listca65_2.a65
	${XA} -XCA65 -P- -Fhtml listca65_2.a65 > a.out
	${CA65} $<; ${LD65} -t none -o listca65_2.ca65 listca65_2.o; rm listca65_2.o
	cmp listca65_2.html a.out
	cmp listca65_2.ca65 a.o65

clean:
	rm -f a.err a.o65 a.out

