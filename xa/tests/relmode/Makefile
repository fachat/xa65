#
# Makefile for tests
#

XA=../../xa

tests: mixabs1 mixabs2 mix1 mix2


mixabs1: mixabsolute.a65
	@echo This should fail
	${XA} $< || exit 0 && exit 1

mixabs2: mixabsolute.a65
	${XA} -R $< -o $@.tmp
	../../file65 -v $@.tmp
	../../reloc65 -bt 40960 -o $@ $@.tmp
	../hextool -cmp=b.ok < $@

mix1: mix1.a65
	${XA} -R -o $@.o65 $<
	../../file65 -v $@.o65
	../../reloc65 -X -o $@ $@.o65
	../hextool -cmp=$@.ok < $@.o65

mix2: mix2.a65
	${XA} -R -o $@.o65 $<
	../../file65 -v $@.o65
	../../reloc65 -X -o $@ $@.o65
	../hextool -cmp=$@.ok < $@.o65

clean:
	rm -f a.err *.o65 a.hex  mixabs2* mix1 mix2 *.tmp

