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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_driver.c,v 1.55 1999/04/27 12:05:15 dawes Exp $ */

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
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

#include "trident_regs.h"
#include "trident.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "globals.h"
#define DPMS_SERVER
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

void 	TridentLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indicies, LOCO *colors, short visualClass);

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;
 
#define VERSION 4000
#define TRIDENT_NAME "TRIDENT"
#define TRIDENT_DRIVER_NAME "trident"
#define TRIDENT_MAJOR_VERSION 1
#define TRIDENT_MINOR_VERSION 0
#define TRIDENT_PATCHLEVEL 0

/* 
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the SetupProc
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
    { PCI_CHIP_9388,		"cyber9388" },
    { PCI_CHIP_9397,		"cyber9397" },
    { PCI_CHIP_939A,		"cyber939a" },
    { PCI_CHIP_9520,		"cyber9520" },
    { PCI_CHIP_9525,		"cyber9525" },
    { PCI_CHIP_9420,		"tgui9420" },
    { PCI_CHIP_9440,		"tgui9440" },
    { PCI_CHIP_9660,		"tgui9660" },
    { PCI_CHIP_9660,		"tgui9680" },
    { PCI_CHIP_9660,		"providia9682" },
    { PCI_CHIP_9660,		"providia9685" },
    { PCI_CHIP_9750,		"3dimage975" },
    { PCI_CHIP_9850,		"3dimage985" },
    { PCI_CHIP_9880,		"blade3d" },
    { -1,				NULL }
};

static PciChipsets TRIDENTPciChipsets[] = {
    { PCI_CHIP_9320,	PCI_CHIP_9320,	RES_SHARED_VGA },
    { PCI_CHIP_9388,	PCI_CHIP_9388,	RES_SHARED_VGA },
    { PCI_CHIP_9397,	PCI_CHIP_9397,	RES_SHARED_VGA },
    { PCI_CHIP_939A,	PCI_CHIP_939A,	RES_SHARED_VGA },
    { PCI_CHIP_9520,	PCI_CHIP_9520,	RES_SHARED_VGA },
    { PCI_CHIP_9525,	PCI_CHIP_9525,	RES_SHARED_VGA },
    { PCI_CHIP_9420,	PCI_CHIP_9420,	RES_SHARED_VGA },
    { PCI_CHIP_9440,	PCI_CHIP_9440,	RES_SHARED_VGA },
    { PCI_CHIP_9660,	PCI_CHIP_9660,	RES_SHARED_VGA },
    { PCI_CHIP_9750,	PCI_CHIP_9750,	RES_SHARED_VGA },
    { PCI_CHIP_9850,	PCI_CHIP_9850,	RES_SHARED_VGA },
    { PCI_CHIP_9880,	PCI_CHIP_9880,	RES_SHARED_VGA },
    { -1,		-1,		RES_UNDEFINED }
};
    
typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_NOACCEL,
    OPTION_SETMCLK,
    OPTION_MUX_THRESHOLD
} TRIDENTOpts;

static OptionInfoRec TRIDENTOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SETMCLK,		"SetMClk",	OPTV_FREQ,	{0}, FALSE },
    { OPTION_MUX_THRESHOLD,	"MUXThreshold",	OPTV_INTEGER,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

#if 0
static int AvailablePitches[] = {
	{ 0 },		/* 8200LX */
	{ 0 },		/* 8800CS */
	{ 0 },		/* 8900B */
	{ 0 },		/* 8900C */
	{ 0 },		/* 8900CL */
	{ 0 },		/* 8900D */
	{ 0 },		/* 9000 */
	{ 0 },		/* 9000i */
	{ 0 },		/* 9100B */
	{ 0 },		/* 9200CXr */
	{ 0 },		/* 9400CXi */
	{ 0 },				/* 9420 */
	{ 0 },				/* 9420DGi */
	{ 0 },				/* 9430DGi */
	{ 512, 1024, 2048, 4096 },	/* 9440AGi */
	{ 512, 640, 800, 1024, 1280, 2048, 4096 },	/* 9320 */
	{ 512, 640, 832, 1024, 1280, 2048, 4096 },	/* 9660 */
	{ 512, 640, 800, 1024, 1280, 2048, 4096 },	/* 9680 */
	{ 512, 640, 800, 832, 1024, 1280, 1600, 2048, 4096 },	/* 9682 */
	{ 512, 640, 800, 832, 1024, 1280, 1600, 2048, 4096 },	/* 9685 */
	{ 512, 640, 800, 832, 1024, 1280, 1600, 2048, 4096 },	/* 9685 */
	{ 512, 640, 800, 832, 1024, 1280, 1600, 2048, 4096 },	/* 9685 */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
};
#endif

