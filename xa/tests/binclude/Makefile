default:
	../../xa test.asm -o test.o
	../hextool -cmp=test.ok < test.o
	../../xa -w test.asm -o test816.o
	../hextool -cmp=test.ok < test816.o
	../../xa test2.asm -o test2.o
	../hextool -cmp=test2.ok < test2.o
	../../xa test3.asm -o test3.o
	../hextool -cmp=test.ok < test3.o

clean:
	rm -f *.o
