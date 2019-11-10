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
#ifndef __XA65_XA_H__
#define __XA65_XA_H__

#include "xah.h"	/* For SEG_MAX */

extern int ncmos, cmosfl, w65816, n65816;
extern int masm, nolink, ppinstr;
extern int noglob;
extern int showblk;
extern int relmode;
extern int crossref;
extern char altppchar;

extern int tlen, tbase;
extern int blen, bbase;
extern int dlen, dbase;
extern int zlen, zbase;
extern int romable, romaddr;

extern int memode,xmode;
extern int segment;
extern int pc[SEG_MAX];

int h_length(void);

void set_align(int align_value);

void errout(int er);
void logout(char *s);

#endif /*__XA65_XA_H__ */
