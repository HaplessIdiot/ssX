/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/rendition.c,v 1.1 1999/04/17 07:06:33 dawes Exp $ */
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
 * This is essentially a transfer of the 3.3 sources written by 
 * Marc Langenbach and Tim Rowley.
 *
 * The initial port of this driver to XFree86 4.0 was done by
 * Marc Langenbach <mlangen@studcs.uni-sb.de>
 */

/*
 * includes 
 */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "compiler.h"
#include "vgaHW.h"

/* This is used for module versioning */
#include "xf86Version.h"

#undef  PSZ
#define PSZ 8
#include "cfb.h"
#undef  PSZ
#include "cfb16.h"
#include "cfb32.h"

#include "mipointer.h"
#include "micmap.h"

#include "xf86RAC.h"

#include "vtypes.h"
#include "vboard.h"
#include "vmodes.h"
#include "vvga.h"



/*
 * defines
 */

#undef DEBUG

#define RENDITION_NAME            "Rendition"
#define RENDITION_DRIVER_NAME     "rendition"
#define RENDITION_VERSION_NAME    "4.0"
#define RENDITION_VERSION_MAJOR   4
#define RENDITION_VERSION_MINOR   0
#define RENDITION_PATCHLEVEL      0
#define RENDITION_VERSION_CURRENT ((RENDITION_VERSION_MAJOR << 24) | \
                 (RENDITION_VERSION_MINOR << 16) | RENDITION_PATCHLEVEL)

#define RENDITIONPTR(p)     ((renditionPtr)((p)->driverPrivate))


/* 
 * local function prototypes
 */

static void       renditionIdentify(int);
static Bool       renditionProbe(DriverPtr, int);
static Bool       renditionPreInit(ScrnInfoPtr, int);
static Bool       renditionScreenInit(int, ScreenPtr, int, char **);
static Bool       renditionSwitchMode(int, DisplayModePtr, int);
static void       renditionAdjustFrame(int, int, int, int);
static Bool       renditionEnterVT(int, int);
static void       renditionLeaveVT(int, int);
static void       renditionFreeScreen(int, int);

static ModeStatus renditionValidMode(int, DisplayModePtr, Bool, int);
static Bool renditionMapMem(ScrnInfoPtr pScreenInfo);
static Bool renditionUnmapMem(ScrnInfoPtr pScreenInfo);



/* 
 * global data
 */

DriverRec RENDITION={
    RENDITION_VERSION_CURRENT,
    "rendition driver",
    renditionIdentify,
    renditionProbe,
    NULL,
    0
};

static const char *vgahwSymbols[]={
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWProtect",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWUnmapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    "vgaHWSave",
    "vgaHWRestore",
    "vgaHWGetIndex",
    "vgaHWDPMSSet",
    "vgaHWBlankScreen",
    "vgaHWHandleColormaps",
    NULL
};

static const char *fbSymbols[]={
    "xf1bppScreenInit",
    "xf4bppScreenInit",
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb32ScreenInit",
    NULL
};

static const char *racSymbols[] = {
    "xf86RACInit",
    NULL
};

#ifdef XFree86LOADER

/* Module loader interface */

static MODULESETUPPROTO(renditionSetup);

