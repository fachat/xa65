#
# Makefile for tests
#

XA=../../xa
OBJS=undef2 undef3 undef5 undef6
tests: bannerf undef1 undef4 undef5f undef6f banner $(OBJS)


banner:
	@echo =======================================
	@echo All tests meant to fail have done so correctly;
	@echo The following tests should pass

bannerf:
	@echo =======================================
	@echo The following should fail

undef1: undef.a65
	@echo These should fail
	${XA} undef.a65 || exit 0 && exit 1
	${XA} -R undef.a65 || exit 0 && exit 1
	${XA} -R -U -DFAIL undef.a65 || exit 0 && exit 1
	${XA} -R -Ll1 -Ll3 -Ll4 -Ll5 undef.a65 || exit 0 && exit 1

undef2: undef.a65
	${XA} -R -Ll1 -Ll3 undef.a65 -o $@.tmp
	../../reloc65 -bt 40960 -o $@ $@.tmp
	../hextool -cmp=undef2.ok < $@

undef3: undef.a65
	${XA} -R -U undef.a65 -o $@.tmp
	../../reloc65 -bt 40960 -o $@ $@.tmp
	../hextool -cmp=undef2.ok < $@

undef4: undef4.a65
	@echo These should fail
	${XA} undef4.a65 || exit 0 && exit 1
	${XA} -R undef4.a65 || exit 0 && exit 1
	${XA} -R -Ll1 undef4.a65 || exit 0 && exit 1
	${XA} -R -U undef4.a65 || exit 0 && exit 1

undef5f: undef5.a65
	@echo These should fail
	${XA} undef5.a65 || exit 0 && exit 1
	${XA} -R undef5.a65 || exit 0 && exit 1
	${XA} -R -Ll1 undef5.a65 || exit 0 && exit 1

undef5: undef5.a65
	${XA} -R -U undef5.a65  -o $@
	../hextool -cmp=undef5.ok < $@

undef6f: undef6.a65
	@echo These should fail
	${XA} undef6.a65 || exit 0 && exit 1
	${XA} -R undef6.a65 || exit 0 && exit 1
	${XA} -R -Ll1 undef6.a65 || exit 0 && exit 1

undef6: undef6.a65
	${XA} -R -U undef6.a65  -o $@
	../hextool -cmp=undef6.ok < $@
	
clean:
	rm -f a.err a.o65 b.o65 *.tmp $(OBJS)

