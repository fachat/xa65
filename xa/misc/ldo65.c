/* ldo65 -- A part of xa65 - 65xx/65816 cross-assembler and utility suite
 * o65 relocatable object file linker
 *
 * A part of the xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1997-2023 André Fachat (fachat@web.de)
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
#define progversion	"v0.2.0"
#define author		"Written by Andre Fachat"
#define copyright	"Copyright (C) 1997-2023 Andre Fachat. Formerly ld65."

#undef	DEBUG

/*

The process of linking works as follows:

1. Every file is loaded in turn via load_file()
2. Calculate new base addresses per segment
3. Merge all globals from all files into a single table, checking for duplicates
4. Resolve undefined labels, and merge remaining into global list

*/

typedef struct {
	char	*name;
	int	len;
	int	newidx;		/* index in new global undef table (for reloc) */
	int	resolved;	/* index in current global label table after resolve (-1 is not found) */
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
int write_undef(FILE *f, file65 *fp);
int len_reloc_seg(unsigned char *buf, int ri);
int reloc_seg(unsigned char *buf, int pos, int addr, int rdiff, int ri, unsigned char *obuf, int *lastaddrp, int *rop, file65 *fp);
unsigned char *reloc_globals(unsigned char *, file65 *fp);
int read_globals(file65 *file);
int write_options(FILE *fp, file65 *file);
int write_reloc(file65 *fp[], int nfp, FILE *f);
int write_globals(FILE *fp);
int find_global(unsigned char *name);
int resolve_undef(file65 *file, int *remains);

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
		"  -U         accept any undef'd labels after linking\n"
//		"  -L<name>   accept specific given undef'd labels after linking\n"
		"  --version  output version information and exit\n"
		"  --help     display this help and exit\n",
		programname);
}

int main(int argc, char *argv[]) {
	int noglob=0;
	int undefok=0;
	int i = 1;
	int tbase = 0x0400, dbase = 0x1000, bbase = 0x4000, zbase = 0x0002;
	int ttlen, tdlen, tblen, tzlen;
	char *outfile = "a.o65";
	int j, jm;
	file65 *file, **fp = NULL;
	FILE *fd;
	int nundef = 0;	// counter/index in list of remaining undef'd labels

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
	    case 'U':
		undefok=1;
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

	// -------------------------------------------------------------------------
	// step 1 - load files

	/* each file is loaded first */
	j=0; jm=0; fp=NULL;
	while(i<argc) {
	  file65 *f;
	  f = load_file(argv[i]);
	  if(f) {
	    if(j>=jm) fp=realloc(fp, (jm=(jm?jm*2:10))*sizeof(file65*));
	    if(!fp) { fprintf(stderr,"Oops, no more memory\n"); exit(1); }
	    fp[j++] = f;
	  } else {
	    exit(1);
	  } 
	  i++;
	}

	// -------------------------------------------------------------------------
	// step 2 - calculate new segment base addresses per file, by 
	//          concatenating the segments per type

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
	}

	// -------------------------------------------------------------------------
	// step 3 - merge globals from all files into single table
	//

	for(i=0;i<j;i++) {
	  file = fp[i];
	  // merge globals into single table
	  read_globals(file);
	}


	// -------------------------------------------------------------------------
	// step 4 - for each file, resolve undefined lables, storing replacement info
	//          in the ud label table; merge remaining undefined labels into global
	//          undef list

	for(i=0;i<j;i++) {
	  file = fp[i];
	  // merge globals into single table
	  resolve_undef(file, &nundef);
	}

	// -------------------------------------------------------------------------
	// step 5 - relocate each text and data segment, replacing the resolved 
	//          undefined labels and re-numbering the remaining ones

	// reloc globals first, so reloc_seg has current info for resolved undef'd labels

	int routtlen = 1;	// end-of-table byte
	int routdlen = 1;	// end-of-table byte

	for(i=0;i<j;i++) {
	  file = fp[i];

	  routtlen += (file->drpos - file->trpos);
	  routdlen += (file->gpos - file->drpos);

	  reloc_globals(file->buf+file->gpos, file);
	}

	// prep global reloc tables
	unsigned char *treloc = malloc(routtlen);
	unsigned char *dreloc = malloc(routdlen);