static XF86ModuleVersionInfo renditionVersionRec = {
    RENDITION_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    RENDITION_VERSION_MAJOR, RENDITION_VERSION_MINOR, RENDITION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

XF86ModuleData renditionModuleData = 
               { &renditionVersionRec, renditionSetup, NULL };

static pointer
renditionSetup(pointer Module, pointer Options, int *ErrorMajor, 
               int *ErrorMinor)
{
    static Bool Initialised=FALSE;

    if (!Initialised) {
        Initialised=TRUE;
        xf86AddDriver(&RENDITION, Module, 0);
        LoaderRefSymLists(vgahwSymbols, fbSymbols, racSymbols, NULL);
        return (pointer)TRUE;
    }

    if (ErrorMajor)
        *ErrorMajor=LDR_ONCEONLY;

    return NULL;
}

#endif


enum renditionTypes {
    CHIP_RENDITION_V1000,
    CHIP_RENDITION_V2x00
};

/* supported chipsets */
static SymTabRec renditionChipsets[] = {
    {CHIP_RENDITION_V1000, "Verite 1000"},
    {CHIP_RENDITION_V2x00, "Verite 2x00"},
    {-1,                   NULL}
};

static PciChipsets renditionPCIchipsets[] = {
  { CHIP_RENDITION_V1000, PCI_CHIP_V1000, RES_SHARED_VGA },
  { CHIP_RENDITION_V2x00, PCI_CHIP_V2x00, RES_SHARED_VGA },
  { -1,                   -1,             RES_UNDEFINED }
};

/* supported options */
typedef enum {
    OPTION_FBWC,
    OPTION_SW_CURSOR,
    OPTION_NOACCEL
} renditionOpts;

static OptionInfoRec renditionOptions[]={
    { OPTION_FBWC,      "FramebufferWC", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_SW_CURSOR, "SWCursor", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_NOACCEL,   "NoAccel",  OPTV_BOOLEAN, {0}, FALSE },
    { -1,                NULL,      OPTV_NONE,    {0}, FALSE }
};



/*
 * functions
 */

static void
renditionIdentify(int flags)
{
    xf86PrintChipsets(RENDITION_NAME,
        "rendition driver (version " RENDITION_VERSION_NAME ") for chipsets",
        renditionChipsets);
}



/*
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */
static Bool
renditionProbe(DriverPtr drv, int flags)
{
    Bool foundScreen=FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections, *usedDevs;
    pciVideoPtr *usedPci;
    int *usedChips;
    int c;

    /* Find the config file Device sections that match this
     * driver, and return if there are none. */
    if ((numDevSections=xf86MatchDevice(RENDITION_NAME, &devSections)) <= 0)
        return FALSE;
  
    /* PCI BUS */
    if (xf86GetPciVideoInfo()) {
        numUsed=xf86MatchPciInstances(RENDITION_NAME, PCI_VENDOR_RENDITION,
                    renditionChipsets, renditionPCIchipsets, 
                    devSections,numDevSections,
                    &usedDevs, &usedPci, &usedChips);
        if (numUsed > 0) {
            for (c=0; c<numUsed; c++) {
                pciVideoPtr pPci;
                BusResource Resource;
                pPci=usedPci[c];
                Resource=xf86FindPciResource(usedChips[c], 
                    renditionPCIchipsets);

                if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func,
                        Resource)) {  
                    ScrnInfoPtr pScrn;
                    /* Allocate a ScrnInfoRec and claim the slot */
                    pScrn=xf86AllocateScreen(drv, 0);
                    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func,
                        Resource, &RENDITION, usedChips[c], pScrn->scrnIndex);
                    pScrn->driverVersion=RENDITION_VERSION_CURRENT;
                    pScrn->driverName   =RENDITION_DRIVER_NAME;
                    pScrn->name         =RENDITION_NAME;
                    pScrn->Probe        =renditionProbe;
                    pScrn->PreInit      =renditionPreInit;
                    pScrn->ScreenInit   =renditionScreenInit;
                    pScrn->SwitchMode   =renditionSwitchMode;
                    pScrn->AdjustFrame  =renditionAdjustFrame;
                    pScrn->EnterVT      =renditionEnterVT;
                    pScrn->LeaveVT      =renditionLeaveVT;
                    pScrn->FreeScreen   =renditionFreeScreen;
                    pScrn->ValidMode    =renditionValidMode;
                    pScrn->device       =usedDevs[c];
                    foundScreen=TRUE;
                }
            }
      
            xfree(usedDevs);
            xfree(usedPci);
        }
    }

    xfree(devSections);
    return foundScreen;
}



static Bool
renditionClockSelect(ScrnInfoPtr pScreenInfo, int ClockNumber)
{
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

    return TRUE;
}


/*
 * This structure is used to wrap the screen's CloseScreen vector.
 */
