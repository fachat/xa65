
XA=../../xa
OBJS=test1.u test2.u test3.u test4.u test5.u test6.u test6a.u test6b.u test7.u

all: $(OBJS)

clean:
	rm -f a.o65 $(OBJS)

#%: %.a65 %.o65
#	${XA} -XC $< -o $@
#	../hextool -cmp=$@.o65 < $@

# BSD make only understands suffix rules

.SUFFIXES: .u .a65

.a65.u:
	${XA} -XC $< -o $@
	../hextool -cmp=$*.o65 < $@

