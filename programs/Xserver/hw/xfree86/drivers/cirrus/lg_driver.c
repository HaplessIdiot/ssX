/*
 * Driver for CL-GD546x -- The Laguna family
 *
 * lg_driver.c
 *
 * (c) 1998 Corin Anderson.
 *          corina@the4cs.com
 *          Tukwila, WA
 *
 * This driver is derived from the cir_driver.c module.
 * Original authors and contributors list include:
 *	Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *	David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *	Guy DESBIEF, Itai Nahshon.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/lg_driver.c,v 1.3 1998/11/22 10:37:20 dawes Exp $ */
 
/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers need this */
#include "xf86_ansic.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers using the vgahw module need this */
/* This driver needs to be modified to not use vgaHW for multihead operation */
#include "vgaHW.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

#include "micmap.h"

/*
 * If using cfb, cfb.h is required.  Select the others for the bpp values
 * the driver supports.
 */
#include "xf4bpp.h"
#include "xf1bpp.h"
#define PSZ 8	/* needed for cfb.h */
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

/*
#define LG_DEBUG
*/

#include "lg.h"

/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */
Bool	LgPreInit(ScrnInfoPtr pScrn, int flags);
Bool	LgScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
Bool	LgEnterVT(int scrnIndex, int flags);
void	LgLeaveVT(int scrnIndex, int flags);
static Bool	LgCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	LgSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
Bool	LgSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
void	LgAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
void	LgFreeScreen(int scrnIndex, int flags);
int	LgValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);

/* Internally used functions */
static void LgRestoreLgRegs(ScrnInfoPtr pScrn, LgRegPtr lgReg);
static int LgFindLineData(int displayWidth, int bpp);

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_NOACCEL
} LgOpts;

static OptionInfoRec LgOptions[] = {
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_TRI,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    /* fifo_conservative/aggressive; fast/med/slow_dram; ... */
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};


/*                                1/4bpp   8bpp   15/16bpp  24bpp  32bpp */
static int gd5462_MaxClocks[] = { 170000, 170000, 135100, 135100,  85500 };
static int gd5464_MaxClocks[] = { 170000, 250000, 170000, 170000, 135100 };
static int gd5465_MaxClocks[] = { 170000, 250000, 170000, 170000, 135100 };

LgLineDataRec LgLineData[] = {
  {5, 640, 0},       /* We're rather use skinny tiles, so put all of */
  {8, 1024, 0},      /* them at the head of the table */
  {10, 1280, 0},
  {13, 1664, 0},
  {16, 2048, 0},
  {20, 2560, 0},
  {10, 2560, 1},
  {26, 3328, 0},
  {5, 1280, 1},
  {8, 2048, 1},
  {13, 3328, 1},
  {16, 4096, 1},
  {20, 5120, 1},
  {26, 6656, 1},
  {-1, -1, -1}     /* Sentinal to indicate end of table */
};
  
static int LgLinePitches[4][11] = {
  /*  8 */ { 640, 1024, 1280, 1664, 2048, 2560, 3328, 4096, 5120, 6656, 0 },
  /* 16 */ { 320,  512,  640,  832, 1024, 1280, 1664, 2048, 2560, 3328, 0 },
  /* 24 */ { 213,  341,  426,  554,  682,  853, 1109, 1365, 1706, 2218, 0 },
  /* 32 */ { 160,  256,  320,  416,  512,  640,  832, 1024, 1280, 1664, 0 } };
	      


static Bool
LgGetRec(ScrnInfoPtr pScrn)
{
    LgPtr pLg;
    /*
     * Allocate a LgRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(LgRec), 1);

    /* Initialize it */
    pLg = LGPTR(pScrn);
    pLg->oldBitmask = 0x00000000;

    return TRUE;
}

static void
LgFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}



/*
 * LgCountRAM --
 *
 * Counts amount of installed RAM 
 */

/* XXX We need to get rid of this PIO (MArk) */
static int
LgCountRam(ScrnInfoPtr pScrn)
{
  CARD8 SR14;

  /* The ROM BIOS scratchpad registers contain, 
     among other things, the amount of installed
     RDRAM on the laguna chip. */
  outb(0x3C4, 0x14);
  SR14 = inb(0x3C5);

  return 1024 * ((SR14&0x7) + 1);

  /* !!! This function seems to be incorrect... */
}


/* Mandatory */
 Bool
LgPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    LgPtr pLg;
    MessageType from;
    int i;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    int fbPCIReg, ioPCIReg;

#ifdef LG_DEBUG
    ErrorF("LgPreInit\n");
#endif

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


#if 1 /* XXX */
    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;
