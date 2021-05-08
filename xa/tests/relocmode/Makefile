default: test1 

test1:
	# this should fail in non-relocating mode
	../../xa  test1.s -o test1.o || exit 0 && exit 1
	../../xa  -R test1.s -o test1.o 
	../hextool -cmp=ok < test1.o

clean:
	rm -f *.o
