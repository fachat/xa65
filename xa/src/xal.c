/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 Andr� Fachat (a.fachat@physik.tu-chemnitz.de)
 *
 * Label management module (also see xau.c)
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
#include <string.h>
#include <ctype.h>

/* structs and defs */

#include "xad.h"
#include "xah.h"
#include "xar.h"
#include "xah2.h"
#include "xap.h"
#include "xa.h"

/* externals */

#include "xam.h"
#include "xal.h"

/* exported globals */

char *lz;

/* local prototypes */

static int b_fget(int*, int);
static int b_ltest(int, int);
static int b_get(int*);
static int b_test(int);
static int ll_def(char *s, int *n, int b, xalabel_t ltype);
static int b_link(int);

static int b_new(void);

static void cll_init();
static int cll_get();
static void cll_clear();
static int cll_getcur();

static Labtab *ltp;

int l_init(void) {
	cll_init();
	//unn_init();
	return 0;
}

int ga_lab(void) {
	return (afile->la.lti);
}

int gm_lab(void) {
	return (ANZLAB);
}

long gm_labm(void) {
	return ((long) LABMEM);
}

long ga_labm(void) {
	return (0 /*lni*/);
}

void printllist(fp)
	FILE *fp; {
	int i;
	LabOcc *p;
	char *fname = NULL;

	for (i = 0; i < afile->la.lti; i++) {
		ltp = afile->la.lt + i;
		fprintf(fp, "%s, 0x%04x, %d, 0x%04x\n", ltp->n, ltp->val, ltp->blk,
				ltp->afl);
		p = ltp->occlist;
		if (p) {
			while (p) {
				if (fname != p->fname) {
					if (p != ltp->occlist)
						fprintf(fp, "\n");
					fprintf(fp, "    %s", p->fname);
					fname = p->fname;
				}
				fprintf(fp, " %d", p->line);
				p = p->next;
			}
			fprintf(fp, "\n");
		}
		fname = NULL;
	}
}

/**********************************************************************************
 * cheap local labels
 */

static int cll_current = 0; /* the current cheap local labels block */

/**
 * init the cheap local labels
 */
void cll_init() {
	cll_current = 0;
}

/**
 * get the block number for a new cheap local label block
 */
int cll_get() {
	if (cll_current == 0) {
		cll_current = b_new();
	}
	return cll_current;
}

/**
 * clear the local labels
 */
void cll_clear() {
	cll_current = 0;
}

int cll_getcur() {
	return cll_current;
}

/**********************************************************************************/

/**
 * define a global label (from the "-L" command line parameter)
 */
int lg_set(char *s) {
	int n, er;

	er = ll_search(s, &n, STD);

	if (er == E_OK) {
		fprintf(stderr, "Warning: global label doubly defined!\n");
	} else {
		if (!(er = ll_def(s, &n, 0, STD))) {
			return lg_import(n);
		}
	}
	return er;
}

/**
 * define a global label (from the .import pseudo opcode))
 * "s" is a pointer to the first label character, end is at \0
 * or at non-alphanumeric/_ char
 */
int lg_import(int n) {
	int er = E_OK;

	ltp = afile->la.lt + n;
	ltp->fl = 2;
	ltp->afl = SEG_UNDEF;

	return er;
}

/*
 * re-define a previously undef'd label as globally undefined 
 * (for -U option)
 */
int lg_toglobal(char *s) {
	int n, er;

//printf("lg_toglobal(%s)\n", s);
	er = ll_search(s, &n, STD);

	if (er == E_OK && ltp->fl != 3) {
		// fonnd, but not yet set as global undef'd label
		ltp = afile->la.lt + n;
		ltp->fl = 3;
		ltp->afl = SEG_UNDEF;
		ltp->origblk = ltp->blk;
		ltp->blk = 0;
	}
	return er;
}

/**
 * define a global zeropage label (from the .importzp pseudo opcode))
 * "s" is a pointer to the first label character, end is at \0
 * or at non-alphanumeric/_ char
 */
int lg_importzp(int n) {
	int er = E_OK;

	ltp = afile->la.lt + n;
	ltp->fl = 2;
	ltp->afl = SEG_UNDEFZP;

	return er;
}

/**********************************************************************************/

