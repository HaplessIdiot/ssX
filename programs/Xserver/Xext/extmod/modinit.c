/* $XFree86: xc/programs/Xserver/Xext/modinit.c,v 3.2.2.5 1998/07/04 13:32:20 dawes Exp $ */

/*
 *
 * Copyright (c) 1997 Matthieu Herrb
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Matthieu Herrb not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Matthieu Herrb makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * MATTHIEU HERRB DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL MATTHIEU HERRB BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef XFree86LOADER
#include "X.h"
#include "misc.h"
#include "xf86_ansic.h"

#include "xf86Module.h"
#include "xf86Opt.h"

MODULEINITPROTO(extmodModuleInit);
static MODULESETUPPROTO(extmodSetup);

extern Bool noTestExtensions;

#ifdef SHAPE
extern void ShapeExtensionInit(INITARGS);
#define _SHAPE_SERVER_  /* don't want Xlib structures */
#include "shapestr.h"
#endif

#ifdef MULTIBUFFER
extern void MultibufferExtensionInit(INITARGS);
#define _MULTIBUF_SERVER_	/* don't want Xlib structures */
#include "regionstr.h"
#include "gcstruct.h"
#include "inputstr.h"
#include "multibufst.h"
#endif

#ifdef MITMISC
extern void MITMiscExtensionInit(INITARGS);
#define _MITMISC_SERVER_
#include "mitmiscstr.h"
#endif

#ifdef XTEST
extern void XTestExtensionInit(INITARGS);
#define _XTEST_SERVER_
#include "XTest.h"
#include "xteststr.h"
#endif

#ifdef BIGREQS
extern void BigReqExtensionInit(INITARGS);
#include "bigreqstr.h"
#endif

#ifdef XSYNC
extern void SyncExtensionInit(INITARGS);
#define _SYNC_SERVER
#include "sync.h"
#include "syncstr.h"
#endif

#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (INITARGS);
#include "saver.h"
#endif

#ifdef XCMISC
extern void XCMiscExtensionInit(INITARGS);
#include "xcmiscstr.h"
#endif

#ifdef XF86VIDMODE
extern void	XFree86VidModeExtensionInit(INITARGS);
#define _XF86VIDMODE_SERVER_
#include "xf86vmstr.h"
#endif

#ifdef XF86MISC
extern void XFree86MiscExtensionInit(INITARGS);
#define _XF86MISC_SERVER_
#define _XF86MISC_SAVER_COMPAT_
#include "xf86mscstr.h"
#endif

#ifdef XFreeXDGA
extern void XFree86DGAExtensionInit(INITARGS);
#define _XF86DGA_SERVER_
#include "xf86dgastr.h"
#endif

#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#include "dpmsstr.h"
#endif

#ifdef XV
extern void XvExtensionInit(INITARGS);
#include "Xv.h"
#endif

/*
 * Array describing extensions to be initialized
 */
ExtensionModule extensionModules[] = {
#ifdef SHAPE
    {
	ShapeExtensionInit,
	SHAPENAME,
	NULL
    },
#endif
#ifdef MULTIBUFFER
    {
	MultibufferExtensionInit,
	MULTIBUFFER_PROTOCOL_NAME,
	NULL
    },
#endif
#ifdef MITMISC
    {
	MITMiscExtensionInit,
	MITMISCNAME,
	NULL
    },
#endif
#ifdef notyet
    {
	XTestExtensionInit,
	XTestExtensionName,
	&noTestExtensions
    },
#endif
#ifdef BIGREQS
     {
	 BigReqExtensionInit,
	 XBigReqExtensionName,
	 NULL
     },
#endif
#ifdef XSYNC
    {
	SyncExtensionInit,
	SYNC_NAME,
	NULL
    },
#endif
#ifdef SCREENSAVER
    {
	ScreenSaverExtensionInit,
	ScreenSaverName,
	NULL
    },
#endif
#ifdef XCMISC
    {
	XCMiscExtensionInit,
	XCMiscExtensionName,
	NULL
    },
#endif
#ifdef XF86VIDMODE
    {
	XFree86VidModeExtensionInit,
	XF86VIDMODENAME,
	NULL
    },
#endif
#ifdef XF86MISC
    {
	XFree86MiscExtensionInit,
	XF86MISCNAME,
	NULL
    },
#endif
#ifdef XFreeXDGA
    {
	XFree86DGAExtensionInit,
	XF86DGANAME,
	NULL
    },
#endif
#ifdef DPMSExtension
    {
	DPMSExtensionInit,
	DPMSExtensionName,
	NULL
    },
#endif
#ifdef XV
    {
	XvExtensionInit,
	XvName,
	NULL
    },
#endif
    {				/* DON'T delete this entry ! */
	NULL,
	NULL,
	NULL
    }
};

static XF86ModuleVersionInfo VersRec =
{
        "extmod",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0x00010001,				/* 1.1 */
        ABI_CLASS_EXTENSION,
        ABI_EXTENSION_VERSION,
        {0,0,0,0}
};

/* 
 * Entry point for the loader code
 */
void
extmodModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		 ModuleTearDownProc *teardown)
{
    *vers = &VersRec;
    *setup = extmodSetup;
    *teardown = NULL;
}


static pointer
extmodSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    int i;

    /* XXX the option stuff here is largely a sample/test case */

    for (i = 0; extensionModules[i].name != NULL; i++) {
	if (opts) {
	    char *s;
	    s = (char *)xalloc(strlen(extensionModules[i].name) + 5);
	    if (s) {
		pointer o;
		strcpy(s, "omit");
		strcat(s, extensionModules[i].name);
		o = xf86FindOption(opts, s);
		xfree(s);
		if (o) {
		    xf86MarkOptionUsed(o);
		    continue;
		}
	    }
	}
	LoadExtension(&extensionModules[i]);
    }
    /* Need a non-NULL return */
    return (pointer)1;
}

#endif /* XFree86LOADER */
