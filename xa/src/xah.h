/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 Andr� Fachat (a.fachat@physik.tu-chemnitz.de)
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

/* 
 * Note: the computations to get the number of bytes necessary to allocate are
 * a historic remnant of the Atari ST (the original platform) not being able to efficiently allocate small chunks
 * of memory so I had to allocate a large chunk myself and manage the tables myself.
 * This has changed and some parts of xa65 are modified to just do a malloc() now.
 * These fixed numbers should actually go away. AF 20110623
 */
#define ANZLAB		5000	/* multiplied by sizeof(Labtab) -> Byte */
#define LABMEM		40000L
#define MAXLAB		32
#define MAXBLK		16
#define MAXFILE		7
#define MAXLINE		2048
#define MAXPP		40000L
#define ANZDEF		2340	/* multiplied by sizeof(List) -> Byte, ANZDEF * 20 < 32768 */
#define TMPMEM		2000000L	/* temporary memory buffer from Pass1 to Pass 2 (includes all source, thus enlarged) */

typedef enum {
        STD = 0,
        CHEAP = 1,
        UNNAMED = 2,
        UNNAMED_DEF = 3
} xalabel_t;

typedef struct LabOcc {
	struct LabOcc *next;
	int line;
	char *fname;
} LabOcc;

/**
 * struct that defines a label, after it has been parsed
 */
typedef struct {
	int blk;
	int origblk;	// only for fl=3
	int val;
	int len;
	int fl;		/* 0 = label value not valid/known,
			 * 1 = label value known
			 * 2 = label value not known, external global label (imported on link)
			 * 3 = label value not known, temporarily on external global label list (for -U)
			 */
	int afl;	/* 0 = no address (no relocation), 1 = address label */
	int nextindex;
	xalabel_t is_cll;	/* 0 is normal label, 1 is cheap local label (used for listing) */
	char *n;
	struct LabOcc *occlist;
	// within a block, make a linked list for the unnamed label counting
	// use indexes, as the label table can be re-alloced (so pointers change)
	// -1 is the "undef'd" end of list
	int blknext;
	int blkprev;
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

#define E_OK		 0	/* No error */
#define E_SYNTAX	-1	/* Syntax error */
#define E_LABDEF	-2	/* Label already defined (duplicate label definition) */
#define E_NODEF		-3	/* Label not defined */
#define E_LABFULL	-4	/* Label table full */
#define E_LABEXP	-5	/* Label expected but not found */
#define E_NOMEM		-6	/* out of memory */
#define E_ILLCODE	-7	/* Illegal Opcode */
#define E_ADRESS	-8	/* Illegal Addressing mode */
#define E_RANGE		-9	/* Branch out of range */
#define E_OVERFLOW	-10	/* overflow */
#define E_DIV		-11	/* Division by zero */
#define E_PSOEXP	-12	/* Pseudo-Opcode expected but not found */
#define E_BLKOVR	-13	/* Block-Stack overflow */
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
#define E_UERROR	-37	/* #error */
#define	E_AERROR	-38	/* .assert failed */
#define	E_NEGDSBLEN	-39	/* .dsb has negative length */

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
#define	W_SUBTRACT	-75	/* subtract a segment pointer from a constant - not supported in -R mode */
/* warnings 75-77 are placeholders available for use */

/* Meta-values for the token list. Note must not overlap with the
 * K* definitions in xat.c, which have outgrown the positive numbers
 * and are now growing up from -128 ... */
#define T_VALUE		-1	/* following is a 24 bit value in the token list */
#define T_LABEL		-2	/* referring to a label, following the token is the 16bit label number */
#define T_OP		-3	/* label oriented operation; following is the label number (16bit), plus the operation char (e.g. '+') */
#define T_END		-4	/* end of line marker */
#define T_LINE		-5	/* new line indicator; following is the 16 bit line number */
#define T_FILE		-6	/* new file indicator; following is the 16 bit line number, then the file name (zero-term) */
#define T_POINTER	-7	/* ??? */
#define T_COMMENT	-8	/* unused */
#define T_DEFINE	-9	/* define a label; inserted at conversion and discarded in pass1, only used in listing output */
#define T_LISTING	-10	/* meta token, inserted after conversion before pass1, used after pass2 to create listing */
#define T_CAST		-11	/* token inserted for a cast */

#define P_START		 0	/* arithmetic operation priorities    */
#define P_LOR		 1	/* of any two operations, the one with */
#define P_LAND		 2	/* the higher priority will be done first */
#define P_OR		 3	
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

#define SEG_UNDEFZP     7	/* is being mapped to UNDEF */

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
		// temporary memory between pass1 and pass2
		signed char *tmp;
		// write pointer
		unsigned long tmpz;
		// read pointer
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
