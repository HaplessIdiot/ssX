/*
 * Copyright 1992-1998 by Alan Hourihane, Wigan, England.
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
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *	    Re-written for XFree86 v4.0
 *
 * Previous driver (pre-XFree86 v4.0) by
 * 	    Alan Hourihane, alanh@fairlite.demon.co.uk
 *	    David Wexelblat (major contributor)
 *	    Massimiliano Ghilardi, max@Linuz.sns.it, some fixes to the
 *				   clockchip programming code.
 */
/* $XFree86$ */

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
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "mipointer.h"

#include "mibstore.h"

#include "trident_regs.h"
#include "trident.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "opaque.h"
#include "extensions/dpms.h"
#endif

static void	TRIDENTIdentify(int flags);
static Bool	TRIDENTProbe(DriverPtr drv, int flags);
static Bool	TRIDENTPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	TRIDENTScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	TRIDENTEnterVT(int scrnIndex, int flags);
static void	TRIDENTLeaveVT(int scrnIndex, int flags);
static Bool	TRIDENTCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	TRIDENTSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool	TRIDENTSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	TRIDENTAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	TRIDENTFreeScreen(int scrnIndex, int flags);
static int	TRIDENTValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);

/* Internally used functions */
static Bool	TRIDENTMapMem(ScrnInfoPtr pScrn);
static Bool	TRIDENTUnmapMem(ScrnInfoPtr pScrn);
static void	TRIDENTSave(ScrnInfoPtr pScrn);
static void	TRIDENTRestore(ScrnInfoPtr pScrn);
static Bool	TRIDENTModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

#define VERSION 4000
#define TRIDENT_NAME "TRIDENT"
#define TRIDENT_DRIVER_NAME "trident"
/*
 * The major version is the 0xffff0000 part and the minor version is the
 * 0x0000ffff part.
 */
#define TRIDENT_DRIVER_VERSION 0x00010001

/* 
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the ModuleInit
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

DriverRec TRIDENT = {
    VERSION,
    "accelerated driver for Trident chipsets",
    TRIDENTIdentify,
    TRIDENTProbe,
    NULL,
    0
};

static SymTabRec TRIDENTChipsets[] = {
    { PCI_CHIP_9320,		"cyber9320" },
    { PCI_CHIP_9420,		"tgui9420" },
    { PCI_CHIP_9440,		"tgui9440" },
    { PCI_CHIP_9660,		"tgui96xx" },
    { PCI_CHIP_9750,		"3dimage975" },
    { PCI_CHIP_9850,		"3dimage985" },
    { -1,				NULL }
};

/* List of PCI chipset names */
static char *TRIDENTPciNames[] = {
    "cyber9320",
    "tgui9420",
    "tgui9440",
    "tgui96xx",
    "3dimage975",
    "3dimage985",
    NULL
};

/* List of PCI IDs */
static unsigned int TRIDENTPciIds[] = {
    PCI_CHIP_9320,
    PCI_CHIP_9420,
    PCI_CHIP_9440,
    PCI_CHIP_9660,
    PCI_CHIP_9750,
    PCI_CHIP_9850,
    ~0
};

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_NOACCEL
} TRIDENTOpts;

static OptionInfoRec TRIDENTOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

#ifdef XFree86LOADER

MODULEINITPROTO(tridentModuleInit);
static MODULESETUPPROTO(tridentSetup);

static XF86ModuleVersionInfo tridentVersRec =
{
	"trident",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	TRIDENT_DRIVER_VERSION,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	{0,0,0,0}
};

/*
 * This function is the magic init function for XFree86 modules.
 * It adds the DriverRec to the list of available drivers.
 *
 * Its name has to be the driver name followed by ModuleInit()
 */
void
tridentModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		ModuleTearDownProc *teardown)
{
    *vers = &tridentVersRec;
    *setup = tridentSetup;
    *teardown = NULL;
}

pointer
tridentSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&TRIDENT, module, 0);

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

static Bool
TRIDENTGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an TRIDENTRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(TRIDENTRec), 1);
    /* Initialise it */

    return TRUE;
}

static void
TRIDENTFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}


/* Mandatory */
static void
TRIDENTIdentify(int flags)
{
    xf86PrintChipsets(TRIDENT_NAME, "driver for Trident chipsets", TRIDENTChipsets);
}


