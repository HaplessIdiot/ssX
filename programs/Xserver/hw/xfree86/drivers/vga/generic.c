/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/vga/generic.c,v 1.7 1998/09/13 05:23:45 dawes Exp $ */
/*
 * Copyright (C) 1998 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/*
 * This is essentially a merge of two different drivers:  a VGA planar driver
 * (originally by David Dawes <dawes@xfree86.org>) and a 256-colour VGA driver
 * by Harm Hanemaayer <hhanemaa@cs.ruu.nl>.
 *
 * The port of this driver to XFree86 4.0 was done by:
 * David Dawes <dawes@xfree86.org>
 * Dirk H. Hohndel <hohndel@xfree86.org>
 * Marc Aurele La France <tsi@ualberta.ca>
 */

#include "xf86.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "vgaHW.h"

#undef  PSZ
#define PSZ 8
#include "cfb.h"
#undef  PSZ

#include "xf4bpp.h"
#include "xf1bpp.h"

#include "mipointer.h"

/* Some systems define VGA for their own purposes */
#undef VGA

/* A few things all drivers should have */
#define VGA_NAME            "VGA"
#define VGA_DRIVER_NAME     "vga"
#define VGA_VERSION_NAME    "4.0"
#define VGA_VERSION_MAJOR   4
#define VGA_VERSION_MINOR   0
#define VGA_VERSION_CURRENT ((VGA_VERSION_MAJOR << 16) | VGA_VERSION_MINOR)


/* Forward definitions */
static void       GenericIdentify(int);
static Bool       GenericProbe(DriverPtr, int);
static Bool       GenericPreInit(ScrnInfoPtr, int);
static Bool       GenericScreenInit(int, ScreenPtr, int, char **);
static Bool       GenericSwitchMode(int, DisplayModePtr, int);
static void       GenericAdjustFrame(int, int, int, int);
static Bool       GenericEnterVT(int, int);
static void       GenericLeaveVT(int, int);
static void       GenericFreeScreen(int, int);
static int        VGAFindIsaDevice();

static ModeStatus GenericValidMode(int, DisplayModePtr, Bool, int);

/* The root of all evil... */
DriverRec VGA =
{
    VGA_VERSION_CURRENT,
    "Generic VGA driver",
    GenericIdentify,
    GenericProbe,
    NULL,
    0
};


#ifdef XFree86LOADER

/* Module loader interface */

MODULEINITPROTO(vgaModuleInit);
static MODULESETUPPROTO(GenericSetup);

static XF86ModuleVersionInfo GenericVersionRec =
{
    VGA_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    VGA_VERSION_CURRENT,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    {0, 0, 0, 0}
};

/*
 * This function is called by the loader to initialise the module.  The
 * function name must be the module name followed by "ModuleInit".
 */
void
vgaModuleInit(XF86ModuleVersionInfo **VersionPtr, ModuleSetupProc *Setup,
              ModuleTearDownProc *TearDown)
{
    *VersionPtr = &GenericVersionRec;
    *Setup      = GenericSetup;
    *TearDown   = NULL;
}

static pointer
GenericSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised)
    {
        Initialised = TRUE;
        xf86AddDriver(&VGA, Module, 0);
        return (pointer)TRUE;
    }

    if (ErrorMajor)
        *ErrorMajor = LDR_ONCEONLY;
    return NULL;
}

#endif


enum GenericTypes
{
    CHIP_VGA_GENERIC
};

/* Supported chipsets */
static SymTabRec GenericChipsets[] =
{
    {CHIP_VGA_GENERIC, "generic"},
    {-1,               NULL}
};

static IsaChipsets GenericISAchipsets[] = {
  {CHIP_VGA_GENERIC, RES_VGA},
  {-1,                0 }
};

static void
GenericIdentify(int flags)
{
    xf86PrintChipsets(VGA_NAME,
        "Generic VGA driver (version " VGA_VERSION_NAME ") for chipsets",
        GenericChipsets);
}


/*
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */


