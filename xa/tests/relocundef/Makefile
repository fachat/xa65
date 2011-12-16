
all: a.o65 b.o65

a.o65: test1.o65
	echo "********* This should give a warning about an undefined relocation table entry"
	reloc65 -X test1.o65
	hexdump -C a.o65 > a.o65.hex

b.o65: test2.o65
	echo "********* This should NOT give a warning"
	reloc65 -X -o b.o65 test2.o65
	hexdump -C b.o65 > b.o65.hex

test1.o65: test1.a65
	xa -R -Lundefl -Ll2 -o test1.o65 test1.a65
	hexdump -C test1.o65 > test1.o65.hex

test2.o65: test2.a65
	xa -R -o test2.o65 test2.a65
	hexdump -C test2.o65 > test2.o65.hex

clean:
	rm -f test1.o65 a.o65 test1.o65.hex a.o65.hex 
	rm -f test2.o65 b.o65 test2.o65.hex b.o65.hex 

