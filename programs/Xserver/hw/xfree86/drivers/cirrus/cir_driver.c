/*
 * Driver for CL-GD5480.
 * Itai Nahshon.
 *
 * This is mainly a cut & paste from the MGA driver.
 * Original autors and contributors list include:
 *	Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *	David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *	Guy DESBIEF
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_driver.c,v 1.24 1998/11/15 04:30:21 dawes Exp $ */

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

/* These need to be checked */
#if 0
#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif
#endif

#include "cir.h"

#ifdef CIRPROBEI2C
/* For debugging... should go away. */
static void CirProbeI2C(int scrnIndex);
#endif

/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */

static void	CIRIdentify(int flags);
static Bool	CIRProbe(DriverPtr drv, int flags);
extern Bool	LgPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	CIRPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	CIRScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
extern Bool	LgScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	CIREnterVT(int scrnIndex, int flags);
static void	CIRLeaveVT(int scrnIndex, int flags);
extern Bool	LgEnterVT(int scrnIndex, int flags);
extern void	LgLeaveVT(int scrnIndex, int flags);
static Bool	CIRCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	CIRSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool	CIRSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
extern Bool	LgSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	CIRAdjustFrame(int scrnIndex, int x, int y, int flags);
extern void	LgAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	CIRFreeScreen(int scrnIndex, int flags);
extern void	LgFreeScreen(int scrnIndex, int flags);
static int	CIRValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);
int	LgValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);
#ifdef DPMSExtension
static void	CIRDisplayPowerManagementSet(ScrnInfoPtr pScrn,
					     int PowerManagementMode,
					     int flags);
#endif

/* Internally used functions */
Bool	CIRMapMem(ScrnInfoPtr pScrn);
Bool	CIRUnmapMem(ScrnInfoPtr pScrn);
static void	CIRSave(ScrnInfoPtr pScrn);
static void	CIRRestore(ScrnInfoPtr pScrn);
static Bool	CIRModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
#define VERSION 4000
#define CIR_NAME "CIRRUS"
#define CIR_DRIVER_NAME "cirrus"
/*
 * The major version is the 0xffff0000 part and the minor version is the
 * 0x0000ffff part.
 */
#define CIR_DRIVER_VERSION 0x00010001

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec CIRRUS = {
    VERSION,
    "Driver for Cirrus Logic GD5446, GD5480, and GD5462/4/5 cards",
    CIRIdentify,
    CIRProbe,
    NULL,
    0
};

/* Supported chipsets */
SymTabRec CIRChipsets[] = {
    { PCI_CHIP_GD5446,		"CLGD5446" },
    { PCI_CHIP_GD5480,		"CLGD5480" },
    { PCI_CHIP_GD5462,          "CL-GD5462" },
    { PCI_CHIP_GD5464,          "CL-GD5464" },
    { PCI_CHIP_GD5464BD,        "CL-GD5464BD" },
    { PCI_CHIP_GD5465,          "CL-GD5465" },
    {-1,			NULL }
};

/* List of PCI chipset names */
static PciChipsets CIRPciChipsets[] = {
    { PCI_CHIP_GD5446,	PCI_CHIP_GD5446,	RES_SHARED_VGA },
    { PCI_CHIP_GD5480,	PCI_CHIP_GD5480,	RES_SHARED_VGA },
    { PCI_CHIP_GD5462,	PCI_CHIP_GD5462,	RES_SHARED_VGA },
    { PCI_CHIP_GD5464,	PCI_CHIP_GD5464,	RES_SHARED_VGA },
    { PCI_CHIP_GD5464BD,PCI_CHIP_GD5464BD,	RES_SHARED_VGA },
    { PCI_CHIP_GD5465,	PCI_CHIP_GD5465,	RES_SHARED_VGA },
    { -1,		-1,			RES_UNDEFINED}
};

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_NOACCEL,
    OPTION_MMIO,
    OPTION_NOMMIO,
    OPTION_MEMCFG1,
    OPTION_MEMCFG2
} CIROpts;

static OptionInfoRec CIROptions[] = {
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_TRI,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_NOMMIO,		"NoMMIO",	OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_MEMCFG1,		"MemCFG1",	OPTV_INTEGER,	{0}, -1 },
    { OPTION_MEMCFG2,		"MemCFG2",	OPTV_INTEGER,	{0}, -1 },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};


#ifdef XFree86LOADER

MODULEINITPROTO(cirrusModuleInit);
static MODULESETUPPROTO(cirSetup);

static XF86ModuleVersionInfo cirVersRec =
{
	"cirrus",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	CIR_DRIVER_VERSION,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	{0,0,0,0}
};

/*
 * This function is the module init function.
 * Its name has to be the driver name followed by ModuleInit()
 */
void
cirrusModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	         ModuleTearDownProc *teardown)
{
    *vers = &cirVersRec;
    *setup = cirSetup;
    *teardown = NULL;
}

static pointer
cirSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&CIRRUS, module, 0);

	/*
	 * Modules that this driver always requires may be loaded here
	 * by calling LoadSubModule().
	 *
	 * Although this driver currently always requires the vgahw module
	 * that dependency will be removed later, so we don't load it here.
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

/*                                1/4bpp   8bpp   15/16bpp  24bpp  32bpp */
static int gd5446_MaxClocks[] = { 135100, 135100,  85500,  85500,      0 };
static int gd5480_MaxClocks[] = { 135100, 200000, 200000, 135100, 135100 };

static Bool
CIRGetRec(ScrnInfoPtr pScrn)
{
#ifdef CIR_DEBUG
   ErrorF("CIRGetRec\n");
#endif
    /*
     * Allocate an CIRRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(CIRRec), 1);
    /* Initialise it */

#ifdef CIR_DEBUG
    ErrorF("CIRGetRec 0x%x\n", CIRPTR(pScrn));
#endif
    return TRUE;
}

static void
CIRFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}


