/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 Andre Fachat (afachat@gmx.de)
 * maintained by Cameron Kaiser (ckaiser@floodgap.com)
 *
 * Main program
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

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

/* macros */
#include "xad.h"

/* structs and defs */
#include "xah.h"
#include "xah2.h"

/* exported functions are defined here */
#include "xa.h"
#include "xal.h"
#include "xam.h"
#include "xao.h"
#include "xap.h"
#include "xar.h"
#include "xat.h"
#include "xacharset.h"
#include "xalisting.h"

#include "version.h"

/* ANZERR: total number of errors */
/* ANZWARN: total number of warnings */

#define ANZERR		64
#define ANZWARN		13

#define programname	"xa"
/* progversion now in xa.h */
#define authors		"Written by Andre Fachat, Jolse Maginnis, David Weinehall and Cameron Kaiser"
#define copyright	"Copyright (C) 1989-2023 Andre Fachat, Jolse Maginnis, David Weinehall\nand Cameron Kaiser."

/* exported globals */
int ncmos, cmosfl, w65816, n65816;

/* compatibility flags */
int masm = 0; /* MASM */
int ca65 = 0; /* CA65 */
int xa23 = 0; /* ^ and recursive comments, disable \ escape */
int ctypes = 0; /* C compatibility, like "0xab" types */
int nolink = 0;
int romable = 0;
int romaddr = 0;
int noglob = 0;
int showblk = 0;
int crossref = 0;
int undefok = 0;// -R only accepts -Llabels; with -U all undef'd labels are ok in -R mode
char altppchar;

/* local variables */

static char out[MAXLINE];
static time_t tim1, tim2;
static FILE *fpout, *fperr, *fplab, *fplist;
static int ner = 0;
static int ner_max = 20;

static int align = 1;

static void printstat(void);
static void usage(int, FILE*);
static int setfext(char*, char*);
static int x_init(void);
static int pass1(void);
static int pass2(void);
static int puttmp(int);
static int puttmpw(int);
static int puttmps(signed char*, int);
static void chrput(int);
static int xa_getline(char*);
static void lineout(void);
static long ga_p1(void);
static long gm_p1(void);
static int set_compat(char *compat_name);

/* text */
int memode, xmode;
int segment;
int tlen = 0, tbase = 0x1000;
int dlen = 0, dbase = 0x0400;
int blen = 0, bbase = 0x4000;
int zlen = 0, zbase = 4;
int fmode = 0;
int relmode = 0;

int pc[SEG_MAX]; /* segments */

