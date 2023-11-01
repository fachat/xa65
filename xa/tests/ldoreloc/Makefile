
default: all
all: t1 t2 t10 t11 t20 t21 t30 t31 t40 t41 t50 t51 t60 t61 t62

%.o65: %.s
	../../xa -R -c -o $@ $<

%.hex: %.o65
	../hextool $< > $@

linked.o65: 1.o65 2.o65
	../../ldo65 -o $@ 1.o65 2.o65

linked2.o65: 1.o65 2.o65
	../../ldo65 -o $@ 2.o65 1.o65

linked10.o65: 10.o65 2.o65
	../../ldo65 -U -o $@ 10.o65 2.o65

linked11.o65: 10.o65 2.o65
	../../ldo65 -U -o $@ 2.o65 10.o65

linked20.o65: 20.o65 2.o65
	../../ldo65 -Ll1 -o $@ 2.o65 20.o65

linked21.o65: 20.o65 2.o65
	../../ldo65 -L l1 -o $@ 20.o65 2.o65

linked22.o65: 20.o65 2.o65
	../../ldo65 -o $@ 20.o65 2.o65 && exit 1 || exit 0

linked30.o65: 30.o65 31.o65
	../../ldo65 -o $@ 30.o65 31.o65

linked31.o65: 30.o65 31.o65
	../../ldo65 -o $@ 31.o65 30.o65

linked40.o65: 40.o65 41.o65
	../../ldo65 -o $@ 40.o65 41.o65

linked41.o65: 40.o65 41.o65
	../../ldo65 -o $@ 41.o65 40.o65

linked50.o65: 50.o65 51.o65
	../../ldo65 -o $@ 50.o65 51.o65

linked51.o65: 50.o65 51.o65
	../../ldo65 -o $@ 51.o65 50.o65

linked60.o65: 60.o65 61.o65
	../../ldo65 -o $@ 60.o65 61.o65

linked61.o65: 60.o65 61.o65
	../../ldo65 -o $@ 61.o65 60.o65

linked62.o65: 60.o65 61.o65
	../../ldo65 -bd 65529 -bt 65523 -bz 255 -o $@ 61.o65 60.o65 && exit 1 || exit 0

t1: linked.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t1.ok

t2: linked2.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t2.ok

t10: linked10.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t10.ok

t11: linked11.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t11.ok

t20: linked20.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t20.ok

t21: linked21.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t21.ok

t22: linked22.o65
	# should fail, so no action

t30: linked30.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t30.ok

t31: linked31.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t31.ok

t40: linked40.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t40.ok

t41: linked41.o65
	../../reloc65 -bt 32768 -xt -o $@ $<
	../hextool -cmp=$@ < t41.ok

t50: linked50.o65
	../../reloc65 -bt 32768 -bd 40960 -o $@ $<
	../hextool -cmp=$@ < t50.ok

t51: linked51.o65
	../../reloc65 -bt 32768 -bd 40960 -o $@ $<
	../hextool -cmp=$@ < t51.ok

t60: linked60.o65
	../../reloc65 -bt 32768 -bd 40960 -o $@ $<
	../hextool -cmp=$@ < t60.ok

t61: linked61.o65
	../../reloc65 -bt 32768 -bd 40960 -o $@ $<
	../hextool -cmp=$@ < t61.ok

t62: linked62.o65

clean:
	rm -f *.o65 *.hex t1 t2 t10 t11 t20 t21 t30 t31 t40 t41 t50 t51 t60 t61