/* Mandatory */
static Bool
TRIDENTProbe(DriverPtr drv, int flags)
{
    int i;
    pciVideoPtr pPci, *usedPci;
    GDevPtr *devSections;
    GDevPtr *usedDevs;
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
     *
     * Since this test version still uses vgaHW, we'll only actually claim
     * one for now, and just print a message about the others.
     */

    /*
     * This bit is only here because we're still using vgaHW.  When we're
     * not it will disappear.
     */
    if (xf86CheckIsaSlot(ISA_COLOR) == FALSE) {
	return FALSE;
    }

    /*
     * Next we check, if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(TRIDENT_DRIVER_NAME,
					  &devSections)) <= 0) {
	/*
	 * There's no matching device section in the config file, so quit
	 * now.
	 */
	return FALSE;
    }

    /*
     * While we're VGA-dependent, can really only have one such instance, but
     * we'll ignore that.
     */

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

    if ((numUsed = xf86MatchPciInstances(TRIDENT_NAME, PCI_VENDOR_TRIDENT,
		   TRIDENTPciIds, TRIDENTPciNames, devSections, numDevSections,
		   &usedDevs, &usedPci)) <= 0) {
	return FALSE;
    }

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];

	/*
	 * Check that nothing else has claimed the slots.
	 * For now we're checking for PCI_SHARED_VGA, but that will change
	 * to PCI_ONLY when we remove the vgaHW dependence.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func,
			      PCI_SHARED_VGA)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    if (!xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func,
				  PCI_SHARED_VGA, pScrn->scrnIndex)) {
		/* This can't happen */
		FatalError("someone claimed the free slot!\n");
	    }

	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = TRIDENT_DRIVER_NAME;
	    pScrn->name		 = TRIDENT_NAME;
	    pScrn->Probe	 = TRIDENTProbe;
	    pScrn->PreInit	 = TRIDENTPreInit;
	    pScrn->ScreenInit	 = TRIDENTScreenInit;
	    pScrn->SwitchMode	 = TRIDENTSwitchMode;
	    pScrn->AdjustFrame	 = TRIDENTAdjustFrame;
	    pScrn->EnterVT	 = TRIDENTEnterVT;
	    pScrn->LeaveVT	 = TRIDENTLeaveVT;
	    pScrn->FreeScreen	 = TRIDENTFreeScreen;
	    pScrn->ValidMode	 = TRIDENTValidMode;
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
	
    for (i = 0; linep[i] != 0; i++) {
	if (linep[i] != -1) {
	    n++;
	    linePitches = (int *)xnfrealloc(linePitches, n * sizeof(int));
	    linePitches[n - 1] = i << 5;
	}
    }

    /* Mark the end of the list */
    if (n > 0) {
	linePitches = (int *)xnfrealloc(linePitches, (n + 1) * sizeof(int));
	linePitches[n] = 0;
    }
    return linePitches;
}