typedef struct _renditionRec
{
    struct v_board_t board;             /* information on the board */
    struct v_modeinfo_t mode;           /* information on the mode */
    int pcitag;                         /* something for mapping? */
    CloseScreenProcPtr CloseScreen;     /* no idea why this is here ;) */
} renditionRec, *renditionPtr;


static renditionPtr
renditionGetRec(ScrnInfoPtr pScreenInfo)
{
    if (!pScreenInfo->driverPrivate)
        pScreenInfo->driverPrivate=xcalloc(sizeof(renditionRec), 1);

    /* perhaps some initialization? <ml> */

    return (renditionPtr)pScreenInfo->driverPrivate;
}


static void
renditionFreeRec(ScrnInfoPtr pScreenInfo)
{
    vgaHWFreeHWRec(pScreenInfo);
    xfree(pScreenInfo->driverPrivate);
    pScreenInfo->driverPrivate=NULL;
}


static void
renditionProtect(ScrnInfoPtr pScreenInfo, Bool On)
{
    vgaHWProtect(pScreenInfo, On);
}


static Bool
renditionSaveScreen(ScreenPtr pScreen, Bool Unblank)
{
    return vgaHWSaveScreen(pScreen, Unblank);
}

static void
renditionBlankScreen(ScrnInfoPtr pScreenInfo, Bool Unblank)
{
    vgaHWBlankScreen(pScreenInfo, Unblank);
}


