	.word $c000
	* = $c000

	lda #147
; depending on the -M option:lda #65
	jsr $ffd2:rts	; maybe some stuff out here:tay

; there will be:cli
; extra code:sei
; or not:rti
; let's ; be ; tricky : ; does it ; work? :nop:nop: ; let's see! ; rts

/* hey,
	; what about
		now?
	: brk */
// do I make you sexy? ; :brk
	rti
