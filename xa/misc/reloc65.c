/* reloc65 -- A part of xa65 - 65xx/65816 cross-assembler and utility suite
 * o65 file relocator
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "version.h"

#define	BUF	(9*2+8)		/* 16 bit header */

#define programname	"reloc65"
#define progversion	"v0.2.1"
#define author		"Written by Andre Fachat"
#define copyright	"Copyright (C) 1997-2002 Andre Fachat."

typedef struct {
	char 		*fname;
	size_t 		fsize;
	unsigned char	*buf;
	int		tbase, tlen, dbase, dlen, bbase, blen, zbase, zlen;
	int		tdiff, ddiff, bdiff, zdiff;
	unsigned char	*segt;
	unsigned char	*segd;
	unsigned char 	*utab;
	unsigned char 	*rttab;
	unsigned char 	*rdtab;
	unsigned char 	*extab;
} file65;


int read_options(unsigned char *f);
int read_undef(unsigned char *f);
unsigned char *reloc_seg(unsigned char *f, int len, unsigned char *rtab, file65 *fp, int undefwarn);
unsigned char *reloc_globals(unsigned char *, file65 *fp);

file65 file;
unsigned char cmp[] = { 1, 0, 'o', '6', '5' };

void usage(FILE *fp)
{
	fprintf(fp,
		"Usage: %s [OPTION]... [FILE]...\n"
		"Relocator for o65 object files\n"
		"\n"
		"  -b? addr   relocates segment '?' (i.e. 't' for text segment,\n"
		"               'd' for data, 'b' for bss and 'z' for zeropage) to the new\n"
		"               address `addr'\n"
		"  -o file    uses `file' as output file. Default is `a.o65'\n"
		"  -x?        extracts text `?' = `t' or data `?' = `d' segment from file\n",
		programname);
	fprintf(fp,
		"               instead of writing back the whole file\n"
		"  -X         extracts the file such that text and data\n"
		"               segments are chained, i.e. possibly relocating\n"
		"               the data segment to the end of the text segment\n" 
		"  --version  output version information and exit\n"
		"  --help     display this help and exit\n");
}

