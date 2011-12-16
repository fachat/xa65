	.word $9000
	* = $9000

	w = 10

glorb

.dsb 9,10			; +9
.byt 6, 6, 6, "devil", 'Q'	; +9 => 18
.bin 0,5,'README.1st'		; +5 => 23
.bin w,w+256,'README.1st'	; +266 => 289 ($0121)

gleeb
	; $0123
	jmp glorb
	; should be $9121 (remember the SA)
	jmp gleeb
	bne *

