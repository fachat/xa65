/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 Andre Fachat (afachat@gmx.de)
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
#ifndef __XA65_VERSION_H__
#define __XA65_VERSION_H__

void version(const char *programname, const char *version, const char *authors,
		const char *copyright) {
	fprintf(stdout, "%s (xa65) %s\n"
			"%s\n"
			"\n"
			"%s\n"
			"This is free software; see the source for "
			"copying conditions.  There is NO\n"
			"warranty; not even for MERCHANTABILIY or "
			"FITNESS FOR A PARTICULAR PURPOSE.\n", programname, version,
			authors, copyright);
}

#endif  /* __XA65_VERSION_H__ */