int main(int argc, char *argv[]) {
	int er = 1, i;
	signed char *s = NULL;
	char *tmpp;

	char *listformat = NULL;

	int mifiles = 5;
	int nifiles = 0;
	int verbose = 0;
	int no_link = 0;

	char **ifiles;
	char *printfile; /* print listing to this file */
	char *ofile; /* output file */
	char *efile; /* error listing goes there */
	char *lfile; /* labels go here */
	char *ifile;

	char old_e[MAXLINE];
	char old_l[MAXLINE];
	char old_o[MAXLINE];

	tim1 = time(NULL);

	// note: unfortunately we do no full distinction between 65C02 and 65816.
	// The conflict is in the column 7 and column f opcodes, where the 65C02
	// has the BBR/BBS/SMB/RMB opcodes, but the 65816 has its own.
	// Also, we potentially could support the 65SC02, which is the 65C02, but
	// without the conflicting BBR/BBS/SMB/RMB opcodes.
	// This, however, is a TODO for a later version.
	cmosfl = 1;
	//fmode = FM_CPU2_65C02;
	w65816 = 0; /* default: 6502 only */

	ncmos = 0;	// counter for CMOS opcodes used
	n65816 = 0;	// counter for 65816-specific opcodes used

	altppchar = '#'; /* i.e., NO alternate char */

	if ((tmpp = strrchr(argv[0], '/'))) {
		tmpp++;
	} else {
		tmpp = argv[0];
	}
	if ((!strcmp(tmpp, "xa65816")) || (!strcmp(tmpp, "XA65816"))
			|| (!strcmp(tmpp, "xa816")) || (!strcmp(tmpp, "XA816"))) {
		w65816 = 1; /* allow 65816 per default */
	}

	/* default output charset for strings in quotes */
	set_charset("ASCII");

	ifiles = malloc(mifiles * sizeof(char*));

	afile = alloc_file();

	if (argc <= 1) {
		usage(w65816, stderr);
		exit(1);
	}

	if (strstr(argv[1], "--help") || strstr(argv[1], "-?")) {
		usage(w65816, stdout);
		exit(0);
	}

	if (strstr(argv[1], "--version")) {
		version(programname, progversion, authors, copyright);
		exit(0);
	}

	ofile = "a.o65";
	efile = NULL;
	lfile = NULL;
	printfile = NULL;

	if (pp_init()) {
		logout("fatal: pp: no memory!");
		return 1;
	}
	if (b_init()) {
		logout("fatal: b: no memory!");
		return 1;
	}
	if (l_init()) {
		logout("fatal: l: no memory!");
		return 1;
	}

	i = 1;
	while (i < argc) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'E':
				ner_max = 0;
				break;
			case 'p':
				/* intentionally not allowing an argument to follow with a
				 space to avoid - being seen as the alternate
				 preprocessor char! */
				if (argv[i][2] == '\0') {
					fprintf(stderr, "-p requires a character argument\n");
					exit(1);
				}
				if (argv[i][2] == '#')
					fprintf(stderr,
							"using -p# is not necessary, '#' is the default\n");
				altppchar = argv[i][2];
				if (argv[i][3] != '\0')
					fprintf(stderr,
							"warning: extra characters to -p ignored\n");
				break;
			case 'M':
				fprintf(stderr,
						"Warning: -M is deprecated (use -XMASM) and will be removed in a future version\n");
				masm = 1; /* MASM compatibility mode */
				break;
			case 'X': /* compatibility across assemblers... */
			{
				char *name = NULL;
				if (argv[i][2] == 0) {
					name = argv[++i];
				} else {
					name = argv[i] + 2;
				}
				if (set_compat(name) < 0) {
					fprintf(stderr,
							"Compatibility set '%s' unknown - ignoring! (check case?)\n",
							name);
				}
			}
				break;
			case 'O': /* output charset */
			{
				char *name = NULL;
				if (argv[i][2] == 0) {
					if (i + 1 < argc)
						name = argv[++i];
					else {
						fprintf(stderr, "-O requires an argument\n");
						exit(1);
					}
				} else {
					name = argv[i] + 2;
				}
				if (set_charset(name) < 0) {
					fprintf(stderr,
							"Output charset name '%s' unknown - ignoring! (check case?)\n",
							name);
				}
			}
				break;
			case 'A': /* make text segment start so that text relocation
			 is not necessary when _file_ starts at adr */
				romable = 2;
				if (argv[i][2] == 0) {
					if (i + 1 < argc)
						romaddr = atoi(argv[++i]);
					else {
						fprintf(stderr, "-A requires an argument\n");
						exit(1);
					}
				} else
					romaddr = atoi(argv[i] + 2);
				break;
			case 'G':
				noglob = 1;
				break;
			case 'L': /* define global label */
				if (argv[i][2])
					lg_set(argv[i] + 2);
				break;
			case 'r':
				crossref = 1;
				break;
			case 'R':
				relmode = 1;
				break;
			case 'U':
				undefok = 1;
				break;
			case 'D':
				s = (signed char*) strstr(argv[i] + 2, "=");
				if (s)
					*s = ' ';
				pp_define(argv[i] + 2);
				break;
			case 'c':
				no_link = 1;
				fmode |= FM_OBJ;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'C':
				cmosfl = 0;
				fmode &= ~FM_CPU2;	// fall back to standard 6502
				// breaks existing tests (compare with pre-assembled files)
				//if (w65816) {
				//	fmode |= FM_CPU2_65816E;
				//}
				break;
			case 'W':
				w65816 = 0;
				fmode &= ~FM_CPU;
				fmode &= ~FM_CPU2;
				// breaks existing tests (compare with pre-assembled files)
				//if (cmosfl) {
				//	fmode |= FM_CPU2_65C02;
				//}
				break;
			case 'w':
				// note: we do not disable cmos here, as opcode tables note CMOS for
				// opcodes common to both, CMOS and 65816 as well.
				w65816 = 1;
				fmode &= ~FM_CPU2;
				// breaks existing tests (compare with pre-assembled files)
				//fmode |= FM_CPU;	// 65816 bit
				//fmode |= FM_CPU2_65816E;// 6502 in 65816 emu, to manage opcode compatibility in ldo65
				break;
			case 'B':
				showblk = 1;
				break;
			case 'I':
				if (argv[i][2] == 0) {
					if (i + 1 < argc)
						reg_include(argv[++i]);
					else {
						fprintf(stderr, "-I requires an argument\n");
						exit(1);
					}
				} else {
					reg_include(argv[i] + 2);
				}
				break;
			case 'P':
				if (argv[i][2] == 0) {
					printfile = argv[++i];
				} else {
					printfile = argv[i] + 2;
				}
				break;
			case 'F':
				if (argv[i][2] == 0) {
					listformat = argv[++i];
				} else {
					listformat = argv[i] + 2;
				}
				break;
			case 'o':
				if (argv[i][2] == 0) {
					if (i + 1 < argc)
						ofile = argv[++i];
					else {
						fprintf(stderr, "-o requires an argument\n");
						exit(1);
					}
				} else {
					ofile = argv[i] + 2;
				}
				break;
			case 'l':
				if (argv[i][2] == 0) {
					if (i + 1 < argc)
						lfile = argv[++i];
					else {
						fprintf(stderr, "-l requires an argument\n");
						exit(1);
					}
				} else {
					lfile = argv[i] + 2;
				}
				break;
			case 'e':
				if (argv[i][2] == 0) {
					if (i + 1 < argc)
						efile = argv[++i];
					else {
						fprintf(stderr, "-e requires an argument\n");
						exit(1);
					}
				} else {
					efile = argv[i] + 2;
				}
				break;
			case 'b': /* set segment base addresses */
				switch (argv[i][2]) {
				case 't':
					if (argv[i][3] == 0)
						tbase = atoi(argv[++i]);
					else
						tbase = atoi(argv[i] + 3);
					break;
				case 'd':
					if (argv[i][3] == 0)
						dbase = atoi(argv[++i]);
					else
						dbase = atoi(argv[i] + 3);
					break;
				case 'b':
					if (argv[i][3] == 0)
						bbase = atoi(argv[++i]);
					else
						bbase = atoi(argv[i] + 3);
					break;
				case 'z':
					if (argv[i][3] == 0)
						zbase = atoi(argv[++i]);
					else
						zbase = atoi(argv[i] + 3);
					break;
				default:
					fprintf(stderr, "unknown segment type '%c' - ignoring!\n",
							argv[i][2]);
					break;
				}
				break;
			case 0:
				fprintf(stderr,
						"Single dash '-' on command line - ignoring!\n");
				break;
			default:
				fprintf(stderr, "Unknown option '%c' - ignoring!\n",
						argv[i][1]);
				break;
			}
		} else { /* no option -> filename */
			ifiles[nifiles++] = argv[i];
			if (nifiles >= mifiles) {
				mifiles += 5;
				ifiles = realloc(ifiles, mifiles * sizeof(char*));
				if (!ifiles) {
					fprintf(stderr,
							"Oops: couldn't alloc enough mem for filelist table..!\n");
					exit(1);
				}
			}
		}
		i++;
	}
	if (!nifiles) {
		fprintf(stderr, "No input files given!\n");
		exit(0);
	}

	if (verbose)
		fprintf(stderr, "%s\n", copyright);

	if (printfile != NULL && !strcmp(printfile, "-")) {
		printfile = NULL;
		fplist = stdout;
	} else {
		fplist = printfile ? xfopen(printfile, "w") : NULL;
	}
	fplab = lfile ? xfopen(lfile, "w") : NULL;
	fperr = efile ? xfopen(efile, "w") : NULL;
	if (!strcmp(ofile, "-")) {
		ofile = NULL;
		fpout = stdout;
	} else {
		fpout = xfopen(ofile, "wb");
	}
	if (!fpout) {
		fprintf(stderr, "Couldn't open output file!\n");
		exit(1);
	}

	if (verbose)
		fprintf(stderr, "%s\n", copyright);

	if (1 /*!m_init()*/) {
		if (1 /*!b_init()*/) {
			if (1 /*!l_init()*/) {
				/*if(!pp_init())*/
				{
					if (!x_init()) {
						/* if(fperr) fprintf(fperr,"%s\n",copyright); */
						if (verbose)
							logout(ctime(&tim1));

						list_setfile(fplist);

						/* Pass 1 */

						pc[SEG_ABS] = 0; /* abs addressing */
						seg_start(fmode, tbase, dbase, bbase, zbase, 0,
								relmode);

						if (relmode) {
							r_mode(RMODE_RELOC);
							segment = SEG_TEXT;
						} else {
							/* prime old_segment in r_mode with SEG_TEXT */
							segment = SEG_ABS;
							r_mode(RMODE_ABS);
						}

						nolink = no_link;

						for (i = 0; i < nifiles; i++) {
							ifile = ifiles[i];

							sprintf(out, "xAss65: Pass 1: %s\n", ifile);
							if (verbose)
								logout(out);

							er = pp_open(ifile);
							puttmpw(0);
							puttmp(T_FILE);
							puttmp(0);
							puttmp(0);
							puttmps((signed char*) &ifile,
									sizeof(filep->fname));

							if (!er) {
								er = pass1();
								pp_close();
							} else {
								sprintf(out,
										"Couldn't open source file '%s'!\n",
										ifile);
								logout(out);
							}
						}

						if ((er = b_depth())) {
							sprintf(out,
									"Still %d blocks open at end of file!\n",
									er);
							logout(out);
						}

						if (tbase & (align - 1)) {
							sprintf(out,
									"Warning: text segment ($%04x) start address doesn't align to %d!\n",
									tbase, align);
							logout(out);
						}
						if (dbase & (align - 1)) {
							sprintf(out,
									"Warning: data segment ($%04x) start address doesn't align to %d!\n",
									dbase, align);
							logout(out);
						}
						if (bbase & (align - 1)) {
							sprintf(out,
									"Warning: bss segment ($%04x) start address doesn't align to %d!\n",
									bbase, align);
							logout(out);
						}
						if (n65816 > 0)
							fmode |= 0x8000;
						switch (align) {
						case 1:
							break;
						case 2:
							fmode |= 1;
							break;
						case 4:
							fmode |= 2;
							break;
						case 256:
							fmode |= 3;
							break;
						}

						if ((!er) && relmode)
							h_write(fpout, fmode, tlen, dlen, blen, zlen, 0);

						if (!er) {
							if (verbose)
								logout("xAss65: Pass 2:\n");

							list_start(listformat);

							seg_pass2();

							if (relmode) {
								r_mode(RMODE_RELOC);
								segment = SEG_TEXT;
							} else {
								/* prime old_segment in r_mode with SEG_TEXT */
								segment = SEG_ABS;
								r_mode(RMODE_ABS);
							}
							er = pass2();

							list_end();
						}

						if (fplab)
							printllist(fplab);
						tim2 = time(NULL);
						if (verbose)
							printstat();

						if ((!er) && relmode)
							seg_end(fpout); /* write reloc/label info */

						if (fplist && fplist != stdout)
							fclose(fplist);
						if (fperr)
							fclose(fperr);
						if (fplab)
							fclose(fplab);
						if (fpout && fpout != stdout)
							fclose(fpout);

					} else {
						logout("fatal: x: no memory!\n");
					}
					pp_end();
					/*           } else {
					 logout("fatal: pp: no memory!");*/
				}
			} else {
				logout("fatal: l: no memory!\n");
			}
		} else {
			logout("fatal: b: no memory!\n");
		}
		/*m_exit();*/
	} else {
		logout("Not enough memory available!\n");
	}

	if (ner || er) {
		if (ner_max > 0) {
			fprintf(stderr, "Break after %d error%c\n", ner, ner ? 's' : 0);
		} else {
			/* ner_max==0, i.e. show all errors */
			fprintf(stderr, "End after %d error%c\n", ner, ner ? 's' : 0);
		}
		/*unlink();*/
		if (ofile) {
			unlink(ofile);
		}
	}

	free(ifiles);

	return ((er || ner) ? 1 : 0);
}

