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

#if(0)
/* enable when backslashed quotes are supported in 2.4 */
	.asc "PASS\"\\PASS/*PASS*///PASS//\"\\PASS"
#endif

	.asc "SIE/*256*/N"
	/* .asc "SIE/*256*/N"
	.asc "FAIL"
	*/ .asc "PASS" // .asc "FAIL" /* .asc "FAIL*/" */

