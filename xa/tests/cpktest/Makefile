default:
	../../xa -l test.l -o test.o pack_eng.a65
	../hextool -cmp=test.l < english.l || exit 1
	../hextool -cmp=eng.ok < test.o

	../../xa -l test.l -o test.o pack_ger.a65
	../hextool -cmp=test.l < deutsch.l || exit 1
	../hextool -cmp=de.ok < test.o
clean:
	rm -f *.o test.l
