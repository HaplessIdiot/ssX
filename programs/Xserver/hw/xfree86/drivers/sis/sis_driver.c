/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
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
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>, 
 *           Juanjo Santamarta <santamarta@ctv.es>, 
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp> 
 *           David Thomas <davtom@dream.org.uk>. 
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_driver.c,v 1.10 1999/01/23 09:55:54 dawes Exp $ */

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "xf1bpp.h"
#include "xf4bpp.h"
#include "mibank.h"
#include "micmap.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86cmap.h"
#include "vgaHW.h"

#include "mipointer.h"

#include "mibstore.h"

#include "sis_regs.h"
#include "sis.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "globals.h"
#include "extensions/dpms.h"
#endif

static void	SISIdentify(int flags);
static Bool	SISProbe(DriverPtr drv, int flags);
static Bool	SISPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	SISScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	SISEnterVT(int scrnIndex, int flags);
static void	SISLeaveVT(int scrnIndex, int flags);
static Bool	SISCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	SISSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool	SISSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	SISAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	SISFreeScreen(int scrnIndex, int flags);
static int	SISValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);

/* Internally used functions */
static Bool	SISMapMem(ScrnInfoPtr pScrn);
static Bool	SISUnmapMem(ScrnInfoPtr pScrn);
static void	SISSave(ScrnInfoPtr pScrn);
static void	SISRestore(ScrnInfoPtr pScrn);
static Bool	SISModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

#define VERSION 4000
#define SIS_NAME "SIS"
#define SIS_DRIVER_NAME "sis"
#define SIS_MAJOR_VERSION 1
#define SIS_MINOR_VERSION 0
#define SIS_PATCHLEVEL 0

/* 
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the ModuleInit
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

DriverRec SIS = {
    VERSION,
    "accelerated driver for SiS chipsets",
    SISIdentify,
    SISProbe,
    NULL,
    0
};

static SymTabRec SISChipsets[] = {
    { PCI_CHIP_SIS6326,		"sis6326" },
    { -1,				NULL }
};

static PciChipsets SISPciChipsets[] = {
    { PCI_CHIP_SIS6326,	PCI_CHIP_SIS6326,	RES_SHARED_VGA },
    { -1,		-1,		RES_UNDEFINED }
};
    
typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_NOACCEL,
    OPTION_NOTURBOQUEUE
} SISOpts;

static OptionInfoRec SISOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"rgbbits",	OPTV_INTEGER,	{0}, -1    },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOTURBOQUEUE,	"NoTurboQueue",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

int sisReg32MMIO[]={0x8280,0x8284,0x8288,0x828C,0x8290,0x8294,0x8298,0x829C,
		    0x82A0,0x82A4,0x82A8,0x82AC};

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

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWProtect",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    NULL
};

static const char *fbSymbols[] = {
    "xf1bppScreenInit",
    "xf4bppScreenInit",
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    NULL
};

static const char *racSymbols[] = {
    "xf86RACInit",
    NULL
};

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    NULL
};

static const char *i2cSymbols[] = {
    "xf86I2CBusInit",
    "xf86CreateI2CBusRec",
    NULL
};

#ifdef XFree86LOADER

MODULEINITPROTO(sisModuleInit);
static MODULESETUPPROTO(sisSetup);

static XF86ModuleVersionInfo sisVersRec =
{
	"sis",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	SIS_MAJOR_VERSION, SIS_MINOR_VERSION, SIS_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	NULL,
	{0,0,0,0}
};

/*
 * This function is the magic init function for XFree86 modules.
 * It adds the DriverRec to the list of available drivers.
 *
 * Its name has to be the driver name followed by ModuleInit()
 */
void
sisModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
		ModuleTearDownProc *teardown)
{
    *vers = &sisVersRec;
    *setup = sisSetup;
    *teardown = NULL;
}

pointer
sisSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&SIS, module, 0);
	LoaderRefSymLists(vgahwSymbols, fbSymbols, racSymbols, i2cSymbols,
			  xaaSymbols, NULL);
	return (pointer)TRUE;
    } 

    if (errmaj) *errmaj = LDR_ONCEONLY;
    return NULL;
}

