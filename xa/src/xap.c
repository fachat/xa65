/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 * Maintained by Cameron Kaiser
 *
 * File handling and preprocessor (also see xaa.c) module
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

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>
#ifndef _MSC_VER
#include  <strings.h>
#endif

#include  "xad.h"
#include  "xah.h"
#include  "xah2.h"

#include  "xar.h"
#include  "xa.h"
#include  "xam.h"
#include  "xal.h"
#include  "xat.h"
#include  "xap.h"

/* define this for recursive evaluation output */
#undef DEBUG_RECMAC
#undef DEBUG_REPLACE

char s[MAXLINE];
Datei *filep;

static int tcompare(char*,char**,int);
static int pp_replace(char*,char*,int,int);
static int searchdef(char*);
static int fgetline(char*,int len, int *rlen, FILE*);

/*static int icl_open(char*);*/
static int pp_ifdef(char*),pp_ifndef(char*);
static int pp_else(char*),pp_endif(char*);
static int pp_echo(char*),pp_if(char*),pp_print(char*),pp_prdef(char*);
static int pp_ifldef(char*),pp_iflused(char*);
static int pp_undef(char*);

#define   ANZBEF    13
#define   VALBEF    6

static int ungeteof = 0;

static char *cmd[]={ "echo","include","define","undef","printdef","print",
			"ifdef","ifndef","else","endif",
               "ifldef","iflused","if" };
               
static int (*func[])(char*) = { pp_echo,icl_open,pp_define,pp_undef,
			 pp_prdef,pp_print, pp_ifdef,pp_ifndef,
                         pp_else,pp_endif,
                         pp_ifldef,pp_iflused,pp_if };

static char 		*mem;
static unsigned long 	memfre;
static int 		nlf;
static int 		nff;
static int 		hashindex[256];

static List 	     	*liste;
static unsigned int     rlist;
static int       	fsp;
static int       	loopfl;
static Datei     	flist[MAXFILE+1];
static char      	in_line[MAXLINE];

int pp_comand(char *t)
{
     int i,l,er=1;

     i=tcompare(t,cmd,ANZBEF);

     if(i>=0)
     {
          if(loopfl && (i<VALBEF))
               er=0;
          else
          {
               l=(int)strlen(cmd[i]);
               while(isspace(t[l])) l++;

               er=(*func[i])(t+l);
          }
     }
     return(er);
}

int pp_ifdef(char *t)
{
/*     int x;
     printf("t=%s\n",t);
     x=searchdef(t);
     printf("searchdef(t)=%d\n",x);
*/   
     loopfl=(loopfl<<1)+( searchdef(t) ? 0 : 1 );
     return(0);
}

int pp_ifndef(char *t)
{
     loopfl=(loopfl<<1)+( searchdef(t) ? 1 : 0 );
     return(0);
}

int pp_ifldef(char *t)
{
	loopfl=(loopfl<<1)+( ll_pdef(t) ? 1 : 0 );
	return(0);
}

int pp_iflused(char *t)
{
	int n;
	loopfl=(loopfl<<1)+( ll_search(t,&n,STD) ? 1 : 0 );
	return(0);
}

int pp_echo(char *t)
{
     int er;
     
     if((er=pp_replace(s,t,-1,rlist)))
          errout(er);
     else
     {
          logout(s);
          logout("\n");
     }
     return(0);
}

int pp_print(char *t)
{
     int f,a,er;
     
     logout(t);
     if((er=pp_replace(s,t,-1,rlist)))
     {
          logout("\n");
          errout(er);
     }
     else
     {
          logout("=");
          logout(s);
          logout("=");
          er=b_term(s,&a,&f,pc[segment]);
          if(er)
          {
               logout("\n");
               errout(er);
          }
          else
               { sprintf(s,"%d\n",a); logout(s); }
     }
     
     return(0);
}

int pp_if(char *t)
{
     int a,f,l,er;

     if((er=pp_replace(s,t,-1,rlist)))
          errout(er);
     else
     {
	dsb_len = 1;
          f=b_term(s,&a,&l,pc[segment]);
	dsb_len = 0;

          if((!loopfl) && f)     
               errout(f);
          else
               loopfl=(loopfl<<1)+( a ? 0 : 1 );
     }
     return(0);
}

