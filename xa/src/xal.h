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
#ifndef __XA65_XAL_H__
#define __XA65_XAL_H__

#include <stdio.h>	/* for FILE */

/* nasty stuff - "lz" is exported from xal.c so xa.c can print the name
 * of the label that was last searched for in the error message that the
 * label was not found... 
 */
extern char *lz;


int l_init(void);
int ga_lab(void);
int gm_lab(void);
long gm_labm(void);
long ga_labm(void);

int lg_set(char *);
int lg_import(int);
int lg_importzp(int);
// used to re-define undef'd labels as global for -U option
int lg_toglobal(char *);

int b_init(void);
int b_depth(void);

void printllist(FILE *fp);
int ga_blk(void);

int l_def(char *s, int* l, int *x, int *f);
int l_search(char *s, int *l, int *x, int *v, int *afl);
void l_set(int n, int v, int afl);
char* l_get_name(int n, xalabel_t *is_cll);
char* l_get_unique_name(int n);
int l_get(int n, int *v, int *afl);
int l_vget(int n, int *v, char **s);
int ll_search(char *s, int *n, xalabel_t labeltype);
int ll_pdef(char *t);

int b_open(void);
int b_close(void);

int l_write(FILE *fp);

#endif /* __XA65_XAL_H__ */
