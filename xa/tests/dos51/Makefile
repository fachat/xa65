default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa -DBUG test.s || exit 0 && exit 1
	# expected-to-fail tests did fail. should be no more errors now.
	../../xa test.s -o test.o
	../hextool -cmp=ok < test.o

clean:
	rm -f *.o
