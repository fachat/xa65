
all: t

1.o65: 1.s
	xa -R -c -o 1.o65 1.s
	hexdump -C 1.o65 > 1.o65.hex

2.o65: 2.s
	xa -R -c -o 2.o65 2.s
	hexdump -C 2.o65 > 2.o65.hex

linked.o65: 1.o65 2.o65
	ldo65 -o linked.o65 1.o65 2.o65
	hexdump -C linked.o65 > linked.o65.hex

t: linked.o65
	reloc65 -bt 32768 -xt -o t linked.o65
	hexdump -C t > t.hex
	diff t t.ok

clean:
	rm -f *.o65 *.hex t