static void printstat(void) {
	logout("Statistics:\n");
	sprintf(out, " %8d of %8d label used\n", ga_lab(), gm_lab());
	logout(out);
	sprintf(out, " %8ld of %8ld byte label-memory used\n", ga_labm(),
			gm_labm());
	logout(out);
	sprintf(out, " %8d of %8d PP-defs used\n", ga_pp(), gm_pp());
	logout(out);
	sprintf(out, " %8ld of %8ld byte PP-memory used\n", ga_ppm(), gm_ppm());
	logout(out);
	sprintf(out, " %8ld of %8ld byte buffer memory used\n", ga_p1(), gm_p1());
	logout(out);
	sprintf(out, " %8d blocks used\n", ga_blk());
	logout(out);
	sprintf(out, " %8ld seconds used\n", (long) difftime(tim2, tim1));
	logout(out);
}

int h_length(void) {
	return 26 + o_length();
}

static int setfext(char *s, char *ext) {
	int j, i = (int) strlen(s);

	if (i > MAXLINE - 5)
		return (-1);

	for (j = i - 1; j >= 0; j--) {
		if (s[j] == DIRCHAR) {
			strcpy(s + i, ext);
			break;
		}
		if (s[j] == '.') {
			strcpy(s + j, ext);
			break;
		}
	}
	if (!j)
		strcpy(s + i, ext);

	return (0);
}

