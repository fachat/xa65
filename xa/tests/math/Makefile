default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa -o divzero.o divzero.s || exit 0 && exit 1
	../../xa -o divzero2.o divzero2.s || exit 0 && exit 1

clean:
	rm -f *.o
