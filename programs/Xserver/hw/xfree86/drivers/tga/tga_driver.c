/*
 * Copyright 1997,1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *           Matthew Grossman, <mattg@oz.net> - acceleration and misc fixes
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tga/tga_driver.c,v 1.15 1999/02/07 11:11:15 dawes Exp $ */

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "micmap.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86cmap.h"
#include "mipointer.h"

#include "mibstore.h"

#include "tga_regs.h"
#include "BT.h"
#include "tga.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "globals.h"
#include "extensions/dpms.h"
#endif

static void	TGAIdentify(int flags);
static Bool	TGAProbe(DriverPtr drv, int flags);
static Bool	TGAPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	TGAScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	TGAEnterVT(int scrnIndex, int flags);
static void	TGALeaveVT(int scrnIndex, int flags);
static Bool	TGACloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	TGASaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool	TGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	TGAAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	TGAFreeScreen(int scrnIndex, int flags);
static int	TGAValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);

/* Internally used functions */
static Bool	TGAMapMem(ScrnInfoPtr pScrn);
static Bool	TGAUnmapMem(ScrnInfoPtr pScrn);
static void	TGASave(ScrnInfoPtr pScrn);
static void	TGARestore(ScrnInfoPtr pScrn);
static Bool	TGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

#define VERSION 4000
#define TGA_NAME "TGA"
#define TGA_DRIVER_NAME "tga"
#define TGA_MAJOR_VERSION 1
#define TGA_MINOR_VERSION 0
#define TGA_PATCHLEVEL 0

/* 
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the SetupProc
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

DriverRec TGA = {
    VERSION,
    "accelerated driver for Digital chipsets",
    TGAIdentify,
    TGAProbe,
    NULL,
    0
};

static SymTabRec TGAChipsets[] = {
    { PCI_CHIP_DEC21030,		"tga" },
    { -1,				NULL }
};

static PciChipsets TGAPciChipsets[] = {
    { PCI_CHIP_DEC21030,	PCI_CHIP_DEC21030,	RES_NONE },
    { -1,			-1,			RES_UNDEFINED }
};

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_NOACCEL
} TGAOpts;

static OptionInfoRec TGAOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

static RamDacSupportedInfoRec BTramdacs[] = {
    { BT485_RAMDAC },
    { -1 }
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(tgaSetup);

static XF86ModuleVersionInfo tgaVersRec =
{
	"tga",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	TGA_MAJOR_VERSION, TGA_MINOR_VERSION, TGA_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

XF86ModuleData tgaModuleData = { &tgaVersRec, tgaSetup, NULL };

pointer
tgaSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&TGA, module, 0);

	/*
	 * Modules that this driver always requires can be loaded here
	 * by calling LoadSubModule().
	 */

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

#endif /* XFree86LOADER */

static unsigned int fb_offset_presets[4] = {
	TGA_8PLANE_FB_OFFSET,
	TGA_24PLANE_FB_OFFSET,
	0xffffffff,
	TGA_24PLUSZ_FB_OFFSET
};

static char *tga_cardnames[4] = {
	"TGA 8 Plane",
	"TGA 24 Plane",
	NULL,
	"TGA 24 Plane 3D"
};

static Bool
TGAGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an TGARec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(TGARec), 1);
    /* Initialise it */


    return TRUE;
}

static void
TGAFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}


/* Mandatory */
static void
TGAIdentify(int flags)
{
    xf86PrintChipsets(TGA_NAME, "driver for Digital chipsets", TGAChipsets);
}


