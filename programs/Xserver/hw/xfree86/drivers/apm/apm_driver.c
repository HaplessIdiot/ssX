/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_driver.c,v 1.11 1999/03/21 07:35:03 dawes Exp $ */


#include "apm.h"
#include "xf86cmap.h"

#if 0
#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif
#endif

#ifdef DPMSExtension
#include "opaque.h"
#include "extensions/dpms.h"
#endif

#define VERSION			4000
#define APM_NAME		"APM"
#define APM_DRIVER_NAME		"apm"
#define APM_MAJOR_VERSION       1
#define APM_MINOR_VERSION       0
#define APM_PATCHLEVEL          0
#ifndef PCI_CHIP_AT3D
#define PCI_CHIP_AT3D	0x643D
#endif

/* bytes to save for text/font data */
#define TEXT_AMOUNT 32768

/* Mandatory functions */
static void     ApmIdentify(int flags);
static Bool     ApmProbe(DriverPtr drv, int flags);
static Bool     ApmPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     ApmScreenInit(int Index, ScreenPtr pScreen, int argc,
                                  char **argv);
static Bool     ApmSwitchMode(int scrnIndex, DisplayModePtr mode,
                                  int flags);
static Bool     ApmAdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool     ApmEnterVT(int scrnIndex, int flags);
static void     ApmLeaveVT(int scrnIndex, int flags);
static Bool     ApmCloseScreen(int scrnIndex, ScreenPtr pScreen);
static void     ApmFreeScreen(int scrnIndex, int flags);
static int      ApmValidMode(int scrnIndex, DisplayModePtr mode,
                                 Bool verbose, int flags);
static Bool	ApmSaveScreen(ScreenPtr pScreen, Bool unblank);
static void	ApmUnlock(ApmPtr pApm);
static void	ApmLock(ApmPtr pApm);
static void	ApmRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg,
			    ApmRegPtr ApmReg);
static void	ApmLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
				LOCO *colors, short visualclass);
#ifdef DPMSExtension
static void	ApmDisplayPowerManagementSet(ScrnInfoPtr pScrn,
					     int PowerManagementMode,
					     int flags);
#endif
#if 0
static Bool	ApmDGAInit(ScreenPtr pScreen);
#endif

DriverRec APM = {
	VERSION,
	"Driver for the Alliance chipsets",
	ApmIdentify,
	ApmProbe,
	NULL,
	0
};

enum ApmChipId {
    AP6422	= 0x6422,
    AT24	= 0x6424,
    AT3D	= 0x643D
};

static SymTabRec ApmChipsets[] = {
    { AP6422,	"AP6422"	},
    { AT24,	"AT24"		},
    { AT3D,	"AT3D"		},
    { -1,	NULL		}
};

static PciChipsets ApmPciChipsets[] = {
    { PCI_CHIP_AP6422,	PCI_CHIP_AP6422,	RES_NONE },
    { PCI_CHIP_AT24,	PCI_CHIP_AT24,		RES_NONE },
    { PCI_CHIP_AT3D,	PCI_CHIP_AT3D,		RES_NONE },
    { -1,			-1,		RES_UNDEFINED }
};

#ifdef XFree86LOADER
#endif

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_BURST,
    OPTION_NOACCEL,
    OPTION_NOCLOCKCHIP,
    OPTION_NOLINEAR,
    OPTION_PCI_RETRY
} ApmOpts;

static OptionInfoRec ApmOptions[] =
{
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_BURST, "pci_burst", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOCLOCKCHIP, "NoClockchip", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOLINEAR, "NoLinear", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_RETRY, "PciRetry", OPTV_BOOLEAN,
	{0}, FALSE},
    {-1, NULL, OPTV_NONE,
	{0}, FALSE}
};

#ifdef XFree86LOADER

/*
 * List of symbols from other modules that this module references.  This
 * list is used to tell the loader that it is OK for symbols here to be
 * unresolved providing that it hasn't been told that they haven't been
 * told that they are essential via a call to xf86LoaderReqSymbols() or
 * xf86LoaderReqSymLists().  The purpose is this is to avoid warnings about
 * unresolved symbols that are not required.
 */

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWProtect",
    "vgaHWSetMmioFuncs",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    "vgaHWddc1SetSpeed",
    NULL
};

static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    "cfb8_32ScreenInit",
    "cfb24_32ScreenInit",
    NULL
};

static const char *xf8_32bppSymbols[] = {
    "xf86Overlay8Plus32Init",
    NULL
};

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

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    "xf86DoEDID_DDC2",
    NULL
};

static const char *i2cSymbols[] = {
    "xf86CreateI2CBusRec",
    "xf86I2CBusInit",
    NULL
};

static const char *shadowSymbols[] = {
    "ShadowFBInit",
    NULL
};


static XF86ModuleVersionInfo apmVersRec = {
    "Alliance Promotion",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    APM_MAJOR_VERSION, APM_MINOR_VERSION, APM_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,			/* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0,0,0,0}
};

static MODULESETUPPROTO(apmSetup);

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
XF86ModuleData apmModuleData = { &apmVersRec, apmSetup, NULL };

static pointer
apmSetup(pointer module, pointer opts, int *errmaj, int *errmain)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&APM, module, 0);

	LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols, 
			  xf8_32bppSymbols, ramdacSymbols,
			  ddcSymbols, i2cSymbols, shadowSymbols, NULL);

	return (pointer)1;
    }
    else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}
#endif

static Bool
ApmGetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
	return TRUE;
    pScrn->driverPrivate = xnfcalloc(sizeof(ApmRec), 1);
    /* pScrn->driverPrivate != NULL at this point */

    return TRUE;
}

static void
ApmFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate) {
	xfree(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
    }
}


