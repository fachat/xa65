/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 * maintained by Cameron Kaiser
 *
 * Core tokenizing module/pass 1 and pass 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* enable this to turn on (copious) optimization output */
/* #define DEBUG_AM */
#undef LISTING_DEBUG

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "xad.h"
#include "xah.h"
#include "xah2.h"

#include "xar.h"
#include "xa.h"
#include "xaa.h"
#include "xal.h"
#include "xat.h"
#include "xao.h"
#include "xap.h"
#include "xacharset.h"
#include "xalisting.h"

int dsb_len = 0;

static int t_conv(signed char*,signed char*,int*,int,int*,int*,int*,int,int*);
static int t_keyword(signed char*,int*,int*);
static int tg_asc(signed char*,signed char*,int*,int*,int*,int*,int);
static void tg_dez(signed char*,int*,int*);
static void tg_hex(signed char*,int*,int*);
static void tg_oct(signed char*,int*,int*);
static void tg_bin(signed char*,int*,int*);
static int t_p2(signed char *t, int *ll, int fl, int *al);
//static void do_listing(signed char *listing, int listing_len, signed char *bincode, int bincode_len);

void list_setbytes(int number_of_bytes_per_line);

/* assembly mnemonics and pseudo-op tokens */
/* ina and dea don't work yet */
/* Note AF 20110624: added some ca65 compatibility pseudo opcodes,
 * many are still missing (and will most likely never by supported in this
 * code base). Potential candidates are .hibytes, .lobytes, .asciiz,
 * .addr, .charmap, .dbyt, .faraddr, .bankbytes, .segment (at least for the known ones)
 * .incbin is similar to our .bin, but with parameters reversed (argh...)
 * I like the .popseg/.pushseg pair; 
 * .global/.globalzp is equivalent to forward-defining a label in the global block
 * .export/.exportzp could be implemented with a commandline switch to NOT export
 * global labels, where .exported labels would still be exported in an o65 file.
 */
char *kt[] ={ 
/*    1     2     3    4      5     6      7   8      9     10   */
     "adc","and","asl","bbr","bbs","bcc","bcs","beq","bit","bmi",
     "bne","bpl","bra","brk","bvc","bvs","brl","clc","cld","cli",
/*
     "clv","cmp","cpx","cpy","cop","dea","dec","dex","dey","eor",
*/
     "clv","cmp","cpx","cpy","cop",/*"dea",*/"dec","dex","dey","eor",

/*
     "ina","inc","inx","iny","jmp","jsr","lda","ldx","ldy","lsr",
*/
     /*"ina",*/"inc","inx","iny","jmp","jsr","lda","ldx","ldy","lsr",
     "mvp","mvn","nop","ora","pha","php","phx","phy","pla","plp",
     "plx","ply","phb","phd","phk","plb","pld","pea","pei","per",

     "rmb","rol","ror","rti","rts","rep","rtl","sbc","sec","sed",
     "sei","smb","sta","stx","sty","stz","sep","stp","tax","tay",
     "trb","tsb","tsx","txa","txs","tya","txy","tyx","tcd","tdc",

     "tcs","tsc","wai","wdb","xba","xce", 

     ".byt",".word",".asc",".dsb", ".(", ".)", "*=", ".text",".data",".bss",
     ".zero",".fopt", ".byte", ".end", ".list", ".xlist", ".dupb", ".blkb", ".db", ".dw",
     ".align",".block", ".bend",".al",".as",".xl",".xs", ".bin", ".aasc", ".code",
     ".include", ".import", ".importzp", ".proc", ".endproc",
     ".zeropage", ".org", ".reloc", ".listbytes",
     ".scope", ".endscope"

};

/* arithmetic operators (purely for listing, parsing is done programmatically */
char *arith_ops[] = {
	"", "+", "-", 
	"*", "/", 
	">>", "<<", 
	"<", ">", "="
	"<=", ">=", "<>", 
	"&", "^", "|",
	"&&", "||"
};

/* length of arithmetic operators indexed by operator number */
static int lp[]= { 0,1,1,1,1,2,2,1,1,1,2,2,2,1,1,1,2,2 };

/* index into token array for pseudo-ops */
/* last valid mnemonic */
#define   Lastbef   93

#define   Kbyt      Lastbef+1
#define   Kword     Lastbef+2
#define   Kasc      Lastbef+3
#define   Kdsb      Lastbef+4
#define   Kopen     Lastbef+5  	/* .(      */
#define   Kclose    Lastbef+6  	/* .)      */
#define   Kpcdef    Lastbef+7  	/* *=value */
#define	  Ktext     Lastbef+8
#define	  Kdata     Lastbef+9
#define	  Kbss      Lastbef+10
#define	  Kzero     Lastbef+11
#define	  Kfopt     Lastbef+12
#define	  Kbyte	    Lastbef+13	/* gets remapped to Kbyt */
#define	  Kend      Lastbef+14	/* ignored (MASM compat.) */
#define	  Klist     Lastbef+15	/* ignored (MASM compat.) */
#define	  Kxlist    Lastbef+16	/* ignored (MASM compat.) */
#define	  Kdupb     Lastbef+17	/* gets remapped to Kdsb */
#define	  Kblkb     Lastbef+18	/* gets remapped to Kdsb */
#define	  Kdb       Lastbef+19	/* gets remapped to Kbyt */
#define	  Kdw       Lastbef+20	/* gets remapped to Kword */
#define	  Kalign    Lastbef+21
#define	  Kblock    Lastbef+22	/* gets remapped to .( */
#define	  Kbend	    Lastbef+23	/* gets remapped to .) */

#define   Kalong    Lastbef+24
#define   Kashort   Lastbef+25
#define   Kxlong    Lastbef+26
#define   Kxshort   Lastbef+27

#define   Kbin      Lastbef+28
#define   Kaasc     Lastbef+29

#define	  Kcode	    Lastbef+30	/* gets remapped to Ktext */

/* 93 + 30 -> 123 */

#define	  Kinclude  Lastbef+31
#define   Kimport   Lastbef+32
#define   Kimportzp Lastbef+33
#define	  Kproc     Lastbef+34	/* mapped to Kopen */
/* 93 + 35 -> 128 */
#define	  Kendproc  Lastbef+35	/* mapped to Kclose */
#define	  Kzeropage Lastbef+36	/* mapped to Kzero */
#define	  Korg      Lastbef+37	/* mapped to Kpcdef - with parameter equivalent to "*=$abcd" */
#define	  Krelocx   Lastbef+38	/* mapped to Kpcdef - without parameter equivalent to "*=" */
#define	  Klistbytes (Lastbef+39-256)
#define	  Kscope    (Lastbef+40) /* mapped to Kopen */
#define	  Kendscope (Lastbef+41) /* mapped to Kclose */

/* last valid token+1 */
#define	  Anzkey    Lastbef+42	/* define last valid token number; last define above plus one */


#define   Kreloc    (Anzkey-256)   	/* *= (relocation mode) */
#define   Ksegment  (Anzkey+1-256)	/* this actually now is above 127, which might be a problem as char is signed ... */

int number_of_valid_tokens = Anzkey;

/* array used for hashing tokens (26 entries, a-z) */

static int ktp[]={ 0,3,17,25,28,29,29,29,29,32,34,34,38,40,41,42,58,
               58,65,76,90,90,90,92,94,94,94,Anzkey };

#define   Admodes   24

/* 
 * opcodes for each addressing mode
 * high byte: supported architecture (no bits = original NMOS 6502)
 *  bit 1: R65C02
 *  bit 2: 65816
 *  bit 3: 65816 and allows 16-bit quantity (immediate only)
 * low byte: opcode itself
 *
 * each opcode is indexed in this order: *=65816, ^=R65C02
 * 00 = implied
 * 01 = zero page
 * 02 = zero page,x
 * 03 = direct page,y*
 * 04 = direct page (indirect)*
 * 05 = (indirect,x)
 * 06 = (indirect),y
 * 07 = immediate (8-bit)
 * 08 = absolute
 * 09 = absolute,x
 * 10 = absolute,y
 * 11 = relative
 * 12 = (indirect-16) i.e., jmp (some_vector)
 * 13 = (absolute,x)*
 * 14 = zero page+relative test'n'branch ^
 * 15 = zero page clear'n'set'bit ^
 * 16 = relative long*
 * 17 = absolute long*
 * 18 = absolute long,x*
 * 19 = stack relative*
 * 20 = stack relative (indirect),y*
 * 21 = direct page (indirect long)*
 * 22 = direct page (indirect long),y*
 * 23 = (indirect long)
 */

