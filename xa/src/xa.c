/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
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

#include "version.h"

/* ANZERR: total number of errors */
/* ANZWARN: total number of warnings */

#define ANZERR		64
#define ANZWARN		13

#define programname	"xa"
#define progversion	"v2.3.12"
#define authors		"Written by Andre Fachat, Jolse Maginnis, David Weinehall and Cameron Kaiser"
#define copyright	"Copyright (C) 1989-2021 Andre Fachat, Jolse Maginnis, David Weinehall\nand Cameron Kaiser."

/* exported globals */
int ncmos, cmosfl, w65816, n65816;
int masm = 0;
int ppinstr = 0;
int nolink = 0;
int romable = 0;
int romaddr = 0;
int noglob = 0;
int showblk = 0;
int crossref = 0;
char altppchar;

/* local variables */
static char out[MAXLINE];
static time_t tim1, tim2;
static FILE *fpout, *fperr, *fplab;
static int ner = 0;

static int align = 1;

static void printstat(void);
static void usage(int, FILE *);
static int setfext(char *, char *);
static int x_init(void);
static int pass1(void);
static int pass2(void);
static int puttmp(int);
static int puttmps(signed char *, int);
static void chrput(int);
static int xa_getline(char *);
static void lineout(void);
static long ga_p1(void);
static long gm_p1(void);

/* text */
int memode,xmode;
int segment;
int tlen=0, tbase=0x1000;
int dlen=0, dbase=0x0400;
int blen=0, bbase=0x4000;
int zlen=0, zbase=4;
int fmode=0;
int relmode=0;

int pc[SEG_MAX];	/* segments */