/* Mandatory */
static void
CIRIdentify(int flags)
{
    xf86PrintChipsets(CIR_NAME, "driver for Cirrus chipsets", CIRChipsets);
}


/* Mandatory */
static Bool
CIRProbe(DriverPtr drv, int flags)
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

#if 1
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

#endif

    /*
     * Next we check, if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(CIR_DRIVER_NAME,
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

    numUsed = xf86MatchPciInstances(CIR_NAME, PCI_VENDOR_CIRRUS,
			CIRChipsets, CIRPciChipsets, devSections,
			numDevSections, &usedDevs, &usedPci, &usedChips);
    /* Free it since we don't need that list after this */
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];
	resource = xf86FindPciResource(usedChips[i], CIRPciChipsets);

	/*
	 * Check that nothing else has claimed the slots.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func, resource)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func, resource,
			     &CIRRUS, usedChips[i], pScrn->scrnIndex);

	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = CIR_DRIVER_NAME;
	    pScrn->name		 = CIR_NAME;
	    pScrn->Probe	 = CIRProbe;
	    /* The Laguna family of chips is so different from the Alpine
	       family that we won't share even the highest-level of 
	       functions.  But, the Laguna chips /are/ Cirrus chips, so
	       they should be handled in this driver (as opposed to their
	       own driver). */
	    if (pPci->chipType == PCI_CHIP_GD5462 ||
		pPci->chipType == PCI_CHIP_GD5464 ||
		pPci->chipType == PCI_CHIP_GD5464BD ||
		pPci->chipType == PCI_CHIP_GD5465) {
	      pScrn->PreInit	 = LgPreInit;
#if 1
	      pScrn->ScreenInit	 = LgScreenInit;
	      pScrn->SwitchMode	 = LgSwitchMode;
	      pScrn->AdjustFrame = LgAdjustFrame;
	      pScrn->EnterVT	 = LgEnterVT;
	      pScrn->LeaveVT	 = LgLeaveVT;
	      pScrn->FreeScreen	 = LgFreeScreen;
	      pScrn->ValidMode	 = LgValidMode;
#endif
	    } else {
	      pScrn->PreInit	 = CIRPreInit;
	      pScrn->ScreenInit	 = CIRScreenInit;
	      pScrn->SwitchMode	 = CIRSwitchMode;
	      pScrn->AdjustFrame = CIRAdjustFrame;
	      pScrn->EnterVT	 = CIREnterVT;
	      pScrn->LeaveVT	 = CIRLeaveVT;
	      pScrn->FreeScreen	 = CIRFreeScreen;
	      pScrn->ValidMode	 = CIRValidMode;
	    }
	    pScrn->device	 = usedDevs[i];
	    foundScreen = TRUE;
	}
    }
    xfree(usedDevs);
    xfree(usedPci);

    return foundScreen;
}

/*
 * CIRCountRAM --
 *
 * Counts amount of installed RAM 
 *
 * XXX Can use options to configure memory on non-promary cards.
 */
static int
CIRCountRam(ScrnInfoPtr pScrn)
{
	CIRPtr pCir = CIRPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	MessageType from;
	int videoram = 0;

	/* If using MMIO we must temporary map the
	   IO ports in order to read/write memory config
	   registers. */
	if(pCir->UseMMIO) {
		/* Map the CIR memory and MMIO areas */
		pCir->FbMapSize = 1024*1024; /* XX temp */
		if (!CIRMapMem(pScrn))
			return 0;
		vgaHWSetMmioFuncs(hwp, pCir->IOBase, -0x3C0);
	}

        if(pCir->SR0F != (CARD32)-1) {
		from = X_CONFIG;
		hwp->writeSeq(hwp, 0x0F, pCir->SR0F);
	}
	else {
		from = X_PROBED;
		pCir->SR0F = hwp->readSeq(hwp, 0x0F);
	}
        xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 1 is 0x%02X\n",
	       pCir->SR0F);

        switch (pCir->Chipset) {
        case PCI_CHIP_GD5446:
		videoram = 1024;

		if(pCir->SR17 != (CARD32)-1) {
			from = X_CONFIG;
			hwp->writeSeq(hwp, 0x17, pCir->SR17);
		}
		else {
			from = X_PROBED;
			pCir->SR17 = hwp->readSeq(hwp, 0x17);
		}
		xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 2 is 0x%02X\n",
			pCir->SR17);

		if ((pCir->SR0F & 0x18) == 0x18) {
			if(pCir->SR0F & 0x80) {
				if(pCir->SR17 & 0x80)
					videoram = 2048;
				else if(pCir->SR17 & 0x02)
					videoram = 3072;
				else
					videoram = 4096;
			}
			else {
				if((pCir->SR17 & 80) == 0)
					videoram = 2048;
			}
		}
		break;
		
	case PCI_CHIP_GD5480:
		videoram = 1024;
		if ((pCir->SR0F & 0x18) == 0x18) {	/* 2 or 4 MB */
			videoram *= 2048;
			if (pCir->SR0F & 0x80)	/* Second bank enable */
				videoram = 4096;
		}
		break;
	}
	/* If using MMIO we must temporary map the
	   IO ports in order to read/write memory config
	   registers. */
	if(pCir->UseMMIO) {
		/* UNMap the CIR memory and MMIO areas */
		if (!CIRUnmapMem(pScrn))
			return 0;
	}
	return videoram;
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
    CIRPtr pCir = CIRPTR(pScrn);
	
    /* XXX ajv - 512, 576, and 1536 may not be supported
       line pitches. see sdk pp 4-59 for more
       details. Why anyone would want less than 640 is 
       bizarre. (maybe lots of pixels tall?) */

    /* The only line pitches the accelerator supports */
#if 0		
    int accelWidths[] = { 512, 576, 640, 768, 800, 960, 
		    1024, 1152, 1280, 1536, 1600, 1920, 2048, 0 };
#else
    int accelWidths[] = { 640, 768, 800, 960, 1024, 1152, 1280,
		    1600, 1920, 2048, 0 };
