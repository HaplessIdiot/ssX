/* $XConsortium: maprules.c /main/4 1996/03/01 14:30:26 kaleb $ */
/************************************************************
 Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be 
 used in advertising or publicity pertaining to distribution 
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability 
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.
 
 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include <stdio.h>
#include <ctype.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif

#include <X11/Xos.h>
#include <X11/Xfuncs.h>

#ifndef XKB_IN_SERVER

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include "XKMformat.h"
#include "XKBfileInt.h"

#else

#include "X.h"
#define	NEED_EVENTS
#include <X11/keysym.h>
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "dix.h"
#include "XKBstr.h"
#define XKBSRV_NEED_FILE_FUNCS	1
#include "XKBsrv.h"

#endif

/***====================================================================***/

#define	DFLT_LINE_SIZE	128

typedef struct {
	int	line_num;
	int	sz_line;
	int	num_line;
	char	buf[DFLT_LINE_SIZE];
	char *	line;
} InputLine;

static void
#if NeedFunctionPrototypes
InitInputLine(InputLine *line)
#else
InitInputLine(line)
    InputLine *	line;
#endif
{
    line->line_num= 1;
    line->num_line= 0;
    line->sz_line= DFLT_LINE_SIZE;
    line->line=	line->buf;
    return;
}

static void
#if NeedFunctionPrototypes
FreeInputLine(InputLine *line)
#else
FreeInputLine(line)
    InputLine *line;
#endif
{
    if (line->line!=line->buf)
	_XkbFree(line->line);
    line->line_num= 1;
    line->num_line= 0;
    line->sz_line= DFLT_LINE_SIZE;
    line->line= line->buf;
    return;
}

int
#if NeedFunctionPrototypes
InputLineAddChar(InputLine *line,int ch)
#else
InputLineAddChar(line,ch)
    InputLine *	line;
    int		ch;
#endif
{
    if (line->num_line>=line->sz_line) {
	if (line->line==line->buf) {
	    line->line= _XkbAlloc(line->sz_line*2);
	    memcpy(line->line,line->buf,line->sz_line);
	}
	else {
	    line->line= _XkbRealloc(line->line,line->sz_line*2);
	}
	line->sz_line*= 2;
    }
    line->line[line->num_line++]= ch;
    return ch;
}

#define	ADD_CHAR(l,c)	((l)->num_line<(l)->sz_line?\
				(int)((l)->line[(l)->num_line++]= (c)):\
				InputLineAddChar(l,c))

static Bool
#if NeedFunctionPrototypes
GetInputLine(FILE *file,InputLine *line)
#else
GetInputLine(file,line)
    FILE *	file;
    InputLine *	line;
#endif
{
int	ch;
Bool	endOfFile,spacePending,slashPending,inComment;

     endOfFile= False;
     while ((!endOfFile)&&(line->num_line==0)) {
	spacePending= slashPending= inComment= False;
	while (((ch=getc(file))!='\n')&&(ch!=EOF)) {
	    if (ch=='\\') {
		if ((ch=getc(file))==EOF)
		    break;
		if (ch=='\n') {
		    inComment= False;
		    ch= ' ';
		    line->line_num++;
		}
	    }
	    if (inComment)
		continue;
	    if (ch=='/') {
		if (slashPending) {
		    inComment= True;
		    slashPending= False;
		}
		else {
		    slashPending= True;
		}
		continue;
	    }
	    else if (slashPending) {
		if (spacePending) {
		    ADD_CHAR(line,' ');
		    spacePending= False;
		}
		ADD_CHAR(line,'/');
		slashPending= False;
	    }
	    if (isspace(ch)) {
		while (isspace(ch)&&(ch!='\n')&&(ch!=EOF)) {
		    ch= getc(file);
		}
		if (ch==EOF)
		    break;
		if ((ch!='\n')&&(line->num_line>0))
		    spacePending= True;
		ungetc(ch,file);
	    }
	    else {
		if (spacePending) {
		    ADD_CHAR(line,' ');
		    spacePending= False;
		}
		if (ch=='!') {
		    if (line->num_line!=0) {
#ifdef DEBUG
			fprintf(stderr,"The '!' legal only at start of line\n");
			fprintf(stderr,"Line containing '!' ignored\n");
#endif
			line->num_line= 0;
			inComment= 0;
			break;
		    }

		}
		ADD_CHAR(line,ch);
	    }
	}
	line->num_line++;
	endOfFile= (ch==EOF);
     }
     if ((line->num_line==0)&&(endOfFile))
	return False;
      ADD_CHAR(line,'\0');
      return True;
}