#endif

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     * We support both 24bpp and 32bpp layouts, so indicate that.
     */
    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, Support24bppFb | Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
	    /* OK */
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    return FALSE;
	}
    }
    xf86PrintDepthBpp(pScrn);

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	/* !!! I think we can force 5-6-5 weight for 16bpp here for
	   the 5462. */

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

    /* The gamma fields must be initialised when using the new cmap code */
    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the LgRec driverPrivate */
    if (!LgGetRec(pScrn)) {
	return FALSE;
    }
    pLg = LGPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, LgOptions);

    pScrn->rgbBits = 6; /* ??? What's this? */
    from = X_DEFAULT;
    pLg->HWCursor = FALSE;
    if (xf86GetOptValBool(LgOptions, OPTION_HW_CURSOR, &pLg->HWCursor)) {
	from = X_CONFIG;
    }
    if (xf86IsOptionSet(LgOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pLg->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pLg->HWCursor ? "HW" : "SW");
    if (xf86IsOptionSet(LgOptions, OPTION_NOACCEL)) {
	pLg->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if(pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Cannot use accelerations in less than 8 bpp\n");
	pLg->NoAccel = TRUE;
    }

    /* Find the PCI slot for this screen */
    /*
     * XXX Ignoring the Type list for now.  It might be needed when
     * multiple cards are supported.
     */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) != 1) {
	/* This shouldn't happen */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Expected one PCI card, but found %d\n", i);
	LgFreeRec(pScrn);
	if (i > 0)
	    xfree(pciList);
	return FALSE;
    }

    pLg->PciInfo = *pciList;
    xfree(pciList);
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pLg->Chipset = xf86StringToToken(CIRChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pLg->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(CIRChipsets, pLg->Chipset);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pLg->Chipset);
    } else {
	from = X_PROBED;
	pLg->Chipset = pLg->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(CIRChipsets, pLg->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pLg->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pLg->ChipRev);
    } else {
	pLg->ChipRev = pLg->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * CIRProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pLg->Chipset);
	return FALSE;
    }
    if (pLg->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);
	
    pLg->PciTag = pciTag(pLg->PciInfo->bus, pLg->PciInfo->device,
			 pLg->PciInfo->func);
    
    /* Cirrus swapped the FB and IO registers in the 5465 (by design). */
    if (PCI_CHIP_GD5465 == pLg->Chipset) {
      fbPCIReg = 0;
      ioPCIReg = 1;
    } else {
      fbPCIReg = 1;
      ioPCIReg = 0;
    }

    /* Find the frame buffer base address */
    if (pScrn->device->MemBase != 0) {
	pLg->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	if (pLg->PciInfo->memBase[fbPCIReg] != 0) {
	    pLg->FbAddress = pLg->PciInfo->memBase[fbPCIReg] & 0xff000000;
	    from = X_PROBED;
	} else {
	   xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                      "No valid FB address in PCI config space\n");
	   LgFreeRec(pScrn);
	   return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pLg->FbAddress);

    /* Find the MMIO base address */
    if (pScrn->device->IOBase != 0) {
	pLg->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	if (pLg->PciInfo->memBase[ioPCIReg] != 0) {
	    pLg->IOAddress = pLg->PciInfo->memBase[ioPCIReg] & 0xfffff000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "No valid MMIO address in PCI config space\n");
	}
    }
    if(pLg->IOAddress != 0) {
        xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
		   (unsigned long)pLg->IOAddress);
    }

    /*
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
        vgaHWProtect(pScrn, TRUE);
	pScrn->videoRam = LgCountRam(pScrn);
        vgaHWProtect(pScrn, FALSE);
	from = X_PROBED;
    }
    if (2048 == pScrn->videoRam) {
      /* Two-way interleaving */
      pLg->memInterleave = 0x40;
    } else if (4096 == pScrn->videoRam || 8192 == pScrn->videoRam) {
      /* Four-way interleaving */
      pLg->memInterleave = 0x80;
    } else {
      /* One-way interleaving */
      pLg->memInterleave = 0x00;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
	
    pLg->FbMapSize = pScrn->videoRam * 1024;
	
    /* XXX Set HW cursor use */

    /* Set the min pixel clock */
    pLg->MinClock = 12000;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pLg->MinClock / 1000);
    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->dacSpeeds[0]) {
        ErrorF("Do not specify a Clocks line for Cirrus chips\n");
        return FALSE;
    } else {
	int speed;
        int *p;
        switch (pLg->Chipset) {
	case PCI_CHIP_GD5462:
	   p = gd5462_MaxClocks;
	   break;
	case PCI_CHIP_GD5464:
	case PCI_CHIP_GD5464BD:
	   p = gd5464_MaxClocks;
	   break;
	case PCI_CHIP_GD5465:
	   p = gd5465_MaxClocks;
	   break;
	  default:
	    ErrorF("???\n");
	    return FALSE;
	}
	switch(pScrn->bitsPerPixel) {
        case 1:
	case 4:
	    speed = p[0];
	    break;
	case 8:
	    speed = p[1];
	    break;
	case 15:
	case 16:
	    speed = p[2];
	    break;
	case 24:
	    speed = p[3];
	    break;
	case 32:
	    speed = p[4];
	    break;
	default:
            /* Should not get here */
	    speed = 0;
	    break;
	}
	pLg->MaxClock = speed;
	from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pLg->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pLg->MinClock;
    clockRanges->maxClock = pLg->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
    clockRanges->ClockMulFactor = 1;
    clockRanges->ClockDivFactor = 1;
    clockRanges->PrivFlags = 0;

    /* Depending upon what sized tiles used, either 128 or 256. */
    /* Aw, heck.  Just say 128. */
    pLg->Rounding = 128 >> pLg->BppShift;

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our CIRValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			  pScrn->display->modes, clockRanges,
			  LgLinePitches[pScrn->bitsPerPixel / 8 - 1],
			  0, 0, 128 * 8,
			  0, 0, /* Any virtual height is allowed. */
			  pScrn->display->virtualX,
			  pScrn->display->virtualY,
			  pLg->FbMapSize,
			  LOOKUP_BEST_REFRESH);
    
    pLg->lineDataIndex = LgFindLineData(pScrn->displayWidth, 
					pScrn->bitsPerPixel);

    if (i == -1) {
	LgFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	LgFreeRec(pScrn);
	return FALSE;
    }

    /*
     * Set the CRTC parameters for all of the modes based on the type
     * of mode, and the chipset's interlace requirements.
     *
     * Calling this is required if the mode->Crtc* values are used by the
     * driver and if the driver doesn't provide code to set them.  They
     * are not pre-initialised at all.
     */
    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 1:  mod = "xf1bpp";  break;
    case 4:  mod = "xf4bpp";  break;
    case 8:  mod = "cfb";     break;
    case 16: mod = "cfb16";   break;
    case 24: mod = "cfb24";   break;
    case 32: mod = "cfb32";   break;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	LgFreeRec(pScrn);
	return FALSE;
    }

    /* Load XAA if needed */
    if (!pLg->NoAccel)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    LgFreeRec(pScrn);
	    return FALSE;
	}

    /* Load ramdac if needed */
    if (pLg->HWCursor)
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    LgFreeRec(pScrn);
	    return FALSE;
	}

    return TRUE;
}