/* the default mode -- suppose we do not need this <ml> */
static DisplayModeRec renditionDefaultMode =
{
    NULL, NULL,                         /* prev & next */
    "rendition 320x200 default mode",
    MODE_OK,                            /* Mode status */
    M_T_CRTC_C,                         /* Mode type   */
    12588,                              /* Pixel clock */
    320, 336, 384, 400,                 /* HTiming */
    0,                                  /* HSkew */
    200, 206, 207, 224,                 /* VTiming */
    2,                                  /* VScan */
    V_CLKDIV2 | V_NHSYNC | V_PVSYNC,    /* Flags */
    0, 25176,                           /* ClockIndex & SynthClock */
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
renditionPreInit(ScrnInfoPtr pScreenInfo, int flags)
{
    static ClockRange renditionClockRange = {NULL, 0, 135000, -1, FALSE, TRUE, 1, 1, 0};
    MessageType       From;
    int               i, videoRam, Rounding, nModes = 0;
    char             *Module;
    const char       *Sym;
    vgaHWPtr          pvgaHW;
    pciVideoPtr       *pciList;

#ifdef DEBUG
    ErrorF("Rendition: renditionPreInit() called\n");
#endif

    xf86AddControlledResource(pScreenInfo,IO);
    xf86EnableAccess(&pScreenInfo->Access);

    /* set the monitor */
    pScreenInfo->monitor=pScreenInfo->confScreen->monitor;

    /* determine depth, bpp, etc. */
    if (!xf86SetDepthBpp(pScreenInfo, 8, 8, 8, NoDepth24Support))
        return FALSE;

    switch (pScreenInfo->depth) {
        case 8:   Module = "cfb";   Sym = "cfbScreenInit";   break;
        case 16:  Module = "cfb16"; Sym = "cfbScreenInit16"; break;
        case 24:  Module = "cfb32"; Sym = "cfbScreenInit32"; break;

        default:
            xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
                "Given depth (%d) is not supported by this driver.\n",
                pScreenInfo->depth);
            return FALSE;
    }

    /* Ensure depth-specific entry points are available */
    if (!xf86LoadSubModule(pScreenInfo, Module))
      return FALSE;

    xf86LoaderReqSymbols(Sym, NULL);

    /* determine colour weights */
    pScreenInfo->rgbBits=8;

    if (pScreenInfo->depth > 8) {
      rgb zeros = {0, 0, 0};

      xf86PrintDepthBpp(pScreenInfo);

      /* Standard defaults are OK if depths are OK */
      if (!xf86SetWeight(pScreenInfo, zeros, zeros))
        return FALSE;
      else{
	/* XXX:  Check that returned weight is supported */
      }
    }

    /* determine default visual */
    if (!xf86SetDefaultVisual(pScreenInfo, -1))
        return FALSE;

    /* the gamma fields must be initialised when using the new cmap code */
    if (pScreenInfo->depth > 1) {
        Gamma zeros = {0.0, 0.0, 0.0};

        if (!xf86SetGamma(pScreenInfo, zeros))
            return FALSE;
    }

    /* the Rendition chips have a programmable clock */
    pScreenInfo->progClock=TRUE;

    /* allocate driver private structure */
    if (!renditionGetRec(pScreenInfo))
        return FALSE;
    /* pRendition=RENDITIONPTR(PScrn); Haeh? <ml> */

    /* collect all of the options flags and process them */
    /* Deal with options */

    xf86CollectOptions(pScreenInfo, NULL);
    xf86ProcessOptions(pScreenInfo->scrnIndex, pScreenInfo->options, 
        renditionOptions);

    /* set various fields according to the given options */
    /* to be filled in <ml> */

    /* now determine some hardware characteristics */
    if ((i=xf86GetPciInfoForScreen(pScreenInfo->scrnIndex, &pciList, NULL)) 
            != 1) {
        xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
            "Some error occured during accessing board information\n");
        renditionFreeRec(pScreenInfo);
        if (i > 0)
            xfree(pciList);
        return FALSE;
    }
    /* for now I assume i equals 1 and pciList[0]->... references the right data
     * <ml> */
    if (PCI_CHIP_V1000==pciList[0]->chipType){
      RENDITIONPTR(pScreenInfo)->board.chip=V1000_DEVICE;
    }
    else {
      RENDITIONPTR(pScreenInfo)->board.chip=V2000_DEVICE;
      renditionClockRange.maxClock = 170000;
      renditionClockRange.clockIndex = -1;
    }

    RENDITIONPTR(pScreenInfo)->board.accel=0;
    RENDITIONPTR(pScreenInfo)->board.io_base=pciList[0]->ioBase[1];
    RENDITIONPTR(pScreenInfo)->board.mmio_base=0;
    RENDITIONPTR(pScreenInfo)->board.vmmio_base=0;
    RENDITIONPTR(pScreenInfo)->board.mem_size=0;
    RENDITIONPTR(pScreenInfo)->board.mem_base=(vu8 *)pciList[0]->memBase[0];
    RENDITIONPTR(pScreenInfo)->board.vmem_base=NULL;
    RENDITIONPTR(pScreenInfo)->board.init=0;

    if (pScreenInfo->chipset)
        xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG, "Chipset: \"%s\".\n",
            pScreenInfo->chipset);
    else
        xf86DrvMsg(pScreenInfo->scrnIndex, X_PROBED, "Chipset: \"%s\".\n",
            renditionChipsets[
        RENDITIONPTR(pScreenInfo)->board.chip==V1000_DEVICE ? 0:1].name);

    /* I do not get the IO base addres <ml> */
    ErrorF("Rendition %s @ %x/%x\n",renditionChipsets[
      RENDITIONPTR(pScreenInfo)->board.chip==V1000_DEVICE ? 0:1].name,
        RENDITIONPTR(pScreenInfo)->board.io_base,
        RENDITIONPTR(pScreenInfo)->board.mem_base);
    /* so I hardcoded it <ml> */
    /* RENDITIONPTR(pScreenInfo)->board.io_base=0x9800; */

    RENDITIONPTR(pScreenInfo)->pcitag=
        pciTag(pciList[0]->bus, pciList[0]->device, pciList[0]->func);

    /* determine video ram -- to do so, we assume a full size memory of 16M,
     * then map it and use v_getmemorysize() to determine the real amount of
     * memory */
    pScreenInfo->videoRam=RENDITIONPTR(pScreenInfo)->board.mem_size=16<<20;
    renditionMapMem(pScreenInfo);
    videoRam=v_getmemorysize(&RENDITIONPTR(pScreenInfo)->board)>>10;
    renditionUnmapMem(pScreenInfo);
    From = X_PROBED;
    xf86DrvMsg(pScreenInfo->scrnIndex, From, "videoRam: %d kBytes\n", videoRam);
    pScreenInfo->videoRam=videoRam;
    RENDITIONPTR(pScreenInfo)->board.mem_size=videoRam;

    if (!xf86LoadSubModule(pScreenInfo, "vgahw"))
        return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    /* ensure vgahw private structure is allocated */
    if (!vgaHWGetHWRec(pScreenInfo))
        return FALSE;

    pvgaHW = VGAHWPTR(pScreenInfo);
    pvgaHW->MapSize = 0x00010000;       /* Standard 64kB VGA window */
    vgaHWGetIOBase(pvgaHW);             /* Get VGA I/O base */

