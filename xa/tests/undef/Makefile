#
# Makefile for tests
#

XA=../../xa

tests: bannerf undef1 undef4 undef5f undef6f banner undef2 undef3 undef5 undef6 clean


banner:
	@echo =======================================
	@echo All tests meant to fail have done so correctly;
	@echo The following tests should pass

bannerf:
	@echo =======================================
	@echo The following should fail

undef1: undef.a65
	@echo These should fail
	${XA} $< || exit 0 && exit 1
	${XA} -R $< || exit 0 && exit 1
	${XA} -R -U -DFAIL $< || exit 0 && exit 1
	${XA} -R -Ll1 -Ll3 -Ll4 -Ll5 $< || exit 0 && exit 1

undef2: undef.a65
	${XA} -R -Ll1 -Ll3 $<
	reloc65 -bt 40960 -o b.o65 a.o65
	hexdump -C b.o65 > b.hex
	cmp undef2.out b.hex

undef3: undef.a65
	${XA} -R -U $<
	reloc65 -bt 40960 -o b.o65 a.o65
	hexdump -C b.o65 > b.hex
	cmp undef2.out b.hex

undef4: undef4.a65
	@echo These should fail
	${XA} $< || exit 0 && exit 1
	${XA} -R $< || exit 0 && exit 1
	${XA} -R -Ll1 $< || exit 0 && exit 1
	${XA} -R -U $< || exit 0 && exit 1

undef5f: undef5.a65
	@echo These should fail
	${XA} $< || exit 0 && exit 1
	${XA} -R $< || exit 0 && exit 1
	${XA} -R -Ll1 $< || exit 0 && exit 1

undef5: undef5.a65
	${XA} -R -U $< 
	hexdump -C a.o65 > a.hex
	cmp undef5.out a.hex

undef6f: undef6.a65
	@echo These should fail
	${XA} $< || exit 0 && exit 1
	${XA} -R $< || exit 0 && exit 1
	${XA} -R -Ll1 $< || exit 0 && exit 1

undef6: undef6.a65
	${XA} -R -U $< 
	hexdump -C a.o65 > a.hex
	cmp undef6.out a.hex
	
clean:
	rm -f a.err a.o65 a.hex b.o65 b.hex