/*
 * This function saves the video state.
 */
static void
LgSave(ScrnInfoPtr pScrn)
{
    LgPtr pLg;
    vgaHWPtr hwp;
    CARD8 *p8;
    CARD16 *p16;
    CARD32 *p32;

#ifdef LG_DEBUG
    ErrorF("LgSave\n");
#endif

    hwp = VGAHWPTR(pScrn);
    pLg = LGPTR(pScrn);

    vgaHWSave(pScrn, &VGAHWPTR(pScrn)->SavedReg, VGA_SR_ALL);

    outb(hwp->IOBase+4, 0x1A); pLg->ModeReg.ExtVga[CR1A] = pLg->SavedReg.ExtVga[CR1A] = inb(hwp->IOBase + 5);
    outb(hwp->IOBase+4, 0x1B); pLg->ModeReg.ExtVga[CR1B] = pLg->SavedReg.ExtVga[CR1B] = inb(hwp->IOBase + 5);
    outb(hwp->IOBase+4, 0x1D); pLg->ModeReg.ExtVga[CR1D] = pLg->SavedReg.ExtVga[CR1D] = inb(hwp->IOBase + 5);
    outb(hwp->IOBase+4, 0x1E); pLg->ModeReg.ExtVga[CR1E] = pLg->SavedReg.ExtVga[CR1E] = inb(hwp->IOBase + 5);
    outb(0x3C4, 0x07);   pLg->ModeReg.ExtVga[SR07] = pLg->SavedReg.ExtVga[SR07] = inb(0x3C5);
    outb(0x3C4, 0x0E);   pLg->ModeReg.ExtVga[SR0E] = pLg->SavedReg.ExtVga[SR0E] = inb(0x3C5);
    outb(0x3C4, 0x12);   pLg->ModeReg.ExtVga[SR12] = pLg->SavedReg.ExtVga[SR12] = inb(0x3C5);
    outb(0x3C4, 0x13);   pLg->ModeReg.ExtVga[SR13] = pLg->SavedReg.ExtVga[SR13] = inb(0x3C5);
    outb(0x3C4, 0x1E);   pLg->ModeReg.ExtVga[SR1E] = pLg->SavedReg.ExtVga[SR1E] = inb(0x3C5);

    p16 = (CARD16 *)(pLg->IOBase + 0xC0);
    pLg->ModeReg.FORMAT = pLg->SavedReg.FORMAT = *p16;

    p32 = (CARD32 *)(pLg->IOBase + 0x3FC);
    pLg->ModeReg.VSC = pLg->SavedReg.VSC = *p32;

    p16 = (CARD16 *)(pLg->IOBase + 0xEA);
    pLg->ModeReg.DTTC = pLg->SavedReg.DTTC = *p16;

    if (pLg->Chipset == PCI_CHIP_GD5465) {
      p16 = (CARD16 *)(pLg->IOBase + 0x2C4);
      pLg->ModeReg.TileCtrl = pLg->SavedReg.TileCtrl = *p16;
    }

    p8 = (CARD8 *)(pLg->IOBase + 0x407);
    pLg->ModeReg.TILE = pLg->SavedReg.TILE = *p8;
      
    if (pLg->Chipset == PCI_CHIP_GD5465)
      p8 = (CARD8 *)(pLg->IOBase + 0x2C0);
    else
      p8 = (CARD8 *)(pLg->IOBase + 0x8C);
    pLg->ModeReg.BCLK = pLg->SavedReg.BCLK = *p8;
 
    p16 = (CARD16 *)(pLg->IOBase + 0x402);
    pLg->ModeReg.CONTROL = pLg->SavedReg.CONTROL = *p16;
}