/* Clock Limits */
static int ClockLimit[] = {
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	90000,
	90000,
	135000,
	135000,
	135000,
	170000,
	135000,
	135000,
	170000,
	170000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
};

static int ClockLimit16bpp[] = {
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	45000,
	45000,
	135000,
	135000,
	135000,
	170000,
	135000,
	135000,
	170000,
	170000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
}; 

static int ClockLimit24bpp[] = {
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	70000,
	70000,
	70000,
	85000,
	70000,
	70000,
	85000,
	85000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
};

static int ClockLimit32bpp[] = {
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	70000,
	70000,
	70000,
	85000,
	70000,
	70000,
	85000,
	85000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
};

static const char *xaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAACopyROP",
    "XAAPatternROP",
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
    "cfb24_32ScreenInit",
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

static MODULESETUPPROTO(tridentSetup);

static XF86ModuleVersionInfo tridentVersRec =
{
	"trident",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	TRIDENT_MAJOR_VERSION, TRIDENT_MINOR_VERSION, TRIDENT_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

XF86ModuleData tridentModuleData = { &tridentVersRec, tridentSetup, NULL };

pointer
tridentSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&TRIDENT, module, 0);
	LoaderRefSymLists(vgahwSymbols, fbSymbols, racSymbols, i2cSymbols,
			  xaaSymbols, NULL);
	return (pointer)TRUE;
    } 

    if (errmaj) *errmaj = LDR_ONCEONLY;
    return NULL;
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

#ifdef DPMSExtension
static void 
TRIDENTDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    unsigned char DPMSCont, PMCont, temp;

    MMIO_OUTB(0x3C4, 0x0E);
    temp = MMIO_INB(0x3C5);
    MMIO_OUTB(0x3C5, 0xC2);
    MMIO_OUTB(0x83C8, 0x04); /* Read DPMS Control */
	PMCont = MMIO_INB(0x83C6) & 0xFC;
	MMIO_OUTB(0x3CE, 0x23);
	DPMSCont = MMIO_INB(0x3CF) & 0xFC;
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
	MMIO_OUTB(0x3CF, DPMSCont);
	MMIO_OUTB(0x83C8, 0x04);
	MMIO_OUTB(0x83C6, PMCont);
	MMIO_OUTW(0x3C4, (temp<<8) | 0x0E);
}
#endif

/* Mandatory */
static void
TRIDENTIdentify(int flags)
{
    xf86PrintChipsets(TRIDENT_NAME, "driver for Trident chipsets", TRIDENTChipsets);
}

static void
Trident1bppColorMap(ScrnInfoPtr pScrn) {
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
/* In 1 bpp we have color 0 at LUT 0 and color 1 at LUT 0x3f.
   This makes white and black look right (otherwise they were both
   black. I'm sure there's a better way to do that, just lazy to
   search the docs.  */

   MMIO_OUTB(0x3C8, 0x00); MMIO_OUTB(0x3C9, 0x00); MMIO_OUTB(0x3C9, 0x00); MMIO_OUTB(0x3C9, 0x00);
   MMIO_OUTB(0x3C8, 0x3F); MMIO_OUTB(0x3C9, 0x3F); MMIO_OUTB(0x3C9, 0x3F); MMIO_OUTB(0x3C9, 0x3F);
}

/* Mandatory */
static Bool
TRIDENTProbe(DriverPtr drv, int flags)
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

    numUsed = xf86MatchPciInstances(TRIDENT_NAME, PCI_VENDOR_TRIDENT,
		   TRIDENTChipsets, TRIDENTPciChipsets, devSections,
		   numDevSections, &usedDevs, &usedPci, &usedChips);

    /* Free it since we don't need that list after this */
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];
	resource = xf86FindPciResource(usedChips[i], TRIDENTPciChipsets);

	/*
	 * Check that nothing else has claimed the slots.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func, resource)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func, resource,
			     &TRIDENT, usedChips[i], pScrn->scrnIndex);

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
#if 0
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
#endif
    int *linePitches = NULL;
    int lines[4] = { 512, 1024, 2048, 4096 }; /* 9440AGi */
