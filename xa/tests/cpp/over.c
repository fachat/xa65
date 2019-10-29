/* #define BUG */
#ifdef BUG
#define WW AA
#define AA WW
#else
#define CC 1
#define WW CC
#define AA WW
#endif

/* This has a .c extension for those cc -E's that won't deal with .asm */

	* = $0000

	lda #AA
	jmp buggy
	rts

#include "over.h"

/* the buggy will force a line number to be printed */

buggy