static long ga_p1(void) {
	return (afile->mn.tmpz);
}
static long gm_p1(void) {
	return (TMPMEM);
}

static int pass2(void) {
	int c, er, l, ll, i, al;
	Datei datei;
	signed char *dataseg = NULL;
	signed char *datap = NULL;

	memode = 0;
	xmode = 0;
	if ((dataseg = malloc(dlen))) {
		if (!dataseg) {
			fprintf(stderr, "Couldn't alloc dataseg memory...\n");
			exit(1);
		}
		datap = dataseg;
	}
	filep = &datei;
	afile->mn.tmpe = 0L;

	while ((ner_max == 0 || ner < ner_max) && afile->mn.tmpe < afile->mn.tmpz) {
		// get the length of the entry (now two byte - need to handle the sign)
		l = 255 & afile->mn.tmp[afile->mn.tmpe++];
		l |= afile->mn.tmp[afile->mn.tmpe++] << 8;
		ll = l;

		//printf("%p: l=%d first=%02x\n", afile->mn.tmp+afile->mn.tmpe-1, l, 0xff & afile->mn.tmp[afile->mn.tmpe]);

		if (!l) {
			if (afile->mn.tmp[afile->mn.tmpe] == T_LINE) {
				datei.fline = (afile->mn.tmp[afile->mn.tmpe + 1] & 255)
						+ (afile->mn.tmp[afile->mn.tmpe + 2] << 8);
				afile->mn.tmpe += 3;
				list_line(datei.fline); /* set line number of next listing output */
			} else if (afile->mn.tmp[afile->mn.tmpe] == T_FILE) {
				// copy the current line number from the current file descriptor
				datei.fline = (afile->mn.tmp[afile->mn.tmpe + 1] & 255)
						+ (afile->mn.tmp[afile->mn.tmpe + 2] << 8);
				// copy the pointer to the file name in the current file descriptor
				// Note: the filename in the current file descriptor is separately malloc'd and
				// thus save to store the pointer
				memcpy(&datei.fname, afile->mn.tmp + afile->mn.tmpe + 3,
						sizeof(datei.fname));
				afile->mn.tmpe += 3 + sizeof(datei.fname);

				list_filename(datei.fname); /* set file name of next listing output */
			}
		} else {
			/* do not attempt address mode optimization on pass 2 */

			/* t_p2_l() includes the listing call to do_listing() */
			er = t_p2_l(afile->mn.tmp + afile->mn.tmpe, &ll, &al);
			if (er == E_NOLINE) {
			} else if (er == E_OK) {
				if (segment < SEG_DATA) {
					for (i = 0; i < ll; i++)
						chrput(afile->mn.tmp[afile->mn.tmpe + i]);
				} else if (segment == SEG_DATA && datap) {
					memcpy(datap, afile->mn.tmp + afile->mn.tmpe, ll);
					datap += ll;
				}
			} else if (er == E_DSB) {
				c = afile->mn.tmp[afile->mn.tmpe];
				if (segment < SEG_DATA) {
					/*printf("E_DSB, ll=%d, l=%d, c=%c\n",ll,l,afile->mn.tmp[afile->mn.tmpe]);*/
					for (i = 0; i < ll; i++)
						chrput(c);
				} else if (segment == SEG_DATA && datap) {
					memset(datap, c, ll);
					datap += ll;
				}
			} else if (er == E_BIN) {
				int i;
				int j;
				int flen;
				int offset;
				int fstart;
				FILE *foo;
				char binfnam[256];

				i = afile->mn.tmpe;
				/*
				 fprintf(stderr, "ok, ready to insert\n");
				 for (i=0; i<ll; i++) {
				 fprintf(stderr, "%i: %02x\n", i, afile->mn.tmp[afile->mn.tmpe+i]);
				 }
				 */

				offset = afile->mn.tmp[i] + (afile->mn.tmp[i + 1] << 8)
						+ (afile->mn.tmp[i + 2] << 16);
				fstart = afile->mn.tmp[i + 3] + 1 + (afile->mn.tmp[i + 4] << 8);
				/* usually redundant but here for single-char names
				 that get interpreted as chars */
				flen = afile->mn.tmp[i + 5];
				if (flen > 1)
					fstart++;
				/* now fstart points either to string past quote and
				 length mark, OR, single char byte */
				/*
				 fprintf(stderr, "offset = %i length = %i fstart = %i flen = %i charo = %c\n",
				 offset, ll, fstart, flen, afile->mn.tmp[afile->mn.tmpe+fstart]);
				 */
				/* there is a race condition here where altering the
				 file between validation in t_p2 (xat.c) and
				 here will cause problems. I'm not going to
				 worry about this right now. */

				for (j = 0; j < flen; j++) {
					binfnam[j] = afile->mn.tmp[i + fstart + j];
				}
				binfnam[flen] = '\0';
				/*
				 fprintf(stderr, "fnam = %s\n", binfnam);
				 */
				/* primitive insurance */
				if (!(foo = fopen(binfnam, "rb"))) {
					errout(E_FNF);
					ner++;
				} else {
					fseek(foo, offset, SEEK_SET);
					for (j = 0; j < ll; j++) {
						/* damn you Andre ;-) */
						i = fgetc(foo);
						if (segment < SEG_DATA) {
							chrput(i);
						}
						if (segment == SEG_DATA && datap) {
							memset(datap++, i, 1);
						}
					}
					fclose(foo);
				}
			} else {
				errout(er);
			}
		}
		afile->mn.tmpe += abs(l);
	}
	if (relmode) {
		if ((ll = fwrite(dataseg, 1, dlen, fpout)) < dlen) {
			fprintf(stderr, "Problems writing %d bytes, return gives %d\n",
					dlen, ll);
		}
	}

	return (ner);
}