/* unlock Alliance registers */
static void
ApmUnlock(ApmPtr pApm)
{
    if (!pApm->UnlockCalled) {
	if (!pApm->noLinear) {
	    pApm->savedSR10 = ApmReadSeq(0x10);
	    pApm->xbase = (ApmReadSeq(0x1F) << 8) | ApmReadSeq(0x1E);
	    pApm->UnlockCalled = TRUE;
	}
	else {
	    pApm->savedSR10 = rdinx(0x3C4, 0x10);
	    pApm->xbase = (rdinx(0x3C4, 0x1F) << 8) | rdinx(0x3C4, 0x1E);
	    pApm->UnlockCalled = TRUE;
	}
    }
    if (!pApm->noLinear)
	ApmWriteSeq(0x10, 0x12);
    else
	wrinx(0x3C4, 0x10, 0x12);
}

/* lock Alliance registers */
static void
ApmLock(ApmPtr pApm)
{
    if (pApm->UnlockCalled) {
	if (!pApm->noLinear)
	    ApmWriteSeq(0x10, pApm->savedSR10);
	else
	    wrinx(0x3C4, 0x10, pApm->savedSR10);
    }
}

static void
ApmIdentify(int flags)
{
    xf86PrintChipsets(APM_NAME, "driver for the Alliance chipsets",
		      ApmChipsets);
}

static Bool
ApmProbe(DriverPtr drv, int flags)
{
    int		numDevSections, numUsed, i;
    GDevPtr	*DevSections, *usedDevs;
    int		*usedChips;
    pciVideoPtr	*usedPci, pPci;
    BusResource resource;
    int		foundScreen = FALSE;
    int		master_VGA = FALSE;

    /*
     * Check if there is a chipset override in the config file
     */
    if ((numDevSections = xf86MatchDevice(APM_DRIVER_NAME,
					   &DevSections)) <= 0)
	return FALSE;

    /*
     * We need to probe the hardware first. We then need to see how this
     * fits in with what is given in the config file, and allow the config
     * file info to override any contradictions.
     */

    if (xf86GetPciVideoInfo()) {
	if ((numUsed = xf86MatchPciInstances(APM_NAME, PCI_VENDOR_ALLIANCE,
			ApmChipsets, ApmPciChipsets, DevSections,numDevSections,
			&usedDevs, &usedPci, &usedChips)) > 0) {
	    for (i = 0; i < numUsed; i++) {
		pPci = usedPci[i];
		resource = xf86FindPciResource(usedChips[i], ApmPciChipsets);

		/*
		 * Check that nothing else has claimed the slots.
		 */
		if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func,
				     resource)) {
		    ScrnInfoPtr	pScrn;

		    pScrn = xf86AllocateScreen(drv, 0);
		    if (!xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func,
					  resource, &APM, usedChips[i],
					  pScrn->scrnIndex)) {
			/* This can't happen */
			FatalError("APM: someone claimed the free slot !\n");
		    }

		    /*
		     * Fill in what we can of the ScrnInfoRec
		     */
		    pScrn->driverVersion	= VERSION;
		    pScrn->driverName		= APM_DRIVER_NAME;
		    pScrn->name			= APM_NAME;
		    pScrn->Probe		= ApmProbe;
		    pScrn->PreInit		= ApmPreInit;
		    pScrn->ScreenInit		= ApmScreenInit;
		    pScrn->SwitchMode		= ApmSwitchMode;
		    pScrn->AdjustFrame		= (void(*)(int,int,int,int))ApmAdjustFrame;
		    pScrn->EnterVT		= ApmEnterVT;
		    pScrn->LeaveVT		= ApmLeaveVT;
		    pScrn->FreeScreen		= ApmFreeScreen;
		    pScrn->ValidMode		= ApmValidMode;
		    pScrn->device		= usedDevs[i];
		    foundScreen = TRUE;

		    master_VGA = xf86IsPrimaryPci(pPci);
		}
	    }
	    xfree(usedDevs);
	    xfree(usedPci);
	}
    }
    if (!master_VGA) {
	char save = rdinx(0x3C4, 0x10);

	/*
	 * Start by probing the VGA chipset.
	 */
	outw(0x3C4, 0x1210);
	if (rdinx(0x3C4, 0x11) == 'P' && rdinx(0x3C4, 0x12) == 'r' &&
	    rdinx(0x3C4, 0x13) == 'o') {
	    char	id_ap6420[] = "6420";
	    char	id_ap6422[] = "6422";
	    char	id_at24[]   = "6424";
	    char	id_at3d[]   = "AT3D";
	    char	idstring[]  = "    ";
	    int		apmChip = -1;

	    /*
	     * Must be an Alliance !!!
	     */
	    for (i = 0; i < 4; i++)
		idstring[i] = rdinx(0x3C4, 0x14 + i);
	    if (!memcmp(id_ap6420, idstring, 4) ||
		!memcmp(id_ap6422, idstring, 4))
		apmChip = AP6422;
	    else if (!memcmp(id_at24, idstring, 4))
		apmChip = AT24;
	    else if (!memcmp(id_at3d, idstring, 4))
		apmChip = AT3D;
	    if (apmChip >= 0) {
		int	apm_xbase;

		apm_xbase = (rdinx(0x3C4, 0x1F) << 8) | rdinx(0x3C4, 0x1E);

		if (!(wrinx(0x3c4, 0x1d, 0xCA >> 2), inb(apm_xbase + 2))) {
		    /*
		     * TODO Not PCI
		     */
		}


		if (!xf86CheckIsaSlot(RES_VGA)) {
		    xf86DrvMsg(-1, X_NOTICE, "someone claimed the slot !\n");
		}
		else {
		    ScrnInfoPtr	pScrn;

		    pScrn = xf86AllocateScreen(drv, 0);

		    xf86ClaimIsaSlot(RES_VGA, &APM, apmChip, pScrn->scrnIndex);
		    /*
		     * Fill in what we can of the ScrnInfoRec
		     */
		    pScrn->driverVersion	= VERSION;
		    pScrn->driverName		= APM_DRIVER_NAME;
		    pScrn->name			= APM_NAME;
		    pScrn->Probe		= ApmProbe;
		    pScrn->PreInit		= ApmPreInit;
		    pScrn->ScreenInit		= ApmScreenInit;
		    pScrn->SwitchMode		= ApmSwitchMode;
		    pScrn->AdjustFrame		= (void(*)(int,int,int,int))ApmAdjustFrame;
		    pScrn->EnterVT		= ApmEnterVT;
		    pScrn->LeaveVT		= ApmLeaveVT;
		    pScrn->FreeScreen		= ApmFreeScreen;
		    pScrn->ValidMode		= ApmValidMode;
		    pScrn->device		= usedDevs[i];
		    foundScreen = TRUE;
		}
	    }
	}
	wrinx(0x3C4, 0x10, save);
    }
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
    int linep[] = {640, 800, 1024, 1152, 1280, 1600, 0};

    if (sizeof linep > 0) {
	linePitches = (int *)xnfalloc(sizeof linep);
	memcpy(linePitches, linep, sizeof linep);
    }

    return linePitches;
}