#endif /* XFree86LOADER */

static Bool
SISGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an SISRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(SISRec), 1);
    /* Initialise it */

    return TRUE;
}

static void
SISFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

#ifdef DPMSExtension
static void 
SISDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{
    unsigned char DPMSCont, PMCont, temp;

#if 0
    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);
#endif

    outb(0x3C4, 0x0E);
    temp = inb(0x3C5);
    outb(0x3C5, 0xC2);
    outb(0x83C8, 0x04); /* Read DPMS Control */
	PMCont = inb(0x83C6) & 0xFC;
	outb(0x3CE, 0x23);
	DPMSCont = inb(0x3CF) & 0xFC;
	switch (PowerManagementMode)
	{
	case DPMSModeOn:
		/* Screen: On, HSync: On, VSync: On */
		PMCont |= 0x03;
		DPMSCont |= 0x00;
		break;
	case DPMSModeStandby:
		/* Screen: Off, HSync: Off, VSync: On */
		PMCont |= 0x02;
		DPMSCont |= 0x01;
		break;
	case DPMSModeSuspend:
		/* Screen: Off, HSync: On, VSync: Off */
		PMCont |= 0x02;
		DPMSCont |= 0x02;
		break;
	case DPMSModeOff:
		/* Screen: Off, HSync: Off, VSync: Off */
		PMCont |= 0x00;
		DPMSCont |= 0x03;
		break;
	}
	outb(0x3CF, DPMSCont);
	outb(0x83C8, 0x04);
	outb(0x83C6, PMCont);
	outw(0x3C4, (temp<<8) | 0x0E);

#if 0
    xf86DelControlledResource(&pScrn->Access, FALSE);
#endif
}
#endif

/* Mandatory */
static void
SISIdentify(int flags)
{
    xf86PrintChipsets(SIS_NAME, "driver for SiS chipsets", SISChipsets);
}

static void
SIS1bppColorMap(ScrnInfoPtr pScrn) {
/* In 1 bpp we have color 0 at LUT 0 and color 1 at LUT 0x3f.
   This makes white and black look right (otherwise they were both
   black. I'm sure there's a better way to do that, just lazy to
   search the docs.  */

#if 0
    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);
#endif
   outb(0x3C8, 0x00); outb(0x3C9, 0x00); outb(0x3C9, 0x00); outb(0x3C9, 0x00);
   outb(0x3C8, 0x3F); outb(0x3C9, 0x3F); outb(0x3C9, 0x3F); outb(0x3C9, 0x3F);
#if 0
    xf86DelControlledResource(&pScrn->Access,FALSE);
#endif
}

