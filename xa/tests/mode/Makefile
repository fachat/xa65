
FILES=at1.o65 at2.o65 at4.o65 at256.o65 ad1.o65 ad2.o65 ad4.o65 ad256.o65 ab1.o65 ab2.o65 ab4.o65 ab256.o65

VERBOSE=
#VERBOSE=-v

XA=../../xa
LDO=../../ldo65

all: test1 test2 test3 test4

%.o65: %.a65
	${XA} -R -o $@ $<

# test with files in order of increasing align
test1: ${FILES}
	${LDO} ${VERBOSE} -o $@.o65 $^
	cmp $@.o65 $@.ok


# test with files in order of decreasing align
test2: ${FILES}
	${LDO} ${VERBOSE} -o $@.o65 ab256.o65 ab4.o65 ab2.o65 ab1.o65 ad256.o65 ad4.o65 ad2.o65 ad1.o65 at256.o65 at4.o65 at2.o65 at1.o65
	cmp $@.o65 $@.ok

# test with files in order of increasing align, not starting at align=1
test3: ${FILES}
	${LDO} ${VERBOSE} -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65
	cmp $@.o65 $@.ok

# test with files in order of increasing align, with non-aligned segment addresses
test4: ${FILES}
	${LDO} ${VERBOSE} -bt 1025 -bd 1025 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -bt 1026 -bd 1026 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -bt 1027 -bd 1027 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -bt 1028 -bd 1028 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1

clean:
	rm -f *.o65
	
