/* $XFree86: xc/programs/Xserver/mi/miinitext.c,v 3.33 1998/08/13 14:46:14 dawes Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


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
/* $TOG: miinitext.c /main/47 1998/02/09 14:47:26 kaleb $ */

#include "misc.h"
#include "extension.h"

#ifdef NOPEXEXT /* sleaze for Solaris cpp building XsunMono */
#undef PEXEXT
#endif
#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
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
#ifdef EVI
extern void EVIExtensionInit(INITARGS);
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
#ifdef PANORAMIX
extern void PanoramiXExtensionInit(INITARGS);
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
#ifdef TOGCUP
extern void XcupExtensionInit(INITARGS);
#endif
#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#endif

#ifndef XFree86LOADER

/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
#ifdef PANORAMIX
#if !defined(PRINT_ONLY_SERVER) && !defined(NO_PANORAMIX)
  if (!noPanoramiXExtension) PanoramiXExtensionInit();
#endif
#endif
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
#ifdef EVI
    EVIExtensionInit();
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
#if defined(XKB) && !defined(PRINT_ONLY_SERVER)
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
#ifdef TOGCUP
    XcupExtensionInit();
#endif
#if defined(DPMSExtension) && !defined(PRINT_ONLY_SERVER)
    DPMSExtensionInit();
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
    { NULL, "BEZIER", NULL, NULL },	/* ?? */
    { NULL, "XTEST1", &noTestExtensions, NULL }, /* ?? */
    { NULL, "SHAPE", NULL, NULL },
    { NULL, "MIT-SHM", NULL, NULL },
    { NULL, "X3D-PEX", NULL, NULL },
    { NULL, "Multi-Buffering", NULL, NULL },
    { NULL, "XInputExtension", NULL, NULL },
    { NULL, "XTEST", &noTestExtensions, NULL },
    { NULL, "BIG-REQUESTS", NULL, NULL },
    { NULL, "MIT-SUNDRY-NONSTANDARD", NULL, NULL },
    { NULL, "XIDLE", NULL, NULL },	/* ?? */
    { NULL, "XTRAP", &noTestExtensions, NULL }, /* ?? */
    { NULL, "MIT-SCREEN-SAVER", NULL, NULL },
    { NULL, "XVideo", NULL, NULL },	/* ?? */
    { NULL, "XIE", NULL, NULL },
    { NULL, "SYNC", NULL, NULL },
#ifdef XKB
    { NULL, "XKEYBOARD", &noXkbExtension, NULL },
#else
    { NULL, "NOXKEYBOARD", NULL, NULL },
#endif
    { NULL, "XC-MISC", NULL, NULL },
    { NULL, "RECORD", &noTestExtensions, NULL },
    { NULL, "LBX", NULL, NULL },
    { NULL, "DOUBLE-BUFFER", NULL, NULL },
    { NULL, "XC-APPGROUP", NULL, NULL },
    { NULL, "SECURITY", NULL, NULL },
    { NULL, "XpExtension", NULL, NULL },
    { NULL, "XFree86-VidModeExtension", NULL, NULL },
    { NULL, "XFree86-Misc", NULL, NULL },
    { NULL, "XFree86-DGA", NULL, NULL },
    { NULL, "DPMS", NULL, NULL },
    { NULL, "GLX", NULL, NULL },
    { NULL, NULL, NULL, NULL }
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
