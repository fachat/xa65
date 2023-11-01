default: test1.a65 test2.a65 test1.o65 test2.o65
	../../xa -XC -R -U test1.a65
	../hextool -cmp=test1.o65 < a.o65
	../../xa -R -U test2.a65
	../hextool -cmp=test2.o65 < a.o65

clean:
	rm -f a.o65