#if 0
    /* Defaultmode needed ? <DI> */
    if (pScreenInfo->depth == 8) {
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
        xf86GetClocks(pScreenInfo, 4, renditionClockSelect, renditionProtect,
            renditionBlankScreen, VGAHW_GET_IOBASE() + 0x0A, 0x08, 1, 28322);
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
            &renditionClockRange, NULL, 8, 2040, Rounding, 1, 1024,
            pScreenInfo->display->virtualX, pScreenInfo->display->virtualY,
            0x10000, LOOKUP_CLOSEST_CLOCK | LOOKUP_CLKDIV2);

        if (nModes < 0)
            return FALSE;

        /* Remove invalid modes */
        xf86PruneDriverModes(pScreenInfo);
    }

    if (!nModes || !pScreenInfo->modes)
    {
  SetDefaultMode:
        /* Set a default mode, overridding any virtual settings */
        pScreenInfo->virtualX = pScreenInfo->displayWidth = 320;
        pScreenInfo->virtualY = 200;
        pScreenInfo->modes = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
        if (!pScreenInfo->modes)
            return FALSE;
        *pScreenInfo->modes = renditionDefaultMode;
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

    /* Only one chipset here */
    if (!pScreenInfo->chipset)
        pScreenInfo->chipset = (char *)renditionChipsets[0].name;

    return TRUE;        /* Tada! */
}


/* Save mode on server entry */
static void
renditionSave(ScrnInfoPtr pScreenInfo)
{
    vgaHWSave(pScreenInfo, &VGAHWPTR(pScreenInfo)->SavedReg, VGA_SR_ALL);
}


/* Restore the mode that was saved on server entry */
static void
renditionRestore(ScrnInfoPtr pScreenInfo)
{
    vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);

    vgaHWProtect(pScreenInfo, TRUE);
    vgaHWRestore(pScreenInfo, &pvgaHW->SavedReg, VGA_SR_ALL);
    vgaHWProtect(pScreenInfo, FALSE);

    v_setmode(&RENDITIONPTR(pScreenInfo)->board, 
        &RENDITIONPTR(pScreenInfo)->mode);
}


