* = $e00
	.byt 10/2	// watchs // phlegm
	/* .word 10/2 */ .word 65536/2
	// .byt 9/9	// yo mama
	// .asc "FAIL"
    .asc "SIEx256SN" // foo
    .asc "SIE/256SN" // bar
    .asc "SIE//256SN" // bar
    .asc "SIEy256SN" // baz
	.asc "PASS/*PASS*///PASS//PASS"

	.asc "PASS\"\\PASS/*PASS*///PASS//\"\\PASS"

	.asc "SIE/*256*/N"
	/* .asc "SIE/*256
	.asc "FAIL"
	*/ .asc "PASS" // .asc "FAIL" /* .asc "FAIL*/" */