#endif

    for (i = 0; accelWidths[i] != 0; i++) {
	if (accelWidths[i] % pCir->Rounding == 0) {
	    n++;
	    linePitches = (int *)xnfrealloc(linePitches, n * sizeof(int));
	    linePitches[n - 1] = accelWidths[i];
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
CIRPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    CIRPtr pCir;
    MessageType from;
    int i;
    ClockRangePtr clockRanges;
    char *mod = NULL;

#ifdef CIR_DEBUG
    ErrorF("CIRPreInit\n");
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

    /* The gamma fields must be initialised when using the new cmap code */
    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the CIRRec driverPrivate */
    if (!CIRGetRec(pScrn)) {
	return FALSE;
    }
    pCir = CIRPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, CIROptions);

    pScrn->rgbBits = 6;
    from = X_DEFAULT;
    pCir->HWCursor = FALSE;
    if (xf86GetOptValBool(CIROptions, OPTION_HW_CURSOR, &pCir->HWCursor)) {
	from = X_CONFIG;
    }
    if (xf86IsOptionSet(CIROptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pCir->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pCir->HWCursor ? "HW" : "SW");
    if (xf86IsOptionSet(CIROptions, OPTION_NOACCEL)) {
	pCir->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if(pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Cannot use accelerations in less than 8 bpp\n");
	pCir->NoAccel = TRUE;
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
	CIRFreeRec(pScrn);
	if (i > 0)
	    xfree(pciList);
	return FALSE;
    }

    pCir->PciInfo = *pciList;
    xfree(pciList);
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pCir->Chipset = xf86StringToToken(CIRChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pCir->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(CIRChipsets, pCir->Chipset);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pCir->Chipset);
    } else {
	from = X_PROBED;
	pCir->Chipset = pCir->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(CIRChipsets, pCir->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pCir->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pCir->ChipRev);
    } else {
	pCir->ChipRev = pCir->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * CIRProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pCir->Chipset);
	return FALSE;
    }
    if (pCir->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);
	
    pCir->PciTag = pciTag(pCir->PciInfo->bus, pCir->PciInfo->device,
			  pCir->PciInfo->func);
    
    /* Find the frame buffer base address */
    if (pScrn->device->MemBase != 0) {
	pCir->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	if (pCir->PciInfo->memBase[0] != 0) {
	    /* 5446B and 5480 use mask of 0xfe000000.
               5446A uses 0xff000000. */
	    pCir->FbAddress = pCir->PciInfo->memBase[0] & 0xff000000;
	    from = X_PROBED;
	} else {
	   xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                      "No valid FB address in PCI config space\n");
	   CIRFreeRec(pScrn);
	   return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pCir->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pCir->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	if (pCir->PciInfo->memBase[1] != 0) {
	    pCir->IOAddress = pCir->PciInfo->memBase[1] & 0xfffff000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "No valid MMIO address in PCI config space\n");
            /* 5446 rev A do not use a separate MMIO segment */
	    /* We do not really need that YET. */
	}
    }
    if(pCir->IOAddress != 0) {
        xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
    	       (unsigned long)pCir->IOAddress);
       	/* Default to MMIO if we have a separate IOAddress and 
           not in monochrome mode (IO 0x3Bx is not relocated!) */
        if (pScrn->bitsPerPixel != 1)
          pCir->UseMMIO = TRUE;
    }

    /* User options can override the MMIO default */
#if 0
    /* Will we ever support MMIO on 5446A or older? */
    if (xf86IsOptionSet(CIROptions, OPTION_MMIO)) {
        pCir->UseMMIO = TRUE;
	from = X_CONFIG;
    }
#endif
    if (xf86IsOptionSet(CIROptions, OPTION_NOMMIO)) {
        pCir->UseMMIO = FALSE;
	from = X_CONFIG;
    }
    if (pCir->UseMMIO) {
	xf86DrvMsg(pScrn->scrnIndex, from, "Using MMIO\n");
    }

    /* XXX If UseMMIO == TRUE and for any reason we cannot do MMIO,
       abort here */

    /* XXX We do not know yet how to configure memory on this card.
       Use options MemCFG1 and MemCFG2 to set registers SR0F and
       SR17 before trying to count ram size. */

    pCir->SR0F = (CARD32)-1;
    pCir->SR17 = (CARD32)-1;

    (void) xf86GetOptValULong(CIROptions, OPTION_MEMCFG1, &pCir->SR0F);
    (void) xf86GetOptValULong(CIROptions, OPTION_MEMCFG2, &pCir->SR17);

    /*
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	pScrn->videoRam = CIRCountRam(pScrn);
	from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
	
    pCir->FbMapSize = pScrn->videoRam * 1024;
	
    /* XXX Set HW cursor use */

    /* Set the min pixel clock */
    pCir->MinClock = 12000;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pCir->MinClock / 1000);
    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->dacSpeeds[0]) {
        ErrorF("Do not specily a Clocks line for Cirrus chips\n");
        return FALSE;
    } else {
	int speed;
        int *p = NULL;
        switch (pCir->Chipset) {
	case PCI_CHIP_GD5446:
	   p = gd5446_MaxClocks;
	   break;
	case PCI_CHIP_GD5480:
	   p = gd5480_MaxClocks;
	   break;
	}
	if (!p)
	  return FALSE;
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
	pCir->MaxClock = speed;
	from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pCir->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pCir->MinClock;
    clockRanges->maxClock = pCir->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
    clockRanges->ClockMulFactor = 1;
    clockRanges->ClockDivFactor = 1;
    clockRanges->PrivFlags = 0;

    pCir->Rounding = 128 >> pCir->BppShift;

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our CIRValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
    if (pCir->NoAccel) {
	/*
	 * XXX Assuming min pitch 256, max 2048
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 2048,
			      pCir->Rounding * pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pCir->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    
    } else {
	/*
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      GetAccelPitchValues(pScrn), 0, 0,
			      pCir->Rounding * pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pCir->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }
    if (i == -1) {
	CIRFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	CIRFreeRec(pScrn);
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
	CIRFreeRec(pScrn);
	return FALSE;
    }

    /* Load XAA if needed */
    if (!pCir->NoAccel)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    CIRFreeRec(pScrn);
	    return FALSE;
	}

    /* Load ramdac if needed */
    if (pCir->HWCursor)
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    CIRFreeRec(pScrn);
	    return FALSE;
	}

   if (!xf86LoadSubModule(pScrn, "i2c")) {
       CIRFreeRec(pScrn);
       return FALSE;
   }

    return TRUE;
}

