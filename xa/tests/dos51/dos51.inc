/* dos wedge code for inlining into routines */

/* define SA51 to allow stand-alone generation */
#ifdef SA51
	.word SA51
	* = SA51
#endif
/* otherwise assume this has been inlined via #include <> */

/* define BASIC51 to use BASIC interface stub */
#ifdef BASIC51
	jsr $aefd
	jsr $ad9e
	jsr $b6a3	; leaving a=len x=lb y=hb
#endif	
/* otherwise load a with length, x/y = lb/hb */

.(

crunchsrv	= $a57c
setlfs		= $ffba
setnam		= $ffbd
open		= $ffc0
close		= $ffc3
clall		= $ffe7
chkin		= $ffc6
chkout		= $ffc9
clrchn		= $ffcc
chrin		= $ffcf
getin		= $ffe4
print		= $ffd2
readst		= $ffb7
chrout		= print
ready		= $e37b
cnvrtdec	= $bdcd
erasealine	= 59903
workzp		= $a5

	sta work1
	sty workzp+1
	stx workzp
	; set the filename (directory, error channel and commands)
	; a, x and y already setup for us
	jsr setnam

	; are we requesting just the error channel?
	ldx work1
	beq errchn	 ; yes

	; are we requesting a disk directory?
	ldy #0
	lda (workzp),y
	cmp #"$"
	bne errchn	; no

	; yes, display directory
	lda #$01
	ldx 186
	ldy #$00
	jsr setlfs
	jsr open	; open 1,dev,0 to read as BASIC formatted text

	lda #147	
	jsr print	
	clc
	ldx #01
	jsr chkin
	bcs dirdone
	jsr chrin
	jsr chrin
	lda #$0d
	jsr print
	; routine to print each line of the directory
dirpll	jsr chrin	
	jsr chrin	; skip line link
	jsr chrin	; grab length (as line #)
	sta work1
	jsr chrin
	sta work2	; stash for later
	jsr readst	; EOF or other error?
	bne dirdone	; yes, end directory
	lda work2	; no, print this line
	ldx work1
	jsr cnvrtdec	; first the number/length
	lda #$20
	jsr print	; and a space
	jsr chrin
dirpl	jsr print	; then the filename in quotes and filetype until a null
	jsr chrin
	adc #0
	bne dirpl	; no null yet
	lda #13		; yes, null, print CR
	jsr print
	jmp dirpll	; and do next line
	; finish directory and clear channel, then fall through to errchn
dirdone	lda #1	
	jsr close
	jsr clall
	lda #0
	jsr setnam

	; common routine for error channel and disk commands
errchn	lda #15
	ldx $ba
	ldy #15
	jsr setlfs	; open 15,8,15,command
	jsr open

	lda #18
	jsr print	; rvson
	; read until there's a null, adding $0d for devices that don't
errchnl	ldx #15
	jsr chkin
	jsr chrin
	sta work1
	jsr readst		; power 64 sucks sometimes =-(
	sta work2		; its emulated messages have no $0d
	lda work1
	jsr print
	lda work2
	beq errchnl
	lda work1
	cmp #13
	beq errchnd
	lda #13
	jsr print
errchnd	lda #15
	jsr close
	jsr clall
	lda #146
	jsr print	; rvsoff
	rts

work1	.byt 0
work2	.byt 0
.)