#if 0
    int lines[sizeof(AvailablePitches[pTrident->Chipset])] =
		AvailablePitches[pTrident->Chipset];
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
TRIDENTPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    TRIDENTPtr pTrident;
    MessageType from;
    unsigned char videoram;
    char *ramtype = NULL, *chipset = NULL;
    Bool Support24bpp;
    int vgaIOBase;
    float mclk;
    double real;
    int i,j;
    unsigned char revision;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *Sym = "";

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
    if (!xf86SetDepthBpp(pScrn, 8, 0, 0, Support24bppFb | Support32bppFb |
			    SupportConvert32to24 /*| PreferConvert32to24*/)) {
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

    xf86PrintDepthBpp(pScrn);

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
	pix24bpp = xf86GetBppFromDepth(pScrn, 24);

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

    /* Allocate the TRIDENTRec driverPrivate */
    if (!TRIDENTGetRec(pScrn)) {
	return FALSE;
    }
    pTrident = TRIDENTPTR(pScrn);
    pTrident->pScrn = pScrn;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, TRIDENTOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* XXX This is here just to test options. */
	/* Default to 8 */
	pScrn->rgbBits = 6;
#if 0
	if (xf86GetOptValInteger(TRIDENTOptions, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
#endif
    }
    from = X_DEFAULT;
    pTrident->HWCursor = TRUE;
    if (xf86GetOptValBool(TRIDENTOptions, OPTION_HW_CURSOR, &pTrident->HWCursor))
	from = X_CONFIG;
    if (xf86ReturnOptValBool(TRIDENTOptions, OPTION_SW_CURSOR, FALSE)) {
	from = X_CONFIG;
	pTrident->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pTrident->HWCursor ? "HW" : "SW");
    if (xf86ReturnOptValBool(TRIDENTOptions, OPTION_NOACCEL, FALSE)) {
	pTrident->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86ReturnOptValBool(TRIDENTOptions, OPTION_PCI_RETRY, FALSE)) {
	pTrident->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }
    pTrident->MUXThreshold = 100000; /* 100MHz */
    if (xf86GetOptValInteger(TRIDENTOptions, OPTION_MUX_THRESHOLD, 
						&pTrident->MUXThreshold)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "MUX Threshold set to %d\n",
						pTrident->MUXThreshold);
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

    pTrident->PciTag = pciTag(pTrident->PciInfo->bus, pTrident->PciInfo->device,
			  pTrident->PciInfo->func);
    
    if (pScrn->device->MemBase != 0) {
	pTrident->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	pTrident->FbAddress = pTrident->PciInfo->memBase[0] & 0xFFFFFFF0;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pTrident->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pTrident->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	pTrident->IOAddress = pTrident->PciInfo->memBase[1] & 0xFFFFC000;
    }

    xf86DrvMsg(pScrn->scrnIndex,X_PROBED,"IO registers at 0x%lX\n",pTrident->IOAddress);

    if (!TRIDENTMapMem(pScrn))
	return FALSE;

    MMIO_OUTB(0x3C4, RevisionID); revision = MMIO_INB(0x3C5);
    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Revision is %d\n",revision);

    pTrident->EngineOperation = 0x00;
    pTrident->UseGERetry = FALSE;
    pTrident->IsCyber = FALSE;
    pTrident->HasSGRAM = FALSE;
    pTrident->NewClockCode = FALSE;
    pTrident->MUX = FALSE;
    Support24bpp = FALSE;

    MMIO_OUTB(vgaIOBase + 4, InterfaceSel);

    switch (pTrident->Chipset) {
	case PCI_CHIP_9440:
    	    pTrident->ddc1Read = Tridentddc1Read;
	    pTrident->HWCursor = FALSE;
	    chipset = "TGUI9440";
	    pTrident->Chipset = TGUI9440AGi;
	    ramtype = "Standard DRAM";
	    if (pTrident->UsePCIRetry)
	    	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry not supported, disabling\n");
	    pTrident->UsePCIRetry = FALSE; /* Not Supported */
	    pTrident->frequency = NTSC;
	    break;
	case PCI_CHIP_9320:
    	    pTrident->ddc1Read = Tridentddc1Read;
	    chipset = "Cyber9320";
	    pTrident->Chipset = CYBER9320;
	    ramtype = "Standard DRAM";
	    if (pTrident->UsePCIRetry)
	    	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry not supported, disabling\n");
	    pTrident->UsePCIRetry = FALSE; /* Not Supported */
	    break;
	case PCI_CHIP_9660:
    	    pTrident->ddc1Read = Tridentddc1Read;
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
		ramtype = "EDO Ram";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C)
		ramtype = "Standard DRAM";
	    switch (revision) {
		case 0x00:
		    chipset = "TGUI9660";
		    pTrident->Chipset = TGUI9660;
		    if (pTrident->UsePCIRetry)
		    	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry not supported, disabling\n");
		    pTrident->UsePCIRetry = FALSE; /* Not Supported */
		    break;
		case 0x01:
		    chipset = "TGUI9680";
		    pTrident->Chipset = TGUI9680;
		    if (pTrident->UsePCIRetry)
		    	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry not supported, disabling\n");
		    pTrident->UsePCIRetry = FALSE; /* Not Supported */
		    break;
		case 0x10:
		    chipset = "ProVidia 9682";
		    Support24bpp = TRUE;
		    pTrident->Chipset = PROVIDIA9682;
		    if (pTrident->UsePCIRetry)
		    	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry not supported, disabling\n");
		    pTrident->UsePCIRetry = FALSE; /* Not Supported */
		    break;
		case 0x21:
		    chipset = "ProVidia 9685";
		    Support24bpp = TRUE;
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = PROVIDIA9685;
		    break;
		case 0x22:
		case 0x23:
		    chipset = "Cyber 9397";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
			ramtype = "EDO Ram";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
			ramtype = "SDRAM";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
			pTrident->HasSGRAM = TRUE;
			ramtype = "SGRAM";
	    	    }
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = CYBER9397;
		    pTrident->IsCyber = TRUE;
		    break;
		case 0x2a:
		    chipset = "Cyber 939A/DVD";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
			ramtype = "EDO Ram";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
			ramtype = "SDRAM";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
			pTrident->HasSGRAM = TRUE;
			ramtype = "SGRAM";
	    	    }
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = CYBER939A;
		    pTrident->IsCyber = TRUE;
		    break;
		case 0x30:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0xB3:
		    chipset = "Cyber 9385";
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = CYBER9385;
		    pTrident->IsCyber = TRUE;
		    break;
		case 0x38:
		case 0x3A:
		    chipset = "Cyber 9385-1";
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = CYBER9385;
		    pTrident->IsCyber = TRUE;
		    break;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		    chipset = "Cyber 9382";
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = CYBER9382;
		    pTrident->IsCyber = TRUE;
		    break;
		case 0x4A:
		    chipset = "Cyber 9388";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
			ramtype = "EDO Ram";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
			ramtype = "SDRAM";
    	    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
			pTrident->HasSGRAM = TRUE;
			ramtype = "SGRAM";
	    	    }
		    pTrident->NewClockCode = TRUE;
		    pTrident->Chipset = CYBER9388;
		    pTrident->IsCyber = TRUE;
		    break;
		default:
		    chipset = "Unknown";
		    pTrident->Chipset = TGUI9660;
		    break;
	    }
	    break;
	case PCI_CHIP_9520:
    	    pTrident->ddc1Read = Tridentddc1Read;
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
		ramtype = "EDO Ram";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
		ramtype = "SDRAM";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
		pTrident->HasSGRAM = TRUE;
		ramtype = "SGRAM";
	    }
	    Support24bpp = TRUE;
	    chipset = "Cyber 9520";
	    pTrident->NewClockCode = TRUE;
	    pTrident->Chipset = CYBER9520;
	    break;
	case PCI_CHIP_9525:
    	    pTrident->ddc1Read = Tridentddc1Read;
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
		ramtype = "EDO Ram";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
		ramtype = "SDRAM";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
		pTrident->HasSGRAM = TRUE;
		ramtype = "SGRAM";
	    }
	    Support24bpp = TRUE;
	    chipset = "Cyber 9525/DVD";
	    pTrident->NewClockCode = TRUE;
	    pTrident->Chipset = CYBER9525;
	    break;
	case PCI_CHIP_9750:
    	    pTrident->ddc1Read = Tridentddc1Read;
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
		ramtype = "EDO Ram";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
		ramtype = "SDRAM";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
		pTrident->HasSGRAM = TRUE;
		ramtype = "SGRAM";
	    }
	    Support24bpp = TRUE;
	    chipset = "3DImage975";
	    pTrident->NewClockCode = TRUE;
	    pTrident->Chipset = IMAGE975;
	    break;
	case PCI_CHIP_9850:
    	    pTrident->ddc1Read = Tridentddc1Read;
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x04)
		ramtype = "EDO Ram";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
		ramtype = "SDRAM";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
		pTrident->HasSGRAM = TRUE;
		ramtype = "SGRAM";
	    }
	    Support24bpp = TRUE;
	    chipset = "3DImage985";
	    pTrident->NewClockCode = TRUE;
	    pTrident->Chipset = IMAGE985;
	    break;
	case PCI_CHIP_9880:
    	    pTrident->ddc1Read = Tridentddc1Read;
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x08)
		ramtype = "SDRAM";
    	    if ((MMIO_INB(vgaIOBase + 5) & 0x0C) == 0x0C) {
		pTrident->HasSGRAM = TRUE;
		ramtype = "SGRAM";
	    }
	    Support24bpp = TRUE;
	    chipset = "Blade3D";
	    pTrident->NewClockCode = TRUE;
	    pTrident->frequency = NTSC;
	    pTrident->Chipset = BLADE3D;
	    break;
    }


    if (!chipset) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No support for \"%s\"\n",
			pScrn->chipset);
	return FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Found %s chip\n", chipset);
    if (ramtype)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "RAM type is %s\n", ramtype);

    if (pScrn->bitsPerPixel == 24 && !Support24bpp) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "No support for 24bpp on this chipset, use -pixmap32.\n");
	return FALSE;
    }

    /* HW bpp matches reported bpp */
    pTrident->HwBpp = pScrn->bitsPerPixel;

    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	MMIO_OUTB(vgaIOBase + 4, SPR);
	videoram = MMIO_INB(vgaIOBase + 5);
	switch (videoram & 0x0F) {
	case 0x03:
	    pScrn->videoRam = 1024;
	    break;
	case 0x04: /* Blade3D */
	    pScrn->videoRam = 8192;
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


#if 0
    pTrident->RamDacRec = RamDacCreateInfoRec();
    pTrident->RamDacRec->ReadAddress = TridentReadAddress;
    pTrident->RamDacRec->WriteAddress = TridentWriteAddress;
    pTrident->RamDacRec->ReadData = TridentReadData;
    pTrident->RamDacRec->WriteData = TridentWriteData;
    if(!RamDacInit(pScrn, pTrident->RamDacRec)) {
	RamDacDestroyInfoRec(pTrident->RamDacRec);
	return FALSE;
    }
#endif

    pTrident->MCLK = 0;
    mclk = CalculateMCLK(pScrn);
    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Memory Clock is %3.2f MHz\n", mclk);
    if (xf86GetOptValFreq(TRIDENTOptions, OPTION_SETMCLK, OPTUNITS_MHZ,
				&real)) {
	pTrident->MCLK = (int)(real * 1000.0);
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Setting new Memory Clock to %3.2f MHz\n",
						(float)(pTrident->MCLK / 1000));
    }
		
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
	switch (pScrn->bitsPerPixel) {
	    case 16:
		pTrident->MaxClock = ClockLimit16bpp[pTrident->Chipset];
		break;
	    case 24:
		pTrident->MaxClock = ClockLimit24bpp[pTrident->Chipset];
		break;
	    case 32:
		pTrident->MaxClock = ClockLimit32bpp[pTrident->Chipset];
		break;
	    default:
		pTrident->MaxClock = ClockLimit[pTrident->Chipset];
		break;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pTrident->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pTrident->MinClock;
    clockRanges->maxClock = pTrident->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our TRIDENTValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    if ((pScrn->depth < 8) || 
        (pScrn->bitsPerPixel == 24))
		pTrident->NoAccel = TRUE;

    /* Select valid modes from those available */
    if (pTrident->NoAccel) {
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
			      pTrident->FbMapSize,
			      LOOKUP_BEST_REFRESH);
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
			      pTrident->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }

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
    case 1:
	pTrident->EngineOperation |= 0x00;
	mod = "xf1bpp";
	Sym = "xf1bppScreenInit";
	break;
    case 4:
	pTrident->EngineOperation |= 0x00;
	mod = "xf4bpp";
	Sym = "xf4bppScreenInit";
	break;
    case 8:
	pTrident->EngineOperation |= 0x00;
	mod = "cfb";
	Sym = "cfbScreenInit";
	break;
	break;
    case 16:
	pTrident->EngineOperation |= 0x01;
	mod = "cfb16";
	Sym = "cfb16ScreenInit";
	break;
    case 24:
	pTrident->EngineOperation |= 0x03;
	if (pix24bpp == 24) {
	    mod = "cfb24";
	    Sym = "cfb24ScreenInit";
	} else {
	    mod = "xf24_32bpp";
	    Sym = "cfb24_32ScreenInit";
	}
	break;
    case 32:
	pTrident->EngineOperation |= 0x02;
	mod = "cfb32";
	Sym = "cfb32ScreenInit";
	break;
    }

    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(Sym, NULL);

    if (!xf86LoadSubModule(pScrn, "rac")) {
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymLists(racSymbols, NULL);

    if (!xf86LoadSubModule(pScrn, "i2c")) {
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymLists(i2cSymbols, NULL);

    /* Load XAA if needed */
    if (!pTrident->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    TRIDENTFreeRec(pScrn);
	    return FALSE;
	}

        xf86LoaderReqSymLists(xaaSymbols, NULL);

        switch (pScrn->displayWidth * pScrn->bitsPerPixel / 8) {
	    case 512:
		pTrident->EngineOperation |= 0x00;
		break;
	    case 1024:
		pTrident->EngineOperation |= 0x04;
		break;
	    case 2048:
		pTrident->EngineOperation |= 0x08;
		break;
	    case 4096:
		pTrident->EngineOperation |= 0x0C;
		break;
	}
    }

    /* Load DDC if needed */
    /* This gives us DDC1 - we should be able to get DDC2B using i2c */
    if (!xf86LoadSubModule(pScrn, "ddc")) {
	TRIDENTFreeRec(pScrn);
	return FALSE;
    }
    xf86LoaderReqSymLists(ddcSymbols, NULL);

#if 0
    /* Initialize DDC1 if possible */
    if (pTrident->ddc1Read) 
	xf86PrintEDID(xf86DoEDID_DDC1(pScrn->scrnIndex,vgaHWddc1SetSpeed,pTrident->ddc1Read ) );
#endif

    TRIDENTUnmapMem(pScrn);

    pTrident->FbMapSize = pScrn->videoRam * 1024;

    return TRUE;
}


