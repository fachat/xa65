/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 Andre Fachat (a.fachat@physik.tu-chemnitz.de)
 * Maintained by Cameron Kaiser
 *
 * Charset conversion module
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


#include <stdio.h>
#include <string.h>

#include "xacharset.h"

static signed char (*convert_func)(signed char);

static signed char convert_char_ascii(signed char c) {
	return c;
}

/* 
 * PETSCII conversion roughly follows the PETSCII to UNICODE 
 * mapping at http://www.df.lth.se/~triad/krad/recode/petscii_c64en_lc.txt
 * linked from wikipedia http://en.wikipedia.org/wiki/PETSCII
 */
static signed char convert_char_petscii(signed char c) {
	if (c >= 0x41 && c < 0x5b) {
		return c + 0x20;
	}
	if (c >= 0x61 && c < 0x7b) {
		return c - 0x20;
	}
	if (c == 0x7f) {
		return 0x14;
	}
	return c;
}

/*
 * Built upon Steve Judd's suggested PETSCII -> screen code algorithm
 * This could probably be written a lot better, but it works.
 * http://www.floodgap.com/retrobits/ckb/display.cgi?572
 */
static signed char convert_char_petscreen(signed char c) {
	int i;

	i = (int)convert_char_petscii(c);
#ifdef SIGH
fprintf(stderr, "input: %i output: %i\n", c, i);
#endif
	if (i< 0)
		i += 0x80;
	i ^= 0xe0;
#ifdef SIGH
fprintf(stderr, "(1)input: %i output: %i\n", c, i);
#endif
	i += 0x20;
	i &= 0xff;
#ifdef SIGH
fprintf(stderr, "(2)input: %i output: %i\n", c, i);
#endif
	if (i < 0x80)
		return (signed char)i;
	i += 0x40;
	i &= 0xff;
#ifdef SIGH
fprintf(stderr, "(3)input: %i output: %i\n", c, i);
#endif
	if (i < 0x80)
		return (signed char)i;
	i ^= 0xa0;
#ifdef SIGH
fprintf(stderr, "(4)input: %i output: %i\n", c, i);
#endif
	return (signed char)i;
}

static signed char convert_char_high(signed char c) {
	return (c | 0x80);
}
	
typedef struct { 
	char *name;
	signed char (*func)(signed char);
} charset;

static charset charsets[] = {
	{ "ASCII", convert_char_ascii },
	{ "PETSCII", convert_char_petscii },
	{ "PETSCREEN", convert_char_petscreen },
	{ "HIGH", convert_char_high },
	{ NULL, NULL }
};

int set_charset(char *charset_name) {
	int i = 0;
	while (charsets[i].name != NULL) {
		if (strcmp(charsets[i].name, charset_name) == 0) {
			convert_func = charsets[i].func;
			return 0;
		}
		i++;
	}
	return -1;
}

signed char convert_char(signed char c) {
	return convert_func(c);
}


