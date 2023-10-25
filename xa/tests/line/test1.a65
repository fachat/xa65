
/*
 * tests the internal line presentation getting over 128
 */

#define	MAX_ENDPOINTS	4

	*=$c000

txmax   .dsb MAX_ENDPOINTS, 0   ; max transfer length per endpoint
txpos   .dsb MAX_ENDPOINTS, 0   ; endpoint buffer position per endpoint, calculated at usbd_start
txlen   .dsb MAX_ENDPOINTS, 0   ; endpoint buffer length, set per transaction

/*
 * tests the internal line presentation getting over 256
 */


txpos2   .byt MAX_ENDPOINTS, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0   ; endpoint buffer position per endpoint, calculated at usbd_start and even more comments, but the many values alone use 5 byte per value internally...

