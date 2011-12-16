	.word $0400
	* = $0400

q = $0005
fizz
/* these should be optimized to zero page */
	sta $05
	sta $0005
	sta q
	dec q
	inc q
	lda <$0005
/* these should not */
	stx !$0005
	sta !q
	sta $8765
	stx $919c
	inc !q
	dec $8342
	sta $4431
	inc $79e0
	sty $1b0a
	jsr $ffe4
	jmp $fce2
	jmp breadbox
	lsr $2020
	bne fizz
	rts

breadbox	rts
		beq fizz
		rts

