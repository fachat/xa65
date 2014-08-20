	.word $9000
	* = $9000

	w = 10
.bin 0,10+w,"README.1st"
.bin 0,10,"README.1st"
#include "test2.s"
.byt "FooBar"
.aasc "FooBar"
.asc "FooBar",65,97,10
.aasc "FooBar"
.bin 0,10,"README.1st"
.aasc "Barfoo",65,97,10

lda #'A'
lda #"A"
