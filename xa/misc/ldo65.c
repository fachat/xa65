/* ldo65 -- A part of xa65 - 65xx/65816 cross-assembler and utility suite
 * o65 relocatable object file linker
 *
 * A part of the xa65 - 65xx/65816 cross-assembler and utility suite
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "version.h"

#define	BUF	(9*2+8)		/* 16 bit header */

#define programname	"ldo65"
#define progversion	"v0.1.1"
#define author		"Written by Andre Fachat"
#define copyright	"Copyright (C) 1997-2002 Andre Fachat. Formerly ld65."

typedef struct {
	char	*name;
	int	len;
} undefs;

/* file information */
typedef struct {
	char		*fname;		/* file name */
	size_t		fsize;		/* length of file */
	unsigned char	*buf;		/* file content */

	int		tbase;		/* header: text base */
	int		tlen;		/* text length */
	int		dbase;		/* data base */
	int		dlen;		/* data length */
	int		bbase;		/* bss base */
	int		blen;		/* bss length */
	int		zbase;		/* zero base */
	int		zlen;		/* zero length */

	int		tdiff;		/* text segment relocation diff */
	int		ddiff;		/* data segment relocation diff */
	int		bdiff;		/* bss  segment relocation diff */
	int		zdiff;		/* zero segment relocation diff */

	int		tpos;		/* position of text segment in file */
	int		dpos;		/* position of data segment in file */
	int		upos;		/* position of undef'd list in file */
	int		trpos;		/* position of text reloc tab in file */
	int		drpos;		/* position of data reloc tab in file */
	int		gpos;		/* position of globals list in file */

	int		lasttreloc;
	int		lastdreloc;

	int		nundef;		/* number of undefined labels */
	undefs 		*ud;		/* undefined labels list NULL if none */
} file65;

/* globally defined lables are stored in this struct */
typedef struct {
	char 	*name;
	int	len;		/* length of labelname */
	int	fl;		/* 0=ok, 1=multiply defined */
	int	val;		/* address value */
	int	seg;		/* segment */
	file65	*file;		/* in which file is it? */
} glob;

file65 *load_file(char *fname);

int read_options(unsigned char *f);
int read_undef(unsigned char *f, file65 *fp);
int len_reloc_seg(unsigned char *buf, int ri);
int reloc_seg(unsigned char *buf, int adr, int ri, int *lreloc, file65 *fp);
unsigned char *reloc_globals(unsigned char *, file65 *fp);
int read_globals(file65 *file);
int write_options(FILE *fp, file65 *file);
int write_reloc(file65 *fp[], int nfp, FILE *f);
int write_globals(FILE *fp);

file65 file;
unsigned char cmp[] = { 1, 0, 'o', '6', '5' };
unsigned char hdr[26] = { 1, 0, 'o', '6', '5', 0 };

void usage(FILE *fp)
{
	fprintf(fp,
		"Usage: %s [OPTION]... [FILE]...\n"
		"Linker for o65 object files\n"
		"\n"
		"  -b? addr   relocates segment `?' (i.e. `t' for text segment,\n"
		"               `d' for data, `b' for bss, and `z' for zeropage) to the new\n"
		"               address `addr'\n"
		"  -o file    uses `file' as output file. Default is `a.o65'\n"
		"  -G         suppress writing of globals\n"
		"  --version  output version information and exit\n"
		"  --help     display this help and exit\n",
		programname);
}

