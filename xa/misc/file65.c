/* file65 -- A part of xa65 - 65xx/65816 cross-assembler and utility suite
 * Print information about o65 files
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

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "version.h"

#define	BUF	(9*4+8)

#define programname	"file65"
#define progversion	"v0.2.1"
#define author		"Written by Andre Fachat"
#define copyright	"Copyright (C) 1997-2002 Andre Fachat."

int read_options(FILE *fp);
int print_labels(FILE *fp, int offset);

unsigned char hdr[BUF];
unsigned char cmp[] = { 1, 0, 'o', '6', '5' };

int xapar = 0;
int rompar = 0;
int romoff = 0;
int labels = 0;

void usage(FILE *fp)
{
	fprintf(fp,
		"Usage: %s [options] [file]\n"
		"Print file information about o65 files\n"
		"\n",
		programname);
	fprintf(fp,
		"  -P         print the segment end addresses according to `xa' command line\n"
		"               parameters `-b?'\n"
		"  -a offset  print `xa' ``romable'' parameter for another file behind this one\n"
		"               in the same ROM. Add offset to start address.\n"
		"  -A offset  same as `-a', but only print the start address of the next\n"
		"               file in the ROM\n"
		"  -V         print undefined and global labels\n"
		"  --version  output version information and exit\n"
		"  --help     display this help and exit\n");
}

int main(int argc, char *argv[]) {
	int i = 1, n, mode, hlen;
	FILE *fp;
	char *aligntxt[4]= {"[align 1]","[align 2]","[align 4]","[align 256]"};
	if(argc<=1) {
		usage(stderr);
		exit(1);
	}

	if (strstr(argv[1], "--help")) {
          usage(stdout);
	  exit(0);
	}

	if (strstr(argv[1], "--version")) {
          version(programname, progversion, author, copyright);
	  exit(0);
	}

	while(i<argc) {
	  if(argv[i][0]=='-') {
	    /* process options */
	    switch(argv[i][1]) {
	    case 'V':
		labels = 1;
		break;
	    case 'v':
		printf("file65: Version 0.2\n");
		break;
	    case 'a':
	    case 'A':
		rompar = 1;
		if(argv[i][1]=='A') rompar++;
		if(argv[i][2]) romoff = atoi(argv[i]+2);
		else romoff = atoi(argv[++i]);
		break;
	    case 'P':
		xapar = 1;
		break;
	    default:
		fprintf(stderr,"file65: %s unknown option\n",argv[i]);
		break;
	    }
	  } else {
	    fp = fopen(argv[i],"rb");
	    if(fp) {
	      n = fread(hdr, 1, 8, fp);
	      if((n>=8) && (!memcmp(hdr, cmp, 5))) {
		mode=hdr[7]*256+hdr[6];
		if(!xapar && !rompar) {
		  printf("%s: o65 version %d %s file\n", argv[i], hdr[5],
				hdr[7]&0x10 ? "object" : "executable");
		  printf(" mode: %04x =",mode );
		  printf("%s%s%s%s%s\n", 
			(mode & 0x1000)?"[object]":"[executable]",
			(mode & 0x2000)?"[32bit]":"[16bit]",
			(mode & 0x4000)?"[page relocation]":"[byte relocation]",
			(mode & 0x8000)?"[CPU 65816]":"[CPU 6502]",
			aligntxt[mode & 3]);
		}
		if(mode & 0x2000) {
	          fprintf(stderr,"file65: %s: 32 bit size not supported\n", argv[i]);
		} else {
		  n=fread(hdr+8, 1, 18, fp);
		  if(n<18) {
	            fprintf(stderr,"file65: %s: truncated file\n", argv[i]);
		  } else {
		    if(!xapar && !rompar) {
		      printf(" text segment @ $%04x - $%04x [$%04x bytes]\n", hdr[9]*256+hdr[8], hdr[9]*256+hdr[8]+hdr[11]*256+hdr[10], hdr[11]*256+hdr[10]);
		      printf(" data segment @ $%04x - $%04x [$%04x bytes]\n", hdr[13]*256+hdr[12], hdr[13]*256+hdr[12]+hdr[15]*256+hdr[14], hdr[15]*256+hdr[14]);
		      printf(" bss  segment @ $%04x - $%04x [$%04x bytes]\n", hdr[17]*256+hdr[16], hdr[17]*256+hdr[16]+hdr[19]*256+hdr[18], hdr[19]*256+hdr[18]);
		      printf(" zero segment @ $%04x - $%04x [$%04x bytes]\n", hdr[21]*256+hdr[20], hdr[21]*256+hdr[20]+hdr[23]*256+hdr[22], hdr[23]*256+hdr[22]);
		      printf(" stack size $%04x bytes %s\n", hdr[25]*256+hdr[24],
				(hdr[25]*256+hdr[24])==0?"(i.e. unknown)":"");
		      if(labels) {
			read_options(fp);
			print_labels(fp,  hdr[11]*256+hdr[10] +  hdr[15]*256+hdr[14]);
		      }
		    } else {
		      struct stat fbuf;
		      hlen = 8+18+read_options(fp);
		      stat(argv[i],&fbuf);
		      if(xapar) {
			if(!rompar) printf("-bt %d ",
		  	  (hdr[9]*256+hdr[8]) + (hdr[11]*256+hdr[10])
			);
			printf("-bd %d -bb %d -bz %d ",
			  (hdr[13]*256+hdr[12]) + (hdr[15]*256+hdr[14]),
			  (hdr[17]*256+hdr[16]) + (hdr[19]*256+hdr[18]),
			  (hdr[21]*256+hdr[20]) + (hdr[23]*256+hdr[22])
		        ); 
		      }
		      if(rompar==1) {
			printf("-A %lu ", (unsigned long)((hdr[9]*256+hdr[8])
						-hlen +romoff +(fbuf.st_size)));
		      } else
		      if(rompar==2) {
			printf("%lu ", (unsigned long)((hdr[9]*256+hdr[8])
						-hlen +romoff +(fbuf.st_size)));
		      }
		      printf("\n");
		    }
		  }
		}
	      } else {
	        fprintf(stderr,"file65: %s: not an o65 file!\n", argv[i]);
		if(hdr[0]==1 && hdr[1]==8 && hdr[3]==8) {
		  printf("%s: C64 BASIC executable (start address $0801)?\n", argv[i]);
		} else
		if(hdr[0]==1 && hdr[1]==4 && hdr[3]==4) {
		  printf("%s: CBM PET BASIC executable (start address $0401)?\n", argv[i]);
		}
	      }
	    } else {
	      fprintf(stderr,"file65: %s: %s\n", argv[i], strerror(errno));
	    }
	  }
	  i++;
	}
	return 0;
}

