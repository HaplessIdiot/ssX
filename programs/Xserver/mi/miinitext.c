/***********************************************************

Copyright (c) 1987  X Consortium

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
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: miinitext.c /main/41 1996/09/28 17:15:08 rws $ */
/* $XFree86: xc/programs/Xserver/mi/miinitext.c,v 3.31 1998/07/26 02:33:07 dawes Exp $ */

#include "misc.h"
#include "extension.h"

#ifdef NOPEXEXT /* sleaze for Solaris cpp building XsunMono */
#undef PEXEXT
#endif

extern Bool noTestExtensions;
#ifdef XKB
extern Bool noXkbExtension;
#endif

#ifndef XFree86LOADER
#if NeedFunctionPrototypes
#define INITARGS void
#else
#define INITARGS /*nothing*/
#endif
typedef void (*InitExtension)(INITARGS);
#else /* XFree86Loader */
#include "xf86Module.h"
#endif

/* FIXME: this whole block of externs should be from the appropriate headers */
#ifdef BEZIER
extern void BezierExtensionInit(INITARGS);
#endif
#ifdef XTESTEXT1
extern void XTestExtension1Init(INITARGS);
#endif
#ifdef SHAPE
extern void ShapeExtensionInit(INITARGS);
#endif
#ifdef MITSHM
extern void ShmExtensionInit(INITARGS);
#endif
#ifdef PEXEXT
extern void PexExtensionInit(INITARGS);
#endif
#ifdef MULTIBUFFER
extern void MultibufferExtensionInit(INITARGS);
#endif
#ifdef XINPUT
extern void XInputExtensionInit(INITARGS);
#endif
#ifdef XTEST
extern void XTestExtensionInit(INITARGS);
#endif
#ifdef BIGREQS
extern void BigReqExtensionInit(INITARGS);
#endif
#ifdef MITMISC
extern void MITMiscExtensionInit(INITARGS);
#endif
#ifdef XIDLE
extern void XIdleExtensionInit(INITARGS);
#endif
#ifdef XTRAP
extern void DEC_XTRAPInit(INITARGS);
#endif
#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (INITARGS);
#endif
#ifdef XV
extern void XvExtensionInit(INITARGS);
#endif
#ifdef XIE
extern void XieInit(INITARGS);
#endif
#ifdef XSYNC
extern void SyncExtensionInit(INITARGS);
#endif
#ifdef XKB
extern void XkbExtensionInit(INITARGS);
#endif
#ifdef XCMISC
extern void XCMiscExtensionInit(INITARGS);
#endif
#ifdef XRECORD
extern void RecordExtensionInit(INITARGS);
#endif
#ifdef LBX
extern void LbxExtensionInit(INITARGS);
#endif
#ifdef DBE
extern void DbeExtensionInit(INITARGS);
#endif
#ifdef XAPPGROUP
extern void XagExtensionInit(INITARGS);
#endif
#ifdef XCSECURITY
extern void SecurityExtensionInit(INITARGS);
#endif
#ifdef XPRINT
extern void XpExtensionInit(INITARGS);
#endif
#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#endif
#ifdef XF86VIDMODE
extern void XFree86VidModeExtensionInit(INITARGS);
#endif
#ifdef XF86MISC
extern void XFree86MiscExtensionInit(INITARGS);
#endif
#ifdef XFreeXDGA
extern void XFree86DGAExtensionInit(INITARGS);
#endif
#ifdef GLXEXT
extern void GlxExtensionInit(INITARGS);
#endif

#ifndef XFree86LOADER

/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
#ifdef BEZIER
    BezierExtensionInit();
#endif
#ifdef XTESTEXT1
    if (!noTestExtensions) XTestExtension1Init();
#endif
#ifdef SHAPE
    ShapeExtensionInit();
#endif
#ifdef MITSHM
    ShmExtensionInit();
#endif
#ifdef PEXEXT
    PexExtensionInit();
#endif
#ifdef MULTIBUFFER
    MultibufferExtensionInit();
#endif
#ifdef XINPUT
    XInputExtensionInit();
#endif
#ifdef XTEST
    if (!noTestExtensions) XTestExtensionInit();
#endif
#ifdef BIGREQS
    BigReqExtensionInit();
#endif
#ifdef MITMISC
    MITMiscExtensionInit();
#endif
#ifdef XIDLE
    XIdleExtensionInit();
#endif
#ifdef XTRAP
    if (!noTestExtensions) DEC_XTRAPInit();
#endif
#if defined(SCREENSAVER) && !defined(PRINT_ONLY_SERVER)
    ScreenSaverExtensionInit ();
#endif
#ifdef XV
    XvExtensionInit();
#endif
#ifdef XIE
    XieInit();
#endif
#ifdef XSYNC
    SyncExtensionInit();
#endif
#ifdef XKB
    if (!noXkbExtension) XkbExtensionInit();
#endif
#ifdef XCMISC
    XCMiscExtensionInit();
#endif
#ifdef XRECORD
    if (!noTestExtensions) RecordExtensionInit(); 
#endif
#ifdef LBX
    LbxExtensionInit();