static int pass1(void) {
	signed char o[2 * MAXLINE]; /* doubled for token listing */
	int l, er, al;

	memode = 0;
	xmode = 0;
	tlen = 0;
	ner = 0;

	/*FIXIT*/
	while (!(er = xa_getline(s))) {
		er = t_p1((signed char*) s, o, &l, &al);
		switch (segment) {
		case SEG_ABS:
		case SEG_TEXT:
			tlen += al;
			break;
		case SEG_DATA:
			dlen += al;
			break;
		case SEG_BSS:
			blen += al;
			break;
		case SEG_ZERO:
			zlen += al;
			break;
		}

		//printf(": er= %d, l=%d\n",er,l);

		if (l) {
			if (er) {
				if (er == E_OKDEF) {
					if (!(er = puttmpw(l)))
						er = puttmps(o, l);
				} else if (er == E_NOLINE)
					er = E_OK;
			} else {
				if (!(er = puttmpw(-l)))
					er = puttmps(o, l);
			}
		}
		if (er) {
			lineout();
			errout(er);
		}

		/*          printf("tmpz =%d\n",afile->mn.tmpz);
		 */
	}

	if (er != E_EOF) {
		errout(er);
	}

	/*     { int i; printf("Pass 1 \n");
	 for(i=0;i<afile->mn.tmpz;i++)
	 fprintf(stderr, " %02x",255 & afile->mn.tmp[i]);
	 getchar();}
	 */
	return (ner);
}

