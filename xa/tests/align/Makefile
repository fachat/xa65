
XA=../../xa

default: all
all: t01 t02 t03 t11 t12 t13 t21 t22 t23 t31 t32 t33 t41 t42 t43

# BSD only has suffix rules

.SUFFIXES: .o65 .hex .a65

#%.o65: %.s
.a65.o65:
	${XA} -R -c -o $@ $<

#%.hex: %.o65
.o65.hex:
	../hextool $< > $@

###############################################
# text segment correctly aligned

t01: t01.o65
	cmp $@.o65 $@.ok

t02: t02.o65
	cmp $@.o65 $@.ok

t03: t03.o65
	cmp $@.o65 $@.ok

###############################################
# data segment correctly aligned

t11: t11.o65
	cmp $@.o65 $@.ok

t12: t12.o65
	cmp $@.o65 $@.ok

t13: t13.o65
	cmp $@.o65 $@.ok

###############################################
# text segment incorrectly aligned

t21: t01.a65
	${XA} -R -c -bt 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

t22: t02.a65
	${XA} -R -c -bt 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

t23: t03.a65
	${XA} -R -c -bt 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

###############################################
# text segment correctly aligned, but data segment not (even if empty)

t31: t01.a65
	${XA} -R -c -bd 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

t32: t02.a65
	${XA} -R -c -bd 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

t33: t03.a65
	${XA} -R -c -bd 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

###############################################
# data segment incorrectly aligned

t41: t11.a65
	${XA} -R -c -bd 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

t42: t12.a65
	${XA} -R -c -bd 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok

t43: t13.a65
	${XA} -R -c -bd 1025 -o $@.o65 $< 2> $@.msg
	cmp $@.msg $@.mok
	cmp $@.o65 $@.ok


clean:
	rm -f *.o65 *.hex 