static Bool
GenericProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int nGDev, numUsed;
    GDevPtr *GDevs, GenericGDev = NULL;
    int usedChip;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((nGDev = xf86MatchDevice(VGA_NAME, &GDevs)) <= 0) {
	return foundScreen;
    }
  
    /*
     * XXX Should get info about PrimaryDevice first and determine if
     * something else has claimed it before enabling it.
     */

    xf86EnablePrimaryDevice();

    usedChip = xf86MatchIsaInstances(VGA_NAME, GenericChipsets,
				     GenericISAchipsets,
				     VGAFindIsaDevice,GDevs,
				     nGDev,&GenericGDev);
    if(usedChip >= 0)
    {
	ScrnInfoPtr pScrn;
	int Resource = xf86FindIsaResource(usedChip, GenericISAchipsets);
	  
	pScrn = xf86AllocateScreen(drv, 0);
	xf86ClaimIsaSlot(Resource, &VGA, usedChip, pScrn->scrnIndex);
	pScrn->driverVersion = VGA_VERSION_CURRENT;
	pScrn->driverName    = VGA_DRIVER_NAME;
	pScrn->name          = VGA_NAME;
	pScrn->Probe         = GenericProbe;
	pScrn->PreInit       = GenericPreInit;
	pScrn->ScreenInit    = GenericScreenInit;
	pScrn->SwitchMode    = GenericSwitchMode;
	pScrn->AdjustFrame   = GenericAdjustFrame;
	pScrn->EnterVT       = GenericEnterVT;
	pScrn->LeaveVT       = GenericLeaveVT;
	pScrn->FreeScreen    = GenericFreeScreen;
	pScrn->ValidMode     = GenericValidMode;
	pScrn->device        = GenericGDev;
	foundScreen = TRUE;
    }

    xfree(GDevs);
    return foundScreen;
}

static int
VGAFindIsaDevice()
{
#ifndef PC98_EGC
    CARD16 GenericIOBase = VGAHW_GET_IOBASE();
    CARD8 CurrentValue, TestValue;

    /* Unlock VGA registers */
    VGAHW_UNLOCK(GenericIOBase);

    /* VGA has one more read/write attribute register than EGA */
    (void) inb(GenericIOBase + 0x0AU);  /* Reset flip-flop */
    outb(0x3C0, 0x14 | 0x20);
    CurrentValue = inb(0x3C1);
    outb(0x3C0, CurrentValue ^ 0x0F);
    outb(0x3C0, 0x14 | 0x20);
    TestValue = inb(0x3C1);
    outb(0x3C0, CurrentValue);

    /* XXX:  This should restore lock state, rather than relock */
    VGAHW_LOCK(GenericIOBase);

    /* Quit now if no VGA is present */
    if ((CurrentValue ^ 0x0F) != TestValue)
	return -1;
#endif
    return (int)CHIP_VGA_GENERIC;
}

static Bool
GenericClockSelect(ScrnInfoPtr pScreenInfo, int ClockNumber)
{
#   ifndef PC98_EGC
        static CARD8 save_misc;

        switch (ClockNumber)
        {
            case CLK_REG_SAVE:
                save_misc = inb(0x3CC);
                break;

            case CLK_REG_RESTORE:
                outb(0x3C2, save_misc);
                break;

            default:
                outb(0x3C2, (save_misc & 0xF3) | ((ClockNumber << 2) & 0x0C));
                break;
        }
#   endif

    return TRUE;
}


/*
 * This structure is used to wrap the screen's CloseScreen vector.
 */
typedef struct _GenericRec
{
    CloseScreenProcPtr CloseScreen;
} GenericRec, *GenericPtr;


static GenericPtr
GenericGetRec(ScrnInfoPtr pScreenInfo)
{
    if (!pScreenInfo->driverPrivate)
        pScreenInfo->driverPrivate = xcalloc(sizeof(GenericRec), 1);

    return (GenericPtr)pScreenInfo->driverPrivate;
}