static Bool
ApmPreInit(ScrnInfoPtr pScrn, int flags)
{
    APMDECL(pScrn);
    pciVideoPtr	*pciList = NULL;
    MessageType	from;
    char	*mod = NULL;
    ClockRangePtr	clockRanges;
    int		i;

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

    /* The vgahw module should be allocated here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;

    vgaHWGetIOBase(VGAHWPTR(pScrn));

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     */
    if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb | Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
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

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the ApmRec driverPrivate */
    if (!ApmGetRec(pScrn)) {
	return FALSE;
    }
    pApm = APMPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ApmOptions);

    pApm->scrnIndex = pScrn->scrnIndex;
    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth > 1 && pScrn->depth <= 8) {
	/* Default to 8 */
	pScrn->rgbBits = 8;
    }
    if (xf86ReturnOptValBool(ApmOptions, OPTION_NOLINEAR, FALSE)) {
	pApm->noLinear = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "No linear framebuffer\n");
    }
    from = X_DEFAULT;
    pApm->hwCursor = FALSE;
    if (xf86GetOptValBool(ApmOptions, OPTION_HW_CURSOR, &pApm->hwCursor))
	from = X_CONFIG;
    if (pApm->noLinear ||
	xf86ReturnOptValBool(ApmOptions, OPTION_SW_CURSOR, FALSE)) {
	from = X_CONFIG;
	pApm->hwCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pApm->hwCursor ? "HW" : "SW");
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8)
	pApm->NoAccel = TRUE;
    if (xf86ReturnOptValBool(ApmOptions, OPTION_NOACCEL, FALSE)) {
	from = X_CONFIG;
	pApm->NoAccel = TRUE;
    }
    if (pApm->NoAccel)
	xf86DrvMsg(pScrn->scrnIndex, from, "Acceleration disabled\n");
    if (xf86ReturnOptValBool(ApmOptions, OPTION_PCI_RETRY, FALSE)) {
	if (xf86ReturnOptValBool(ApmOptions, OPTION_PCI_BURST, FALSE)) {
	  pApm->UsePCIRetry = TRUE;
	  xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
	}
	else
	  xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"pci_retry\" option requires pci_burst \"on\".\n");
    }

    /* Find the PCI slot for this screen */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) != 1) {
	/* This shouldn't happen */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Expected one PCI card, but found %d\n", i);
	ApmFreeRec(pScrn);
	return FALSE;
    }

    pApm->PciInfo = *pciList;
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pApm->Chipset = xf86StringToToken(ApmChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pApm->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(ApmChipsets, pApm->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pApm->Chipset);
    } else {
	from = X_PROBED;
	pApm->Chipset = pApm->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(ApmChipsets, pApm->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pApm->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pApm->ChipRev);
    } else {
	pApm->ChipRev = pApm->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * ApmProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pApm->Chipset);
	return FALSE;
    }
    if (pApm->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    pApm->PciTag = pciTag(pApm->PciInfo->bus, pApm->PciInfo->device,
			  pApm->PciInfo->func);

    if (pScrn->device->MemBase != 0) {
	pApm->LinAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	pApm->LinAddress = pApm->PciInfo->memBase[0] & 0xFF800000;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pApm->LinAddress);

    if (pApm->noLinear) {
	pApm->LinMapSize  =  4 * 1024 * 1024 /* 0x10000 */;
	pApm->FbMapSize   =  4 * 1024 * 1024 /* 0x10000 */;
	pApm->LinAddress +=  8 * 1024 * 1024 /* 0xA0000 */;
    }
    else {
	pApm->LinMapSize  = 16 * 1024 * 1024;
	pApm->FbMapSize   =  4 * 1024 * 1024;
    }

    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else if (!pApm->noLinear) {
	unsigned char		d9, db, uc;
	unsigned long		save;
	volatile unsigned char	*LinMap;

	LinMap = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
				     pApm->PciTag, (pointer)pApm->LinAddress,
				     pApm->LinMapSize);
	save = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save | PCI_CMD_MEM_ENABLE);
	d9 = LinMap[0xFFECD9];
	db = LinMap[0xFFECDB];
	LinMap[0xFFECDB] = (db & 0xF0) | 0x0A;
	LinMap[0xFFECD9] = (d9 & 0xCF) | 0x20;
	LinMap[0xFFF3C4] = 0x1C;
	uc = LinMap[0xFFF3C5];
	LinMap[0xFFF3C5] = 0x3F;
	LinMap[0xFFF3C4] = 0x20;
	pScrn->videoRam = LinMap[0xFFF3C5] * 64;
	LinMap[0xFFF3C4] = 0x1C;
	LinMap[0xFFF3C5] = uc;
	LinMap[0xFFECDB] = db;
	LinMap[0xFFECD9] = d9;
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save);
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)LinMap, pApm->LinMapSize);
    }
    else {
	unsigned long		save;

	save = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save | PCI_CMD_IO_ENABLE);
	pScrn->videoRam = rdinx(0x3C4, 0x20) * 64;
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save);
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    pApm->MinClock = 23125;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock set to %d MHz\n",
	       pApm->MinClock / 1000);

    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->dacSpeeds[0]) {
	int speed = 0;

	switch (pScrn->bitsPerPixel) {
	case 4:
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
	    pApm->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pApm->MaxClock = speed;
	from = X_CONFIG;
    } else {
	switch(pApm->Chipset)
	{
	  /* These values come from the Manual for AT24 and AT3D 
	     in the overview of various modes. I've taken the largest
	     number for the different modes. Alliance wouldn't 
	     tell me what the maximum frequency was, so...
	   */
	  case AT24:
	       switch(pScrn->bitsPerPixel)
	       {
		 case 4:
		 case 8:
		      pApm->MaxClock = 160000;
		      break;
		 case 16:
		      pApm->MaxClock = 144000;
		      break;
		 case 24:
		      pApm->MaxClock = 75000; /* Hmm. */
		      break;
		 case 32:
		      pApm->MaxClock = 94500;
		      break;
		 default:
		      return FALSE;
	       }
	       break;
	  case AT3D:
	       switch(pScrn->bitsPerPixel)
	       {
		 case 4:
		 case 8:
		      pApm->MaxClock = 175500;
		      break;
		 case 16:
		      pApm->MaxClock = 144000;
		      break;
		 case 24:
		      pApm->MaxClock = 94000;
		      break;
		 case 32:
		      pApm->MaxClock = 94500;
		      break;
		 default:
		      return FALSE;
	       }
	       break;
	  default:
	       pApm->MaxClock = 135000;
	       break;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pApm->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pApm->MinClock;
    clockRanges->maxClock = pApm->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX change this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /* Select valid modes from those available */
    if (pApm->NoAccel) {
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
			      pApm->FbMapSize,
			      LOOKUP_BEST_REFRESH);
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
			      pApm->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }

    if (i == -1) {
	ApmFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	ApmFreeRec(pScrn);
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
    case 4:
	mod = "xf4bpp";
	break;
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
	ApmFreeRec(pScrn);
	return FALSE;
    }

    /* Load XAA if needed */
    if (!pApm->NoAccel)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    ApmFreeRec(pScrn);
	    return FALSE;
	}

    return TRUE;
}