static void usage(int default816, FILE *fp) {
	fprintf(fp, "Usage: %s [options] file\n"
			"Cross-assembler for 65xx/R65C02/65816\n"
			"\n",
	programname);
	fprintf(fp, " -v           verbose output\n"
			" -E           do not break after 20 errors, but show all\n"
			" -C           no CMOS-opcodes\n"
			" -W           no 65816-opcodes%s\n"
			" -w           allow 65816-opcodes%s\n",
			default816 ? "" : " (default)", default816 ? " (default)" : "");
	fprintf(fp,
			" -B           show lines with block open/close\n"
					" -c           produce `o65' object instead of executable files (i.e. don't link)\n"
					" -o filename  sets output filename, default is `a.o65'\n"
					"                A filename of `-' sets stdout as output file\n");
	fprintf(fp,
			" -e filename  sets errorlog filename, default is none\n"
					" -l filename  sets labellist filename, default is none\n"
					" -P filename  sets filename for listing, default is none, '-' is stdout\n"
					" -F format    sets format for listing, default is plain, 'html' is current only other\n"
					"              supported format\n"
					" -r           adds crossreference list to labellist (if `-l' given)\n"
					" -M           allow ``:'' to appear in comments for MASM compatibility\n"
					"              (deprecated: prefer -XMASM)\n"
					" -Xcompatset  set compatibility flags for other assemblers, known values are:\n"
					"              C, MASM, CA65, XA23 (deprecated: for better 2.3 compatibility)\n"
					" -R           start assembler in relocating mode\n"
					" -U           allow all undefined labels in relocating mode\n");
	fprintf(fp,
			" -Llabel      defines `label' as absolute, undefined label even when linking\n"
					" -p<c>        replace preprocessor char '#' with custom, e.g. '-p!' replaces it with '!'\n"
					" -b? addr     set segment base address to integer value addr\n"
					"                `?' stands for t(ext), d(ata), b(ss) and z(ero) segment\n"
					"                (address can be given more than once, last one is used)\n");
	fprintf(fp,
			" -A addr      make text segment start at an address that when the _file_\n"
					"                starts at addr, relocation is not necessary. Overrides -bt\n"
					"                Other segments must be specified with `-b?'\n"
					" -G           suppress list of exported globals\n");
	fprintf(fp,
			" -p?          set preprocessor character to ?, default is #\n"
					" -DDEF=TEXT   defines a preprocessor replacement\n"
					" -Ocharset    set output charset (PETSCII, ASCII, etc.), case-sensitive\n"
					" -Idir        add directory `dir' to include path (before XAINPUT)\n"
					"  --version   output version information and exit\n"
					"  --help      display this help and exit\n");
}

