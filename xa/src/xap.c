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
/* #define DEBUG_RECMAC */

char s[MAXLINE];
Datei *filep;

static int tcompare(char*,char**,int);
static int pp_replace(char*,char*,int,int);
static int searchdef(char*);
static int fgetline(char*,int len, int *rlen, FILE*);

static int icl_open(char*),pp_ifdef(char*),pp_ifndef(char*);
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
	loopfl=(loopfl<<1)+( ll_search(t,&n) ? 1 : 0 );
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
	char name[MAXLINE];

	if(sscanf(t, " %d \"%s\"", &filep->fline, name) == 2) {
		/* massage it into our parameters and drop last quote */
		char *u;

		filep->fline--;
		if((u = (char *)strrchr(name, '"')))
			*u = '\0';

		free(filep->fname);
		filep->fname = strdup(name);
		if(!filep->fname) {
			fprintf(stderr,"Oops, no more memory!\n");
			exit(1);
		}
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
               for(j=0; t[i+j]!=')' && t[i+j]!=',' && t[i+j]!='\0';j++);
               if(j<memfre)
               {
                    strncpy(mem,t+i,j);
                    mem+=j;
		    *mem++='\0';
                    memfre-=j+1;
               }
               i+=j;
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

int pp_replace(char *to, char *ti, int a,int b)
{
     char *t=to,c,*x,*y,*mx,*rs;
     int i,l,n,sl,d,ld,er=E_OK,hkfl,klfl;
     char fti[MAXLINE],fto[MAXLINE];
/*
     int flag=!strncmp(ti,"TOUT",4);
     if(flag) printf("flag=%d\n",flag);
*/   
     (void)strcpy(t,ti);

     if(rlist)
     {
       while(t[0]!='\0')
       {
 	  /* find start of a potential token to be replaced */
          while(!isalpha(t[0]) && t[0]!='_') {
	      
	       /* escape strings quoted with " */ 
	       if (!ppinstr && t[0] == '\"') {
		    do {
			t++;
			ti++;
		    } while (t[0] && t[0]!='\"');
	       }

	       /* escape strings quoted with ' */ 
	       if (!ppinstr && t[0] == '\'') {
		    do {
			t++;
			ti++;
		    } while (t[0] && t[0]!='\'');
	       }

               if(t[0]=='\0')
                    break;    /*return(E_OK);*/
               else
               {
                    t++;
                    ti++;
               }
	  }
         
          for(l=0;isalnum(t[l])||t[l]=='_';l++);
          ld=l;
/*          
          if(flag) printf("l=%d,a=%d,t=%s\n",l,a,t);
*/        
          if(a<0)
          {
            n=hashindex[hashcode(t,l)];
            
            do      
            {
               sl=liste[n].s_len;
          
               if(sl && (sl==l))
               {
                    i=0;
                    x=liste[n].search;
                    while(t[i]==*x++ && t[i])
                         i++;

                    if(i==sl)
                    {     
                         rs=liste[n].replace;
                         
                         if(liste[n].p_anz)        
                         {
                              (void)strcpy(fti,liste[n].replace);

                              if(rlist+liste[n].p_anz>=ANZDEF || memfre<MAXLINE*2)
                                   er=E_NOMEM;
                              else
                              {
                                   y=t+sl;
                                   x=liste[n].search+sl+1;
                                   if(*y!='(')
                                        er=E_SYNTAX;
                                   else
                                   {
                                        mx=mem-1;
                                        for(i=0;i<liste[n].p_anz;i++)
                                        {
                                             liste[rlist+i].search=x;
                                             liste[rlist+i].s_len=(int)strlen(x);
                                             x+=strlen(x)+1;
                                             liste[rlist+i].p_anz=0;
                                             liste[rlist+i].replace=mx+1;
                                             c=*(++mx)=*(++y);
                                             hkfl=klfl=0;
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
                                             *mx='\0';
                                             if(c!=((i==liste[n].p_anz-1) ? ')' : ','))
                                             {
                                                  er=E_ANZPAR;
                                                  break;
                                             }
                                        }   
#ifdef DEBUG_RECMAC
     printf("replace:\n");
     printf("%s=%s\n",liste[n].search,liste[n].replace);
#endif
     for(i=0;i<liste[n].p_anz;i++) {
/* recursively evaluate arguments */
	char nfto[MAXLINE];
	char nfwas[MAXLINE];
	int j = 0;
	int k;

	(void)strcpy(nfwas, liste[rlist+i].replace);
	if (!er)
		er=pp_replace(nfto,nfwas,-1,rlist);
#ifdef DEBUG_RECMAC
        printf("-%s=%s\n",liste[rlist+i].search,liste[rlist+i].replace);
	printf("SUBB: -%s=%s\n", nfwas, nfto);
#endif
	while ((k = strcmp(nfto, nfwas))) {
		(void)strcpy(nfto, nfwas);
		if (!er)
			er=pp_replace(nfto,nfwas,-1,rlist);
		(void)strcpy(nfwas, nfto);
#ifdef DEBUG_RECMAC
		printf("SUBB2 (%i): -%s=%s\n", k, liste[rlist+i].replace, nfto);
#endif
		if (++j>10)
			errout(E_ORECMAC);
	}
	(void)strcpy(liste[rlist+i].replace, nfto);
#ifdef DEBUG_RECMAC
        printf("FINAL: -%s=%s\n",liste[rlist+i].search,liste[rlist+i].replace);
#endif
    }
                                        if(!er)
                                             er=pp_replace(fto,fti,rlist,rlist+i);
/*               if(flag) printf("sl=%d,",sl);*/
                                        sl=(int)((long)y+1L-(long)t);
/*               if(flag) printf("sl=%d\n",sl);*/
                                        rs=fto;
/*     printf("->%s\n",fto);*/
                                   }    
                              }
                              if(er)
                                   return(er);     
                         }

                         d=(int)strlen(rs)-sl;

                         if(strlen(to)+d>=MAXLINE)
                              return(E_NOMEM);

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
                         if(d)
                              (void)strcpy(t+sl+d,ti+sl);

                         i=0;
                         while((c=rs[i]))
                              t[i++]=c;
                         l=sl+d;/*=0;*/
                         break;
                    }
               }
               if(!n)
                    break;
                    
               n=liste[n].nextindex;
               
            } while(1);
          } else
          {
            for(n=b-1;n>=a;n--)
            {
               sl=liste[n].s_len;
          
               if(sl && (sl==l))
               {
                    i=0;
                    x=liste[n].search;
                    while(t[i]==*x++ && t[i])
                         i++;

                    if(i==sl)
                    {     
                         rs=liste[n].replace;
                         if(liste[n].p_anz)        
                         {
                              (void)strcpy(fti,liste[n].replace);
                              if(rlist+liste[n].p_anz>=ANZDEF || memfre<MAXLINE*2)
                                   er=E_NOMEM;
                              else
                              {
                                   y=t+sl;
                                   x=liste[n].search+sl+1;
                                   if(*y!='(')
                                        er=E_SYNTAX;
                                   else
                                   {
                                        mx=mem-1;
                                        for(i=0;i<liste[n].p_anz;i++)
                                        {
                                             liste[rlist+i].search=x;
                                             liste[rlist+i].s_len=strlen(x);
                                             x+=strlen(x)+1;
                                             liste[rlist+i].p_anz=0;
                                             liste[rlist+i].replace=mx+1;
                                             c=*(++mx)=*(++y);
                                             hkfl=klfl=0;
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
                                             *mx='\0';
                                             if(c!=((i==liste[n].p_anz-1) ? ')' : ','))
                                             {
                                                  er=E_ANZPAR;
                                                  break;
                                             }  
                                        }   
                                        if(!er)
                                             er=pp_replace(fto,fti,0,rlist+i);
                                        sl=(int)((long)y+1L-(long)t);
                                        rs=fto;
                                   }    
                              }
                              if(er)
                                   return(er);     
                         }
                         d=(int)strlen(rs)-sl;

                         if(strlen(to)+d>=MAXLINE)
                              return(E_NOMEM);
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
                         if(d)
                              (void)strcpy(t+sl+d,ti+sl);
                              
                         i=0;
                         while((c=rs[i]))
                              t[i++]=c;
                         l+=d;/*0;*/
                         break;
                    }
               }
            }
          }
          ti+=ld;
          t+=l;
       }
     }
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

     /* we have to alloc it dynamically to make the name survive another
 	pp_open - it's used in the cross-reference list */	
     flist[0].fname = malloc(strlen(name)+1);
     if(!flist[0].fname) {
	fprintf(stderr,"Oops, no more memory!\n");
	exit(1);
     }	
     (void)strcpy(flist[0].fname,name);
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

     /* we have to alloc it dynamically to make the name survive another
        pp_open - it's used in the cross-reference list */
     flist[fsp].fname = malloc(strlen(s+i)+1);
     if(!flist[fsp].fname) {
        fprintf(stderr,"Oops, no more memory!\n");
        exit(1);
     }
     strcpy(flist[fsp].fname,s+i);
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

     if(!er)
          er=pp_replace(t,in_line,-1,rlist);

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
to it because I can't think of any other F$%Y#*U(%&Y##^#KING way to fix the
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