/* Mandatory */
static Bool
TGAProbe(DriverPtr drv, int flags)
{
    int i;
    pciVideoPtr pPci, *usedPci;
    GDevPtr *devSections;
    GDevPtr *usedDevs;
    int *usedChips;
    int numDevSections;
    int numUsed;
    Bool foundScreen = FALSE;

    /*
     * The aim here is to find all cards that this driver can handle,
     * and for the ones not already claimed by another driver, claim the
     * slot, and allocate a ScrnInfoRec.
     *
     * This should be a minimal probe, and it should under no circumstances
     * change the state of the hardware.  Because a device is found, don't
     * assume that it will be used.  Don't do any initialisations other than
     * the required ScrnInfoRec initialisations.  Don't allocate any new
     * data structures.
     */

    /*
     * Next we check, if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(TGA_DRIVER_NAME,
					  &devSections)) <= 0) {
	/*
	 * There's no matching device section in the config file, so quit
	 * now.
	 */
	return FALSE;
    }

    /*
     * We need to probe the hardware first.  We then need to see how this
     * fits in with what is given in the config file, and allow the config
     * file info to override any contradictions.
     */

    /*
     * All of the cards this driver supports are PCI, so the "probing" just
     * amounts to checking the PCI data that the server has already collected.
     */
    if (xf86GetPciVideoInfo() == NULL) {
	/*
	 * We won't let anything in the config file override finding no
	 * PCI video cards at all.  This seems reasonable now, but we'll see.
	 */
	return FALSE;
    }

    numUsed = xf86MatchPciInstances(TGA_NAME, PCI_VENDOR_DIGITAL,
		   TGAChipsets, TGAPciChipsets, devSections, numDevSections,
		   &usedDevs, &usedPci, &usedChips);
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];

	/*
	 * Check that nothing else has claimed the slots.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func, RES_NONE)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func,
			     RES_NONE, &TGA, usedChips[i], pScrn->scrnIndex);

	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = TGA_DRIVER_NAME;
	    pScrn->name		 = TGA_NAME;
	    pScrn->Probe	 = TGAProbe;
	    pScrn->PreInit	 = TGAPreInit;
	    pScrn->ScreenInit	 = TGAScreenInit;
	    pScrn->SwitchMode	 = TGASwitchMode;
	    pScrn->AdjustFrame	 = TGAAdjustFrame;
	    pScrn->EnterVT	 = TGAEnterVT;
	    pScrn->LeaveVT	 = TGALeaveVT;
	    pScrn->FreeScreen	 = TGAFreeScreen;
	    pScrn->ValidMode	 = TGAValidMode;
	    pScrn->device	 = usedDevs[i];
	    foundScreen = TRUE;
	}
    }
    xfree(usedDevs);
    xfree(usedPci);
    return foundScreen;
}
	
/*
 * GetAccelPitchValues -
 *
 * This function returns a list of display width (pitch) values that can
 * be used in accelerated mode.
 */
static int *
GetAccelPitchValues(ScrnInfoPtr pScrn)
{
    int *linePitches = NULL;
    int i, n = 0;
    int *linep = NULL;
    /*     TGAPtr pTga = TGAPTR(pScrn); */
	
    for (i = 0; linep[i] != 0; i++) {
	if (linep[i] != -1) {
	    n++;
	    linePitches = xnfrealloc(linePitches, n * sizeof(int));
	    linePitches[n - 1] = i << 5;
	}
    }

    /* Mark the end of the list */
    if (n > 0) {
	linePitches = xnfrealloc(linePitches, (n + 1) * sizeof(int));
	linePitches[n] = 0;
    }
    return linePitches;
}

