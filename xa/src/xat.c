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
/*
#define DEBUG_AM
*/

#include <ctype.h>
#include <stdio.h>

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

int dsb_len = 0;

static int t_conv(signed char*,signed char*,int*,int,int*,int*,int*,int,int*);
static int t_keyword(signed char*,int*,int*);
static int tg_asc(signed char*,signed char*,int*,int*,int*,int*,int);
static void tg_dez(signed char*,int*,int*);
static void tg_hex(signed char*,int*,int*);
static void tg_oct(signed char*,int*,int*);
static void tg_bin(signed char*,int*,int*);

/* assembly mnemonics and pseudo-op tokens */
/* ina and dea don't work yet */
static char *kt[] ={ 
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
     ".align",".block", ".bend",".al",".as",".xl",".xs", ".bin", ".aasc"

};

static int lp[]= { 0,1,1,1,1,2,2,1,1,1,2,2,2,1,1,1,2,2 };

/* mvn and mvp are handled specially, they have a weird syntax */
#define  Kmvp 38
#define  Kmvn Kmvp+1

/* index into token array for pseudo-ops */
/* last valid mnemonic */
#define   Lastbef   93
/* last valid token+1 */
#define   Anzkey    123

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

#define   Kreloc    Anzkey   	/* *= (relocation mode) */
#define   Ksegment  Anzkey+1

/* array used for hashing tokens (26 entries, a-z) */

static int ktp[]={ 0,3,17,25,28,29,29,29,29,32,34,34,38,40,41,42,58,
               58,65,76,90,90,90,92,94,94,94,Anzkey };

#define   Admodes   24

