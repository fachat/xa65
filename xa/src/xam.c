/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 *
 * Memory manager/malloc() stub module
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

#include "xah.h"         /*   structs        */

static int ninc = 0;
static char **nip = NULL;
 
void reg_include(char *path) {
	char **nip2;
	if(path && *path) {
	  nip2 = realloc(nip,sizeof(char*)*(ninc+1));
	  if(nip2) {
	    nip = nip2;
	    nip[ninc++] = path;
	  } else {
	    fprintf(stderr,"Warning: couldn' alloc mem (reg_include)\n");
	  }
	}
}

FILE *xfopen(const char *fn,const char *mode)
{
	FILE *file;
	char c,*cp,n[MAXLINE],path[MAXLINE];
	char xname[MAXLINE], n2[MAXLINE];
	int i,l=(int)strlen(fn);

	if(l>=MAXLINE) {
	  fprintf(stderr,"filename '%s' too long!\n",fn);
	  return NULL;
	}

	for(i=0;i<l+1;i++) {
	  xname[i]=((fn[i]=='\\')?DIRCHAR:fn[i]);
	}

	if(mode[0]=='r')
	{
	    if((file=fopen(fn,mode))==NULL 
			&& (file=fopen(xname, mode))==NULL) {
		for(i=0;(!file) && (i<ninc);i++) {
		  strcpy(n,nip[i]);
		  c=n[(int)strlen(n)-1];
		  if(c!=DIRCHAR) strcat(n,DIRCSTRING);
		  strcpy(n2,n);
		  strcat(n2,xname);
		  strcat(n,fn);
/* printf("name=%s,n2=%s,mode=%s\n",n,n2,mode); */
		  file=fopen(n,mode);
		  if(!file) file=fopen(n2,mode);
		}
		if((!file) && (cp=getenv("XAINPUT"))!=NULL)
		{
			strcpy(path,cp);
			cp=strtok(path,",");
			while(cp && !file)
			{
				if(cp[0])
				{
					strcpy(n,cp);
					c=n[(int)strlen(n)-1];
					if(c!=DIRCHAR&&c!=':')
						strcat(n,DIRCSTRING);
					strcpy(n2,n);
					strcat(n2,xname);
					strcat(n,fn);
/* printf("name=%s,n2=%s,mode=%s\n",n,n2,mode); */
					file=fopen(n,mode);
					if(!file) file=fopen(n2,mode);
				}
				cp=strtok(NULL,",");
			}
		}
	    }
	} else
	{
		if((cp=getenv("XAOUTPUT"))!=NULL)
		{
			strcpy(n,cp);
			if(n[0])
			{
				c=n[(int)strlen(n)-1];
				if(c!=DIRCHAR&&c!=':')
					strcat(n,DIRCSTRING);
			}
			cp=strrchr(fn,DIRCHAR);
			if(!cp)
			{
				cp=strrchr(fn,':');
				if(!cp)
					cp=(char*)fn;
				else
					cp++;
			} else
				cp++;
			strcat(n,cp);
			file=fopen(n,mode);
		} else
			file=fopen(fn,mode);
	}
	if(file)
		setvbuf(file,NULL,_IOFBF,BUFSIZE);

	return(file);		
}

#if 0

static char *m_base;
static char *m_act;
static char *m_end;

int m_init(void) 
{
     int er=E_NOMEM;

     m_base=m_end=m_act=0L;
/*
     fprintf(stderr, "MEMLEN=%ld\n",MEMLEN);
     getchar();
*/    
/*
     if ((m_base=(char*)malloc(MEMLEN))!=NULL)
     {
          m_end =m_base+MEMLEN;
          m_act =(char*)(((long)m_base+3)&0xfffffffcl);
          er=E_OK;
     }
     else m_base=NULL;
*/
    er=E_OK;

     return(er);
}

void m_exit(void)
{
/*
	free(m_base);
*/
}

int m_alloc(long n, char **adr)
{
     int er=E_NOMEM;

     if((*adr=calloc(n,1))) {
	er=E_OK;
     }
/*
     if(m_act+n<m_end)
     {
          *adr=m_act;
          m_act=m_act+n;
          er=E_OK;
     }
*/
/*
     fprintf(stderr, "m_alloc n=%ld adr=%lx\n",n,*adr);
     getchar();
*/     
     return(er);
}

#endif