/* Set a graphics mode */
static Bool
renditionSetMode(ScrnInfoPtr pScreenInfo, DisplayModePtr pMode)
{
    vgaHWPtr pvgaHW=VGAHWPTR(pScreenInfo);
    struct v_modeinfo_t *modeinfo=&RENDITIONPTR(pScreenInfo)->mode;

#ifdef DEBUG
    ErrorF("RENDITION: renditionSetMode() called\n");
#endif

    /* construct a modeinfo for the v_setmode function */
    modeinfo->clock=pMode->SynthClock;
    modeinfo->hdisplay=pMode->HDisplay;
    modeinfo->hsyncstart=pMode->HSyncStart;
    modeinfo->hsyncend=pMode->HSyncEnd;
    modeinfo->htotal=pMode->HTotal;
    modeinfo->hskew=pMode->HSkew;
    modeinfo->vdisplay=pMode->VDisplay;
    modeinfo->vsyncstart=pMode->VSyncStart;
    modeinfo->vsyncend=pMode->VSyncEnd;
    modeinfo->vtotal=pMode->VTotal;

    modeinfo->screenwidth=pMode->HDisplay;
    modeinfo->virtualwidth=pScreenInfo->virtualX&0xfff8;
    modeinfo->screenheight=pMode->VDisplay;
    modeinfo->virtualheight=pScreenInfo->virtualY&0xfff8;

    if ((pMode->Flags&(V_PHSYNC|V_NHSYNC)) 
        && (pMode->Flags&(V_PVSYNC|V_NVSYNC))) {
        modeinfo->hsynchi=((pMode->Flags&V_PHSYNC) == V_PHSYNC);
        modeinfo->vsynchi=((pMode->Flags&V_PVSYNC) == V_PVSYNC);
    }
    else {
        int VDisplay=pMode->VDisplay;
        if (pMode->Flags & V_DBLSCAN)
            VDisplay*=2;
        if (VDisplay < 400) {
            /* +hsync -vsync */
            modeinfo->hsynchi=1;
            modeinfo->vsynchi=0;
        }
        else if (VDisplay < 480) {
            /* -hsync +vsync */
            modeinfo->hsynchi=0;
            modeinfo->vsynchi=1;
        }
        else if (VDisplay < 768) {
            /* -hsync -vsync */
            modeinfo->hsynchi=0;
            modeinfo->vsynchi=0;
        }
        else {
            /* +hsync +vsync */
            modeinfo->hsynchi=1;
            modeinfo->vsynchi=1;
        }
    }

    switch (pScreenInfo->bitsPerPixel) {
        case 8:
            modeinfo->bitsperpixel=8;
            modeinfo->pixelformat=V_PIXFMT_8I;
            break;
        case 16:
            modeinfo->bitsperpixel=16;
#if 0
            if (vga256InfoRec.weight.green == 5)
                /* on a V1000, this looks too 'red/magenta' <ml> */
                modeinfo->pixelformat=V_PIXFMT_1555;
            else
#endif
                modeinfo->pixelformat=V_PIXFMT_565;
            break;
        case 32:
            modeinfo->bitsperpixel=32;
            modeinfo->pixelformat=V_PIXFMT_8888;
            break;
    }
    modeinfo->fifosize=128;

    v_setmode(&RENDITIONPTR(pScreenInfo)->board, 
        &RENDITIONPTR(pScreenInfo)->mode);

    return TRUE;
}


static Bool
renditionEnterGraphics(ScreenPtr pScreen, ScrnInfoPtr pScreenInfo)
{
    vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);

#ifdef DEBUG
    ErrorF("RENDITION: renditionEnterGraphics() called\n");
#endif

    /* Map VGA aperture */
    if (!vgaHWMapMem(pScreenInfo))
        return FALSE;

    /* Unlock VGA registers */
    vgaHWUnlock(pvgaHW);

    /* Save the current state and setup the current mode */
    renditionSave(pScreenInfo);
    if (!renditionSetMode(pScreenInfo, pScreenInfo->currentMode))
        return FALSE;

    /* Possibly blank the screen */
    if (pScreen)
        renditionSaveScreen(pScreen, FALSE);

    (*pScreenInfo->AdjustFrame)(pScreenInfo->scrnIndex,
        pScreenInfo->frameX0, pScreenInfo->frameY0, 0);

    return TRUE;
}


static void
renditionLeaveGraphics(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionLeaveGraphics() called\n");
#endif

    renditionRestore(pScreenInfo);
    vgaHWLock(VGAHWPTR(pScreenInfo));
    vgaHWUnmapMem(pScreenInfo);

    v_textmode(&RENDITIONPTR(pScreenInfo)->board);
}


/* Unravel the screen */
static Bool
renditionCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
    renditionPtr prenditionPriv=renditionGetRec(pScreenInfo);
    Bool Closed = TRUE;

#ifdef DEBUG
    ErrorF("RENDITION: renditionCloseScreen() called\n");
#endif

    if (prenditionPriv && (pScreen->CloseScreen = prenditionPriv->CloseScreen))
    {
        prenditionPriv->CloseScreen = NULL;
        Closed = (*pScreen->CloseScreen)(scrnIndex, pScreen);
    }

    renditionLeaveGraphics(pScreenInfo);
    pScreenInfo->vtSema = FALSE;

    return Closed;
}


