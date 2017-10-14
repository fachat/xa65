/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 * Maintained by Cameron Kaiser
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
#ifndef __XA65_XAH_H__
#define __XA65_XAH_H__

#define ANZLAB		5000	/* mal 14 -> Byte */
#define LABMEM		40000L
#define MAXLAB		32
#define MAXBLK		16
#define MAXFILE		7
#define MAXLINE		2048
#define MAXPP		40000L
#define ANZDEF		2340	/* mal 14 -> Byte, ANZDEF * 14 < 32768 */
#define TMPMEM		200000L	/* Zwischenspeicher von Pass1 nach Pass 2 */

typedef struct LabOcc {
	struct LabOcc *next;
	int line;
	char *fname;
} LabOcc;

typedef struct {
	int blk;
	int val;
	int len;
	int fl;		/* 0 = label value not valid/known,
			 * 1 = label value known
			 */
	int afl;	/* 0 = no address (no relocation), 1 = address label */
	int nextindex;
	char *n;
	struct LabOcc *occlist;
} Labtab;

typedef struct {
	char *search;
	int s_len;
	char *replace;
	int p_anz;
	int nextindex;
} List;


#define MEMLEN		(4 + TMPMEM + MAXPP + LABMEM +			\
			 (long)(sizeof (Labtab) * ANZLAB) +		\
			 (long)(sizeof (List) * ANZDEF))

#define DIRCHAR		'/'
#define DIRCSTRING	"/"
/* for Atari:
#define DIRCHAR		'\\'
#define DIRCSTRING	"\\"
*/

#define	BUFSIZE		4096	/* File-Puffegroesse (wg Festplatte) */

#define E_OK		 0	/* Fehlernummern */
#define E_SYNTAX	-1	/* Syntax Fehler */
#define E_LABDEF	-2	/* Label definiert */
#define E_NODEF		-3	/* Label nicht definiert */
#define E_LABFULL	-4	/* Labeltabelle voll */
#define E_LABEXP	-5	/* Label erwartet */
#define E_NOMEM		-6	/* kein Speicher mehr */
#define E_ILLCODE	-7	/* Illegaler Opcode */
#define E_ADRESS	-8	/* Illegale Adressierung */
#define E_RANGE		-9	/* Branch out of range */
#define E_OVERFLOW	-10	/* Ueberlauf */
#define E_DIV		-11	/* Division durch Null */
#define E_PSOEXP	-12	/* Pseudo-Opcode erwartet */
#define E_BLKOVR	-13	/* Block-Stack Uebergelaufen */
#define E_FNF		-14	/* File not found (pp) */
#define E_EOF		-15	/* End of File */
#define E_BLOCK		-16	/* Block inkonsistent */
#define E_NOBLK		-17
#define E_NOKEY		-18
#define E_NOLINE	-19
#define E_OKDEF		-20	/* okdef */
#define E_DSB		-21
#define E_NEWLINE	-22
#define E_NEWFILE	-23
#define E_CMOS		-24
#define E_ANZPAR	-25
#define E_ILLPOINTER	-26	/* illegal pointer arithmetic! */
#define E_ILLSEGMENT	-27	/* illegal pointer arithmetic! */
#define E_OPTLEN	-28	/* file header option too long */
#define E_ROMOPT	-29	/* header option not directly after
				 * file start in romable mode
				 */
#define E_ILLALIGN	-30	/* illegal align value */

#define E_65816		-31

#define E_ORECMAC	-32	/* exceeded recursion limit for label eval */
#define E_OPENPP	-33	/* open preprocessor directive */
#define E_OUTOFDATA	-34	/* out of data */
#define E_ILLQUANT	-35	/* generic illegal quantity error */
#define E_BIN		-36	/* okdef */
/* errors thru 64 are placeholders available for use */

#define W_ADRRELOC	-65	/* word relocation in byte value */
#define W_BYTRELOC	-66	/* byte relocation in word value */
#define E_WPOINTER	-67	/* illegal pointer arithmetic!   */
#define W_ADDRACC	-68	/* addr access to low or high byte pointer */
#define W_HIGHACC	-69	/* high byte access to low byte pointer */
#define W_LOWACC	-70	/* low byte access to high byte pointer */
#define W_FORLAB	-71	/* no zp-optimization for a forward label */
#define W_OPENPP	-72	/* warning about open preprocessor directive */
#define W_OVER64K	-73	/* included binary over 64K in 6502 mode */
#define W_OVER16M	-74	/* included binary over 16M in 65816 mode */
#define W_OLDMVNS	-75	/* use of old mv? $xxxx syntax */
/* warnings 76-77 are placeholders available for use */

#define T_VALUE		-1
#define T_LABEL		-2
#define T_OP		-3
#define T_END		-4
#define T_LINE		-5
#define T_FILE		-6
#define T_POINTER	-7

#define P_START		 0	/* Prioritaeten fuer Arithmetik    */
#define P_LOR		 1	/* Von zwei Operationen wird immer */
#define P_LAND		 2	/* die mit der hoeheren Prioritaet */
#define P_OR		 3	/* zuerst ausgefuehrt              */
#define P_XOR		 4
#define P_AND		 5
#define P_EQU		 6
#define P_CMP		 7
#define P_SHIFT		 8
#define P_ADD		 9
#define P_MULT		10
#define P_INV		11

#define A_ADR		0x8000	/* all are or'd with (afl = segment type)<<8 */
#define A_HIGH		0x4000	/* or'd with the low byte */
#define A_LOW		0x2000
#define A_MASK		0xe000	/* reloc type mask */
#define A_FMASK		0x0f00	/* segment type mask */

#define A_LONG		0xc000

#define FM_OBJ		0x1000
#define FM_SIZE		0x2000
#define FM_RELOC	0x4000
#define FM_CPU		0x8000

#define SEG_ABS		0
#define SEG_UNDEF	1
#define SEG_TEXT	2
#define SEG_DATA	3
#define SEG_BSS		4
#define SEG_ZERO	5
#define SEG_MAX         6

typedef struct Fopt {
	signed char *text;	/* text after pass1 */
	int len;
} Fopt;

typedef struct relocateInfo {
	int next;
	int adr;
	int afl;
	int lab;
} relocateInfo;

typedef struct File {
	int fmode;
	int slen;
	int relmode;
	int old_abspc;
	int base[SEG_MAX];
	int len[SEG_MAX];
	struct {
		signed char *tmp;
		unsigned long tmpz;
		unsigned long tmpe;
	} mn;
	struct {
		int *ulist;
		int un;
		int um;
	} ud;
	struct {
		relocateInfo *rlist;
		int mlist;
		int nlist;
		int first;
	} rt;
	struct {
		relocateInfo *rlist;
		int mlist;
		int nlist;
		int first;
	} rd;
	struct {
		Fopt *olist;
		int mlist;
		int nlist;
	} fo;
	struct {
		int hashindex[256];
		Labtab *lt;
		int lti;
		int ltm;
	} la;
} File;

extern File *afile;

#endif /* __XA65_XAH_H__ */
