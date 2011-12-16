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
#ifndef __XA65_XAT_H__
#define __XA65_XAT_H__

extern int dsb_len;

void list_setfile(FILE *fp);
int t_p1(signed char *s, signed char *t, int *ll, int *al);
int t_p2_l(signed char *t, int *ll, int *al);
int b_term(char *s, int *v, int *l, int pc);

void list_line(int l);		/* set line number for the coming listing output */
void list_filename(char *fname);/* set file name for the coming listing output */

#endif /* __XA65_XAT_H__ */