#ifdef DEBUG
	printf("prep'd text reloc table at %p (%d bytes)\n", treloc, routtlen);
	printf("prep'd data reloc table at %p (%d bytes)\n", dreloc, routdlen);
#endif
	int tro = 0;
	int dro = 0;

	// segment position of last relocation entry to compute offsets across files
	int lasttaddr = tbase - 1;
	int lastdaddr = dbase - 1;

	for(i=0;i<j;i++) {
	  file = fp[i];

	  reloc_seg(file->buf,		// input buffer
			file->tpos,	// position of segment in input buffer 
			file->tbase, 	// segment base address
			file->tdiff,	// reloc difference
			file->trpos,	// position of reloc table in input
			treloc,		// output reloc buffer
			&lasttaddr,	// last relocated target address
			&tro,		// pointer in output reloc bufer
			file);
#if 0
	  reloc_seg(file->buf,
			file->dpos,
			file->dbase,
			file->ddiff,
			file->drpos,
			treloc,
			&lastdaddr,
			&dro,
			file);
#endif
	  // change file information to relocated values
	  file->tbase += file->tdiff;
	  file->dbase += file->ddiff;
	  file->bbase += file->bdiff;
	  file->zbase += file->zdiff;
	}

	// finalize global reloc table
	treloc[tro++] = 0;
	dreloc[dro++] = 0;

	// -------------------------------------------------------------------------
	// step 7 - write out the resulting o65 file
	//

	if (nundef > 0) {
		if (!undefok) {
			fprintf(stderr, "%d Undefined labels remain - aborting\n", nundef);
			exit(1);
		}
	}

	// prepare header
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

	// open file
	fd = fopen(outfile, "wb");
	if(!fd) {
	  fprintf(stderr,"Couldn't open output file %s (%s)\n",
		outfile, strerror(errno));
	  exit(2);
	}

	// write header
	fwrite(hdr, 1, 26, fd);

	// write options - this writes _all_ options from _all_files! 
	for(i=0;i<j;i++) {
	  write_options(fd, fp[i]);
	}
	fputc(0,fd);

	// write text segment 
	for(i=0;i<j;i++) {
	  fwrite(fp[i]->buf + fp[i]->tpos, 1, fp[i]->tlen, fd);
	}

	// write data segment 
	for(i=0;i<j;i++) {
	  fwrite(fp[i]->buf + fp[i]->dpos, 1, fp[i]->dlen, fd);
	}

	// write list of undefined labels
	fputc(nundef & 0xff,fd);
	fputc((nundef >> 8) & 0xff,fd);
	if (nundef > 0) {
		for(i=0;i<j;i++) {
			write_undef(fd, fp[i]);
		}
	}

	// write relocation tables
	fwrite(treloc, tro, 1, fd);
	fwrite(dreloc, dro, 1, fd);

	// write globals
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

/***************************************************************************/

int read_undef(unsigned char *buf, file65 *file) {
	int bufp;	// pointer in input buffer
	int startp;	// pointer to start of label name
	int nlabels;	// number of labels in file
	undefs *current = NULL;
	int i;

	bufp = 0;
	nlabels = buf[bufp] + 256*buf[bufp+1];
	bufp += 2;

	file->nundef = nlabels;

	if (nlabels == 0) {
		file->ud = NULL;
	} else {	
		file->ud = malloc(nlabels*sizeof(undefs));
		if(!file->ud) {
		  fprintf(stderr,"Oops, no more memory\n");
		  exit(1);
		}
		i=0;
		while(i<nlabels){
		  // find length of label name
		  startp = bufp;
		  while(buf[bufp++]);
		  // store label info
		  current = &file->ud[i];
		  current->name = (char*) buf+startp;
		  current->len = bufp-startp-1;
		  current->resolved = -1;
/*printf("read undef '%s'(%p), len=%d, ll=%d, l=%d, buf[l]=%d\n",
		file->ud[i].name, file->ud[i].name, file->ud[i].len,ll,l,buf[l]);*/
		  i++;
		}
	}
	return bufp;
}

