
#define X W
#define W 5

fuzz	lda #W
	sta $0400
	bne fuzz
	rts

	lda @$c0c0c0
