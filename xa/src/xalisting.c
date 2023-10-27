/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 * maintained by Cameron Kaiser
 *
 * Assembler listing
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

/* enable this to turn on (copious) optimization output */
/* #define DEBUG_AM */
#undef LISTING_DEBUG

#include <stdio.h>
#include <string.h>

#include "xah.h"
#include "xal.h"
#include "xat.h"


/*********************************************************************************************/
/* this is the listing code
 *
 * Unfortunately this code has to go here (for now), as this file is the only one
 * where we have access to the tables that allow to convert the tokens back to 
 * a listing
 */

static FILE *listfp = NULL;
static int list_lineno = 1;		/* current line number */
static int list_last_lineno = 0;	/* current line number */
static char *list_filenamep = NULL;	/* current file name pointer */

static int list_numbytes = 8;

static int list_string(char *buf, char *string);
static int list_tokens(char *buf, signed char *input, int len);
static int list_value(char *buf, int val, signed char format);
static int list_nchar(char *buf, signed char c, int n);
static int list_char(char *buf, signed char c);
static int list_sp(char *buf);
static int list_word(char *buf, int outword);
static int list_word_f(char *buf, int outword, signed char format);
static int list_byte(char *buf, int outbyte);
static int list_byte_f(char *buf, int outbyte, signed char format);
static int list_nibble_f(char *buf, int outnib, signed char format);

/*********************************************************************************************/

// formatter
typedef struct {
	void	(*start_listing)(char *name);
	void	(*start_line)();
	int	(*set_anchor)(char *buf, char *name); 	// returns number of bytes added to buf
	int	(*start_label)(char *buf, char *name);	// returns number of bytes added to buf
	int	(*end_label)(char *buf);
	void	(*end_line)();
	void	(*end_listing)();
	char*	(*escape)(char *toescape);	// returns pointer to static buffer, valid until next call
	char*	(*escape_char)(char toescape);	// returns pointer to static buffer, valid until next call
} formatter_t;

static char *def_escape(char *toescape) {
	return toescape;
}

static char *def_escape_char(char toescape) {
	static char buf[2];
	buf[0] = toescape;
	buf[1] = 0;
	return buf;
}

static formatter_t def_format = { 
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,
	def_escape, def_escape_char
};

static void html_start_listing(char *name) {
	// really short version for now
	fprintf(listfp, "<html><head><title>%s</title></head><body><pre>\n", 
		(name == NULL) ? "(null)" : name);
}

static int html_set_anchor(char *buf, char *name) {
	sprintf(buf, "<a name=\"%s\"> </a>", name);
	return strlen(buf);
}

static int html_start_label(char *buf, char *name) {
	sprintf(buf, "<a href=\"#%s\">", name);
	return strlen(buf);
}

static int html_end_label(char *buf) {
	sprintf(buf, "</a>");
	return strlen(buf);
}

static void html_end_listing() {
	fprintf(listfp, "</pre></body></html>\n");
}

static char *html_escape(char *toescape) {
	static char buf[MAXLINE];
	
	char *p = toescape;
	char *q = buf;

	while (*p != 0) {
		if (*p == '<') {
			strcpy(q, "&lt;");
			q+=4;
			p++;
		} else
		if (*p == '&') {
			strcpy(q, "&amp;");
			q+=5;
			p++;
		} else {
			*q = *p;
			q++;
			p++;
		}
	}
	*q = 0;	// string terminator
	return buf;
}

static char *html_escape_char(char toescape) {
	static char buf[2];
	
	buf[0] = toescape;
	buf[1] = 0;

	return html_escape(buf);
}

static formatter_t html_format = {
	html_start_listing, 
	NULL, 
	html_set_anchor,
	html_start_label,
	html_end_label,
	NULL, 
	html_end_listing,
	html_escape, html_escape_char
};

static formatter_t *formatp = &def_format;

/*********************************************************************************************/

void list_flush() {
	if (listfp != NULL) {
		fflush(listfp);
	}
}

void list_start(const char *formatname) {
	formatp = &def_format;

	if (formatname != NULL && strcmp("html", formatname) == 0) {
		formatp = &html_format;
	}

	if (listfp != NULL) {
		if (formatp->start_listing != NULL) formatp->start_listing(list_filenamep);
	}
}