int pp_else(char *t)
{
     (void)t;		/* quench warning */
     loopfl ^=1;
     return(0);
}

int pp_endif(char *t)
{
     (void)t;		/* quench warning */
     loopfl=loopfl>>1;
     return(0);
}


/* stub for handling CPP directives */
int pp_cpp(char *t) {

	if(sscanf(t, " %d \"%s\"", &filep->fline, filep->fname) == 2) {
		/* massage it into our parameters and drop last quote */
		char *u = "";

		filep->fline--;
		if((u = (char *)strrchr(filep->fname, '"')))
			*u = '\0';
		return (0);
	} else {
		return(E_SYNTAX);
	}
}

/* pp_undef is a great hack to get it working fast... */
int pp_undef(char *t) {
     int i;
     if((i=searchdef(t))) {
	i+=rlist;
	liste[i].s_len=0;
     }
     return 0;
}

int pp_prdef(char *t)
{
     char *x;
     int i,j;

     if((i=searchdef(t)))
     {
          i+=rlist;
          x=liste[i].search;
          sprintf(s,"\n%s",x);
          if(liste[i].p_anz)
          {
               sprintf(s+strlen(s),"(");
               for(j=0;j<liste[i].p_anz;j++)
               {
                    x+=strlen(x)+1;
                    sprintf(s+strlen(s),"%s%c",x,(liste[i].p_anz-j-1) ? ',' : ')');
               }
          }
          sprintf(s+strlen(s),"=%s\n",liste[i].replace);
          logout(s);
     }
     return(E_OK);
}

int searchdef(char *t)
{
     int i=0,j,k,l=0;

     while(t[l]!=' ' && t[l]!='\0') l++;

     if(rlist)
     {
       i=hashindex[hashcode(t,l)];
     
       do   /*for(i=0;i<rlist;i++)*/
       {
          k=liste[i].s_len;
          j=0;

          if(k && (k==l))
          {
               while((t[j]==liste[i].search[j])&&j<k) j++;
               if(j==k)
                    break;
          }
          
          if(!i)
          {
               i=rlist;
               break;
          }
          
          i=liste[i].nextindex;
               
       } while(1);
     } 
    
     return(i-rlist);
}

int ga_pp(void)
{
	return(rlist);
}

int gm_pp(void)
{
	return(ANZDEF);
}

long gm_ppm(void)
{
	return(MAXPP);
}

long ga_ppm(void)
{
	return(MAXPP-memfre);
}
    
int pp_define(char *k)
{
     int i,er=E_OK,hash,rl;
     char h[MAXLINE*2],*t;
     unsigned int j;
     
     t=k;
          
     if(rlist>=ANZDEF || memfre<MAXLINE*2)
          return(E_NOMEM);
/*
     printf("define:mem=%04lx\n",mem);
     getchar();
*/   
     rl=rlist++;
     
     liste[rl].search=mem;
     for(i=0; (t[i]!=' ') && (t[i]!='\0') && (t[i]!='(') ;i++)
          *mem++=t[i];
     *mem++='\0';
     memfre-=i+1;
     liste[rl].s_len=i;
     liste[rl].p_anz=0;


/*   printf("define:%s\nlen1=%d\n",liste[rl].search,liste[rl].s_len);
     getchar();
*/

     if(t[i]=='(')
     {
          while(t[i]!=')' && t[i]!='\0')
          {
               i++;
               liste[rl].p_anz++;
	       // skip whitespace before parameter name
	       while(isspace(t[i])) i++;
	       // find length
               for(j=0; (!isspace(t[i+j])) && t[i+j]!=')' && t[i+j]!=',' && t[i+j]!='\0';j++);
               if(j<memfre)
               {
                    strncpy(mem,t+i,j);
                    mem+=j;
		    *mem++='\0';
                    memfre-=j+1;
               }
               i+=j;
	       // skip trailing whitespace after parameter name
	       while(isspace(t[i])) i++;
          }
          if(t[i]==')')
               i++;
     }
     while(t[i]==' ')
          i++;
     t+=i;
     
     pp_replace(h,t,-1,0);

     t=h;     

     liste[rl].replace=mem;
     (void)strcpy(mem,t);
     mem+=strlen(t)+1;
#if 0	/* debug */
     { char *ss;
     if(liste[rl].p_anz)
     {
          ss=liste[rl].search;
          printf("define:\n%s(",liste[rl].search);
          for(j=0;j<liste[rl].p_anz;j++)
          {
               ss+=strlen(ss)+1;
               printf("%s%c",ss,(liste[rl].p_anz-j-1) ? ',' : ')');
          }
          printf("=%s\n",liste[rl].replace);
          getchar();
     }
     else
     {
          printf("define:%s=%s\nlen1=%d\n",liste[rl].search,
               liste[rl].replace,liste[rl].s_len);
     }
     }
#endif
     if(!er)
     {
          hash=hashcode(liste[rl].search,liste[rl].s_len);
          liste[rl].nextindex=hashindex[hash];
          hashindex[hash]=rl;
     } else
          rlist=rl;
     
     return(er);
}