#ifdef DPMSExtension
static void
renditionDPMSSet(ScrnInfoPtr pScreen, int mode, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionDPMSSet() called\n");
#endif

    vgaHWDPMSSet(pScreen, mode, flags);
}
#endif


static Bool
renditionScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];

    renditionPtr prenditionPriv;
    Bool Inited = FALSE;
    unsigned char *FBBase=RENDITIONPTR(pScreenInfo)->board.vmem_base;
    VisualPtr visual;

#ifdef DEBUG
    ErrorF("RENDITION: renditionScreenInit() called\n");
#endif

    xf86EnableAccess(&pScreenInfo->Access);
    
    /* Get driver private */
    prenditionPriv=renditionGetRec(pScreenInfo);
    if (NULL == prenditionPriv) /* xcalloc failed */
      return FALSE;

    /* Initialise graphics mode */
    if (!renditionEnterGraphics(pScreen, pScreenInfo))
        return FALSE;

    /* Get vgahw private
    pvgaHW = VGAHWPTR(pScreenInfo);
    */

    miClearVisualTypes();

    if (!miSetVisualTypes(pScreenInfo->depth,
              miGetDefaultVisualMask(pScreenInfo->depth),
              pScreenInfo->rgbBits, pScreenInfo->defaultVisual))
    return FALSE;

    /* initialise the framebuffer */
    switch (pScreenInfo->depth)
    {
        case 8:
            Inited = cfbScreenInit(pScreen, FBBase,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
            break;
        case 16:
            Inited = cfb16ScreenInit(pScreen, FBBase,
        pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
        break;
        case 32:
            Inited = cfb32ScreenInit(pScreen, FBBase,
                pScreenInfo->virtualX, pScreenInfo->virtualY,
                pScreenInfo->xDpi, pScreenInfo->yDpi,
                pScreenInfo->displayWidth);
        break;
    default:
        xf86DrvMsg(scrnIndex, X_ERROR,
                   "Internal error: invalid bpp (%d) in renditionScreenInit\n",
                   pScreenInfo->bitsPerPixel);
        break;
    }

    if (!Inited)
        return FALSE;

    miInitializeBackingStore(pScreen);

    if (pScreenInfo->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual=pScreen->visuals+pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	        if (0 && (visual->class | DynamicClass) == DirectColor) {
		        visual->offsetRed = pScreenInfo->offset.red;
		        visual->offsetGreen = pScreenInfo->offset.green;
		        visual->offsetBlue = pScreenInfo->offset.blue;
		        visual->redMask = pScreenInfo->mask.red;
		        visual->greenMask = pScreenInfo->mask.green;
		        visual->blueMask = pScreenInfo->mask.blue;
	        }
            else {
                ErrorF("Changing masks!!!\n");
		        visual->offsetRed=11;
		        visual->offsetGreen=5;
		        visual->offsetBlue=0;
		        visual->redMask=0xf800;
		        visual->greenMask=0x7e0;
		        visual->blueMask=0x1f;
/*
		        visual->offsetRed=0;
		        visual->offsetGreen=5;
		        visual->offsetBlue=11;
		        visual->redMask=0x1f;
		        visual->greenMask=0x7e0;
		        visual->blueMask=0xf800;
*/
            }
	    }
    }

    xf86SetBlackWhitePixels(pScreen);

    /* Initialise cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Setup default colourmap */
    Inited = miCreateDefColormap(pScreen);

    /* Try the new code based on the new colormap layer */
    if (pScreenInfo->depth > 1)
        vgaHWHandleColormaps(pScreen);

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, renditionDPMSSet, 0);
#endif

    /* Wrap the screen's CloseScreen vector and set its SaveScreen vector */
    prenditionPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = renditionCloseScreen;
    pScreen->SaveScreen = renditionSaveScreen;

    if (!Inited)
        renditionCloseScreen(scrnIndex, pScreen);

    if (!xf86LoadSubModule(pScreenInfo, "rac")){
        renditionFreeRec(pScreenInfo);
        return FALSE;
    }
    xf86LoaderReqSymLists(racSymbols, NULL);

    /* xf86RACInit(pScreen, RAC_COLORMAP | RAC_FB); FAILS!!*/

    if (serverGeneration == 1)
      xf86ShowUnusedOptions(pScreenInfo->scrnIndex, pScreenInfo->options);

    return Inited;
}
/*

    ErrorF ("XX Pee: **\n");
    LoaderCheckUnresolved(LD_RESOLV_IFDONE);
    return FALSE;
*/

