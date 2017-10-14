	.word $9000
	* = $9000

; test specific single characters can be quoted

	LDA #';'
	CMP #':'
	AND #'"'
	SBC #";"
	ORA #":"
	EOR #"'"