void list_end() {
	if (listfp != NULL) {
		if (formatp->end_listing != NULL) formatp->end_listing();
	}
}

// set number of bytes per line displayed as hex
void list_setbytes(int number_of_bytes_per_line) {
	list_numbytes = number_of_bytes_per_line;
}

/* set line number for the coming listing output */
void list_line(int l) {
	list_lineno = l;
}

/* set file name for the coming listing output */
void list_filename(char *fname) {
	if (list_filenamep == NULL || (fname != NULL && strcmp(fname, list_filenamep) != 0)) {
		list_filenamep = fname;
		list_lineno = 1;
		list_last_lineno = 0;

		/* Hack */
		if (listfp != NULL) {
			fprintf(listfp, "\n%s\n\n", fname);
		}
	}
}

/**
 * set the output file descriptor where to write the listing
 */
void list_setfile(FILE *fp) {
	listfp = fp;
}

char *list_preamble(char *buf, int lineno, int seg, int pc) {
	/* line number in file */
	snprintf(buf, 10, "% 5d", lineno);
	int i = strlen(buf);
	buf += i;
	buf += list_char(buf, ' ');

	char c = '?';
	/* preamble <segment>':'<address>' ' */
	switch(seg) {
	case SEG_ABS:	c='A'; break;
	case SEG_TEXT:	c='T'; break;
	case SEG_BSS:	c='B'; break;
	case SEG_DATA:	c='D'; break;
	case SEG_UNDEF:	c='U'; break;
	case SEG_ZERO:	c='Z'; break;
	}
	buf = buf + list_char(buf, c);
	buf = buf + list_char(buf, ':');
	buf = buf + list_word(buf, pc);
	buf = buf + list_nchar(buf, ' ', 2);

	return buf;
}


/**
 * listing/listing_len give the buffer address and length respectively that contains
 * 	the token as they are produced by the tokenizer. 
 * bincode/bincode_len give the buffer address and length that contain the binary code
 * 	that is produced from the token listing
 * 
 * Note that both lengths may be zero
 */
void do_listing(signed char *listing, int listing_len, signed char *bincode, int bincode_len) {
	
	int i, n_hexb;

	char outline[MAXLINE];
	char *buf = outline;

	int lst_seg = listing[0];
	int lst_pc = (listing[2]<<8) | (listing[1] & 255);

	/* no output file (not even stdout) */
	if (listfp == NULL) return;


	/*printf("do_listing: listing=%p (%d), bincode=%p (%d)\n", listing, listing_len, bincode, bincode_len);*/

	if (bincode_len < 0) bincode_len = -bincode_len;

	/* do we need a separation line? */
	if (list_lineno > list_last_lineno+1) {
		/* yes */
		/*fprintf(listfp, "line=%d, last=%d\n", list_lineno, list_last_lineno);*/
		fprintf(listfp, "\n");
	}
	list_last_lineno = list_lineno;

	// could be extended to include the preamble...
	if (formatp->start_line != NULL) formatp->start_line();

	buf = list_preamble(buf, list_lineno, lst_seg, lst_pc);

	// check if we have labels, so we can adjust the max printable number of 
	// bytes in the last line
	int num_last_line = 11;
	int tmp = listing[3] & 255;
	if (tmp == (T_DEFINE & 255)) {
		// we have label definition
		num_last_line = 8;
	}
	int overflow = 0;

	/* binary output (up to 8 byte. If more than 8 byte, print 7 plus "..." */
	n_hexb = bincode_len;
	if (list_numbytes != 0 && n_hexb >= list_numbytes) {
		n_hexb = list_numbytes-1;
		overflow = 1;
	}
	for (i = 0; i < n_hexb; i++) {
		buf = buf + list_byte(buf, bincode[i]);
		buf = buf + list_sp(buf);
		if ( (i%16) == 15) {
			// make a break
			buf[0] = 0;
			fprintf(listfp, "%s\n", outline);
			if (formatp->end_line != NULL) formatp->end_line();
			if (formatp->start_line != NULL) formatp->start_line();
			buf = outline;
			buf = list_preamble(buf, list_lineno, lst_seg, lst_pc + i + 1);
		}
	}
	if (overflow) {
		// are we at the last byte?
		if (n_hexb + 1 == bincode_len) {
			// just print the last byte
			buf = buf + list_byte(buf, bincode[i]);
			buf = buf + list_sp(buf);
		} else {
			// display "..."
			buf = buf + list_nchar(buf, '.', 3);
		}
		n_hexb++;
	}
	i = n_hexb % 16;
	if (i > num_last_line) {
		// make a break (Note: with original PC, as now the assembler text follows
		buf[0] = 0;
		fprintf(listfp, "%s\n", outline);
		if (formatp->end_line != NULL) formatp->end_line();
		if (formatp->start_line != NULL) formatp->start_line();
		buf = outline;
		buf = list_preamble(buf, list_lineno, lst_seg, lst_pc);
		i = 0;
	} 
	i = num_last_line - i;
	buf = buf + list_nchar(buf, ' ', i * 3);

	buf = buf + list_sp(buf);

	buf += list_tokens(buf, listing + 3, listing_len - 3);

#ifdef LISTING_DEBUG
	/* for now only do a hex dump so we see what actually happens */
	{
	    char valbuf[32];
	    i = buf - outline;
	    if (i<80) buf += list_nchar(buf, ' ', 80-i);
		
	    buf += list_string(buf, " >>");
	    sprintf(valbuf, "%p", listing+3);
	    buf += list_string(buf, valbuf);
	    buf += list_sp(buf);
	    for (i = 3; i < listing_len; i++) {
		buf = buf + list_byte(buf, listing[i]);
		buf = buf + list_sp(buf);
	    }
	}
#endif
	buf[0] = 0;
	
	fprintf(listfp, "%s\n", outline);

	if (formatp->end_line != NULL) formatp->end_line();
}