int resolve_undef(file65 *file, int *remains) {

	int nlabels = file->nundef;
#ifdef DEBUG
printf("resolved undef file %s (%d undef'd)\n", file->fname, nlabels);
#endif
	if (nlabels == 0) {
		return 0;
	}
	undefs *current = file->ud;

	for (int i = 0; i < nlabels; i++) {
		// store pointer to global in label info
		// if NULL is returned, is not resolved
		current->resolved = find_global(current->name);
#ifdef DEBUG
printf("resolved undef label %s to: resolved=%d, newidx=%d\n", current->name, current->resolved, *remains);
#endif
		if (current->resolved == -1) {
			// keep in global undef list
			current->newidx = *remains;
			*remains += 1;
		}
		current++;
	}
	return 0;
}


int write_undef(FILE *f, file65 *fp) {

	for (int i = 0; i < fp->nundef; i++) {
		undefs *current = &fp->ud[i];

		if (current->resolved == -1) {
			// only write unresolved entries
			fprintf(f, "%s%c", current->name, 0);
		}
	}
}


/***************************************************************************/

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
	  } else {
	    fprintf(stderr,"Error: %s: not an o65 file\n", fname);
	    return NULL;
	  }
	} else {
	  fprintf(stderr,"file65: %s: %s\n", fname, strerror(errno));
	  return NULL;
	}
	return file;
}

/***************************************************************************/

// global list of all global labels
glob *gp = NULL;
// number of global labels
int g=0;
// number of globals for which memory is already allocated
int gm=0;


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
#ifdef DEBUG
printf("set global label '%s' (l=%d, seg=%d, val=%04x)\n", gp[g].name,
					gp[g].len, gp[g].seg, gp[g].val);
#endif
	    g++;
	  }

	  buf +=3;
	  n--;
	}
	return 0;
}

int find_global(unsigned char *name) {

	for (int i = 0; i < g; i++) {

		if (!strcmp(gp[i].name, name)) {
			// found
			return i;
		}
	}
	return -1;
}