/*
 * Enable MMIO.
 */

static void
TRIDENTEnableMMIO(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    unsigned char temp;

    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);

    /* Goto New Mode */
    outb(0x3C4, 0x0B); inb(0x3C5);

    /* Unprotect registers */
    outb(0x3C4, NewMode1); temp = inb(0x3C5);
    outb(0x3C5, 0xC0);

    /* Enable MMIO */
    outb(vgaIOBase + 4, PCIReg); pTrident->REGPCIReg = inb(vgaIOBase + 5);
    outb(vgaIOBase + 5, pTrident->REGPCIReg | 0x01);

    xf86DelControlledResource(&pScrn->Access, FALSE);

    xf86AddControlledResource(pScrn,MEM);
    xf86EnableAccess(&pScrn->Access);

    /* Protect registers */
    MMIO_OUTB(0x3C4, NewMode1);
    MMIO_OUTB(0x3C5, temp);
}

/*
 * Disable MMIO.
 */

static void
TRIDENTDisableMMIO(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    unsigned char temp;

    /* Goto New Mode */
    MMIO_OUTB(0x3C4, 0x0B); temp = MMIO_INB(0x3C5);

    /* Unprotect registers */
    MMIO_OUTB(0x3C4, NewMode1); temp = MMIO_INB(0x3C5);
    MMIO_OUTB(0x3C5, 0xC0);

    /* Disable MMIO access */
    MMIO_OUTB(vgaIOBase + 4, PCIReg);
    MMIO_OUTB(vgaIOBase + 5, pTrident->REGPCIReg & 0xFE);

    xf86DelControlledResource(&pScrn->Access, FALSE);

    xf86AddControlledResource(pScrn,IO);
    xf86EnableAccess(&pScrn->Access);

    /* Protect registers */
    outb(0x3C4, NewMode1);
    outb(0x3C5, temp);

    xf86DelControlledResource(&pScrn->Access, FALSE);
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
TRIDENTMapMem(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    int mmioFlags;

    /*
     * Map IO registers to virtual address space
     */ 
#if !defined(__alpha__)
    mmioFlags = VIDMEM_MMIO;
#else
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.
     */
    mmioFlags = VIDMEM_MMIO | VIDMEM_SPARSE;
#endif
    pTrident->IOBase = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, 
		pTrident->PciTag, pTrident->IOAddress, 0x20000);
    if (pTrident->IOBase == NULL)
	return FALSE;

#ifdef __alpha__
    /*
     * for Alpha, we need to map DENSE memory as well, for
     * setting CPUToScreenColorExpandBase.
     */
    pTrident->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
		pTrident->PciTag, pTrident->IOAddress, 0x1000);

    if (pTrident->IOBaseDense == NULL)
	return FALSE;