int list_tokens(char *buf, signed char *input, int len) {
	int outp = 0;
	int inp = 0;
	int tmp;
	char *name;
	signed char c;
	xalabel_t is_cll;
	int tabval;
	signed char format;

	if (inp >= len) return 0;

	tmp = input[inp] & 255;

	tabval = 0;
	if (tmp == (T_DEFINE & 255)) {
		while (inp < len && tmp == (T_DEFINE & 255)) {
			tmp = ((input[inp+2]&255)<<8) | (input[inp+1]&255);
/*printf("define: len=%d, inp=%d, tmp=%d\n", len, inp, tmp);*/
			name=l_get_name(tmp, &is_cll);

			// duplicate anchor names?
			if (formatp->set_anchor != NULL) {
				outp += formatp->set_anchor(buf+outp, l_get_unique_name(tmp));
			}

			if (is_cll == CHEAP) {
				outp += list_char(buf+outp, '@');
			} else
			if (is_cll == UNNAMED_DEF || is_cll == UNNAMED) {
				outp += list_char(buf+outp, ':');
			}
			if (is_cll != UNNAMED) {
				tmp = list_string(buf+outp, name);
				tabval += tmp + 1 + is_cll;
				outp += tmp;
			}
			outp += list_char(buf+outp, ' ');
			inp += 3;
			tmp = input[inp] & 255;
		}
		if (tabval < 10) {
			outp += list_nchar(buf+outp, ' ', 10-tabval);
		}
	} else {
		if (tmp >= 0 && tmp < number_of_valid_tokens) {
			outp += list_string(buf+outp, " ");
		}
	}

	if (tmp >= 0 && tmp < number_of_valid_tokens) {
		/* assembler keyword */
		/*printf("tmp=%d, kt[tmp]=%p\n", tmp, kt[tmp]);*/
		if (kt[tmp] != NULL) {
			outp += list_string(buf+outp, kt[tmp]);
		}
		outp += list_sp(buf + outp);
		inp += 1;
#if 0
		if (tmp == Kinclude) {
			/* just another exception from the rule... */
			/* next char is terminator (", ') then the length and then the file name */
			char term = input[inp];
			int len = input[inp+1] & 255;
			outp += list_char(buf+outp, term);
			for (tmp = 2; tmp < len+2; tmp++) {
				outp += list_char(buf+outp, input[inp+tmp]);
			}
			outp += list_char(buf+outp, term);
			inp += len + 2;
		}
#endif
	}

	while (inp < len) {
		int operator = 0;

		switch(input[inp]) {
		case T_CAST:
			outp += list_string(buf+outp, formatp->escape_char(input[inp+1]));
			inp+=2;
			break;
		case T_VALUE:
			/*outp += list_char(buf+outp, 'V');*/
			/* 24 bit value */
			tmp = ((input[inp+3]&255)<<16) | ((input[inp+2]&255)<<8) | (input[inp+1]&255);
			format = input[inp+4];
			outp += list_value(buf+outp, tmp, format);
			inp += 5;
			operator = 1;	/* check if arithmetic operator follows */
			break;
		case T_LABEL:
			/*outp += list_char(buf+outp, 'L');*/
			/* 16 bit label number */
			tmp = ((input[inp+2]&255)<<8) | (input[inp+1]&255);
			name=l_get_name(tmp, &is_cll);

			// duplicate label name
			if (formatp->start_label != NULL) {
				outp += formatp->start_label(buf+outp, l_get_unique_name(tmp));
			}
			if (is_cll == CHEAP) {
				outp += list_char(buf+outp, '@');
			} else
			if (is_cll == UNNAMED || is_cll == UNNAMED_DEF) {
				outp += list_char(buf+outp, ':');
			}
			if (is_cll != UNNAMED) {
				outp += list_string(buf+outp, name == NULL ? "<null>" : name);
			}

			if (formatp->end_label != NULL) outp += formatp->end_label(buf+outp);

			inp += 3;
			operator = 1;	/* check if arithmetic operator follows */
			break;
		case T_OP:
			/* label arithmetic operation; inp[3] is operation like '=' or '+' */
			tmp = ((input[inp+2]&255)<<8) | (input[inp+1]&255);
			name=l_get_name(tmp, &is_cll);
			if (input[inp+3] == '=') {
				// label definition
				if (formatp->set_anchor != NULL) {
					outp += formatp->set_anchor(buf+outp, l_get_unique_name(tmp));
				}
			}
			if (is_cll) outp += list_char(buf+outp, '@');
			outp += list_string(buf+outp, name);
			outp += list_char(buf+outp, input[inp+3]);
			inp += 4;
			
			break;
		case T_END:
			/* end of operation */
			/*outp += list_string(buf+outp, ";");*/
			inp += 1;
			goto end;
			break;
		case T_COMMENT:
			if (inp > 0 && inp < 20) {
				outp += list_nchar(buf+outp, ' ', 20-inp);
			}
			tmp = ((input[inp+2]&255)<<8) | (input[inp+1]&255);
			outp += list_char(buf+outp, ';');
			outp += list_string(buf+outp, (char*)input+inp+3);
			inp += tmp + 3;
			break;
		case T_LINE:
		case T_FILE:
			/* those two are meta-tokens, evaluated outside the t_p2 call,
 			 * they result in calls to list_line(), list_filename() */
			break;
		case T_POINTER:
			/* what is this? It's actually resolved during token conversion */
			tmp = ((input[inp+5]&255)<<8) | (input[inp+4]&255);
			name=l_get_name(tmp, &is_cll);
			if (formatp->start_label != NULL) {
				outp += formatp->start_label(buf+outp, l_get_unique_name(tmp));
			}
			if (is_cll) outp += list_char(buf+outp, '@');
			outp += list_string(buf+outp, name);

			if (formatp->end_label != NULL) outp += formatp->end_label(buf+outp);
			/*
			outp += list_byte(buf+outp, input[inp+1]);
			outp += list_char(buf+outp, '#');
			tmp = ((input[inp+3]&255)<<8) | (input[inp+2]&255);
			outp += list_value(buf+outp, tmp);
			*/
			inp += 6;
			operator = 1;	/* check if arithmetic operator follows */
			break;
		case '"': {
			int i, len;

			// string display
			inp++;
			outp += list_char(buf+outp, '"');
			len = input[inp] & 0xff;
			for (i = 0; i < len; i++) {
				inp++;
				outp += list_char(buf+outp, input[inp]);
			}
			inp++;
			outp += list_char(buf+outp, '"');
			break;
		}
		case '*': {
			// If '*' appears as operand, it is the PC. We need to switch to operator then
			inp++;
			outp += list_char(buf+outp, '*');
			operator = 1;
			break;
		}
		default:
			c = input[inp];
			if (c > 31) {
				outp += list_string(buf+outp, formatp->escape_char(input[inp]));
			} else {
				outp += list_char(buf+outp, '\'');
				outp += list_byte(buf+outp, input[inp]);
			}
			inp += 1;
			break;
		}

		if (operator && inp < len) {
			signed char op = input[inp];
			if (op > 0 && op <= 17) {
				outp += list_string(buf+outp, formatp->escape(arith_ops[op]));
				inp += 1;
			}
			operator = 0;
		}
	}
end:
	return outp;	
}

