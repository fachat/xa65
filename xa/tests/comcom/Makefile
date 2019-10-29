default:
	# this should fail.
	../../xa scomcom.asm || exit 0 && exit 1
	# it did fail. these should now all succeed.
	../../xa -M scomcom.asm
	../hextool -cmp=scomcomm.ok < a.o65
	../../xa comcom.asm
	../hextool -cmp=comcom.ok < a.o65
	../../xa -M comcom.asm
	../hextool -cmp=comcomm.ok < a.o65

clean:
	rm -f a.o65

