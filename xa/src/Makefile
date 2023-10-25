OBJ = xa.o xaa.o xal.o xap.o xat.o xar.o xar2.o xao.o xau.o xam.o xacharset.o xalisting.o

#CFLAGS=-W -Wall -pedantic -ansi -g
#CFLAGS=-W -Wall -ansi -O2
#CFLAGS=-g -std=c11 -D_GNU_SOURCE
#LD = ${CC}
#LDFLAGS = "-lc"

all: xa

xa: ${OBJ}
	${LD} -o ../xa ${OBJ} ${LDFLAGS}

clean: 
	rm -f *.o *.o65

mrproper: clean
	rm -f ../xa