/*
 * Map the framebuffer and MMIO memory.
 */

Bool
CIRMapMem(ScrnInfoPtr pScrn)
{
    CARD32 save;
    CIRPtr pCir;

#ifdef CIR_DEBUG
    ErrorF("CIRMapMem\n");
#endif

    pCir = CIRPTR(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

    save = pciReadLong(pCir->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pCir->PciTag, PCI_CMD_STAT_REG,
		 save & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

#ifdef CIR_DEBUG
    ErrorF("CIRMapMem save=0x%08x\n", save);
#endif

    /*
     * Map the frame buffer.
     */

    pCir->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pCir->PciTag,
				 (pointer)((unsigned long)pCir->FbAddress),
				 pCir->FbMapSize);
    if (pCir->FbBase == NULL)
	return FALSE;

#ifdef CIR_DEBUG
    ErrorF("CIRMapMem pCir->FbBase=0x%08x\n", pCir->FbBase);
#endif

    /*
     * Map IO registers to virtual address space
     */ 
    if (pCir->IOAddress == 0) {
        pCir->IOBase = NULL; /* Until we are ready to use MMIO */
    }
    else {
#if !defined(__alpha__)
        pCir->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
                                     pCir->PciTag,
				     (pointer)pCir->IOAddress, 0x4000);
#else
        /*
         * For Alpha, we need to map SPARSE memory, since we need
         * byte/short access.
         */
        pCir->IOBase = xf86MapPciMemSparse(pScrn->scrnIndex, VIDMEM_MMIO,
				           (pointer)pCir->IOAddress, 0x4000);
#endif
        if (pCir->IOBase == NULL)
	    return FALSE;
    }

#ifdef CIR_DEBUG
    ErrorF("CIRMapMem pCir->IOBase=0x%08x\n", pCir->IOBase);
#endif

#if defined(SVR4)
    /*
     * For some SVR4 versions, a 32-bit read is done for the first
     * location in each page when the page is first mapped.  If this
     * is done while memory and I/O are enabled, the result will be
     * a lockup, so make sure each page is mapped here while it is safe
     * to do so.
     */
    if (pCir->IOBase != NULL) {
	CARD32 val;

	val = *(volatile CARD32 *)(pCir->IOBase+0);
	val = *(volatile CARD32 *)(pCir->IOBase+0x1000);
	val = *(volatile CARD32 *)(pCir->IOBase+0x2000);
	val = *(volatile CARD32 *)(pCir->IOBase+0x3000);
    }
#endif

#ifdef __alpha__
    if (pCir->IOBase != NULL) {
        /*
         * for Alpha, we need to map DENSE memory as well, for
         * setting CPUToScreenColorExpandBase.
         */
        pCir->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
					    pCir->PciTag, (pointer)pCir->IOAddr,
					    0x4000);
        if (pCir->IOBaseDense == NULL)
	    return FALSE;
    }
#endif /* __alpha__ */

    /* Re-enable I/O and memory */
    pciWriteLong(pCir->PciTag, PCI_CMD_STAT_REG,
		 save | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));
    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

Bool
CIRUnmapMem(ScrnInfoPtr pScrn)
{
    CIRPtr pCir;

#ifdef CIR_DEBUG
    ErrorF("CIRUnmapMem\n");
#endif

    pCir = CIRPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
#ifndef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pCir->IOBase, 0x4000);
#else
    xf86UnMapVidMemSparse(pScrn->scrnIndex, (pointer)pCir->IOBase, 0x4000);
#endif
    pCir->IOBase = NULL;

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pCir->IOBaseDense, 0x4000);
    pCir->IOBaseDense = NULL;
