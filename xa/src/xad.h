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
#ifndef __XA65_XAD_H__
#define __XA65_XAD_H__

#ifndef abs
#define abs(a)		(a >= 0) ? a : -a
#endif

#define hashcode(n, l)	(n[0] & 0x0f) | (((l - 1) ? (n[1] & 0x0f) : 0) << 4)
#define fputw(a, fp)	do {						\
				fputc(a & 255, fp);			\
				fputc((a >> 8) & 255, fp);		\
			} while (0)

#define cval(s)		256 * ((s)[1] & 255) + ((s)[0]&255)
#define lval(s)		65536 * ((s)[2] & 255) + 256 * ((s)[1] & 255) + ((s)[0] & 255)
#define wval(i, v)	do {						\
				t[i++] = T_VALUE;			\
				t[i++] = v & 255;			\
				t[i++] = (v >> 8) & 255;		\
				t[i++] = (v >> 16) & 255;		\
			} while (0)

#endif /* __XA65_XAD_H__ */
