/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 *
 * Preprocessing arithmetic module
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

#include "xah.h"

#include "xad.h"
#include "xar.h"
#include "xa.h"
#include "xal.h"
#include "xaa.h"
#include "xat.h"

static int pr[]= { P_START,P_ADD,P_ADD,P_MULT,P_MULT,P_SHIFT,P_SHIFT,P_CMP,
            P_CMP,P_EQU,P_CMP,P_CMP,P_EQU,P_AND,P_XOR,P_OR,
            P_LAND,P_LOR };

static int pp,pcc;
static int fundef;

static int ag_term(signed char*,int,int*,int*,int*);
static int get_op(signed char*,int*);
static int do_op(int*,int,int);

/* s = string, v = variable */
int a_term(signed char *s, int *v, int *l, int xpc, int *pfl, int *label, int f)
{
     int er=E_OK;
     int afl = 0, bfl;

     *pfl = 0;
     fundef = f;

     pp=0;
     pcc=xpc;

     if(s[0]=='<')
     {
          pp++;
          er=ag_term(s,P_START,v,&afl, label);
	  bfl = afl & (A_MASK>>8);
	  if( bfl && (bfl != (A_ADR>>8)) && (bfl != (A_LOW>>8)) ) {
/*fprintf(stderr,"low byte relocation for a high byte - won't work!\n");*/
	    errout(W_LOWACC);
	  }
	  if(afl) *pfl=A_LOW | ((afl<<8) & A_FMASK);
          *v = *v & 255;
     } else
     if(s[pp]=='>')
     {    
          pp++;
          er=ag_term(s,P_START,v,&afl, label);
	  bfl = afl & (A_MASK>>8);
	  if( bfl && (bfl != (A_ADR>>8)) && (bfl != (A_HIGH>>8)) ) {
/*fprintf(stderr,"high byte relocation for a low byte - won't work!\n");*/
	    errout(W_HIGHACC);
	  }
	  if(afl) *pfl=A_HIGH | ((afl<<8) & A_FMASK) | (*v & 255);
          *v=(*v>>8)&255;
     }
     else {
          er=ag_term(s,P_START,v,&afl, label);
	  bfl = afl & (A_MASK>>8);
	  if(bfl && (bfl != (A_ADR>>8)) ) {
/*fprintf(stderr,"address relocation for a low or high byte - won't work!\n");*/
	    errout(W_ADDRACC);
	  }
	  if(afl) *pfl = A_ADR | ((afl<<8) & A_FMASK);
     }

     *l=pp;
/* printf("a_term: afl->%04x *pfl=%04x, (pc=%04x)\n",afl,*pfl, xpc); */
     return(er);
}

static int ag_term(signed char *s, int p, int *v, int *nafl, int *label)
{
     int er=E_OK,o,w,mf=1,afl;

     afl = 0;

/*
printf("ag_term(%02x %02x %02x %02x %02x %02x\n",s[0],s[1],s[2],s[3],s[4],s[5]);
*/
     while(s[pp]=='-')
     {
          pp++;
          mf=-mf;
     }

     if(s[pp]=='(')
     {
          pp++;
          if(!(er=ag_term(s,P_START,v,&afl,label)))
          {
               if(s[pp]!=')')
                    er=E_SYNTAX;
               else 
                    pp++;
          }
     } else
     if(s[pp]==T_LABEL)
     {
          er=l_get(cval(s+pp+1),v, &afl);
/*
 printf("label: er=%d, seg=%d, afl=%d, nolink=%d, fundef=%d\n", 
			er, segment, afl, nolink, fundef);
*/
	  if(er==E_NODEF && segment != SEG_ABS && fundef ) {
	    if( nolink || (afl==SEG_UNDEF)) {
	      er = E_OK;
	      *v = 0;
	      afl = SEG_UNDEF;
	      *label = cval(s+pp+1);
	    }
	  }
          pp+=3;
     }
     else
     if(s[pp]==T_VALUE)
     {
          *v=lval(s+pp+1);
          pp+=4;
/*
printf("value: v=%04x\n",*v);
*/
     }
     else
     if(s[pp]==T_POINTER)
     {
	  afl = s[pp+1];
          *v=cval(s+pp+2);
          pp+=4;
/*
printf("pointer: v=%04x, afl=%04x\n",*v,afl);
*/
     }
     else
     if(s[pp]=='*')
     {
          *v=pcc;
          pp++;
	  afl = segment;
     }
     else {
          er=E_SYNTAX;
	}

     *v *= mf;

     while(!er && s[pp]!=')' && s[pp]!=']' && s[pp]!=',' && s[pp]!=T_END)
     {
          er=get_op(s,&o);

          if(!er && pr[o]>p)
          {
               pp+=1;
               if(!(er=ag_term(s,pr[o],&w, nafl, label)))
               {
		    if(afl || *nafl) {	/* check pointer arithmetic */
		      if((afl == *nafl) && (afl!=SEG_UNDEF) && o==2) {
			afl = 0; 	/* substract two pointers */
		      } else 
		      if(((afl && !*nafl) || (*nafl && !afl)) && o==1) {
			afl=(afl | *nafl);  /* add constant to pointer */
		      } else 
		      if((afl && !*nafl) && o==2) {
			afl=(afl | *nafl);  /* substract constant from pointer */
		      } else {
                        /* allow math in the same segment */
			if(segment!=SEG_ABS && segment != afl) { 
			  if(!dsb_len) {
			    er=E_ILLSEGMENT;
			  }
			}
			afl=0;
		      }
		    }
                    if(!er) er=do_op(v,w,o);
               }
          } else {
               break;
	  }
     }
     *nafl = afl;
     return(er);
}

static int get_op(signed char *s, int *o)
{
     int er;

     *o=s[pp];

     if(*o<1 || *o>17)
          er=E_SYNTAX;
     else
          er=E_OK;

     return(er);
}
         
static int do_op(int *w,int w2,int o)
{
     int er=E_OK;
     switch (o) {
     case 1:
          *w +=w2;
          break;
     case 2:
          *w -=w2;
          break;
     case 3:
          *w *=w2;
          break;
     case 4:
          if (w2!=0)
               *w /=w2;
          else
               er =E_DIV;
          break;
     case 5:
          *w >>=w2;
          break;
     case 6:
          *w <<=w2;
          break;
     case 7:
          *w = *w<w2;
          break;
     case 8:
          *w = *w>w2;
          break;
     case 9:
          *w = *w==w2;
          break;
     case 10:
          *w = *w<=w2;
          break;
     case 11:
          *w = *w>=w2;
          break;
     case 12:
          *w = *w!=w2;
          break;
     case 13:
          *w &=w2;
          break;
     case 14:
          *w ^=w2;
          break;
     case 15:
          *w |=w2;
          break;
     case 16:
          *w =*w&&w2;
          break;
     case 17:
          *w =*w||w2;
          break;
     }
     return(er);
}