static struct { int opt; int strfl; char *string; } otab[] = {
	{ 0, 1, "Filename" },
	{ 1, 0, "O/S Type" },
	{ 2, 1, "Assembler" },
	{ 3, 1, "Author" },
	{ 4, 1, "Creation Date" },
	{ -1, -1, NULL }
};

static char* stab[] = {
	"undefined" ,
	"absolute" ,
	"text" ,
	"data" ,
	"bss" ,
	"zero" ,
	"-" ,
	"-" 
};

void print_option(unsigned char *buf, int len) {
	int i, strfl=0;
	for(i=0;otab[i].opt>=0; i++) if(*buf==otab[i].opt) break;
	if(otab[i].opt>=0) {
	    printf("fopt: %-17s: ", otab[i].string);
	    strfl = otab[i].strfl;
	} else {
	    printf("fopt: Unknown Type $%02x : ", (*buf & 0xff));
	}
	if(strfl) {
	    buf[len]=0;
	    printf("%s\n", buf+1);
	} else {
	    for (i=1; i<len-1; i++) {
		printf("%02x ", buf[i] & 0xff);
	    }
	    printf("\n");
	}
}

int read_options(FILE *fp) {
	int c, l=0;
	unsigned char tb[256];

	c=fgetc(fp); l++;
	while(c && c!=EOF) {
	  c&=255;
	  fread(tb, 1, c-1, fp);
	  if(labels) print_option(tb, c);
	  l+=c;
	  c=fgetc(fp);
	}
	return l;
}

int print_labels(FILE *fp, int offset) {
	int i, nud, c, seg, off;
/*
printf("print_labels:offset=%d\n",offset);
*/
	fseek(fp, offset, SEEK_CUR);

	nud = (fgetc(fp) & 0xff);
	nud += ((fgetc(fp) << 8) & 0xff00);

	printf("Undefined Labels: %d\n", nud);

	if(nud) {
	  do {
	    c=fgetc(fp);
	    while(c && c!=EOF) {
	 	fputc(c, stdout);
	        c=fgetc(fp);
	    }
	    printf("\t");
	  } while(--nud);
	  printf("\n");
	}

	for(i=0;i<2;i++) {
	 c=fgetc(fp);
	 while(c && c!=EOF) {
	  c&= 0xff;
	  while(c == 255 && c!= EOF) {
	    c=fgetc(fp);
	    if(c==EOF) break;
	    c&= 0xff;
	  }
	  if(c==EOF) break;

	  c=fgetc(fp);
	  if( (c & 0xe0) == 0x40 ) fgetc(fp);
	  if( (c & 0x07) == 0 ) { fgetc(fp); fgetc(fp); }

	  c=fgetc(fp);
	 }
	}

	nud = (fgetc(fp) & 0xff);
	nud += ((fgetc(fp) << 8) & 0xff00);
	printf("Global Labels: %d\n", nud);

	if(nud) {	
	  do {
	    c=fgetc(fp);
	    while(c && c!=EOF) {
	 	fputc(c, stdout);
	        c=fgetc(fp);
	    }
	    if(c==EOF) break;

	    seg = fgetc(fp) & 0xff;
	    off= (fgetc(fp) & 0xff);
	    off+= ((fgetc(fp) << 8) & 0xff00);
	    printf(" (segID=%d (%s), offset=%04x)\n", seg, stab[seg&7], off);
		 
	  } while(--nud);
	}
	return 0;
}	


