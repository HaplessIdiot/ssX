/* $XConsortium: ddxLoad.c /main/8 1996/02/05 06:18:40 kaleb $ */
/* $XFree86: xc/programs/Xserver/xkb/ddxLoad.c,v 3.5 1996/02/09 10:17:55 dawes Exp $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

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
#define	NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/extensions/XKM.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"
#include "XI.h"

#if defined(CSRG_BASED) || defined(linux) || defined(__sgi) || defined(AIXV3) || defined(__osf__)
#include <paths.h>
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define	PATH_MAX MAXPATHLEN
#else
#define	PATH_MAX 1024
#endif
#endif

	/*
	 * If XKM_OUTPUT_DIR specifies a path without a leading slash, it is
	 * relative to the top-level XKB configuration directory.
	 * Making the server write to a subdirectory of that directory
	 * requires some work in the general case (install procedure
	 * has to create links to /var or somesuch on many machines),
	 * so we just compile into /usr/tmp for now.
	 */
#ifndef XKM_OUTPUT_DIR
#ifdef NOTYET
#define	XKM_OUTPUT_DIR	"compiled/"
#else
#ifdef _PATH_VARTMP
#define XKM_OUTPUT_DIR	_PATH_VARTMP
#else
#ifndef __EMX__
#define	XKM_OUTPUT_DIR	"/usr/tmp/"
#else
#define	XKM_OUTPUT_DIR	"./"
#endif
#endif
#endif
#endif

#define	PRE_ERROR_MSG "\"The XKB keymap compiler (xkbcomp) reports:\""
#define	ERROR_PREFIX	"\"> \""
#define	POST_ERROR_MSG1 "\"Errors from xkbcomp are not fatal to the X server\""
#define	POST_ERROR_MSG2 "\"End of messages from xkbcomp\""

Bool
#if NeedFunctionPrototypes
XkbDDXCompileNamedKeymap(	XkbDescPtr		xkb,
				XkbComponentNamesPtr	names,
				char *			nameRtrn,
				int			nameRtrnLen)
#else
XkbDDXCompileNamedKeymap(xkb,names,nameRtrn,nameRtrnLen)
    XkbDescPtr			xkb;
    XkbComponentNamesPtr	names;
    char *			nameRtrn;
    int				nameRtrnLen;
#endif
{
char 	cmd[PATH_MAX],file[PATH_MAX],*map,*outFile;

    if (names->keymap==NULL)
	return False;
    strncpy(file,names->keymap,PATH_MAX); file[PATH_MAX-1]= '\0';
    if ((map= strrchr(file,'('))!=NULL) {
	char *tmp;
	if ((tmp= strrchr(map,')'))!=NULL) {
	    *map++= '\0';
	    *tmp= '\0';
	}
	else {
	    map= NULL;
	}
    }
    if ((outFile= strrchr(file,'/'))!=NULL)
	 outFile= _XkbDupString(&outFile[1]);
    else outFile= _XkbDupString(file);
    XkbEnsureSafeMapName(outFile);
    if (XkbBaseDirectory!=NULL) {
	sprintf(cmd,"%s/xkbcomp -w %d -R%s -xkm %s%s -em1 %s -emp %s -eml %s keymap/%s %s%s.xkm",
		XkbBaseDirectory,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		XkbBaseDirectory,(map?"-m ":""),(map?map:""),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,file,
		XKM_OUTPUT_DIR,outFile);
    }
    else {
	sprintf(cmd,"xkbcomp -w %d -xkm %s%s -em1 %s -emp %s -eml %s keymap/%s %s%s.xkm",
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		(map?"-m ":""),(map?map:""),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,file,
		XKM_OUTPUT_DIR,outFile);
    }
#ifdef DEBUG
    if (xkbDebugFlags) {
	ErrorF("XkbDDXCompileNamedKeymap compiling keymap using:\n");
	ErrorF("    \"cmd\"\n");
    }
#endif
    if (system(cmd)==0) {
	if (nameRtrn) {
	    strncpy(nameRtrn,outFile,nameRtrnLen);
	    nameRtrn[nameRtrnLen-1]= '\0';
	}
	if (outFile!=NULL)
	    Xfree(outFile);
	return True;
    } 
#ifdef DEBUG
    ErrorF("Error compiling keymap (%s)\n",names->keymap);
#endif
    if (outFile!=NULL)
	Xfree(outFile);
    return False;
}
        	
Bool    	
#if NeedFunctionPrototypes
XkbDDXCompileKeymapByNames(	XkbDescPtr		xkb,
				XkbComponentNamesPtr	names,
				unsigned		want,
				unsigned		need,
				char *			nameRtrn,
				int			nameRtrnLen)
#else
XkbDDXCompileKeymapByNames(xkb,names,want,need,nameRtrn,nameRtrnLen)
    XkbDescPtr			xkb;
    XkbComponentNamesPtr	names;
    unsigned			want;
    unsigned			need;
    char *			nameRtrn;
    int				nameRtrnLen;