/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
ApmMapMem(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

    pApm->saveCmd = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG,
		 pApm->saveCmd & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    pApm->LinMap = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pApm->PciTag,
				 (pointer)((unsigned long)pApm->LinAddress),
				 pApm->LinMapSize);
    if (pApm->LinMap == NULL)
	return FALSE;

    if (!pApm->noLinear) {
	pApm->FbBase = (void *)(((char *)pApm->LinMap) + 0x800000);
	pApm->VGAMap = ((char *)pApm->LinMap) + 0xFFF000;
	pApm->MemMap = ((char *)pApm->LinMap) + 0xFFEC00;
	pApm->BltMap = (void *)(((char *)pApm->LinMap) + 0x3F8000);

	/* Re-enable memory */
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG,
		     pApm->saveCmd | PCI_CMD_MEM_ENABLE);

	/*
	 * Initialize chipset
	 */
	pApm->d9 = RDXB(0xD9);
	pApm->db = RDXB(0xDB);
	WRXB(0xDB, (pApm->db & 0xF0) | 0x0A);
	WRXB(0xD9, (pApm->d9 & 0xCF) | 0x20);
	vgaHWSetMmioFuncs(VGAHWPTR(pScrn), (CARD8 *)pApm->LinMap, 0xFFF000);
    }
    else {
	pApm->FbBase = pApm->LinMap;

	/* Re-enable memory  and I/O */
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG,
		     pApm->saveCmd | PCI_CMD_MEM_ENABLE | PCI_CMD_IO_ENABLE);

	/*
	 * Initialize chipset
	 */
	pApm->d9 = RDXB_IOP(0xD9);
	pApm->db = RDXB_IOP(0xDB);
	WRXB_IOP(0xDB, (pApm->db & 0xF0) | 0x08);
    }

    return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory
 */

static Bool
ApmUnmapMem(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);

    if (pApm->LinMap) {
	if (!pApm->noLinear) {
	    WRXB(0xD9, pApm->d9);
	    WRXB(0xDB, pApm->db);
	}
	else {
	    WRXB_IOP(0xD9, pApm->d9);
	    WRXB_IOP(0xDB, pApm->db);
	}
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pApm->LinMap, pApm->LinMapSize);
	pApm->LinMap = NULL;
    }
    else if (pApm->FbBase)
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pApm->LinMap, 0x10000);
    pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, pApm->saveCmd);

    return TRUE;
}

/*
 * This function saves the video state.
 */
