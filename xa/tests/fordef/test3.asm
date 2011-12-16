	.word $0400
	*=$0400

/* define this if you want to crash and burn */

#ifdef FAIL
	jmp `forward3
	bne `forward3
	lda (`forward1),y
	lda (`forward3),y
#echo congrats, you have FAILED!
#endif

	sta `forward3

/* this looks like it should fail, but won't because there is no ambiguity */

	lda (forward1),y
	jmp forward3

#include "test2.asm"