/***====================================================================***/

#define	MODEL		0
#define	LANGUAGE	1
#define	KEYCODES	2
#define SYMBOLS		3
#define	TYPES		4
#define	COMPAT		5
#define	GEOMETRY	6
#define	KEYMAP		7
#define	MAX_WORDS	8

static	char *	cname[MAX_WORDS] = {
	"model", "language", "keycodes", "symbols", "types", "compat",
			   "geometry", "keymap"
};

typedef	struct _RemapSpec {
	int			num_remap;
	int			remap[MAX_WORDS];
} RemapSpec;

typedef struct _FileSpec {
	char *			name[MAX_WORDS];
	struct _FileSpec *	pending;
} FileSpec;

FileSpec *
#if NeedFunctionPrototypes
CreateFileSpec(void)
#else
CreateFileSpec()
#endif
{
     return _XkbTypedCalloc(1,FileSpec);
}

void
#if NeedFunctionPrototypes
InitFileSpec(FileSpec *spec)
#else
InitFileSpec(spec)
     FileSpec *	spec;
#endif
{
    bzero((char *)spec,sizeof(FileSpec));
    return;
}

void
#if NeedFunctionPrototypes
FreeFileSpec(FileSpec *spec)
#else
FreeFileSpec(spec)
     FileSpec *	spec;
#endif
{
FileSpec *next;

    while (spec) {
	register int i;
	next= spec->pending;
	for (i=0;i<MAX_WORDS;i++) {
	    if (spec->name[i]!=NULL) {
		_XkbFree(spec->name[i]);
		spec->name[i]= NULL;
	    }
	}
	bzero((char *)spec,sizeof(FileSpec));
	spec= next;
    }
    return;
}

FileSpec *
#if NeedFunctionPrototypes
DupFileSpec(FileSpec *old_spec)
#else
DupFileSpec(old_spec)
    FileSpec *	old_spec;
#endif
{
FileSpec *	new_spec;

    new_spec= _XkbTypedAlloc(FileSpec);
    if (new_spec) {
	register int i;
	for (i=0;i<MAX_WORDS;i++) {
	    new_spec->name[i]= NULL;
	    if (old_spec->name[i]!=NULL)
		 new_spec->name[i]= _XkbDupString(old_spec->name[i]);
	}
	new_spec->pending= NULL;
    }
    return new_spec;
}

/***====================================================================***/

void
#if NeedFunctionPrototypes
SetUpRemap(InputLine *line,RemapSpec *remap)
#else
SetUpRemap(line,remap)
   InputLine *	line;
   RemapSpec *	remap;
