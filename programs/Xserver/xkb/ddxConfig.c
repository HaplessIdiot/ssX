/* $XConsortium: ddxConfig.c /main/5 1996/01/14 16:45:41 kaleb $ */
/************************************************************
Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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
#define	NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "os.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"
#include <X11/extensions/XKBconfig.h>

Bool
#if NeedFunctionPrototypes
XkbDDXApplyConfig(XPointer cfg_in,XkbSrvInfoPtr info)
#else
XkbDDXApplyConfig(cfg_in,info)
    XPointer		cfg_in;	
    XkbSrvInfoPtr	info;
#endif
{
XkbConfigRtrnPtr 	rtrn;
XkbDescPtr		xkb;
Bool			ok;
XkbEventCauseRec	cause;

    xkb= info->desc;
    rtrn= (XkbConfigRtrnPtr)cfg_in;
    if (rtrn==NULL)
	return True;
    ok= XkbCFApplyRtrnValues(rtrn,XkbCFDflts,xkb);
    if (rtrn->initial_mods.replace) {
	info->state.locked_mods= rtrn->initial_mods.mods;
    }
    else {
	info->state.locked_mods|= rtrn->initial_mods.mods;
	if (rtrn->initial_mods.mods_clear)
	    info->state.locked_mods&= ~rtrn->initial_mods.mods_clear;
    }
    XkbComputeDerivedState(info);
    cause.kc=		0;
    cause.event=	0;
    cause.mjr=		0;
    cause.mnr=		0;
    XkbUpdateIndicators(info->device,XkbAllIndicatorsMask,False,NULL,&cause);
    if (info->device && info->device->kbdfeed) {
	DeviceIntPtr	dev;
	KeybdCtrl	newCtrl;
	dev= info->device;
	newCtrl= dev->kbdfeed->ctrl;
	if (rtrn->click_volume>=0)
	    newCtrl.click= rtrn->click_volume;
	if (rtrn->bell_volume>=0)
	    newCtrl.bell= rtrn->bell_volume;
	if (rtrn->bell_pitch>0)
	    newCtrl.bell_pitch= rtrn->bell_pitch;
	if (rtrn->bell_duration>0)
	    newCtrl.bell_duration= rtrn->bell_duration;
	if (dev->kbdfeed->CtrlProc)
	    (*dev->kbdfeed->CtrlProc)(dev,&newCtrl);
    }
    XkbCFFreeRtrn(rtrn,XkbCFDflts,xkb);
    return ok;
}

XPointer
#if NeedFunctionPrototypes
XkbDDXPreloadConfig(XkbComponentNamesPtr names,DeviceIntPtr dev)
#else
XkbDDXPreloadConfig(names,dev)
    XkbComponentNamesPtr	names;
    DeviceIntPtr		dev;
#endif
{
char			buf[PATH_MAX];
char *			dName;
FILE *			file;
XkbConfigRtrnPtr	rtrn;
extern char *		display;

#if defined(MetroLink)
    if (dev && dev->name)
	dName= dev->name;
    else
	/* NULL, not ""  see test below */
	dName= NULL;
    /* It doesn't appear that XkbBaseDirectory could ever get set to NULL */
    sprintf(buf,"%s/X%s-config%s%s",XkbBaseDirectory,display,
 			(dName?".":""),(dName?dName:""));
#else
    if (dev && dev->name)
	 dName= dev->name;
    else dName= "";
    if (XkbBaseDirectory!=NULL)
	 sprintf(buf,"%s/X%s-config%s%s",XkbBaseDirectory,display,
						(dName?".":""),dName);
    else sprintf(buf,"%s/X%s-config%s%s",display,(dName?".":""),dName);
#endif
#ifdef __EMX__
    strcpy(buf,(char*)__XOS2RedirRoot(buf));
#endif
#ifdef DEBUG
    ErrorF("Looking for keyboard configuration in %s...",buf);
#endif
    file= fopen(buf,"r");
    if (file==NULL) {
#ifdef DEBUG
	ErrorF("file not found\n");
#endif
	return NULL;
    }
    rtrn= _XkbTypedCalloc(1,XkbConfigRtrnRec);
    if (rtrn!=NULL) {
	if (!XkbCFParse(file,XkbCFDflts,NULL,rtrn)) {
#ifdef DEBUG
	    ErrorF("error\n");
#endif
	    ErrorF("Error parsing config file: ");
	    XkbCFReportError(stderr,buf,rtrn->error,rtrn->line);
	    _XkbFree(rtrn);
	    fclose(file);
	    return NULL;
	}
#ifdef DEBUG
	ErrorF("found it\n");
#endif
	if (rtrn->keycodes!=NULL) {
	    names->keycodes= rtrn->keycodes;
	    rtrn->keycodes= NULL;
	}
	if (rtrn->geometry!=NULL) {
	    names->geometry= rtrn->geometry;
	    rtrn->geometry= NULL;
	}
	if (rtrn->symbols!=NULL) {
	    if (rtrn->phys_symbols==NULL)
		rtrn->phys_symbols= _XkbDupString(names->symbols);
	    names->symbols= rtrn->symbols;
	    rtrn->symbols= NULL;
	}
	if (rtrn->types!=NULL) {
	    names->types= rtrn->types;
	    rtrn->types= NULL;
	}
	if (rtrn->compat!=NULL) {
	    names->compat= rtrn->compat;
	    rtrn->compat= NULL;
	}
    }
    fclose(file);
    return (XPointer)rtrn;
}
