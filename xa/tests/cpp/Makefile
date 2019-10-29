default:
	$(CC) -E over.c > over.asm
	../../xa -w over.asm
	../hextool -cmp=expected.o < a.o65

clean:
	rm -f over.asm a.o65
