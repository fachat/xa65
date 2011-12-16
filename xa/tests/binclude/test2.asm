	.word $9000
	* = $9000

	w = 10

glorb

.dsb 9,10
.byt 6, 6, 6, "devil", 'Q'
.bin 0,109,'README.1st'
.bin w,w+119,'README.1st'

gleeb

	jmp glorb
	jmp gleeb
	bne *

