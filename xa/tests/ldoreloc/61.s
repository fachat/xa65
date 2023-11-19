	jsr loop
bla: 	lda ptr
	lda #ptr

	.zero
ptr2	.byt 0

	.data

	.word foo
	.word bar
	.byte <foo
	.byte >foo


