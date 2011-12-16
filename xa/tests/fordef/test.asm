	.word $0400
	* = $0400

/* this should generate optimizer warnings */

	lda forward1
	sta forward2

/* this shouldn't */

	jmp forward3

/* and this won't */

t1	lda `forward1
	sta `forward2
	rts

#include "test2.asm"