/* Mandatory */
static Bool
SISProbe(DriverPtr drv, int flags)
{
    int i;
    pciVideoPtr pPci, *usedPci;
    GDevPtr *devSections;
    GDevPtr *usedDevs;
    int *usedChips;
    int numDevSections;
    int numUsed;
    BusResource resource;
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
     * Next we check, if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(SIS_DRIVER_NAME,
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

    numUsed = xf86MatchPciInstances(SIS_NAME, PCI_VENDOR_SIS,
		   SISChipsets, SISPciChipsets, devSections,
		   numDevSections, &usedDevs, &usedPci, &usedChips);

    /* Free it since we don't need that list after this */
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];
	resource = xf86FindPciResource(usedChips[i], SISPciChipsets);

	/*
	 * Check that nothing else has claimed the slots.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func, resource)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func, resource,
			     &SIS, usedChips[i], pScrn->scrnIndex);

	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = SIS_DRIVER_NAME;
	    pScrn->name		 = SIS_NAME;
	    pScrn->Probe	 = SISProbe;
	    pScrn->PreInit	 = SISPreInit;
	    pScrn->ScreenInit	 = SISScreenInit;
	    pScrn->SwitchMode	 = SISSwitchMode;
	    pScrn->AdjustFrame	 = SISAdjustFrame;
	    pScrn->EnterVT	 = SISEnterVT;
	    pScrn->LeaveVT	 = SISLeaveVT;
	    pScrn->FreeScreen	 = SISFreeScreen;
	    pScrn->ValidMode	 = SISValidMode;
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
#if 0
    SISPtr pSiS = SISPTR(pScrn);
#endif
    int *linePitches = NULL;
    int lines[4] = { 512, 1024, 2048, 4096 }; /* 9440AGi */
#if 0
    int lines[sizeof(AvailablePitches[pSiS->Chipset])] =
		AvailablePitches[pSiS->Chipset];
#endif
    int i, n = 0;
	
    for (i = 0; i < 4; i++) {
	n++;
	linePitches = xnfrealloc(linePitches, n * sizeof(int));
	linePitches[n - 1] = lines[i];
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
SISPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    SISPtr pSiS;
    MessageType from;
    unsigned char videoram;
    char *ramtype = NULL, *mclk = NULL;
    Bool Support24bpp;
    int vgaIOBase;
    int i,j;
    unsigned char temp, unlock;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *Sym;

    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);

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

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

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
	case 1:
	case 4:
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

    /*
     * The new cmap layer needs this to be initialised.
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the SISRec driverPrivate */
    if (!SISGetRec(pScrn)) {
	return FALSE;
    }
    pSiS = SISPTR(pScrn);
    pSiS->pScrn = pScrn;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, SISOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* XXX This is here just to test options. */
	/* Default to 8 */
	pScrn->rgbBits = 6;
#if 0
	if (xf86GetOptValInteger(SISOptions, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
#endif
    }
    from = X_DEFAULT;
    pSiS->HWCursor = TRUE;
    if (xf86IsOptionSet(SISOptions, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	pSiS->HWCursor = TRUE;
    }
    if (xf86IsOptionSet(SISOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pSiS->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pSiS->HWCursor ? "HW" : "SW");
    if (xf86IsOptionSet(SISOptions, OPTION_NOACCEL)) {
	pSiS->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86IsOptionSet(SISOptions, OPTION_PCI_RETRY)) {
	pSiS->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }

    /* Find the PCI slot for this screen */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) != 1) {
	/* This shouldn't happen */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Expected one PCI card, but found %d\n", i);
	SISFreeRec(pScrn);
	return FALSE;
    }

    pSiS->TurboQueue = FALSE; /* For now */
    pSiS->PciInfo = *pciList;
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pSiS->Chipset = xf86StringToToken(SISChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pSiS->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(SISChipsets, pSiS->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pSiS->Chipset);
    } else {
	from = X_PROBED;
	pSiS->Chipset = pSiS->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(SISChipsets, pSiS->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pSiS->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pSiS->ChipRev);
    } else {
	pSiS->ChipRev = pSiS->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * SISProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pSiS->Chipset);
	return FALSE;
    }
    if (pSiS->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    outb(0x3C4, 0x05); unlock = inb(0x3C5);
    outw(0x3C4, 0x8605); /* Unlock registers */

    switch (pSiS->Chipset) {
	case PCI_CHIP_SIS6326:
	    pSiS->TurboQueue = TRUE; /* Turn on for 6326 */
    	    if (xf86IsOptionSet(SISOptions, OPTION_NOTURBOQUEUE)) {
		pSiS->TurboQueue = FALSE;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Disabling TurboQueue\n");
    	    }
	    outb(0x3C4, ExtConfStatus1); temp = inb(0x3C5);
	    switch (temp & 0x03) {
		case 0x00:
		    ramtype = "SGRAM/SDRAM";
		    switch ((temp & 0xE0)>>5) {
			case 0x00:
			    mclk = "66";
			    break;
			case 0x01:
			    mclk = "75";
			    break;
			case 0x02:
			    mclk = "83";
			    break;
			case 0x03:
			    mclk = "90";
			    break;
			case 0x04:
			    mclk = "100";
			    break;
			case 0x05:
			    mclk = "115";
			    break;
			case 0x06:
			    mclk = "134";
			    break;
			case 0x07:
			    mclk = "50";
			    break;
		    }
		    break;
		case 0x01:
		    ramtype = "2cycle EDO DRAM";
		    switch ((temp & 0xE0)>>5) {
			case 0x00:
			    mclk = "65";
			    break;
			case 0x01:
			    mclk = "70";
			    break;
			case 0x02:
			    mclk = "75";
			    break;
			case 0x03:
			    mclk = "80";
			    break;
			case 0x04:
			    mclk = "85";
			    break;
			case 0x05:
			    mclk = "90";
			    break;
			case 0x06:
			    mclk = "55";
			    break;
			case 0x07:
			    mclk = "60";
			    break;
		    }
		    break;
		case 0x02:
		    ramtype = "1cycle EDO DRAM";
		    switch ((temp & 0xE0)>>5) {
			case 0x00:
			    mclk = "50";
			    break;
			case 0x01:
			    mclk = "55";
			    break;
			case 0x02:
			    mclk = "60";
			    break;
			case 0x03:
			    mclk = "65";
			    break;
			case 0x04:
			    mclk = "70";
			    break;
			case 0x05:
			    mclk = "75";
			    break;
			case 0x06:
			    mclk = "80";
			    break;
			case 0x07:
			    mclk = "45";
			    break;
		    }
		    break;
		case 0x03:
		    ramtype = "Fast Page DRAM";
		    switch ((temp & 0xE0)>>5) {
			case 0x00:
			    mclk = "55";
			    break;
			case 0x01:
			    mclk = "60";
			    break;
			case 0x02:
			    mclk = "65";
			    break;
			case 0x03:
			    mclk = "70";
			    break;
			case 0x04:
			    mclk = "75";
			    break;
			case 0x05:
			    mclk = "80";
			    break;
			case 0x06:
			    mclk = "45";
			    break;
			case 0x07:
			    mclk = "50";
			    break;
		    }
		    break;
		break;
	}
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Installed RAM type is %s\n",ramtype);
    xf86DrvMsg(pScrn->scrnIndex, from, "Memory Clock is %sMHz\n",mclk);

    pSiS->PciTag = pciTag(pSiS->PciInfo->bus, pSiS->PciInfo->device,
			  pSiS->PciInfo->func);
    
    if (pScrn->device->MemBase != 0) {
	pSiS->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	pSiS->FbAddress = pSiS->PciInfo->memBase[0] & 0xFFFFFFF0;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pSiS->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pSiS->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	pSiS->IOAddress = pSiS->PciInfo->memBase[1] & 0xFFFFFFF0;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pSiS->IOAddress);

    /* HW bpp matches reported bpp */
    pSiS->HwBpp = pScrn->bitsPerPixel;

    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	outb(0x3C4, RAMSize); /* Get memory size */
	videoram = (inb(0x3C5) >> 1);
	switch (videoram & 0x0B) {
	case 0x00:
	    pScrn->videoRam = 1024;
	    break;
	case 0x01:
	    pScrn->videoRam = 2048;
	    break;
	case 0x02:
	    pScrn->videoRam = 4096;
	    break;
	case 0x03:
	    pScrn->videoRam = 1024;
	    break;
	case 0x08:
	    pScrn->videoRam = 0; /* OUCH ! */
	    break;
	case 0x09:
	    pScrn->videoRam = 2048;
	    break;
	case 0x0A:
	    pScrn->videoRam = 4096;
	    break;
	case 0x0B:
	    pScrn->videoRam = 8192;
	    break;
	default:
	    pScrn->videoRam = 1024;
	    xf86DrvMsg(pScrn->scrnIndex, from, 
			"Unable to determine VideoRam, defaulting to 1MB\n",
				pScrn->videoRam);
	    break;
	}
    }
    outw(0x3C4, (unlock << 8) | 0x05);

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    pSiS->FbMapSize = pScrn->videoRam * 1024;

    /* Set the min pixel clock */
    pSiS->MinClock = 16250;	/* XXX Guess, need to check this */
    pSiS->MaxClock = 170000;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pSiS->MinClock / 1000);

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
	    pSiS->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pSiS->MaxClock = speed;
	from = X_CONFIG;
    } 
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pSiS->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pSiS->MinClock;
    clockRanges->maxClock = pSiS->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our SISValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