int main(int argc, char *argv[]) {
	int noglob=0;
	int i = 1;
	int tbase = 0x0400, dbase = 0x1000, bbase = 0x4000, zbase = 0x0002;
	int ttlen, tdlen, tblen, tzlen;
	char *outfile = "a.o65";
	int j, jm;
	file65 *file, **fp = NULL;
	FILE *fd;

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

	/* read options */
	while(i<argc && argv[i][0]=='-') {
	    /* process options */
	    switch(argv[i][1]) {
	    case 'G':
		noglob=1;
		break;
	    case 'o':
		if(argv[i][2]) outfile=argv[i]+2;
		else outfile=argv[++i];
		break;
	    case 'b':
		switch(argv[i][2]) {
		case 't':
			if(argv[i][3]) tbase = atoi(argv[i]+3);
			else tbase = atoi(argv[++i]);
			break;
		case 'd':
			if(argv[i][3]) dbase = atoi(argv[i]+3);
			else dbase = atoi(argv[++i]);
			break;
		case 'b':
			if(argv[i][3]) bbase = atoi(argv[i]+3);
			else bbase = atoi(argv[++i]);
			break;
		case 'z':
			if(argv[i][3]) zbase = atoi(argv[i]+3);
			else zbase = atoi(argv[++i]);
			break;
		default:
			printf("Unknown segment type '%c' - ignored!\n", argv[i][2]);
			break;
		}
		break;
	    default:
		fprintf(stderr,"file65: %s unknown option, use '-?' for help\n",argv[i]);
		break;
	    }
	    i++;
	}
	/* each file is loaded first */
	j=0; jm=0; fp=NULL;
	while(i<argc) {
	  file65 *f;
	  f = load_file(argv[i]);
	  if(f) {
	    if(j>=jm) fp=realloc(fp, (jm=(jm?jm*2:10))*sizeof(file65*));
	    if(!fp) { fprintf(stderr,"Oops, no more memory\n"); exit(1); }
	    fp[j++] = f;
	  }
	  i++;
	}

	/* now [tdbz]base holds new segment base address */
	/* set total length to zero */
	ttlen = tdlen = tblen = tzlen = 0;

	/* find new addresses for the files and read globals */
	for(i=0;i<j;i++) {
	  file = fp[i];

	  /* compute relocation differences */
	  file->tdiff =  ((tbase + ttlen) - file->tbase);
	  file->ddiff =  ((dbase + tdlen) - file->dbase);
	  file->bdiff =  ((bbase + tblen) - file->bbase);
	  file->zdiff =  ((zbase + tzlen) - file->zbase);
/*printf("tbase=%04x, file->tbase=%04x, ttlen=%04x -> tdiff=%04x\n",
		tbase, file->tbase, ttlen, file->tdiff);*/

	  /* update globals (for result file) */
	  ttlen += file->tlen;
	  tdlen += file->dlen;
	  tblen += file->blen;
	  tzlen += file->zlen;

	  read_globals(file);
	}

	for(i=0;i<j;i++) {
	  file = fp[i];

	  reloc_seg(file->buf, 
			file->tpos, 
			file->trpos,
			&(file->lasttreloc),
			file);
	  reloc_seg(file->buf,
			file->dpos,
			file->drpos,
			&(file->lastdreloc),
			file);
	  reloc_globals(file->buf+file->gpos, file);

	  file->tbase += file->tdiff;
	  file->dbase += file->ddiff;
	  file->bbase += file->bdiff;
	  file->zbase += file->zdiff;

	  file->lasttreloc += file->tbase - file->tpos;
	  file->lastdreloc += file->dbase - file->dpos;

	}

	hdr[ 6] = 0;           hdr[ 7] = 0;
	hdr[ 8] = tbase & 255; hdr[ 9] = (tbase>>8) & 255;
	hdr[10] = ttlen & 255; hdr[11] = (ttlen >>8)& 255;
	hdr[12] = dbase & 255; hdr[13] = (dbase>>8) & 255;
	hdr[14] = tdlen & 255; hdr[15] = (tdlen >>8)& 255;
	hdr[16] = bbase & 255; hdr[17] = (bbase>>8) & 255;
	hdr[18] = tblen & 255; hdr[19] = (tblen >>8)& 255;
	hdr[20] = zbase & 255; hdr[21] = (zbase>>8) & 255;
	hdr[22] = tzlen & 255; hdr[23] = (tzlen >>8)& 255;
	hdr[24] = 0;           hdr[25] = 0;

	fd = fopen(outfile, "wb");
	if(!fd) {
	  fprintf(stderr,"Couldn't open output file %s (%s)\n",
		outfile, strerror(errno));
	  exit(2);
	}
	fwrite(hdr, 1, 26, fd);
	/* this writes _all_ options from _all_files! */
	for(i=0;i<j;i++) {
	  write_options(fd, fp[i]);
	}
	fputc(0,fd);
	/* write text segment */
	for(i=0;i<j;i++) {
	  fwrite(fp[i]->buf + fp[i]->tpos, 1, fp[i]->tlen, fd);
	}
	/* write data segment */
	for(i=0;i<j;i++) {
	  fwrite(fp[i]->buf + fp[i]->dpos, 1, fp[i]->dlen, fd);
	}
	write_reloc(fp, j, fd);
	if(!noglob) { 
	  write_globals(fd);
	} else {
	  fputc(0,fd);
	  fputc(0,fd);
	}

	fclose(fd);
	return 0;
}