#endif /* __alpha__ */

    if (pTrident->FbMapSize != 0) {
    	pTrident->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pTrident->PciTag,
				 (unsigned long)pTrident->FbAddress,
				 pTrident->FbMapSize);
    	if (pTrident->FbBase == NULL)
	    return FALSE;
    }

    TRIDENTEnableMMIO(pScrn);
    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
TRIDENTUnmapMem(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    unsigned char temp;

    TRIDENTDisableMMIO(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTrident->IOBase, 0x20000);
    pTrident->IOBase = NULL;

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTrident->IOBaseDense, 0x1000);
    pTrident->IOBaseDense = NULL;
#endif /* __alpha__ */

    if (pTrident->FbMapSize != 0) {
    	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pTrident->FbBase, pTrident->FbMapSize);
    	pTrident->FbBase = NULL;
    }

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

    pTrident = TRIDENTPTR(pScrn);
    vgaReg = &VGAHWPTR(pScrn)->SavedReg;
    tridentReg = &pTrident->SavedReg;

    if (xf86IsPrimaryPci(pTrident->PciInfo))
	vgaHWSave(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_FONTS);
    else
	vgaHWSave(pScrn, vgaReg, VGA_SR_MODE);

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
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    TRIDENTRegPtr tridentReg;

    if (mode->Clock > pTrident->MUXThreshold) pTrident->MUX = TRUE;
    else				      pTrident->MUX = FALSE;

    switch (pTrident->Chipset) {
	case TGUI9660:
	case TGUI9680:
	case PROVIDIA9682:
	case PROVIDIA9685:
	case IMAGE975:
	case IMAGE985:
	case BLADE3D:
	case CYBER9520:
	case CYBER9525:
	case CYBER9397:
	case CYBER939A:
	    /* Get ready for MUX mode */
	    if (pTrident->MUX && 
		pScrn->bitsPerPixel == 8 && 
		!mode->CrtcHAdjusted) {
		mode->CrtcHDisplay >>= 1;
		mode->CrtcHSyncStart >>= 1;
		mode->CrtcHSyncEnd >>= 1;
		mode->CrtcHBlankStart >>= 1;
		mode->CrtcHBlankEnd >>= 1;
		mode->CrtcHTotal >>= 1;
		mode->CrtcHAdjusted = TRUE;
	    }
	    break;
    }

    vgaHWUnlock(hwp);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    if (!TridentInit(pScrn, mode))
	return FALSE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    tridentReg = &pTrident->ModeReg;

    vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);

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

    hwp = VGAHWPTR(pScrn);
    pTrident = TRIDENTPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    tridentReg = &pTrident->SavedReg;

    vgaHWProtect(pScrn, TRUE);

    TridentRestore(pScrn, tridentReg);

    if (xf86IsPrimaryPci(pTrident->PciInfo))
	vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_FONTS);
    else
	vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);

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

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];
    pTrident = TRIDENTPTR(pScrn);

    /* Map the TRIDENT memory and MMIO areas */
    if (!TRIDENTMapMem(pScrn))
	return FALSE;

    hwp = VGAHWPTR(pScrn);
    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    /* Initialize the MMIO vgahw functions */
    if (xf86IsPrimaryPci(pTrident->PciInfo)) {
	if (!vgaHWMapMem(pScrn))
   	    return FALSE;
    }
    vgaHWSetMmioFuncs(hwp, pTrident->IOBase, 0);
    vgaHWGetIOBase(hwp);

    /* Save the current state */
    TRIDENTSave(pScrn);

    /* Initialise the first mode */
    if (!TRIDENTModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    /* Darken the screen for aesthetic reasons and set the viewport */
    TRIDENTSaveScreen(pScreen, FALSE);
    TRIDENTAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
	ret = xf1bppScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	break;
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
	if (pix24bpp == 24)
	    ret = cfb24ScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
			pScrn->virtualY, pScrn->xDpi, pScrn->yDpi, 
			pScrn->displayWidth);
	else
	    ret = cfb24_32ScreenInit(pScreen, pTrident->FbBase, pScrn->virtualX,
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
	Trident1bppColorMap(pScrn);
    }