#if 0
    if (pSiS->NoAccel) {
#endif
	/*
	 * XXX Assuming min pitch 256, max 4096
	 * XXX Assuming min height 128, max 4096
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 4096,
			      pScrn->bitsPerPixel, 128, 4096,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pSiS->FbMapSize,
			      LOOKUP_BEST_REFRESH);
#if 0
    } else {
	/*
	 * XXX Assuming min height 128, max 2048
	 */
	j = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      GetAccelPitchValues(pScrn), 0, 0,
			      pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pSiS->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }
#endif

    if (i == -1) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	SISFreeRec(pScrn);
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
    case 1:
	mod = "xf1bpp";
	Sym = "xf1bppScreenInit";
	break;
    case 4:
	mod = "xf4bpp";
	Sym = "xf4bppScreenInit";
	break;
    case 8:
	mod = "cfb";
	Sym = "cfbScreenInit";
	break;
    case 16:
	mod = "cfb16";
	Sym = "cfb16ScreenInit";
	break;
    case 24:
	mod = "cfb24";
	Sym = "cfb24ScreenInit";
	break;
    case 32:
	mod = "cfb32";
	Sym = "cfb32ScreenInit";
	break;
    }

    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(Sym, NULL);

    if (!xf86LoadSubModule(pScrn, "rac")) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymLists(racSymbols, NULL);

    if (!xf86LoadSubModule(pScrn, "i2c")) {
	SISFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymLists(i2cSymbols, NULL);

    /* Load XAA if needed */
    if (!pSiS->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    SISFreeRec(pScrn);
	    return FALSE;
	}

        xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
SISMapMem(ScrnInfoPtr pScrn)
{
    CARD32 save = 0;
    SISPtr pSiS;

    pSiS = SISPTR(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

    save = pciReadLong(pSiS->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pSiS->PciTag, PCI_CMD_STAT_REG,
		 save & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    /*
     * Map IO registers to virtual address space
     */ 
#if !defined(__alpha__)
    pSiS->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
		pSiS->PciTag, (pointer)pSiS->IOAddress, 0x10000);
#else
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.
     */
    pSiS->IOBase = xf86MapPciMemSparse(pScrn->scrnIndex, VIDMEM_MMIO,
				       (pointer)pSiS->IOAddress, 0x10000);
#endif
    if (pSiS->IOBase == NULL)
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

	val = *(volatile CARD32 *)(pSiS->IOBase+0);
	val = *(volatile CARD32 *)(pSiS->IOBase+0x1000);
	val = *(volatile CARD32 *)(pSiS->IOBase+0x2000);
	val = *(volatile CARD32 *)(pSiS->IOBase+0x3000);
    }
#endif

#ifdef __alpha__
    /*
     * for Alpha, we need to map DENSE memory as well, for
     * setting CPUToScreenColorExpandBase.
     */
    pSiS->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
		pSiS->PciTag, (pointer)pSiS->IOAddress, 0x1000);

    if (pSiS->IOBaseDense == NULL)
	return FALSE;
#endif /* __alpha__ */

    pSiS->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pSiS->PciTag,
				 (pointer)((unsigned long)pSiS->FbAddress),
				 pSiS->FbMapSize);
    if (pSiS->FbBase == NULL)
	return FALSE;

    /* Re-enable I/O and memory */
    pciWriteLong(pSiS->PciTag, PCI_CMD_STAT_REG,
		 save | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
SISUnmapMem(ScrnInfoPtr pScrn)
{
    SISPtr pSiS;

    pSiS = SISPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
#ifndef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pSiS->IOBase, 0x1000);
#else
    xf86UnMapVidMemSparse(pScrn->scrnIndex, (pointer)pSiS->IOBase, 0x1000);
#endif
    pSiS->IOBase = NULL;

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pSiS->IOBaseDense, 0x1000);
    pSiS->IOBaseDense = NULL;
#endif /* __alpha__ */

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pSiS->FbBase, pScrn->videoRam);
    pSiS->FbBase = NULL;
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
SISSave(ScrnInfoPtr pScrn)
{
    SISPtr pSiS;
    vgaRegPtr vgaReg;
    SISRegPtr sisReg;

    pSiS = SISPTR(pScrn);
    vgaReg = &VGAHWPTR(pScrn)->SavedReg;
    sisReg = &pSiS->SavedReg;

#if 0
    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);
