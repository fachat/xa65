
	; forward reference 
	; results in illegal ptr arithmetic when segment
	; is not reset between pass1 and pass2

	bne foo
foo

	.data