static char *ertxt[] = { "Syntax",			// E_SYNTAX	=-1
		"Label already defined",	// E_LABDEF	=-2
		"Label not defined",		// E_NODEF	=-3
		"Label table full",		// E_LABFULL	=-4
		"Label expected",		// E_LABEXP	=-5
		"Out of memory",		// E_NOMEM	=-6
		"Illegal opcode",		// E_ILLCODE	=-7
		"Wrong addressing mode",	// E_ADRESS	=-8
		"Branch out of range",		// E_RANGE	=-9
		"Overflow",			// E_OVERFLOW	=-10
		"Division by zero",		// E_DIV	=-11
		"Pseudo-opcode expected",	// E_PSOEXP	=-12
		"Block stack overflow",		// E_BLKOVR	=-13
		"File not found",		// E_FNF	=-14
		"End of file",			// E_EOF	=-15
		"Unmatched block close",	// E_BLOCK	=-16
		"NoBlk",			// E_NOBLK	=-17
		"NoKey",			// E_NOKEY	=-18
		"NoLine",			// E_NOLINE	=-19
		"OKDef",			// E_OKDEF	=-20
		"DSB",				// E_DSB	=-21
		"NewLine",			// E_NEWLINE	=-22
		"NewFile",			// E_NEWFILE	=-23
		"CMOS instruction used with -C", 	// E_DMOS	=-24
		"pp:Wrong parameter count",		// E_ANZPAR	=-25
		"Illegal pointer arithmetic (-26)", 	// E_ILLPOINTER	=-26
		"Illegal segment",			// E_ILLSEGMENT	=-27
		"File header option too long",		// E_OPTLEN	=-28
		"File option not at file start (when ROM-able)", // E_ROMOPT =-29
		"Illegal align value",			// E_ILLALIGN	=-30
		"65816 mode used/required",		// E_65816	=-31
		"Exceeded recursion limit for label evaluation", // E_ORECMAC =-32
		"Unresolved preprocessor directive at end of file", // E_OPENPP =-33
		"Data underflow",		// E_OUTOFDATA	=-34
		"Illegal quantity",		// E_ILLQUANT	=-35
		".bin",				// E_BIN	=-36
		"#error directive",		// E_UERROR	=-37
		"Assertion",		// E_AERROR	=-38
		"DSB has negative length",	// E_NEGDSBLEN	=-39
		/* placeholders for future fatal errors */
		"",			// -40
		"",			// -41
		"",			// -42
		"",			// -43
		"",			// -44
		"",			// -45
		"",			// -46
		"",			// -47
		"",			// -48
		"",			// -49
		"",			// -50
		"",			// -51
		"",			// -52
		"",			// -53
		"",			// -54
		"",			// -55
		"",			// -56
		"",			// -57
		"",			// -58
		"",			// -59
		"",			// -60
		"",			// -61
		"",			// -62
		"",			// -63
		"",			// -64 (was missing...)
		/* warnings */
		"Cutting word relocation in byte value",	// W_ADRRELOC	=-65
		"Byte relocation in word value",		// W_BYTERELOC	=-66
		"Illegal pointer arithmetic (-66)",		// E_WPOINTER	=-67
		"Address access to low or high byte pointer",	// W_ADDRACC	=-68
		"High byte access to low byte pointer",	// W_HIGHACC	=-69
		"Low byte access to high byte pointer",	// W_LOWACC	=-70
		"Can't optimize forward-defined label; using absolute addressing", // W_FORLAB =-71
		"Open preprocessor directive at end of file (intentional?)", // W_OPENPP =-72
		"Included binary data exceeds 64KB",		// W_OVER64K	=-73
		"Included binary data exceeds 16MB",		// W_OVER16M	=-74
		"Subtracting pointer from constant not supported in -R mode", // W_SUBTRACT =-75
		/* more placeholders */
		"",			// -76
		"",			// -77

		};

static int gl;
static int gf;

static int x_init(void) {
	return 0;
#if 0
     int er=0;
     /*er=m_alloc(TMPMEM,&tmp);*/
     afile->mn.tmp=malloc(TMPMEM);
     if(!afile->mn.tmp) er=E_NOMEM;
     afile->mn.tmpz=0L;
     return(er);
#endif
}

static int puttmp(int c) {
	int er = E_NOMEM;

	//printf("puttmp: %02x -> %p \n",0xff & c, afile->mn.tmp+afile->mn.tmpz);

	if (afile->mn.tmpz < TMPMEM) {
		afile->mn.tmp[afile->mn.tmpz++] = c;
		er = E_OK;
	}
	return (er);
}

static int puttmpw(int c) {
	int er = E_NOMEM;

	//printf("puttmp: %02x -> %p \n",0xff & c, afile->mn.tmp+afile->mn.tmpz);

	if (afile->mn.tmpz < TMPMEM - 1) {
		afile->mn.tmp[afile->mn.tmpz++] = c & 0xff;
		afile->mn.tmp[afile->mn.tmpz++] = (c >> 8) & 0xff;
		er = E_OK;
	}
	return (er);
}

static int puttmps(signed char *s, int l) {
	int i = 0, er = E_NOMEM;

	// printf("puttmps %d bytes from %p to %p:", l, s, afile->mn.tmp+afile->mn.tmpz);

	if (afile->mn.tmpz + l < TMPMEM) {
		while (i < l) {
			//printf(" %02x", 0xff & s[i]);
			afile->mn.tmp[afile->mn.tmpz++] = s[i++];
		}

		er = E_OK;
	}
	// printf("\n");
	return (er);
}