#endif

    vgaHWSave(pScrn, vgaReg, VGA_SR_ALL);

    SiSSave(pScrn, sisReg);

#if 0
    xf86DelControlledResource(&pScrn->Access,FALSE);
#endif
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
SISModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    SISPtr pSiS = SISPTR(pScrn);
    SISRegPtr sisReg;

#if 0
    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);
#endif

    vgaHWUnlock(hwp);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    if (!SiSInit(pScrn, mode))
	return FALSE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    sisReg = &pSiS->ModeReg;

    vgaReg->Attribute[0x10] = 0x01;

    vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);

    SiSRestore(pScrn, sisReg);

    vgaHWProtect(pScrn, FALSE);

#if 0
    xf86DelControlledResource(&pScrn->Access,FALSE);
#endif

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
SISRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    SISPtr pSiS;
    SISRegPtr sisReg;

    hwp = VGAHWPTR(pScrn);
    pSiS = SISPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    sisReg = &pSiS->SavedReg;

#if 0
    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);
#endif

    vgaHWProtect(pScrn, TRUE);

    SiSRestore(pScrn, sisReg);

    vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);

    vgaHWProtect(pScrn, FALSE);

#if 0
    xf86DelControlledResource(&pScrn->Access,FALSE);
#endif
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
SISScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    /* The vgaHW references will disappear one day */
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    SISPtr pSiS;
    int ret;
    VisualPtr visual;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);

    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    pSiS = SISPTR(pScrn);

    /* Map the VGA memory and get the VGA IO base */
    if (!vgaHWMapMem(pScrn))
	return FALSE;
    vgaHWGetIOBase(hwp);

    /* Map the SIS memory and MMIO areas */
    if (!SISMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    SISSave(pScrn);

    /* Initialise the first mode */
    if (!SISModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    /* Darken the screen for aesthetic reasons and set the viewport */
    SISSaveScreen(pScreen, FALSE);
    SISAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
     * support TrueColor and not DirectColor.
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
    case 1:
	ret = xf1bppScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 8:
	ret = cfbScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pSiS->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in SISScrnInit\n",
		   pScrn->bitsPerPixel);
	    ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

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
    } else if (pScrn->depth == 1) {
	SIS1bppColorMap(pScrn);
    }

    if (!pSiS->NoAccel)
	SiSAccelInit(pScreen);

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

