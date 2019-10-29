#define AA 1
#define POKE(a,b) lda #b: sta a
#define WW AA

start
        ; next line works
        POKE(1, 2)
        ; next two lines work
        lda #AA
        sta 2
        ; next line fails
        POKE(AA, 2)
	POKE(WW, 2)
	lda #WW
	sta 2

#define WW 94
	POKE(WW, 2)
#define WW 5
#define AA WW
#define POKE(a,b) lda #a: sta b
	POKE(AA, 3)

	bne start
	rts

/* this should bug out */

#ifdef FAIL
	POKE(NOGOOD, 7)
#endif