int list_string(char *buf, char *string) {
	if (buf == NULL || string == NULL) {
		fprintf(stderr, "NULL pointer: buf=%p, string=%p\n", buf, string);
		fflush(stderr);
		//exit(1);
		return 0;
	}

	int p = 0;
	while (string[p] != 0) {
		buf[p] = string[p];
		p++;
	}
	return p;
}

int list_value(char *buf, int val, signed char format) {
	int p = 0;
	char valbuf[32];

	switch (format) {
	case '$':
		p += list_char(buf + p, '$');
		if (val & (255<<16)) {
			p += list_byte(buf+p, val>>16);
			p += list_word(buf+p, val);
		} else
		if (val & (255<<8)) {
			p += list_word(buf+p, val);
		} else {
			p += list_byte(buf+p, val);
		}
		break;
	case '%':
		p += list_char(buf + p, '%');
		if (val & (255<<16)) {
			p += list_byte_f(buf+p, val>>16,'%');
			p += list_word_f(buf+p, val,'%');
		} else
		if (val & (255<<8)) {
			p += list_word_f(buf+p, val,'%');
		} else {
			p += list_byte_f(buf+p, val,'%');
		}
		break;
	case '&':
		snprintf(valbuf, 32, "%o",val);
		p+= list_char(buf+p, '&');
		p+= list_string(buf+p, valbuf);
		break;
	case 'd':
		snprintf(valbuf, 32, "%d",val);
		p+= list_string(buf+p, valbuf);
		break;
	case '\'':
	case '"':
		p+= list_char(buf+p, format);
		p+= list_char(buf+p, val);
		p+= list_char(buf+p, format);
		break;
	default:
		/* hex format as fallback */
		p += list_char(buf + p, '$');
		if (val & (255<<16)) {
			p += list_byte(buf+p, val>>16);
			p += list_word(buf+p, val);
		} else
		if (val & (255<<8)) {
			p += list_word(buf+p, val);
		} else {
			p += list_byte(buf+p, val);
		}
		break;
	}
	return p;
}

