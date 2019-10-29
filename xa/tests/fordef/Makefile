default:
	# this must not pass.
	../../xa -DFAIL test3.asm || exit 0 && exit 1
	# these must pass.
	../../xa test3.asm
	../hextool -cmp=test3.ok < a.o65
	# testing warnings.
	../../xa -e test.out test.asm
	../hextool -cmp=ok.out < test.out
	../hextool -cmp=test1.ok < a.o65

clean:
	rm -f a.o65 test.out