int main(int argc, char *argv[]) {
	int i = 1, mode, hlen;
	size_t n;
	FILE *fp;
	int tflag = 0, dflag = 0, bflag = 0, zflag = 0;
	int tbase = 0, dbase = 0, bbase = 0, zbase = 0;
	char *outfile = "a.o65";
	int extract = 0;

	if (argc <= 1) {
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
	    case 'o':
		if(argv[i][2]) outfile=argv[i]+2;
		else outfile=argv[++i];
		break;
	    case 'X':
		extract=3;
		break;
	    case 'b':
		switch(argv[i][2]) {
		case 't':
			tflag= 1;
			if(argv[i][3]) tbase = atoi(argv[i]+3);
			else tbase = atoi(argv[++i]);
			break;
		case 'd':
			dflag= 1;
			if(argv[i][3]) dbase = atoi(argv[i]+3);
			else dbase = atoi(argv[++i]);
			break;
		case 'b':
			bflag= 1;
			if(argv[i][3]) bbase = atoi(argv[i]+3);
			else bbase = atoi(argv[++i]);
			break;
		case 'z':
			zflag= 1;
			if(argv[i][3]) zbase = atoi(argv[i]+3);
			else zbase = atoi(argv[++i]);
			break;
		default:
			printf("Unknown segment type '%c' - ignored!\n", argv[i][2]);
			break;
		}
		break;
	    case 'x':		/* extract segment */
	        switch(argv[i][2]) {
		case 't':
			extract = 1;
			break;
		case 'd':
			extract = 2;
			break;
		case 'z':
		case 'b':
			printf("Cannot extract segment type '%c' - ignored!\n", argv[i][2]);
			break;
		default:
			printf("Unknown segment type '%c' - ignored!\n", argv[i][2]);
			break;
		}
		break;
	    default:
		fprintf(stderr,"reloc65: %s unknown option, use '-?' for help\n",argv[i]);
		break;
	    }
	  } else {
	    struct stat fs;
	    file.fname=argv[i];
	    stat(argv[i], &fs);
	    file.fsize=fs.st_size;
	    file.buf=malloc(file.fsize);
	    if(!file.buf) {
	      fprintf(stderr,"Oops, no more memory!\n");
	      exit(1);
	    }
	    printf("reloc65: read file %s -> %s\n",argv[i],outfile);
	    fp = fopen(argv[i],"rb");
	    if(fp) {
	      n = fread(file.buf, 1, file.fsize, fp);
	      fclose(fp);
	      if((n>=file.fsize) && (!memcmp(file.buf, cmp, 5))) {
		mode=file.buf[7]*256+file.buf[6];
		if(mode & 0x2000) {
	          fprintf(stderr,"reloc65: %s: 32 bit size not supported\n", argv[i]);
		} else
		if(mode & 0x4000) {
	          fprintf(stderr,"reloc65: %s: pagewise relocation not supported\n", argv[i]);
		} else {
		  hlen = BUF+read_options(file.buf+BUF);
		  
		  file.tbase = file.buf[ 9]*256+file.buf[ 8];
		  file.tlen  = file.buf[11]*256+file.buf[10];
		  file.tdiff = tflag? tbase - file.tbase : 0;
		  file.dbase = file.buf[13]*256+file.buf[12];
		  file.dlen  = file.buf[15]*256+file.buf[14];
		  if (extract == 3) {
		    if (dflag) {
	              fprintf(stderr,"reloc65: %s: Warning: data segment address ignored for -X option\n", argv[i]);
		    } 
		    dbase = file.tbase + file.tdiff + file.tlen;
		    file.ddiff = dbase - file.dbase;
		  } else {
		    file.ddiff = dflag? dbase - file.dbase : 0;
		  }
		  file.bbase = file.buf[17]*256+file.buf[16];
		  file.blen  = file.buf[19]*256+file.buf[18];
		  file.bdiff = bflag? bbase - file.bbase : 0;
		  file.zbase = file.buf[21]*256+file.buf[20];
		  file.zlen  = file.buf[23]*256+file.buf[21];
		  file.zdiff = zflag? zbase - file.zbase : 0;

		  file.segt  = file.buf + hlen;
		  file.segd  = file.segt + file.tlen;
		  file.utab  = file.segd + file.dlen;

		  file.rttab = file.utab + read_undef(file.utab);
		  file.rdtab = reloc_seg(file.segt, file.tlen, file.rttab, 
					&file, extract);
		  file.extab = reloc_seg(file.segd, file.dlen, file.rdtab, 
					&file, extract);

		  reloc_globals(file.extab, &file);

		  if(tflag) {
			file.buf[ 9]= (tbase>>8)&255;
			file.buf[ 8]= tbase & 255;
		  }
  		  if(dflag) {
			file.buf[13]= (dbase>>8)&255;
			file.buf[12]= dbase & 255;
		  }
		  if(bflag) {
			file.buf[17]= (bbase>>8)&255;
			file.buf[16]= bbase & 255;
		  }
		  if(zflag) {
			file.buf[21]= (zbase>>8)&255;
			file.buf[20]= zbase & 255;
		  }

		  fp = fopen(outfile, "wb");
		  if(fp) {
		    switch(extract) {
		    case 0:	/* whole file */
		    	fwrite(file.buf, 1, file.fsize, fp);
			break;
		    case 1:	/* text segment */
			fwrite(file.segt, 1, file.tlen, fp);
			break;
		    case 2:	/* data segment */
			fwrite(file.segd, 1, file.dlen, fp);
			break;
		    case 3:	/* text+data */
			fwrite(file.segt, 1, file.tlen, fp);
			fwrite(file.segd, 1, file.dlen, fp);
			break;
		    }
		    fclose(fp);
		  } else {
	            fprintf(stderr,"reloc65: write '%s': %s\n", 
						outfile, strerror(errno));
		  }
		}
	      } else {
	        fprintf(stderr,"reloc65: %s: not an o65 file!\n", argv[i]);
		if(file.buf[0]==1 && file.buf[1]==8 && file.buf[3]==8) {
		  printf("%s: C64 BASIC executable (start address $0801)?\n", argv[i]);
		} else
		if(file.buf[0]==1 && file.buf[1]==4 && file.buf[3]==4) {
		  printf("%s: CBM PET BASIC executable (start address $0401)?\n", argv[i]);
		}
	      }
	    } else {
	      fprintf(stderr,"reloc65: read '%s': %s\n", 
						argv[i], strerror(errno));
	    }
	  }
	  i++;
	}
	exit(0);
}