/* Mandatory */
static Bool
TRIDENTPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    TRIDENTPtr pTrident;
    MessageType from;
    unsigned char videoram;
    int vgaIOBase;
    int i;
    ClockRangePtr clockRanges;
    char *mod = NULL;

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

    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;

    VGAHWPTR(pScrn)->MapSize = 0x10000;		/* Standard 64k VGA window */

    if (!vgaHWMapMem(pScrn))
	return FALSE;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* The ramdac module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "ramdac"))
	return FALSE;

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     * Our preference for depth 24 is 24bpp, so tell it that too.
     */
    if (!xf86SetDepthBpp(pScrn, 8, 0, 0, Support24bppFb | Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 15:
	case 16:
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

#if 0
    /*
     * If the driver can do gamma correction, it should call xf86SetGamma()
     * here.
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }
#endif

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the TRIDENTRec driverPrivate */
    if (!TRIDENTGetRec(pScrn)) {
	return FALSE;
    }
    pTrident = TRIDENTPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, TRIDENTOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* XXX This is here just to test options. */
	/* Default to 8 */
	pScrn->rgbBits = 8;
	if (xf86GetOptValInteger(TRIDENTOptions, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
    }
    from = X_DEFAULT;
    pTrident->HWCursor = FALSE;
    if (xf86IsOptionSet(TRIDENTOptions, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	pTrident->HWCursor = TRUE;
    }
    if (xf86IsOptionSet(TRIDENTOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pTrident->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pTrident->HWCursor ? "HW" : "SW");
    if (xf86IsOptionSet(TRIDENTOptions, OPTION_NOACCEL)) {
	pTrident->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86IsOptionSet(TRIDENTOptions, OPTION_PCI_RETRY)) {
	pTrident->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }

    /* Find the PCI slot for this screen */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) != 1) {
	/* This shouldn't happen */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Expected one PCI card, but found %d\n", i);
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }

    pTrident->PciInfo = *pciList;
    pTrident->RamDac = -1;
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pTrident->Chipset = xf86StringToToken(TRIDENTChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pTrident->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(TRIDENTChipsets, pTrident->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pTrident->Chipset);
    } else {
	from = X_PROBED;
	pTrident->Chipset = pTrident->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(TRIDENTChipsets, pTrident->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pTrident->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pTrident->ChipRev);
    } else {
	pTrident->ChipRev = pTrident->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * TRIDENTProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pTrident->Chipset);
	return FALSE;
    }
    if (pTrident->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    pTrident->PciTag = pciTag(pTrident->PciInfo->bus, pTrident->PciInfo->device,
			  pTrident->PciInfo->func);
    
    if (pScrn->device->MemBase != 0) {
	pTrident->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	pTrident->FbAddress = pTrident->PciInfo->memBase[0] & 0xFF800000;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pTrident->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pTrident->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	pTrident->IOAddress = pTrident->PciInfo->memBase[1] & 0xFFFFC000;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pTrident->IOAddress);

    /* HW bpp matches reported bpp */
    pTrident->HwBpp = pScrn->bitsPerPixel;

    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	outb(vgaIOBase + 4, SPR);
	videoram = inb(vgaIOBase + 5);
	switch (videoram & 0x0F) {
	case 0x03:
	    pScrn->videoRam = 1024;
	    break;
	case 0x07:
	    pScrn->videoRam = 2048;
	    break;
	case 0x0F:
	    pScrn->videoRam = 4096;
	    break;
	default:
	    pScrn->videoRam = 1024;
	    xf86DrvMsg(pScrn->scrnIndex, from, 
			"Unable to determine VideoRam, defaulting to 1MB\n",
				pScrn->videoRam);
	    break;
	}
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    pTrident->FbMapSize = pScrn->videoRam * 1024;

    pTrident->RamDacRec = RamDacCreateInfoRec();
    pTrident->RamDacRec->ReadAddress = TridentReadAddress;
    pTrident->RamDacRec->WriteAddress = TridentWriteAddress;
    pTrident->RamDacRec->ReadData = TridentReadData;
    pTrident->RamDacRec->WriteData = TridentWriteData;
    if(!RamDacInit(pScrn, pTrident->RamDacRec)) {
	RamDacDestroyInfoRec(pTrident->RamDacRec);
	return FALSE;
    }

    /* XXX Set HW cursor use */

    /* Set the min pixel clock */
    pTrident->MinClock = 16250;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pTrident->MinClock / 1000);

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
	    pTrident->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pTrident->MaxClock = speed;
	from = X_CONFIG;
    } else {
	pTrident->MaxClock = 230000;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pTrident->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pTrident->MinClock;
    clockRanges->maxClock = pTrident->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our TRIDENTValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
#if 0
    if (pTrident->NoAccel) {
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
			      pTrident->FbMapSize,
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
			      pTrident->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }
#endif

    if (i == -1) {
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	TRIDENTFreeRec(pScrn);
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
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }

    /* Load XAA if needed */
    if (!pTrident->NoAccel || pTrident->HWCursor)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    TRIDENTFreeRec(pScrn);
	    return FALSE;
	}
 
    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
TRIDENTMapMem(ScrnInfoPtr pScrn)
{
    CARD32 save = 0;
    TRIDENTPtr pTrident;

    pTrident = TRIDENTPTR(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

    save = pciReadLong(pTrident->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pTrident->PciTag, PCI_CMD_STAT_REG,
		 save & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    /*
     * Map IO registers to virtual address space
     */ 
#if !defined(__alpha__)
    pTrident->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
		pTrident->PciTag, (pointer)pTrident->IOAddress, 0x2000);
#else
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.
     */
    pTrident->IOBase = xf86MapPciMemSparse(pScrn->scrnIndex, VIDMEM_MMIO,
				       (pointer)pTrident->IOAddress, 0x2000);
#endif
    if (pTrident->IOBase == NULL)
	return FALSE;

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

	val = *(volatile CARD32 *)(pTrident->IOBase+0);
	val = *(volatile CARD32 *)(pTrident->IOBase+0x1000);
	val = *(volatile CARD32 *)(pTrident->IOBase+0x2000);
	val = *(volatile CARD32 *)(pTrident->IOBase+0x3000);
    }
#endif

#ifdef __alpha__
    /*
     * for Alpha, we need to map DENSE memory as well, for
     * setting CPUToScreenColorExpandBase.
     */
    pTrident->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
		pTrident->PciTag, (pointer)pTrident->IOAddress, 0x2000);

    if (pTrident->IOBaseDense == NULL)
	return FALSE;
#endif /* __alpha__ */

    pTrident->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pTrident->PciTag,
				 (pointer)((unsigned long)pTrident->FbAddress),
				 pTrident->FbMapSize);
    if (pTrident->FbBase == NULL)
	return FALSE;

    /* Re-enable I/O and memory */
    pciWriteLong(pTrident->PciTag, PCI_CMD_STAT_REG,
		 save | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
TRIDENTUnmapMem(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident;

    pTrident = TRIDENTPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
#ifndef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTrident->IOBase, 0x2000);
#else
    xf86UnMapVidMemSparse(pScrn->scrnIndex, (pointer)pTrident->IOBase, 0x2000);
#endif
    pTrident->IOBase = NULL;

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTrident->IOBaseDense, 0x2000);
    pTrident->IOBaseDense = NULL;
#endif /* __alpha__ */

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTrident->FbBase, pScrn->videoRam);
    pTrident->FbBase = NULL;
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
TRIDENTSave(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident;
    vgaRegPtr vgaReg;
    TRIDENTRegPtr tridentReg;
    RamDacHWRecPtr pIBM;
    RamDacRegRecPtr IBMreg;

    pTrident = TRIDENTPTR(pScrn);
    vgaReg = &VGAHWPTR(pScrn)->SavedReg;
    pIBM = RAMDACHWPTR(pScrn);
    tridentReg = &pTrident->SavedReg;
    IBMreg = &pIBM->SavedReg;

    vgaHWSave(pScrn, vgaReg, TRUE);

    TridentSave(pScrn, tridentReg);
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
TRIDENTModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int ret = -1;
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    TRIDENTPtr pTrident;
    TRIDENTRegPtr tridentReg;
    RamDacHWRecPtr pIBM;
    RamDacRegRecPtr IBMreg;

    hwp = VGAHWPTR(pScrn);
    vgaHWUnlock(hwp);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    pTrident = TRIDENTPTR(pScrn);
    pIBM = RAMDACHWPTR(pScrn);

    ret = TridentInit(pScrn, mode);

    if (!ret)
	return FALSE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    tridentReg = &pTrident->ModeReg;
    IBMreg = &pIBM->ModeReg;

    vgaHWRestore(pScrn, vgaReg, FALSE);

    TridentRestore(pScrn, tridentReg);

    vgaHWProtect(pScrn, FALSE);

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
TRIDENTRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    TRIDENTPtr pTrident;
    TRIDENTRegPtr tridentReg;
    RamDacHWRecPtr pIBM;
    RamDacRegRecPtr IBMreg;

    hwp = VGAHWPTR(pScrn);
    pTrident = TRIDENTPTR(pScrn);
    pIBM = RAMDACHWPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    tridentReg = &pTrident->SavedReg;
    IBMreg = &pIBM->SavedReg;

    vgaHWProtect(pScrn, TRUE);

    TridentRestore(pScrn, tridentReg);

    vgaHWRestore(pScrn, vgaReg, TRUE);

    vgaHWProtect(pScrn, FALSE);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
TRIDENTScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    /* The vgaHW references will disappear one day */
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    TRIDENTPtr pTrident;
    int ret;
    VisualPtr visual;
    int savedDefaultVisualClass;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);

    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    pTrident = TRIDENTPTR(pScrn);

    /* Map the VGA memory and get the VGA IO base */
    if (!vgaHWMapMem(pScrn))
	return FALSE;
    vgaHWGetIOBase(hwp);

    /* Map the TRIDENT memory and MMIO areas */
    if (!TRIDENTMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    TRIDENTSave(pScrn);

    /* Initialise the first mode */
    TRIDENTModeInit(pScrn, pScrn->currentMode);

    /* Darken the screen for aesthetic reasons and set the viewport */
    TRIDENTSaveScreen(pScreen, FALSE);
    TRIDENTAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    /* XXX Fill the screen with black */
#if 0
    TRIDENTSaveScreen(pScreen, TRUE);
#endif

    /*
     * The next step is to setup the screen's visuals, and initialise the
     * framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before calling
     * a fb ScreenInit() function and fixed up after.
     *
     * XXX NOTE: cfbScreenInit() will not result in the default visual
     * being set correctly when there is a screen-specific value given
     * in the config file as opposed to a global value given on the
     * command line.  Saving and restoring 'defaultColorVisualClass'
     * around the fb's ScreenInit() solves this problem.
     *
     * For most PC hardware at depths >= 8, the defaults that cfb uses
     * are not appropriate.  In this driver, we fixup the visuals after.
     */

    /*
     * Reset cfb's visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.  To deal with this, call
     * cfbSetVisualTypes for each visual supported.
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
     * Temporarily set the global defaultColorVisualClass to make
     * cfbInitVisuals do what we want.
     */
    savedDefaultVisualClass = xf86GetDefaultColorVisualClass();
    xf86SetDefaultColorVisualClass(pScrn->defaultVisual);

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    switch (pScrn->bitsPerPixel) {
    case 8:
	ret = cfbScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in TRIDENTScrnInit\n",
		   pScrn->bitsPerPixel);
	    ret = FALSE;
	break;
    }
    xf86SetDefaultColorVisualClass(savedDefaultVisualClass);
    if (!ret)
	return FALSE;

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel == 8) {
	RamDacHandleColormaps(pScreen, pScrn);	
    } else {
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
	RamDacSetGamma(pScrn, FALSE);
    }