#endif
{
char *		tok,*str;
unsigned	present;
register int	i;
Bool		found;

   present= 0;
   str= &line->line[1];
   bzero((char *)remap,sizeof(RemapSpec));
   while ((tok=strtok(str," "))!=NULL) {
	found= False;
	str= NULL;
	if (strcmp(tok,"=")==0)
	    continue;
	for (i=0;i<MAX_WORDS;i++) {
	    if (strcmp(cname[i],tok)==0) {
		found= True;
		if (present&(1<<i)) {
#ifdef DEBUG
		    fprintf(stderr,"Component \"%s\" listed twice\n",tok);
		    fprintf(stderr,"Second definition ignored\n");
#endif
		    break;
		}
		present|= (1<<i);
		remap->remap[remap->num_remap++]= i;
		break;
	    }
	}
#ifdef DEBUG
	if (!found) {
	    fprintf(stderr,"Unknown component \"%s\" ignored\n",tok);
	}
#endif
   }
   if ((present&0x3)==0) {
#ifdef DEBUG
	fprintf(stderr,"Mapping needs at one of \"model\" or \"language\"\n");
	fprintf(stderr,"Illegal mapping ignored\n");
#endif
	remap->num_remap= 0;
	return;
   }
   if ((present&0xFC)==0) {
#ifdef DEBUG
	fprintf(stderr,"Mapping needs at least one component\n");
	fprintf(stderr,"Illegal mapping ignored\n");
#endif
	remap->num_remap= 0;
	return;
   }
   if (((present&0xFC)&(1<<KEYMAP))&&((present&0xFC)!=(1<<KEYMAP))) {
#ifdef DEBUG
	fprintf(stderr,"Keymap cannot appear with other components\n");
	fprintf(stderr,"Illegal mapping ignored\n");
#endif
	remap->num_remap= 0;
	return;
   }
   return;
}

Bool
#if NeedFunctionPrototypes
CheckLine(	char *		model,
		char *		language,
		InputLine *	line,
		FileSpec *	spec,
		RemapSpec *	remap)
#else
CheckLine(model,layout,line,spec,remap)
    char *	model
    char *	language;
    InputLine *	line;
    FileSpec *	spec;
    RemapSpec *	remap;
#endif
{
char *		str,*tok;
register int	i,nread;
FileSpec	tmp;
Bool		complete;

    if (line->line[0]=='!') {
	SetUpRemap(line,remap);
	return False;
    }
    if (remap->num_remap==0) {
#ifdef DEBUG
	fprintf(stderr,"Must have a mapping before first line of data\n");
	fprintf(stderr,"Illegal line of data ignored\n");
#endif
	return False;
    }
    InitFileSpec(&tmp);
    str= line->line;
    for (nread= 0;(tok=strtok(str," "))!=NULL;nread++) {
	str= NULL;
	if (strcmp(tok,"=")==0) {
	    nread--;
	    continue;
	}
	if (nread>remap->num_remap) {
#ifdef DEBUG
	    fprintf(stderr,"Too many words on a line\n");
	    fprintf(stderr,"Extra word \"%s\" ignored\n",tok);
#endif
	    continue;
	}
	tmp.name[remap->remap[nread]]= tok;
    }
    if (nread<remap->num_remap) {
#ifdef DEBUG
	fprintf(stderr,"Too few words on a line\n");
	fprintf(stderr,"line ignored\n");
	return False;
#endif
    }
    if ((tmp.name[MODEL]!=NULL)&&(strcmp(tmp.name[MODEL],"*")==0))
	tmp.name[MODEL]= NULL;
    if ((tmp.name[LANGUAGE]!=NULL)&&(strcmp(tmp.name[LANGUAGE],"*")==0))
	tmp.name[LANGUAGE]= NULL;

    if ((tmp.name[MODEL]!=NULL)&&(strcmp(tmp.name[MODEL],model)!=0))
	return False; /* no match */
    if ((tmp.name[LANGUAGE]!=NULL)&&(strcmp(tmp.name[LANGUAGE],language)!=0))
	return False; /* no match */
    if ((tmp.name[MODEL]==NULL)||(tmp.name[LANGUAGE]==NULL)) { /* partial */
	while (spec->pending)  {
	    spec= spec->pending;
	}
	spec->pending= DupFileSpec(&tmp);
	return False;
    }
    /* exact match */
    if (spec->name[KEYMAP]!=NULL)
	return True;
    complete= True;
    for (i=KEYCODES;i<KEYMAP;i++) {
	if ((spec->name[i]==NULL)&&(tmp.name[i]!=NULL))
	    spec->name[i]= _XkbDupString(tmp.name[i]);
	complete= complete && (spec->name[i]!=NULL);
    }
    return complete;
}