#endif
#ifdef DBE
    DbeExtensionInit();
#endif
#ifdef XAPPGROUP
    XagExtensionInit();
#endif
#ifdef XCSECURITY
    SecurityExtensionInit();
#endif
#ifdef XPRINT
    XpExtensionInit();
#endif
#if defined(XF86VIDMODE) && !defined(PRINT_ONLY_SERVER)
    XFree86VidModeExtensionInit();
#endif
#if defined(XF86MISC) && !defined(PRINT_ONLY_SERVER)
    XFree86MiscExtensionInit();
#endif
#if defined(XFreeXDGA) && !defined(PRINT_ONLY_SERVER)
    XFree86DGAExtensionInit();
#endif
#if defined(DPMSExtension)
    DPMSExtensionInit();
#endif
#ifdef GLXEXT
#ifndef XPRINT	/* we don't want Glx in the Xprint server */
    GlxExtensionInit();
#endif
#endif
}

#else /* XFree86LOADER */
/* FIXME:The names here must come from the headers. those with ?? are 
   not included in X11R6.3 sample implementation, so there's a problem... */
/* XXX use the correct #ifdefs for symbols not present when an extension
   is disabled */
ExtensionModule extension[] =
{
    { NULL, "BEZIER", NULL },	/* ?? */
    { NULL, "XTEST1", &noTestExtensions }, /* ?? */
    { NULL, "SHAPE", NULL },
    { NULL, "MIT-SHM", NULL },
    { NULL, "X3D-PEX", NULL },
    { NULL, "Multi-Buffering", NULL },
    { NULL, "XInputExtension", NULL },
    { NULL, "XTEST", &noTestExtensions },
    { NULL, "BIG-REQUESTS", NULL },
    { NULL, "MIT-SUNDRY-NONSTANDARD", NULL },
    { NULL, "XIDLE", NULL },	/* ?? */
    { NULL, "XTRAP", &noTestExtensions }, /* ?? */
    { NULL, "MIT-SCREEN-SAVER", NULL },
    { NULL, "XVideo", NULL },	/* ?? */
    { NULL, "XIE", NULL },
    { NULL, "SYNC", NULL },
#ifdef XKB
    { NULL, "XKEYBOARD", &noXkbExtension },
#else
    { NULL, "NOXKEYBOARD", NULL },
#endif
    { NULL, "XC-MISC", NULL },
    { NULL, "RECORD", &noTestExtensions },
    { NULL, "LBX", NULL },
    { NULL, "DOUBLE-BUFFER", NULL },
    { NULL, "XC-APPGROUP", NULL },
    { NULL, "SECURITY", NULL },
    { NULL, "XpExtension", NULL },
    { NULL, "XFree86-VidModeExtension", NULL },
    { NULL, "XFree86-Misc", NULL },
    { NULL, "XFree86-DGA", NULL },
    { NULL, "DPMS", NULL },
    { NULL, "GLX", NULL },
    { NULL, NULL, NULL }
};

/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
    int i;
#if 1 
   /* Add static extensions */
#ifdef BEZIER
    extension[0].initFunc = BezierExtensionInit;
#endif 
#ifdef XTESTEXT1
    extension[1].initFunc = XTestExtension1Init;
#endif
    /* 2 - SHAPE */
#ifdef MITSHM
    extension[3].initFunc = ShmExtensionInit;
#endif
    /* 4 - pex */
    /* 5 - multibuf */
#ifdef XINPUT
    extension[6].initFunc = XInputExtensionInit;
#endif
#ifdef XTEST
    extension[7].initFunc = XTestExtensionInit;
#endif
    /* 8 - BigReqs */
    /* 9 - MITMisc */
#ifdef XIDLE
    extension[10].initFunc = XIdleExtensionInit;
#endif
#ifdef XTRAP
    extension[11].initFunc = DEC_XTRAPIbit;
#endif
    /* 12 - ScreenSaver */
    /* 13 - XVideo */
    /* 14 - XIE */
    /* 15 - XSYNC */
#ifdef XKB
    extension[16].initFunc = XkbExtensionInit;
#endif
    /* 17 - XCMISC */
    /* 18 - XRECORD */
#ifdef LBX
    extension[19].initFunc = LbxExtensionInit;
#endif
    /* 20 - DBE */
#ifdef XAPPGROUP
    extension[21].initFunc = XagExtensionInit;
#endif
#ifdef XCSECURITY
    extension[22].initFunc = SecurityExtensionInit;
#endif
#ifdef XPRINT
    extension[23].initFunc = XpExtensionInit;
#endif
    /* 24 - XF86VidMode */
    /* 25 - XF86Misc */
    /* 26 - XF86DGA */
    /* 27 - DPMS */
    /* 28 - GLX */
#endif
    for (i = 0; extension[i].name != NULL; i++) 
	if (extension[i].initFunc != NULL && 
	    (extension[i].disablePtr == NULL || 
	     (extension[i].disablePtr != NULL && !*extension[i].disablePtr))) {
	    (*extension[i].initFunc)();
	}
}

#endif /* XFree86LOADER */