#if 0
    if (!pTrident->NoAccel) {
        switch (pTrident->Chipset)
        {
        case PCI_CHIP_9750:
	    TridentAccelInit(pScreen);
	    break;
        }
    }
#endif

    miInitializeBackingStore(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen,
			(miPointerScreenFuncPtr)xf86GetPointerScreenFuncs());

#if 0
    if (pTrident->HWCursor && 
	pTrident->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V) {
	/* Initialise HW cursor functions */
	TridentHWCursorInit(pScreen);
    }
#endif
    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    pTrident->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = TRIDENTCloseScreen;
    pScreen->SaveScreen = TRIDENTSaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}


/* Usually mandatory */
static Bool
TRIDENTSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return TRIDENTModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
TRIDENTAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    TRIDENTPtr pTrident;
    vgaHWPtr hwp;

    pScrn = xf86Screens[scrnIndex];
    hwp = VGAHWPTR(pScrn);
    pTrident = TRIDENTPTR(pScrn);
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
TRIDENTEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    /* Should we re-save the text mode on each VT enter? */
    if (!TRIDENTModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    RamDacRestoreDACValues(pScrn);
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
TRIDENTLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    TRIDENTRestore(pScrn);
    vgaHWLock(hwp);
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
TRIDENTCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);

    TRIDENTRestore(pScrn);
    vgaHWLock(hwp);
    TRIDENTUnmapMem(pScrn);
    vgaHWFreeHWRec(pScrn);
    if(pTrident->AccelInfoRec)
	XAADestroyInfoRec(pTrident->AccelInfoRec);
    if(pTrident->CursorInfoRec)
	xf86DestroyCursorInfoRec(pTrident->CursorInfoRec);
    pScrn->vtSema = FALSE;
    
    pScreen->CloseScreen = pTrident->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any per-generation data structures */

/* Optional */
static void
TRIDENTFreeScreen(int scrnIndex, int flags)
{
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    RamDacFreeRec(xf86Screens[scrnIndex]);
    TRIDENTFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
TRIDENTValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    if (mode->Flags & V_INTERLACE)
	return(MODE_BAD);

    return(MODE_OK);
}

/* Do screen blanking */

/* Mandatory */
static Bool
TRIDENTSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    return vgaHWSaveScreen(pScreen, unblank);
}
