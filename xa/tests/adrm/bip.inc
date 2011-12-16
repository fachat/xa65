/*

- tbasic.0.asm: if you make vecwri absolute with !, then the branch gets
        generated as if it were NOT absolute. works okay without it (and
        gets a warning)

*/

test	lda !$0095
	bne test

	ldx #13
lup0	lda @vecwri,x
	sta $2005,x
	dex
	bne lup0
	lda #$00
	sta $0020
	lda #$02
	sta $0021
	lda #$ff
	sta $0022
	lda #$13
	sta $0023
	jmp $2003
	
vectors	.byt $4c, $5a, $1e, $4c, $a0, $1e, $4c, $00, $01
vecwri	= vectors - 1