/* Mandatory */
static Bool
TGAPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    TGAPtr pTga;
    MessageType from;
    int i;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    pointer Base;

    /*
     * Note: This function is only called once at server startup, and
     * not at the start of each server generation.  This means that
     * only things that are persistent across server generations can
     * be initialised here.  xf86Screens[] is (pScrn is a pointer to one
     * of these).  Privates allocated using xf86AllocateScrnInfoPrivateIndex()  
     * are too, and should be used for data that must persist across
     * server generations.
     *
     * Per-generation data should be allocated with
     * AllocateScreenPrivateIndex() from the ScreenInit() function.
     */

    /* The ramdac module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "ramdac"))
	return FALSE;

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     */
    if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 24:
	    /* OK */
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    return FALSE;
	}
    }

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros)) {
	    return FALSE;
	} else {
	    /* XXX check that weight returned is supported */
            ;
        }
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
	return FALSE;
    } else {
	/* We don't currently support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
		       " (%s) is not supported at depth %d\n",
		       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    return FALSE;
	}
    }

    /*
     * The new cmap code requires this to be initialised.
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the TGARec driverPrivate */
    if (!TGAGetRec(pScrn)) {
	return FALSE;
    }
    pTga = TGAPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, TGAOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* XXX This is here just to test options. */
	/* Default to 8 */
	pScrn->rgbBits = 8;
	if (xf86GetOptValInteger(TGAOptions, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
    }
    from = X_DEFAULT;
    pTga->HWCursor = FALSE;
    if (xf86IsOptionSet(TGAOptions, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	pTga->HWCursor = TRUE;
    }
    if (xf86IsOptionSet(TGAOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pTga->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pTga->HWCursor ? "HW" : "SW");
    if (xf86IsOptionSet(TGAOptions, OPTION_NOACCEL)) {
	pTga->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86IsOptionSet(TGAOptions, OPTION_PCI_RETRY)) {
	pTga->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }
    if(pScrn->depth > 8) {
      pTga->NoAccel = TRUE;
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "No acceleration for >8bpp cards yet\n");
    }

    /* Find the PCI slot for this screen */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) != 1) {
	/* This shouldn't happen */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Expected one PCI card, but found %d\n", i);
	TGAFreeRec(pScrn);
	return FALSE;
    }

    pTga->PciInfo = *pciList;
    pTga->RamDac = NULL;
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pTga->Chipset = xf86StringToToken(TGAChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pTga->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(TGAChipsets, pTga->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pTga->Chipset);
    } else {
	from = X_PROBED;
	pTga->Chipset = pTga->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(TGAChipsets, pTga->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pTga->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pTga->ChipRev);
    } else {
	pTga->ChipRev = pTga->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * TGAProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pTga->Chipset);
	return FALSE;
    }
    if (pTga->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    pTga->PciTag = pciTag(pTga->PciInfo->bus, pTga->PciInfo->device,
			  pTga->PciInfo->func);
    
    if (pScrn->device->MemBase != 0) {
	pTga->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
      pTga->FbAddress = pTga->PciInfo->memBase[0] & 0xFF800000;
    }

    /* Adjust MMIO region */
    pTga->IOAddress = pTga->FbAddress + TGA_REGS_OFFSET;

    /* Check what sort of TGA card we have */

    /* check what the user has specified in XF86Config */
    if(pScrn->device->videoRam) {
      switch(pScrn->device->videoRam) {
      case 2048:
	pTga->CardType = TYPE_TGA_8PLANE;
	break;
      case 8192:
	pTga->CardType = TYPE_TGA_24PLANE;
	break;
      case 16384:
	pTga->CardType = TYPE_TGA_24PLUSZ;
	break;
      default:
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "%d KB video RAM specified, driver only supports 2048, 8192, or 16384 KB cards\n",
		   pScrn->device->videoRam);
	return FALSE;
      }
    }
    else { /* try to divine the amount of RAM */
      Base = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
			   pTga->PciTag, (pointer)pTga->FbAddress, 4);
      pTga->CardType = (*(unsigned int *)Base >> 12) & 0xf;
      xf86UnMapVidMem(pScrn->scrnIndex, Base, 4);
    }

    switch (pTga->CardType) {
        case TYPE_TGA_8PLANE:
        case TYPE_TGA_24PLANE:
        case TYPE_TGA_24PLUSZ:
            xf86DrvMsg(pScrn->scrnIndex, from, "Card Name: \"%s\"\n", 
			tga_cardnames[pTga->CardType]);
	    break;
	default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                 "Card \"0x%02x\" is not recognised\n", pTga->CardType);
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Assuming 8 plane TGA with 2MB frame buffer\n");
	    pTga->CardType = TYPE_TGA_8PLANE;
	    break;
    }
    
    /* Adjust framebuffer for card type */
    pTga->FbAddress += fb_offset_presets[pTga->CardType];

    if (!(((pScrn->depth ==  8) && (pTga->CardType == TYPE_TGA_8PLANE)) ||
	  ((pScrn->depth == 24) && (pTga->CardType == TYPE_TGA_24PLANE)) ||
	  ((pScrn->depth == 24) && (pTga->CardType == TYPE_TGA_24PLUSZ)))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Given depth (%d) is not supported by this card\n",
		   pScrn->depth);
	return FALSE;
    }


    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pTga->FbAddress);

    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pTga->IOAddress);

    /* HW bpp matches reported bpp */
    pTga->HwBpp = pScrn->bitsPerPixel;

    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
#if 0
	pTga->FbMapSize = 0; /* Need to set FbMapSize for MMIO access */
	/* Need to access MMIO to determine videoRam */
        TGAMapMem(pScrn);
#endif
	switch (pTga->CardType) {
        case TYPE_TGA_8PLANE:
                pScrn->videoRam = 2*1024;
                break;
        case TYPE_TGA_24PLANE:
                pScrn->videoRam = 8*1024;
                break;
        case TYPE_TGA_24PLUSZ:
                pScrn->videoRam = 16*1024;
                break;
	}	  
