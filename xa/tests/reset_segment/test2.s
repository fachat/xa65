
	; test absolute code embedded into relocatable mode

	.text

	lda foo
	lda bar

	; go into absolute mode
	*=$1234

foo	.asc "absolute",0

	lda foo
	lda bar

	; go back into relocatble mode
	*=

bar	.asc "reloc",0
	


