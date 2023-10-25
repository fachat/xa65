
XA=../../xa
OBJS=test1 test2 test3 test4 test5 test6 test6a test6b test7

all: $(OBJS)

clean:
	rm -f a.o65 $(OBJS)

%: %.a65 %.o65
	${XA} -XC $< -o $@
	../hextool -cmp=$@.o65 < $@