int l_def(char *s, int *l, int *x, int *f) {
	int n, er, b, i = 0;
	xalabel_t cll_fl;

	*f = 0; /* flag (given as param) that the label is to be re-defined and the
	 "label defined error" is to be skipped */
	b = 0; /* block level on block stack, resp. block number */
	n = 0; /* flag, when set, b is absolute block number and not being translated */
	cll_fl = STD; /* when 0, clear the cheap local label block */

	if (s[0] == ':') {
		// ca65 unnamed label
		i++;
		//n++;		/* block number b is absolute */
		//b=unn_get();	/* current (possibly newly allocated) unnamed label block */
		cll_fl = UNNAMED;	// keep the cheap local label block
	} else if (s[0] == '-') {
		*f += 1; /* label is being redefined */
		i++;
	} else if (s[0] == '@') {
		i++;
		n++; /* block number b is absolute */
		b = cll_get(); /* current (possibly newly allocated) cheap label block */
		cll_fl = CHEAP; /* do not clear the cll block again... */
	} else if (s[0] == '+') {
		i++;
		n++; /* block number b is absolute */
		b = 0; /* global block number */
	}
	while (s[i] == '&') {
		if (n)
			b = 0; /* reset block number */
		n = 0; /* block number is relative */
		i++;
		b++; /* one (more) level up the block stack */
	}
	if (!n) {
		/* translate from block stack level to absolute block number */
		b_fget(&b, b);
	}

	if (cll_fl == STD) {
		/* clear cheap local labels */
		cll_clear();
	}

	if ((!isalpha(s[i])) && (s[i] != '_')
			&& !((ca65 || collab) && ((cll_fl == UNNAMED) || isdigit(s[i])))) {
		//printf("SYNTAX ca65=%d collab=%d cll_fl=%d, i=%d, s[i]=%02x (%c)\n", ca65, collab, cll_fl, i, s[i], s[i]);
		er = E_SYNTAX;
	} else {
		er = E_NODEF;
		if (cll_fl != UNNAMED) {
			er = ll_search(s + i, &n, cll_fl);
		}

		if (er == E_OK) {
			//printf("l_def OK: cll_fl=%d, i=%d, s=%s\n", cll_fl, i, s);
			/* we actually found an existing label in the same scope */
			ltp = afile->la.lt + n;

			if (*f) {
				/* redefinition of label */
				*l = ltp->len + i;
			} else if (ltp->fl == 0) {
				/* label has not been defined yet, (e.g. pass1 forward ref), so we try to set it. */
				*l = ltp->len + i;
				if (b_ltest(ltp->blk, b))
					er = E_LABDEF;
				else
					ltp->blk = b;

			} else if (ltp->fl == 3) {
				/* label has been defined as -U undef'd label so far - we need to check */
				*l = ltp->len + i;
				if (b_ltest(ltp->origblk, b))
					er = E_LABDEF;
				else
					ltp->blk = b;

			} else
				er = E_LABDEF;
		} else if (er == E_NODEF) {

			if (!(er = ll_def(s + i, &n, b, cll_fl))) /* store the label in the table of labels */
			{
				ltp = afile->la.lt + n;
				*l = ltp->len + i;
				ltp->fl = 0;
				ltp->is_cll = cll_fl;
			}
			//printf("l_def NODEF: n=%d, s=%s\n", n, ltp->n);
		}

		*x = n;
	}
	return (er);
}

int l_search(char *s, int *l, int *x, int *v, int *afl) {
	int n, er, b;
	xalabel_t cll_fl;

	*afl = 0;

	/* check cheap local label */
	cll_fl = STD;
	if (s[0] == '@') {
		cll_fl = CHEAP;
		s++;
	} else if (s[0] == ':') {
		cll_fl = UNNAMED_DEF;
		s++;
	}

	er = E_NODEF;
	if (cll_fl != UNNAMED_DEF) {
		er = ll_search(s, &n, cll_fl);
	}

//printf("l_search: lab=%s(afl=%d, er=%d, cll_fl=%d, cll_cur=%d)\n",s,*afl,er, cll_fl, cll_getcur());
	if (er == E_OK) {
		ltp = afile->la.lt + n;
		*l = ltp->len + ((cll_fl == STD) ? 0 : 1);
		if (ltp->fl == 1) {
			l_get(n, v, afl);/*               *v=lt[n].val;*/
			*x = n;
		} else {
			er = E_NODEF;
			lz = ltp->n;
			*x = n;
		}
	} else {
		if (cll_fl == CHEAP) {
			b = cll_get();
		} else if (cll_fl == UNNAMED_DEF) {
			b_get(&b); // b=unn_get();
		} else {
			b_get(&b);
		}

		er = ll_def(s, x, b, cll_fl); /* ll_def(...,*v); */

		ltp = afile->la.lt + (*x);
		ltp->is_cll = cll_fl;

		*l = ltp->len + ((cll_fl == STD) ? 0 : 1);
		//*l=ltp->len + cll_fl;

		if (!er) {
			er = E_NODEF;
			lz = ltp->n;
		}
	}
	return (er);
}

