/* $XFree86$ */
/**************************************************************************

Copyright 2000 ATI Technologies Inc. and VA Linux Systems, Inc.,
                                         Sunnyvale, California.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *
 */


/*
 * To add a new driver (e.g., zzzzz) to the r128 directory:
 *
 * 1. Add a new XF86ModuleData line for the new driver:
 *       XF86ModuleData zzzzzModuleData = { &ATI2VersRec, ATI2Setup, 0 };
 *
 * 2. Add a new driver AvailableOptions function call to
 *    ATI2AvailableOptions:
 *       if (!opts) opts = ZZZZZAvailableOptions(chipid, busid);
 *
 * 3. Add a new driver Identify function call to ATI2Identify:
 *       ZZZZZIdentify(flags);
 *
 * 4. Add a new driver Probe function call ATI2Probe:
 *       foundScreen |= ZZZZZProbe(drv, flags);
 *
 * 5. Add any symbols that are needed in your driver as an argument to
 *    the call to LoaderRefSymbLists() in ATI2Setup.
 *
 * Note: see r128_driver.c for examples of how to write the following
 *       functions:
 *          OptionInfoPtr ZZZZZAvailableOptions(int chipid, int busid);
 *          void          ZZZZZIdentify(int flags);
 *          Bool ZZZZZProbe(DriverPtr drv, int flags); */


				/* X and server generic header files */
#include "xf86.h"

				/* Driver data structures */
#include "ati2.h"
#include "r128_probe.h"
#include "radeon_probe.h"

				/* Forward definitions for driver functions */
static OptionInfoPtr ATI2AvailableOptions(int chipid, int busid);
static Bool          ATI2Probe(DriverPtr drv, int flags);
static void          ATI2Identify(int flags);

				/* Define driver */
/* NOTE: This structure must be named R128 since the directory's name is
   r128 so that the static X server can find the driver. */
DriverRec R128 = {
    ATI2_VERSION,
    "ATI Rage 128 and Radeon",
    ATI2Identify,
    ATI2Probe,
    ATI2AvailableOptions,
    NULL
};

const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWFreeHWRec",
    "vgaHWLock",
    "vgaHWUnlock",
    "vgaHWSave",
    "vgaHWRestore",
    NULL
};

const char *fbdevHWSymbols[] = {
    "fbdevHWInit",
    "fbdevHWUseBuildinMode",

    "fbdevHWGetDepth",
    "fbdevHWGetVidmem",

    /* colormap */
    "fbdevHWLoadPalette",

    /* ScrnInfo hooks */
    "fbdevHWSwitchMode",
    "fbdevHWAdjustFrame",
    "fbdevHWEnterVT",
    "fbdevHWLeaveVT",
    "fbdevHWValidMode",
    "fbdevHWRestore",
    "fbdevHWModeInit",
    "fbdevHWSave",

    "fbdevHWUnmapMMIO",
    "fbdevHWUnmapVidmem",
    "fbdevHWMapMMIO",
    "fbdevHWMapVidmem",

    NULL
};

const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    "xf86DoEDID_DDC2",
    NULL
};

#ifdef XFree86LOADER
#ifdef USE_FB
static const char *fbSymbols[] = {
    "fbScreenInit",
    NULL
};
#else
static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    "cfb24_32ScreenInit",
    NULL
};
#endif

static const char *xaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAAInit",
    "XAAStippleScanlineFuncLSBFirst",
    "XAAOverlayFBfuncs",
    "XAACachePlanarMonoStipple",
    "XAAScreenIndex",
    NULL
};

static const char *xf8_32bppSymbols[] = {
    "xf86Overlay8Plus32Init",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

#ifdef XF86DRI
static const char *drmSymbols[] = {
    "drmAddBufs",
    "drmAddMap",
    "drmAvailable",
    "drmCtlAddCommand",
    "drmCtlInstHandler",
    "drmGetInterruptFromBusID",
    "drmMapBufs",
    "drmMarkBufs",
    "drmUnmapBufs",
    "drmFreeVersion",
    "drmGetVersion",
    NULL
};

static const char *driSymbols[] = {
    "DRIGetDrawableIndex",
    "DRIFinishScreenInit",
    "DRIDestroyInfoRec",
    "DRICloseScreen",
    "DRIDestroyInfoRec",
    "DRIScreenInit",
    "DRIDestroyInfoRec",
    "DRICreateInfoRec",
    "DRILock",
    "DRIUnlock",
    "DRIGetSAREAPrivate",
    "DRIGetContext",
    "DRIQueryVersion",
    "GlxSetVisualConfigs",
    NULL
};

static const char *vbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
    NULL
};
#endif

static MODULESETUPPROTO(ATI2Setup);

static XF86ModuleVersionInfo ATI2VersRec =
{
    ATI2_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    ATI2_VERSION_MAJOR, ATI2_VERSION_MINOR, ATI2_VERSION_PATCH,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    { 0, 0, 0, 0 }
};

XF86ModuleData ati2ModuleData   = { &ATI2VersRec, ATI2Setup, 0 };
XF86ModuleData r128ModuleData   = { &ATI2VersRec, ATI2Setup, 0 };
XF86ModuleData radeonModuleData = { &ATI2VersRec, ATI2Setup, 0 };

static pointer ATI2Setup(pointer module, pointer opts, int *errmaj,
			 int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
        setupDone = TRUE;
        xf86AddDriver(&R128, module, 0);

	/*
	 * Modules that this driver always requires may be loaded here
	 * by calling LoadSubModule().
	 */
				/* FIXME: add DRI support here */

	/*
	 * Tell the loader about symbols from other modules that this
	 * module might refer to.
	 */
	LoaderRefSymLists(vgahwSymbols,
#ifdef USE_FB
			  fbSymbols,
#else
			  cfbSymbols,
#endif
			  xaaSymbols,
			  xf8_32bppSymbols,
			  ramdacSymbols,
#ifdef XF86DRI
			  drmSymbols,
			  driSymbols,
#endif
			  fbdevHWSymbols,
			  vbeSymbols,
			  /* ddcsymbols, */
			  /* i2csymbols, */
			  /* shadowSymbols, */
			  NULL);

        /*
         * The return value must be non-NULL on success even though there
         * is no TearDownProc.
         */
        return (pointer)1;
    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        return NULL;
    }
}
#endif

/* Return the options for supported chipset 'n'; NULL otherwise. */
static OptionInfoPtr ATI2AvailableOptions(int chipid, int busid)
{
    OptionInfoPtr opts = NULL;

    opts = R128AvailableOptions(chipid, busid);
    if (!opts) opts = RADEONAvailableOptions(chipid, busid);

    return opts;
}

/* Return the string name for supported chipset 'n'; NULL otherwise. */
static void ATI2Identify(int flags)
{
    R128Identify(flags);
    RADEONIdentify(flags);
}

/* Return TRUE if chipset is present; FALSE otherwise. */
static Bool ATI2Probe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;

    foundScreen |= R128Probe(drv, flags);
    foundScreen |= RADEONProbe(drv, flags);

    return foundScreen;
}
