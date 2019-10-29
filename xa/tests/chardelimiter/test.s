
	*=$033c

	lda #"A"
	lda #'A'

	.asc "12345"
	.asc '12345'

	.asc "^""	; ^ is escape character
	.asc '^'' 
	.asc "'"
	.asc '"'

	.asc "^n"

