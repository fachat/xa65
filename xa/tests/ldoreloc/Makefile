
default: all
#all: t1 t2 t10
all: t1 t2 

%.o65: %.s
	../../xa -R -c -o $@ $<

%.hex: %.o65
	../hextool $< > $@

linked.o65: 1.o65 2.o65
	../../ldo65 -o $@ 1.o65 2.o65

linked2.o65: 1.o65 2.o65
	../../ldo65 -o $@ 2.o65 1.o65

linked10.o65: 10.o65 2.o65
	../../ldo65 -o $@ 10.o65 2.o65

t1: linked.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t1.ok

t2: linked2.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t2.ok

t10: linked10.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t10.ok

clean:
	rm -f *.o65 *.hex t1 t2

