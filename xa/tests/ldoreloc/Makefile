
default: all
all: t1 t2 t10 t11 t20 t21 t30 t31 clean

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

linked11.o65: 10.o65 2.o65
	../../ldo65 -o $@ 2.o65 10.o65

linked20.o65: 20.o65 2.o65
	../../ldo65 -o $@ 2.o65 20.o65

linked21.o65: 20.o65 2.o65
	../../ldo65 -o $@ 20.o65 2.o65

linked30.o65: 30.o65 31.o65
	../../ldo65 -o $@ 30.o65 31.o65

linked31.o65: 30.o65 31.o65
	../../ldo65 -o $@ 31.o65 30.o65

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

t11: linked11.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t11.ok

t20: linked20.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t20.ok

t21: linked21.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t21.ok

t30: linked30.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t30.ok

t31: linked31.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool $@ > $@.hex
	../hextool -cmp=$@ < t31.ok

clean:
	rm -f *.o65 *.hex t1 t2 t10 t11 t20 t21 t30 t31

