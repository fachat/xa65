# 1 "over.c"
 









 

	lda #1   
	jmp buggy
	rts

# 1 "over.h" 1




fuzz	lda #5 
	sta $0400
	bne fuzz
	rts

	lda @$c0c0c0
# 17 "over.c" 2


 