// searches for a global label in a file by name.
// returns the value of a found global value
int find_file_global(unsigned char *bp, file65 *fp, int *seg) {
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

/***************************************************************************/

#define	forwardpos()	\
	while(addr-lastaddr>254){obuf[ro++]=255;lastaddr+=254;}obuf[ro++]=addr-lastaddr;lastaddr=addr

int reloc_seg(unsigned char *buf, int pos, int addr, int rdiff, int ri,
		unsigned char *obuf, int *lastaddrp, int *rop, file65 *fp) {
	int type, seg, old, new, ro, lastaddr, diff;
	int base;

	/* 
	   pos = address of current position
	   ri  = position of relocation table in *buf for reading the reloc entries
 	   ro(p)  = position of relocation table entry for writing the modified entries
	*/
	base = addr;
	addr--;
	ro = *rop;
	lastaddr = *lastaddrp - rdiff;

#ifdef DEBUG
printf("reloc_seg: %s: addr=%04x, pos=%04x, lastaddr=%04x (%04x - %04x)\n", 
		fp->fname, addr, pos, lastaddr, *lastaddrp, rdiff); 
#endif

	while(buf[ri]) {
	  // still reloc entry
	  if((buf[ri] & 255) == 255) {
	    addr += 254;
	    ri++;
	  } else {
	    addr += buf[ri] & 255;
	    type = buf[ri+1] & 0xe0;
	    seg = buf[ri+1] & 0x07;
#ifdef DEBUG
printf("reloc entry @ ri=%04x, pos=%04x, type=%02x, seg=%d, offset=%d, reldiff=%04x\n",
		ri, pos, type, seg, addr-lastaddr, reldiff(seg));
#endif
	    switch(type) {
	    case 0x80:
		// address (word) relocation
		old = buf[addr-base+pos] + 256*buf[addr-base+pos+1];
		if(seg) {
			diff = reldiff(seg);
			ri++;			// skip position byte
			forwardpos();		// re-write position offset
			obuf[ro++] = buf[ri++];	// relocation byte ($8x for segments text, data, bss, zp)
		} else {
			// undefined
			undefs *u = &fp->ud[buf[ri+2]+256*buf[ri+3]];
#ifdef DEBUG
printf("found undef'd label %s, resolved=%d, newidx=%d, (ri=%d, ro=%d)\n", u->name, u->resolved, u->newidx, ri, ro);
#endif
			if (u->resolved == -1) {
				// not resolved
				diff = 0;
				ri++;			// skip position byte
				forwardpos();		// re-write position offset
				obuf[ro++] = buf[ri++];	// relocation byte ($8x for segments text, data, bss, zp)
				obuf[ro++] = u->newidx & 0xff;	// output label number lo/hi
				obuf[ro++] = (u->newidx >> 8) & 0xff;
				ri += 2;	// acount for label number in input
			} else {
				// resolved from global list
				glob *gl = &gp[u->resolved];
				diff = gl->val;
				seg = gl->seg;
				if (seg != 1) {
					// not an absolute value
					forwardpos();		// re-write position offset
					obuf[ro++] = 0x80 | seg;// relocation byte for new segment	
				} else {
					// absolute value - do not write a new relocation entry
				}
				ri += 4;	// account for position, segment byte, label number in reloc table 
			}
		}
		new = old + diff;
/*printf("old=%04x, new=%04x\n",old,new);*/
		buf[addr-base+pos] = new & 255;
		buf[addr-base+pos+1] = (new>>8)&255;
		break;
	    case 0x40:
		// high byte relocation
		if(seg) {
			old = buf[addr-base+pos]*256 + buf[ri+2];
			diff = reldiff(seg);
			forwardpos();	// re-write position offset
			obuf[ro++] = buf[ri+1];	// relocation byte ($4x for segments text, data, bss, zp)
			obuf[ro++] = (old + diff) & 255;
			ri += 3;	// skip position, segment, and low byte
		} else {
			old = buf[addr-base+pos]*256 + buf[ri+4];
			// undefined
			undefs *u = &fp->ud[buf[ri+2]+256*buf[ri+3]];
			if (u->resolved == -1) {
				// not resolved
				diff = 0;
				forwardpos();		// re-write position offset
				obuf[ro++] = buf[ri+1];	// relocation byte ($8x for segments text, data, bss, zp)
				obuf[ro++] = u->newidx & 0xff;	// output label number lo/hi
				obuf[ro++] = (u->newidx >> 8) & 0xff;
				obuf[ro++] = buf[ri+4];	// low byte for relocation
			} else {
				// resolved from global list
				glob *gl = &gp[u->resolved];
				diff = gl->val;
				seg = gl->seg;
				if (seg != 1) {
					// not an absolute value
					forwardpos();		// re-write position offset
					obuf[ro++] = 0x40 | seg;	// relocation byte for new segment
					obuf[ro++] = (old + diff) & 0xff;	// low byte for relocation
				} else {
					// absolute value - do not write a new relocation entry
				}
			}
			ri += 5; // account for position, segment byte, label number in reloc table, low byte 
		}
		new = old + diff;
		buf[addr-base+pos] = (new>>8)&255;
		break;
	    case 0x20:
		// low byte relocation
		old = buf[addr-base+pos];
		diff = 0;
		if(seg) {
			diff = reldiff(seg);
			forwardpos();
			obuf[ro++] = buf[ri+1];	// relocation byte ($4x for segments text, data, bss, zp)
			ri += 2;	// account for position & segment
		} else {
			// undefined
			undefs *u = &fp->ud[buf[ri+2]+256*buf[ri+3]];
			if (u->resolved == -1) {
				// not resolved
				diff = 0;
				forwardpos();		// re-write position offset
				obuf[ro++] = buf[ri+1];	// relocation byte ($8x for segments text, data, bss, zp)
				obuf[ro++] = u->newidx & 0xff;	// output label number lo/hi
				obuf[ro++] = (u->newidx >> 8) & 0xff;
			} else {
				// resolved from global list
				glob *gl = &gp[u->resolved];
				diff = gl->val;
				seg = gl->seg;
				if (seg != 1) {
					// not an absolute value
					forwardpos();		// re-write position offset
					obuf[ro++] = 0x20 | seg;	// relocation byte for new segment
				} else {
					// absolute value - do not write a new relocation entry
				}
			}
			ri += 4;// account for position, segment byte, label number in reloc table
		}
		new = old + diff;
		if (((diff & 0xff) + (old & 0xff)) > 0xff) {
			fprintf(stderr,"Warning: overflow in byte relocation at %04x in file %s\n",
				pos, fp->fname);
		}
		buf[addr-base+pos] = new & 255;
		break;
	    }
	  }
	}

	*lastaddrp = lastaddr + rdiff;
 	*rop = ro;
#ifdef DEBUG
	printf(" --> lastaddr=%04x (%04x - %04x), rop=%d\n", lastaddr, *lastaddrp, rdiff, ro);
#endif
	return ++ri;
}



