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
#ifndef __XA65_XAP_H__
#define __XA65_XAP_H__

#include "xah2.h"	/* for Datei */

int pp_comand(char *t);
int pp_init(void);
int pp_open(char *name);
int pp_define(char *name);
void pp_close(void);
void pp_end(void);
int pgetline(char *t);
Datei *pp_getidat(void);

int ga_pp(void);
int gm_pp(void);
long gm_ppm(void);
long ga_ppm(void);

extern Datei *filep;
extern char s[MAXLINE];

#endif /* __XA65_XAP_H__ */