int main(int argc,char *argv[])
{
     int er=1,i;
     signed char *s=NULL;
     char *tmpp;

     int mifiles = 5;
     int nifiles = 0;
     int verbose = 0;
     int oldfile = 0;
     int no_link = 0;

     char **ifiles;
     char *ofile;
     char *efile;
     char *lfile;
     char *ifile;
     
     char old_e[MAXLINE];
     char old_l[MAXLINE];
     char old_o[MAXLINE];

     tim1=time(NULL);
     
     ncmos=0;
     n65816=0;
     cmosfl=1;
     w65816=0;	/* default: 6502 only */

     altppchar = '#' ; /* i.e., NO alternate char */

     if((tmpp = strrchr(argv[0],'/'))) {
	tmpp++;
     } else {
	tmpp = argv[0];
     }
     if( (!strcmp(tmpp,"xa65816"))
	|| (!strcmp(tmpp,"XA65816"))
	|| (!strcmp(tmpp,"xa816"))
	|| (!strcmp(tmpp,"XA816"))
	) {
	w65816 = 1;	/* allow 65816 per default */
     }

     /* default output charset for strings in quotes */
     set_charset("ASCII");

     ifiles = malloc(mifiles*sizeof(char*));

     afile = alloc_file();

     if (argc <= 1) {
          usage(w65816, stderr);
          exit(1);
     }

     if (strstr(argv[1], "--help")) {
          usage(w65816, stdout);
	  exit(0);
     }

     if (strstr(argv[1], "--version")) {
          version(programname, progversion, authors, copyright);
	  exit(0);
     }

     ofile="a.o65";
     efile=NULL;
     lfile=NULL;

     if(pp_init()) {
       logout("fatal: pp: no memory!");
       return 1;
     }
     if(b_init()) {
       logout("fatal: b: no memory!");
       return 1;
     }
     if(l_init()) {
       logout("fatal: l: no memory!");
       return 1;
     }

     i=1;
     while(i<argc) {
	if(argv[i][0]=='-') {
	  switch(argv[i][1]) {
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
				"using -p# is evidence of stupidity\n");
		altppchar = argv[i][2];
		if (argv[i][3] != '\0')
			fprintf(stderr,
				"warning: extra characters to -p ignored\n");
		break;
	  case 'M':
		masm = 1;	/* MASM compatibility mode */
		break;
          case 'S':
                ppinstr = 1;    /* preprocessor substitution in strings ok */
                fprintf(stderr, "Warning: -S is deprecated and will be removed in 2.4+!\n");
		break;
	  case 'O':		/* output charset */
		{
		  char *name = NULL;
		  if (argv[i][2] == 0) { 
		    name = argv[++i]; 
		  } else {
		    name = argv[i]+2;
		  }
		  if (set_charset(name) < 0) {
		    fprintf(stderr, "Output charset name '%s' unknown - ignoring! (check case?)\n", name);
		  }
		}
		break;
	  case 'A':		/* make text segment start so that text relocation
				   is not necessary when _file_ starts at adr */
		romable = 2;
		if(argv[i][2]==0) romaddr = atoi(argv[++i]);
		else romaddr = atoi(argv[i]+2);
		break;
	  case 'G':
		noglob = 1;
		break;
	  case 'L':		/* define global label */
		if(argv[i][2]) lg_set(argv[i]+2);
		break;
	  case 'r':
		crossref = 1;
		break;
	  case 'R':
		relmode = 1;
		break;
	  case 'D':
		s = (signed char*)strstr(argv[i]+2,"=");
		if(s) *s = ' ';
		pp_define(argv[i]+2);
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
		break;
          case 'W':
                w65816 = 0;
                break;
          case 'w':
                w65816 = 1;
                break;
	  case 'B':
		showblk = 1;
		break;
	  case 'x':		/* old filename behaviour */
		oldfile = 1;
		fprintf(stderr, "Warning: -x is now deprecated and will be removed in 2.4+!\n");
		break;
	  case 'I':
		if(argv[i][2]==0) {
		  reg_include(argv[++i]);
		} else {
		  reg_include(argv[i]+2);
		}
		break;
	  case 'o':
		if(argv[i][2]==0) {
		  ofile=argv[++i];
		} else {
		  ofile=argv[i]+2;
		}
		break;
	  case 'l':
		if(argv[i][2]==0) {
		  lfile=argv[++i];
		} else {
		  lfile=argv[i]+2;
		}
		break;
	  case 'e':
		if(argv[i][2]==0) {
		  efile=argv[++i];
		} else {
		  efile=argv[i]+2;
		}
		break;
	  case 'b':			/* set segment base addresses */
		switch(argv[i][2]) {
		case 't':
			if(argv[i][3]==0) tbase = atoi(argv[++i]);
			else tbase = atoi(argv[i]+3);
			break;
		case 'd':
			if(argv[i][3]==0) dbase = atoi(argv[++i]);
			else dbase = atoi(argv[i]+3);
			break;
		case 'b':
			if(argv[i][3]==0) bbase = atoi(argv[++i]);
			else bbase = atoi(argv[i]+3);
			break;
		case 'z':
			if(argv[i][3]==0) zbase = atoi(argv[++i]);
			else zbase = atoi(argv[i]+3);
			break;
		default:
			fprintf(stderr,"unknown segment type '%c' - ignoring!\n",
								argv[i][2]);
			break;
		}
		break;
	  case 0:
		fprintf(stderr, "Single dash '-' on command line - ignoring!\n");
		break;
	  default:
		fprintf(stderr, "Unknown option '%c' - ignoring!\n",argv[i][1]);
		break;
	  }
	} else {		/* no option -> filename */
	  ifiles[nifiles++] = argv[i];
	  if(nifiles>=mifiles) {
	    mifiles += 5;
	    ifiles=realloc(ifiles, mifiles*sizeof(char*));
	    if(!ifiles) {
		fprintf(stderr, "Oops: couldn't alloc enough mem for filelist table..!\n");
		exit(1);
	    }
	  }
	}
	i++;
     }
     if(!nifiles) {
	fprintf(stderr, "No input files given!\n");
	exit(0);
     }

     if(oldfile) {
	strcpy(old_e, ifiles[0]);
	strcpy(old_o, ifiles[0]);
	strcpy(old_l, ifiles[0]);

	if(setfext(old_e,".err")==0) efile = old_e;
	if(setfext(old_o,".obj")==0) ofile = old_o;
	if(setfext(old_l,".lab")==0) lfile = old_l;
     }
     if(verbose) fprintf(stderr, "%s\n",copyright);

     fplab= lfile ? xfopen(lfile,"w") : NULL;
     fperr= efile ? xfopen(efile,"w") : NULL;
     if(!strcmp(ofile,"-")) {
	ofile=NULL;
        fpout = stdout;
     } else {
        fpout= xfopen(ofile,"wb");
     }
     if(!fpout) {
	fprintf(stderr, "Couldn't open output file!\n");
	exit(1);
     }

     if(1 /*!m_init()*/)
     {
       if(1 /*!b_init()*/)
       {
         if(1 /*!l_init()*/)
         {
           /*if(!pp_init())*/
           {
             if(!x_init())
             {
	       /* if(fperr) fprintf(fperr,"%s\n",copyright); */
	       if(verbose) logout(ctime(&tim1));

	       /* Pass 1 */

               pc[SEG_ABS]= 0;		/* abs addressing */
	       seg_start(fmode, tbase, dbase, bbase, zbase, 0, relmode);

	       if(relmode) {
		 r_mode(RMODE_RELOC);
		 segment = SEG_TEXT;
	       } else {
		 /* prime old_segment in r_mode with SEG_TEXT */
	         segment = SEG_ABS;
		 r_mode(RMODE_ABS);
	       }

	       nolink = no_link;

               for (i=0; i<nifiles; i++)
               {
		 ifile = ifiles[i];

                 sprintf(out,"xAss65: Pass 1: %s\n",ifile);
                 if(verbose) logout(out);

                 er=pp_open(ifile);
                    puttmp(0);
                    puttmp(T_FILE);
                    puttmp(0);
                    puttmp(0);
                    puttmps((signed char*)&ifile, sizeof(filep->fname));

                 if(!er) {
                   er=pass1();
                   pp_close();
                 } else {
		   sprintf(out, "Couldn't open source file '%s'!\n", ifile);
		   logout(out);
                 }
               }           

	       if((er=b_depth())) {
		 sprintf(out,"Still %d blocks open at end of file!\n",er);
		 logout(out);
	       }

	       if(tbase & (align-1)) {
		   sprintf(out,"Warning: text segment ($%04x) start address doesn't align to %d!\n", tbase, align);
		   logout(out);
	       }
	       if(dbase & (align-1)) {
		   sprintf(out,"Warning: data segment ($%04x) start address doesn't align to %d!\n", dbase, align);
		   logout(out);
	       }
	       if(bbase & (align-1)) {
		   sprintf(out,"Warning: bss segment ($%04x) start address doesn't align to %d!\n", bbase, align);
		   logout(out);
	       }
	       if(zbase & (align-1)) {
		   sprintf(out,"Warning: zero segment ($%04x) start address doesn't align to %d!\n", zbase, align);
		   logout(out);
	       }
               if (n65816>0)
                   fmode |= 0x8000;
	       switch(align) {
		case 1: break;
		case 2: fmode |= 1; break;
		case 4: fmode |= 2; break;
		case 256: fmode |=3; break;
	       }
	       
	       if((!er) && relmode) 
			h_write(fpout, fmode, tlen, dlen, blen, zlen, 0);


               if(!er)
               {
                    if(verbose) logout("xAss65: Pass 2:\n");

		    seg_pass2();

	     	    if(relmode) {
		 	r_mode(RMODE_RELOC);
		 	segment = SEG_TEXT;
	            } else {
		 	/* prime old_segment in r_mode with SEG_TEXT */
	         	segment = SEG_ABS;
		 	r_mode(RMODE_ABS);
	            }
                    er=pass2();
               } 

               if(fplab) printllist(fplab);
               tim2=time(NULL);
	       if(verbose) printstat();

	       if((!er) && relmode) seg_end(fpout);	/* write reloc/label info */
			                              
               if(fperr) fclose(fperr);
               if(fplab) fclose(fplab);
               if(fpout) fclose(fpout);

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

     if(ner || er)
     {
          fprintf(stderr, "Break after %d error%c\n",ner,ner?'s':0);
	  /*unlink();*/
	  if(ofile) {
	    unlink(ofile);
	  }
     }

     free(ifiles);

     return( (er || ner) ? 1 : 0 );
}

static void printstat(void)
{
	logout("Statistics:\n");
	sprintf(out," %8d of %8d label used\n",ga_lab(),gm_lab()); logout(out);
	sprintf(out," %8ld of %8ld byte label-memory used\n",ga_labm(),gm_labm()); logout(out);
	sprintf(out," %8d of %8d PP-defs used\n",ga_pp(),gm_pp()); logout(out);
	sprintf(out," %8ld of %8ld byte PP-memory used\n",ga_ppm(),gm_ppm()); logout(out);
	sprintf(out," %8ld of %8ld byte buffer memory used\n",ga_p1(),gm_p1()); logout(out);
	sprintf(out," %8d blocks used\n",ga_blk()); logout(out);
	sprintf(out," %8ld seconds used\n",(long)difftime(tim2,tim1)); logout(out);
}

int h_length(void) {
	return 26+o_length();
}

#if 0
/* write header for relocatable output format */
int h_write(FILE *fp, int tbase, int tlen, int dbase, int dlen, 
				int bbase, int blen, int zbase, int zlen) {

	fputc(1, fp);			/* version byte */
	fputc(0, fp);			/* hi address 0 -> no C64 */
	fputc("o", fp);
	fputc("6", fp);
	fputc("5", fp);			
	fputc(0, fp);			/* format version */
	fputw(mode, fp);		/* file mode */
	fputw(tbase,fp);		/* text base */
	fputw(tlen,fp);			/* text length */
	fputw(dbase,fp);		/* data base */
	fputw(dlen,fp);			/* data length */
	fputw(bbase,fp);		/* bss base */
	fputw(blen,fp);			/* bss length */
	fputw(zbase,fp);		/* zerop base */
	fputw(zlen,fp);			/* zerop length */

	o_write(fp);

	return 0;
}
#endif

static int setfext(char *s, char *ext)
{
     int j,i=(int)strlen(s);

     if(i>MAXLINE-5)
          return(-1);
          
     for(j=i-1;j>=0;j--)
     {
          if(s[j]==DIRCHAR)
          {
               strcpy(s+i,ext);
               break;
          }
          if(s[j]=='.')
          {
               strcpy(s+j,ext);
               break;
          }
     }
     if(!j)
          strcpy(s+i,ext);

     return(0);
}

/*
static char *tmp;
static unsigned long tmpz;
static unsigned long tmpe;
*/

static long ga_p1(void)
{
	return(afile->mn.tmpz);
}
static long gm_p1(void)
{
	return(TMPMEM);
}

static int pass2(void)
{
     int c,er,l,ll,i,al;
     Datei datei;
     signed char *dataseg=NULL;
     signed char *datap=NULL;

     memode=0;
     xmode=0;
     if((dataseg=malloc(dlen))) {
       if(!dataseg) {
	 fprintf(stderr, "Couldn't alloc dataseg memory...\n");
	 exit(1);
       }
       datap=dataseg;
     }
     filep=&datei;
     afile->mn.tmpe=0L;

     while(ner<20 && afile->mn.tmpe<afile->mn.tmpz)
     {
          l=afile->mn.tmp[afile->mn.tmpe++];
          ll=l;

          if(!l)
          {
               if(afile->mn.tmp[afile->mn.tmpe]==T_LINE)
               {
                    datei.fline=(afile->mn.tmp[afile->mn.tmpe+1]&255)+(afile->mn.tmp[afile->mn.tmpe+2]<<8);
                    afile->mn.tmpe+=3;
               } else
               if(afile->mn.tmp[afile->mn.tmpe]==T_FILE)
               {
                    datei.fline=(afile->mn.tmp[afile->mn.tmpe+1]&255)+(afile->mn.tmp[afile->mn.tmpe+2]<<8);

		    memcpy(&datei.fname, afile->mn.tmp+afile->mn.tmpe+3, sizeof(datei.fname));
                    afile->mn.tmpe+=3+sizeof(datei.fname);
/*
		    datei.fname = malloc(strlen((char*) afile->mn.tmp+afile->mn.tmpe+3)+1);
		    if(!datei.fname) {
			fprintf(stderr,"Oops, no more memory\n");
			exit(1);
		    }
                    strcpy(datei.fname,(char*) afile->mn.tmp+afile->mn.tmpe+3);
                    afile->mn.tmpe+=3+strlen(datei.fname);
*/
               }
          } else
          {
/* do not attempt address mode optimization on pass 2 */
               er=t_p2(afile->mn.tmp+afile->mn.tmpe,&ll,1,&al);

               if(er==E_NOLINE)
               {
               } else
               if(er==E_OK)
               {
		  if(segment<SEG_DATA) { 
                    for(i=0;i<ll;i++)
                         chrput(afile->mn.tmp[afile->mn.tmpe+i]);
		  } else if (segment==SEG_DATA && datap) {
		    memcpy(datap,afile->mn.tmp+afile->mn.tmpe,ll);
		    datap+=ll;
		  }
               } else
               if(er==E_DSB)
               {
                  c=afile->mn.tmp[afile->mn.tmpe];
		  if(segment<SEG_DATA) {
                    /*printf("E_DSB, ll=%d, l=%d, c=%c\n",ll,l,afile->mn.tmp[afile->mn.tmpe]);*/
                    for(i=0;i<ll;i++)
                         chrput(c);
		  } else if (segment==SEG_DATA && datap) {
		    memset(datap, c, ll);
		    datap+=ll;
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

			offset = afile->mn.tmp[i] +
				(afile->mn.tmp[i+1] << 8) +
				(afile->mn.tmp[i+2] << 16);
			fstart = afile->mn.tmp[i+3] + 1 +
				(afile->mn.tmp[i+4] << 8);
			/* usually redundant but here for single-char names
				that get interpreted as chars */
			flen = afile->mn.tmp[i+5];
			if (flen > 1) fstart++; 
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

			for(j=0; j<flen; j++) {
				binfnam[j] = afile->mn.tmp[i+fstart+j];
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
				for(j=0; j<ll; j++) {
					/* damn you Andre ;-) */
					i = fgetc(foo);
					if (segment<SEG_DATA) {
						chrput(i);
					}
					if (segment==SEG_DATA && datap) {
						memset(datap++, i, 1);
					}
				}
				fclose(foo);
			}			
		} else {
                    errout(er);
               }
          }
          afile->mn.tmpe+=abs(l);
     }
     if(relmode) {
       if((ll=fwrite(dataseg, 1, dlen, fpout))<dlen) {
	fprintf(stderr, "Problems writing %d bytes, return gives %d\n",dlen,ll);
       }
     }

     return(ner);
}     
     

static int pass1(void)
{
     signed char o[MAXLINE];
     int l,er,temp_er,al;

     memode=0;
     xmode=0;
     tlen=0;
     ner=0;

	temp_er = 0;

/*FIXIT*/
     while(!(er=xa_getline(s)))
     {         
          er=t_p1((signed char*)s,o,&l,&al);
	  switch(segment) {
	    case SEG_ABS:
	    case SEG_TEXT: tlen += al; break;
	    case SEG_DATA: dlen += al; break;
	    case SEG_BSS : blen += al; break;
	    case SEG_ZERO: zlen += al; break;
	  }

          /*printf(": er= %d, l=%d, tmpz=%d\n",er,l,tmpz); */

          if(l)
          {
            if(er)
            {
               if(er==E_OKDEF)
               {
                    if(!(er=puttmp(l)))
                         er=puttmps(o,l);
               } else
               if(er==E_NOLINE)
                    er=E_OK;
            } else
            {
               if(!(er=puttmp(-l)))
                    er=puttmps(o,l);
            }
          }
          if(er)
          {
               lineout();
               errout(er);
          }

/*          printf("tmpz =%d\n",afile->mn.tmpz);
*/
     } 

     if(er!=E_EOF) {
          errout(er);
	}

          

/*     { int i; printf("Pass 1 \n");
     for(i=0;i<afile->mn.tmpz;i++)
          fprintf(stderr, " %02x",255 & afile->mn.tmp[i]);
     getchar();}
*/
     return(ner);
}

static void usage(int default816, FILE *fp)
{
     fprintf(fp,
	    "Usage: %s [options] file\n"
	    "Cross-assembler for 65xx/R65C02/65816\n"
	    "\n",
            programname);
	fprintf(fp,
	    " -v           verbose output\n"
            " -C           no CMOS-opcodes\n"
            " -W           no 65816-opcodes%s\n"
            " -w           allow 65816-opcodes%s\n",
	    default816 ? "" : " (default)",
	    default816 ? " (default)" : "");
	fprintf(fp,
            " -B           show lines with block open/close\n"
            " -c           produce `o65' object instead of executable files (i.e. don't link)\n"
	    " -o filename  sets output filename, default is `a.o65'\n"
	    "                A filename of `-' sets stdout as output file\n");
	fprintf(fp,
	    " -e filename  sets errorlog filename, default is none\n"
	    " -l filename  sets labellist filename, default is none\n"
	    " -r           adds crossreference list to labellist (if `-l' given)\n"
	    " -M           allow ``:'' to appear in comments for MASM compatibility\n"
	    " -R           start assembler in relocating mode\n");
	fprintf(fp,
	    " -Llabel      defines `label' as absolute, undefined label even when linking\n"
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
	fprintf(fp,
	    "== These options are deprecated and will be removed in 2.4+! ==\n"
	    " -x           old filename behaviour (overrides `-o', `-e', `-l')\n"
	    " -S           allow preprocessor substitution within strings\n");
}

/*
static char *ertxt[] = { "Syntax","Label definiert",
          "Label nicht definiert","Labeltabelle voll",
          "Label erwartet","Speicher voll","Illegaler Opcode",
          "Falsche Adressierungsart","Branch ausserhalb des Bereichs",
          "Ueberlauf","Division durch Null","Pseudo-Opcode erwartet",
          "Block-Stack-Ueberlauf","Datei nicht gefunden",
          "End of File","Block-Struktur nicht abgeschlossen",
          "NoBlk","NoKey","NoLine","OKDef","DSB","NewLine",
          "NewFile","CMOS-Befehl","pp:Falsche Anzahl Parameter" };
*/
static char *ertxt[] = {
	"Syntax",
	"Label already defined",
        "Label not defined",
	"Label table full",
        "Label expected",
	"Out of memory",
	"Illegal opcode",
        "Wrong addressing mode",
	"Branch out of range",
        "Overflow",
	"Division by zero",
	"Pseudo-opcode expected",
        "Block stack overflow",
	"File not found",
        "End of file",
	"Unmatched block close",
        "NoBlk",
	"NoKey",
	"NoLine",
	"OKDef",
	"DSB",
	"NewLine",
        "NewFile",
	"CMOS instruction used with -C",
	"pp:Wrong parameter count",
	"Illegal pointer arithmetic", 
	"Illegal segment",
	"File header option too long",
	"File option not at file start (when ROM-able)",
	"Illegal align value",
        "65816 mode used/required",
	"Exceeded recursion limit for label evaluation",
	"Unresolved preprocessor directive at end of file",
	"Data underflow",
	"Illegal quantity",
	".bin",
/* placeholders for future fatal errors */
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
/* warnings */
	  "Cutting word relocation in byte value",
	  "Byte relocation in word value",
	  "Illegal pointer arithmetic",
	  "Address access to low or high byte pointer",
	  "High byte access to low byte pointer",
	  "Low byte access to high byte pointer",
	  "Can't optimize forward-defined label; using absolute addressing",
	  "Open preprocessor directive at end of file (intentional?)",
	  "Included binary data exceeds 64KB",
	  "Included binary data exceeds 16MB",
          "MVN/MVP $XXXX syntax is deprecated and will be removed",
/* more placeholders */
		"",
		"",

 };

static int gl;
static int gf;  

static int x_init(void)
{
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

static int puttmp(int c)
{
     int er=E_NOMEM;
/*printf("puttmp: afile=%p, tmp=%p, tmpz=%d\n",afile, afile?afile->mn.tmp:0, afile?afile->mn.tmpz:0);*/
     if(afile->mn.tmpz<TMPMEM)
     {
          afile->mn.tmp[afile->mn.tmpz++]=c;
          er=E_OK;
     }
     return(er);
}

static int puttmps(signed char *s, int l)
{
     int i=0,er=E_NOMEM;
     
     if(afile->mn.tmpz+l<TMPMEM)
     {
          while(i<l)
               afile->mn.tmp[afile->mn.tmpz++]=s[i++];

          er=E_OK;
     }
     return(er);
}

static char l[MAXLINE];

static int xa_getline(char *s)
{
     static int ec;

     static int i,c;
     int hkfl,j,comcom;

     j=hkfl=comcom=0;
     ec=E_OK;

     if(!gl)
     {
          do
          {
               ec=pgetline(l);
               i=0;
               while(l[i]==' ')
                    i++;
               while(l[i]!='\0' && isdigit(l[i]))
                    i++;
               gf=1;

               if(ec==E_NEWLINE)
               {
                    puttmp(0);
                    puttmp(T_LINE);
                    puttmp((filep->fline)&255);
                    puttmp(((filep->fline)>>8)&255);
		ec=E_OK;

               }
		else
               if(ec==E_NEWFILE)
               {
                    puttmp(0);
                    puttmp(T_FILE);
                    puttmp((filep->fline)&255);
                    puttmp(((filep->fline)>>8)&255);
		    puttmps((signed char*)&(filep->fname), sizeof(filep->fname));
/*
                    puttmps((signed char*)filep->fname,
					1+(int)strlen(filep->fname));
*/
                    ec=E_OK;
               }
          } while(!ec && l[i]=='\0');
     }

     gl=0;
     if(!ec || ec==E_EOF)
     {
          do {
               c=s[j]=l[i++];

                if (!(hkfl&2) && c=='\"')
                    hkfl^=1;
                if (!comcom && !(hkfl&1) && c=='\'')
                    hkfl^=2;
		if (c==';' && !hkfl)
			comcom = 1;
               if (c=='\0') 
                    break;	/* hkfl = comcom = 0 */
		if (c==':' && !hkfl && (!comcom || !masm)) {
                    		gl=1;
                    		break;
               }
               j++;
          } while (c!='\0' && j<MAXLINE-1 && i<MAXLINE-1);
     
          s[j]='\0';
     } else
          s[0]='\0';

     return(ec);
}

void set_align(int a) {
	align = (a>align)?a:align;
}

static void lineout(void)
{
     if(gf)
     {
          logout(filep->flinep);
          logout("\n");
          gf=0;
     }
}

void errout(int er)
{
     if (er<-ANZERR || er>-1) {
	if(er>=-(ANZERR+ANZWARN) && er < -ANZERR) {
	  sprintf(out,"%s:line %d: %04x: Warning - %s\n",
		filep->fname, filep->fline, pc[segment], ertxt[(-er)-1]);
	} else {
          /* sprintf(out,"%s:Zeile %d: %04x:Unbekannter Fehler Nr.: %d\n",*/
          sprintf(out,"%s:line %d: %04x: Unknown error # %d\n",
               filep->fname,filep->fline,pc[segment],er);
	  ner++;
	}
     } else {
       if (er==E_NODEF)
          sprintf(out,"%s:line %d: %04x:Label '%s' not defined\n",
               filep->fname,filep->fline,pc[segment],lz);
       else  
          sprintf(out,"%s:line %d: %04x:%s error\n",
               filep->fname,filep->fline,pc[segment],ertxt[(-er)-1]);

       ner++;
     }
     logout(out);
}

static void chrput(int c)
{
     /*     printf(" %02x",c&255);*/

     putc( c&0x00ff,fpout);
}

void logout(char *s)
{
     fprintf(stderr, "%s",s);
     if(fperr)
          fprintf(fperr,"%s",s);
}