/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
LgModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    LgPtr pLg;
    int width;
    Bool VDiv2 = FALSE;
    CARD16 clockData;
    LgLineDataPtr lineData;

#ifdef LG_DEBUG
    ErrorF("LgModeInit %d bpp,   %d   %d %d %d %d   %d %d %d %d\n",
        pScrn->bitsPerPixel,
        mode->Clock,
        mode->HDisplay,
        mode->HSyncStart,
        mode->HSyncEnd,
        mode->HTotal,
        mode->VDisplay,
        mode->VSyncStart,
        mode->VSyncEnd,
        mode->VTotal);

    ErrorF("LgModeInit: depth %d bits\n", pScrn->depth);
#endif

    pLg = LGPTR(pScrn);
    hwp = VGAHWPTR(pScrn);
    vgaHWUnlock(hwp);

    if (mode->VTotal >= 1024 && !(mode->Flags & V_INTERLACE)) {
        /* For non-interlaced vertical timing >= 1024, the vertical timings */
        /* are divided by 2 and VGA CRTC 0x17 bit 2 is set. */
        if (!mode->CrtcVAdjusted) {
            mode->CrtcVDisplay >>= 1;
            mode->CrtcVSyncStart >>= 1;
            mode->CrtcVSyncEnd >>= 1;
            mode->CrtcVTotal >>= 1;
            mode->CrtcVAdjusted = TRUE;
        }
       VDiv2 = TRUE;
    }

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);

    if(VDiv2)
        hwp->ModeReg.CRTC[0x17] |= 0x04;

#ifdef LG_DEBUG
    ErrorF("SynthClock = %d\n", mode->SynthClock);
