	.word $0400
	* = $0400

q = $0005
r = $000005
fizz
/* these should be optimized to zero page */
	sta $05
	sta $0005
	sta q
	sta r
/* 16-bit */
	sta !$0005
	sta !q
	sta $8765
/* 24-bit */
	sta @q
	sta @$000005
	sta $876543
	rts

	jmp $fce2
	jmp $99fce2
	jmp breadbox
	jmp @breadbox

breadbox	rts
		bne fizz
		rts
