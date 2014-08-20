default:
	# in 6502 mode, it will fail.
	../../xa test.s || exit 0 && exit 1
	# in 65816 mode, it will pass.
	../../xa -w test.s

clean:
	rm -f a.o65