int read_options(unsigned char *buf) {
	int c, l=0;

	c=buf[0];
	while(c && c!=EOF) {
	  c&=255;
	  l+=c;
	  c=buf[l];
	}
	return ++l;
}

int read_undef(unsigned char *buf) {
	int n, l = 2;

	n = buf[0] + 256*buf[1];
	while(n){
	  n--;
	  while(buf[l] != 0) {
		l++;
	  }
	  l++;
	}
	return l;
}

#define	reldiff(s)	(((s)==2)?fp->tdiff:(((s)==3)?fp->ddiff:(((s)==4)?fp->bdiff:(((s)==5)?fp->zdiff:0))))

unsigned char *reloc_seg(unsigned char *buf, int len, unsigned char *rtab, 
			file65 *fp, int undefwarn) {
	int adr = -1;
	int type, seg, old, new;
/*printf("tdiff=%04x, ddiff=%04x, bdiff=%04x, zdiff=%04x\n",
		fp->tdiff, fp->ddiff, fp->bdiff, fp->zdiff);*/
	while(*rtab) {
	  if((*rtab & 255) == 255) {
	    adr += 254;
	    rtab++;
	  } else {
	    adr += *rtab & 255;
	    rtab++;
	    type = *rtab & 0xe0;
	    seg = *rtab & 0x07;
/*printf("reloc entry @ rtab=%p (offset=%d), adr=%04x, type=%02x, seg=%d\n",rtab-1, *(rtab-1), adr, type, seg);*/
	    rtab++;
	    switch(type) {
	    case 0x80:	/* WORD - two byte address */
		old = buf[adr] + 256*buf[adr+1];
		new = old + reldiff(seg);
		buf[adr] = new & 255;
		buf[adr+1] = (new>>8)&255;
		break;
	    case 0x40:	/* HIGH - high byte of an address */
		old = buf[adr]*256 + *rtab;
		new = old + reldiff(seg);
		buf[adr] = (new>>8)&255;
		*rtab = new & 255;
		rtab++;
		break;
	    case 0x20:	/* LOW - low byt of an address */
		old = buf[adr];
		new = old + reldiff(seg);
		buf[adr] = new & 255;
		break;
	    }
	    if(seg==0) {
		/* undefined segment entry */
		if (undefwarn) {
	          fprintf(stderr,"reloc65: %s: Warning: undefined relocation table entry not handled!\n", fp->fname);
		}
		rtab+=2;
	    }
	  }
	}
	if(adr > len) {
	  fprintf(stderr,"reloc65: %s: Warning: relocation table entries past segment end!\n",
		fp->fname);
	  fprintf(stderr, "reloc65: adr=%x len=%x\n", adr, len);
	}
	return ++rtab;
}

unsigned char *reloc_globals(unsigned char *buf, file65 *fp) {
	int n, old, new, seg;

	n = buf[0] + 256*buf[1];
	buf +=2;

	while(n) {
/*printf("relocating %s, ", buf);*/
	  while(*(buf++));
	  seg = *buf;
	  old = buf[1] + 256*buf[2];
	  new = old + reldiff(seg);
/*printf("old=%04x, seg=%d, rel=%04x, new=%04x\n", old, seg, reldiff(seg), new);*/
	  buf[1] = new & 255;
	  buf[2] = (new>>8) & 255;
	  buf +=3;
	  n--;
	}
	return buf;
}

