	.word $c000
	* = $c000
#if 0
#error BUGGGGG
#endif

#if 1
	lda #93
	jsr $ffd2
#endif

/* comment this out to stop testing included gaffes */
#ifdef BUG
#include "test.inc"
#endif

#if 1
#if 2
	lda #93
	jsr $ffd2
#endif
#endif

#ifdef X
/* comment this out for bugs in this file */
#endif

