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