int tcompare(char s[],char *v[], int n)
{
     int i,j,l;
     static char t[MAXLINE];

     for(i=0; s[i]!='\0'; i++) 
          t[i]=tolower(s[i]);
     t[i]='\0';

     for(i=0;i<n;i++)
     {
          l=(int)strlen(v[i]);
          
          for(j=0;j<l;j++)
          {
               if(t[j]!=v[i][j])
                    break;
          }
          if(j==l)
               break;
     }
     return((i==n)? -1 : i);
}

static int check_name(char *t, int n) {

                    int i=0;
		    { 
                        char *x=liste[n].search;
                        while(t[i]==*x++ && t[i] && (isalnum(t[i]) || t[i]=='_'))
                            i++;
		    }

#ifdef DEBUG_RECMAC	
	printf("check name with n=%d, name='%s' ->i=%d, len=%d\n", n, liste[n].search,i, liste[n].s_len);
#endif
		    return i == liste[n].s_len;
}

/**
 * this is a break out of the original pp_replace code, as this code
 * was basically duplicated for initial and recursive calls.
 */
static int pp_replace_part(char *to, char *t, int n, int sl, int recursive, int *l, int blist) 
{
	int er = E_OK;

	// save mem, to restore it when we don't need the pseudo replacements anymore
	// Note: in a real version, that should probably be a parameter, and not fiddling
	// with global variables...
	char *saved_mem = mem;

#ifdef DEBUG_RECMAC
	printf("replace part: n=%d, sl=%d, rec=%d, %s\n", n, sl, recursive, t);
#endif

			 // yes, mark replacement string
                         char *rs=liste[n].replace;
                        
			 // does it have parameters? 
                         if(liste[n].p_anz)        
                         {
			      // yes, we have parameters, so we need to pp_replace them
			      // as well.
     			      char fti[MAXLINE],fto[MAXLINE];

			      // copy replacement into temp buffer
                              (void)strcpy(fti,liste[n].replace);

			      // boundary checks ...
                              if(blist+liste[n].p_anz>=ANZDEF || memfre<MAXLINE*2)
                                   er=E_NOMEM;
                              else
                              {
				   // ... passed
				   // y points to the char behind the input name (i.e. the '(')
                                   char *y=t+sl; 
				   // x points into the pp definition, with the parameter definition names
				   // following the definition name, separated by \0. Starts here behind the 
				   // name - i.e. now points to the name of the first parameter
                                   char *x=liste[n].search+sl+1;
				   // does the input actually have a '(' as parameter marker?
                                   if(*y!='(') {
					// no. Should probably get an own error (E_PARAMETER_EXPECTED)
                                        er=E_SYNTAX;
				   }
                                   else
                                   {
					// mx now points to next free memory (mem current pointer in text table)
                                        char *mx=mem-1;
					// walk through the pp parameters
					// by creating "fake" preprocessor definitions at the end
					// of the current definition list, i.e. in liste[] after index
					// rlist.
                                        for(int i=0;i<liste[n].p_anz;i++)
                                        {
					     // create new replacement entry
                                             liste[blist+i].search=x;
                                             liste[blist+i].s_len=(int)strlen(x);
                                             liste[blist+i].p_anz=0;
                                             liste[blist+i].replace=mx+1;
					     // move x over to the next parameter name
                                             x+=strlen(x)+1;
					     // points to first char of the parameter name in the input
					     // copy over first char into text memory (note that the position
					     // is already stored in liste[].replace above)
                                             char c=*(++mx)=*(++y);
                                             int hkfl=0;	// quote flag
					     int klfl=0;	// brackets counter
					     // copy over the other characters
                                             while(c!='\0' 
                                                  && ((hkfl!=0 
                                                       || klfl!=0) 
                                                       || (c!=',' 
                                                       && c!=')') 
                                                       )
                                                  )
                                             {
                                                  if(c=='\"')
                                                       hkfl=hkfl^1;
                                                  if(!hkfl)
                                                  {
                                                       if(c=='(')
                                                            klfl++;
                                                       if(c==')')
                                                            klfl--;
                                                  }     
                                                  c=*(++mx)=*(++y);
                                             } 
                                             // zero-terminate stored string
                                             *mx='\0';
					     // if i is for the last parameter, then check if the
					     // last copied char was ')', otherwise it should be ','
					     // as separator for the next parameter
                                             if(c!=((i==liste[n].p_anz-1) ? ')' : ','))
                                             {
                                                  er=E_ANZPAR;
						  // note: this break only exits the innermost loop!
                                                  break;
                                             }
                                        }
					// at this point we have "augmented" the pp definitions
					// with a list of new definitions for the macro parameters
					// so that when pp_replace'ing them recursively the macro parameters
					// will automatically be replaced.   
					
					// if we ran into an error, report so
					if (er != E_OK) {
						return (er);
					}

					// let mx point to first free (and not last non-free) byte
					mx++;
	
					// before we use the parameter replacements, we need to 
					// recursively evaluate and replace them as well.
					
#ifdef DEBUG_RECMAC
     printf("replace (er=%d):\n", er);
     printf("%s=%s\n",liste[n].search,liste[n].replace);
#endif
     // loop over all arguments
     for(int i=0;i<liste[n].p_anz;i++) {
/* recursively evaluate arguments */
	char nfto[MAXLINE];
	char nfwas[MAXLINE];
	//int j = 0;
	//int k = 0;

	// copy over the replacement string into a buffer nfwas
	(void)strcpy(nfwas, liste[blist+i].replace);
	if (!er) {
		// replace the tokens in the parameter, adding possible pseudo params
		// on top of the liste[] into nfto
		er=pp_replace(nfto,nfwas,-1,blist+liste[n].p_anz);
	}
#ifdef DEBUG_RECMAC
        printf("-%s=%s\n",liste[rlist+i].search,liste[rlist+i].replace);
	printf("SUBB: -%s=%s\n", nfwas, nfto);
#endif
#if 0
	// as long as the strings don't match, loop...
	while ((k = strcmp(nfto, nfwas))) {
		// copy original from nfwas back to nfto
		(void)strcpy(nfto, nfwas);
		if (!er) {
			// save-guard our replacement strings in global memory for the
			// recursive pp_replace call. Note: is cleaned up after return, 
			// so need to restore mem only at the end of this function.
			mem = mx;
			// replace tokens
			er=pp_replace(nfto,nfwas,-1,rlist);
		}
		// and copy result into input buffer
		(void)strcpy(nfwas, nfto);
#ifdef DEBUG_RECMAC
		printf("SUBB2 (%i): -%s=%s\n", k, liste[rlist+i].replace, nfto);
#endif
		if (++j>10) {
			// we ran into 10 recursions deep - that does not look sound, bail out
			errout(E_ORECMAC);
		}
	}
#endif
	// copy over the replacement string into free memory (using mx as pointer)
	(void)strcpy(mx, nfto);
	// replace the pointer to the (now obsolete) old replacement with the one we just created
	// Note that due to the nature of the (more or less static) memory allocation, this is not
	// being freed. Oh well...
	liste[blist+i].replace = mx;
	mx += strlen(mx)+1;

#ifdef DEBUG_RECMAC
        printf("FINAL: -%s=%s\n",liste[rlist+i].search,liste[rlist+i].replace);
#endif
    }

                                        if(!er) {
					     // safe-guard our memory allocations
					     mem = mx;
					     // only change (of two) from recursive: rlist is 0 there
#ifdef DEBUG_RECMAC
	printf("replace macro: recursive=%d, blist=%d, -> b=%d\n", recursive, blist, blist+liste[n].p_anz);
	printf("         from: %s\n", fti);					    
#endif
                                             er=pp_replace(fto,fti,recursive ? 0 : blist,blist+liste[n].p_anz);
#ifdef DEBUG_RECMAC
	printf("           to: %s\n", fto);					    
#endif
					}
/*               if(flag) printf("sl=%d,",sl);*/
                                        sl=(int)((long)y+1L-(long)t);
/*               if(flag) printf("sl=%d\n",sl);*/
                                        rs=fto;
#ifdef DEBUG_RECMAC
     printf("->%s\n",fto);
#endif
                                   }    
                              }
                              if(er) {
				   mem = saved_mem;
                                   return(er);     
			      }
                         }

                         int d=(int)strlen(rs)-sl;

                         if(strlen(to)+d>=MAXLINE) {
			      mem = saved_mem;
                              return(E_NOMEM);
			 }

/*
                         if(d<0)
                         {
                              y=t+sl+d;
                              x=t+sl;
                              while(*y++=*x++);
                         }
                         if(d>0)
                         {
                              for(ll=strlen(t);ll>=sl;ll--)
                                   t[ll+d]=t[ll];
                         }
*/
                         if(d) {
			      // d can be positive or negative, so strcpy cannot be used, use memmove instead
                              (void)memmove(t+sl+d,t+sl, strlen(t) - sl + 1);
			 }

                         int i=0;
			 char c;
                         while((c=rs[i]))
                              t[i++]=c;
			 // other change from recursive. there sl is missing from add
                         *l=(recursive ? 0 : sl) + d;/*=0;*/

	mem = saved_mem;

	return (er);
}