#endif
    clockData = CirrusSetClock(pScrn, mode->SynthClock);
    pLg->ModeReg.ExtVga[SR0E] = (clockData >> 8) & 0xFF;
    pLg->ModeReg.ExtVga[SR1E] = clockData & 0xFF;

    vgaReg = &hwp->ModeReg;

    if(pScrn->bitsPerPixel == 1) {
       hwp->IOBase = 0x3B0;
       hwp->ModeReg.MiscOutReg &= ~0x01;
    }
    else {
       hwp->IOBase = 0x3D0;
       hwp->ModeReg.MiscOutReg |= 0x01;
    }
    
    outb(0x3C2, hwp->ModeReg.MiscOutReg);

    /* ??? Should these be both ...End or ...Start, not one of each? */
    pLg->ModeReg.ExtVga[CR1A] = (((mode->CrtcVSyncStart + 1) & 0x300 ) >> 2)
      | (((mode->CrtcHSyncEnd >> 3) & 0xC0) >> 2);

    width = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
    if(pScrn->bitsPerPixel == 1)
       width <<= 2;
    hwp->ModeReg.CRTC[0x13] = (width + 7) >> 3;
    /* Offset extension (see CR13) */
    pLg->ModeReg.ExtVga[CR1B] &= 0xEF;
    pLg->ModeReg.ExtVga[CR1B] |= (((width + 7) >> 3) & 0x100)?0x10:0x00;
    pLg->ModeReg.ExtVga[CR1B] |= 0x22;
    pLg->ModeReg.ExtVga[CR1D] = (((width + 7) >> 3) & 0x200)?0x01:0x00;

    /* Set the 28th bit to enable extended modes. */
    pLg->ModeReg.VSC = 0x10000000;

    /* Overflow register (sure are a lot of overflow bits around...) */
    pLg->ModeReg.ExtVga[CR1E] = 0x00;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcHTotal>>3 & 0x0100)?1:0)<<7;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcHDisplay>>3 & 0x0100)?1:0)<<6;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcHSyncStart>>3 & 0x0100)?1:0)<<5;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcHSyncStart>>3 & 0x0100)?1:0)<<4;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcVTotal & 0x0400)?1:0)<<3;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcVDisplay & 0x0400)?1:0)<<2;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcVSyncStart & 0x0400)?1:0)<<1;
    pLg->ModeReg.ExtVga[CR1E] |= ((mode->CrtcVSyncStart & 0x0400)?1:0)<<0;

    lineData = &LgLineData[pLg->lineDataIndex];

    pLg->ModeReg.TILE = lineData->tilesPerLine & 0x3F;

    if (8 == pScrn->bitsPerPixel) {
      pLg->ModeReg.FORMAT = 0x0000;
      
      pLg->ModeReg.DTTC = (pLg->ModeReg.TILE << 8) | 0x0080 | 
	(lineData->width << 6);
      pLg->ModeReg.CONTROL = 0x0000 | (lineData->width << 11);


      /* There is an optimal FIFO threshold value (lower 5 bits of DTTC)
	 for every resolution and color depth combination.  We'll hit
	 the highlights here, and get close for anything that's not 
	 covered. */
      if (mode->CrtcHDisplay <= 640) {
	/* BAD numbers:  0x1E */
	/* GOOD numbers:  0x14 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0014);
      } else if (mode->CrtcHDisplay <= 800) {
	/* BAD numbers:  0x16 */
	/* GOOD numbers:  0x13 0x14 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0014);
      } else if (mode->CrtcHDisplay <= 1024) {
	/* BAD numbers:  */
	/* GOOD numbers: 0x15 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0015);
      } else if (mode->CrtcHDisplay <= 1280) {
	/* BAD numbers:  */
	/* GOOD numbers:  0x16 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0016);
      } else {
	/* BAD numbers:  */
	/* GOOD numbers:  */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0017);
      }

    } else if (16 == pScrn->bitsPerPixel) {

      /* !!! Assume 5-6-5 RGB mode (for now...) */
      pLg->ModeReg.FORMAT = 0x1400;
      
      pLg->ModeReg.DTTC = (pLg->ModeReg.TILE << 8) | 0x0080 | 
	(lineData->width << 6);
      pLg->ModeReg.CONTROL = 0x2000 | (lineData->width << 11);
      
      if (mode->CrtcHDisplay <= 640) {
	/* BAD numbers:  0x12 */
	/* GOOD numbers: 0x10 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0010);
      } else if (mode->CrtcHDisplay <= 800) {
	/* BAD numbers:  0x13 */
	/* GOOD numbers:  0x11 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0011);
      } else if (mode->CrtcHDisplay <= 1024) {
	/* BAD numbers:  0x14 */
	/* GOOD numbers: 0x12  */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0012);
      } else if (mode->CrtcHDisplay <= 1280) {
	/* BAD numbers:   0x08 0x10 */
	/* Borderline numbers: 0x12 */
	/* GOOD numbers:  0x15 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0015);
      } else {
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0017);
      }
      
    } else if (24 == pScrn->bitsPerPixel) {
      
      pLg->ModeReg.FORMAT = 0x2400;
      
      pLg->ModeReg.DTTC = (pLg->ModeReg.TILE << 8) | 0x0080 | 
	(lineData->width << 6);
      pLg->ModeReg.CONTROL = 0x4000 | (lineData->width << 11);
      
      
      if (mode->CrtcHDisplay <= 640) {
	/* BAD numbers:   */
	/* GOOD numbers:  0x10 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0010);
      } else if (mode->CrtcHDisplay <= 800) {
	/* BAD numbers:   */
	/* GOOD numbers:   0x11 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0011);
      } else if (mode->CrtcHDisplay <= 1024) {
	/* BAD numbers:  0x12 0x13 */
	/* Borderline numbers:  0x15 */
	/* GOOD numbers:  0x17 */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0017);
      } else if (mode->CrtcHDisplay <= 1280) {
	/* BAD numbers:   */
	/* GOOD numbers:  0x1E */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x001E);
      } else {
	/* BAD numbers:   */
	/* GOOD numbers:  */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0020);
      }
      
    } else if (32 == pScrn->bitsPerPixel) {
      
      pLg->ModeReg.FORMAT = 0x3400;
      
      pLg->ModeReg.DTTC = (pLg->ModeReg.TILE << 8) | 0x0080 | 
	(lineData->width << 6);
      pLg->ModeReg.CONTROL = 0x6000 | (lineData->width << 11);
      
      
      if (mode->CrtcHDisplay <= 640) {
	/* GOOD numbers:  0x0E */
	/* BAD numbers:  */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x000E);
      } else if (mode->CrtcHDisplay <= 800) {
	/* GOOD numbers:  0x17 */
	/* BAD numbers:  */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0017);
      } else if (mode->CrtcHDisplay <= 1024) {
	/* GOOD numbers: 0x1D */
	/* OKAY numbers:  0x15 0x14 0x16 0x18 0x19 */
	/* BAD numbers:  0x0E 0x12 0x13 0x0D */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x001D);
      } else if (mode->CrtcHDisplay <= 1280) {
	/* GOOD numbers:  */
	/* BAD numbers:  */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0022); /* 10 */
      } else {
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xFFE0) | (0x0024);
      }
    } else {
      /* ??? What could it be?  Use some sane numbers. */
    }

    /* Setup the appropriate memory interleaving */
    pLg->ModeReg.DTTC |= (pLg->memInterleave << 8);
    pLg->ModeReg.TILE |= pLg->memInterleave & 0xC0;

    if (PCI_CHIP_GD5465 == pLg->Chipset) {
      /* The tile control information in the DTTC is also mirrored
	 elsewhere. */
      pLg->ModeReg.TileCtrl = pLg->ModeReg.DTTC & 0xFFC0;

      /* The 5465's DTTC records _fetches_ per line, not
	 tiles per line.  Fetchs are 128-byte fetches. */
      if (pLg->ModeReg.DTTC & 0x0040) {
	/* Using 256-byte wide tiles.  Double the fetches
	   per line field. */
	pLg->ModeReg.DTTC = (pLg->ModeReg.DTTC & 0xC0FF) | 
	  ((pLg->ModeReg.DTTC & 0x3F00) << 1);
      }
    }

    /* Write those registers out to the card. */
    LgRestoreLgRegs(pScrn, &pLg->ModeReg);
    
    /* Programme the registers */
    vgaHWRestore(pScrn, &hwp->ModeReg, VGA_SR_MODE | VGA_SR_CMAP);

    vgaHWProtect(pScrn, FALSE);

    return TRUE;
}

