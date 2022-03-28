#define HI(z) (((z) >> 16) & $FFFF)
#define LO(z) ((z) & $FFFF)

     .al
     .xl
     *=$112233
     SYM:
         rts
	jmp SYM
	jmp $112233
	lda $112233
	lda @$2233
	lda $2233
	lda $33

#print HI(SYM)
         lda #HI(SYM)
         pha
#print LO(SYM)
         lda #LO(SYM)
         pha