#if 0 /* Banking support has been disabled */
    if (pScrn->depth < 8) {
	miBankInfoPtr pBankInfo;

	/* Setup the vga banking variables */
	pBankInfo = xnfcalloc(sizeof(miBankInfoRec),1);
	if (pBankInfo == NULL)
	    return FALSE;
	
	pBankInfo->pBankA = hwp->Base;
	pBankInfo->pBankB = hwp->Base;
	pBankInfo->BankSize = 0x10000;
	pBankInfo->nBankDepth = pScrn->depth;

	pBankInfo->SetSourceBank = 
		(miBankProcPtr)TGUISetRead;
	pBankInfo->SetDestinationBank = 
		(miBankProcPtr)TGUISetWrite;
	pBankInfo->SetSourceAndDestinationBanks = 
		(miBankProcPtr)TGUISetReadWrite;
	if (!miInitializeBanking(pScreen, pScrn->virtualX, pScrn->virtualY,
				 pScrn->displayWidth, pBankInfo)) {
	    xfree(pBankInfo);
	    pBankInfo = NULL;
	    return FALSE;
	}
    }
#endif

    if (!pTrident->NoAccel) {
	if (Is3Dchip) {
	    if (pTrident->Chipset == BLADE3D)
		BladeAccelInit(pScreen);
	    else
	    	ImageAccelInit(pScreen);
	} else {
	    	TridentAccelInit(pScreen);
        }
    }

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    if (pTrident->HWCursor)
	TridentHWCursorInit(pScreen);

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    if(!xf86HandleColormaps(pScreen, 256, 6, TridentLoadPalette,
			NULL, CMAP_RELOAD_ON_MODE_SWITCH|CMAP_PALETTED_TRUECOLOR))
	return FALSE;

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, (DPMSSetProcPtr)TRIDENTDisplayPowerManagementSet, 0);
#endif

    pTrident->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = TRIDENTCloseScreen;
    pScreen->SaveScreen = TRIDENTSaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

