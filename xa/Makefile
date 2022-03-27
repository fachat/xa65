# Unix gcc or DOS go32 cross-compiling gcc
#
CC = gcc
LD = gcc
# for testing. not to be used; build failures in misc/.
#CFLAGS = -O2 -W -Wall -pedantic -ansi
CFLAGS = -O2
LDFLAGS = -lc

# for DOS?
# CC = gcc-go32
# LD = gcc-go32
# CFLAGS = -W -Wall -pedantic 

# Other cc
#CC = cc
#CFLAGS =
#LD = ld

DESTDIR = /usr/local

BINDIR = $(DESTDIR)/bin
MANDIR = $(DESTDIR)/share/man/man1
DOCDIR = $(DESTDIR)/share/doc

MKDIR = mkdir -p
INSTALL = install

all: killxa xa uncpk

killxa:
	rm -f xa

xa:
	(cd src && LD=${LD} CC="${CC} ${CFLAGS}" ${MAKE})

load:	
	(cd loader && CC="${CC} ${CFLAGS}" ${MAKE})

uncpk:
	(cd misc && CC="${CC} ${CFLAGS}" ${MAKE})

dos: clean
	(cd src && LD=gcc-go32 CC=gcc-go32 CFLAGS="-W -Wall -pedantic" ${MAKE})
	(cd misc && CC=gcc-go32 CFLAGS="-W -Wall -pedantic" ${MAKE})
	rm -f xa file65 ldo65 uncpk printcbm reloc65 mkrom.sh src/*.o

mingw: clean
	(cd src && LD=${LD} CC=${CC} CFLAGS="${CFLAGS}" LDFLAGS="" ${MAKE})
	(cd misc && LD=${LD} CC=${CC} CFLAGS="${CFLAGS}" LDFLAGS="" ${MAKE})
	
clean:
	(cd src && ${MAKE} clean)
	(cd loader && ${MAKE} clean)
	(cd misc && ${MAKE} mrproper)
	rm -f xa *.exe *.o65

install: xa uncpk
	$(MKDIR) $(BINDIR)
	$(MKDIR) $(MANDIR)
	$(INSTALL) xa reloc65 ldo65 file65 printcbm uncpk $(BINDIR)
	$(INSTALL) man/file65.1 man/ldo65.1 man/printcbm.1 man/reloc65.1 man/uncpk.1 man/xa.1 $(MANDIR)
	#$(MKDIR) $(DOCDIR)/xa65

dist: clean
	cd .. ; tar cvf xa-2.3.12.tar xa-2.3.12 ; gzip xa-2.3.12.tar

test: xa uncpk
	cd tests && ./harness -make="$(MAKE)" -cc="$(CC)" -cflags="$(CFLAGS)"