int l_vget(int n, int *v, char **s) {
	ltp = afile->la.lt + n;
	(*v) = ltp->val;
	*s = ltp->n;
	return 0;
}

void l_addocc(int n, int *v, int *afl) {
	LabOcc *p, *pp;

	(void) v; /* quench warning */
	(void) afl; /* quench warning */
	ltp = afile->la.lt + n;
	pp = NULL;
	p = ltp->occlist;
	while (p) {
		if (p->line == filep->fline && p->fname == filep->fname)
			return;
		pp = p;
		p = p->next;
	}
	p = malloc(sizeof(LabOcc));
	if (!p) {
		fprintf(stderr, "Oops, out of memory!\n");
		exit(1);
	}
	p->next = NULL;
	p->line = filep->fline;
	p->fname = filep->fname;
	if (pp) {
		pp->next = p;
	} else {
		ltp->occlist = p;
	}
}

/* for the list functionality */
char* l_get_name(int n, xalabel_t *is_cll) {
	if (n > afile->la.ltm) {
		fprintf(stderr, "Corrupted structures! n=%d, but max=%d\n", n,
				afile->la.ltm);
		exit(1);
	}
	ltp = afile->la.lt + n;
	*is_cll = ltp->is_cll;
	return ltp->n;
}

// works on the static(!) ltp "label table pointer"
// also returns the actual index in the table of the current ltp
static int resolve_unnamed() {
	// need to count up/down in the linkd label list for the block
	char *namep = ltp->n;
	int nextp = -1;
	//printf("::: unnamed_def: %s, n=%d\n", namep, n);
	while ((*namep == '+') || (*namep == '-')) {
		char c = *namep;
		nextp = -1;
		if (c == '+') {
			nextp = ltp->blknext;
		} else if (c == '-') {
			nextp = ltp->blkprev;
		}
		//printf("::: nextp=%d\n", nextp);
		if (nextp == -1) {
			return -1;	// E_NODEF
		}
		ltp = afile->la.lt + nextp;
		//printf("::: leads to: %s, nextp=%d\n", ltp->n, nextp);
		if (ltp->is_cll == UNNAMED) {
			namep++;
		}
	}
	return nextp;
}

/* for the listing, esp. html links; returns a pointer to a static buffer, available till next call */
char* l_get_unique_name(int n) {
	static char buf[MAXLINE];
	ltp = afile->la.lt + n;

	if (ltp->is_cll == CHEAP || ltp->is_cll == STD) {
		sprintf(buf, "%d%c%s", ltp->blk, (ltp->is_cll == CHEAP) ? 'C' : '_',
				ltp->n);
	} else if (ltp->is_cll == UNNAMED) {
		// definition of unnamed label - name is NULL
		// so use the actual index
		sprintf(buf, "%dU%d", ltp->blk, n);
	} else if (ltp->is_cll == UNNAMED_DEF) {
		// we actually need to find the correct label from the "+" and "-"
		// in the name
		int tmp = resolve_unnamed();
		if (tmp >= 0) {
			sprintf(buf, "%dU%d", ltp->blk, tmp);
		} else {
			sprintf(buf, "__%d", tmp);
		}
	} else {
		buf[0] = 0;	// no value
	}
	return buf;
}

int l_get(int n, int *v, int *afl) {
	if (crossref)
		l_addocc(n, v, afl);

	ltp = afile->la.lt + n;

	if (ltp->is_cll == UNNAMED_DEF) {
		int tmp = resolve_unnamed();
		if (tmp == -1)
			return E_NODEF;
		// now ltp is set to the actual label
	}
	(*v) = ltp->val;
	lz = ltp->n;
	*afl = ltp->afl;
	/*printf("l_get('%s'(%d), v=$%04x, afl=%d, fl=%d\n",ltp->n, n, *v, *afl, ltp->fl);*/
	return ((ltp->fl == 1) ? E_OK : E_NODEF);
}

void l_set(int n, int v, int afl) {
	ltp = afile->la.lt + n;
	ltp->val = v;
	ltp->fl = 1;
	ltp->afl = afl;

//printf("l_set('%s'(%d), v=$%04x, afl=%d\n",ltp->n, n, v, afl);
}

static void ll_exblk(int a, int b) {
	int i;
	for (i = 0; i < afile->la.lti; i++) {
		ltp = afile->la.lt + i;
		if ((!ltp->fl) && (ltp->blk == a))
			ltp->blk = b;
	}
}

