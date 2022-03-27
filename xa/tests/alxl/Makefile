default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa alxl.s || exit 0 && exit 1
	../../xa alxlo1.s || exit 0 && exit 1
	../../xa alxlo2.s || exit 0 && exit 1
	../../xa alxlx1.s || exit 0 && exit 1
	../../xa alxlx2.s || exit 0 && exit 1
	../../xa alxlx3.s || exit 0 && exit 1
	../../xa sizes.s || exit 0 && exit 1
	../../xa -w alxlx1.s || exit 0 && exit 1
	../../xa -w alxlx2.s || exit 0 && exit 1
	../../xa -w alxlx3.s || exit 0 && exit 1
	# expected-to-fail tests did fail. should be no more errors now.
	../../xa -w alxl.s -o alxl.o
	../hextool -cmp=alxl.ok < alxl.o
	../../xa -w alxlo1.s -o alxlo1.o
	../hextool -cmp=alxlo1.ok < alxlo1.o
	../../xa -w alxlo2.s -o alxlo2.o
	../hextool -cmp=alxlo2.ok < alxlo2.o
	../../xa -w sizes.s -o sizes.o
	../hextool -cmp=sizes.ok < sizes.o

clean:
	rm -f *.o