static void
ApmSave(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);
    ApmRegPtr	ApmReg = &pApm->SavedReg;
    vgaHWPtr	vgaHWP = VGAHWPTR(pScrn);

    if (!pApm->noLinear) {
	ApmReg->SEQ[0x1B] = ApmReadSeq(0x1B);
	ApmReg->SEQ[0x1C] = ApmReadSeq(0x1C);

	/*
	 * Save fonts
	 */
	if (!(vgaHWP->SavedReg.Attribute[0x10] & 1)) {
	    if (pApm->FontInfo || (pApm->FontInfo = (pointer)xalloc(TEXT_AMOUNT))) {
		ApmWriteSeq(0x1C, 0x3F);
		memcpy(pApm->FontInfo, pApm->FbBase, TEXT_AMOUNT);
		ApmWriteSeq(0x1C, ApmReg->SEQ[0x1C]);
	    }
	}
	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	vgaHWSave(pScrn, &vgaHWP->SavedReg, VGA_SR_MODE | VGA_SR_CMAP);

	/* Hardware cursor registers. */
	ApmReg->EX[XR140] = RDXL(0x140);
	ApmReg->EX[XR144] = RDXW(0x144);
	ApmReg->EX[XR148] = RDXL(0x148);
	ApmReg->EX[XR14C] = RDXW(0x14C);

	ApmReg->CRT[0x19] = ApmReadCrtc(0x19);
	ApmReg->CRT[0x1A] = ApmReadCrtc(0x1A);
	ApmReg->CRT[0x1B] = ApmReadCrtc(0x1B);
	ApmReg->CRT[0x1C] = ApmReadCrtc(0x1C);
	ApmReg->CRT[0x1D] = ApmReadCrtc(0x1D);
	ApmReg->CRT[0x1E] = ApmReadCrtc(0x1E);

	/* RAMDAC registers. */
	ApmReg->EX[XRE8] = RDXL(0xE8);
	ApmReg->EX[XREC] = RDXL(0xEC);

	/* Color correction */
	ApmReg->EX[XRE0] = RDXL(0xE0);

	ApmReg->EX[XR80] = RDXB(0x80);
    }
    else {
	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	vgaHWSave(pScrn, &vgaHWP->SavedReg, VGA_SR_ALL);

	ApmReg->SEQ[0x1B] = rdinx(0x3C4, 0x1B);
	ApmReg->SEQ[0x1C] = rdinx(0x3C4, 0x1C);

	/* Hardware cursor registers. */
	ApmReg->EX[XR140] = RDXL_IOP(0x140);
	ApmReg->EX[XR144] = RDXW_IOP(0x144);
	ApmReg->EX[XR148] = RDXL_IOP(0x148);
	ApmReg->EX[XR14C] = RDXW_IOP(0x14C);

	ApmReg->CRT[0x19] = rdinx(0x3D4, 0x19);
	ApmReg->CRT[0x1A] = rdinx(0x3D4, 0x1A);
	ApmReg->CRT[0x1B] = rdinx(0x3D4, 0x1B);
	ApmReg->CRT[0x1C] = rdinx(0x3D4, 0x1C);
	ApmReg->CRT[0x1D] = rdinx(0x3D4, 0x1D);
	ApmReg->CRT[0x1E] = rdinx(0x3D4, 0x1E);

	/* RAMDAC registers. */
	ApmReg->EX[XRE8] = RDXL_IOP(0xE8);
	ApmReg->EX[XREC] = RDXL_IOP(0xEC);

	/* Color correction */
	ApmReg->EX[XRE0] = RDXL_IOP(0xE0);

	ApmReg->EX[XR80] = RDXB_IOP(0x80);
    }
}

#define WITHIN(v,c1,c2) (((v) >= (c1)) && ((v) <= (c2)))

static unsigned
comp_lmn(ApmPtr pApm, long clock)
{
  int     n, m, l, f;
  double  fvco;
  double  fout;
  double  fmax;
  double  fref;
  double  fvco_goal;
  double  k, c;

  if (pApm->Chipset >= AT3D)
    fmax = 400000.0;
  else
    fmax = 250000.0;

  fref = 14318.0;

  for (m = 1; m <= 5; m++)
  {
    for (l = 3; l >= 0; l--)
    {
      for (n = 8; n <= 127; n++)
      {
        fout = ((double)(n + 1) * fref)/((double)(m + 1) * (1 << l));
        fvco_goal = (double)clock * (double)(1 << l);
        fvco = fout * (double)(1 << l);
        if (!WITHIN(fvco, 0.995*fvco_goal, 1.005*fvco_goal))
          continue;
        if (!WITHIN(fvco, 125000.0, fmax))
          continue;
        if (!WITHIN(fvco / (double)(n+1), 300.0, 300000.0))
          continue;
        if (!WITHIN(fref / (double)(m+1), 300.0, 300000.0))
          continue;

        /* The following formula was empirically derived by
           matching a number of fvco values with acceptable
           values of f.

           (fvco can be 125MHz - 400MHz on AT3D)
           (fvco can be 125MHz - 250MHz on AT24/AP6422)

           The table that was measured up follows:

           AT3D

           fvco       f
           (125)     (x-7) guess
           200       5-7
           219       4-7
           253       3-6
           289       2-5
           320       0-4
           (400)     (0-x) guess

           AT24

           fvco       f
           126       7
           200       5-7
           211       4-7

           AP6422

           fvco       f
           126       7
           169       5-7
           200       4-5
           211       4-5

           From this, a function "f = k * fvco + c" was derived.

           For AT3D, this table was measured with MCLK == 50MHz.
           The driver has since been set to use MCLK == 57.3MHz for,
           but I don't think that makes a difference here.
         */

        if (pApm->Chipset >= AT24)
        {
          k = 7.0 / (175.0 - 380.0);
          c = -k * 380.0;
          f = (int)(k * fvco/1000.0 + c + 0.5);
          if (f > 7) f = 7;
          if (f < 0) f = 0;
        }

        if (pApm->Chipset < AT24) /* i.e AP6422 */
        {
          c = (211.0*6.0-169.0*4.5)/(211.0-169.0);
          k = (4.5-c)/211.0;
          f = (int)(k * fvco/1000.0 + c + 0.5);
          if (f > 7) f = 7;
          if (f < 0) f = 0;
        }

        return (n << 16) | (m << 8) | (l << 2) | (f << 4);
      }
    }
  }
  xf86DrvMsg(pApm->scrnIndex, X_PROBED,
		"Cannot find register values for clock %6.2f MHz. "
		"Please use a (slightly) different clock.\n",
		 (double)clock / 1000.0);
  return 0;
}