static void
GenericFreeRec(ScrnInfoPtr pScreenInfo)
{
    vgaHWFreeHWRec(pScreenInfo);
    xfree(pScreenInfo->driverPrivate);
    pScreenInfo->driverPrivate = NULL;
}


static void
GenericProtect(ScrnInfoPtr pScreenInfo, Bool On)
{
    vgaHWProtect(pScreenInfo, On);
}


static Bool
GenericSaveScreen(ScreenPtr pScreen, Bool Unblank)
{
    return vgaHWSaveScreen(pScreen, Unblank);
}

static void
GenericBlankScreen(ScrnInfoPtr pScreenInfo, Bool Unblank)
{
    vgaHWBlankScreen(pScreenInfo, Unblank);
}


/* The default mode */
static DisplayModeRec GenericDefaultMode =
{
    NULL, NULL,                         /* prev & next */
    "Generic 320x200 default mode",
    MODE_OK,                            /* Mode status */
    12588,                              /* Pixel clock */
    320, 336, 384, 400,                 /* HTiming */
    0,                                  /* HSkew */
    200, 206, 207, 224,                 /* VTiming */
    2,                                  /* VScan */
    V_CLKDIV2 | V_NHSYNC | V_PVSYNC,    /* Flags */
    0, 0,                               /* ClockIndex & SynthClock */
    0, 0, 0, 0, 0, 0,                   /* Crtc timings set by ... */
    0,                                  /* ... xf86SetCrtcForModes() */
    0, 0, 0, 0, 0, 0,
    FALSE, FALSE,                       /* These are unadjusted timings */
    0, NULL                             /* PrivSize & Private */
};


/*
 * This function is called once for each screen at the start of the first
 * server generation to initialise the screen for all server generations.
 */
static Bool
GenericPreInit(ScrnInfoPtr pScreenInfo, int flags)
{
    static rgb        defaultWeight = {0, 0, 0};
    static ClockRange GenericClockRange = {NULL, 0, 80000, 0, FALSE, TRUE, 1, 1, 0};
    MessageType       From;
    int               i, videoRam, Rounding, nModes = 0;
    char             *Module;
    vgaHWPtr          pvgaHW;

    /* Set the monitor */
    pScreenInfo->monitor = pScreenInfo->confScreen->monitor;

    /* Print chipset, but don't set it yet */
    if (pScreenInfo->chipset)
        xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG, "Chipset: \"%s\".\n",
            pScreenInfo->chipset);
    else
        xf86DrvMsg(pScreenInfo->scrnIndex, X_PROBED, "Chipset: \"%s\".\n",
            GenericChipsets[0].name);

    /* Determine depth, bpp, etc. */
    if (!xf86SetDepthBpp(pScreenInfo, 4, 0, 4, NoDepth24Support))
        return FALSE;

    switch (pScreenInfo->depth)
    {
        case 1:  Module = "xf1bpp";  break;
        case 4:  Module = "xf4bpp";  break;
        case 8:  Module = "cfb";     break;

        default:
            xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
                "Given depth (%d) is not supported by this driver.\n",
                pScreenInfo->depth);
            return FALSE;
    }
    xf86PrintDepthBpp(pScreenInfo);

    /* Determine colour weights */
    pScreenInfo->rgbBits = 6;
    if (!xf86SetWeight(pScreenInfo, defaultWeight, defaultWeight))
        return FALSE;

    /* XXX:  Check that returned weight is supported */

    /* Determine default visual */
    if (!xf86SetDefaultVisual(pScreenInfo, -1))
        return FALSE;

    /*
     * Determine videoRam.  For mode validation purposes, this needs to be
     * limited to VGA specifications.
     */
    if ((videoRam = pScreenInfo->device->videoRam))
    {
        pScreenInfo->videoRam = videoRam;
        if (pScreenInfo->depth == 8)
        {
            if (videoRam > 64)
                pScreenInfo->videoRam = 64;
        }
        else
        {
            if (videoRam > 256)
                pScreenInfo->videoRam = 256;
        }
        From = X_CONFIG;
    }
    else
    {
        if (pScreenInfo->depth == 8)
            videoRam = 64;
        else
            videoRam = 256;
        pScreenInfo->videoRam = videoRam;
        From = X_DEFAULT;       /* Instead of X_PROBED */
    }
    if (pScreenInfo->depth == 1)
        pScreenInfo->videoRam >>= 2;
    xf86DrvMsg(pScreenInfo->scrnIndex, From, "videoRam: %d kBytes", videoRam);
    if (videoRam != pScreenInfo->videoRam)
        xf86ErrorF(" (using %d)", pScreenInfo->videoRam);
    xf86ErrorF(".\n");

    /* Ensure vgahw entry points are available for the clock probe */
    if (!xf86LoadSubModule(pScreenInfo, "vgahw"))
        return FALSE;

    /* Ensure depth-specific entry points are available */
    if (!xf86LoadSubModule(pScreenInfo, Module))
        return FALSE;

    /* Allocate driver private structure */
    if (!GenericGetRec(pScreenInfo))
        return FALSE;

    /* Ensure vgahw private structure is allocated */
    if (!vgaHWGetHWRec(pScreenInfo))
        return FALSE;

    pvgaHW = VGAHWPTR(pScreenInfo);
    pvgaHW->MapSize = 0x00010000;       /* Standard 64kB VGA window */
    vgaHWGetIOBase(pvgaHW);             /* Get VGA I/O base */

