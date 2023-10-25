#
# Makefile for tests
#

XA=../../xa

tests: mixabs1 mixabs2


mixabs1: mixabsolute.a65
	@echo This should fail
	${XA} $< || exit 0 && exit 1

mixabs2: mixabsolute.a65
	${XA} -R $< -o $@.tmp
	../../file65 -V $@.tmp
	../../reloc65 -bt 40960 -o $@ $@.tmp
	../hextool -cmp=b.ok < $@

clean:
	rm -f a.err a.o65 a.hex b.o65 mixabs2*

