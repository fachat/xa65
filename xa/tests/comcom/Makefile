default:
	# this should fail.
	../../xa scomcom.asm || exit 0 && exit 1
	../../xa -a 816com.asm || exit 0 && exit 1
	../../xa -w 816com.asm || exit 0 && exit 1
	# it did fail. these should now all succeed.
	../../xa -XMASM scomcom.asm
	../hextool -cmp=scomcomm.ok < a.o65
	../../xa -a scomcom.asm
	../hextool -cmp=scomcomm.ok < a.o65
	../../xa comcom.asm
	../hextool -cmp=comcom.ok < a.o65
	../../xa -XMASM comcom.asm
	../hextool -cmp=comcomm.ok < a.o65
	../../xa -a comcom.asm
	../hextool -cmp=comcomm.ok < a.o65
	../../xa -a -w 816com.asm
	../hextool -cmp=816com.ok < a.o65

clean:
	rm -f a.o65

