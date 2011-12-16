/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-2006 André Fachat (a.fachat@physik.tu-chemnitz.de)
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
#ifndef __XA65_XA_CHARSET_H__
#define __XA65_XA_CHARSET_H__

/* set the target character set the chars - values in quotes - should
   be converted to 
   returns 0 on success and -1 when the name is not found */
int set_charset(char *charset_name);

/* convert a char */
signed char convert_char(signed char c);

#endif /*__XA65_XA_H__ */

