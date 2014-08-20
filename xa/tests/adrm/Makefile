default:
	# xa should not allow this to happen. if it does, this test is no good.
	../../xa 816.asm || exit 0 && exit 1
	../../xa zab.asm || exit 0 && exit 1
	../../xa zpa.asm || exit 0 && exit 1
	../../xa -C c02.asm || exit 0 && exit 1
	# expected-to-fail tests did fail. should be no more errors now.
	../../xa -C 02.asm -o 02.o
	../hextool -cmp=02.ok < 02.o
	../../xa c02.asm -o c02.o
	../hextool -cmp=c02.ok < c02.o
	../../xa -w 816.asm -o 816.o
	../hextool -cmp=816.ok < 816.o
	../../xa -w zab.asm -o zab.o
	../hextool -cmp=zab.ok < zab.o
	../../xa -w zpa.asm -o zpa.o
	../hextool -cmp=zpa.ok < zpa.o

clean:
	rm -f *.o