/**
 * copy the input string pointed to by ti into
 * an output string buffer pointed to by to, replacing all
 * preprocessor definitions in the process.
 *
 * Note: this method is called recursively, with "a" being -1
 * when called from the "outside" respectively in a new context
 * (like macro parameters)
 *
 * The "b" parameter denotes the index in the list from which on
 * pseudo replacement entries are being created for replacement
 * parameters
 *
 */
int pp_replace(char *to, char *ti, int a,int b)
{
     char *t=to;
     int l,n,sl,er=E_OK;
     int ld; 	// length of name/token to analyse
/*
     int flag=!strncmp(ti,"TOUT",4);
     if(flag) printf("flag=%d\n",flag);
*/   
     // t points to to, so copy input to output 1:1
     // then below work on the copy in-place
     (void)strcpy(t,ti);

     // if there are replacements in the list of replacements
     if(rlist)
     {
       // loop over the whole input
       while(t[0]!='\0' && t[0] != ';')
       {
	  // skip over the whitespace
	  // comment handling is NASTY NASTY NASTY
	  // but I currently don't see another way, as comments and colons
	  // can (and do) appear in preprocessor replacements
          char quotefl = 0;
	  char commentfl = 0;
          while((t[0] != 0) && (commentfl || ((!isalpha(t[0]) && t[0]!='_')))) {
               	if (t[0]=='\0') {
                    break;    /*return(E_OK);*/
	       	} else 
               	{
	       	    if (t[0] == ';' && !quotefl) {
			commentfl = 1;
		    }
		    if (t[0] == ':' && !quotefl && !ca65 && !masm) {
			// note that both ca65 and masm allow colons in comments
			// so in these cases we cannot reset the comment handling here
			commentfl = 0;
		    }
		    if (quotefl) {
			// ignore other quotes within a quote
			if (t[0] == quotefl) {
				quotefl = 0;
			}
		    } else
		    if (t[0] == '"' || t[0] == '\'') {
			quotefl = t[0];
		    }
                    t++;
                    ti++;
               	}
	  }
        
	  // determine the length of the name 
          for(l=0;isalnum(t[l])||t[l]=='_';l++);
	  // store in ld
          ld=l;
#ifdef DEBUG_RECMAC          
          printf("l=%d,a=%d,b=%d, t=%s\n",l,a, b ,t);
#endif
       
	  // the following if() is executed when being called from an 
	  // 'external' context, i.e. not from a recursion 
          if(a<0)
          {
	    // when called from an external context, not by recursion

	    // compute hashcode for the name for the search index entry (liste[n])
            n=hashindex[hashcode(t,l)];
            
	    // loop over all entries in linked list for hash code (think: hash collisions)
            do  // while(1); 
            {
	       // length of name of pp definition
               sl=liste[n].s_len;

	       // does pp definition match what we have found?
               if(sl && (sl==l) && check_name(t, n))
               {
			er = pp_replace_part(to, t, n, sl, 0, &l, b);
                        break;
               }
               if(!n)
                    break;
                   
	       // next index in linked list for given hash code 
               n=liste[n].nextindex;
               
            } while(1);
          } else
          {
	    // called when in recursive call
	    // loop over all the replacement entries from the given b down to 0
	    // that allows to replace the parameters first (as they were added at
	    // the end of the list)
            for(n=b-1;n>=a;n--)
            {
               sl=liste[n].s_len;
          
               if(sl && (sl==l) && check_name(t, n))
               {
			er = pp_replace_part(to, t, n, sl, 1, &l, b); 
                        break;
               }
            }
          }
	  // advance input by length of name
          ti+=ld;
	  // advance output by l
          t+=l;
       } /* end while(t[0] != 0) */
     }  /* end if(rlist) */
     return(E_OK);
}