static int ct[Lastbef+1][Admodes] ={
/*     0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   imm */
{     -1,  0x65,0x75,-1,0x172,0x61,0x71,0x469,0x6d,0x7d,0x79,-1,  -1,  -1,  -1,  -1,  -1,0x26f,0x27f,0x263,0x273,0x267,0x277,-1 },  /*adc*/
{     -1,  0x25,0x35,-1,0x132,0x21,0x31,0x429,0x2d,0x3d,0x39,-1,  -1,  -1,  -1,  -1,  -1,0x22f,0x23f,0x223,0x233,0x227,0x237,-1 },  /*and*/
{     0x0a,0x06,0x16,-1,  -1,  -1,  -1,  -1,  0x0e,0x1e,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*asl*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 0x10f,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bbr*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 0x18f,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bbs*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x90,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bcc*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0xb0,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bcs*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0xf0,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*beq*/
{     -1,  0x24,0x134,-1, -1,  -1,  -1, 0x589,0x2c,0x13c,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bit*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x30,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bmi*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0xd0,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bne*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x10,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bpl*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x180,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bra*/
{     0x00,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*brk*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x50,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bvc*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x70,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*bvs*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x282, -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*brl*/
{     0x18,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*clc*/
{     0xd8,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*cld*/
{     0x58,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*cli*/
{     0xb8,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*clv*/
{     -1,  0xc5,0xd5,-1, 0x1d2,0xc1,0xd1,0x4c9,0xcd,0xdd,0xd9,-1, -1,  -1,  -1,  -1,-1,0x2cf,0x2df,0x2c3,0x2d3,0x2c7,0x2d7,-1 },  /*cmp*/
{     -1,  0xe4,-1,  -1,  -1,  -1,  -1, 0x8e0,0xec,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*cpx*/
{     -1,  0xc4,-1,  -1,  -1,  -1,  -1, 0x8c0,0xcc,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*cpy*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x202,-1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*cop*/
/*
{     0x13a,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },*/ /*dea*/
{     0x13a,0xc6,0xd6,-1,  -1,  -1,  -1,  -1,  0xce,0xde,-1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*dec*/
{     0xca,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*dex*/
{     0x88,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*dey*/
{     -1,  0x45,0x55,-1, 0x152,0x41,0x51,0x449,0x4d,0x5d,0x59,-1, -1,  -1,  -1,  -1,-1,0x24f,0x25f,0x243,0x253,0x247,0x257,-1 },  /*eor*/
/*
{     0x11a,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },*/ /*ina*/
{     0x11a,0xe6,0xf6,-1,  -1,  -1,  -1,  -1,  0xee,0xfe,-1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*inc*/
{     0xe8,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*inx*/
{     0xc8,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*iny*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x4c,-1,  -1,  -1, 0x6c,0x17c,-1,  -1,  -1,0x25c, -1,  -1,  -1,  -1,  -1,0x2dc},  /*jmp*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x20,-1,  -1,  -1,  -1, 0x2fc,-1,  -1,  -1,0x222, -1,  -1,  -1,  -1,  -1,  -1 },  /*jsr*/
{     -1,  0xa5,0xb5,-1, 0x1b2,0xa1,0xb1,0x4a9,0xad,0xbd,0xb9,-1, -1,  -1,  -1,  -1,-1,0x2af,0x2bf,0x2a3,0x2b3,0x2a7,0x2b7,-1 },  /*lda*/
{     -1,  0xa6,-1,  0xb6,-1,  -1,  -1, 0x8a2,0xae,-1,  0xbe,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*ldx*/
{     -1,  0xa4,0xb4,-1,  -1,  -1,  -1, 0x8a0,0xac,0xbc,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*ldy*/
{     0x4a,0x46,0x56,-1,  -1,  -1,  -1,  -1,  0x4e,0x5e,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*lsr*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x244,-1,  -1,  -1 ,-1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*mvp*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x254,-1,  -1,  -1 ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*mvn*/
{     0xea,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*nop*/
{     -1,  0x05,0x15,-1, 0x112,0x01,0x11,0x409,0x0d,0x1d,0x19,-1, -1,  -1,  -1,  -1,-1,0x20f,0x21f,0x203,0x213,0x207,0x217,-1 },  /*ora*/
{     0x48,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*pha*/
{     0x08,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*php*/
{     0x1da,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*phx*/
{     0x15a,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*phy*/
{     0x68,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*pla*/
{     0x28,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*plp*/
{     0x1fa,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*plx*/
{     0x17a,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*ply*/
{     0x28b,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*phb*/
{     0x20b,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*phd*/
{     0x24b,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*phk*/
{     0x2ab,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*plb*/
{     0x22b,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*pld*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  0x2f4,-1,  -1, -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*pea*/
{     -1,  -1,  -1,  -1,0x2d4, -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*pei*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1, 0x262,-1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*per*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x107, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*rmb*/
{     0x2a,0x26,0x36,-1,  -1,  -1,  -1,  -1,  0x2e,0x3e,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*rol*/
{     0x6a,0x66,0x76,-1,  -1,  -1,  -1,  -1,  0x6e,0x7e,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*ror*/
{     0x40,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*rti*/
{     0x60,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*rts*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1, 0x2c2,-1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*rep*/
{     0x26b,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*rtl*/
{     -1,  0xe5,0xf5,-1, 0x1f2,0xe1,0xf1,0x4e9,0xed,0xfd,0xf9,-1, -1,  -1,  -1,  -1,-1,0x2ef,0x2ff,0x2e3,0x2f3,0x2e7,0x2f7,-1 },  /*sbc*/
{     0x38,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*sec*/
{     0xf8,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*sed*/
{     0x78,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*sei*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x187, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*smb*/
{     -1,  0x85,0x95,-1,  0x192,0x81,0x91,-1,  0x8d,0x9d,0x99,-1, -1,  -1,  -1,  -1,-1,0x28f,0x29f,0x283,0x293,0x287,0x297,-1 },  /*sta*/
{     -1,  0x86,-1,  0x96,-1,  -1,  -1,  -1,  0x8e,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*stx*/
{     -1,  0x84,0x94,-1,  -1,  -1,  -1,  -1,  0x8c,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*sty*/
{     -1,  0x164,0x174,-1, -1,  -1,  -1,  -1, 0x19c,0x19e,-1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*stz*/
{     -1,  -1,  -1,  -1,  -1,  -1,  -1, 0x2e2,-1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*sep*/
{     0x2db,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*stp*/
{     0xaa,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tax*/
{     0xa8,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tay*/
{     -1,  0x114,-1,  -1,  -1,  -1,  -1, -1, 0x11c,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*trb*/
{     -1,  0x104,-1,  -1,  -1,  -1,  -1, -1, 0x10c,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tsb*/
{     0xba,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tsx*/
{     0x8a,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*txa*/
{     0x9a,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*txs*/
{     0x98,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tya*/
{     0x29b,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*txy*/
{     0x2bb,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tyx*/
{     0x25b,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tcd*/
{     0x27b,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tdc*/
{     0x21b,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tcs*/
{     0x23b,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*tsc*/
{     0x2cb,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*wai*/
{     0x242,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*wdb*/
{     0x2eb,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 },  /*xba*/
{     0x2fb,-1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1  ,-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1 }   /*xce*/


} ;

#define   Syntax    14
#define   Maxbyt    4

/* grouped syntaxes. each row = operand type, column = bytes allowed */
static int at[Syntax][Maxbyt] ={
{     0,   -1,  -1  ,-1 },	/* implied: no operand */
{     -1,  7,   -1  ,-1 },	/* immediate: single byte operand only */
{     -1,  15,  -1  ,-1 },	/* relative: single byte operand only */
{     -1,  -1,  14  ,-1 },	/* test'n'branch: two bytes only */
{     -1,  1,    8  ,17 },	/* addressing: 1 byte for zp,
					2 for absolute,
					3 for absolute long */
{     -1,  2,    9  ,18 },	/* ,x: same */
{     -1,  3,   10  ,-1 },	/* ,y: 1 byte for dp,y,
					2 for absolute,y */
{     -1,  4,   12  ,-1 },	/* (indirect): 1 byte for (dp),
					2 for (absolute) */
{     -1,  5,   13  ,-1 },	/* (i,x): 1 byte for (zp,x),
					2 for (a,x) */
{     -1,  6,   -1  ,-1 },	/* (i),y: 1 byte only */
{     -1,  21,  23  ,-1 },	/* (indirect long): 1 byte for (dp),
					2 for (a) */
{     -1,  22,  -1  ,-1 },	/* (indirect long),y: 1 byte only */
{     -1,  19,  -1  ,-1 },	/* stack relative: 1 byte only */
{     -1,  20,  -1  ,-1 }	/* SR (in),y: 1 byte only */
};

#define   AnzAlt    5

/* disambiguation table. for example, arbitrary instruction xxx $0000 could
	be either interpreted as an absolute operand, or possibly relative.
	note: does not look at comma or after, if present. */
static int xt[AnzAlt][2] ={ /* Alternativ Adr-Modes  */
{     8,   11 },      /* abs -> rel  */
{     2,   3  },      /* z,x -> z,y  */
{     5,   6  },      /* ,x) -> ),y  */
{     9,   10 },      /* a,x -> a,y  */

{     8,   16 }       /* abs -> relong */
};

/* cross check: instruction should be this many bytes long in total */
/* indexed by addressing mode */
static int le[] ={ 1,2,2,2,2,2,2,2,3,3,3,2,3,3,3,2,
                /* new modes */ 3,4,4,2,2,2,2,3 };

/* indicates absolute->zp optimizable addressing modes (abs->zp) */
/* indexed by addressing mode */
static int opt[] ={ -1,-1,-1,-1,-1,-1,-1,-1,1,2,3,-1,4,5,-1,-1,
           /*new*/ -1,8,9,-1,-1,-1,-1,-1 }; /* abs -> zp */

/*********************************************************************************************/
/* pass 1 */
int t_p1(signed char *s, signed char *t, int *ll, int *al)
{
     static int er,l,n,v,nk,na1,na2,bl,am,sy,i,label,byte; /*,j,v2 ;*/
     int afl = 0;
     int tlen;	/* token listing length, to adjust length that is returned */
     int inp;	/* input pointer in t[] */
     unsigned char cast;

/* notes and typical conventions ... er = error code
	am = addressing mode in use
*/

     cast='\0';
     bl=0;
     *al = 0;

/*     printf("\n"); */

     /* convert the next token from string s */
#ifdef DEBUG_AM
fprintf(stderr, "- p1 %d starting -\n", pc[segment]);
#endif
    
     /* As the t_p1 code below always works through the tokens
      * from t_conv in such a way that it always produces a shorter
      * result, the conversion below takes place "in place".
      * This, however, means that the original token sequence, which
      * would be useful for some assembler listing, is overwritten.
      * While the original assumption was ok for a constrained 
      * environment like the Atari ST, this is no longer true.
      * Converting the code below to have separate input and output
      * areas would be error-prone, so we do some copy-magic here
      * instead...*/
     /* we keep three bytes buffer for "T_LISTING" and the length of the
      * token list
      */
     t[0]=T_LISTING;
     er=t_conv(s,t+6,&l,pc[segment],&nk,&na1,&na2,0,&byte);
     tlen = l+6;
     t[1]=tlen&255;
     t[2]=(tlen>>8)&255;
     t[3]=segment;
     t[4]=pc[segment]&255;
     t[5]=(pc[segment]>>8)&255;
     /* now we have to duplicate the token sequence from the T_LISTING buffer
      * to the end of "t", so we can then in-place convert it
      * below. Non-overlapping, size is known in advance, so 
      * using memcpy is fine here
      */
     inp = 0;
     /* discard label definitions before copying the buffer */
     while (inp<l && t[6+inp]==T_DEFINE) {
	inp+=3;
     }
     /* copy the buffer */
     memcpy(t+tlen, t+6+inp, l-inp);
     t=t+tlen;
     l-=inp;
     /* the result of this is that we always have a Klisting entry in the buffer
      * for each tokenization call */
     /* here continue as before, except for adjusting the returne *ll length 
      * in the end, just before return */

     /* return length default is input length */
     *ll=l;

#if 0
     printf("t_conv (er=%d):",er);
     for(i=0;i<l;i++)
          printf("%02x,",t[i] & 0xff);
     printf("\n");
#endif

     /* if text/data produced, then no more fopt allowed in romable mode */
     /* TODO: need to check, Kbyte is being remapped to Kbyt. What is the effect here? */
     if((romable>1) && (t[inp]<Kopen || t[inp]==Kbyte || t[inp]==Kpcdef)) {
       afile->base[SEG_TEXT] = pc[SEG_TEXT] = romaddr + h_length();
       romable=1;
     }
     if(!er)
     {

/*
 *
 * pseudo-op dispatch (except .byt, .asc)
 *
 */
	  // fix sign
          n=t[0]; // & 0xff;

	  /* TODO: make that a big switch statement... */
	  /* maybe later. Cameron */

	  if(n==Kend || n==Klist || n==Kxlist) {
	    *ll = 0;		/* ignore */
	  } else
	  if(n==Kinclude) {
	    *ll = 0;		/* no output length */
	    i=1;
            if(t[i]=='\"') {
	       int k,j=0;
	       char binfname[255];
               i++;
               k=t[i]+i+1;
               i++;
               while(i<k && !er) {
                 binfname[j++] = t[i++];
                 if (j > 255)
                    er = E_NOMEM; /* buffer overflow */
               }
               binfname[j] = '\0';
	       er=icl_open(binfname);
	    } else {
	       er=E_SYNTAX;
	    }
	  } else
	  if(n==Kfopt) {
	    if(romable==1) er=E_ROMOPT;
	    t[0] = Kbyt;
	    set_fopt(l,t,nk+1-na1+na2);
	    *ll = 0;
	  } else
          if(n==Klistbytes) {
	    int p = 0;
            if(!(er=a_term(t+1,&p,&l,pc[segment],&afl,&label,0))) {
                er=E_OKDEF;
	    }
	    *ll = 3;
	    t[0] = Klistbytes;
	    t[1] = p & 0xff;
	    t[2] = (p >> 8) & 0xff;
	    //printf("Klistbytes p1: er=%d, l=%d\n", er, l);
          } else
          if(n==Kpcdef)
          {
	       int tmp;
               if(!(er=a_term(t+1,&tmp /*&pc[SEG_ABS]*/,&l,pc[segment],&afl,&label,0)))
               {
                    i=1;
                    wval(i,tmp /*pc[SEG_ABS]*/, 0);	/* writes T_VALUE, 3 bytes value, plus one byte */
                    t[i++]=T_END;
                    *ll=7;
                    er=E_OKDEF;
/*printf("set pc=%04x, oldsegment=%d, pc[segm]=%04x, ", 
				pc[SEG_ABS], segment, pc[segment]);
printf(" wrote %02x %02x %02x %02x %02x %02x, %02x, %02x\n",
				t[0],t[1],t[2],t[3],t[4],t[5],t[6],t[7]);*/
		    if(segment==SEG_TEXT) {
		      pc[SEG_ABS] = tmp;
		      r_mode(RMODE_ABS);
		    } else {
		      if(!relmode) {
		        pc[segment] = tmp;
		      } else {
			er = E_ILLSEGMENT;
		      }
		    }
/*printf("newsegment=%d, pc[ABS]=%04x\n", segment, pc[SEG_ABS]);*/
               } else {			/* TODO: different error code */
	         if((segment==SEG_ABS) && (er==E_SYNTAX && l==0)) {
/*printf("reloc: oldseg=%d, pc[oldseg]=%04x, pc[abs]=%04x, pc[text]=%04x\n",
			segment, pc[segment], pc[SEG_ABS], pc[SEG_TEXT]);*/
		   t[0]=Kreloc;
		   i=1;
		   wval(i,pc[SEG_TEXT], 0);
		   t[i++]=T_END;
		   *ll=6;
	  	   er=E_OKDEF;
		   r_mode(RMODE_RELOC);
/*printf("     : newseg=%d, pc[newseg]=%04x, pc[abs]=%04x, pc[text]=%04x\n",
			segment, pc[segment], pc[SEG_ABS], pc[SEG_TEXT]);*/
	         }
	       }
          } else
          if(n==Kopen)
          {
	       if(showblk) fprintf(stderr, "%s line %d: .(\n", pp_getidat()->fname, pp_getidat()->fline);
               b_open();
               er=E_NOLINE;
          } else
          if(n==Kclose)
          {
	       if(showblk) fprintf(stderr, "%s line %d: .)\n", pp_getidat()->fname, pp_getidat()->fline);
               er=b_close();
               if(!er) er=E_NOLINE;
          } else
          if(n==Kalong)
          {
		if (!w65816) {
			er=E_65816;
		} else {
               memode=1;
               t[0]=Kalong;
               *ll=1;
               er=E_OKDEF;
		}
          } else
          if(n==Kashort)
          {
               memode=0;
               t[0]=Kashort;
               *ll=1;
               er=E_OKDEF;
          } else
          if(n==Kxlong)
          {
		if (!w65816) {
			er=E_65816;
		} else {
               xmode=1;
               t[0]=Kxlong;
               *ll=1;
               er=E_OKDEF;
		}
          } else
          if(n==Kxshort)
          {
               xmode=0;
               t[0]=Kxshort;
               *ll=1;
               er=E_OKDEF;
          } else
          if(n==Kdsb)
          {
	       dsb_len = 1;
               if(!(er=a_term(t+1,&bl,&l,pc[segment],&afl,&label,0))) {
                    er=E_OKDEF;
	       }
	       dsb_len = 0;
          } else
	  if(n==Ktext) {
/*	    if(segment!=SEG_ABS) {    */
	      segment = relmode ? SEG_TEXT : SEG_ABS;
	      t[0]=Ksegment;
	      t[1]=segment;
	      *ll=2;
              er=E_OKDEF;
/*	    } else {
	      er=E_ILLSEGMENT;
	    }                        */
	  } else
	  if(n==Kdata) {
/*  	    if(segment!=SEG_ABS) {   */
	      segment = SEG_DATA;
	      t[0]=Ksegment;
	      t[1]=SEG_DATA;
	      *ll=2;
              er=E_OKDEF;
/*	    } else {
	      er=E_ILLSEGMENT;
	    }                        */
	  } else
	  if(n==Kbss) {
/*  	    if(segment!=SEG_ABS) {   */
	      segment = SEG_BSS;
	      t[0]=Ksegment;
	      t[1]=SEG_BSS;
	      *ll=2;
              er=E_OKDEF;
/*	    } else {
	      er=E_ILLSEGMENT;
	    }                        */
	  } else
	  if(n==Kzero) {
/*  	    if(segment!=SEG_ABS) {   */
	      segment = SEG_ZERO;
	      t[0]=Ksegment;
	      t[1]=SEG_ZERO;
	      *ll=2;
              er=E_OKDEF;
/*	    } else {
	      er=E_ILLSEGMENT;
	    }                        */
	  } else
	if (n==Kbin) {
		int j;
		int l;

		/* this first pass just calculates a prospective length
			for pass 2. */
		char binfnam[255];
		int offset;
		int length;

		i = 1;
		j = 0;

		/* get offset */
		if(!(er=a_term(t+i,&offset,&l,pc[segment],&afl,&label,1))) {
			i += l;
		}
		if (offset < 0)
			er = E_ILLQUANT;
		if(t[i] == ',') { /* skip comma */
			i++;
		} else {
			er = E_SYNTAX;
		}

		/* get length */
		if (!er &&
			!(er=a_term(t+i,&length,&l,pc[segment],&afl,&label,1)))
		{
			i += l;
		}
		if (length < 0)
			er = E_ILLQUANT;
		if(t[i] == ',') { /* skip comma */
			i++;
		} else {
			er = E_SYNTAX;
		}

		/* get filename.
		   the tokenizer can either see it as a multichar string ... */
		if (!er) {
		   int k;
		
		   //fstart = i;
		   if(t[i]=='\"') {
			i++;
			k=t[i]+i+1;
			i++;
			while(i<k && !er) {
				binfnam[j++] = t[i++];
				if (j > 255)
					er = E_NOMEM; /* buffer overflow */
			}
			binfnam[j] = '\0';
		/* or as a 'char' if it's a single character ("word" would
			have been caught by the above) */
		    } else
			if(!(er=a_term(t+i,&v,&l,pc[segment],&afl,&label,1))) {
			binfnam[0] = v;
			binfnam[1] = '\0';
			i += l;
		    }
		}

		/* three arguments only please */
		if (!er && t[i] != T_END && t[i] != T_COMMENT) {
			er = E_SYNTAX;
		}

		if (!er) {
			FILE *foo;

#ifdef DEBUG_AM
			fprintf(stderr,
"binclude1 offset = %i len = %i filename = %s endchar = %i\n",
		offset, length, binfnam, i);
#endif
			if (!(foo = fopen(binfnam, "r"))) {
				er = E_FNF;
			} else {
				fseek(foo, 0, SEEK_END);
				if ((length+offset) > ftell(foo)) {
					er = E_OUTOFDATA;
				} else {
					length = (length) ? length :
						(ftell(foo)-offset);
				}
				fclose(foo);
			}
			if (!er) {
				if (length > 65535 && !w65816) {
					errout(W_OVER64K);
				} else if (length > 16777215) {
					errout(W_OVER16M);
				}
				/* pass parameters back to xa.c */
				*ll=i+1;
/*
				bl=length+2;
*/
				bl=length;
				er = E_OKDEF; /* defer to pass 2 */
			}
		}
	} else
	  if(n==Kalign) {
	    int tmp;
	    if(segment!=SEG_ABS) {
              if(!(er=a_term(t+1,&tmp,&l,pc[segment],&afl,&label,0))) {
		if(tmp == 1 || tmp == 2 || tmp == 4 || tmp == 256) {
		  set_align(tmp);
		  if(pc[segment] & (tmp-1)) { /* not aligned */
		    int tmp2;
		    t[0]=Kdsb;
		    i=1;
		    bl=tmp=(tmp - (pc[segment] & (tmp-1))) & (tmp-1);
		    wval(i,tmp, 0);
                    t[i++]=',';
		    tmp2= 0xea;
		    wval(i,tmp2, 0);	/* nop opcode */
                    t[i++]=T_END;
		    *ll=9;
		    er=E_OKDEF;
		  } else {
		    *ll=0;	/* ignore if aligned right */
		  }
		} else {
		  er=E_ILLALIGN;
		}
	      }
	    } else {
	      er=E_ILLSEGMENT;
	    }
	  } else
            /* optimization okay on pass 1: use 0 for fl */
		{
#ifdef DEBUG_AM
fprintf(stderr, "E_OK ... t_p2 xat.c\n");
#endif
	       /* this actually calls pass2 on the current tokenization stream,
 		* but without including the Klisting token listing */
               er=t_p2(t,ll,(0 | byte), al);
	}
          
     } else
     if(er==E_NODEF)
     {

     /*
      * no label was found from t_conv!
      * try to figure out most likely length
      *
      */

#ifdef DEBUG_AM
fprintf(stderr, "E_NODEF pass1 xat.c\n");
#endif
	er = E_OK; /* stuff error */
          n=t[0];	/* look at first token */

	/* mnemonic dispatch -- abbreviated form in t_p2, but changed here
		to not do anything other than 24-bit optimization since we
		don't know the value of the label */

		/* choose addressing mode; add commas found */

          if(n>=0 && n<=Lastbef)
          {
	       int inp = 1;	/* input pointer */

               if(t[inp]==T_END || t[inp]==T_COMMENT)
               {
                    sy=0;	/* implied */
		    inp++;
               } else
               if(t[inp]=='#')
               {
                    sy=1+nk;	/* immediate */
		    inp++;
               } else
               if(t[inp]=='(')
               {
                    sy=7+nk;	/* computed */
		    inp++;
               } else {
                    sy=4+nk;	/* absolute or zero page */
	       }

	       /* this actually finds the cast for all addressing modes,
 		  but t_conv() only puts it there for immediate (#) or absolute/
		  absolute indexed addressing modes */
	       if (t[inp] == T_CAST) {
		    inp++;
		    cast = t[inp];
		    inp++;
	       }

	       
		/* length counter set to maximum length + 1 */
               bl=Maxbyt+1;
               
		/* find best fit for length of this operand */
               while(--bl)
               {

		/* look at syntax table (at) using syntax (sy) as index.
			is there an addressing mode for an operand
			of this length? am = addressing mode */

                    if((am=at[sy][bl-1])>=0)
                    {
                         if(am>Admodes-1) /* no, it's -1, syntax error */
                         {
                              er=E_SYNTAX;
                              break;
                         }
                         if(ct[n][am]>=0) /* yes, valid token *and* mode,
						so we're done */
                              break;

			/* no valid mode for this token, see if it's something
				ambiguous; if so, try to interpret in that
				context. */
                         for(v=0;v<AnzAlt;v++)
                              if(xt[v][0]==am && ct[n][xt[v][1]]>=0)
                                   break;
                         if(v<AnzAlt) /* got a match for another context */
                         {
                              am=xt[v][1];
                              break;
                         }
                    }
               }

		/* optimize operand length for 24-bit quantities */
		/* look at cast byte from t_conv */
               if (cast!='@' && cast!= '!')
               {
                     if(bl && !er && opt[am]>=0 && am>16) /* <<< NOTE! */
                          if(ct[n][opt[am]]>=0)
                               am=opt[am];
               }
		/* if ` is declared, force further optimization */
		if (cast=='`') {
			if (opt[am]<0 || ct[n][opt[am]]<0)
				errout(E_ADRESS);
			am=opt[am];
		}
		/* if ! is declared, force to 16-bit quantity */
		if (cast=='!' && am>16 && opt[am]>=0 && bl) {
			am=opt[am];
		}

		/* couldn't match anything for this opcode */
               if(!bl)
                    er=E_SYNTAX;
               else {
		/* ok, get length of instruction */
                    bl=le[am];
		/* and add one for 65816 special instruction modes */
                    if( ((ct[n][am]&0x400) && memode) ||
			((ct[n][am]&0x800) && xmode)) {
                          bl++;
			}
                }


		if (er == E_NODEF)
			er = E_OK;

		/* .byt, .asc, .word, .dsb, .fopt pseudo-op dispatch */

          } else
	  if(n==Kimportzp) {
	    int i;
	    *ll=0;		/* no output */
	    bl = 0;		/* no output length */
	    /* import labels; next follow a comma-separated list of labels that are
 	       imported. Tokenizer has already created label entries, we only need to
	       set the flags appropriately */
	    i=1;
/*printf("Kimport: t[i]=%d\n",t[i]);*/
	    while(t[i]==T_LABEL) {
		int n = (t[i+1] & 255) | (t[i+2] << 8); 	/* label number */
/*printf("lg_import: %d\n",n);*/
		lg_importzp(n);
		i+=3;
		while (t[i]==' ') i++;
		if (t[i]!=',') break;
		i++;
		while (t[i]==' ') i++;
	    }
	    er=E_NOLINE;
	  } else
	  if(n==Kimport) {
	    int i;
	    *ll=0;		/* no output */
	    bl = 0;		/* no output length */
	    /* import labels; next follow a comma-separated list of labels that are
 	       imported. Tokenizer has already created label entries, we only need to
	       set the flags appropriately */
	    i=1;
/*printf("Kimport: t[i]=%d\n",t[i]);*/
	    while(t[i]==T_LABEL) {
		int n = (t[i+1] & 255) | (t[i+2] << 8); 	/* label number */
/*printf("lg_import: %d\n",n);*/
		lg_import(n);
		i+=3;
		while (t[i]==' ') i++;
		if (t[i]!=',') break;
		i++;
		while (t[i]==' ') i++;
	    }
	    er=E_NOLINE;
	  } else
          if(n==Kbyt || n==Kasc || n==Kaasc)
          {
#ifdef DEBUG_AM
fprintf(stderr, "byt pass 1 %i\n", nk+1-na1+na2);
#endif
               bl=nk+1-na1+na2;
          } else
          if(n==Kword)
          {
               bl=2*nk+2;
          } else
          if(n==Kdsb)
          {
               er=a_term(t+1,&bl,&l,pc[segment],&afl,&label,0);
          } else
	  if(n==Kfopt) 
	  {
	    set_fopt(l-1,t+1, nk+1-na1+na2);
	    *ll = 0;
	  } else
          if(n==T_OP)
          {
               er=E_OKDEF;
          } else
               er=E_NODEF;
          
          if(!er)
               er=E_OKDEF;
#ifdef DEBUG_AM
fprintf(stderr, "guessing instruction length is %d\n", bl);
#endif
     }
     if(er==E_NOLINE)
     {
          er=E_OK;
          *ll=0;
     }

     *al += bl;
     pc[segment]+=bl;
     if(segment==SEG_TEXT) pc[SEG_ABS]+=bl;
     if(segment==SEG_ABS) pc[SEG_TEXT]+=bl;

     /* adjust length by token listing buffer length */
     *ll = *ll + tlen;
     return(er);
}

/*********************************************************************************************/
/* t_pass 2
 *
 * *t is the token list as given from pass1
 * *ll is the returned length of bytes (doubles as 
 *     input for whether OK or OKDEF status from pass1)
 * fl defines if we allow zeropage optimization
 *
 * Conversion takes place "in place" in the *t array.
 */

/**
 * function called from the main loop, where "only" the
 * undefined labels have to be resolved and the affected 
 * opcodes are assembled, the rest is passed through from
 * pass1 (pass-through is done in t_p2, when *ll<0)
 * As this is not called from p1, assume that we do not
 * do length optimization
 *
 * *t	is the input token list
 * *ll	is the input length of the token list,
 * 	and the output of how many bytes of the buffer are to be taken
 * 	into the file
 */
int t_p2_l(signed char *t, int *ll, int *al)
{
	int er = E_OK;

	if (t[0] == T_LISTING) {
	  int tlen;
	  tlen=((t[2]&255)<<8) | (t[1]&255);
	  if (*ll<0) {
	    *ll=(*ll) + tlen;
	  } else {
	    *ll=(*ll) - tlen;
	  }

	  if (*ll != 0) {
  	    er = t_p2(t+tlen, ll, 1, al);
	  }

	  /* do the actual listing (*ll-2 as we need to substract the place for the tlen value) */
	  do_listing(t+3, tlen-3, t+tlen, *ll);

	  /* adapt back, i.e. remove token listing */
	  if (*ll != 0) {
	  	memmove(t, t+tlen, abs(*ll));
	  }
	} else {
	  er = t_p2(t, ll, 1, al);
	}
	return er;
}

/**
 * This method does not handle a token list. Thus it
 * is called internally from pass1 without the token listing, and 
 * from the t_p2_l() method that strips the token listing 
 * as well
 *
 * *t	is the input token list
 * *ll	is the input length of the token list,
 * 	and the output of how many bytes of the buffer are to be taken
 * 	into the file
 * fl	when set, do not do length optimization, when not set,
 * 	allow length optimization (when called from p1)
 */
int t_p2(signed char *t, int *ll, int fl, int *al)
{
     static int afl,nafl, i,j,k,er,v,n,l,bl,sy,am,c,vv[3],v2,label;
     static int rlt[3];	/* relocation table */
     static int lab[3];	/* undef. label table */


     er=E_OK;
     bl=0;
     if(*ll<0) /* <0 when E_OK, >0 when E_OKDEF     */
     {
          *ll=-*ll;
          bl=*ll;
          er=E_OK;
	
     } else
     {
          n=t[0];
          if(n==T_OP)
          {
               n=cval(t+1);
               er=a_term(t+4,&v,&l,pc[segment],&nafl,&label,0);

               if(!er)
               {
                    if(t[3]=='=')
                    {
                      v2=v;
                    } else {
                      if( (!(er=l_get(n,&v2, &afl))) 
				&& ((afl & A_FMASK)!=(SEG_UNDEF<<8)) 
				&& ((afl & A_FMASK)!=(SEG_UNDEFZP<<8)) )
                      {
                         if(t[3]=='+')
                         {
			      if(afl && nafl) { errout(E_WPOINTER); nafl=0; }
			      nafl = afl;
                              v2+=v;
                         } else
                         if(t[3]=='-')
                         {	
			      if( (((nafl & A_FMASK)>>8) != afl) 
					|| ((nafl & A_MASK)==A_HIGH) ) {
			        errout(E_WPOINTER); 
				nafl=0; 
			      } else {
				nafl = afl;
			      }
                              v2-=v;
                         } else
                         if(t[3]=='*')
                         {
			      if(afl || nafl) { errout(E_WPOINTER); nafl=0; }
                              v2*=v;
                         } else
                         if(t[3]=='/')
                         {
			      if(afl || nafl) { errout(E_WPOINTER); nafl=0; }
                              if(v)
                                   v2/=v;
                              else
                                   er=E_DIV;
                         } else
                         if(t[3]=='|')
                         {
			      if(afl || nafl) { errout(E_WPOINTER); nafl=0; }
                              v2=v|v2;
                         } else
                         if(t[3]=='&')
                         {
			      if(afl || nafl) { errout(E_WPOINTER); nafl=0; }
                              v2=v2&v;
                         }
                      }
		    }
                    l_set(n,v2,nafl>>8);	

                    *ll=0;
                    if(!er)
                         er=E_NOLINE;
               }
          } else
          if(n==Kword)
          {
               i=1;
               j=0;
               while(!er && t[i]!=T_END && t[i] != T_COMMENT)
               {
                    if(!(er=a_term(t+i,&v,&l,pc[segment],&afl,&label,1)))
                    {   
/*if(afl) printf("relocation 1 %04x at pc=$%04x, value now =$%04x\n",
							afl,pc[segment],v); */
			 if(afl) u_set(pc[segment]+j, afl, label, 2);
                         t[j++]=v&255;
                         t[j++]=(v>>8)&255;

                         i+=l;     
                         if(t[i]!=T_END && t[i] != T_COMMENT && t[i]!=',')
                              er=E_SYNTAX;
                         else
                         if(t[i]==',')
                              i++;

                    }
               }
               *ll=j;
               bl=j;
          } else if (n == Kbin) {
		int j;
		int l;

		/* figure out our parameters again. repeat most of
			the error checking since we might not be over
			the total number of bogosities */
		char binfnam[255];
		int offset;
		int length;
		int fstart;
		int flen;

		i = 1;
		j = 0;
		flen = 0;

		/* get offset */
		if(!(er=a_term(t+i,&offset,&l,pc[segment],&afl,&label,1))) {
			i += l;
		}
		if (offset < 0)
			er = E_ILLQUANT;
		if(t[i] == ',') { /* skip comma */
			i++;
		} else {
			er = E_SYNTAX;
		}

		/* get length */
		if (!er &&
			!(er=a_term(t+i,&length,&l,pc[segment],&afl,&label,1)))
		{
			i += l;
		}
		if (length < 0)
			er = E_ILLQUANT;
		if(t[i] == ',') { /* skip comma */
			i++;
		} else {
			er = E_SYNTAX;
		}

		/* get filename.
		   the tokenizer can either see it as a multichar string ... */
		if (!er) {
		   int k;
		
		   fstart = i;
		   if(t[i]=='\"') {
			i++;
			k=t[i]+i+1;
			i++;
			while(i<k && !er) {
				binfnam[j++] = t[i++];
				if (j > 255)
					er = E_NOMEM; /* buffer overflow */
			}
			binfnam[j] = '\0';
			flen = j;
		/* or as a 'char' if it's a single character ("word" would
			have been caught by the above) */
		    } else
			if(!(er=a_term(t+i,&v,&l,pc[segment],&afl,&label,1))) {
			binfnam[0] = v;
			binfnam[1] = '\0';
			i += l;
			flen = 1;
		    }
		}

		/* three arguments only please */
		if (!er && t[i] != T_END && t[i] != T_COMMENT) {
			er = E_SYNTAX;
		}

		if (!er) {
			FILE *foo;

#ifdef DEBUG_AM
			fprintf(stderr,
"binclude2 offset = %i len = %i filename = %s endchar = %i\n",
		offset, length, binfnam, i);
#endif
			if (!(foo = fopen(binfnam, "r"))) {
				er = E_FNF;
			} else {
				fseek(foo, 0, SEEK_END);
				if ((length+offset) > ftell(foo)) {
					er = E_OUTOFDATA;
				} else {
					length = (length) ? length :
						(ftell(foo)-offset);
				}
				fclose(foo);
			}
			if (!er) {
				if (length > 65535 && !w65816) {
					errout(W_OVER64K);
				} else if (length > 16777215) {
					errout(W_OVER16M);
				}
				/* pass parameters back to xa.c */
				*ll=length;
/*
				bl=length+2;
*/
				bl=length;
				t[0] = offset & 255;
				t[1] = (offset >> 8) & 255;
				t[2] = (offset >> 16) & 255;
				/* God help us if the index is > 65535 */
				t[3] = fstart & 255;
				t[4] = (fstart >> 8) & 255;
				t[5] = flen; /* to massage 'char' types */
				er = E_BIN;
			}
		}
	} else if(n==Kasc || n==Kbyt || n==Kaasc) {
               i=1;
               j=0;
               while(!er && t[i]!=T_END && t[i] != T_COMMENT)
               {
                    if(t[i]=='\"')
                    {
                         i++;
                         k=t[i]+i+1;
                         i++;
                         while(i<k)
                              t[j++]=t[i++];
                    } else
                    {
                         if(!(er=a_term(t+i,&v,&l,pc[segment],&afl,&label,1)))
                         {
/*if(afl) printf("relocation 2 %04x at pc=$%04x, value now =$%04x\n",afl,pc[segment]+j,v); */
			      if(afl) u_set(pc[segment]+j, afl, label, 1);
                              if(v&0xff00)
                                   er=E_OVERFLOW;
                              else
                              {
                                   t[j++]=v;                        
                                   i+=l;     
                              }
                         }
                    }
                    if(t[i]!=T_END && t[i] != T_COMMENT && t[i]!=',')
                         er=E_SYNTAX;
                    else
                         if(t[i]==',')
                              i++;
               }
               *ll=j;
               bl=j;
          } else
          if(n==Kalong)
          {
               memode=1;
               *ll=0;
          } else
          if(n==Kashort)
          {
               memode=0;
               *ll=0;
          } else
          if(n==Kxlong)
          {
               xmode=1;
               *ll=0;
          } else
          if(n==Kxshort)
          {
               xmode=0;
               *ll=0;
          } else
          if(n==Kpcdef)
          {
	       int npc;
               er=a_term(t+1,&npc,&l,pc[segment],&afl,&label,0);
               bl=0;     
               *ll=0;
	       if(segment==SEG_TEXT) {
	         r_mode(RMODE_ABS);
	       }
	       pc[segment] = npc;
          } else
          if(n==Kreloc) {
	       int npc;
               er=a_term(t+1,&npc,&l,pc[segment],&afl,&label,0);
/*printf("Kreloc: segment=%d, pc[seg]=%04x\n", segment, pc[segment]);*/
               bl=0;     
               *ll=0;
	       r_mode(RMODE_RELOC);
	       pc[segment] = npc;
/*printf("Kreloc: newsegment=%d, pc[seg]=%04x\n", segment, pc[segment]);*/
	  } else
	  if(n==Klistbytes) {
	       int nbytes = (t[1] & 0xff) + (t[2] << 8);
	       //printf("Klistbytes --> er=%d, nbytes=%d\n", er, nbytes);
	       list_setbytes(nbytes);
	       l = 2;
	       *ll=0;
	       bl =0;
	  } else
	  if(n==Ksegment) {
	       segment = t[1];
	       *ll=0;
	       bl =0;
	  } else
          if(n==Kdsb)
          {
	       dsb_len = 1;
               if(!(er=a_term(t+1,&j,&i,pc[segment],&afl,&label,0)))
               {
/*
                    if(t[i+1]!=',')
                         er=E_SYNTAX;
                    else
*/
/*
		    if((segment!=SEG_ABS) && afl) 
			 er=E_ILLPOINTER;
		    else
*/
                    {
			 dsb_len = 0;

			 if(t[i+1]==',') {
                           er=a_term(t+2+i,&v,&l,pc[segment],&afl,&label,0);
			 } else {
			   v=0;
			 }
                         if(!er && v>>8)
                              er=E_OVERFLOW;
                         t[0]=v&255;
                         if(!er)
                         {
                              *ll=j;
                              bl=j;
#ifdef DEBUG_AM
fprintf(stderr, "Kdsb E_DSB %i\n", j);
#endif
                              er=E_DSB;
                         }
                    }
                    if(!er)
                         bl=j;
               }
	       dsb_len = 0;
          } else
          if(n>=0 && n<=Lastbef)
          {
	       int inp = 1;		/* input pointer */
 	       signed char cast = '\0';	/* cast value */

	       c = t[inp];

               if(c=='#')
               {
                    inp++;
		    if (t[inp] == T_CAST) {
			inp++;
			cast = t[inp];
			inp++;
		    }
                    sy=1;
                    if(!(er=a_term(t+inp,vv,&l,pc[segment],&afl,&label,1)))
                    {
/* if(1) printf("a_term returns afl=%04x\n",afl); */

			 rlt[0] = afl;
			 lab[0] = label;
                         inp+=l;
                         if(t[inp]!=T_END && t[inp] != T_COMMENT)
                         {
                              if(t[inp]!=',')
                                   er=E_SYNTAX;
                              else
                              {
                                   inp++;
                                   sy++;
                                   if(!(er=a_term(t+inp,vv+1,&l,pc[segment],&afl,&label,1)))
                                   {
			                rlt[1] = afl;
					lab[1] = label;
                                        inp+=l;
                                        if(t[inp]!=T_END && t[inp] != T_COMMENT)
                                        {
                                             if(t[inp]!=',')
                                                  er=E_SYNTAX;
                                             else
                                             {
                                                  inp++;
                                                  sy++;
                                                  if(!(er=a_term(t+inp,vv+2,&l,pc[segment],&afl,&label,1)))
                                                  {
			                               rlt[2] = afl;
						       lab[2] = label;
                                                       inp+=l;
                                                       if(t[inp]!=T_END && t[inp]!=T_COMMENT)
                                                            er=E_SYNTAX;
                                                  }
                                             }
                                        }
                                   }
                              }
                         }
                    }
               } else
               if(c==T_END || c==T_COMMENT)
               {
                    sy=0;
               } else
               if(c=='(')
               {
		    inp++;
		    if (t[inp] == T_CAST) {
			inp++;
			cast = t[inp];
			inp++;
		    }
                    sy=7;
                    if(!(er=a_term(t+inp,vv,&l,pc[segment],&afl,&label,1)))
                    {
			 inp += l;
			 rlt[0] = afl;
			 lab[0] = label;

                         if(t[inp]!=T_END && t[inp]!=T_COMMENT)
                         {
                              if(t[inp]==',')
                              {
				   inp++;
                                   if (tolower(t[inp])=='x')
                                        sy=8;
                                   else
                                        sy=13;

                              } else
                              if(t[inp]==')')
                              {
				   inp++;
                                   if(t[inp]==',')
                                   {
					inp++;
                                        if(tolower(t[inp])=='y')
                                             sy=9;
                                        else
                                             er=E_SYNTAX;
                                   } else
                                   if(t[inp]!=T_END && t[inp]!=T_COMMENT)
                                        er=E_SYNTAX;
                              } 
                         } else
                              er=E_SYNTAX;
                    }
               } else
               if(c=='[')
               {
		    inp++;
		    if (t[inp] == T_CAST) {
			inp++;
			cast = t[inp];
			inp++;
		    }
                    sy=10;
                    if(!(er=a_term(t+inp,vv,&l,pc[segment],&afl,&label,1)))
                    {
			 inp += l;
                         rlt[0] = afl;
			 lab[0] = label;

                         if(t[inp]!=T_END && t[inp]!=T_COMMENT)
                         {
                              if(t[inp]==']')
                              {
				   inp++;
                                   if(t[inp]==',')
                                   {
					inp++;
                                        if(tolower(t[inp])=='y')
                                             sy=11;
                                        else
                                             er=E_SYNTAX;
                                   } else
                                   if(t[inp]!=T_END && t[inp]!=T_COMMENT)
                                        er=E_SYNTAX;
                              } 
                         } else
                              er=E_SYNTAX;
                    }
               } else
               {
		    if (t[inp] == T_CAST) {
			inp++;
			cast = t[inp];
			inp++;
		    }
                    sy=4;
                    if(!(er=a_term(t+inp,vv,&l,pc[segment],&afl,&label,1)))
                    {
			 inp += l;
			 rlt[0] = afl;
			 lab[0] = label;
                         if(t[inp]!=T_END && t[inp]!=T_COMMENT)
                         {
                              if(t[inp]==',')
                              {
				   inp++;
                                   if(tolower(t[inp])=='y')
                                        sy=6;
                                   else
                                   if(tolower(t[inp])=='s')
                                        sy=12;
                                   else
                                        sy=5;
                              } else
                                   er=E_SYNTAX;
                         }
                    }
               }
                
               bl=Maxbyt+1;
               
               while(--bl)
               {
                    if((am=at[sy][bl-1])>=0)
                    {
                         if(am>Admodes)
                         {
                              er=E_SYNTAX;
                              break;
                         }
                         if(ct[n][am]>=0)
                              break;

                         for(v=0;v<AnzAlt;v++)
                              if(xt[v][0]==am && ct[n][xt[v][1]]>=0)
                                   break;
                         if(v<AnzAlt) 
                         {
                              am=xt[v][1];
                              break;
                         }
                    }
               }

/* FIXIT1 
		fprintf(stderr, "t has: ");
		for(v=0;v<l;v++) {
			fprintf(stderr, "%02x ", t[v]);
		}
		fprintf(stderr, "\n");
*/

/* only do optimization if we're being called in pass 1 -- never pass 2 */
/* look at cast byte */
               if (cast!='@')
               {
#ifdef DEBUG_AM
fprintf(stderr,
"b4: pc= %d, am = %d and vv[0] = %d, optimize = %d, bitmask = %d\n",
	pc[segment], am, vv[0], fl, (vv[0]&0xffff00));
#endif

/* terrible KLUDGE!!!! OH NOES!!!1!
   due to the way this is constructed, you must absolutely always specify @ to
   get an absolute long or it will absolutely always be optimized down */

                    if(bl && am>16 &&
			!er && !(vv[0]&0xff0000) && opt[am]>=0)
                         if(ct[n][opt[am]]>=0)
                              am=opt[am];
#ifdef DEBUG_AM
fprintf(stderr,
"aftaa1: pc= %d, am = %d and vv[0] = %d, optimize = %d, bitmask = %d\n",
	pc[segment], am, vv[0], fl, (vv[0]&0xffff00));
#endif
                    if(cast!='!') {
                         if(bl && !er && !(vv[0]&0xffff00) && opt[am]>=0) {
                              if(ct[n][opt[am]]>=0) {
				if (!fl || cast=='`') {
                                   	am=opt[am];
				} else {
					errout(W_FORLAB);
				}
			      }
			}
                    }
#ifdef DEBUG_AM
fprintf(stderr,
"aftaa2: pc=%d, am=%d and vv[0]=%d, optimize=%d, bitmask=%d, op=%d\n",
	pc[segment], am, vv[0], fl, (vv[0]&0xffff00), ct[n][opt[am]]);
#endif
               }

               if(!bl)
                    er=E_SYNTAX;
               else
               {
                    bl=le[am];
                    if( ((ct[n][am]&0x400) && memode) || ((ct[n][am]&0x800) && xmode)) {
                         bl++;
		}
                    *ll=bl;

               }

#ifdef DEBUG_AM
fprintf(stderr, "byte length is now %d\n", bl);
#endif

               if(!er)
               {
                    t[0]=ct[n][am]&0x00ff;
                    if(ct[n][am]&0x0300)
                    {
                         if(ct[n][am]&0x100) {
                              ncmos++;
                              if(!cmosfl)
                                  er=E_CMOS;
                         } else {
                              n65816++;
                              if(!w65816)
                                  er=E_65816;
                         }
                    }
                    if(am!=0)
                    {
                         if((am<8 && !( ((ct[n][am]&0x400) && memode) || ((ct[n][am]&0x800) && xmode) )) || (am>=19 && am!=23))
                         {
                              if(vv[0]&0xff00) {
#ifdef DEBUG_AM
fprintf(stderr, "address mode: %i address: %i\n", am, vv[0]);
#endif
                                   er=E_OVERFLOW;
				}
                              else
                                   t[1]=vv[0];
/*if(rlt[0]) printf("relocation 1 byte %04x at pc=$%04x, value now =$%04x\n",rlt[0],pc[segment]+1,*vv); */
			      if(rlt[0]) u_set(pc[segment]+1, rlt[0], lab[0], 1);
                         } else
                         if(((am<14 || am==23) && am!=11) || am==7)
                         {
                              if (vv[0]>0xffff) {
#ifdef DEBUG_AM
fprintf(stderr, "address mode: %i address: %i\n", am, vv[0]);
#endif
                                   er=E_OVERFLOW;
				}
                              else {
                                   t[1]=vv[0]&255;
                                   t[2]=(vv[0]>>8)&255;
/*if(rlt[0]) printf("relocation 2 byte %04x at pc=$%04x, value now =$%04x\n",rlt[0],pc[segment]+1,*vv); */
			           if(rlt[0]) u_set(pc[segment]+1, rlt[0], lab[0], 2);
                              }
                         } else
                         if(am==11 || am==16) {
			   if((segment!=SEG_ABS) && (!rlt[0])) {
			     er=E_ILLPOINTER;
			   } else {
/*printf("am=11, pc=%04x, vv[0]=%04x, segment=%d\n",pc[segment],vv[0], segment);*/
                              v=vv[0]-pc[segment]-le[am];
                              if(((v&0xff80)!=0xff80) && (v&0xff80) && (am==11))
                                   er=E_RANGE;
                              else {
                                   t[1]=v&255;
                                   t[2]=(v>>8)&255;
                              }
			   }
                         } else
                         if(am==14) {
                              if(vv[0]&0xfff8 || vv[1]&0xff00)
                                   er=E_RANGE;
                              else
			      if((segment!=SEG_ABS) && (rlt[0] || !rlt[2])) {
				   er=E_ILLPOINTER;
			      } else {
/*if(rlt[1]) printf("relocation 1 byte %04x at pc=$%04x, value now =$%04x\n",rlt[1],pc[segment]+1,*vv); */
			           if(rlt[1]) u_set(pc[segment]+1, rlt[1], lab[1], 1);
                                   t[0]=t[0]|(vv[0]<<4);
                                   t[1]=vv[1];
                                   v=vv[2]-pc[segment]-3;
                                   if((v&0xff80) && ((v&0xff80)!=0xff80))
                                        er=E_OVERFLOW;
                                   else
                                        t[2]=v;
                              }
                         } else
                         if(am==15)
                         {
/*if(rlt[1]) printf("relocation 1 byte %04x at pc=$%04x, value now =$%04x\n",rlt[1],pc[segment]+1,*vv); */
			      if(rlt[1]) u_set(pc[segment]+1, rlt[1], lab[1], 1);
                              if(vv[0]&0xfff8 || vv[1]&0xff00)
                                   er=E_OVERFLOW;
                              else
                              {
                                   t[0]=t[0]|(vv[0]<<4);
                                   t[1]=vv[1];
                              }
                         } else
                         if(am==17 || am==18)
                         {
                                   t[1]=vv[0]&255;
                                   t[2]=(vv[0]>>8)&255;
                                   t[3]=(vv[0]>>16)&255;
			           if(rlt[0]) {
                                        rlt[0]|=A_LONG;
			                u_set(pc[segment]+1, rlt[0], lab[0], 3);
                                   }

                         } else
                              er=E_SYNTAX;
                    }
               }          
               
          } else
               er=E_SYNTAX;
     }

#ifdef DEBUG_AM
fprintf(stderr, "-- endof P2\n");
#endif
     pc[segment]+=bl;
     if(segment==SEG_TEXT) pc[SEG_ABS]+=bl;
     if(segment==SEG_ABS) pc[SEG_TEXT]+=bl;
     *al = bl;
     return(er);
}

/*********************************************************************************************/
/* helper function for the preprocessor, to compute an arithmetic value
 * (e.g. for #if or #print).
 * First tokenizes it, then calculates the value
 */
int b_term(char *s, int *v, int *l, int pc)
{
     static signed char t[MAXLINE];
     int er,i,afl, label;

     if(!(er=t_conv((signed char*)s,t,l,pc,&i,&i,&i,1,NULL)))
     {
          er=a_term(t,v,&i,pc,&afl,&label,0);
     
     }
     return(er);
}
     
/*********************************************************************************************/
/* translate a string into a first-pass sequence of tokens;
 * Take the text from *s (stopping at \0 or ';'), tokenize it
 * and write the result to *t, returning the length of the
 * token sequence in *l
 *
 * Input params:
 * 	s	source input line
 * 	t	output token sequence buffer
 * 	l	return length of output token sequence here
 * 	pc	the current PC to set address labels to that value
 * 	nk	return number of comma in the parameters
 * 	na1	asc text count returned
 * 	na2	total byte count in asc texts returned
 * 	af	arithmetic flag: 0=do label definitions, parse opcodes and params;
 * 		1=only tokenize parameters, for b_term() call from the preprocessor
 * 		for arithmetic conditions
 * 	bytep	???
 */
static int t_conv(signed char *s, signed char *t, int *l, int pc, int *nk,
             int *na1, int *na2, int af, int *bytep)  
{
     static int v,f;
     static int operand,o;
     int fl,afl;
     int p,q,ll,mk,er;
     int ud;	/* counts undefined labels */
     int n;	/* label number to be passed between l_def (definition) and l_set (set the value) */
     int byte;
     int uz;	/* unused at the moment */
     /*static unsigned char cast;*/

/* ich verstehe deutsch, aber verstehen andere leute nicht; so, werde ich
   diese bemerkungen uebersetzen ... cameron */
/* I understand German, but other folks don't, so I'll translate these
   comments ... Cameron */
/* note that I don't write so good tho' ;) */

     *nk=0;         /* comma count */
     *na1=0;        /* asc text count */
     *na2=0;        /* total bytecount in asc texts */
     ll=0;
     er=E_OK;		/* error state */
     p=0;
     q=0;
     ud = uz = byte =0;
     mk=0;          /* 0 = add'l commas ok */
     fl=0;          /* 1 = pass text thru */
     afl=0;         /* pointer flag for label */

     while(isspace(s[p])) p++;

     n=T_END;
     /*cast='\0';*/

     if(!af)
     {
          while(s[p]!='\0' && s[p]!=';')
          {
		//printf("CONV: %s\n", s);

	       if (s[p] == ':') {
			// this is a ca65 unnamed label
			if ((er = l_def((char*)s+p, &ll, &n, &f)))
				break;
                    	l_set(n,pc,segment);        /* set as address value */
		    	t[q++]=T_DEFINE;
		    	t[q++]=n&255;
		    	t[q++]=(n>>8)&255;
                    	n=0;

			p+=ll;

               		while(isspace(s[p])) p++;

			// end of line
			if (s[p] == 0 || s[p] == ';') {
				break;
			}
	       }
			
		/* is keyword? */
               if(!(er=t_keyword(s+p,&ll,&n)))
                    break;

		/* valid syntax, but just not a real token? */
               if(er && er!=E_NOKEY)
                    break;

		// if so, try to understand as label 
		// it returns the label number in n
               if((er=l_def((char*)s+p,&ll,&n,&f)))
                    break;

               p+=ll;

               while(isspace(s[p])) p++;

               if(s[p]=='=')
               {
		    /*printf("Found = @%s\n",s+p);*/
                    t[q++]=T_OP;
                    t[q++]=n&255;
                    t[q++]=(n>>8)&255;
                    t[q++]='=';
                    p++;
                    ll=n=0;
                    break;
               } else
               if(s[p]==':' && s[p+1]=='=')		/* support := label assignments (ca65 compatibility) */
	       {
		    /*printf("Found := @%s\n", s+p);*/
		    t[q++]=T_OP;
		    t[q++]=n&255;
		    t[q++]=(n>>8)&255;
		    t[q++]='=';
		    p+=2;
		    ll=n=0;
		    break;
               } else
               if(f && s[p]!='\0' && s[p+1]=='=')
               {
                    t[q++]=T_OP;
                    t[q++]=n&255;
                    t[q++]=(n>>8)&255;
                    t[q++]=s[p];
                    p+=2;
                    ll=n=0;
                    break;
               } else
               if(s[p]==':')	/* to support label: ... syntax */
               {
                    p++;
                    while(s[p]==' ') p++;
                    l_set(n,pc,segment);        /* set as address value */
		    t[q++]=T_DEFINE;
		    t[q++]=n&255;
		    t[q++]=(n>>8)&255;
                    n=0;

               } else {		/* label ... syntax */
                    l_set(n,pc,segment);        /* set as address value */
      		    t[q++]=T_DEFINE;
		    t[q++]=n&255;
		    t[q++]=(n>>8)&255;
        	    n=0;
               }

          }

          if(n>=0 && n<=Lastbef)
               mk=1;     /* 1= nur 1 Komma erlaubt *//* = only 1 comma ok */
     }

     if(s[p]=='\0' || s[p]==';')
     {
          er=E_NOLINE;
          ll=0;
     } else
     if(!er)
     {

          p+=ll;
          if(ll) {
             t[q++]= n & 0xff;
/*
             if( (n&0xff) == Kmacro) {
                t[q++]= (n >> 8) & 0xff;
             }
*/
          }

          operand=1;

          while(s[p]==' ') p++;

          if(s[p]=='#')
          {
               mk=0;
               t[q++]=s[p++];
               while(s[p]==' ') p++;
          }

/*
 *
 * operand processing
 * byte = length of operand in bytes to be assembled
 *
 *
 */

          /* this addresses forced high/low/two byte addressing, but only
             for the first operand. Further processing is done in a_term()
          */

/* FIXIT2 */

          while(s[p]!='\0' && s[p]!=';' && !er)
          {
               if(fl)
               {
                    t[q++]=s[p++];
               } else
               {
                 if(operand)
                 {
			/* are we forcing the operand into a particular
			   addressing mode? !, @, ` operators 
			   Note these are not available in ca65, but we only
			   switch off "@" which are used for cheap local labels*/
                    if(s[p]=='!' || (s[p]=='@' && !ca65) || s[p]=='`')
                    {
		       t[q++]=T_CAST;
		       t[q++]=s[p];
                       /*cast=s[p];*/
                       operand= -operand+1;
                       p++;
                    } else
                    if(s[p]=='(' || s[p]=='-' || s[p]=='>' ||
			s[p]=='<' || s[p]=='[')
                    {
                         t[q++]=s[p++];
                         operand= -operand+1; /* invert to become reinverted */
                    } else
                    if(s[p]=='*')
                    {
                         t[q++]=s[p++];
                    } else
		    /* maybe it's a label 
 			Note that for ca65 cheap local labels, we check for "@" */
                    if(isalpha(s[p]) || s[p]=='_' || ((s[p]==':' || s[p]=='@') && ca65))
                    {
                       
			int p2 = 0; 
			if (n == (Klistbytes & 0xff)) {
				// check for "unlimited"
				// Note: this could be done by a more general "constants" handling,
				// where in appropriate places (like the one here), constants are
				// replaced by a pointer to a predefined constants info, e.g. using 
				// a T_CONSTANT. Which would also fix the listing of this constant
				// (which is currently listed as "0")
				static char *unlimited = "unlimited";
				while (s[p+p2] != 0 && unlimited[p2] != 0 && s[p+p2] == unlimited[p2]) p2++;
			} 
			if (p2 == 9) {	// length of "unlimited"
				er = E_OK;
				// found constant
                       		wval(q, 0, 'd');
				p += p2;
			} else {
			 //m=n;
                         er=l_search((char*)s+p,&ll,&n,&v,&afl);
/*
                         if(m==Kglobl || m==Kextzero) {
                              if(er==E_NODEF) {
                                  er=E_OK;
                              }
                              t[q++]=T_LABEL;
                              t[q++]=n & 255;
                              t[q++]=(n>>8) & 255;
                         } else
*/

                         if(!er)
                         {
                           if(afl) {
                             t[q++]=T_POINTER;
                             t[q++]=afl & 255;
                             t[q++]=v & 255;
                             t[q++]=(v>>8) & 255;
			     t[q++]=n & 255;		/* cheap fix for listing */
			     t[q++]=(n>>8) & 255;	/* why is the label already resolved in t_conv? */
                           } else {
                              t[q++]=T_LABEL;
                              t[q++]=n & 255;
                              t[q++]=(n>>8) & 255;
                             /*wval(q,v, 0);*/
                           }
                         } else
                         if(er==E_NODEF)
                         {
#ifdef DEBUG_AM
fprintf(stderr, "could not find %s\n", (char *)s+p);
#endif
                              t[q++]=T_LABEL;
                              t[q++]=n & 255;
                              t[q++]=(n>>8) & 255;
/*
                              if(afl==SEG_ZEROUNDEF) uz++;
*/
                              ud++;
                              er=E_OK;
                         }
                         p+=ll;
			}
                    }
                    else
                    if(s[p]<='9' && (s[p]>'0' || (s[p] == '0' && !ctypes)))
                    {
                         tg_dez(s+p,&ll,&v);
                         p+=ll;
                         wval(q,v, 'd');
                    }
                    else
		    /* handle encodings: hex, binary, octal, quoted strings */
                    switch(s[p]) {
                    case '0':
			 // only gets here when "ctypes" is set, and starts with 0
			 // we here check for the C stype "0xHEX" and "0OCTAL" encodings
			 if ('x' == tolower(s[p+1])) {
				// c-style hex
				tg_hex(s+p+2, &ll, &v);
				p+=2+ll;
				wval(q, v, '$');
			 } else {
				// c-style octal
                         	tg_oct(s+p+1,&ll,&v);
                         	p+=1+ll;
                         	wval(q,v, '&');
			 }
			 break;
                    case '$':
                         tg_hex(s+p+1,&ll,&v);
                         p+=1+ll;
                         wval(q,v, '$');
                         break;
                    case '%':
                         tg_bin(s+p+1,&ll,&v);
                         p+=1+ll;
                         wval(q,v, '%');
                         break;
                    case '&':
                         tg_oct(s+p+1,&ll,&v);
                         p+=1+ll;
                         wval(q,v, '&');
                         break;
                    case '\'':
                    case '\"':
                         er=tg_asc(s+p,t+q,&q,&p,na1,na2,n);
                         break;
                    case ',':
                         if(mk)
                              while(s[p]!='\0' && s[p]!=';')
                              {
                                   while(s[p]==' ') p++;
                                   *nk+=(s[p]==',');
                                   t[q++]=s[p++];
                              }
                         else
                         {
                              *nk+=1;
                              t[q++]=s[p++];
                         }
                         break;
                    default :
                         er=E_SYNTAX;
                         break;
                    }
                    operand= -operand+1;

                 } else    /* operator    */
                 {
                    o=0;
                    if(s[p]==')' || s[p]==']')
                    {
                         t[q++]=s[p++];
                         operand =-operand+1;
                    } else
                    if(s[p]==',')
                    {
                         t[q++]=s[p++];
                         if(mk)
                              fl++;
                         *nk+=1;
                    } else
                    switch(s[p]) {
                    case '+':
                         o=1;
                         break;
                    case '-':
                         o=2;
                         break;
                    case '*':
                         o=3;
                         break;
                    case '/':
                         o=4;
                         break;
                    case '<':
                         switch (s[p+1]) {
                         case '<':
                              o=6;
                              break;
                         case '>':
                              o=12;
                              break;
                         case '=':
                              o=10;
                              break;
                         default :
                              o=7;
                              break;
                         }
                         break;
                    case '>':
                         switch (s[p+1]) {
                         case '>':
                              o=5;
                              break;
                         case '<':
                              o=12;
                              break;
                         case '=':
                              o=11;
                              break;
                         default:
                              o=8;
                              break;
                         }
                         break;
                    case '=':
                         switch (s[p+1]) {
                         case '<':
                              o=10;
                              break;
                         case '>':
                              o=11;
                              break;
                         default:
                              o=9;
                              break;
                         }
                         break;
                    case '&':
                         if (s[p+1]=='&')
                              o=16;
                         else
                              o=13;
                         break;
                    case '|':
                         if (s[p+1]=='|')
                              o=17;
                         else
                              o=15;
                         break;
                    case '^':
                         o=14;
                         break;
                    default:
                         er=E_SYNTAX;
                         break;
                    }
                    if(o)
                    {
                         t[q++]=o;
                         p+=lp[o];
                    }
                    operand= -operand+1;
                 }

                 while(s[p]==' ') p++;
               }
          }
     }
//printf("er=%d, ud=%d\n", er, ud);
     if(!er)
     {
/*
          if(uz==1 && ud==1 && byte!=2) {
               byte=1;
	  }
          if(byte == 1) {
               t[q++] = T_FBYTE;
          } else if(byte == 2) {
               t[q++] = T_FADDR;
          }
*/
	  byte = 0;
          if(ud > 0) {
               er=E_NODEF;
		byte = 1;
          }
     }
    
     if (s[p] == ';') {
	/* handle comments */
	/* first find out how long */
	int i;
	for (i = p+1; s[i] != '\0'; i++);
	i = i - p;	/* actual length of the comment, including zero-byte terminator */
	/*if (i >= 1) {*/
		/* there actually is a comment */
		t[q++] = T_COMMENT;
		t[q++] = i&255;
		t[q++] = (i>>8)&255;
		memcpy(t+q, s+p+1, i);	/* also copy zero terminator, used in listing */
		q += i;
	/*}*/
     } 
     t[q++]=T_END;
     /* FIXME: this is an unholy union of two "!" implementations :-( */
     /* FIXME FIXME FIXME ... 
     if (operand==1) {
         t[q++]='\0';
         t[q++]=cast;
     }
     */
     *l=q;
     if(bytep) *bytep=byte; 
     return(er);
}

/*********************************************************************************************
 * identifies a keyword in s, if it is found, starting with s[0]
 * A keyword is either a mnemonic, or a pseudo-opcode
 */
static int t_keyword(signed char *s, int *l, int *n)
{
     int i = 0;		// index into keywords
     int j = 0;
     int hash;

     // keywords either start with a character, a "." or "*"
     if(!isalpha(s[0]) && s[0]!='.' && s[0]!='*' )
          return(E_NOKEY);

     // if first char is a character, use it as hash...
     if(isalpha(s[0]))
          hash=tolower(s[0])-'a';
     else
          hash=26;
     

     // check for "*="
     if(s[0]=='*') {
 	j=1;
	while(s[j] && isspace(s[j])) j++;
	if(s[j]=='=') {
	  i=Kpcdef;
	  j++;
	}
     } 

     // no keyword yet found?
     if(!i) {    
       // get sub-table from hash code, and compare with table content
       // (temporarily) redefine i as start index in opcode table, and hash as end index
       i=ktp[hash];
       hash=ktp[hash+1];
       // check all entries in opcode table from start to end for that hash code
       while(i<hash)
       {
          j=0;
          while(kt[i][j]!='\0' && kt[i][j]==tolower(s[j]))
               j++;

          if((kt[i][j]=='\0') && ((i==Kpcdef) || ((s[j]!='_') && !isalnum(s[j]))))
               break;
          i++;
       }
     }    
     if(i==Kbyte) i=Kbyt;
     if(i==Kdupb) i=Kdsb;
     if(i==Kblkb) i=Kdsb;
     if(i==Kdb) i=Kbyt;
     if(i==Kdw) i=Kword;
     if(i==Kblock) i=Kopen;
     if(i==Kbend) i=Kclose;
     if(i==Kcode) i=Ktext;
     if(i==Kproc || i==Kscope) i=Kopen;
     if(i==Kendproc || i==Kendscope) i=Kclose;
     if(i==Kzeropage) i=Kzero;
     if(i==Korg) i=Kpcdef;
     if(i==Krelocx) i=Kpcdef;


     *l=j;
     *n=i;
     return( i==hash ? E_NOKEY : E_OK );
}

static void tg_dez(s,l,v)
signed char *s;
int *l,*v;
{
     int i=0,val=0;

     while(isdigit(s[i]))
          val=val*10+(s[i++]-'0');

     *l=i;
     *v=val;
}

static void tg_bin(signed char *s, int *l, int *v)
{
     int i=0,val=0;

     while(s[i]=='0' || s[i]=='1')
          val=val*2+(s[i++]-'0');

     *l=i;
     *v=val;
}

static void tg_oct(signed char *s, int *l, int *v)
{
     int i=0,val=0;

     while(s[i]<'8' && s[i]>='0')
          val=val*8+(s[i++]-'0');

     *l=i;
     *v=val;
}

static void tg_hex(signed char *s, int *l, int *v)
{
     int i=0,val=0;

     while((s[i]>='0' && s[i]<='9') || (tolower(s[i])<='f' && tolower(s[i])>='a'))
     {
          val=val*16+(s[i]<='9' ? s[i]-'0' : tolower(s[i])-'a'+10);
          i++;
     }
     *l=i;
     *v=val;
}

/* 
 * tokenize a string - handle two delimiter types, ' and " 
 */
static int tg_asc(signed char *s, signed char *t, int *q, int *p, int *na1, int *na2,int n)
{

     int er=E_OK,i=0,j=0;

     signed char delimiter = s[i++];
     
#ifdef DEBUG_AM
fprintf(stderr, "tg_asc token = %i\n", n);
#endif

     t[j++]='"';	/* pass2 token for string */
     j++;		/* skip place for length */

     while(s[i]!='\0' && s[i]!=delimiter)
     {
	/* do NOT convert for Kbin or Kaasc, or for initial parse */
	  if (!n || n == Kbin || n == Kaasc) {
		t[j++]=s[i];
          } else if(ca65 || s[i]!='^') { 	/* no escape code "^" - TODO: does ca65 has an escape code */
               t[j++]=convert_char(s[i]);
          } else { 		/* escape code */
		  signed char payload = s[i+1];
	          switch(payload) {
	          case '\0':
	               er=E_SYNTAX;
	               break;
	          case '\"':
		       if (payload == delimiter) {
	                 t[j++]=convert_char(payload);
	                 i++;
		       } else {
	                 er=E_SYNTAX;
		       }
	               break;
	          case '\'':
		       if (payload == delimiter) {
	                 t[j++]=convert_char(payload);
	                 i++;
		       } else {
	                 er=E_SYNTAX;
		       }
	               break;
	          case '^':
	               t[j++]=convert_char('^');
	               i++;
	               break;
	          default:
	               t[j++]=convert_char(payload&0x1f);
	               i++;
	               break;
	          }
	  }
          i++;
     }
     if(j==3)	/* optimize single byte string to value */
     {
          t[0]=T_VALUE;
          t[1]=t[2];
          t[2]=0;
          t[3]=0;
	  t[4]=delimiter;
          j+=2;
     } else
     {		/* handle as string */
          t[1]=j-2;
          *na1 +=1;
          *na2 +=j-2;
     }
     if(s[i]==delimiter) {	/* in case of no error */
          i++;			/* skip ending delimiter */
     }
     *q +=j;
     *p +=i;
     return(er);
}