/* defines next label, returns new label number in out param n  */
static int ll_def(char *s, int *n, int b, xalabel_t ltype) {
	int j = 0, er = E_NOMEM, hash;
	char *s2 = NULL;

//printf("ll_def: s=%s, ltype=%d, no_name=%d\n",s, ltype, no_name);    

	// label table for the file ...
	if (!afile->la.lt) {
		// ... does not exist yet, so malloc it
		afile->la.lti = 0;
		afile->la.ltm = 1000;
		afile->la.lt = malloc(afile->la.ltm * sizeof(Labtab));
	}
	if (afile->la.lti >= afile->la.ltm) {
		// ... or is at its capacity limit, so realloc it
		afile->la.ltm *= 1.5;
		afile->la.lt = realloc(afile->la.lt, afile->la.ltm * sizeof(Labtab));
	}
	if (!afile->la.lt) {
		fprintf(stderr, "Oops: no memory!\n");
		exit(1);
	}

	// current pointer in label table
	ltp = afile->la.lt + afile->la.lti;

	if (ltype != UNNAMED) {
		// alloc space and copy over name
		if (ltype == UNNAMED_DEF) {
			// unnamed lables are like ":--" or ":+" with variable length
			while ((s[j] != '\0') && (s[j] == '+' || s[j] == '-'))
				j++;
		} else {
			// standard (and cheap) labels are normal text
			while ((s[j] != '\0') && (isalnum(s[j]) || (s[j] == '_')))
				j++;
		}
		s2 = malloc(j + 1);
		if (!s2) {
			fprintf(stderr, "Oops: no memory!\n");
			exit(1);
		}
		strncpy(s2, s, j);
		s2[j] = 0;
	}

	// init new entry in label table
	er = E_OK;
	ltp->len = j;		// length of label
	ltp->n = s2;		// name of label (char*)
	ltp->blk = b;		// block number
	ltp->fl = 0;
	ltp->afl = 0;
	ltp->is_cll = ltype;	// STD, CHEAP, or UNNAMED label
	ltp->occlist = NULL;
	hash = hashcode(s, j); 	// compute hashcode
	ltp->nextindex = afile->la.hashindex[hash];	// and link in before last entry with same hashcode
	afile->la.hashindex[hash] = afile->la.lti;// set as start of list for that hashcode

	// TODO: does not work across files!
	ltp->blknext = -1;	// no next block
	ltp->blkprev = b_link(afile->la.lti);// previous block, linked within block

	if (ltp->blkprev != -1) {
		ltp = afile->la.lt + ltp->blkprev;
		ltp->blknext = afile->la.lti;
	}

	*n = afile->la.lti;	// return the list index for that file in the out parameter n
	afile->la.lti++;		// increase last index in lable table

	return (er);
}

/**
 * search a label name in the label table. Return the label number
 * in "n". Finds only labels that are in a block that is in the current
 * set of blocks (in the block stack)
 *
 * If cll_fl is set, the label is also searched in the local cheap label scope
 *
 * Do not define the label (as is done in l_search()!)
 */
int ll_search(char *s, int *n, xalabel_t cll_fl) /* search Label in Tabelle ,nr->n    */
{
	int i, j = 0, k, er = E_NODEF, hash;

	while (s[j] && (isalnum(s[j]) || (s[j] == '_')))
		j++;

	hash = hashcode(s, j);
	i = afile->la.hashindex[hash];

	if (i >= afile->la.ltm)
		return E_NODEF;

	do {
		ltp = afile->la.lt + i;

		if (j == ltp->len) {
			for (k = 0; (k < j) && (ltp->n[k] == s[k]); k++)
				;

			if ((j == k) && cll_fl == CHEAP) {
				if (ltp->blk == cll_getcur()) {
					er = E_OK;
					break;
				}
			} else if (cll_fl == UNNAMED) {
				// TODO
			} else {
//printf("ll_search:match labels %s with %p (%s) from block %d, block check is %d\n", s, ltp, ltp->n, ltp->blk, b_test(ltp->blk));
				/* check if the found label is in any of the blocks in the
				 current block stack */
				if ((j == k) && (!b_test(ltp->blk))) {
					/* ok, label found and it is reachable (its block nr is in the current block stack */
					er = E_OK;
					break;
				}
			}
		}

		if (!i)
			break;

		i = ltp->nextindex;

	} while (1);

	*n = i;
#if 0
     if(er!=E_OK && er!=E_NODEF)
     {
          fprintf(stderr, "Fehler in ll_search:er=%d\n",er);
          getchar();
     }
#endif
//printf("l_search(%s) returns er=%d, n=%d\n", s, er, *n);

	return (er);
}