#if 0
    TRIDENTI2CInit(pScreen);

    xf86PrintEDID(xf86DoEDID_DDC2(pScrn->scrnIndex,pTrident->DDC));
#endif

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
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    TRIDENTPtr pTrident;
    vgaHWPtr hwp;
    int base = y * pScrn->displayWidth + x;
    int vgaIOBase;
    unsigned char temp;

    hwp = VGAHWPTR(pScrn);
    pTrident = TRIDENTPTR(pScrn);
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    switch (pScrn->bitsPerPixel) {
	case 4:
	    base >>= 3;
	    break;
	case 8:
	    base = (base & 0xFFFFFFF8) >> 2;
	    break;
	case 16:
	    base >>= 1;
	    break;
	case 24:
	    base = (((base + 1) & ~0x03) * 3) >> 2;
	    break;
	case 32:
	    break;
    }

    /* CRT bits 0-15 */
    MMIO_OUTW(vgaIOBase + 4, (base & 0x00FF00) | 0x0C);
    MMIO_OUTW(vgaIOBase + 4, ((base & 0x00FF) << 8) | 0x0D);
    /* CRT bit 16 */
    MMIO_OUTB(vgaIOBase + 4, CRTCModuleTest); temp = MMIO_INB(vgaIOBase + 5) & 0xDF;
    MMIO_OUTB(vgaIOBase + 5, temp | ((base & 0x10000) >> 11));
    /* CRT bit 17-19 */
    MMIO_OUTB(vgaIOBase + 4, CRTHiOrd); temp = MMIO_INB(vgaIOBase + 5) & 0xF8;
    MMIO_OUTB(vgaIOBase + 5, temp | ((base & 0xE0000) >> 17));
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 */

/* Mandatory */
static Bool
TRIDENTEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    unsigned char temp;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    TRIDENTEnableMMIO(pScrn);

    /* Should we re-save the text mode on each VT enter? */
    if (!TRIDENTModeInit(pScrn, pScrn->currentMode))
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
TRIDENTLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int vgaIOBase = hwp->IOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    unsigned char temp;

    TRIDENTRestore(pScrn);
    vgaHWLock(hwp);
   
    TRIDENTDisableMMIO(pScrn);
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

    if (pScrn->vtSema) {
    	TRIDENTRestore(pScrn);
    	vgaHWLock(hwp);
    	TRIDENTUnmapMem(pScrn);
    }
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
    return(MODE_OK);
}

/* Do screen blanking */

/* Mandatory */
static Bool
TRIDENTSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    return vgaHWSaveScreen(pScreen, unblank);
}
