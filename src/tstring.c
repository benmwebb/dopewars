/* tstring.c      "Translated string" wrappers for dopewars             */
/* Copyright (C)  1998-2000  Ben Webb                                   */
/*                Email: ben@bellatrix.pcl.ox.ac.uk                     */
/*                WWW: http://bellatrix.pcl.ox.ac.uk/~ben/dopewars/     */

/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU General Public License          */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */

/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */

/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston,               */
/*                   MA  02111-1307, USA.                               */


#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "dopewars.h"
#include "message.h"
#include "tstring.h"

gchar *GetTranslatedString(gchar *str,gchar *code,gboolean Caps) {
   gchar *dstr,*pt,*tstr,*Default,*tcode;

   dstr=g_strdup(str);
   g_strdelimit(dstr,"_",'^');
   pt=dstr;
   Default=GetNextWord(&pt,"");
   tstr=NULL;

   while(1) {
      tcode=GetNextWord(&pt,NULL);
      tstr=GetNextWord(&pt,"");
      if (!tcode) { tstr=NULL; break; }
      if (strcmp(tcode,code)==0) {
         break;
      } else tstr=NULL;
   }

   if (tstr) {
      if (Caps) tstr=InitialCaps(tstr); else tstr=g_strdup(tstr);
   } else {
      if (Caps) tstr=InitialCaps(Default); else tstr=g_strdup(Default);
   }

   g_free(dstr);
   return tstr;
}

void tstring_fmt(gchar **tformat,gchar ***tstrings,char *OrigFormat, ...) {
   va_list ap;
   GString *text;
   int i;
   gchar *str,*tstr,code[3],**strings;
   GPtrArray *ptrarr;
   gboolean Caps;

   text=g_string_new("");
   va_start(ap,OrigFormat);
   ptrarr=g_ptr_array_new();

   i=0;
   while (i<strlen(OrigFormat)) {
      g_string_append_c(text,OrigFormat[i]);
      if (OrigFormat[i]=='%') {
         i++;
         if ((OrigFormat[i]=='T' || OrigFormat[i]=='t')
             && i+2<strlen(OrigFormat)) {
            Caps = (OrigFormat[i]=='T');
            code[0]=OrigFormat[i+1];
            code[1]=OrigFormat[i+2];
            code[2]='\0';
            i+=3;
            g_string_append_c(text,'s');
            str=va_arg(ap,char *);
            tstr=GetTranslatedString(str,code,Caps);
            g_ptr_array_add(ptrarr,(gpointer)tstr);
         }
      } else i++;
   }
   va_end(ap);
   *tformat=text->str;
   strings=g_new(char *,ptrarr->len+10);
   for (i=0;i<ptrarr->len;i++) {
      strings[i]=(gchar *)g_ptr_array_index(ptrarr,i);
   }
   strings[ptrarr->len]=NULL;
   g_ptr_array_free(ptrarr,FALSE);
   *tstrings=strings;
   g_string_free(text,FALSE);
}

void tstring_free(gchar *tformat,gchar **tstrings) {
   gchar **pt;
   g_free(tformat);
   for (pt=tstrings;*pt;pt++) g_free(*pt);
   g_free(tstrings);
}

void GetNextFormat(int Index,gchar *str,int *StartPos,
                   int *EndPos,int *ArgNum,char *Code,gboolean *Caps) {
   int anum;
   *StartPos=*EndPos=*ArgNum=0;
   Code[0]=0;
   anum=0;
   while (str[Index]) {
      if (str[Index]=='%') {
         *StartPos=*EndPos=Index++;
         while (str[Index]>='0' && str[Index]<='9') {
            anum=anum*10+str[Index]-'0';
            Index++;
         }
         if (str[Index]=='$') {
            *EndPos=Index++; *ArgNum=anum;
         }
         if ((str[Index]=='T' || str[Index]=='t') && Index+2<strlen(str)) {
            *Caps=(str[Index]=='T');
            Code[0]=str[Index+1];
            Code[1]=str[Index+2];
            Code[2]=0;
            *EndPos=Index+2;
         }
         return;
      } else Index++;
   }
}

void GetNextTString(gchar *str,int index,gchar *Code,gboolean *Caps,
                    int *NumArg,int *StartPos,int *EndPos) {
   int i;
   *StartPos=*EndPos=0;
   i=index;
   while (str[i]) {
      if (str[i]=='%') {
         i++;
         if ((str[i]=='T' || str[i]=='t')
             && i+2<strlen(str)) {
            (*NumArg)++;
            *StartPos=i-1;
            *Caps = (str[i]=='T');
            Code[0]=str[i+1];
            Code[1]=str[i+2];
            Code[2]='\0';
            i+=3;
            *EndPos=i;
            return;
         }
      } else i++;
   }
}

int SkipNextTString(gchar *str,int index) {
   gchar Code[3];
   gboolean Caps;
   int NumArg,StartPos,EndPos;
   GetNextTString(str,index,Code,&Caps,&NumArg,&StartPos,&EndPos);
   return EndPos;
}

void SubstNextTString(GString *string,int *NumArg,GPtrArray *strs) {
   gchar Code[3];
   gboolean Caps;
   int StartPos,EndPos;
   gchar *str,*tstr;

   GetNextTString(string->str,0,Code,&Caps,NumArg,&StartPos,&EndPos);
   if (EndPos!=0 && *NumArg>=1 && *NumArg<=strs->len) {
      str=(gchar *)g_ptr_array_index(strs,*NumArg-1);
      tstr=GetTranslatedString(str,Code,Caps);
      g_string_erase(string,StartPos,EndPos-StartPos);
      g_string_insert(string,StartPos,tstr);
      g_free(tstr);
   }
}

gchar *HandleTFmt(gchar *format, va_list args) {
   GString *string;
   gchar *retstr;
   GPtrArray *tstrs;
   int i,numtstr,NumArg;

   string=g_string_new(format);
   tstrs=g_ptr_array_new();
   i=numtstr=0;
   while (1) {
      i=SkipNextTString(string->str,i);
      if (i!=0) numtstr++; else break;
   }
   for (i=0;i<numtstr;i++) {
      g_ptr_array_add(tstrs,(gpointer)va_arg(args,char *));
   }
   NumArg=0;
   for (i=0;i<numtstr;i++) {
      SubstNextTString(string,&NumArg,tstrs);
   }
   retstr=string->str;
   g_ptr_array_free(tstrs,FALSE);
   g_string_free(string,FALSE);
   return retstr;
}

gchar *dpg_strdup_printf(gchar *format, ...) {
   va_list ap;
   gchar *newfmt,*retstr;
   va_start(ap,format);
   newfmt=HandleTFmt(format,ap);
   retstr=g_strdup_vprintf(newfmt,ap);
   g_free(newfmt);
   va_end(ap);
   return retstr;
}

void dpg_string_sprintf(GString *string, gchar *format, ...) {
   va_list ap;
   gchar *newfmt,*retstr;
   va_start(ap,format);
   newfmt=HandleTFmt(format,ap);
   retstr=g_strdup_vprintf(newfmt,ap);
   g_string_assign(string,retstr);
   g_free(newfmt); g_free(retstr);
   va_end(ap);
}

void dpg_string_sprintfa(GString *string, gchar *format, ...) {
   va_list ap;
   gchar *newfmt,*retstr;
   va_start(ap,format);
   newfmt=HandleTFmt(format,ap);
   retstr=g_strdup_vprintf(newfmt,ap);
   g_string_append(string,retstr);
   g_free(newfmt); g_free(retstr);
   va_end(ap);
}