int pp_init(void)
{
     int er;

     for(er=0;er<256;er++)
          hashindex[er]=0;
          
     fsp=0;

     er=0;
     mem=malloc(MAXPP);
     if(!mem) er=E_NOMEM;

     memfre=MAXPP;
     rlist=0;
     nlf=1;
     nff=1;
     if(!er) {
          liste=malloc((long)ANZDEF*sizeof(List));
	  if(!liste) er=E_NOMEM;
     }
     return(er);
}

int pp_open(char *name)
{
     FILE *fp;

     fp=xfopen(name,"r");

     int l = strlen(name);

     /* we have to alloc it dynamically to make the name survive another
 	pp_open - it's used in the cross-reference list */	
     flist[0].fname = malloc(l+1);
     if(!flist[0].fname) {
	fprintf(stderr,"Oops, no more memory!\n");
	exit(1);
     }	
     (void)strncpy(flist[0].fname,name,l+1);
     flist[0].fline=0;
     flist[0].bdepth=b_depth();
     flist[0].filep=fp;
     flist[0].flinep=NULL;    

     return(((long)fp)==0l);
}

void pp_close(void)
{
     if(flist[fsp].bdepth != b_depth()) {
	fprintf(stderr, "Blocks not consistent in file %s: start depth=%d, end depth=%d\n",
	  flist[fsp].fname, flist[fsp].bdepth, b_depth());
     }
     fclose(flist[fsp].filep);
}

