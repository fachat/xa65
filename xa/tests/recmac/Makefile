default:
	# this should fail.
	../../xa -DFAIL test.asm || exit 0 && exit 1
	# these should not fail.
	../../xa test.asm
	../hextool -cmp=test.ok < a.o65
	../../xa testi.asm
	../hextool -cmp=testi.ok < a.o65

clean:
	rm -f a.o65