/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
ApmModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    APMDECL(pScrn);
    ApmRegPtr	ApmReg = &pApm->ModeReg;
    vgaHWPtr	hwp;

    /* set clockIndex to "2" for programmable clocks */
    if (pScrn->progClock)
	mode->ClockIndex = 2;

    /* prepare standard VGA register contents */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;
    hwp = VGAHWPTR(pScrn);

    memcpy(ApmReg, &pApm->SavedReg, sizeof pApm->SavedReg);

    /*
     * The APM chips have a scale factor of 8 for the
     * scanline offset. There are four extended bit in addition
     * to the 8 VGA bits.
     */
    {
	int offset;

	offset = (pScrn->displayWidth *
		  pScrn->bitsPerPixel / 8)	>> 3;
	hwp->ModeReg.CRTC[0x13] = offset;
	/* Bit 8 resides at CR1C bits 7:4. */
	ApmReg->CRT[0x1C] = (offset & 0xf00) >> 4;
    }

    /* Set pixel depth. */
    switch(pScrn->bitsPerPixel)
    {
    case 4:
	 ApmReg->EX[XR80] = 0x01;
	 break;
    case 8:
	 ApmReg->EX[XR80] = 0x02;
	 break;
    case 15:
	 ApmReg->EX[XR80] = 0x0C;
	 break;
    case 16:
	 ApmReg->EX[XR80] = 0x0D;
	 break;
    case 24:
	 ApmReg->EX[XR80] = 0x0E;
	 break;
    case 32:
	 ApmReg->EX[XR80] = 0x0F;
	 break;
    default:
	 FatalError("Unsupported bit depth %d\n", pScrn->bitsPerPixel);
	 break;
    }

    /* Set banking register to zero. */
    ApmReg->EX[XRC0] = 0;

    /* Handle the CRTC overflow bits. */
    {
	unsigned char val;
	/* Vertical Overflow. */
	val = 0;
	if ((mode->CrtcVTotal - 2) & 0x400)
	    val |= 0x01;
	if ((mode->CrtcVDisplay - 1) & 0x400)
	    val |= 0x02;
	/* VBlankStart is equal to VSyncStart + 1. */
	if (mode->CrtcVSyncStart & 0x400)
	    val |= 0x04;
	/* VRetraceStart is equal to VSyncStart + 1. */
	if (mode->CrtcVSyncStart & 0x400)
	    val |= 0x08;
	ApmReg->CRT[0x1A] = val;

	/* Horizontal Overflow. */
	val = 0;
	if ((mode->CrtcHTotal / 8 - 5) & 0x100)
	    val |= 1;
	if ((mode->CrtcHDisplay / 8 - 1) & 0x100)
	    val |= 2;
	/* HBlankStart is equal to HSyncStart - 1. */
	if ((mode->CrtcHSyncStart / 8 - 1) & 0x100)
	    val |= 4;
	/* HRetraceStart is equal to HSyncStart. */
	if ((mode->CrtcHSyncStart / 8) & 0x100)
	    val |= 8;
	ApmReg->CRT[0x1B] = val;
    }
    ApmReg->CRT[0x1E] = 1;          /* disable autoreset feature */

    /* Program clock select. */
    ApmReg->EX[XREC] = comp_lmn(pApm, mode->Clock);
    if (!ApmReg->EX[XREC])
      return FALSE;
    hwp->ModeReg.MiscOutReg |= 0x0C;

    /* Set up the RAMDAC registers. */

    if (pScrn->bitsPerPixel > 8)
	/* Get rid of white border. */
	hwp->ModeReg.Attribute[0x11] = 0x00;
    else
	hwp->ModeReg.Attribute[0x11] = 0xFF;
    if (pApm->Chipset >= AT3D)
	ApmReg->EX[XRE8] = 0x071F01E8; /* Enable 58MHz MCLK (actually 57.3 MHz)
				       This is what is used in the Windows
				       drivers. The BIOS sets it to 50MHz. */
    else if (!pApm->noLinear)
	ApmReg->EX[XRE8] = RDXL(0xE8); /* No change */
    else
	ApmReg->EX[XRE8] = RDXL_IOP(0xE8); /* No change */

    ApmReg->EX[XRE0] = 0x10;

    ApmReg->SEQ[0x1B] = 0x20;
    ApmReg->SEQ[0x1C] = 0x3F;

    /* ICICICICI */
    ApmRestore(pScrn, &hwp->ModeReg, ApmReg);

    return TRUE;
}

/*
 * Restore the initial mode.
 */