void pp_end(void) { }

Datei *pp_getidat(void) {
	return &flist[fsp];
}

int icl_close(int *c)
{
     *c='\n';
     if(!fsp)
          return(E_EOF);
     
     if(flist[fsp].bdepth != b_depth()) {
	fprintf(stderr, "Blocks not consistent in file %s: start depth=%d, end depth=%d\n",
	  flist[fsp].fname, flist[fsp].bdepth, b_depth());
     }

     fclose(flist[fsp--].filep);
     nff=1;

     return(E_OK);
}

int icl_open(char *tt)
{
     FILE *fp2;
     int j,i=0;

     pp_replace(s,tt,-1,rlist);

     if(fsp>=MAXFILE)
          return(-1);

     if(s[i]=='<' || s[i]=='"')
          i++;

     for(j=i;s[j];j++)
          if(s[j]=='>' || s[j]=='"')
               s[j]='\0';

     fp2=xfopen(s+i,"r");

     if(!fp2)
          return(E_FNF);

	setvbuf(fp2,NULL,_IOFBF,BUFSIZE);
	
     fsp++;

     char *namep = s+i;
     int len = strlen(namep);

     /* we have to alloc it dynamically to make the name survive another
        pp_open - it's used in the cross-reference list */
     flist[fsp].fname = malloc(len+1);
     if(!flist[fsp].fname) {
        fprintf(stderr,"Oops, no more memory!\n");
        exit(1);
     }
     strncpy(flist[fsp].fname,namep, len+1);
     flist[fsp].fline=0;
     flist[fsp].bdepth=b_depth();
     flist[fsp].flinep=NULL;
     flist[fsp].filep=fp2;
     nff=1;

     return(0);
}

