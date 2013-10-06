#
# Makefile for tests
#

XA=../../xa

tests: mixabs1 mixabs2 clean


mixabs1: mixabsolute.a65
	${XA} $<
	hexdump -C a.o65 > a.hex
	cmp mixabs1.out a.hex

mixabs2: mixabsolute.a65
	${XA} -R $<
	file65 -V a.o65
	reloc65 -bt 40960 -o b.o65 a.o65
	hexdump -C b.o65 > b.hex
	cmp mixabs2.out b.hex

clean:
#	rm -f a.err a.o65 a.hex b.o65 b.hex