static void
ApmRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, ApmRegPtr ApmReg)
{
    APMDECL(pScrn);

    vgaHWProtect(pScrn, TRUE);
    ApmUnlock(pApm);

    /* Set aperture index to 0. */
    if (pApm->LinMap) {
	/*
	 * Restore fonts
	 */
	if (!(vgaReg->Attribute[0x10] & 1) && pApm->FontInfo) {
	    ApmWriteSeq(0x1C, 0x3F);
	    memcpy(pApm->FbBase, pApm->FontInfo, TEXT_AMOUNT);
	}

	WRXW(0xC0, 0);

	/*
	 * Write the extended registers first
	 */
	ApmWriteSeq(0x1B, ApmReg->SEQ[0x1B]);
	ApmWriteSeq(0x1C, ApmReg->SEQ[0x1C]);

	/* Hardware cursor registers. */
	WRXL(0x140, ApmReg->EX[XR140]);
	WRXW(0x144, ApmReg->EX[XR144]);
	WRXL(0x148, ApmReg->EX[XR148]);
	WRXW(0x14C, ApmReg->EX[XR14C]);

	ApmWriteCrtc(0x19, ApmReg->CRT[0x19]);
	ApmWriteCrtc(0x1A, ApmReg->CRT[0x1A]);
	ApmWriteCrtc(0x1B, ApmReg->CRT[0x1B]);
	ApmWriteCrtc(0x1C, ApmReg->CRT[0x1C]);
	ApmWriteCrtc(0x1D, ApmReg->CRT[0x1D]);
	ApmWriteCrtc(0x1E, ApmReg->CRT[0x1E]);

	/* RAMDAC registers. */
	WRXL(0xE8, ApmReg->EX[XRE8]);
	WRXL(0xEC, ApmReg->EX[XREC] & ~(1 << 7));
	WRXL(0xEC, ApmReg->EX[XREC] | (1 << 7)); /* Do a PLL resync */

	/* Color correction */
	WRXL(0xE0, ApmReg->EX[XRE0]);

	WRXB(0x80, ApmReg->EX[XR80]);

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_CMAP);
    }
    else {
	WRXW_IOP(0xC0, 0);

	/*
	 * Write the extended registers first
	 */
	wrinx(0x3C4, 0x1B, ApmReg->SEQ[0x1B]);
	wrinx(0x3C4, 0x1C, ApmReg->SEQ[0x1C]);

	/* Hardware cursor registers. */
	WRXL_IOP(0x140, ApmReg->EX[XR140]);
	WRXW_IOP(0x144, ApmReg->EX[XR144]);
	WRXL_IOP(0x148, ApmReg->EX[XR148]);
	WRXW_IOP(0x14C, ApmReg->EX[XR14C]);

	wrinx(0x3D4, 0x19, ApmReg->CRT[0x19]);
	wrinx(0x3D4, 0x1A, ApmReg->CRT[0x1A]);
	wrinx(0x3D4, 0x1B, ApmReg->CRT[0x1B]);
	wrinx(0x3D4, 0x1C, ApmReg->CRT[0x1C]);
	wrinx(0x3D4, 0x1D, ApmReg->CRT[0x1D]);
	wrinx(0x3D4, 0x1E, ApmReg->CRT[0x1E]);

	/* RAMDAC registers. */
	WRXL_IOP(0xE8, ApmReg->EX[XRE8]);
	WRXL_IOP(0xEC, ApmReg->EX[XREC] & ~(1 << 7));
	WRXL_IOP(0xEC, ApmReg->EX[XREC] | (1 << 7)); /* Do a PLL resync */

	/* Color correction */
	WRXL_IOP(0xE0, ApmReg->EX[XRE0]);

	WRXB_IOP(0x80, ApmReg->EX[XR80]);

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
    }

    vgaHWProtect(pScrn, FALSE);
}

/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
ApmScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr	pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    int ret;

    /* Map the chip memory and MMIO areas */
    if (pApm->noLinear) {
	pApm->saveCmd = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, pApm->saveCmd | (PCI_CMD_IO_ENABLE|PCI_CMD_MEM_ENABLE));
	pApm->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pApm->PciTag, (pointer)0xA0000, 0x10000);
    }
    else
	if (!ApmMapMem(pScrn))
	    return FALSE;

    /* No memory reserved yet */
    pApm->OffscreenReserved = 0;

    /* Save the current state */
    ApmSave(pScrn);

    /* Initialise the first mode */
    ApmModeInit(pScrn, pScrn->currentMode);

    /* Darken the screen for aesthetic reasons and set the viewport */
    ApmSaveScreen(pScreen, FALSE);
    ApmAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
    case 1:
	ret = xf1bppScreenInit(pScreen, pApm->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, pApm->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 8:
	ret = cfbScreenInit(pScreen, pApm->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pApm->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pApm->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pApm->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
	    "Internal error: invalid bpp (%d) in ApmScrnInit\n",
	    pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

    miInitializeBackingStore(pScreen);

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel > 8) {
	VisualPtr	visual;

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

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    if (pApm->hwCursor) { /* Initialize HW cursor layer */
	if (!ApmHWCursorInit(pScreen))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "Hardware cursor initialization failed\n");
    }

#if 0
    if(!ApmDGAInit(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "DGA initialization failed\n");
    }
#endif

    if (!ApmI2CInit(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "I2C initialization failed\n");
    }
    else
	xf86PrintEDID(xf86DoEDID_DDC2(pScrn->scrnIndex,pApm->I2CPtr));

    /*
     * Initialize the acceleration interface.
     */
    if (!pApm->NoAccel) {
	if (!ApmAccelInit(pScreen)) {	/* set up XAA interface */
	    return FALSE;
	}
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    /*
     * Initialize colormap layer.
     * Must follow initialization of the default colormap.
     */
    if (!xf86HandleColormaps(pScreen, 256, 8, ApmLoadPalette, NULL,
				CMAP_RELOAD_ON_MODE_SWITCH)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Colormap initialization failed\n");
	return FALSE;
    }

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, ApmDisplayPowerManagementSet, 0);
#endif

    pScreen->SaveScreen  = ApmSaveScreen;

    pApm->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = ApmCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}

/* mandatory */
static void
ApmLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
	       short visualclass)
{
    APMDECL(pScrn);
    int i, index, last = -1;

    for (i = 0; i < numColors; i++) {
	index = indices[i];
	if (index != last) 
	    ApmWriteDacWriteAddr(index);
	last = index + 1;
	ApmWriteDacData(colors[index].red);
	ApmWriteDacData(colors[index].green);
	ApmWriteDacData(colors[index].blue);
    }
}

/* Usually mandatory */
static Bool
ApmSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return ApmModeInit(xf86Screens[scrnIndex], mode);
}

