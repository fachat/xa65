
default:
	# in 6502 mode, it will fail.
	../../xa test.a65 || exit 0 && exit 1
	# in 65816 mode, it will pass.
	../../xa -w test.a65

clean:
	rm -f a.o65

tests:
	./runtest.sh -q -C

