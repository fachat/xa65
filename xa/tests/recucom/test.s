	*=$1000

	lda #0

	; recursive comments (may / should?) go away with 2.4

/*
	sta $2000
	/* commented
	lda #1
	*/
*/

	; C++ comments

//	sta $2001


	; quotes in comments

	lda #2		/* this is '$' */
	lda #3		/* this is "$" */
	lda #4 		/* this is "\"" */
	lda #5		/* this is '"' */

	; examples in preprocessor statements

#define	E_ILLNAM	<-34	/* illegal name (joker "*", "?", "\"")	*/

	/* TODO: interpret "\" escape codes, shell variables */



