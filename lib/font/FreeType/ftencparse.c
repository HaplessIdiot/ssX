/*
Copyright (c) 1998 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* $XFree86$ */

/* This code is ASCII-dependent */

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "fntfilio.h"
#include "fntfilst.h"

#include "freetype.h"
#include "ft.h"
#include "ftenc.h"

#define EOF_TOKEN -1
#define ERROR_TOKEN -2
#define EOL_TOKEN 0
#define NUMBER_TOKEN 1
#define KEYWORD_TOKEN 2

#define EOF_LINE -1
#define ERROR_LINE -2
#define STARTENCODING_LINE 1
#define STARTMAPPING_LINE 2
#define ENDMAPPING_LINE 3
#define CODE_LINE 4
#define CODE_UNDEFINE_LINE 5
#define SIZE_LINE 6

/* Return from lexer */
#define MAXKEYWORDLEN 100

static long number_value;
static char keyword_value[MAXKEYWORDLEN+1];

static long value1, value2;

/* Lexer code */

/* Skip to the beginning of new line */
static void
skipEndOfLine(FontFilePtr f, int c)
{
  if(c==0)
    c==FontFileGetc(f);
  
  for(;;)
    if(c<=0 || c=='\n')
      return;
    else
      c=FontFileGetc(f);
}

/* Get a number; we're at the first digit. */
static long 
getnum(FontFilePtr f, int c, int *cp)
{
  long n=0;
  int base=10;

  /* look for `0' or `0x' prefix */
  if(c=='0') {
    c=FontFileGetc(f);
    base=8;
    if(c=='x' || c=='X') {
      base=16;
      c=FontFileGetc(f);
    }
  }

  /* accumulate digits */
  for(;;) {
    if('0'<=c && c<='9') {
      n*=base; n+=c-'0';
    } else if('a'<=c && c<='f') {
      n*=base; n+=c-'a'+10;
    } else if('A'<=c && c<='F') {
      n*=base; n+=c-'A'+10;
    } else
      break;
    c=FontFileGetc(f);
  }

  *cp=c; return n;
}
 
/* Skip to beginning of new line; return 1 if only whitespace was found. */
static int
endOfLine(FontFilePtr f, int c)
{
  if(c==0)
    c=FontFileGetc(f);

  for(;;) {
    if(c<=0 || c=='\n')
      return 1;
    else if(c=='#') {
      skipEndOfLine(f,c);
      return 1;
    }
    else if(!isspace(c)) {
      skipEndOfLine(f,c);
      return 0;
    }
    c=FontFileGetc(f);
  }
}

/* Get a token; we're at first char */
static int
gettoken(FontFilePtr f, int c, int *cp)
{
  char *p;

  if(c<=0)
    c=FontFileGetc(f);

  if(c<=0) {
    return EOF_TOKEN;
  }

  while(isspace(c) && c!='\n')
    c=FontFileGetc(f);

  if(c=='\n') {
    return EOL_TOKEN;
  } else if(c=='#') {
    skipEndOfLine(f,c);
    return EOL_TOKEN;
  } else if(isdigit(c)) {
    number_value=getnum(f,c,cp);
    return NUMBER_TOKEN;
  } else if(isalpha(c) || c=='/' || c=='_' || c=='-' || c=='.') {
    p=keyword_value;
    *p++=c;
    while(p-keyword_value<MAXKEYWORDLEN) {
      c=FontFileGetc(f);
      if(c<=0 || isspace(c) || c=='#')
        break;
      *p++=c;
    }
    *cp=c;
    *p='\0';
    return KEYWORD_TOKEN;
  } else {
    *cp=c;
    return ERROR_TOKEN;
  }
}

/* Parse a line.
 * Always skips to the beginning of a new line, even if an error occurs */