/***************************************************************************/

int write_options(FILE *fp, file65 *file) {
	return fwrite(file->buf+BUF, 1, file->tpos-BUF-1, fp);
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

int read_undef(unsigned char *buf, file65 *file) {
	int i, n, l = 2, ll;

	n = buf[0] + 256*buf[1];

	file->nundef = n;

	if (n == 0) {
		file->ud = NULL;
	} else {	
		file->ud = malloc(n*sizeof(undefs));
		if(!file->ud) {
		  fprintf(stderr,"Oops, no more memory\n");
		  exit(1);
		}
		i=0;
		while(i<n){
		  file->ud[i].name = (char*) buf+l;
		  ll=l;
		  while(buf[l++]);
		  file->ud[i].len = l-ll-1;
/*printf("read undef '%s'(%p), len=%d, ll=%d, l=%d, buf[l]=%d\n",
		file->ud[i].name, file->ud[i].name, file->ud[i].len,ll,l,buf[l]);*/
		  i++;
		}
	}
	return l;
}

/* compute and return the length of the relocation table */
int len_reloc_seg(unsigned char *buf, int ri) {
	int type, seg;

	while(buf[ri]) {
	  if((buf[ri] & 255) == 255) {
	    ri++;
	  } else {
	    ri++;
	    type = buf[ri] & 0xe0;
	    seg = buf[ri] & 0x07;
/*printf("reloc entry @ rtab=%p (offset=%d), adr=%04x, type=%02x, seg=%d\n",buf+ri-1, *(buf+ri-1), adr, type, seg);*/
	    ri++;
	    switch(type) {
	    case 0x80:
		break;
	    case 0x40:
		ri++;
		break;
	    case 0x20:
		break;
	    }
	    if(seg==0) ri+=2;
	  }
	}
	return ++ri;
}

#define	reldiff(s) (((s)==2)?fp->tdiff:(((s)==3)?fp->ddiff:(((s)==4)?fp->bdiff:(((s)==5)?fp->zdiff:0))))

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

/***************************************************************************/

file65 *load_file(char *fname) {
	file65 *file;
	struct stat fs;
	FILE *fp;
	int mode, hlen;
	size_t n;

	file=malloc(sizeof(file65));
	if(!file) {
	  fprintf(stderr,"Oops, not enough memory!\n");
	  exit(1);
	}

/*printf("load_file(%s)\n",fname);*/

	file->fname=fname;
	stat(fname, &fs);
	file->fsize=fs.st_size;
	file->buf=malloc(file->fsize);
	if(!file->buf) {
	  fprintf(stderr,"Oops, no more memory!\n");
	  exit(1);
	}

	fp = fopen(fname,"rb");
    	if(fp) {
	  n = fread(file->buf, 1, file->fsize, fp);
	  fclose(fp);
	  if((n>=file->fsize) && (!memcmp(file->buf, cmp, 5))) {
	    mode=file->buf[7]*256+file->buf[6];
	    if(mode & 0x2000) {
	      fprintf(stderr,"file65: %s: 32 bit size not supported\n", fname);
	      free(file->buf); free(file); file=NULL;
	    } else
	    if(mode & 0x4000) {
	      fprintf(stderr,"file65: %s: pagewise relocation not supported\n", 
									fname);
	      free(file->buf); free(file); file=NULL;
	    } else {
	      hlen = BUF+read_options(file->buf+BUF);
		  
	      file->tbase = file->buf[ 9]*256+file->buf[ 8];
	      file->tlen  = file->buf[11]*256+file->buf[10];
	      file->dbase = file->buf[13]*256+file->buf[12];
	      file->dlen  = file->buf[15]*256+file->buf[14];
	      file->bbase = file->buf[17]*256+file->buf[16];
	      file->blen  = file->buf[19]*256+file->buf[18];
	      file->zbase = file->buf[21]*256+file->buf[20];
	      file->zlen  = file->buf[23]*256+file->buf[21];

	      file->tpos = hlen;
	      file->dpos = hlen + file->tlen;
	      file->upos = file->dpos + file->dlen;
	      file->trpos= file->upos + read_undef(file->buf+file->upos, file);
	      file->drpos= len_reloc_seg(file->buf, file->trpos);
	      file->gpos = len_reloc_seg(file->buf, file->drpos);
	    }
	  } else
	    fprintf(stderr,"file65: %s: %s\n", fname, strerror(errno));
	} else
	  fprintf(stderr,"file65: %s: %s\n", fname, strerror(errno));

	return file;
}

/***************************************************************************/

glob *gp = NULL;
int gm=0;
int g=0;

int write_reloc(file65 *fp[], int nfp, FILE *f) {
	int tpc, pc, i;
	unsigned char *p;
	int low = 0, seg, typ, lab;

	/* no undefined labels ? TODO */
	fputc(0,f);
	fputc(0,f);

	tpc = fp[0]->tbase-1;

	for(i=0;i<nfp;i++) {
	  pc = fp[i]->tbase-1;
	  p = fp[i]->buf + fp[i]->trpos;

	  while(*p) {
	    while((*p)==255) { pc+=254; p++; }
	    pc+=*(p++);
	    seg=(*p)&7;
	    typ=(*p)&0xe0;
	    if(typ==0x40) low=*(++p);
	    p++;
	    if(seg==0) {
	      lab=p[0]+256*p[1];
	      seg=gp[lab].seg;
	      p+=2;
	    }
	    if(seg>1) {
	      while(pc-tpc>254) {
		fputc(255,f);
		tpc+=254;
	      }
	      fputc(pc-tpc, f);
	      tpc=pc;
	      fputc(typ | seg, f);
	      if(typ==0x40) {
		fputc(low,f);
	      }
	    }
	  }
	}
	fputc(0,f);

	tpc = fp[0]->dbase-1;

	for(i=0;i<nfp;i++) {
	  pc = fp[i]->dbase-1;
	  p = fp[i]->buf + fp[i]->drpos;

	  while(*p) {
	    while((*p)==255) { pc+=254; p++; }
	    pc+=*(p++);
	    seg=(*p)&7;
	    typ=(*p)&0xe0;
	    if(typ==0x40) low=*(++p);
	    p++;
	    if(seg==0) {
	      lab=p[0]+256*p[1];
	      seg=gp[lab].seg;
	      p+=2;
	    }
	    if(seg>1) {
	      while(pc-tpc>254) {
		fputc(255,f);
		tpc+=254;
	      }
	      fputc(pc-tpc, f);
	      tpc=pc;
	      fputc(typ | seg, f);
	      if(typ==0x40) {
		fputc(low,f);
	      }
	    }
	  }
	}
	fputc(0,f);

	return 0;
}

int write_globals(FILE *fp) {
	int i;

	fputc(g&255, fp);
	fputc((g>>8)&255, fp);

	for(i=0;i<g;i++) {
	  fprintf(fp,"%s%c%c%c%c",gp[i].name,0,gp[i].seg, 
			gp[i].val & 255, (gp[i].val>>8)&255);
	}
	return 0;
}

int read_globals(file65 *fp) {
	int i, l, n, old, new, seg, ll;
	char *name;
	unsigned char *buf = fp->buf + fp->gpos;

	n = buf[0] + 256*buf[1];
	buf +=2;

	while(n) {
/*printf("reading %s, ", buf);*/
	  name = (char*) buf;
	  l=0;
	  while(buf[l++]);
	  buf+=l;
	  ll=l-1;
	  seg = *buf;
	  old = buf[1] + 256*buf[2];
	  new = old + reldiff(seg);
/*printf("old=%04x, seg=%d, rel=%04x, new=%04x\n", old, seg, reldiff(seg), new);*/

	  /* multiply defined? */
	  for(i=0;i<g;i++) {
	    if(ll==gp[i].len && !strcmp(name, gp[i].name)) {
	      fprintf(stderr,"Warning: label '%s' multiply defined (%s and %s)\n",
			name, fp->fname, gp[i].file->fname);
	      gp[i].fl = 1;
	      break;
	    }
	  }
	  /* not already defined */
	  if(i>=g) {
	    if(g>=gm) {
	      gp = realloc(gp, (gm=(gm?2*gm:40))*sizeof(glob));
	      if(!gp) {
	        fprintf(stderr,"Oops, no more memory\n");
	        exit(1);
	      }
	    }
	    if(g>=0x10000) {
	      fprintf(stderr,"Outch, maximum number of labels (65536) exceeded!\n");
	      exit(3);
	    }
	    gp[g].name = name;
	    gp[g].len = ll;
	    gp[g].seg = seg;
	    gp[g].val = new;
	    gp[g].fl = 0;
	    gp[g].file = fp;
/*printf("set label '%s' (l=%d, seg=%d, val=%04x)\n", gp[g].name,
					gp[g].len, gp[g].seg, gp[g].val);*/
	    g++;
	  }

	  buf +=3;
	  n--;
	}
	return 0;
}

int find_global(unsigned char *bp, file65 *fp, int *seg) {
	int i,l;
	char *n;
	int nl = bp[0]+256*bp[1];

	l=fp->ud[nl].len;
	n=fp->ud[nl].name;
/*printf("find_global(%s (len=%d))\n",n,l);*/

	for(i=0;i<g;i++) {
	  if(gp[i].len == l && !strcmp(gp[i].name, n)) {
	    *seg = gp[i].seg;
	    bp[0] = i & 255; bp[1] = (i>>8) & 255;
/*printf("return gp[%d]=%s (len=%d), val=%04x\n",i,gp[i].name,gp[i].len,gp[i].val);*/
	    return gp[i].val;
	  }
	}
	fprintf(stderr,"Warning: undefined label '%s' in file %s\n",
		 n, fp->fname);
	return 0;
}

int reloc_seg(unsigned char *buf, int pos, int ri, int *lreloc, file65 *fp) {
	int type, seg, old, new;

	/* 
	   pos = position of segment in *buf
	   ri  = position of relocation table in *buf
	*/
	pos--;
/*printf("reloc_seg: adr=%04x, tdiff=%04x, ddiff=%04x, bdiff=%04x, zdiff=%04x\n", pos, fp->tdiff, fp->ddiff, fp->bdiff, fp->zdiff); */
	while(buf[ri]) {
	  if((buf[ri] & 255) == 255) {
	    pos += 254;
	    ri++;
	  } else {
	    pos += buf[ri] & 255;
	    ri++;
	    type = buf[ri] & 0xe0;
	    seg = buf[ri] & 0x07;
/*printf("reloc entry @ ri=%04x, pos=%04x, type=%02x, seg=%d\n",ri, pos, type, seg);*/
	    ri++;
	    switch(type) {
	    case 0x80:
		old = buf[pos] + 256*buf[pos+1];
		if(seg) {
			new = old + reldiff(seg);
		} else {
			new = old + find_global(buf+ri, fp, &seg);
			ri += 2;	/* account for label number */
		}
/*printf("old=%04x, new=%04x\n",old,new);*/
		buf[pos] = new & 255;
		buf[pos+1] = (new>>8)&255;
		break;
	    case 0x40:
		old = buf[pos]*256 + buf[ri];
		if(seg) {
			new = old + reldiff(seg);
		} else {
			new = old + find_global(buf+ri+1, fp, &seg);
			ri += 2;	/* account for label number */
		}
		buf[pos] = (new>>8)&255;
		buf[ri] = new & 255;
		ri++;
		break;
	    case 0x20:
		old = buf[pos];
		if(seg) {
			new = old + reldiff(seg);
		} else {
			new = old + find_global(buf+ri, fp, &seg);
			ri += 2;	/* account for label number */
		}
		buf[pos] = new & 255;
		break;
	    }
	  }
	}
	*lreloc = pos;
	return ++ri;
}