#ifndef __NOT_YET__
    if (pScreenInfo->depth == 8)
    {
        pScreenInfo->numClocks = 1;
        pScreenInfo->clock[0] = 25175;
        goto SetDefaultMode;
    }
#endif

    /*
     * Determine clocks.  Limit them to the first four because that's all that
     * can be addressed.
     */
    if ((pScreenInfo->numClocks = pScreenInfo->device->numclocks))
    {
        if (pScreenInfo->numClocks > 4)
            pScreenInfo->numClocks = 4;
        for (i = 0;  i < pScreenInfo->numClocks;  i++)
            pScreenInfo->clock[i] = pScreenInfo->device->clock[i];
        From = X_CONFIG;
    }
    else
    {
        xf86GetClocks(pScreenInfo, 4, GenericClockSelect, GenericProtect,
            GenericBlankScreen, VGAHW_GET_IOBASE() + 0x0A, 0x08, 1, 28322);
        From = X_PROBED;
    }
    xf86ShowClocks(pScreenInfo, From);

    if (pScreenInfo->display->modes && pScreenInfo->display->modes[0])
    {
        /* Set the virtual X rounding (in bits) */
        if (pScreenInfo->depth == 8)
            Rounding = 16 * 8;
        else
            Rounding = 16;

        /*
         * Validate the modes.  Note that the limits passed to
         * xf86ValidateModes() are VGA CRTC architectural limits.
         */
        pScreenInfo->maxHValue = 2080;
        pScreenInfo->maxVValue = 1025;
        nModes = xf86ValidateModes(pScreenInfo,
            pScreenInfo->monitor->Modes, pScreenInfo->display->modes,
            &GenericClockRange, NULL, 8, 2040, Rounding, 1, 1024,
            pScreenInfo->display->virtualX, pScreenInfo->display->virtualY,
            0x10000, LOOKUP_CLOSEST_CLOCK | LOOKUP_CLKDIV2);

        if (nModes < 0)
            return FALSE;

        /* Remove invalid modes */
        xf86PruneDriverModes(pScreenInfo);
    }

    if (!nModes || !pScreenInfo->modes)
    {
#ifndef __NOT_YET__
  SetDefaultMode:
#endif
        /* Set a default mode, overridding any virtual settings */
        pScreenInfo->virtualX = pScreenInfo->displayWidth = 320;
        pScreenInfo->virtualY = 200;
        pScreenInfo->modes = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
        if (!pScreenInfo->modes)
            return FALSE;
        *pScreenInfo->modes = GenericDefaultMode;
        pScreenInfo->modes->prev = pScreenInfo->modes;
        pScreenInfo->modes->next = pScreenInfo->modes;

        pScreenInfo->virtualFrom = X_DEFAULT;
    }

    /* Set CRTC values for the modes */
    xf86SetCrtcForModes(pScreenInfo, 0);

    /* Set current mode to the first in list */
    pScreenInfo->currentMode = pScreenInfo->modes;

    /* Print mode list */
    xf86PrintModes(pScreenInfo);

    /* Set display resolution */
    xf86SetDpi(pScreenInfo, 0, 0);

    /* Deal with options */
    xf86CollectOptions(pScreenInfo, NULL);

    /* Only one chipset here */
    if (!pScreenInfo->chipset)
        pScreenInfo->chipset = (char *)GenericChipsets[0].name;

    return TRUE;        /* Tada! */
}


