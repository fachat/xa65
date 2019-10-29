default:
	../../xa test.s
	../hextool -cmp=no-charset.o < a.o65
	../../xa -O PETSCII test.s
	../hextool -cmp=petscii.o < a.o65
	../../xa -O PETSCREEN test.s
	../hextool -cmp=petscreen.o < a.o65
	../../xa -O HIGH test.s
	../hextool -cmp=high.o < a.o65

clean:
	rm -rf a.o65