/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static Bool
ApmAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);
    int Base;

    if (pScrn->bitsPerPixel == 24)
	x = (x + 3) & ~3;
    Base = ((y * pScrn->displayWidth + x) * (pScrn->bitsPerPixel / 8)) >> 2;
    /*
     * These are the generic starting address registers.
     */
    if (pApm->VGAMap) {
	ApmWriteCrtc(0x0C, Base >> 8);
	ApmWriteCrtc(0x0D, Base);

	/*
	 * Here the high-order bits are masked and shifted, and put into
	 * the appropriate extended registers.
	 */
	ApmWriteCrtc(0x1C, (ApmReadCrtc(0x1C) & 0xF0) | ((Base & 0x0F0000) >> 16));
    }
    else {
	outw(0x3D4, (Base & 0x00FF00) | 0x0C);
	outw(0x3D4, ((Base & 0x00FF) << 8) | 0x0D);

	/*
	 * Here the high-order bits are masked and shifted, and put into
	 * the appropriate extended registers.
	 */
	modinx(0x3D4, 0x1C, 0x0F, (Base & 0x0F0000) >> 16);
    }
    return TRUE;
}

#if 0
static Bool
ApmDGAGetParams(int scrnIndex, unsigned long *offset,
		int *banksize, int *memsize)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);

    *offset = (unsigned long)pApm->FbBase;
    *banksize = pApm->FbMapSize;
    *memsize = pScrn->device->videoRam * 1024;
    return TRUE;
}

static Bool
ApmDGASetDirect(int scrnIndex, Bool enable)
{
    return TRUE;
}

static Bool
ApmDGASetBank(int scrnIndex, int bank, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);
    int Base = (bank * pApm->FbMapSize) / 4096;

    if (!pApm->noLinear)
	WRXW(0xC0, Base);
    else
	WRXW_IOP(0xC0, Base);
    return TRUE;
}

static  Bool
ApmDGAViewportChanged(int scrnIndex, int n, int flags)
{
    return TRUE;
}

static Bool
ApmDGAInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    DGAInfoPtr pDGAInfo;

    pDGAInfo = DGACreateInfoRec();
    if(pDGAInfo == NULL)
        return FALSE;

    pApm->DGAInfo = pDGAInfo;

    pDGAInfo->GetParams = ApmDGAGetParams;
    pDGAInfo->SetDirectMode = ApmDGASetDirect;
    pDGAInfo->SetBank = ApmDGASetBank;
    pDGAInfo->SetViewport = ApmAdjustFrame;
    pDGAInfo->ViewportChanged = ApmDGAViewportChanged;;

    return DGAInit(pScreen, pDGAInfo, 0);
}
#endif

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
ApmEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    vgaHWUnlock(VGAHWPTR(pScrn));
    ApmUnlock(APMPTR(pScrn));
    /* Should we re-save the text mode on each VT enter? */
    if (!ApmModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    return TRUE;
}

/* Mandatory */
static void
ApmLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);

    ApmRestore(pScrn, &VGAHWPTR(pScrn)->SavedReg, &pApm->SavedReg);
    ApmLock(pApm);
    vgaHWLock(VGAHWPTR(pScrn));
}

/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
ApmCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);

    ApmRestore(pScrn, &VGAHWPTR(pScrn)->SavedReg, &pApm->SavedReg);
    if(pApm->AccelInfoRec)
	XAADestroyInfoRec(pApm->AccelInfoRec);
    pApm->AccelInfoRec = NULL;
    if(pApm->CursorInfoRec)
	xf86DestroyCursorInfoRec(pApm->CursorInfoRec);
    pApm->CursorInfoRec = NULL;
#if 0
    if (pApm->DGAInfo)
        DGADestroyInfoRec(pApm->DGAInfo);
    pApm->DGAInfo = NULL;
#endif
    if (pApm->I2CPtr)
	xf86DestroyI2CBusRec(pApm->I2CPtr, TRUE, TRUE);
    pApm->I2CPtr = NULL;

    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = pApm->CloseScreen;
    ApmUnmapMem(pScrn);
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Free up any per-generation data structures */

/* Optional */
static void
ApmFreeScreen(int scrnIndex, int flags)
{
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    ApmFreeRec(xf86Screens[scrnIndex]);
}

/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
ApmValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    if (mode->Flags & V_INTERLACE)
	return(MODE_BAD);

    return(MODE_OK);
}


/*
 * ApmDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
ApmDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
    APMDECL(pScrn);
    unsigned char dpmsreg, tmp;

    switch (PowerManagementMode)
    {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	dpmsreg = 0x00;
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	dpmsreg = 0x01;
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	dpmsreg = 0x02;
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	dpmsreg = 0x03;
	break;
    default:
	dpmsreg = 0;
    }
    tmp = RDXB_IOP(0xD0);
    if (pApm->noLinear)
	WRXB_IOP(0xD0, (tmp & 0xFC) | dpmsreg);
    else
	WRXB(0xD0, (tmp & 0xFC) | dpmsreg);
}
#endif

static Bool
ApmSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

   if (unblank)
      SetTimeSinceLastInputEvent();

   if (pScrn->vtSema)
       vgaHWBlankScreen(pScrn, unblank);
   return TRUE;
}

unsigned char _L_ACR(unsigned char *x);
unsigned char _L_ACR(unsigned char *x)
{
    return *x;
}

unsigned short _L_ASR(unsigned short *x);
unsigned short _L_ASR(unsigned short *x)
{
    return *x;
}

unsigned int _L_AIR(unsigned int *x);
unsigned int _L_AIR(unsigned int *x)
{
    return *x;
}

void _L_ACW(char *x, char y);
void _L_ACW(char *x, char y)
{
    *x = y;
}

void _L_ASW(short *x, short y);
void _L_ASW(short *x, short y)
{
    *x = y;
}

void _L_AIW(int *x, int y);
void _L_AIW(int *x, int y)
{
    *x = y;
}
