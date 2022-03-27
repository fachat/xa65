; test for the xa .xl opcode that should set the XR handling to 16 bit

	*=$f000

	; set X/Y registers to 16 bit ...
	rep #%00010000
	; ... and tell the assembler about it

	.al
	lda #$1234
