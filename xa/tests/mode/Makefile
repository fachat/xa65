
FILES=at1.o65 at2.o65 at4.o65 at256.o65 ad1.o65 ad2.o65 ad4.o65 ad256.o65 ab1.o65 ab2.o65 ab4.o65 ab256.o65

VERBOSE=
#VERBOSE=-v

XA=../../xa
LDO=../../ldo65
RELOC=../../reloc65
FILE=../../file65

all: test1.o65 test2.o65 test3.o65 test4 cpus relocsame overwrite linkup1.tmp linkup2

.SUFFIXES: .a65 .o65

.a65.o65:
	${XA} -R -o $@ $?

# test with files in order of increasing align
test1.o65: ${FILES}
	${LDO} ${VERBOSE} -o $@ ${FILES}
	../hextool -cmp=$@.ok $@

# test with files in order of decreasing align
test2.o65: ${FILES}
	${LDO} ${VERBOSE} -o $@ ab256.o65 ab4.o65 ab2.o65 ab1.o65 ad256.o65 ad4.o65 ad2.o65 ad1.o65 at256.o65 at4.o65 at2.o65 at1.o65
	../hextool -cmp=$@.ok $@

# test with files in order of increasing align, not starting at align=1
test3.o65: ${FILES}
	${LDO} ${VERBOSE} -o $@ ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65
	../hextool -cmp=$@.ok $@

# test with files in order of increasing align, with non-aligned segment addresses
test4: ${FILES}
	${LDO} ${VERBOSE} -bt 1025 -bd 1025 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -bt 1026 -bd 1026 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -bt 1027 -bd 1027 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -bt 1028 -bd 1028 -o $@.o65 ab2.o65 at2.o65 ad2.o65 ab4.o65 at4.o65 ad4.o65 ad256.o65 at256.o65 ab256.o65 || exit 0 && exit 1

#c6502: at2.o65 ad4.o65
#	for i in $^; do ${RELOC} -C 6502 -o $@-$$i $$i; done
#	cmp $@-at2.o65 $@-at2.ok
#	cmp $@-ad4.o65 $@-ad4.ok
c6502: c6502-at2.o65 c6502-ad4.o65
c6502-at2.o65: at2.o65
	${RELOC} -C 6502 -o $@ $?
	../hextool -cmp=$@.ok $@
c6502-ad4.o65: ad4.o65
	${RELOC} -C 6502 -o $@ $?
	../hextool -cmp=$@.ok $@

#c65c02: at2.o65 ad4.o65
#	for i in $^; do ${RELOC} -C 65C02 -o $@-$$i $$i; done
#	cmp $@-at2.o65 $@-at2.ok
#	cmp $@-ad4.o65 $@-ad4.ok
c65c02: c65c02-at2.o65 c65c02-ad4.o65
c65c02-at2.o65: at2.o65
	${RELOC} -C 65C02 -o $@ $?
	../hextool -cmp=$@.ok $@
c65c02-ad4.o65: ad4.o65
	${RELOC} -C 65C02 -o $@ $?
	../hextool -cmp=$@.ok $@

#c65ce02: at2.o65 ad4.o65
#	for i in $^; do ${RELOC} -C 65CE02 -o $@-$$i $$i; done
#	cmp $@-at2.o65 $@-at2.ok
#	cmp $@-ad4.o65 $@-ad4.ok
c65ce02: c65ce02-at2.o65 c65ce02-ad4.o65
c65ce02-at2.o65: at2.o65
	${RELOC} -C 65CE02 -o $@ $?
	../hextool -cmp=$@.ok $@
c65ce02-ad4.o65: ad4.o65
	${RELOC} -C 65CE02 -o $@ $?
	../hextool -cmp=$@.ok $@

#c65sc02: at2.o65 ad4.o65
#	for i in $^; do ${RELOC} -C 65SC02 -o $@-$$i $$i; done
#	cmp $@-at2.o65 $@-at2.ok
#	cmp $@-ad4.o65 $@-ad4.ok
c65sc02: c65sc02-at2.o65 c65sc02-ad4.o65
c65sc02-at2.o65: at2.o65
	${RELOC} -C 65SC02 -o $@ $?
	../hextool -cmp=$@.ok $@
c65sc02-ad4.o65: ad4.o65
	${RELOC} -C 65SC02 -o $@ $?
	../hextool -cmp=$@.ok $@

#c65816: at2.o65 ad4.o65
#	for i in $^; do ${RELOC} -C 65816 -o $@-$$i $$i; done
#	cmp $@-at2.o65 $@-at2.ok
#	cmp $@-ad4.o65 $@-ad4.ok
c65816: c65816-at2.o65 c65816-ad4.o65
c65816-at2.o65: at2.o65
	${RELOC} -C 65816 -o $@ $?
	../hextool -cmp=$@.ok $@
c65816-ad4.o65: ad4.o65
	${RELOC} -C 65816 -o $@ $?
	../hextool -cmp=$@.ok $@

#n6502: at2.o65 ad4.o65
#	for i in $^; do ${RELOC} -C NMOS6502 -o $@-$$i $$i; done
#	cmp $@-at2.o65 $@-at2.ok
#	cmp $@-ad4.o65 $@-ad4.ok
n6502: n6502-at2.o65 n6502-ad4.o65
n6502-at2.o65: at2.o65
	${RELOC} -C NMOS6502 -o $@ $?
	../hextool -cmp=$@.ok $@
n6502-ad4.o65: ad4.o65
	${RELOC} -C NMOS6502 -o $@ $?
	../hextool -cmp=$@.ok $@

cpus: c65816 c6502 n6502 c65sc02 c65c02 c65ce02
	# prereq for the following

relocsame: cpus
	${RELOC} -o $@.o65 c65816-ad4.o65
	cmp $@.o65 c65816-ad4.o65
	${RELOC} -o $@.o65 c65sc02-at2.o65
	cmp $@.o65 c65sc02-at2.o65

overwrite: 
	# overwrite 65816 with nmos 6502
	${RELOC} -C NMOS6502 -o $@-1.o65 c65816-at2.o65
	cmp $@-1.o65 n6502-at2.o65
	# overwrite 65sc02 with 65c02
	${RELOC} -C 65C02 -o $@-2.o65 c65sc02-ad4.o65
	cmp $@-2.o65 c65c02-ad4.o65

linkup1.tmp: 
	${LDO} -v -o $@.tmp c6502-ad4.o65 c65c02-at2.o65 c65sc02-ad4.o65
	${FILE} $@.tmp > $@
	../hextool -cmp=$@.ok $@

linkup2: 
	# incompatible links
	${LDO} ${VERBOSE} -o $@.o65 c6502-ad4.o65 c65c02-at2.o65 c65816-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 c6502-ad4.o65 c65ce02-at2.o65 c65816-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 c6502-ad4.o65 c65816-at2.o65 c65c02-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 c6502-ad4.o65 c65816-at2.o65 c65ce02-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 c65816-at2.o65 c65ce02-ad4.o65 c65c02-at2.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 n6502-at2.o65 c65ce02-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 n6502-at2.o65 c65c02-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 n6502-at2.o65 c65sc02-ad4.o65 || exit 0 && exit 1
	${LDO} ${VERBOSE} -o $@.o65 n6502-at2.o65 c65816-ad4.o65 || exit 0 && exit 1

clean:
	rm -f *.o65 *.tmp
	