Bool
#if NeedFunctionPrototypes
ApplyPartialFileSpecs(FileSpec *spec)
#else
ApplyPartialFileSpecs(spec)
    FileSpec *	spec;
#endif
{
register int	i;
Bool		complete;
FileSpec *	partial;

    partial= spec->pending;
    complete= False;
    for (;(partial!=NULL)&&(!complete);partial= partial->pending) {
	complete= True;
	if (partial->name[KEYMAP]!=NULL) {
	    spec->name[KEYMAP]= partial->name[KEYMAP];
	    partial->name[KEYMAP]= NULL;
	}
	else {
	    for (i=KEYCODES;i<KEYMAP;i++) {
		if ((spec->name[i]==NULL)&&(partial->name[i]!=NULL)) {
		    spec->name[i]= partial->name[i];
		    partial->name[i]= NULL;
		}
		complete= complete&&(spec->name[i]!=NULL);
	    }
	}
    }
    return complete;
}

/***====================================================================***/

char *
#if NeedFunctionPrototypes
_CompName(char **p_name,char *model,char *language)
#else
_CompName(p_name,model,language)
    char **	p_name;
    char *	model;
    char *	language;
#endif
{
char *	name,*str,*outstr;
int	len;

    name= *p_name;
    str= index(name,'%');
    if (str==NULL) {
	*p_name= NULL;
	return name;
    }
    len= strlen(name);
    while (str!=NULL) {
	if (str[1]=='l')	len+= strlen(language);
	else if (str[1]=='k')	len+= strlen(model);
	str= index(name,'%');
    }
    name= (char *)_XkbAlloc(len)+1;
    str= *p_name;
    outstr= name;
    while (*str!='\0') {
	if ((str[0]=='%')&&(str[1]=='l')) {
	    strcpy(outstr,language);
	    outstr+= strlen(language);
	}
	else if ((str[0]=='%')&&(str[1]=='k')) {
	    strcpy(outstr,model);
	    outstr+= strlen(model);
	}
	else {
	    *outstr++= *str++;
	}
    }
    return name;
}

/***====================================================================***/

Bool
#if NeedFunctionPrototypes
XkbComponentNamesFromMapFile(	FILE *			file,
				char *			model,
				char *			language,
				XkbComponentNamesPtr	names)
#else
XkbComponentNamesFromMapFile(file,model,language,names)
    FILE *			file;
    char *			model;
    char *			language;
    XkbComponentNamesPtr	names;
#endif
{
register int	i;
InputLine	line;
RemapSpec	remap;
FileSpec *	spec;
Bool		complete;

    if ((!file)||(!model)||(!language)||(!names))
	return False;
    bzero((char *)names,sizeof(XkbComponentNamesRec));
    bzero((char *)&remap,sizeof(RemapSpec));
    spec= NULL;
    InitInputLine(&line);
    complete= False;
    spec= CreateFileSpec();
    while (GetInputLine(file,&line)) {
	if (CheckLine(model,language,&line,spec,&remap)) {
	    complete= True; 
	    break;
	}
	line.num_line= 0;
    }
    FreeInputLine(&line);
    if (!complete)
	complete= ApplyPartialFileSpecs(spec);
    if (spec->name[KEYCODES])
	names->keycodes= _CompName(&spec->name[KEYCODES],model,language);
    if (spec->name[SYMBOLS])
	names->symbols= _CompName(&spec->name[SYMBOLS],model,language);
    if (spec->name[TYPES])
	names->types= _CompName(&spec->name[TYPES],model,language);
    if (spec->name[COMPAT])
	names->compat= _CompName(&spec->name[COMPAT],model,language);
    if (spec->name[GEOMETRY])
	names->geometry= _CompName(&spec->name[GEOMETRY],model,language);
    if (spec->name[KEYMAP])
	names->keymap= _CompName(&spec->name[KEYMAP],model,language);
    FreeFileSpec(spec);
    return complete;
}
