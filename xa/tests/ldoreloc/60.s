
	.zero
ptr	.word 0
	.text

foo	=$1234

	lda ptr2 
loop: 	jmp loop

	.data
bar	.word bla

