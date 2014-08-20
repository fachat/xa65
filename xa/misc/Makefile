
XCBMLIB = ..

# -Wall -ansi et al. cause compile problems.
CFLAGS =  -O2

LIBS = #-lncurses -ltermcap -lm


all: ../mkrom.sh ../uncpk ../printcbm ../file65 ../reloc65 ../ldo65

../uncpk: uncpk.c
	${CC} ${CFLAGS} uncpk.c -o $(XCBMLIB)/uncpk

../printcbm: printcbm.c
	${CC} ${CFLAGS} printcbm.c -o $(XCBMLIB)/printcbm

../file65: file65.c
	${CC} ${CFLAGS} file65.c -o $(XCBMLIB)/file65

../ldo65: ldo65.c
	${CC} ${CFLAGS} ldo65.c -o $(XCBMLIB)/ldo65

../reloc65: reloc65.c
	${CC} ${CFLAGS} reloc65.c -o $(XCBMLIB)/reloc65

../mkrom.sh: mkrom.sh
	cp mkrom.sh ../mkrom.sh

lt1: lt1.a
	../xa -R -c -o lt1 lt1.a

lt2: lt2.a
	../xa -R -c -o lt2 lt2.a

lt: lt1 lt2
	../ldo65 -o lt lt1 lt2 

clean:
	rm -f *.o 

mrproper: clean
	rm -f ../uncpk ../printcbm ../file65 ../mkrom.sh ../reloc65 ../ldo65
	rm -f lt1 lt2 lt