static int LgFindLineData(int displayWidth, int bpp) {
  /* Find the smallest tile-line-pitch such that the total byte pitch
     is greater than or equal to displayWidth*Bpp. */
  int i;

  /* Some pitch sizes are duplicates in the table.  BUT, the invariant is 
     that the _first_ time a pitch occurs in the table is always _before_
     all other pitches greater than it.  Said in another way... if all 
     duplicate entries from the table were removed, then the resulting pitch
     values are strictly increasing. */

  for (i = 0; LgLineData[i].pitch > 0; i++)
    if (LgLineData[i].pitch >= displayWidth*bpp>>3) {
      return i;
    }

  /* Um, uh oh! */
  return -1;
}




static void
LgRestoreLgRegs(ScrnInfoPtr pScrn, LgRegPtr lgReg) {
  CARD8 *p8;
  CARD16 *p16;
  CARD32 *p32;
  LgPtr pLg;
  vgaHWPtr hwp;
  CARD8 cr1D;
  
  pLg = LGPTR(pScrn);

  /* First, VGAish registers. */
  hwp = VGAHWPTR(pScrn);
  outw(hwp->IOBase + 4, (lgReg->ExtVga[CR1A] << 8) | 0x1A);
  outw(hwp->IOBase + 4, (lgReg->ExtVga[CR1B] << 8) | 0x1B);
  outb(hwp->IOBase+4, 0x1D); 
  cr1D = inb(hwp->IOBase + 5);
  cr1D &= ~(0x01);
  cr1D |= (lgReg->ExtVga[CR1D] & 0x01);
  outw(hwp->IOBase + 4, (cr1D << 8) | 0x1D);
  outw(hwp->IOBase + 4, (lgReg->ExtVga[CR1E] << 8) | 0x1E);
  outw(0x3C4, (lgReg->ExtVga[SR07] << 8) | 0x07);
  outw(0x3C4, (lgReg->ExtVga[SR0E] << 8) | 0x0E);
  outw(0x3C4, (lgReg->ExtVga[SR12] << 8) | 0x12);
  outw(0x3C4, (lgReg->ExtVga[SR13] << 8) | 0x13);
  outw(0x3C4, (lgReg->ExtVga[SR1E] << 8) | 0x1E);

  p16 = (CARD16 *)(pLg->IOBase + 0xC0);
  *p16 = lgReg->FORMAT;

  /* Vendor Specific Control is touchy.  Only bit 28 is of concern. */
  p32 = (CARD32 *)(pLg->IOBase + 0x3FC);
  *p32 = (*p32 & ~(1<<28)) | (lgReg->VSC & (1<<28));

  p16 = (CARD16 *)(pLg->IOBase + 0xEA);
  *p16 = lgReg->DTTC;

  if (pLg->Chipset == PCI_CHIP_GD5465) {
    p16 = (CARD16 *)(pLg->IOBase + 0x2C4);
    *p16 = lgReg->TileCtrl;
  }

  p8 = (CARD8 *)(pLg->IOBase + 0x407);
  *p8 = lgReg->TILE;
      
  if (pLg->Chipset == PCI_CHIP_GD5465)
    p8 = (CARD8 *)(pLg->IOBase + 0x2C0);
  else
    p8 = (CARD8 *)(pLg->IOBase + 0x8C);
  *p8 = lgReg->BCLK;
  
  p16 = (CARD16 *)(pLg->IOBase + 0x402);
  *p16 = lgReg->CONTROL;
}

/*
 * Restore the initial (text) mode.
 */
static void 
LgRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    LgPtr pLg;
    LgRegPtr lgReg;

#ifdef LG_DEBUG
    ErrorF("LgRestore  pScrn = 0x%08X\n", pScrn);