int pgetline(char *t)
{
     int c,er=E_OK;
     int rlen, tlen;
     char *p = 0;

     loopfl =0; /* set if additional fetch needed */

     filep =flist+fsp;

     do {
          c=fgetline(in_line, MAXLINE, &rlen, flist[fsp].filep);
	  /* continuation lines */
	  tlen = rlen;
	  while(c=='\n' && tlen && in_line[tlen-1]=='\\') {
	    c=fgetline(in_line + tlen-1, MAXLINE-tlen, &rlen, flist[fsp].filep);
	    tlen += rlen-1;
	  }
          if(in_line[0]=='#' || in_line[0] == altppchar)
          {
		if (in_line[1]==' ') { /* cpp comment -- pp_comand doesn't
					handle this right */
			er=pp_cpp(in_line+1);
		} else {
               if((er=pp_comand(in_line+1)))
               {
                    if(er!=1)
                    {
                         logout(in_line);
                         logout("\n");
                    }
               }
		}
          } else
               er=1;

          if(c==EOF) {
		if (loopfl && fsp) {
			char bletch[MAXLINE];
			sprintf(bletch, 
		"at end of included file %s:\n", flist[fsp].fname);
			logout(bletch);
			errout(W_OPENPP);
		}
               er=icl_close(&c);
	}

     } while(!er || (loopfl && er!=E_EOF));

	if (loopfl) {
		errout(E_OPENPP);
	}

     /* handle the double-slash comment (like in C++) */
     p = strchr(in_line, '/');
     if (p != NULL) {
	if (p[1] == '/') {
	    *p = 0;	/* terminate string */
	}
     }

     if(!er || loopfl) {
          in_line[0]='\0';
     }

     er= (er==1) ? E_OK : er ;

     if(!er) {
#ifdef DEBUG_REPLACE
//	  printf("<<<: %s\n", in_line);
#endif
          er=pp_replace(t,in_line,-1,rlist);
#ifdef DEBUG_REPLACE
	  printf(">>>: %s\n", t);
#endif
     }

     if(!er && nff)
          er=E_NEWFILE;
     if(!er && nlf)
          er=E_NEWLINE;
     nlf=nff=0;

     filep=flist+fsp;
     filep->flinep=in_line;
     
     return(er);
}


/*************************************************************************/

/* this is the most disgusting code I have ever written, but Andre drove me
to it because I can't think of any other way to fix the
last line bug ... a very irritated Cameron */

/* however, it also solved the problem of open #ifdefs not bugging out */

/* #define DEBUG_EGETC */
int egetc(FILE *fp) {
	int c;

	c = getc(fp);
	if (c == EOF) {
		if (ungeteof) {
#ifdef DEBUG_EGETC
			fprintf(stderr, "eof claimed\n");
#endif
			return c;
		} else {
#ifdef DEBUG_EGETC
			fprintf(stderr, "got eof!!\n");
#endif
			ungeteof = 1;
			return '\n';
		}
	}
	ungeteof = 0;
	return c;
}

/* smart getc that can skip C comment blocks */
int rgetc(FILE *fp)
{
     static int c,d,fl;

     fl=0;

     do
     {
          while((c=egetc(fp))==13);  /* remove ^M for unices */

          if(fl && (c=='*'))
          {
               if((d=egetc(fp))!='/')
                    ungetc(d,fp);
               else
               {
                    fl--;
                    while((c=egetc(fp))==13);
               }
          }
          if(c=='\n')
          {
               flist[fsp].fline++;
               nlf=1;
          } else
          if(c=='/')
          {
               if((d=egetc(fp))!='*')
                    ungetc(d,fp);
               else
                    fl++;
          }

     } while(fl && (c!=EOF));

     return(c-'\t'?c:' ');
}

int fgetline(char *t, int len, int *rlen, FILE *fp)
{
     static int c,i;

     i=0;

     do {
          c=rgetc(fp);
          
          if(c==EOF || c=='\n')
          {
             t[i]='\0';
             break;
          }
          t[i]=c;
          i= (i<len-1) ? i+1 : len-1;
     } while(1);

     *rlen = i;
     return(c);
}