/* Save mode on server entry */
static void
GenericSave(ScrnInfoPtr pScreenInfo)
{
    vgaHWSave(pScreenInfo, &VGAHWPTR(pScreenInfo)->SavedReg, TRUE);
}


/* Restore the mode that was saved on server entry */
static void
GenericRestore(ScrnInfoPtr pScreenInfo)
{
    vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);

    vgaHWProtect(pScreenInfo, TRUE);
    vgaHWRestore(pScreenInfo, &pvgaHW->SavedReg, TRUE);
    vgaHWProtect(pScreenInfo, FALSE);
}


/* Set a graphics mode */
static Bool
GenericSetMode(ScrnInfoPtr pScreenInfo, DisplayModePtr pMode)
{
    vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);

    if (!vgaHWInit(pScreenInfo, pMode))
        return FALSE;
    pScreenInfo->vtSema = TRUE;

#ifndef __NOT_YET__
    if (pScreenInfo->depth == 8)
    {
        int i;

        static const CARD8 CRTC[24] =
        {
            0x5F, 0x4F, 0x4F, 0x80, 0x54, 0x00, 0xBF, 0x1F,
            0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x8F, 0xC0, 0xA3
        };

        /* Override vgaHW's CRTC timings */
        for (i = 0;  i < 24;  i++)
            pvgaHW->ModeReg.CRTC[i] = CRTC[i];

        /* Clobber any CLKDIV2 */
        pvgaHW->ModeReg.Sequencer[1] = 0x01;
    }
#endif

    /* Programme the registers */
    vgaHWProtect(pScreenInfo, TRUE);
    vgaHWRestore(pScreenInfo, &pvgaHW->ModeReg, FALSE);
    vgaHWProtect(pScreenInfo, FALSE);

    return TRUE;
}


static Bool
GenericEnterGraphics(ScreenPtr pScreen, ScrnInfoPtr pScreenInfo)
{
    vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);

    /* Map VGA aperture */
    if (!vgaHWMapMem(pScreenInfo))
        return FALSE;

    /* Unlock VGA registers */
    vgaHWUnlock(pvgaHW);

    /* Save the current state and setup the current mode */
    GenericSave(pScreenInfo);
    if (!GenericSetMode(pScreenInfo, pScreenInfo->currentMode))
        return FALSE;

    /* Possibly blank the screen */
    if (pScreen)
        GenericSaveScreen(pScreen, FALSE);

    (*pScreenInfo->AdjustFrame)(pScreenInfo->scrnIndex,
        pScreenInfo->frameX0, pScreenInfo->frameY0, 0);

    return TRUE;
}


static void
GenericLeaveGraphics(ScrnInfoPtr pScreenInfo)
{
    GenericRestore(pScreenInfo);
    vgaHWLock(VGAHWPTR(pScreenInfo));
    vgaHWUnmapMem(pScreenInfo);
}


/* Unravel the screen */
static Bool
GenericCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
    GenericPtr pGenericPriv = GenericGetRec(pScreenInfo);
    Bool Closed = TRUE;

    if (pGenericPriv && (pScreen->CloseScreen = pGenericPriv->CloseScreen))
    {
        pGenericPriv->CloseScreen = NULL;
        Closed = (*pScreen->CloseScreen)(scrnIndex, pScreen);
    }

    GenericLeaveGraphics(pScreenInfo);
    pScreenInfo->vtSema = FALSE;

    return Closed;
}