#if 0
        TGAUnmapMem(pScrn);
#endif
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    pTga->FbMapSize = pScrn->videoRam * 1024;

    /* Let's check what type of DAC we have and reject if necessary */
    switch (pTga->Chipset)
    {
	case PCI_CHIP_DEC21030:
	    if (pTga->CardType != TYPE_TGA_8PLANE) {
	        pTga->RamDacRec = NULL;
	        pTga->RamDac = NULL;
	        break;
	    }
	    pTga->RamDacRec = RamDacCreateInfoRec();
	    pTga->RamDacRec->ReadDAC = tgaBTInIndReg;
	    pTga->RamDacRec->WriteDAC = tgaBTOutIndReg;
	    pTga->RamDacRec->ReadAddress = tgaBTReadAddress;
	    pTga->RamDacRec->WriteAddress = tgaBTWriteAddress;
	    pTga->RamDacRec->ReadData = tgaBTReadData;
	    pTga->RamDacRec->WriteData = tgaBTWriteData;
	    if(!RamDacInit(pScrn, pTga->RamDacRec)) {
		RamDacDestroyInfoRec(pTga->RamDacRec);
		return FALSE;
	    }

            TGAMapMem(pScrn);

	    
	    pTga->RamDac = BTramdacProbe(pScrn, BTramdacs);

	    TGAUnmapMem(pScrn);

	    if (pTga->RamDac == NULL)
		return FALSE;
	    break;
    }

    /* XXX Set HW cursor use */

    /* Set the min pixel clock */
    pTga->MinClock = 16250;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pTga->MinClock / 1000);

    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->dacSpeeds[0]) {
	int speed = 0;

	switch (pScrn->bitsPerPixel) {
	case 8:
	   speed = pScrn->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = pScrn->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = pScrn->device->dacSpeeds[DAC_BPP24];
	   break;
	case 32:
	   speed = pScrn->device->dacSpeeds[DAC_BPP32];
	   break;
	}
	if (speed == 0)
	    pTga->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pTga->MaxClock = speed;
	from = X_CONFIG;
    } else {
	if (pTga->Chipset == PCI_CHIP_DEC21030)
		pTga->MaxClock = 135000;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pTga->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pTga->MinClock;
    clockRanges->maxClock = pTga->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our TGAValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
#if 0
    if (pTga->NoAccel) {
#endif
	/*
	 * XXX Assuming min pitch 256, max 2048
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 2048,
			      pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pTga->FbMapSize,
			      LOOKUP_BEST_REFRESH);
#if 0
    } else {
	/*
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      GetAccelPitchValues(pScrn), 0, 0,
			      pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pTga->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }
#endif

    if (i == -1) {
	TGAFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	TGAFreeRec(pScrn);
	return FALSE;
    }

    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 8:
	mod = "cfb";
	break;
    case 16:
	mod = "cfb16";
	break;
    case 24:
	mod = "cfb24";
	break;
    case 32:
	mod = "cfb32";
	break;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	TGAFreeRec(pScrn);
	return FALSE;
    }


    /* Load XAA if needed */
    if (!pTga->NoAccel || pTga->HWCursor)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    TGAFreeRec(pScrn);
	    return FALSE;
	}


    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
TGAMapMem(ScrnInfoPtr pScrn)
{
    CARD32 save;
    TGAPtr pTga;

    pTga = TGAPTR(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

    save = pciReadLong(pTga->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pTga->PciTag, PCI_CMD_STAT_REG,
		 save & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    /*
     * Map IO registers to virtual address space
     */ 
#if 0
#if !defined(__alpha__)
    pTga->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
		pTga->PciTag, (pointer)pTga->IOAddress, 0x10000);
#else
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.
     */
    pTga->IOBase = xf86MapPciMemSparse(pScrn->scrnIndex, VIDMEM_MMIO,
		pTga->PciTag, (pointer)pTga->IOAddress, 0x10000);
#endif
    if (pTga->IOBase == NULL)
	return FALSE;
#endif

#if defined(SVR4)
    /*
     * For some SVR4 versions, a 32-bit read is done for the first
     * location in each page when the page is first mapped.  If this
     * is done while memory and I/O are enabled, the result will be
     * a lockup, so make sure each page is mapped here while it is safe
     * to do so.
     */
    {
	CARD32 val;

	val = *(volatile CARD32 *)(pTga->IOBase+0);
	val = *(volatile CARD32 *)(pTga->IOBase+0x1000);
	val = *(volatile CARD32 *)(pTga->IOBase+0x2000);
	val = *(volatile CARD32 *)(pTga->IOBase+0x3000);
    }
#endif

#ifdef __alpha__
    /*
     * for Alpha, we need to map DENSE memory as well, for
     * setting CPUToScreenColorExpandBase.
     */
    pTga->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
				      pTga->PciTag,
				      (pointer)pTga->IOAddress, 0x100000);
    if (pTga->IOBaseDense == NULL)
	return FALSE;

#endif /* __alpha__ */

    pTga->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pTga->PciTag,
				 (pointer)((unsigned long)pTga->FbAddress),
				 pTga->FbMapSize);
    if (pTga->FbBase == NULL)
	return FALSE;

    /* Re-enable I/O and memory */
    pciWriteLong(pTga->PciTag, PCI_CMD_STAT_REG,
		 save | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
TGAUnmapMem(ScrnInfoPtr pScrn)
{
    TGAPtr pTga;

    pTga = TGAPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
#if 0
#ifndef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTga->IOBase, 0x10000);
#else
    xf86UnMapVidMemSparse(pScrn->scrnIndex, (pointer)pTga->IOBase, 0x10000);
#endif
    pTga->IOBase = NULL;
#endif

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTga->IOBaseDense, 0x100000);
    pTga->IOBaseDense = NULL;

#endif /* __alpha__ */

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTga->FbBase, pScrn->videoRam);
    pTga->FbBase = NULL;

    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
TGASave(ScrnInfoPtr pScrn)
{
    TGAPtr pTga;
    TGARegPtr tgaReg;
    RamDacHWRecPtr pBT;
    RamDacRegRecPtr BTreg;

    pTga = TGAPTR(pScrn);
    tgaReg = &pTga->SavedReg;

    switch (pTga->Chipset)
    {
    case PCI_CHIP_DEC21030:
	DEC21030Save(pScrn, tgaReg);
	if (pTga->RamDac) {
	    pBT = RAMDACHWPTR(pScrn);
	    BTreg = &pBT->SavedReg;
	    (*pTga->RamDac->Save)(pScrn, pTga->RamDacRec, BTreg);
	} else {
	    BT463ramdacSave(pScrn, pTga->Bt463saveReg);
	}
	break;
    }
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
TGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int ret = -1;
    TGAPtr pTga;
    TGARegPtr tgaReg;
    RamDacHWRecPtr pBT;
    RamDacRegRecPtr BTreg;

    pTga = TGAPTR(pScrn);

    pScrn->vtSema = TRUE;

    switch (pTga->Chipset) {
    case PCI_CHIP_DEC21030:
	ret = DEC21030Init(pScrn, mode);
	break;
    }

    if (!ret)
	return FALSE;

    /* Program the registers */
    tgaReg = &pTga->ModeReg;

    switch (pTga->Chipset) {
    case PCI_CHIP_DEC21030:
	DEC21030Restore(pScrn, tgaReg);
	if (pTga->RamDac != NULL) {
	    pBT = RAMDACHWPTR(pScrn);
	    BTreg = &pBT->ModeReg;
	    (*pTga->RamDac->Restore)(pScrn, pTga->RamDacRec, BTreg);
	} else {
	    BT463ramdacRestore(pScrn, pTga->Bt463modeReg);
	}
	break;
    }

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
TGARestore(ScrnInfoPtr pScrn)
{
    TGAPtr pTga;
    TGARegPtr tgaReg;
    RamDacHWRecPtr pBT;
    RamDacRegRecPtr BTreg;

    pTga = TGAPTR(pScrn);
    tgaReg = &pTga->SavedReg;

    /* Initial Text mode clock */
    tgaReg->tgaRegs[0x0A] = 25175;

    switch (pTga->Chipset) {
    case PCI_CHIP_DEC21030:
	DEC21030Restore(pScrn, tgaReg);
	if (pTga->RamDac != NULL) {
	    pBT = RAMDACHWPTR(pScrn);
	    BTreg = &pBT->SavedReg;
	    (*pTga->RamDac->Restore)(pScrn, pTga->RamDacRec, BTreg);
	} else {
	    BT463ramdacRestore(pScrn, pTga->Bt463saveReg);
	}
	break;
    }
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
TGAScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    TGAPtr pTga;
    int ret;
    VisualPtr visual;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    pTga = TGAPTR(pScrn);

    /* Map the TGA memory and MMIO areas */
    if (!TGAMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    TGASave(pScrn);

    /* Initialise the first mode */
    TGAModeInit(pScrn, pScrn->currentMode);

    /* Darken the screen for aesthetic reasons and set the viewport */
    TGASaveScreen(pScreen, FALSE);
    TGAAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    /* XXX Fill the screen with black */
#if 0
    TGASaveScreen(pScreen, TRUE);
#endif

    /*
     * The next step is to setup the screen's visuals, and initialise the
     * framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before calling
     * a fb ScreenInit() function and fixed up after.
     *
     * For most PC hardware at depths >= 8, the defaults that cfb uses
     * are not appropriate.  In this driver, we fixup the visuals after.
     */

    /*
     * Reset visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.  To deal with this, call
     * miSetVisualTypes for each visual supported.
     */

    if (pScrn->bitsPerPixel > 8) {
	if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits,
				pScrn->defaultVisual))
	    return FALSE;
    } else {
	if (!miSetVisualTypes(pScrn->depth,
			      miGetDefaultVisualMask(pScrn->depth),
			      pScrn->rgbBits, pScrn->defaultVisual))
	    return FALSE;
    }

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    switch (pScrn->bitsPerPixel) {
    case 8:
	ret = cfbScreenInit(pScreen, pTga->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pTga->FbBase, pScrn->virtualX,
			      pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
			      pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in TGAScrnInit\n",
		   pScrn->bitsPerPixel);
	    ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

    miInitializeBackingStore(pScreen);

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }

    if (!pTga->NoAccel) {
        switch (pTga->Chipset)
        {
	case PCI_CHIP_DEC21030:

	    if(DEC21030AccelInit(pScreen) == FALSE)
	      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			 "XAA Initialization failed\n");
	    break;
        }
    }

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());
#if 0
    /* AFAIK, TGA HW cursor doesn't support the features of the X
       hardware cursor */
    /* Initialize HW cursor layer. 
	Must follow software cursor initialization*/
    if (pTga->HWCursor) { 
	if(!TGAHWCursorInit(pScreen))
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
		"Hardware cursor initialization failed\n");
    }