/* 
 * opcodes for each addressing mode
 * high byte: supported architecture (no bits = original NMOS 6502)
 *  bit 1: R65C02
 *  bit 2: 65816 and allows 16-bit quantity (accum only)
 *  bit 3: 65816 and allows 16-bit quantity (index only)
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

/* pass 1 */
int t_p1(signed char *s, signed char *t, int *ll, int *al)
{
     static int er,l,n,v,nk,na1,na2,bl,am,sy,i,label,byte; /*,j,v2 ;*/
     int afl = 0;

/* notes and typical conventions ... er = error code
	am = addressing mode in use
*/

     bl=0;
     *al = 0;

/*     printf("\n"); */

     /* convert the next token from string s */
#ifdef DEBUG_AM
fprintf(stderr, "- p1 %d starting -\n", pc[segment]);
#endif
     er=t_conv(s,t,&l,pc[segment],&nk,&na1,&na2,0,&byte);
     /* leaving our token sequence in t */

     *ll=l;
/*
     printf("t_conv (er=%d):",er);
     for(i=0;i<l;i++)
          printf("%02x,",t[i]);
     printf("\n");
*/
     /* if text/data produced, then no more fopt allowed in romable mode */
     if((romable>1) && (t[0]<Kopen || t[0]==Kbyte || t[0]==Kpcdef)) {
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
          n=t[0];
	  /* TODO: make that a big switch statement... */
	  /* maybe later. Cameron */

	  if(n==Kend || n==Klist || n==Kxlist) {
	    *ll = 0;		/* ignore */
	  } else
	  if(n==Kfopt) {
	    if(romable==1) er=E_ROMOPT;
	    t[0] = Kbyt;
	    set_fopt(l,t,nk+1-na1+na2);
	    *ll = 0;
	  } else
          if(n==Kpcdef)
          {
	       int tmp;
               if(!(er=a_term(t+1,&tmp /*&pc[SEG_ABS]*/,&l,pc[segment],&afl,&label,0)))
               {
                    i=1;
                    wval(i,tmp /*pc[SEG_ABS]*/);
                    t[i++]=T_END;
                    *ll=6;
                    er=E_OKDEF;
/*printf("set pc=%04x, oldsegment=%d, pc[segm]=%04x, ", 
				pc[SEG_ABS], segment, pc[segment]);
printf(" wrote %02x %02x %02x %02x %02x %02x\n",
				t[0],t[1],t[2],t[3],t[4],t[5]);*/
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
		   wval(i,pc[SEG_TEXT]);
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
	      segment = relmode ? SEG_TEXT : SEG_ABS;
	      t[0]=Ksegment;
	      t[1]=segment;
	      *ll=2;
              er=E_OKDEF;
	  } else
	  if(n==Kdata) {
  	    if(relmode) {   
	      segment = SEG_DATA;
	      t[0]=Ksegment;
	      t[1]=SEG_DATA;
	      *ll=2;
              er=E_OKDEF;
	    } else {
	      er=E_ILLSEGMENT;
	    } 
	  } else
	  if(n==Kbss) {
  	    if(relmode) { 
	      segment = SEG_BSS;
	      t[0]=Ksegment;
	      t[1]=SEG_BSS;
	      *ll=2;
              er=E_OKDEF;
	    } else {
	      er=E_ILLSEGMENT;
	    } 
	  } else
	  if(n==Kzero) {
  	    if(relmode) {   
	      segment = SEG_ZERO;
	      t[0]=Ksegment;
	      t[1]=SEG_ZERO;
	      *ll=2;
              er=E_OKDEF;
	    } else {
	      er=E_ILLSEGMENT;
	    }  
	  } else
	if (n==Kbin) {
		int j;
		int l;

		/* this first pass just calculates a prospective length
			for pass 2. */
		char binfnam[255];
		int offset;
		int length;
		int fstart;

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
		if (!er && t[i] != T_END) {
			er = E_SYNTAX;
		}

		if (!er) {
			FILE *foo;

#ifdef DEBUG_AM
			fprintf(stderr,
"binclude1 offset = %i len = %i filename = %s endchar = %i\n",
		offset, length, binfnam, i);
#endif
			if (!(foo = fopen(binfnam, "rb"))) {
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
		    wval(i,tmp);
                    t[i++]=',';
		    tmp2= 0xea;
		    wval(i,tmp2);	/* nop opcode */
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
fprintf(stderr, "E_OK ... t_p2 xat.c %i %i\n", t[0], *ll);
#endif
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

          if(n>=0 && n<=Lastbef && n != Kmvn && n != Kmvp) /* not for mvn/p */
          {
               if(t[1]==T_END)
               {
                    sy=0;	/* implied */
               } else
               if(t[1]=='#')
               {
                    sy=1+nk;	/* immediate */
               } else
               if(t[1]=='(')
               {
                    sy=7+nk;	/* computed */
               } else
                    sy=4+nk;	/* absolute or zero page */

	       /* length counter set to maximum length + 1 */
	       if (w65816 || (t[l-1]=='@' || t[l-1] == '!')) {
		       	/* for 65816 allow addressing modes up to 4 byte overall length */
               		bl=Maxbyt+1;
	       } else {
		       	/* for other modes only check for addressing modes up to 3 byte overall length */
		 	bl=Maxbyt;
	       }
               
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
               if (t[l-1]!='@' && t[l-1] != '!')
               {
                     if(bl && !er && opt[am]>=0 && am>16) /* <<< NOTE! */
                          if(ct[n][opt[am]]>=0)
                               am=opt[am];
               }
		/* if ` is declared, force further optimization */
		if (t[l-1]=='`') {
			if (opt[am]<0 || ct[n][opt[am]]<0)
				errout(E_ADRESS);
			am=opt[am];
		}
		/* if ! is declared, force to 16-bit quantity */
		if (t[l-1]=='!' && am>16 && opt[am]>=0 && bl) {
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
	  if(n==Kmvn || n==Kmvp)
	  {
               bl=3;
		if (!w65816) er = E_65816;
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

     return(er);
}

/*t_pass 2*/
int t_p2(signed char *t, int *ll, int fl, int *al)
{
     static int afl,nafl, i,j,k,er,v,n,l,bl,sy,am,c,vv[3],v2,label;
     static int rlt[3];	/* relocation table */
     static int lab[3];	/* undef. label table */

#if(0)
     (void)fl;		/* quench warning */
#endif
/* fl was not used in 2.2.0 so I'm overloading it for zp-optimization
     control */

     er=E_OK;
     bl=0;
     if(*ll<0) /* <0 bei E_OK, >0 bei E_OKDEF     */
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
				&& ((afl & A_FMASK)!=(SEG_UNDEF<<8)) )
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
               while(!er && t[i]!=T_END)
               {
                    if(!(er=a_term(t+i,&v,&l,pc[segment],&afl,&label,1)))
                    {   
/*if(afl) printf("relocation 1 %04x at pc=$%04x, value now =$%04x\n",
							afl,pc[segment],v); */
			 if(afl) u_set(pc[segment]+j, afl, label, 2);
                         t[j++]=v&255;
                         t[j++]=(v>>8)&255;

                         i+=l;     
                         if(t[i]!=T_END && t[i]!=',')
                              er=E_SYNTAX;
                         else
                         if(t[i]==',')
                              i++;

                    }
               }
               *ll=j;
               bl=j;
          } else
          if (n == Kbin)
          {
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
		if (!er && t[i] != T_END) {
			er = E_SYNTAX;
		}

		if (!er) {
			FILE *foo;

#ifdef DEBUG_AM
			fprintf(stderr,
"binclude2 offset = %i len = %i filename = %s endchar = %i\n",
		offset, length, binfnam, i);
#endif
			if (!(foo = fopen(binfnam, "rb"))) {
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
	} else
	if (n==Kmvn || n==Kmvp)
	{
	/* special case these instructions' syntax */
		int wide=0;
		i=1;
		j=1;
		/* write opcode */
		t[0] = ((n == Kmvp) ? 0x44 : 0x54);
		while(!er && t[i]!=T_END)
		{
			if (wide) /* oops */
				er = E_SYNTAX;
#ifdef DEBUG_AM
fprintf(stderr, "mvn mvp: %i %i %i %i %i\n", t[0], t[i], wide, i, j);
#endif
			if(!(er=a_term(t+i,&v,&l,pc[segment],&afl,&label,1)))
			{   
/*if(afl) printf("relocation 1 %04x at pc=$%04x, value now =$%04x\n",
							afl,pc[segment],v); */
				if(afl) u_set(pc[segment]+j, afl, label, 2);
				i+=l;     
			/* for backwards compatibility, accept the old
				mv? $xxxx syntax, but issue a warning.
				mv? $00xx can be legal, so accept that too. */
				if ((v & 0xff00) || (j==1 && t[i]==T_END)) {
					errout(W_OLDMVNS);
					wide = 1;
					t[j++] = ((v & 0xff00) >> 8);
					t[j++] = (v & 0x00ff);
				} else {
					t[j++] = v;
				}
			}
			if (j > 3)
				er=E_SYNTAX;
			if(t[i]!=T_END && t[i]!=',')
				er=E_SYNTAX;
			else
			if(t[i]==',')
                              i++;
		}
		if (j != 3) er = E_SYNTAX; /* oops */

		/* before we leave, swap the bytes. although disassembled as
			mv? src,dest it's actually represented as
			mv? $ddss -- see 
			http://6502org.wikidot.com/software-65816-memorymove */
		i = t[2];
		t[2] = t[1];
		t[1] = i;

		*ll = j;
		bl = j;
		if (!w65816) er = E_65816;
	} else if(n==Kasc || n==Kbyt || n==Kaasc) {
               i=1;
               j=0;
               while(!er && t[i]!=T_END)
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
                    if(t[i]!=T_END && t[i]!=',')
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
                    if (j<0)
			er=E_SYNTAX;
		    else
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
          if(n<=Lastbef)
          {
/* instruction */
               if((c=t[1])=='#')
               {
                    i=2;
                    sy=1;
                    if(!(er=a_term(t+i,vv,&l,pc[segment],&afl,&label,1)))
                    {
/* if(1) printf("a_term returns afl=%04x\n",afl); */

			 rlt[0] = afl;
			 lab[0] = label;
                         i+=l;
                         if(t[i]!=T_END)
                         {
                              if(t[i]!=',')
                                   er=E_SYNTAX;
                              else
                              {
                                   i++;
                                   sy++;
                                   if(!(er=a_term(t+i,vv+1,&l,pc[segment],&afl,&label,1)))
                                   {
			                rlt[1] = afl;
					lab[1] = label;
                                        i+=l;
                                        if(t[i]!=T_END)
                                        {
                                             if(t[i]!=',')
                                                  er=E_SYNTAX;
                                             else
                                             {
                                                  i++;
                                                  sy++;
                                                  if(!(er=a_term(t+i,vv+2,&l,pc[segment],&afl,&label,1)))
                                                  {
			                               rlt[2] = afl;
						       lab[2] = label;
                                                       i+=l;
                                                       if(t[i]!=T_END)
                                                            er=E_SYNTAX;
                                                  }
                                             }
                                        }
                                   }
                              }
                         }
                    }
               } else
               if(c==T_END)
               {
                    sy=0;
               } else
               if(c=='(')
               {
                    sy=7;
                    if(!(er=a_term(t+2,vv,&l,pc[segment],&afl,&label,1)))
                    {
			 rlt[0] = afl;
			 lab[0] = label;

                         if(t[2+l]!=T_END)
                         {
                              if(t[2+l]==',')
                              {
                                   if (tolower(t[3+l])=='x')
                                        sy=8;
                                   else
                                        sy=13;

                              } else
                              if(t[2+l]==')')
                              {
                                   if(t[3+l]==',')
                                   {
                                        if(tolower(t[4+l])=='y')
                                             sy=9;
                                        else
                                             er=E_SYNTAX;
                                   } else
                                   if(t[3+l]!=T_END)
                                        er=E_SYNTAX;
                              } 
                         } else
                              er=E_SYNTAX;
                    }
               } else
               if(c=='[')
               {
                    sy=10;
                    if(!(er=a_term(t+2,vv,&l,pc[segment],&afl,&label,1)))
                    {
                         rlt[0] = afl;
			 lab[0] = label;

                         if(t[2+l]!=T_END)
                         {
                              if(t[2+l]==']')
                              {
                                   if(t[3+l]==',')
                                   {
                                        if(tolower(t[4+l])=='y')
                                             sy=11;
                                        else
                                             er=E_SYNTAX;
                                   } else
                                   if(t[3+l]!=T_END)
                                        er=E_SYNTAX;
                              } 
                         } else
                              er=E_SYNTAX;
                    }
               } else
               {
                    sy=4;
                    if(!(er=a_term(t+1,vv,&l,pc[segment],&afl,&label,1)))
                    {
			 rlt[0] = afl;
			 lab[0] = label;
                         if(t[1+l]!=T_END)
                         {
                              if(t[1+l]==',')
                              {
                                   if(tolower(t[2+l])=='y')
                                        sy=6;
                                   else
                                   if(tolower(t[2+l])=='s')
                                        sy=12;
                                   else
                                        sy=5;
                              } else
                                   er=E_SYNTAX;
                         }
                    }
               }
               
	       /* set bl to maximum overall length +1 as while() below starts with decrementing it */
	       if (w65816 || (t[*ll-1]=='@' || t[*ll-1] == '!')) {
		       	/* for 65816 allow addressing modes up to 4 byte overall length */
               		bl=Maxbyt+1;
	       } else {
		       	/* for other modes only check for addressing modes up to 3 byte overall length */
		 	bl=Maxbyt;
	       }
              
#ifdef DEBUG_AM
	      printf("--- trying to find am using: (max+1) bl=%d, sy=%d\n", bl, sy); 
#endif
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
               if (t[*ll-1]!='@')
               {
#ifdef DEBUG_AM
fprintf(stderr,
"b4: pc= %d, am = %d and vv[0] = %d, optimize = %d, bitmask = %u, er=%d, bl=%d\n",
	pc[segment], am, vv[0], fl, (vv[0]&0xffff00), er, bl);
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
"aftaa1: pc= %d, am = %d and vv[0] = %d, optimize = %d, bitmask = %d, bl = %d\n",
	pc[segment], am, vv[0], fl, (vv[0]&0xffff00), bl);
#endif
                    if(t[*ll-1]!='!') {
                         if(bl && !er && !(vv[0]&0xffff00) && opt[am]>=0) {
                              if(ct[n][opt[am]]>=0) {
				if (!fl || t[*ll-1]=='`') {
                                   	am=opt[am];
				} else {
					errout(W_FORLAB);
				}
			      }
			}
                    }
#ifdef DEBUG_AM
fprintf(stderr,
"aftaa2: pc=%d, am=%d and vv[0]=%d, optimize=%d, bitmask=%d, op=%d, bl=%d\n",
	pc[segment], am, vv[0], fl, (vv[0]&0xffff00), ct[n][opt[am]], bl);
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
		    if ((am != 11 && am != 16) && (vv[0] > 255 || vv[0] < -256) && bl == 2) {
			    er = E_OVERFLOW;
		    } else
		    if ((am != 11 && am != 16) && (vv[0] > 65535 || vv[0] < -65536) && (bl == 2 || bl == 3)) {
			    er = E_OVERFLOW;
		    }
                    *ll=bl;
               }

#ifdef DEBUG_AM
fprintf(stderr, "byte length is now %d, am=%d, er=%d\n", bl, am, er);
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
			   /* relative, relative long */
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
     
/* translate a string into a first-pass sequence of tokens */
static int t_conv(signed char *s, signed char *t, int *l, int pc, int *nk,
             int *na1, int *na2, int af, int *bytep)  /* Pass1 von s nach t */
/* tr. pass1, from s to t */
{
     static int v,f;
     static int operand,o;
     int fl,afl;
     int p,q,ud,n,ll,mk,er;
     int m, uz, byte;
     static unsigned char cast;

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

     while(s[p]==' ') p++;

     n=T_END;
     cast='\0';

     if(!af)
     {
          while(s[p]!='\0' && s[p]!=';')
          {

		/* is keyword? */
               if(!(er=t_keyword(s+p,&ll,&n)))
                    break;

		/* valid syntax, but just not a real token? */
               if(er && er!=E_NOKEY)
                    break;

		/* if so, try to understand as label */
               if((er=l_def((char*)s+p,&ll,&n,&f)))
                    break;

               p+=ll;

               while(s[p]==' ') p++;

               if(s[p]=='=')
               {
                    t[q++]=T_OP;
                    t[q++]=n&255;
                    t[q++]=(n>>8)&255;
                    t[q++]='=';
                    p++;
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
                    n=0;
               } else {		/* label ... syntax */
                    l_set(n,pc,segment);        /* set as address value */
                    n=0;
               }

          }

          if(n != Kmvn && n != Kmvp && ((n & 0xff) <=Lastbef))
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
				addressing mode? !, @, ` operators */
                    if(s[p]=='!' || s[p]=='@' || s[p]=='`')
                    {
                       cast=s[p];
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
		/* maybe it's a label */
                    if(isalpha(s[p]) || s[p]=='_')
                    {
                         m=n;
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
                           } else {
                             wval(q,v);
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
                    else
                    if(s[p]<='9' && s[p]>='0')
                    {
                         tg_dez(s+p,&ll,&v);
                         p+=ll;
                         wval(q,v);
                    }
                    else

		/* handle encodings: hex, binary, octal, quoted strings */
                    switch(s[p]) {
                    case '$':
                         tg_hex(s+p+1,&ll,&v);
                         p+=1+ll;
                         wval(q,v);
                         break;
                    case '%':
                         tg_bin(s+p+1,&ll,&v);
                         p+=1+ll;
                         wval(q,v);
                         break;
                    case '&':
                         tg_oct(s+p+1,&ll,&v);
                         p+=1+ll;
                         wval(q,v);
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
#if(0)
                         uz++; /* disable 8-bit detection */
#endif
                    }
                    operand= -operand+1;
                 }

                 while(s[p]==' ') p++;
               }
          }
     }
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
          t[q++]=T_END;
          if(ud > 0) {
               er=E_NODEF;
		byte = 1;
          }
     }
     /* FIXME: this is an unholy union of two "!" implementations :-( */
     t[q++]='\0';
     t[q++]=cast;
     *l=q;
     if(bytep) *bytep=byte; 
     return(er);
}

static int t_keyword(signed char *s, int *l, int *n)
{
     int i = 0, j = 0, hash;

     if(!isalpha(s[0]) && s[0]!='.' && s[0]!='*' )
          return(E_NOKEY);

     if(isalpha(s[0]))
          hash=tolower(s[0])-'a';
     else
          hash=26;
     

     if(s[0]=='*') {
 	j=1;
	while(s[j] && isspace(s[j])) j++;
	if(s[j]=='=') {
	  i=Kpcdef;
	  j++;
	}
     } 
     if(!i) {    
       i=ktp[hash];
       hash=ktp[hash+1];
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
          } else if(s[i]!='^') { 	/* no escape code "^" */
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
          j++;
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