#ifdef DPMSExtension
static void
GenericDPMSSet(ScrnInfoPtr pScreen, int mode, int flags)
{
    vgaHWDPMSSet(pScreen, mode, flags);
}
#endif


static Bool
GenericScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
    vgaHWPtr pvgaHW;
    GenericPtr pGenericPriv;
    int savedDefaultVisualClass;
    Bool Inited = FALSE;

    /* Get driver private */
    pGenericPriv = GenericGetRec(pScreenInfo);

    /* Initialise graphics mode */
    if (!GenericEnterGraphics(pScreen, pScreenInfo))
        return FALSE;

    /* Get vgahw private */
    pvgaHW = VGAHWPTR(pScreenInfo);

    /* Temporarily set the global defaultColorVisualClass */
    savedDefaultVisualClass = xf86GetDefaultColorVisualClass();
    xf86SetDefaultColorVisualClass(pScreenInfo->defaultVisual);

    /* Initialise the framebuffer */
    switch (pScreenInfo->depth)
    {
        case 1:
            Inited = xf1bppScreenInit(pScreen, pvgaHW->Base,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 4:
            Inited = xf4bppScreenInit(pScreen, pvgaHW->Base,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;

        case 8:
            cfbClearVisualTypes();

            Inited = cfbScreenInit(pScreen, pvgaHW->Base,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;
    }

    /* Restore defaultColorVisualClass */
    xf86SetDefaultColorVisualClass(savedDefaultVisualClass);

    if (!Inited)
        return FALSE;

    miInitializeBackingStore(pScreen);

    xf86SetBlackWhitePixels(pScreen);

    if (pScreenInfo->depth > 1)
        vgaHandleColormaps(pScreen, pScreenInfo);

    /* Initialise cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Setup default colourmap */
    switch (pScreenInfo->depth)
    {
        case 1:
            Inited = xf1bppCreateDefColormap(pScreen);
            break;

        case 4:
            Inited = xf4bppCreateDefColormap(pScreen);
            break;

        default:
            Inited = cfbCreateDefColormap(pScreen);
            break;
    }

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, GenericDPMSSet, 0);
#endif

    /* Wrap the screen's CloseScreen vector and set its SaveScreen vector */
    pGenericPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = GenericCloseScreen;
    pScreen->SaveScreen = GenericSaveScreen;

    if (!Inited)
        GenericCloseScreen(scrnIndex, pScreen);

    if (serverGeneration == 1)
	xf86ShowUnusedOptions(pScreenInfo->scrnIndex, pScreenInfo->options);

    return Inited;
}


static Bool
GenericSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
    return GenericSetMode(xf86Screens[scrnIndex], pMode);
}


static void
GenericAdjustFrame(int scrnIndex, int x, int y, int flags)
{
#   ifndef PC98_EGC
        ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
        vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);
        int Base = (y * pScreenInfo->displayWidth + x) >> 3;

        outw(pvgaHW->IOBase + 4, (Base & 0x00FF00) | 0x0C);
        outw(pvgaHW->IOBase + 4, ((Base & 0x0000FF) << 8) | 0x0D);
#   endif
}


static Bool
GenericEnterVT(int scrnIndex, int flags)
{
     return GenericEnterGraphics(NULL, xf86Screens[scrnIndex]);
}


static void
GenericLeaveVT(int scrnIndex, int flags)
{
    GenericLeaveGraphics(xf86Screens[scrnIndex]);
}


static void
GenericFreeScreen(int scrnIndex, int flags)
{
    GenericFreeRec(xf86Screens[scrnIndex]);
}


static ModeStatus
GenericValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
    if (pMode->Flags & V_INTERLACE)
        return MODE_NO_INTERLACE;

    return MODE_OK;
}