int ll_pdef(char *t) {
	int n;

	if (ll_search(t, &n, STD) == E_OK) {
		ltp = afile->la.lt + n;
		if (ltp->fl)
			return (E_OK);
	}
	return (E_NODEF);
}

/*
 * Write out the list of global labels in an o65 file
 */
int l_write(FILE *fp) {
	int i, afl, n = 0;

	if (noglob) {
		fputc(0, fp);
		fputc(0, fp);
		return 0;
	}
	// calculate number of global labels
	for (i = 0; i < afile->la.lti; i++) {
		ltp = afile->la.lt + i;
		if ((!ltp->blk) && (ltp->fl == 1)) {
			n++;
		}
	}
	// write number of globals to file
	fputc(n & 255, fp);
	fputc((n >> 8) & 255, fp);
	// iterate over labels and write out label
	for (i = 0; i < afile->la.lti; i++) {
		ltp = afile->la.lt + i;
		if ((!ltp->blk) && (ltp->fl == 1)) {
			// write global name
			fprintf(fp, "%s", ltp->n);
			fputc(0, fp);

			// segment byte
			afl = ltp->afl;
			// hack to switch undef and abs flag from internal to file format
			// if asolute of undefined (< SEG_TEXT, i.e. 0 or 1)
			// then invert bit 0 (0 = absolute)
			if ((afl & (A_FMASK >> 8)) < SEG_TEXT) {
				afl ^= 1;
			}
			// remove residue flags, only write out real segment number
			// according to o65 file format definition
			afl = afl & (A_FMASK >> 8);
			fputc(afl, fp);

			// value
			fputc(ltp->val & 255, fp);
			fputc((ltp->val >> 8) & 255, fp);
		}
	}
	/*fputc(0,fp);*/
	return 0;
}

/*******************************************************************************************
 * block management code. Here the ".(" and ".)" blocks are maintained. 
 *
 * Blocks are numbered uniquely, every time a new block is opened, the "blk" variable
 * is increased and its number used as block number.
 *
 * The currently open blocks are maintained in a stack (bt[]). The lowest entry is the outermost
 * block number, adding block numbers as blocks are opened. When a block is closed,
 * the block stack is shortened again (bi has the length of the block stack)
 *
 * Methods exist to open new blocks, close a block, and do some checks, e.g. whether
 * a specific block number is contained in the current block stack.
 */
static int bt[MAXBLK]; /* block stack */
static int labind; /* last allocated label, -1 none yet alloc'd - used for linking to find unnamed labels */
static int bi; /* length of the block stack (minus 1, i.e. bi[bi] has the innermost block) */
static int blk; /* current block number for allocation */

int b_init(void) {
	blk = 0;
	bi = 0;
	bt[bi] = blk;
	labind = -1;

	return (E_OK);
}

int b_new(void) {
	return ++blk;
}

int b_depth(void) {
	return bi;
}

int ga_blk(void) {
	return (blk);
}

/**
 * open a new block scope
 */
int b_open(void) {
	int er = E_BLKOVR;

	if (bi < MAXBLK - 1) {
		bi++;
		bt[bi] = b_new();

		er = E_OK;
	}
	return (er);
}

/**
 * close a block scope
 */
int b_close(void) {

	if (bi) {
		ll_exblk(bt[bi], bt[bi - 1]);
		bi--;
	} else {
		return E_BLOCK;
	}
	cll_clear();
	//unn_clear();
	return (E_OK);
}

/**
 * get the block number of the current innermost block
 */
static int b_get(int *n) {
	*n = bt[bi];

	return (E_OK);
}

/**
 * returns the block number of the block "i" levels up in the current block stack
 */
static int b_fget(int *n, int i) {
	if ((bi - i) >= 0)
		*n = bt[bi - i];
	else
		*n = 0;
	return (E_OK);
}

/**
 * tests whether the given block number n is in the current stack of
 * current block numbers bt[]
 */
static int b_test(int n) {
	int i = bi;

	while (i >= 0 && n != bt[i])
		i--;

	return (i + 1 ? E_OK : E_NOBLK);
}

/**
 * tests whether the given block number "a" is in the 
 */
static int b_ltest(int a, int b) /* testet ob bt^-1(b) in intervall [0,bt^-1(a)]   */
{
	int i = 0, er = E_OK;

	if (a != b) {
		er = E_OK;

		while (i <= bi && b != bt[i]) {
			if (bt[i] == a) {
				er = E_NOBLK;
				break;
			}
			i++;
		}
	}
	return (er);
}

int b_link(int newlab) {
	int tmp = labind;
	//printf("b_link: old was %d, set to %d\n", tmp, newlab);
	labind = newlab;
	return tmp;
}