#endif /* __alpha__ */

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pCir->FbBase, pScrn->videoRam);
    pCir->FbBase = NULL;
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
CIRSave(ScrnInfoPtr pScrn)
{
    CIRPtr pCir;
    vgaHWPtr hwp;

#ifdef CIR_DEBUG
    ErrorF("CIRSave\n");
#endif

    hwp = VGAHWPTR(pScrn);
    pCir = CIRPTR(pScrn);

    vgaHWSave(pScrn, &VGAHWPTR(pScrn)->SavedReg, VGA_SR_ALL);

#if 1
    pCir->ModeReg.ExtVga[CR1B] = pCir->SavedReg.ExtVga[CR1B] = hwp->readCrtc(hwp, 0x1B);
    pCir->ModeReg.ExtVga[CR1D] = pCir->SavedReg.ExtVga[CR1D] = hwp->readCrtc(hwp, 0x1D);
    pCir->ModeReg.ExtVga[SR07] = pCir->SavedReg.ExtVga[SR07] = hwp->readSeq(hwp, 0x07);
    pCir->ModeReg.ExtVga[SR0E] = pCir->SavedReg.ExtVga[SR0E] = hwp->readSeq(hwp, 0x0E);
    pCir->ModeReg.ExtVga[SR12] = pCir->SavedReg.ExtVga[SR12] = hwp->readSeq(hwp, 0x12);
    pCir->ModeReg.ExtVga[SR13] = pCir->SavedReg.ExtVga[SR13] = hwp->readSeq(hwp, 0x13);
    pCir->ModeReg.ExtVga[SR1E] = pCir->SavedReg.ExtVga[SR1E] = hwp->readSeq(hwp, 0x1E);
    pCir->ModeReg.ExtVga[GR17] = pCir->SavedReg.ExtVga[GR17] = hwp->readGr(hwp, 0x17);
    pCir->ModeReg.ExtVga[GR18] = pCir->SavedReg.ExtVga[GR18] = hwp->readGr(hwp, 0x18);
    /* The first 4 reads are for the pixel mask register. After 4 times that
       this register is accessed in succession reading/writing this address
       accesses the HDR. */
    hwp->readDacMask(hwp); hwp->readDacMask(hwp);
    hwp->readDacMask(hwp); hwp->readDacMask(hwp); 
    pCir->ModeReg.ExtVga[HDR ] = pCir->SavedReg.ExtVga[HDR ] = hwp->readDacMask(hwp);
#else
    outb(hwp->IOBase+4, 0x1B); pCir->ModeReg.ExtVga[CR1B] = pCir->SavedReg.ExtVga[CR1B] = inb(hwp->IOBase + 5);
    outb(hwp->IOBase+4, 0x1D); pCir->ModeReg.ExtVga[CR1D] = pCir->SavedReg.ExtVga[CR1D] = inb(hwp->IOBase + 5);
    outb(0x3C4, 0x07);   pCir->ModeReg.ExtVga[SR07] = pCir->SavedReg.ExtVga[SR07] = inb(0x3C5);
    outb(0x3C4, 0x0E);   pCir->ModeReg.ExtVga[SR0E] = pCir->SavedReg.ExtVga[SR0E] = inb(0x3C5);
    outb(0x3C4, 0x12);   pCir->ModeReg.ExtVga[SR12] = pCir->SavedReg.ExtVga[SR12] = inb(0x3C5);
    outb(0x3C4, 0x13);   pCir->ModeReg.ExtVga[SR13] = pCir->SavedReg.ExtVga[SR13] = inb(0x3C5);
    outb(0x3C4, 0x1E);   pCir->ModeReg.ExtVga[SR1E] = pCir->SavedReg.ExtVga[SR1E] = inb(0x3C5);
    outb(0x3CE, 0x17);   pCir->ModeReg.ExtVga[GR17] = pCir->SavedReg.ExtVga[GR17] = inb(0x3CF);
    outb(0x3CE, 0x18);   pCir->ModeReg.ExtVga[GR18] = pCir->SavedReg.ExtVga[GR18] = inb(0x3CF);
    /* The first 4 reads are for the pixel mask register. After 4 times that
       this register is accessed in succession reading/writing this address
       accesses the HDR. */
    inb(0x3C6); inb(0x3C6); inb(0x3C6); inb(0x3C6);
    pCir->ModeReg.ExtVga[HDR ] = pCir->SavedReg.ExtVga[HDR ] = inb(0x3C6);
#endif
}