#if 0
    if (pSiS->HWCursor)
	SiSHWCursorInit(pScreen);
#endif

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    if (!vgaHWHandleColormaps(pScreen))
	return FALSE;

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, (DPMSSetProcPtr)SISDisplayPowerManagementSet, 0);
#endif

    pSiS->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = SISCloseScreen;
    pScreen->SaveScreen = SISSaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Turn on the screen now */
    SISSaveScreen(pScreen, TRUE);

    return TRUE;
}


/* Usually mandatory */
static Bool
SISSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return SISModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
SISAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    SISPtr pSiS;
    vgaHWPtr hwp;
    int base = y * pScrn->displayWidth + x;
    int vgaIOBase;
    unsigned char temp;

    hwp = VGAHWPTR(pScrn);
    pSiS = SISPTR(pScrn);
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
SISEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    /* Should we re-save the text mode on each VT enter? */
    if (!SISModeInit(pScrn, pScrn->currentMode))
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
SISLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    SISRestore(pScrn);
    vgaHWLock(hwp);
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
SISCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    SISPtr pSiS = SISPTR(pScrn);

    if (pScrn->vtSema) {
    	SISRestore(pScrn);
    	vgaHWLock(hwp);
    	SISUnmapMem(pScrn);
    }
    if(pSiS->AccelInfoRec)
	XAADestroyInfoRec(pSiS->AccelInfoRec);
    if(pSiS->CursorInfoRec)
	xf86DestroyCursorInfoRec(pSiS->CursorInfoRec);
    pScrn->vtSema = FALSE;
    
    pScreen->CloseScreen = pSiS->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any per-generation data structures */

/* Optional */
static void
SISFreeScreen(int scrnIndex, int flags)
{
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    SISFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
SISValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    return(MODE_OK);
}

/* Do screen blanking */

/* Mandatory */
static Bool
SISSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    return vgaHWSaveScreen(pScreen, unblank);
}
