default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa test.inc || exit 0 && exit 1
	# expected-to-fail tests did fail. should be no more errors now.
	../../xa -DBUG test.s -o test.o
	../hextool -cmp=ok < test.o
	../../xa test.s -o test.o
	../hextool -cmp=ok < test.o

clean:
	rm -f *.o