static int
getline(FontFilePtr f)
{
  int c, token, i;
  c=FontFileGetc(f);
  if(c<=0)
    return EOF_LINE;

retry:
  token=gettoken(f,c,&c);

  switch(token) {
  case EOF_TOKEN:
    return EOF_LINE;
  case EOL_TOKEN:
    /* empty line */
    c=FontFileGetc(f);
    goto retry;
  case NUMBER_TOKEN:
    value1=number_value;
    token=gettoken(f,c,&c);
    if(token!=NUMBER_TOKEN) {
      skipEndOfLine(f,c);
      return ERROR_LINE;
    }
    value2=number_value;
    if(!endOfLine(f,c))
      return ERROR_LINE;
    else
      return CODE_LINE;
  case KEYWORD_TOKEN:
    if(!strcasecmp(keyword_value, "STARTENCODING")) {
      token=gettoken(f,c,&c);
      if(token==KEYWORD_TOKEN) {
        if(endOfLine(f,c))
          return STARTENCODING_LINE;
        else
          return ERROR_LINE;
      } else {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "SIZE")) {
      token=gettoken(f,c,&c);
      if(token==NUMBER_TOKEN) {
        value1=number_value;
        if(endOfLine(f,c))
          return SIZE_LINE;
        else
          return ERROR_LINE;
      } else {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "STARTMAPPING")) {
      keyword_value[0]=0;
      value1=0; value1=0;
      /* read up to three tokens, the first being a keyword */
      for(i=0; i<3; i++) {
        token=gettoken(f,c,&c);
        if(token==EOL_TOKEN) {
          if(i>0)
            return STARTMAPPING_LINE;
          else
            return ERROR_LINE;
        } else if(token==KEYWORD_TOKEN) {
          if(i!=0) {
            skipEndOfLine(f,c);
            return ERROR_LINE;
          }
        } else if(token==NUMBER_TOKEN) {
          if(i==1) value1=number_value;
          else if(i==2) value2=number_value;
          else {
            skipEndOfLine(f,c);
            return ERROR_LINE;
          }
        } else {
          skipEndOfLine(f,c);
          return ERROR_LINE;
        }
      }
      if(!endOfLine(f,c))
         return ERROR_LINE;
      else {
        return STARTMAPPING_LINE;
      }
    } else if(!strcasecmp(keyword_value, "UNDEFINE")) {
      token=gettoken(f,c,&c);
      if(token!=NUMBER_TOKEN) {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
      value1=number_value;
      token=gettoken(f,c,&c);
      if(token==EOL_TOKEN) {
        value2=value1;
        return CODE_UNDEFINE_LINE;
      } else if(token==NUMBER_TOKEN) {
        value2=number_value;
        if(endOfLine(f,c)) {
          return CODE_UNDEFINE_LINE;
        } else
          return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "ENDENCODING")) {
      if(endOfLine(f,c))
        return EOF_LINE;
      else
        return ERROR_LINE;
    } else if(!strcasecmp(keyword_value, "ENDMAPPING")) {
      if(endOfLine(f,c))
        return ENDMAPPING_LINE;
      else
        return ERROR_LINE;
    } else {
      skipEndOfLine(f,c);
      return ERROR_LINE;
    }
  default:
    return ERROR_LINE;
  }
}

static void 
install_alternative(struct ttf_encoding_info *info,
                    struct ttf_encoding_alternative *alt)
{
  struct ttf_encoding_alternative *a=info->alternatives;

  if(a==NULL)
    info->alternatives=alt;
  else {
    while(a->next!=NULL)
      a=a->next;
    a->next=alt;
  }
  alt->next=NULL;
}

/* Parser. */
static struct ttf_encoding_info*
parseEncodingFile(FontFilePtr f)
{
  int line;

  unsigned short *encoding=NULL;
  unsigned i, first=0xFFFF, last=0, encsize=0;
  struct ttf_encoding_info *info=NULL;
  struct ttf_encoding_alternative *alt=NULL;
  struct ttf_simple_remapping *mapping;

no_encoding:
  line=getline(f);
  switch(line) {
  case EOF_LINE:
    goto done;
  case STARTENCODING_LINE:
    if((info=
        (struct ttf_encoding_info*)xalloc(sizeof(struct ttf_encoding_info)))
       ==NULL)
      goto error;
    if((info->charset=(char*)xalloc(strlen(keyword_value)+1))==NULL)
      goto error;
    strcpy(info->charset, keyword_value);
    info->size=256;
    info->alternatives=NULL;
    info->next=NULL;
    goto no_mapping;
  default:
    goto no_encoding;           /* ignore unknown lines */
  }

no_mapping:
  line=getline(f);
  switch(line) {
  case EOF_LINE: goto done;
  case SIZE_LINE:
    info->size=value1; 
    goto no_mapping;
  case STARTMAPPING_LINE:
    if(!strcasecmp(keyword_value, "unicode")) {
      if((alt=
          (struct ttf_encoding_alternative*)
          xalloc(sizeof(struct ttf_encoding_alternative)))
         ==NULL)
        goto error;
      alt->pid = -2;
      alt->eid = 0;
    } else if(!strcasecmp(keyword_value, "cmap")) {
      if((alt=
          (struct ttf_encoding_alternative*)
          xalloc(sizeof(struct ttf_encoding_alternative)))
         ==NULL)
        goto error;
      alt->pid = value1;
      alt->eid = value2;
      goto mapping;
    } else {                    /* unknown mapping type -- ignore */
      MUMBLE("Unknown mapping type\n");
      goto skipmapping;
    }
    goto mapping;
  default: goto no_mapping;     /* ignore unknown lines */
  }

skipmapping:
  line=getline(f);
  switch(line) {
  case ENDMAPPING_LINE:
    goto no_mapping;
  case EOF_LINE:
    goto error;
  default:
    goto skipmapping;
  }
    

mapping:
  line=getline(f);
  switch(line) {
  case EOF_LINE: goto error;
  case ENDMAPPING_LINE:
    alt->recode=ttf_simple_remap;
    if((alt->client_data=mapping=
        (struct ttf_simple_remapping*)
        xalloc(sizeof(struct ttf_simple_remapping)))==NULL)
      goto error;
    mapping->first=first;
    mapping->len=last-first+1;
    if((mapping->map=
        (unsigned short*)xalloc(mapping->len*sizeof(unsigned short)))
       ==NULL) {
      xfree(mapping);
      alt->client_data=mapping=NULL;
      goto error;
    }
    for(i=0; i<mapping->len; i++)
      mapping->map[i]=encoding[first+i];
    install_alternative(info,alt);
    alt=0;
    first=0xFFFF; last=0;
    goto no_mapping;
  case CODE_LINE:
    /* Optimize away useless identity mappings */
    if(value1==value2 && (value1<first || value1>last))
      goto mapping;
    if(encsize==0) {
      encsize=value1<256?256:0x10000;
      if((encoding=
          (unsigned short*)xalloc(encsize*sizeof(unsigned short)))==NULL)
        goto error;
    } else if(encsize<=value1) {
      encsize=0x10000;
      encoding=(unsigned short*)xrealloc(encoding, encsize);
    }
    if(first>last) {
      first=last=value1;
    }
    if(value1<first) {
      for(i=value1; i<first; i++)
        encoding[i]=i;
      first=value1;
    }
    if(value1>last) {
      for(i=last+1; i<=value1; i++)
        encoding[i]=i;
      last=value1;
    }
    encoding[value1]=value2;
    goto mapping;
  case CODE_UNDEFINE_LINE:
    if(value2<value1) {
      i=value1; value1=value2; value2=i;
    }
    if(encsize==0) {
      encsize=value2<256?256:0x10000;
      if((encoding=
          (unsigned short*)xalloc(encsize*sizeof(unsigned short)))==NULL)
        goto error;
    } else if(encsize<=value2) {
      encsize=0x10000;
      encoding=(unsigned short*)xrealloc(encoding, encsize);
    }
    if(first>last) {
      first=value1;
      last=value2;
    }
    if(value1<first) {
      first=value1;
    }
    if(value1>last) {
      last=value2;
    }
    for(i=value1; i<=value2; i++) {
      encoding[i]=0;
    }
    goto mapping;

  default: goto mapping;        /* ignore unknown lines */
  }

done:
  if(encsize) xfree(encoding);
  return info;

error:
  if(encsize) xfree(encoding);
  if(alt) {
    if(alt->client_data) xfree(alt->client_data);
    xfree(alt);
  }
  if(info) {
    if(info->charset) xfree(info->charset);
    for(alt=info->alternatives; alt; alt=alt->next) {
      if(alt->client_data) xfree(alt->client_data);
      xfree(alt);
    }
    xfree(info);
  }
  return 0;
}

/* Public entrypoint */  
struct ttf_encoding_info* 
loadEncodingFile(char *charset, char *fontFileName)
{
  FontFilePtr f;
  FILE *file;
  struct ttf_encoding_info *info;
  char dir[MAXFONTNAMELEN], buf[MAXFONTNAMELEN],
    file_name[MAXFONTNAMELEN], encoding_name[MAXFONTNAMELEN],
    *p, *q, *lastslash;
  int count, n;

  for(p=fontFileName, q=dir, lastslash=NULL; *p; p++, q++) {
    *q=*p;
    if(*p=='/')
      lastslash=q+1;
  }

  if(!lastslash)
    lastslash=dir;

  *lastslash='\0';

  /* As we don't really expect to open encodings that often, we don't
   * take the trouble of caching encodings directories. */

  strcpy(buf, dir);
  strcat(buf, "encodings.dir");

  if((file=fopen(buf, "r"))==NULL) {
    return NULL;
  }

  count=fscanf(file, "%d\n", &n);
  if(count==EOF || count!=1) {
    fclose(file);
    return NULL;
  }

  info=NULL;
  for(;;) {
    if((count=fscanf(file, "%s %[^\n]\n", encoding_name, file_name))==EOF)
      break;
    if(count!=2)
      break;
    if(!strcasecmp(encoding_name, charset)) {
      strcpy(buf, dir);
      strcat(buf, file_name);
      if((f=FontFileOpen(buf))==NULL) {
        return NULL;
      }
      info=parseEncodingFile(f);
      FontFileClose(f);
      break;
    }
  }

  fclose(file);

  return info;
}