#endif

    pLg = LGPTR(pScrn);
    hwp = VGAHWPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    lgReg = &pLg->SavedReg;

    vgaHWProtect(pScrn, TRUE);

    LgRestoreLgRegs(pScrn, lgReg);

    vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
    vgaHWProtect(pScrn, FALSE);
}

/* Mandatory */

/* This gets called at the start of each server generation */

Bool
LgScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    /* The vgaHW references will disappear one day */
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    LgPtr pLg;
    int i, ret;
    VisualPtr visual;

#ifdef LG_DEBUG
    ErrorF("LgScreenInit\n");
#endif

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);

    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    pLg = LGPTR(pScrn);

    /* Map the VGA memory and get the VGA IO base */
    if (!vgaHWMapMem(pScrn))
	return FALSE;

    vgaHWGetIOBase(hwp);

    /* Map the CIR memory and MMIO areas */
    if (!CIRMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    LgSave(pScrn);

    /* Initialise the first mode */
    if (!LgModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    /* Set the viewport */
    LgAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
     * Reset the visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.  To deal with this, call
     * miSetVisualTypes with the appropriate visual mask.
     */
#ifdef LG_DEBUG
    ErrorF("LgScreenInit before miSetVisualTypes\n");
#endif
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
#ifdef LG_DEBUG
    ErrorF("LgScreenInit after miSetVisualTypes\n");
#endif

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    switch (pScrn->bitsPerPixel) {
    case 1:
        ret = xf1bppScreenInit(pScreen, pLg->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
        break;
    case 4:
        ret = xf4bppScreenInit(pScreen, pLg->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
        break;
    case 8:
	ret = cfbScreenInit(pScreen, pLg->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pLg->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pLg->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pLg->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "X11: Internal error: invalid bpp (%d) in LgScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

#ifdef LG_DEBUG
    ErrorF("LgScreenInit after depth dependent init\n");
#endif

    /* Override the default mask/offset settings */
    if (pScrn->bitsPerPixel > 8) {
        for (i = 0; i < pScreen->numVisuals; i++) {
            visual = &pScreen->visuals[i];
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

    miInitializeBackingStore(pScreen);

    /*
     * Set initial black & white colourmap indices.
     */
    xf86SetBlackWhitePixels(pScreen);

    /* Initialise cursor functions */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    if(!pLg->NoAccel) { /* Initialize XAA functions */
       if(!LgXAAInit(pScreen))
          xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
              "Could not initialize XAA\n");
    }

    if (pLg->HWCursor) { /* Initialize HW cursor layer */
        if(!LgHWCursorInit(pScreen))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "Hardware cursor initialization failed\n");
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
        return FALSE;

    if (pScrn->bitsPerPixel > 1 &&
            pScrn->bitsPerPixel <= 8)
        vgaHWHandleColormaps(pScreen);

    /*
     * Wrap the CloseScreen vector and set SaveScreen.
     */
    pScreen->SaveScreen = LgSaveScreen;
    pLg->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = LgCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}


/* Usually mandatory */
Bool
LgSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return LgModeInit(xf86Screens[scrnIndex], mode);
}

#define ROUND_DOWN(x, mod) (((x) / (mod)) * (mod))
#define ROUND_UP(x, mod)   ((((x) + (mod) - 1) / (mod)) * (mod))

/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
void 
LgAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    int Base, tmp;
    LgPtr pLg = LGPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int cursorX, cursorY;
    int middleX, middleY;
    const LgLineDataPtr lineData = &LgLineData[pLg->lineDataIndex];
    const int viewportXRes = 
      (PCI_CHIP_GD5465 == pLg->Chipset) ? (24==pScrn->bitsPerPixel?24:1) :
      (lineData->width?256:128) / 
      (24==pScrn->bitsPerPixel?1:(pScrn->bitsPerPixel>>3));
    const int viewportYRes = 
      (PCI_CHIP_GD5465 == pLg->Chipset) ? 1 : (24==pScrn->bitsPerPixel?3:1);

    /* Where's the pointer? */
    miPointerPosition(&cursorX, &cursorY);

    /* Where's the middle of the screen?  We want to eventually know
       which side of the screen the pointer is on. */
    middleX = (pScrn->frameX1 + pScrn->frameX0) / 2;
    middleY = (pScrn->frameY1 + pScrn->frameY0) / 2;
    
    if (cursorX < middleX) {
      /* Pointer is on left side of screen.  Round the frame value
	 down. */
      pScrn->frameX0 = ROUND_DOWN(pScrn->frameX0, viewportXRes);
    } else {
      /* Pointer is on right side of screen.  Round the frame value
	 up.  A side effect of this rounding up is that we might expose
	 a part of the screen that's actually on the far /left/ of the
	 frame buffer.  That's because, although the virtual desktop might
	 be an integral number of tiles, the display might not.  We'll
	 just live with this artifact. */
      pScrn->frameX0 = ROUND_UP(pScrn->frameX0, viewportXRes);
    }
    pScrn->frameX1 = pScrn->frameX0 + pScrn->currentMode->HDisplay - 1;

    if (cursorY < middleY) {
      pScrn->frameY0 = ROUND_DOWN(pScrn->frameY0, viewportYRes);
    } else {      
      pScrn->frameY0 = ROUND_UP(pScrn->frameY0, viewportYRes);
    }
    pScrn->frameY1 = pScrn->frameY0 + pScrn->currentMode->VDisplay - 1;


    if (x != pScrn->frameX0 || y != pScrn->frameY0) {
      /* !!! */
      /* We moved the frame from where xf86SetViewport() placed it.
	 If we're using a SW cursor, that's okay -- the pointer exists in 
	 the framebuffer, and those bits are still all aligned.  But
	 if we're using a HW cursor, then we need to re-align the pointer.
	 Call SetCursorPosition() with the appropriate new pointer
	 values, adjusted to be wrt the new frame. */

      x = pScrn->frameX0;
      y = pScrn->frameY0;
    }

    /* ??? Will this work for 1bpp?  */
    Base = (y * lineData->pitch + (x*pScrn->bitsPerPixel/8)) / 4;

    if ((Base & ~0x000FFFFF) != 0) {
      /* ??? */
        ErrorF("X11: Internal error: LgAdjustFrame: cannot handle overflow\n");
        return;
    }

    outw(hwp->IOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(hwp->IOBase + 4, ((Base & 0x0000FF) << 8) | 0x0D);
    outb(hwp->IOBase + 4, 0x1B);
    tmp = inb(hwp->IOBase + 5);
    tmp &= 0xF2;
    tmp |= (Base >> 16) & 0x01;
    tmp |= (Base >> 15) & 0x0C;
    outb(hwp->IOBase + 5, tmp);
    outb(hwp->IOBase + 4, 0x1D);
    tmp = inb(hwp->IOBase + 5);
    tmp &= 0xE7;
    tmp |= (Base >> 16) & 0x18;
    outb(hwp->IOBase + 5, tmp);
}

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
Bool
LgEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    LgPtr pLg = LGPTR(pScrn);
#ifdef LG_DEBUG
    ErrorF("LgEnterVT\n");
#endif

    /* Disable HW cursor */
    if (pLg->HWCursor)
      LgHideCursor(pScrn);

    /* Should we re-save the text mode on each VT enter? */
    return LgModeInit(pScrn, pScrn->currentMode);
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
void
LgLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    LgPtr pLg = LGPTR(pScrn);
#ifdef LG_DEBUG
    ErrorF("LgLeaveVT\n");
#endif

    /* Enable HW cursor */
    if (pLg->HWCursor)
      LgShowCursor(pScrn);

    LgRestore(pScrn);
    vgaHWLock(hwp);
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should also unmap the video memory, and free
 * any per-generation data allocated by the driver.  It should finish
 * by unwrapping and calling the saved CloseScreen function.
 */

/* Mandatory */
static Bool
LgCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    LgPtr pLg = LGPTR(pScrn);

    LgRestore(pScrn);

    if (pLg->HWCursor)
      LgHideCursor(pScrn);

    vgaHWLock(hwp);

    CIRUnmapMem(pScrn);

    if (pLg->AccelInfoRec)
	XAADestroyInfoRec(pLg->AccelInfoRec);
    pLg->AccelInfoRec = NULL;

    if (pLg->CursorInfoRec)
    	xf86DestroyCursorInfoRec(pLg->CursorInfoRec);
    pLg->CursorInfoRec = NULL;

    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = pLg->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any persistent data structures */

/* Optional */
void
LgFreeScreen(int scrnIndex, int flags)
{
#ifdef LG_DEBUG
    ErrorF("LgFreeScreen\n");
#endif
    /*
     * This only gets called when a screen is being deleted.  It does not
     * get called routinely at the end of a server generation.
     */
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    LgFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
int
LgValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    int lace;

    lace = 1 + ((mode->Flags & V_INTERLACE) != 0);

    if ((mode->CrtcHDisplay <= 2048) &&
	(mode->CrtcHSyncStart <= 4096) && 
	(mode->CrtcHSyncEnd <= 4096) && 
	(mode->CrtcHTotal <= 4096) &&
	(mode->CrtcVDisplay <= 2048 * lace) &&
	(mode->CrtcVSyncStart <= 4096 * lace) &&
	(mode->CrtcVSyncEnd <= 4096 * lace) &&
	(mode->CrtcVTotal <= 4096 * lace)) {
	return(MODE_OK);
    } else {
	return(MODE_BAD);
    }
}


/* Do screen blanking */

/* Mandatory */
static Bool
LgSaveScreen(ScreenPtr pScreen, Bool unblank)
{
  /* !!! More work here! */

    return vgaHWSaveScreen(pScreen, unblank);
}