static Bool
renditionSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionSwitchMode() called\n");
#endif
    return renditionSetMode(xf86Screens[scrnIndex], pMode);
}


static void
renditionAdjustFrame(int scrnIndex, int x, int y, int flags)
{
        ScrnInfoPtr pScreenInfo=xf86Screens[scrnIndex];
        int offset, virtualwidth, bitsPerPixel;

#ifdef DEBUG
    ErrorF("RENDITION: renditionAdjustFrame() called\n");
#endif

    bitsPerPixel=pScreenInfo->bitsPerPixel;
    virtualwidth=RENDITIONPTR(pScreenInfo)->mode.virtualwidth;
    offset=(y*virtualwidth+x)*(bitsPerPixel>>3);

    v_setframebase(&RENDITIONPTR(pScreenInfo)->board, offset+FBOFFSET);
}


static Bool
renditionEnterVT(int scrnIndex, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionEnterVT() called\n");
#endif
     return renditionEnterGraphics(NULL, xf86Screens[scrnIndex]);
}


static void
renditionLeaveVT(int scrnIndex, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionLeaveVT() called\n");
#endif
    renditionLeaveGraphics(xf86Screens[scrnIndex]);
}


static void
renditionFreeScreen(int scrnIndex, int flags)
{
    renditionFreeRec(xf86Screens[scrnIndex]);
}


static ModeStatus
renditionValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
    if (pMode->Flags & V_INTERLACE)
        return MODE_NO_INTERLACE;

    return MODE_OK;
}

static Bool renditionMapMem(ScrnInfoPtr pScreenInfo)
{
  Bool WriteCombine;
  int mapOption;

#if DEBUG0
  ErrorF("Mapping ...\n");
  ErrorF("%d %d %d %x %d\n", pScreenInfo->scrnIndex, VIDMEM_FRAMEBUFFER, 
     RENDITIONPTR(pScreenInfo)->pcitag,
     RENDITIONPTR(pScreenInfo)->board.mem_base, pScreenInfo->videoRam);
#endif

  if (RENDITIONPTR(pScreenInfo)->board.chip==V1000_DEVICE){
    /* Some V1000 boards are known to have problems with Write-Combining */
    WriteCombine = 0;
  }
  else{
    /* Activate Write_Combine if possible */
    WriteCombine = 1;
  }
  /* Override on users request */
  WriteCombine=xf86ReturnOptValBool(renditionOptions, OPTION_FBWC, WriteCombine);
  if (WriteCombine)
    mapOption = VIDMEM_FRAMEBUFFER;
  else 
    mapOption = VIDMEM_MMIO;

  xf86MarkOptionUsedByName(renditionOptions,"FramebufferWC");

    RENDITIONPTR(pScreenInfo)->board.vmem_base=
        xf86MapPciMem(pScreenInfo->scrnIndex, mapOption,
        RENDITIONPTR(pScreenInfo)->pcitag,
        (unsigned long)RENDITIONPTR(pScreenInfo)->board.mem_base,
	pScreenInfo->videoRam);
    return TRUE;

#if DEBUG0
    ErrorF("Done\n");
#endif
}

static Bool renditionUnmapMem(ScrnInfoPtr pScreenInfo)
{
#if DEBUG0
  ErrorF("Unmapping ...\n");
#endif
    xf86UnMapVidMem(pScreenInfo->scrnIndex,
        RENDITIONPTR(pScreenInfo)->board.mem_base, pScreenInfo->videoRam);
    return TRUE;
#if DEBUG0
    ErrorF("Done\n");
#endif
}