/* XXX */
static void
CIRFix1bppColorMap(ScrnInfoPtr pScrn) {
   vgaHWPtr hwp = VGAHWPTR(pScrn);
/* In 1 bpp we have color 0 at LUT 0 and color 1 at LUT 0x3f.
   This makes white and black look right (otherwise they were both
   black. I'm sure there's a better way to do that, just lazy to
   search the docs.  */

#if 1
   hwp->writeDacWriteAddr(hwp, 0x00);
   hwp->writeDacData(hwp, 0x00); hwp->writeDacData(hwp, 0x00); hwp->writeDacData(hwp, 0x00);
   hwp->writeDacWriteAddr(hwp, 0x3F);
   hwp->writeDacData(hwp, 0x3F); hwp->writeDacData(hwp, 0x3F); hwp->writeDacData(hwp, 0x3F);
#else
   outb(0x3C8, 0x00); outb(0x3C9, 0x00); outb(0x3C9, 0x00); outb(0x3C9, 0x00);
   outb(0x3C8, 0x3F); outb(0x3C9, 0x3F); outb(0x3C9, 0x3F); outb(0x3C9, 0x3F);
#endif
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
CIRModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    CIRPtr pCir;
    int depthcode;
    int width;
    Bool HDiv2 = FALSE, VDiv2 = FALSE;

#ifdef CIR_DEBUG
    ErrorF("CIRModeInit %d bpp,   %d   %d %d %d %d   %d %d %d %d\n",
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

    ErrorF("CIRModeInit: depth %d bits\n", pScrn->depth);
#endif

    pCir = CIRPTR(pScrn);
    hwp = VGAHWPTR(pScrn);
    vgaHWUnlock(hwp);

    depthcode = pScrn->depth;
    if(pScrn->bitsPerPixel == 32)
       depthcode = 32;

    if ((pCir->Chipset == PCI_CHIP_GD5480 && mode->Clock > 135100) |
        (pCir->Chipset == PCI_CHIP_GD5446 && mode->Clock >  85500)) {
        /* The actual DAC register value is set later. */
        /* The CRTC is clocked at VCLK / 2, so we must half the */
        /* horizontal timings. */
        if (!mode->CrtcHAdjusted) {
            mode->CrtcHDisplay >>= 1;
            mode->CrtcHSyncStart >>= 1;
            mode->CrtcHTotal >>= 1;
            mode->CrtcHSyncEnd >>= 1;
            mode->SynthClock >>= 1;
            mode->CrtcHAdjusted = TRUE;
        }
        depthcode += 64;
        HDiv2 = TRUE;
    }

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

    /* Turn off HW cursor, gamma correction, overscan color protect.  */
    pCir->ModeReg.ExtVga[SR12] = 0;
#if 1
    hwp->writeSeq(hwp, 0x12, pCir->ModeReg.ExtVga[SR12]);
#else
    outw(0x3C4, (pCir->ModeReg.ExtVga[SR12] << 8) | 0x12);
#endif

    if(VDiv2)
        hwp->ModeReg.CRTC[0x17] |= 0x04;

#ifdef CIR_DEBUG
    ErrorF("SynthClock = %d\n", mode->SynthClock);
#endif
    CirrusSetClock(pScrn, mode->SynthClock);

    /* Disable DCLK pin driver, interrupts. */
    pCir->ModeReg.ExtVga[GR17] |= 0x08;
    pCir->ModeReg.ExtVga[GR17] &= ~0x04;
#if 1
    hwp->writeGr(hwp, 0x17, pCir->ModeReg.ExtVga[GR17]);
#else
    outw(0x3CE, (pCir->ModeReg.ExtVga[GR17] << 8) | 0x17);
#endif

    vgaReg = &hwp->ModeReg;

    pCir->ModeReg.ExtVga[HDR] = 0;
    /* Enable linear mode and high-res packed pixel mode */
    pCir->ModeReg.ExtVga[SR07] &= 0xe0;	
#ifdef CIR_DEBUG
    ErrorF("depthcode = %d\n", depthcode);
#endif

    if(pScrn->bitsPerPixel == 1) {
       hwp->IOBase = 0x3B0;
       hwp->ModeReg.MiscOutReg &= ~0x01;
    }
    else {
       hwp->IOBase = 0x3D0;
       hwp->ModeReg.MiscOutReg |= 0x01;
    }
    
    switch(depthcode) {
    case 1:
    case 4:
        pCir->ModeReg.ExtVga[SR07] |= 0x10;	
        break;
    case 8:
        pCir->ModeReg.ExtVga[SR07] |= 0x11;	
        break;
    case 64+8:
        pCir->ModeReg.ExtVga[SR07] |= 0x17;
        break;
    case 15:
        pCir->ModeReg.ExtVga[SR07] |= 0x17;
        pCir->ModeReg.ExtVga[HDR ]  = 0xC0;
        break;
    case 64+15:
        pCir->ModeReg.ExtVga[SR07] |= 0x19;
        pCir->ModeReg.ExtVga[HDR ]  = 0xC0;
        break;
    case 16:
        pCir->ModeReg.ExtVga[SR07] |= 0x17;
        pCir->ModeReg.ExtVga[HDR ]  = 0xC1;
        break;
    case 64+16:
        pCir->ModeReg.ExtVga[SR07] |= 0x19;
        pCir->ModeReg.ExtVga[HDR ]  = 0xC1;
        break;
    case 24:
        pCir->ModeReg.ExtVga[SR07] |= 0x15;
        pCir->ModeReg.ExtVga[HDR ]  = 0xC5;
        break;
    case 32:
        pCir->ModeReg.ExtVga[SR07] |= 0x19;
        pCir->ModeReg.ExtVga[HDR ]  = 0xC5;
        break;
    default:
        ErrorF("X11: Internal error: CIRModeInit: Cannot Initialize display to requested mode\n");
#ifdef CIR_DEBUG
        ErrorF("CIRModeInit returning FALSE on depthcode %d\n", depthcode);
#endif
        return FALSE;
    }
    if(HDiv2)
        pCir->ModeReg.ExtVga[GR18] |= 0x20;
    else
        pCir->ModeReg.ExtVga[GR18] &= ~0x20;

#if 1
    hwp->writeGr(hwp, 0x18, pCir->ModeReg.ExtVga[GR18]);
#else
    outw(0x3CE, (pCir->ModeReg.ExtVga[GR18] << 8) | 0x18);
#endif

#if 1
    hwp->writeMiscOut(hwp, hwp->ModeReg.MiscOutReg);
#else
    outb(0x3C2, hwp->ModeReg.MiscOutReg);
#endif

#if 1
    hwp->writeSeq(hwp, 0x07, pCir->ModeReg.ExtVga[SR07]);
    hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp);
    hwp->writeDacMask(hwp, pCir->ModeReg.ExtVga[HDR ]);
#else
    outw(0x3C4, (pCir->ModeReg.ExtVga[SR07] << 8) | 0x07);
    inb(0x3C6); inb(0x3C6); inb(0x3C6); inb(0x3C6);
    outb(0x3C6, pCir->ModeReg.ExtVga[HDR ]);
#endif

    width = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
    if(pScrn->bitsPerPixel == 1)
       width <<= 2;
    hwp->ModeReg.CRTC[0x13] = width >> 3;
    /* Offset extension (see CR13) */
    pCir->ModeReg.ExtVga[CR1B] &= 0xAF;
    pCir->ModeReg.ExtVga[CR1B] |= (width >> (3+4)) & 0x10;
    pCir->ModeReg.ExtVga[CR1B] |= (width >> (3+3)) & 0x40;
    pCir->ModeReg.ExtVga[CR1B] |= 0x22;

#if 1
    hwp->writeCrtc(hwp, 0x1B, pCir->ModeReg.ExtVga[CR1B]);
#else
    outw(hwp->IOBase + 4, (pCir->ModeReg.ExtVga[CR1B] << 8) | 0x1B);
#endif

    /* Programme the registers */
    vgaHWRestore(pScrn, &hwp->ModeReg, VGA_SR_MODE | VGA_SR_CMAP);

    /* XXX */
    if(pScrn->bitsPerPixel == 1)
       CIRFix1bppColorMap(pScrn);

    vgaHWProtect(pScrn, FALSE);

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
CIRRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    CIRPtr pCir;
    CIRRegPtr cirReg;

#ifdef CIR_DEBUG
    ErrorF("CIRRestore\n");
#endif

    hwp = VGAHWPTR(pScrn);
    pCir = CIRPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    cirReg = &pCir->SavedReg;

    vgaHWProtect(pScrn, TRUE);

#if 1
    hwp->writeCrtc(hwp, 0x1B, cirReg->ExtVga[CR1B]);
    hwp->writeCrtc(hwp, 0x1D, cirReg->ExtVga[CR1D]);
    hwp->writeSeq(hwp, 0x07, cirReg->ExtVga[SR07]);
    hwp->writeSeq(hwp, 0x0E, cirReg->ExtVga[SR0E]);
    hwp->writeSeq(hwp, 0x12, cirReg->ExtVga[SR12]);
    hwp->writeSeq(hwp, 0x13, cirReg->ExtVga[SR13]);
    hwp->writeSeq(hwp, 0x1E, cirReg->ExtVga[SR1E]);
    hwp->writeGr(hwp, 0x17, cirReg->ExtVga[GR17]);
    hwp->writeGr(hwp, 0x18, cirReg->ExtVga[GR18]);
    /* The first 4 reads are for the pixel mask register. After 4 times that
       this register is accessed in succession reading/writing this address
       accesses the HDR. */
    hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp);
    hwp->writeDacMask(hwp, pCir->SavedReg.ExtVga[HDR ]);
