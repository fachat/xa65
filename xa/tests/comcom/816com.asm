:
	lda @$123456
	beq :+
	bne :-
:
	sta @$654321
