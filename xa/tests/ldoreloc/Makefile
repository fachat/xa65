
default: all
all: t

1.o65: 1.s
	../../xa -R -c -o 1.o65 1.s
	../hextool 1.o65 > 1.o65.hex

2.o65: 2.s
	../../xa -R -c -o 2.o65 2.s
	../hextool 2.o65 > 2.o65.hex

linked.o65: 1.o65 2.o65
	../../ldo65 -o linked.o65 1.o65 2.o65
	../hextool linked.o65 > linked.o65.hex

t: linked.o65
	../../reloc65 -bt 32768 -xt -o t linked.o65
	../hextool t > t.hex
	../hextool -cmp=t < t.ok

clean:
	rm -f *.o65 *.hex t

