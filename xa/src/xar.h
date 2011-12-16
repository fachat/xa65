/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
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
#ifndef __XA65_XAR_H__
#define __XA65_XAR_H__

#define	RMODE_ABS	0
#define	RMODE_RELOC	1

extern File *alloc_file(void);

/* jumps to r[td]_set, depending on segment */
int r_set(int pc, int reloc, int len);
int u_set(int pc, int reloc, int label, int len);

int rt_set(int pc, int reloc, int len, int label);
int rd_set(int pc, int reloc, int len, int label);
int rt_write(FILE *fp, int pc);
int rd_write(FILE *fp, int pc);

void r_mode(int mode);

/* int rmode; */

int h_write(FILE *fp, int mode, int tlen, int dlen, int blen, int zlen,
	   int stacklen);

void seg_start(int fmode, int tbase, int dbase, int bbase, int zbase,
	      int stacklen, int relmode);
void seg_end(FILE *);
void seg_pass2(void);

#endif /* __XA65_XAR_H__ */
