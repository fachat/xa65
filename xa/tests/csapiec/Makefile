default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa -DBUG=1 -o test.o test.s || exit 0 && exit 1
	../../xa -R -DBUG=1 -o testseg.o test.s || exit 0 && exit 1
	# xa *should* allow this to happen.
	../../xa -o test.o test.s
	../hextool -cmp=test.ok < test.o
	../../xa -R -o testseg.o test.s
	../hextool -cmp=testseg.ok < testseg.o

clean:
	rm -f *.o
