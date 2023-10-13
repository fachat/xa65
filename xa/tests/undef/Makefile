#
# Makefile for tests
#

XA=../../xa

tests: undef1 undef2 undef3 clean


undef1: undef.a65
	@echo This should fail
	${XA} $< || exit 0 && exit 1
	${XA} -R $< || exit 0 && exit 1
	${XA} -R -U -DFAIL $< || exit 0 && exit 1
	${XA} -R -Ll1 -Ll3 -Ll4 -Ll5 $< || exit 0 && exit 1

undef2: undef.a65
	${XA} -R -Ll1 -Ll3 $<
	file65 -V a.o65
	reloc65 -bt 40960 -o b.o65 a.o65
	hexdump -C b.o65 > b.hex
	cmp undef2.out b.hex

undef3: undef.a65
	${XA} -R -U $<
	file65 -V a.o65
	reloc65 -bt 40960 -o b.o65 a.o65
	hexdump -C b.o65 > b.hex
	cmp undef2.out b.hex

clean:
	rm -f a.err a.o65 a.hex b.o65 b.hex