#else
    outw(hwp->IOBase + 4, (cirReg->ExtVga[CR1B] << 8) | 0x1B);
    outw(hwp->IOBase + 4, (cirReg->ExtVga[CR1D] << 8) | 0x1D);
    outw(0x3C4, (cirReg->ExtVga[SR07] << 8) | 0x07);
    outw(0x3C4, (cirReg->ExtVga[SR0E] << 8) | 0x0E);
    outw(0x3C4, (cirReg->ExtVga[SR12] << 8) | 0x12);
    outw(0x3C4, (cirReg->ExtVga[SR13] << 8) | 0x13);
    outw(0x3C4, (cirReg->ExtVga[SR1E] << 8) | 0x1E);
    outw(0x3CE, (cirReg->ExtVga[GR17] << 8) | 0x17);
    outw(0x3CE, (cirReg->ExtVga[GR18] << 8) | 0x18);
    /* The first 4 reads are for the pixel mask register. After 4 times that
       this register is accessed in succession reading/writing this address
       accesses the HDR. */
    inb(0x3C6); inb(0x3C6); inb(0x3C6); inb(0x3C6);
    outb(0x3C6, pCir->SavedReg.ExtVga[HDR ]);
#endif

    if (xf86IsPrimaryPci(pCir->PciInfo))
      vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
    else
      vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
    vgaHWProtect(pScrn, FALSE);
}

/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
CIRScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    /* The vgaHW references will disappear one day */
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    CIRPtr pCir;
    int i, ret;
    VisualPtr visual;

#ifdef CIR_DEBUG
    ErrorF("CIRScreenInit\n");
