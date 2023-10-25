default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa bad.s || exit 0 && exit 1
	../../xa bad2.s || exit 0 && exit 1
	../../xa bad3.s || exit 0 && exit 1
	../../xa bad4.s || exit 0 && exit 1
	# expected-to-fail tests did fail. should be no more errors now.
	../../xa good.s -o good.o
	../hextool -cmp=good.ok < good.o

clean:
	rm -f *.o