int list_nchar(char *buf, signed char c, int n) {
	int i;
	for (i = 0; i < n; i++) {
		buf[i]=c;
	}
	return n;
}

int list_char(char *buf, signed char c) {
	buf[0] = c;
	return 1;
}

int list_sp(char *buf) {
	buf[0] = ' ';
	return 1;
}

int list_word(char *buf, int outword) {
	return list_word_f(buf, outword, '$');
}

int list_word_f(char *buf, int outword, signed char format) {
	int p = 0;
	p+= list_byte_f(buf+p, outword >> 8, format);
	p+= list_byte_f(buf+p, outword, format);
	return p;
}

int list_byte(char *buf, int outbyte) {
	return list_byte_f(buf, outbyte, '$');
}

int list_byte_f(char *buf, int outbyte, signed char format) {
	int p = 0;
	p+= list_nibble_f(buf+p, (outbyte >> 4), format);
	p+= list_nibble_f(buf+p, outbyte, format);
	return p;
}

int list_nibble_f(char *buf, int outnib, signed char format) {
	int p = 0;
	outnib = outnib & 0xf;
	switch(format) {
	case '$':
		if (outnib < 10) {
			buf[p]='0'+outnib;
		} else {
			buf[p]='a'-10+outnib;
		}
		p++;
		break;
	case '%':
		buf[p++] = (outnib&8)?'1':'0';
		buf[p++] = (outnib&4)?'1':'0';
		buf[p++] = (outnib&2)?'1':'0';
		buf[p++] = (outnib&1)?'1':'0';
		break;
	default:
		/* hex as default */
		if (outnib < 10) {
			buf[p]='0'+outnib;
		} else {
			buf[p]='a'-10+outnib;
		}
		p++;
		break;
	}
	return p;
}