#endif
{
FILE *	out;
char	buf[PATH_MAX],keymap[PATH_MAX];;
    
    if ((names->keymap==NULL)||(names->keymap[0]=='\0')) {
	extern char *display;
	sprintf(keymap,"server-%s",display);
    }
    else {
	strcpy(keymap,names->keymap);
    }

    XkbEnsureSafeMapName(keymap);
    if (XkbBaseDirectory!=NULL) {
#ifdef __EMX__
        char *tmpbase = (char*)__XOS2RedirRoot(XkbBaseDirectory);
        int i;
        for (i=0; i<strlen(tmpbase); i++) if (tmpbase[i]=='/') tmpbase[i]='\\';
	sprintf(buf,"%s\\xkbcomp -w %d -R%s -xkm - -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		tmpbase,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		tmpbase,
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,XKM_OUTPUT_DIR,keymap);
#else
	sprintf(buf,"%s/xkbcomp -w %d -R%s -xkm - -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		XkbBaseDirectory,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		XkbBaseDirectory,
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,XKM_OUTPUT_DIR,keymap);
#endif
    }
    else {
	sprintf(buf,"xkbcomp -w %d -xkm - -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:xkbDebugFlags)),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,XKM_OUTPUT_DIR,keymap);
    }

    out= popen(buf,"w");
    if (out!=NULL) {
#ifdef DEBUG
	if (xkbDebugFlags) {
	    ErrorF("XkbDDXCompileKeymapByNames compiling keymap:\n");
	    XkbWriteXKBKeymapForNames(stderr,names,NULL,xkb,want,need);
	}
#endif
	XkbWriteXKBKeymapForNames(out,names,NULL,xkb,want,need);
	if (pclose(out)==0) {
	    if (nameRtrn) {
		strncpy(nameRtrn,keymap,nameRtrnLen);
		nameRtrn[nameRtrnLen-1]= '\0';
	    }
	    return True;
	}
#ifdef DEBUG
	ErrorF("Error compiling keymap (%s)\n",keymap);
#endif
    }
#ifdef DEBUG
    else {
	ErrorF("Could not invoke keymap compiler\n");
    }
#endif
    if (nameRtrn)
	nameRtrn[0]= '\0';
    return False;
}

FILE *
#if NeedFunctionPrototypes
XkbDDXOpenConfigFile(char *mapName,char *fileNameRtrn,int fileNameRtrnLen)
#else
XkbDDXOpenConfigFile(mapName,fileNameRtrn,fileNameRtrnLen)
    char *	mapName;
    char *	fileNameRtrn;
    int		fileNameRtrnLen;
#endif
{
char	buf[PATH_MAX];
FILE *	file;

    buf[0]= '\0';
    if (mapName!=NULL) {
	if ((XkbBaseDirectory!=NULL)&&(XKM_OUTPUT_DIR[0]!='/'))
	     sprintf(buf,"%s/%s%s.xkm",XkbBaseDirectory,XKM_OUTPUT_DIR,mapName);
	else sprintf(buf,"%s%s.xkm",XKM_OUTPUT_DIR,mapName);
	file= fopen(buf,"r");
    }
    else file= NULL;
    if ((fileNameRtrn!=NULL)&&(fileNameRtrnLen>0)) {
	strncpy(fileNameRtrn,buf,fileNameRtrnLen);
	buf[fileNameRtrnLen-1]= '\0';
    }
    return file;
}

unsigned
#if NeedFunctionPrototypes
XkbDDXLoadKeymapByNames(	DeviceIntPtr		keybd,
				XkbComponentNamesPtr	names,
				unsigned		want,
				unsigned		need,
				XkbFileInfo *		finfoRtrn,
				char *			nameRtrn,
				int 			nameRtrnLen)
#else
XkbDDXLoadKeymapByNames(keybd,names,want,need,finfoRtrn,nameRtrn,nameRtrnLen)
    DeviceIntPtr		keybd;
    XkbComponentNamesPtr	names;
    unsigned			want;
    unsigned			need;
    XkbFileInfo *		finfoRtrn;
    char *			nameRtrn;
    int 			nameRtrnLen;
#endif
{
XkbDescPtr	xkb;
FILE	*	file;
char		fileName[PATH_MAX];
unsigned	missing;

    bzero(finfoRtrn,sizeof(XkbFileInfo));
    if ((keybd==NULL)||(keybd->key==NULL)||(keybd->key->xkbInfo==NULL))
	 xkb= NULL;
    else xkb= keybd->key->xkbInfo->desc;
    if ((names->keycodes==NULL)&&(names->types==NULL)&&
	(names->compat==NULL)&&(names->symbols==NULL)&&
	(names->geometry==NULL)) {
	if (names->keymap==NULL) {
	    bzero(finfoRtrn,sizeof(XkbFileInfo));
	    if (xkb && XkbDetermineFileType(finfoRtrn,XkbXKMFile,NULL) &&
	   				((finfoRtrn->defined&need)==need) ) {
		finfoRtrn->xkb= xkb;
		nameRtrn[0]= '\0';
		return finfoRtrn->defined;
	    }
	    return 0;
	}
	else if (!XkbDDXCompileNamedKeymap(xkb,names,nameRtrn,nameRtrnLen)) {
#ifdef NOISY
	    ErrorF("Couldn't compile keymap file\n");
#endif
	    return 0;
	}
    }
    else if (!XkbDDXCompileKeymapByNames(xkb,names,want,need,
						nameRtrn,nameRtrnLen)){
#ifdef NOISY
	ErrorF("Couldn't compile keymap file\n");
#endif
	return 0;
    }
    file= XkbDDXOpenConfigFile(nameRtrn,fileName,PATH_MAX);
    if (file==NULL) {
	ErrorF("Couldn't open compiled keymap file %s\n",fileName);
	return 0;
    }
    missing= XkmReadFile(file,need,want,finfoRtrn);
    if (finfoRtrn->xkb==NULL) {
	ErrorF("Error loading keymap %s\n",fileName);
	fclose(file);
	return 0;
    }
#ifdef DEBUG
    else if (xkbDebugFlags) {
	ErrorF("Loaded %s, defined=0x%x\n",fileName,finfoRtrn->defined);
    }
#endif
    fclose(file);
    return (need|want)&(~missing);
}
