# Unix gcc or DOS go32 cross-compiling gcc
#
VERS = 2.4.0
CC = gcc
LD = gcc
# for testing. not to be used; build failures in misc/.
#CFLAGS = -O2 -W -Wall -pedantic -ansi
#CFLAGS = -O2 -g
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

TESTS=ALL

all: killxa xa uncpk

killxa:
	rm -f xa

xa:
	(cd src && LD=${LD} CC="${CC} ${CFLAGS}" ${MAKE})

#load:	
#	(cd loader && CC="${CC} ${CFLAGS}" ${MAKE})

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
	#(cd loader && ${MAKE} clean)
	(cd misc && ${MAKE} mrproper)
	rm -f xa *.exe *.o65 *.s core

install: all
	$(MKDIR) $(BINDIR)
	$(MKDIR) $(MANDIR)
	$(INSTALL) xa reloc65 ldo65 file65 printcbm uncpk $(BINDIR)
	$(INSTALL) man/file65.1 man/ldo65.1 man/printcbm.1 man/reloc65.1 man/uncpk.1 man/xa.1 $(MANDIR)
	#$(MKDIR) $(DOCDIR)/xa65

dist: clean
	cd .. ; tar cvf xa-$(VERS).tar xa-$(VERS) ; gzip xa-$(VERS).tar

# no prereqs to force parallel make to play nice
test: 
	rm -rf xa
	$(MAKE) xa
	$(MAKE) uncpk
	cd tests && ./harness \
		-tests="$(TESTS)" \
		-cc="$(CC)" -cflags="$(CFLAGS)"  \
		-make="$(MAKE)" -makeflags="$(MAKEFLAGS)"