static char l[MAXLINE];

static int xa_getline(char *s) {
	static int ec;

	static int i, c;
	int hkfl, j, comcom;

	j = hkfl = comcom = 0;
	ec = E_OK;

	if (!gl) {
		do {
			ec = pgetline(l);
			i = 0;
			while (l[i] == ' ')
				i++;
			while (l[i] != '\0' && isdigit(l[i]))
				i++;
			gf = 1;

			if (ec == E_NEWLINE) {
				puttmpw(0);
				puttmp(T_LINE);
				puttmpw(filep->fline);
				ec = E_OK;

			} else if (ec == E_NEWFILE) {
				puttmpw(0);
				puttmp(T_FILE);
				puttmpw(filep->fline);
				puttmps((signed char*) &(filep->fname), sizeof(filep->fname));
				ec = E_OK;
			}
		} while (!ec && l[i] == '\0');
	}

	gl = 0;
	if (!ec || ec == E_EOF) {
		int startofline = 1;
		do {
			c = s[j] = l[i++];

			if (!(hkfl & 2) && c == '\"')
				hkfl ^= 1;
			if (!comcom && !(hkfl & 1) && c == '\'')
				hkfl ^= 2;
			if (c == ';' && !hkfl) {
				comcom = 1;
			}
			if (c == '\0') {
				// end of line
				break; /* hkfl = comcom = 0 */
			}
			if (c == ':' && !hkfl) {
				/* if the next char is a "=" - so that we have a ":=" - and we
				 we have ca65 compatibility, we ignore the colon */
				// also check for ":+" and ":-"
				if (((!startofline) && l[i] != '=' && l[i] != '+' && l[i] != '-')
						|| !ca65 || comcom) {
					/* but otherwise we check if it is in a comment and we have
					 MASM or CA65 compatibility, then we ignore the colon as well */
					if (!comcom || !(masm || ca65)) {
						/* we found a colon, so we keep the current line in memory
						 but return the part before the colon, and next time the part
						 after the colon, so we can parse C64 BASIC text assembler... */
						gl = 1;
						break;
					}
				}
			}
			if (!isspace(c)) {
				startofline = 0;
			}
			j++;
		} while (c != '\0' && j < MAXLINE - 1 && i < MAXLINE - 1);

		s[j] = '\0';
	} else
		s[0] = '\0';
#if 0
	printf("got line: %s\n", s);
#endif
	return (ec);
}

void set_align(int a) {
	align = (a > align) ? a : align;
}

static void lineout(void) {
	if (gf) {
		logout(filep->flinep);
		logout("\n");
		gf = 0;
	}
}

void errout(int er) {
	if (er <= -ANZERR || er > -1) {
		if (er >= -(ANZERR + ANZWARN) && er <= -ANZERR) {
			sprintf(out, "%s:line %d: %04x: Warning - %s\n", filep->fname,
					filep->fline, pc[segment], ertxt[(-er) - 1]);
		} else {
			/* sprintf(out,"%s:Zeile %d: %04x:Unbekannter Fehler Nr.: %d\n",*/
			sprintf(out, "%s:line %d: %04x: Unknown error # %d\n", filep->fname,
					filep->fline, pc[segment], er);
			ner++;
		}
	} else {
		if (er == E_NODEF)
			sprintf(out, "%s:line %d: %04x:Label '%s' not defined\n",
					filep->fname, filep->fline, pc[segment], lz);
		else
			sprintf(out, "%s:line %d: %04x:%s error\n", filep->fname,
					filep->fline, pc[segment], ertxt[(-er) - 1]);

		ner++;
	}
	logout(out);
}

static void chrput(int c) {
	/*     printf(" %02x",c&255);*/

	putc(c & 0x00ff, fpout);
}

void logout(char *s) {
	fprintf(stderr, "%s", s);
	if (fperr)
		fprintf(fperr, "%s", s);
}

/*****************************************************************/

typedef struct {
	char *name;
	int *flag;
} compat_set;

static compat_set compat_sets[] = { { "MASM", &masm }, { "CA65", &ca65 }, { "C",
		&ctypes }, { "XA23", &xa23 }, { NULL, NULL } };

int set_compat(char *compat_name) {
	int i = 0;
	while (compat_sets[i].name != NULL) {
		if (strcmp(compat_sets[i].name, compat_name) == 0) {
			/* set appropriate compatibility flag */
			(*compat_sets[i].flag) = 1;

			/* warn on old versions of xa */
			if (xa23)
				fprintf(stderr, "Warning: -XXA23 is explicitly deprecated\n");

			return 0;
		}
		i++;
	}
	return -1;
}