#endif

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    if ((pScrn->bitsPerPixel==8) && 
        (!RamDacHandleColormaps(pScreen, 256, pScrn->rgbBits,
	 CMAP_RELOAD_ON_MODE_SWITCH | CMAP_PALETTED_TRUECOLOR)))
	return FALSE;

    pTga->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = TGACloseScreen;
    pScreen->SaveScreen = TGASaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}


/* Usually mandatory */
static Bool
TGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return TGAModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
TGAAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    pScrn = xf86Screens[scrnIndex];
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
TGAEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    /* Should we re-save the text mode on each VT enter? */
    if (!TGAModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    return TRUE;
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
static void
TGALeaveVT(int scrnIndex, int flags)
{
  TGAPtr pTga;
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    pTga = TGAPTR(pScrn);
    TGARestore(pScrn);

    /* clear the screen...there's probably a better way to do this */
    memset(pTga->FbBase, 0, pTga->FbMapSize);
    return;
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
TGACloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    TGAPtr pTga = TGAPTR(pScrn);

    TGARestore(pScrn);
    xf86memset(pTga->FbBase, 0, pScrn->videoRam * 1024);
    TGAUnmapMem(pScrn);
    if(pTga->AccelInfoRec)
	XAADestroyInfoRec(pTga->AccelInfoRec);
    pScrn->vtSema = FALSE;
    
    pScreen->CloseScreen = pTga->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any per-generation data structures */

/* Optional */
static void
TGAFreeScreen(int scrnIndex, int flags)
{
    RamDacFreeRec(xf86Screens[scrnIndex]);
    TGAFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
TGAValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    if (mode->Flags & V_INTERLACE)
	return(MODE_BAD);

    return(MODE_OK);
}

/* Do screen blanking */

/* Mandatory */
static Bool
TGASaveScreen(ScreenPtr pScreen, Bool unblank)
{
	return TRUE;
}