#endif

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);
    pCir = CIRPTR(pScrn);

    /* Map the VGA memory when the primary video */
    if (xf86IsPrimaryPci(pCir->PciInfo)) {
      hwp->MapSize = 0x10000;		/* Standard 64k VGA window */
      if (!vgaHWMapMem(pScrn))
	  return FALSE;
    }

    /* Map the CIR memory and MMIO areas */
    if (!CIRMapMem(pScrn))
	return FALSE;

    if(pCir->UseMMIO) {
      vgaHWSetMmioFuncs(hwp, pCir->IOBase, -0x3C0);
    }

    vgaHWGetIOBase(hwp);

    /* Save the current state */
    CIRSave(pScrn);

    /* Initialise the first mode */
    if (!CIRModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    /* Set the viewport */
    CIRAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
#ifdef CIR_DEBUG
    ErrorF("CIRScreenInit before miSetVisualTypes\n");
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
#ifdef CIR_DEBUG
    ErrorF("CIRScreenInit after miSetVisualTypes\n");
#endif

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    switch (pScrn->bitsPerPixel) {
    case 1:
        ret = xf1bppScreenInit(pScreen, pCir->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
        break;
    case 4:
        ret = xf4bppScreenInit(pScreen, pCir->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
        break;
    case 8:
	ret = cfbScreenInit(pScreen, pCir->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pCir->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pCir->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pCir->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "X11: Internal error: invalid bpp (%d) in CIRScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

#ifdef CIR_DEBUG
    ErrorF("CIRScreenInit after depth dependent init\n");
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

    if(!pCir->NoAccel) { /* Initialize XAA functions */
       if(!(pCir->UseMMIO ? CIRXAAInitMMIO(pScreen) :
	                    CIRXAAInit(pScreen)))
          xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
              "Could not initialize XAA\n");
    }

    if (pCir->HWCursor) { /* Initialize HW cursor layer */
        if(!CIRHWCursorInit(pScreen))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "Hardware cursor initialization failed\n");
    }
    if(!CIRDGAInit(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
            "DGA initialization failed\n");
    }

    if(!CIRI2CInit(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
            "I2C initialization failed\n");
    }

#ifdef CIRPROBEI2C
    CirProbeI2C(pScrn->scrnIndex);
#endif

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
        return FALSE;

    if (pScrn->bitsPerPixel > 1 &&
            pScrn->bitsPerPixel <= 8)
        vgaHWHandleColormaps(pScreen);

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, CIRDisplayPowerManagementSet, 0);
#endif

    /*
     * Wrap the CloseScreen vector and set SaveScreen.
     */
    pScreen->SaveScreen = CIRSaveScreen;
    pCir->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CIRCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}


/* Usually mandatory */
static Bool
CIRSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return CIRModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
CIRAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    int Base, tmp;
    CIRPtr pCir;
    vgaHWPtr hwp;

    pScrn = xf86Screens[scrnIndex];
    hwp = VGAHWPTR(pScrn);
    pCir = CIRPTR(pScrn);

    Base = ((y * pScrn->displayWidth + x) / 8);
    if(pScrn->bitsPerPixel != 1)
        Base *= (pScrn->bitsPerPixel/4);

#ifdef CIR_DEBUG
    ErrorF("CIRAdjustFrame %d %d 0x%x %d %x\n", x, y, flags, Base, Base);
#endif

    if ((Base & ~0x000FFFFF) != 0) {
        ErrorF("X11: Internal error: CIRAdjustFrame: cannot handle overflow\n");
        return;
    }

#if 1
    hwp->writeCrtc(hwp, 0x0C, (Base >> 8) & 0xff);
    hwp->writeCrtc(hwp, 0x0D, Base & 0xff);
    tmp = hwp->readCrtc(hwp, 0x1B);
#else
    outw(hwp->IOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(hwp->IOBase + 4, ((Base & 0x0000FF) << 8) | 0x0D);
    outb(hwp->IOBase + 4, 0x1B);
    tmp = inb(hwp->IOBase + 5);
#endif
    tmp &= 0xF2;
    tmp |= (Base >> 16) & 0x01;
    tmp |= (Base >> 15) & 0x0C;
#if 1
    hwp->writeCrtc(hwp, 0x1B, tmp);
    tmp = hwp->readCrtc(hwp, 0x1D);
#else
    outb(hwp->IOBase + 5, tmp);
    outb(hwp->IOBase + 4, 0x1D);
    tmp = inb(hwp->IOBase + 5);
#endif
    tmp &= 0x7F;
    tmp |= (Base >> 12) & 0x80;
#if 1
    hwp->writeCrtc(hwp, 0x1D, tmp);
#else
    outb(hwp->IOBase + 5, tmp);
#endif
}

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
CIREnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
#ifdef CIR_DEBUG
    ErrorF("CIREnterVT\n");
#endif

    /* Should we re-save the text mode on each VT enter? */
    return CIRModeInit(pScrn, pScrn->currentMode);
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
static void
CIRLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef CIR_DEBUG
    ErrorF("CIRLeaveVT\n");
#endif

    CIRRestore(pScrn);
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
CIRCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CIRPtr pCir = CIRPTR(pScrn);

    CIRRestore(pScrn);
    vgaHWLock(hwp);

    CIRUnmapMem(pScrn);
    if (pCir->AccelInfoRec)
	XAADestroyInfoRec(pCir->AccelInfoRec);
    pCir->AccelInfoRec = NULL;
    if (pCir->CursorInfoRec)
    	xf86DestroyCursorInfoRec(pCir->CursorInfoRec);
    pCir->CursorInfoRec = NULL;
    if (pCir->DGAInfo)
        DGADestroyInfoRec(pCir->DGAInfo);
    pCir->DGAInfo = NULL;

    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = pCir->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any persistent data structures */

/* Optional */
static void
CIRFreeScreen(int scrnIndex, int flags)
{
#ifdef CIR_DEBUG
    ErrorF("CIRFreeScreen\n");
#endif
    /*
     * This only gets called when a screen is being deleted.  It does not
     * get called routinely at the end of a server generation.
     */
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    CIRFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
CIRValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
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
CIRSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    return vgaHWSaveScreen(pScreen, unblank);
}

#if 0
static CARD16
CirrusSetClock(ScrnInfoPtr pScrn, int freq)
{
	int num, den, usemclk;
	CARD8 tmp;
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	CIRPtr pCir = CIRPTR(pScrn);

	ErrorF("CirrusSetClock\n");

	if(!CirrusFindClock(hwp, freq, pCir->MaxClock, &num, &den, &usemclk))
		return 0;

	ErrorF("CirrusSetClock: nom=%x den=%x usemclk=%x\n",
		num, den, usemclk);

	/* Set VCLK3. */
#if 1
	tmp = hwp->readSeq(hwp, 0x0E);
	hwp->writeSeq(hwp, 0x0E, (tmp & 0x80) | num);
	hwp->writeSeq(hwp, 0x1E, den);
#else
	outb(0x3c4, 0x0e);
	tmp = inb(0x3c5);
	outb(0x3c5, (tmp & 0x80) | num);
	outb(0x3c4, 0x1e);
	outb(0x3c5, den);
#endif

	return (num << 8) | den;
}
#endif

/*
 * CIRDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
CIRDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
	unsigned char sr01, gr0e;
	vgaHWPtr hwp;

#ifdef CIR_DEBUG
	ErrorF("CIRDisplayPowerManagementSet\n");
#endif

	hwp = VGAHWPTR(pScrn);

#ifdef CIR_DEBUG
	ErrorF("CIRDisplayPowerManagementSet: %d\n", PowerManagementMode);
#endif

	switch (PowerManagementMode)
	{
	case DPMSModeOn:
	    /* Screen: On; HSync: On, VSync: On */
	    sr01 = 0x00;
	    gr0e = 0x00;
	    break;
	case DPMSModeStandby:
	    /* Screen: Off; HSync: Off, VSync: On */
	    sr01 = 0x20;
	    gr0e = 0x02;
	    break;
	case DPMSModeSuspend:
	    /* Screen: Off; HSync: On, VSync: Off */
	    sr01 = 0x20;
	    gr0e = 0x04;
	    break;
	case DPMSModeOff:
	    /* Screen: Off; HSync: Off, VSync: Off */
	    sr01 = 0x20;
	    gr0e = 0x06;
	    break;
	default:
	    return;
	}

        sr01 |= hwp->readSeq(hwp, 0x01) & ~0x20;
	hwp->writeSeq(hwp, 0x01, sr01);
        gr0e |= hwp->readGr(hwp, 0x0E) & ~0x06;
	hwp->writeGr(hwp, 0x0E, gr0e);
}
#endif

#ifdef CIRPROBEI2C
static void CirProbeI2C(int scrnIndex) {
       int i;
       I2CBusPtr b;

       b = xf86I2CFindBus(scrnIndex, "I2C bus 1");
       if (b == NULL)
               ErrorF("Could not find I2C bus \"%s\"\n", "I2C bus 1");
       else {
               for(i = 2; i < 256; i += 2)
                       if(xf86I2CProbeAddress(b, i))
                               ErrorF("Found device 0x%02x on bus \"%s\"\n",
                                       i, b->BusName);
       }
       b = xf86I2CFindBus(scrnIndex, "I2C bus 2");
       if (b == NULL)
               ErrorF("Could not find I2C bus \"%s\"\n", "I2C bus 2");
       else {
               for(i = 2; i < 256; i += 2)
                       if(xf86I2CProbeAddress(b, i))
                               ErrorF("Found device 0x%02x on bus \"%s\"\n",
                                       i, b->BusName);
       }
}
#endif
