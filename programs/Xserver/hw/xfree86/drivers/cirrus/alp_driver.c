/*
 * Driver for CL-GD5480.
 * Itai Nahshon.
 *
 * This is mainly a cut & paste from the MGA driver.
 * Original autors and contributors list include:
 *    Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *    David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *    Guy DESBIEF
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/alp_driver.c,v 1.44 1999/12/03 19:17:32 eich Exp $ */

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

#include "xf86RAC.h"

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
#include "cfb24_32.h"

/* These need to be checked */
#if 0
#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif
#endif

#include "xf86DDC.h"

#include "cir.h"
#include "alp.h"

#ifdef XvExtension
#include "xf86xv.h"
#include "Xv.h"
#endif

#ifdef ALPPROBEI2C
/* For debugging... should go away. */
static void AlpProbeI2C(int scrnIndex);
#endif

/*
 * Forward definitions for the functions that make up the driver.
 */

Bool AlpProbe(int entity, ScrnInfoPtr pScrn);

/* Mandatory functions */

Bool AlpPreInit(ScrnInfoPtr pScrn, int flags);
Bool AlpScreenInit(int Index, ScreenPtr pScreen, int argc, char **argv);
Bool AlpEnterVT(int scrnIndex, int flags);
void AlpLeaveVT(int scrnIndex, int flags);
static Bool	AlpCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	AlpSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
Bool AlpSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
void AlpAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
void AlpFreeScreen(int scrnIndex, int flags);
int	AlpValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags);
/* Internally used functions */
static void	AlpSave(ScrnInfoPtr pScrn);
static void	AlpRestore(ScrnInfoPtr pScrn);
static Bool	AlpModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

static void AlpSetClock(CirPtr pCir, vgaHWPtr hwp, int freq);

#ifdef DPMSExtension
static void	AlpDisplayPowerManagementSet(ScrnInfoPtr pScrn,
											int PowerManagementMode, int flags);
#endif

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;

typedef enum {
	OPTION_SW_CURSOR,
	OPTION_HW_CURSOR,
	OPTION_PCI_RETRY,
	OPTION_NOACCEL,
	OPTION_MMIO,
	OPTION_MEMCFG1,
	OPTION_MEMCFG2
} CirOpts;

static OptionInfoRec CirOptions[] = {
	{ OPTION_HW_CURSOR,	"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_MEMCFG1,	"MemCFG1",	OPTV_INTEGER,	{0}, -1 },
	{ OPTION_MEMCFG2,	"MemCFG2",	OPTV_INTEGER,	{0}, -1 },
	{ -1,				NULL,		OPTV_NONE,		{0}, FALSE }
};

/*                                1/4bpp   8bpp   15/16bpp  24bpp  32bpp
static int unsupp_MaxClocks[] = {      0,      0,      0,      0,      0 }; */
static int gd5430_MaxClocks[] = {  85500,  85500,  50000,  28500,      0 };
static int gd5446_MaxClocks[] = { 135100, 135100,  85500,  85500,      0 };
static int gd5480_MaxClocks[] = { 135100, 200000, 200000, 135100, 135100 };

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
	"xf1bppScreenInit",
	"xf4bppScreenInit",
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

#define ALPuseI2C 0

static const char *ddcSymbols[] = {
	"xf86PrintEDID",
	"xf86DoEDID_DDC1",
#if ALPuseI2C
	"xf86DoEDID_DDC2",
#endif
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


#ifdef XFree86LOADER

#define ALP_MAJOR_VERSION 1
#define ALP_MINOR_VERSION 0
#define ALP_PATCHLEVEL 0

static MODULESETUPPROTO(alpSetup);

static XF86ModuleVersionInfo alpVersRec =
{
	"cirrus_alpine",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	ALP_MAJOR_VERSION, ALP_MINOR_VERSION, ALP_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_NONE,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
XF86ModuleData cirrus_alpineModuleData = { &alpVersRec, alpSetup, NULL };

static pointer
alpSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	/* This module should be loaded only once, but check to be sure. */

	if (!setupDone) {
		setupDone = TRUE;

		/*
		 * Modules that this driver always requires may be loaded here
		 * by calling LoadSubModule().
		 *
		 * Although this driver currently always requires the vgahw module
		 * that dependency will be removed later, so we don't load it here.
		 */

		/*
		 * Tell the loader about symbols from other modules that this module
		 * might refer to.
		 */
		LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols,
							xf8_32bppSymbols, ramdacSymbols,
							ddcSymbols, i2cSymbols, shadowSymbols, NULL);

		/*
		 * The return value must be non-NULL on success even though there
		 * is no TearDownProc.
		 */
		return (pointer)1;
	}

	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
}

#endif /* XFree86LOADER */

OptionInfoPtr
AlpAvailableOptions(int chipid)
{
    return CirOptions;
}

Bool
AlpProbe(int entity, ScrnInfoPtr pScrn)
{
	pScrn->PreInit		= AlpPreInit;
	pScrn->ScreenInit	= AlpScreenInit;
	pScrn->SwitchMode	= AlpSwitchMode;
	pScrn->AdjustFrame	= AlpAdjustFrame;
	pScrn->EnterVT		= AlpEnterVT;
	pScrn->LeaveVT		= AlpLeaveVT;
	pScrn->FreeScreen	= AlpFreeScreen;
	pScrn->ValidMode	= AlpValidMode;

	xf86ConfigActivePciEntity(pScrn, entity, CIRPciChipsets, NULL,
								NULL, NULL, NULL, NULL);

	return TRUE;
}


static Bool
AlpGetRec(ScrnInfoPtr pScrn)
{
#ifdef ALP_DEBUG
	ErrorF("AlpGetRec\n");
#endif
	/*
	 * Allocate an AlpRec, and hook it into pScrn->driverPrivate.
	 * pScrn->driverPrivate is initialised to NULL, so we can check if
	 * the allocation has already been done.
	 */
	if (pScrn->driverPrivate != NULL)
		return TRUE;

	pScrn->driverPrivate = xnfcalloc(sizeof(AlpRec), 1);
	/* Initialise it */

#ifdef ALP_DEBUG
	ErrorF("AlpGetRec 0x%x\n", ALPPTR(pScrn));
#endif
	return TRUE;
}

static void
AlpFreeRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate == NULL)
		return;
	xfree(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}


/*
 * AlpCountRAM --
 *
 * Counts amount of installed RAM
 *
 * XXX Can use options to configure memory on non-primary cards.
 */
static int
AlpCountRam(ScrnInfoPtr pScrn)
{
	AlpPtr pAlp = ALPPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	MessageType from;
	int videoram = 0;

	/* Map the Alp memory and MMIO areas */
	pAlp->CirRec.FbMapSize = 1024*1024; /* XX temp */
	pAlp->CirRec.IoMapSize = 0x4000;	/* 16K for moment */
	if (!CirMapMem(&pAlp->CirRec, pScrn->scrnIndex))
		return 0;

	if (pAlp->CirRec.UseMMIO)
		vgaHWSetMmioFuncs(hwp, pAlp->CirRec.IOBase, -0x3C0);

	if (pAlp->SR0F != (CARD32)-1) {
		from = X_CONFIG;
		hwp->writeSeq(hwp, 0x0F, pAlp->SR0F);
	} else {
		from = X_PROBED;
		pAlp->SR0F = hwp->readSeq(hwp, 0x0F);
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 1 is 0x%02X\n",
		pAlp->SR0F);

	switch (pAlp->CirRec.Chipset) {
	case PCI_CHIP_GD5430:
/*  case PCI_CHIP_GD5440: */
		switch (pAlp->SR0F & 0x18) {
		case 0x08:
			videoram =  512;
			break;
		case 0x10:
			videoram = 1024;
			break;
		case 0x18:
			videoram = 2048;
			break;
		}
		break;

	case PCI_CHIP_GD5434_4:
	case PCI_CHIP_GD5434_8:
	case PCI_CHIP_GD5436:
		switch (pAlp->SR0F & 0x18) {
		case 0x10:
			videoram = 1024;
			break;
		case 0x18:
			videoram = 2048;
			if (pAlp->SR0F & 0x80)
				videoram = 4096;
			break;
		}

	case PCI_CHIP_GD5446:
		videoram = 1024;

		if (pAlp->SR17 != (CARD32)-1) {
			from = X_CONFIG;
			hwp->writeSeq(hwp, 0x17, pAlp->SR17);
		} else {
			from = X_PROBED;
			pAlp->SR17 = hwp->readSeq(hwp, 0x17);
		}
		xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 2 is 0x%02X\n",
			pAlp->SR17);

		if ((pAlp->SR0F & 0x18) == 0x18) {
			if (pAlp->SR0F & 0x80) {
				if (pAlp->SR17 & 0x80)
					videoram = 2048;
				else if (pAlp->SR17 & 0x02)
					videoram = 3072;
				else
					videoram = 4096;
			} else {
				if ((pAlp->SR17 & 80) == 0)
					videoram = 2048;
			}
		}
		break;

	case PCI_CHIP_GD5480:
		videoram = 1024;
		if ((pAlp->SR0F & 0x18) == 0x18) {	/* 2 or 4 MB */
			videoram *= 2048;
			if (pAlp->SR0F & 0x80)	/* Second bank enable */
				videoram = 4096;
		}
		break;
	}

	/* UNMap the Alp memory and MMIO areas */
	if (!CirUnmapMem(&pAlp->CirRec, pScrn->scrnIndex))
		return 0;

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
	AlpPtr pAlp = ALPPTR(pScrn);

	/* XXX ajv - 512, 576, and 1536 may not be supported
	   line pitches. see sdk pp 4-59 for more
	   details. Why anyone would want less than 640 is
	   bizarre. (maybe lots of pixels tall?) */

	/* The only line pitches the accelerator supports */
#if 1
	int accelWidths[] = { 640, 768, 800, 960, 1024, 1152, 1280,
							1600, 1920, 2048, 0 };
#else
	int accelWidths[] = { 512, 576, 640, 768, 800, 960, 1024, 1152,
							1280, 1536, 1600, 1920, 2048, 0 };
#endif

	for (i = 0; accelWidths[i] != 0; i++) {
		if (accelWidths[i] % pAlp->CirRec.Rounding == 0) {
			n++;
			linePitches = xnfrealloc(linePitches, n * sizeof(int));
			linePitches[n - 1] = accelWidths[i];
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
Bool
AlpPreInit(ScrnInfoPtr pScrn, int flags)
{
	AlpPtr pAlp;
	MessageType from;
	int i;
	ClockRangePtr clockRanges;
	char *mod = NULL;

#ifdef ALP_DEBUG
	ErrorF("AlpPreInit\n");
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

	/* Check the number of entities, and fail if it isn't one. */
	if (pScrn->numEntities != 1)
		return FALSE;

	/* The vgahw module should be loaded here when needed */
	if (!xf86LoadSubModule(pScrn, "vgahw"))
		return FALSE;

	xf86LoaderReqSymLists(vgahwSymbols, NULL);

	/*
	 * Allocate a vgaHWRec
	 */
	if (!vgaHWGetHWRec(pScrn))
		return FALSE;

	/* Allocate the AlpRec driverPrivate */
	if (!AlpGetRec(pScrn))
		return FALSE;

	pAlp = ALPPTR(pScrn);
	pAlp->CirRec.pScrn = pScrn;

	/* Get the entity, and make sure it is PCI. */
	pAlp->CirRec.pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
	if (pAlp->CirRec.pEnt->location.type != BUS_PCI)
		return FALSE;

	/* Find the PCI info for this screen */
	pAlp->CirRec.PciInfo = xf86GetPciInfoForEntity(pAlp->CirRec.pEnt->index);
	pAlp->CirRec.PciTag = pciTag(pAlp->CirRec.PciInfo->bus,
									pAlp->CirRec.PciInfo->device,
									pAlp->CirRec.PciInfo->func);

	/*
	 * XXX Check which of the VGA resources are decode and/or actually
	 * required in operating mode?  For now, assume everything is needed,
	 * so don't call xf86SetOperatingState().
	 */
	pScrn->racMemFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
	pScrn->racIoFlags =  RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;

	/* Set pScrn->monitor */
	pScrn->monitor = pScrn->confScreen->monitor;

	/*
	 * The first thing we should figure out is the depth, bpp, etc.
	 * Our default depth is 8, so pass it to the helper function.
	 * We support both 24bpp and 32bpp layouts, so indicate that.
	 */
	if (!xf86SetDepthBpp(pScrn, 8, 8, 8, Support24bppFb | Support32bppFb |
				SupportConvert32to24 | PreferConvert32to24)) {
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

	/* The gamma fields must be initialised when using the new cmap code */
	if (pScrn->depth > 1) {
		Gamma zeros = {0.0, 0.0, 0.0};

		if (!xf86SetGamma(pScrn, zeros))
			return FALSE;
	}

	/* We use a programamble clock */
	pScrn->progClock = TRUE;

	/* Collect all of the relevant option flags (fill in pScrn->options) */
	xf86CollectOptions(pScrn, NULL);

	/* Process the options */
	xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, CirOptions);

	pScrn->rgbBits = 6;
	from = X_DEFAULT;
	pAlp->CirRec.HWCursor = FALSE;
	if (xf86GetOptValBool(CirOptions, OPTION_HW_CURSOR, &pAlp->CirRec.HWCursor))
		from = X_CONFIG;

	if (xf86ReturnOptValBool(CirOptions, OPTION_SW_CURSOR, FALSE)) {
		from = X_CONFIG;
		pAlp->CirRec.HWCursor = FALSE;
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pAlp->CirRec.HWCursor ? "HW" : "SW");
	if (xf86ReturnOptValBool(CirOptions, OPTION_NOACCEL, FALSE)) {
		pAlp->CirRec.NoAccel = TRUE;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
	}
	if(pScrn->bitsPerPixel < 8) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"Cannot use accelerations in less than 8 bpp\n");
		pAlp->CirRec.NoAccel = TRUE;
	}

	/*
	 * Set the Chipset and ChipRev, allowing config file entries to
	 * override.
	 */
	if (pAlp->CirRec.pEnt->device->chipset && *pAlp->CirRec.pEnt->device->chipset) {
		pScrn->chipset = pAlp->CirRec.pEnt->device->chipset;
		pAlp->CirRec.Chipset = xf86StringToToken(CIRChipsets, pScrn->chipset);
		from = X_CONFIG;
	} else if (pAlp->CirRec.pEnt->device->chipID >= 0) {
		pAlp->CirRec.Chipset = pAlp->CirRec.pEnt->device->chipID;
		pScrn->chipset = (char *)xf86TokenToString(CIRChipsets, pAlp->CirRec.Chipset);
		from = X_CONFIG;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		pAlp->CirRec.Chipset);
	} else {
		from = X_PROBED;
		pAlp->CirRec.Chipset = pAlp->CirRec.PciInfo->chipType;
		pScrn->chipset = (char *)xf86TokenToString(CIRChipsets, pAlp->CirRec.Chipset);
	}
	if (pAlp->CirRec.pEnt->device->chipRev >= 0) {
		pAlp->CirRec.ChipRev = pAlp->CirRec.pEnt->device->chipRev;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
			pAlp->CirRec.ChipRev);
	} else {
		pAlp->CirRec.ChipRev = pAlp->CirRec.PciInfo->chipRev;
	}

	/*
	 * This shouldn't happen because such problems should be caught in
	 * CIRProbe(), but check it just in case.
	 */
	if (pScrn->chipset == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"ChipID 0x%04X is not recognised\n", pAlp->CirRec.Chipset);
		return FALSE;
	}
	if (pAlp->CirRec.Chipset < 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"Chipset \"%s\" is not recognised\n", pScrn->chipset);
		return FALSE;
	}

	xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

	/* Find the frame buffer base address */
	if (pAlp->CirRec.pEnt->device->MemBase != 0) {
		/*
		 * XXX Should check that the config file value matches one of the
		 * PCI base address values.
		 */
		pAlp->CirRec.FbAddress = pAlp->CirRec.pEnt->device->MemBase;
		from = X_CONFIG;
	} else {
		if (pAlp->CirRec.PciInfo->memBase[0] != 0) {
			/* 5446B and 5480 use mask of 0xfe000000.
			   5446A uses 0xff000000. */
			pAlp->CirRec.FbAddress = pAlp->CirRec.PciInfo->memBase[0] & 0xff000000;
			from = X_PROBED;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				"No valid FB address in PCI config space\n");
			AlpFreeRec(pScrn);
			return FALSE;
		}
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
		(unsigned long)pAlp->CirRec.FbAddress);

	if (pAlp->CirRec.pEnt->device->IOBase != 0) {
		pAlp->CirRec.IOAddress = pAlp->CirRec.pEnt->device->IOBase;
		from = X_CONFIG;
	} else {
		if (pAlp->CirRec.PciInfo->memBase[1] != 0) {
			pAlp->CirRec.IOAddress = pAlp->CirRec.PciInfo->memBase[1] & 0xfffff000;
			from = X_PROBED;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				"No valid MMIO address in PCI config space\n");
			/* 5446 rev A do not use a separate MMIO segment */
			/* We do not really need that YET. */
		}
	}
	if(pAlp->CirRec.IOAddress != 0) {
		xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
			(unsigned long)pAlp->CirRec.IOAddress);
		/* Default to MMIO if we have a separate IOAddress and
		   not in monochrome mode (IO 0x3Bx is not relocated!) */
		if (pScrn->bitsPerPixel != 1)
			pAlp->CirRec.UseMMIO = TRUE;
	}

	/* User options can override the MMIO default */
#if 0
	/* Will we ever support MMIO on 5446A or older? */
	if (xf86ReturnOptValBool(CirOptions, OPTION_MMIO, FALSE)) {
		pAlp->CirRec.UseMMIO = TRUE;
		from = X_CONFIG;
	}
#endif
	if (!xf86ReturnOptValBool(CirOptions, OPTION_MMIO, TRUE)) {
		pAlp->CirRec.UseMMIO = FALSE;
		from = X_CONFIG;
	}
	if (pAlp->CirRec.UseMMIO)
		xf86DrvMsg(pScrn->scrnIndex, from, "Using MMIO\n");

	/* Register the PCI-assigned resources. */
	if (xf86RegisterResources(pAlp->CirRec.pEnt->index, NULL, ResExclusive)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"xf86RegisterResources() found resource conflicts\n");
		return FALSE;
	}

	/* XXX If UseMMIO == TRUE and for any reason we cannot do MMIO,
	   abort here */

	/* XXX We do not know yet how to configure memory on this card.
	   Use options MemCFG1 and MemCFG2 to set registers SR0F and
	   SR17 before trying to count ram size. */

	pAlp->SR0F = (CARD32)-1;
	pAlp->SR17 = (CARD32)-1;

	(void) xf86GetOptValULong(CirOptions, OPTION_MEMCFG1, &pAlp->SR0F);
	(void) xf86GetOptValULong(CirOptions, OPTION_MEMCFG2, &pAlp->SR17);

	/*
	 * If the user has specified the amount of memory in the XF86Config
	 * file, we respect that setting.
	 */
	if (pAlp->CirRec.pEnt->device->videoRam != 0) {
		pScrn->videoRam = pAlp->CirRec.pEnt->device->videoRam;
		pAlp->CirRec.IoMapSize = 0x4000;	/* 16K for moment */
		from = X_CONFIG;
	} else {
		pScrn->videoRam = AlpCountRam(pScrn);
		from = X_PROBED;
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n", pScrn->videoRam);

	pAlp->CirRec.FbMapSize = pScrn->videoRam * 1024;

	/* XXX Set HW cursor use */

	/* Set the min pixel clock */
	pAlp->CirRec.MinClock = 12000;	/* XXX Guess, need to check this */
	xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
		pAlp->CirRec.MinClock / 1000);
	/*
	 * If the user has specified ramdac speed in the XF86Config
	 * file, we respect that setting.
	 */
	if (pAlp->CirRec.pEnt->device->dacSpeeds[0]) {
		ErrorF("Do not specily a Clocks line for Cirrus chips\n");
		return FALSE;
	} else {
		int speed;
		int *p = NULL;
		switch (pAlp->CirRec.Chipset) {
		case PCI_CHIP_GD5430:
		case PCI_CHIP_GD5434_4:
		case PCI_CHIP_GD5434_8:
		case PCI_CHIP_GD5436:
	/*	case PCI_CHIP_GD5440: */
			p = gd5430_MaxClocks;
			break;
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
		pAlp->CirRec.MaxClock = speed;
		from = X_PROBED;
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	pAlp->CirRec.MaxClock / 1000);

	/*
	 * Setup the ClockRanges, which describe what clock ranges are available,
	 * and what sort of modes they can be used for.
	 */
	clockRanges = xnfalloc(sizeof(ClockRange));
	clockRanges->next = NULL;
	clockRanges->minClock = pAlp->CirRec.MinClock;
	clockRanges->maxClock = pAlp->CirRec.MaxClock;
	clockRanges->clockIndex = -1;		/* programmable */
	clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
	clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
	clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
	clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
	clockRanges->ClockMulFactor = 1;
	clockRanges->ClockDivFactor = 1;
	clockRanges->PrivFlags = 0;

	pAlp->CirRec.Rounding = 128 >> pAlp->CirRec.BppShift;

#if 0
	if (pAlp->CirRec.Chipset != PCI_CHIP_GD5446 &&
		pAlp->CirRec.Chipset != PCI_CHIP_GD5480) {
		/* XXX Kludge */
		pAlp->CirRec.NoAccel = TRUE;
	}
#endif

	/*
	 * xf86ValidateModes will check that the mode HTotal and VTotal values
	 * don't exceed the chipset's limit if pScrn->maxHValue and
	 * pScrn->maxVValue are set.  Since our AlpValidMode() already takes
	 * care of this, we don't worry about setting them here.
	 */

	/* Select valid modes from those available */
	if (pAlp->CirRec.NoAccel) {
		/*
		 * XXX Assuming min pitch 256, max 2048
		 * XXX Assuming min height 128, max 2048
		 */
		i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
						pScrn->display->modes, clockRanges,
						NULL, 256, 2048,
						pAlp->CirRec.Rounding * pScrn->bitsPerPixel, 128, 2048,
						pScrn->display->virtualX,
						pScrn->display->virtualY,
						pAlp->CirRec.FbMapSize,
						LOOKUP_BEST_REFRESH);
	} else {
		/*
		 * XXX Assuming min height 128, max 2048
		 */
		i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
						pScrn->display->modes, clockRanges,
						GetAccelPitchValues(pScrn), 0, 0,
						pAlp->CirRec.Rounding * pScrn->bitsPerPixel, 128, 2048,
						pScrn->display->virtualX,
						pScrn->display->virtualY,
						pAlp->CirRec.FbMapSize,
						LOOKUP_BEST_REFRESH);
	}
	if (i == -1) {
		AlpFreeRec(pScrn);
		return FALSE;
	}

	/* Prune the modes marked as invalid */
	xf86PruneDriverModes(pScrn);

	if (i == 0 || pScrn->modes == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
		AlpFreeRec(pScrn);
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
	case 24:
		if (pix24bpp == 24)
			mod = "cfb24";
		else
			mod = "xf24_32bpp";
		break;
	case 32: mod = "cfb32";   break;
	}
	if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
		AlpFreeRec(pScrn);
		return FALSE;
	}

	/* Load XAA if needed */
	if (!pAlp->CirRec.NoAccel) {
		if (!xf86LoadSubModule(pScrn, "xaa")) {
			AlpFreeRec(pScrn);
			return FALSE;
		}
		xf86LoaderReqSymLists(xaaSymbols, NULL);
	}

	/* Load ramdac if needed */
	if (pAlp->CirRec.HWCursor) {
		if (!xf86LoadSubModule(pScrn, "ramdac")) {
			AlpFreeRec(pScrn);
			return FALSE;
		}
		xf86LoaderReqSymLists(ramdacSymbols, NULL);
	}

	if (!xf86LoadSubModule(pScrn, "i2c")) {
		AlpFreeRec(pScrn);
		return FALSE;
	}
	xf86LoaderReqSymLists(i2cSymbols,NULL);

	if (!xf86LoadSubModule(pScrn, "ddc")) {
		AlpFreeRec(pScrn);
		return FALSE;
	}
	xf86LoaderReqSymLists(ddcSymbols, NULL);

	return TRUE;
}

/*
 * This function saves the video state.
 */
static void
AlpSave(ScrnInfoPtr pScrn)
{
	AlpPtr pAlp;
	vgaHWPtr hwp;

#ifdef ALP_DEBUG
	ErrorF("AlpSave\n");
#endif

	hwp = VGAHWPTR(pScrn);
	pAlp = ALPPTR(pScrn);

	vgaHWSave(pScrn, &VGAHWPTR(pScrn)->SavedReg, VGA_SR_ALL);

#if 1
	pAlp->ModeReg.ExtVga[CR1A] = pAlp->SavedReg.ExtVga[CR1A] = hwp->readCrtc(hwp, 0x1A);
	pAlp->ModeReg.ExtVga[CR1B] = pAlp->SavedReg.ExtVga[CR1B] = hwp->readCrtc(hwp, 0x1B);
	pAlp->ModeReg.ExtVga[CR1D] = pAlp->SavedReg.ExtVga[CR1D] = hwp->readCrtc(hwp, 0x1D);
	pAlp->ModeReg.ExtVga[SR07] = pAlp->SavedReg.ExtVga[SR07] = hwp->readSeq(hwp, 0x07);
	pAlp->ModeReg.ExtVga[SR0E] = pAlp->SavedReg.ExtVga[SR0E] = hwp->readSeq(hwp, 0x0E);
	pAlp->ModeReg.ExtVga[SR12] = pAlp->SavedReg.ExtVga[SR12] = hwp->readSeq(hwp, 0x12);
	pAlp->ModeReg.ExtVga[SR13] = pAlp->SavedReg.ExtVga[SR13] = hwp->readSeq(hwp, 0x13);
	pAlp->ModeReg.ExtVga[SR1E] = pAlp->SavedReg.ExtVga[SR1E] = hwp->readSeq(hwp, 0x1E);
	pAlp->ModeReg.ExtVga[GR17] = pAlp->SavedReg.ExtVga[GR17] = hwp->readGr(hwp, 0x17);
	pAlp->ModeReg.ExtVga[GR18] = pAlp->SavedReg.ExtVga[GR18] = hwp->readGr(hwp, 0x18);
	/* The first 4 reads are for the pixel mask register. After 4 times that
	   this register is accessed in succession reading/writing this address
	   accesses the HDR. */
	hwp->readDacMask(hwp); hwp->readDacMask(hwp);
	hwp->readDacMask(hwp); hwp->readDacMask(hwp);
	pAlp->ModeReg.ExtVga[HDR] = pAlp->SavedReg.ExtVga[HDR] = hwp->readDacMask(hwp);
#else
	outb(hwp->IOBase+4, 0x1A); pAlp->ModeReg.ExtVga[CR1A] = pAlp->SavedReg.ExtVga[CR1A] = inb(hwp->IOBase + 5);
	outb(hwp->IOBase+4, 0x1B); pAlp->ModeReg.ExtVga[CR1B] = pAlp->SavedReg.ExtVga[CR1B] = inb(hwp->IOBase + 5);
	outb(hwp->IOBase+4, 0x1D); pAlp->ModeReg.ExtVga[CR1D] = pAlp->SavedReg.ExtVga[CR1D] = inb(hwp->IOBase + 5);
	outb(0x3C4, 0x07);   pAlp->ModeReg.ExtVga[SR07] = pAlp->SavedReg.ExtVga[SR07] = inb(0x3C5);
	outb(0x3C4, 0x0E);   pAlp->ModeReg.ExtVga[SR0E] = pAlp->SavedReg.ExtVga[SR0E] = inb(0x3C5);
	outb(0x3C4, 0x12);   pAlp->ModeReg.ExtVga[SR12] = pAlp->SavedReg.ExtVga[SR12] = inb(0x3C5);
	outb(0x3C4, 0x13);   pAlp->ModeReg.ExtVga[SR13] = pAlp->SavedReg.ExtVga[SR13] = inb(0x3C5);
	outb(0x3C4, 0x1E);   pAlp->ModeReg.ExtVga[SR1E] = pAlp->SavedReg.ExtVga[SR1E] = inb(0x3C5);
	outb(0x3CE, 0x17);   pAlp->ModeReg.ExtVga[GR17] = pAlp->SavedReg.ExtVga[GR17] = inb(0x3CF);
	outb(0x3CE, 0x18);   pAlp->ModeReg.ExtVga[GR18] = pAlp->SavedReg.ExtVga[GR18] = inb(0x3CF);
	/* The first 4 reads are for the pixel mask register. After 4 times that
	   this register is accessed in succession reading/writing this address
	   accesses the HDR. */
	inb(0x3C6); inb(0x3C6); inb(0x3C6); inb(0x3C6);
	pAlp->ModeReg.ExtVga[HDR] = pAlp->SavedReg.ExtVga[HDR] = inb(0x3C6);
#endif
}

/* XXX */
static void
AlpFix1bppColorMap(ScrnInfoPtr pScrn)
{
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
AlpModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	vgaHWPtr hwp;
	vgaRegPtr vgaReg;
	AlpPtr pAlp;
	int depthcode;
	int width;
	Bool HDiv2 = FALSE, VDiv2 = FALSE;

#ifdef ALP_DEBUG
	ErrorF("AlpModeInit %d bpp,   %d   %d %d %d %d   %d %d %d %d\n",
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

	ErrorF("AlpModeInit: depth %d bits\n", pScrn->depth);
#endif

	pAlp = ALPPTR(pScrn);
	hwp = VGAHWPTR(pScrn);
	vgaHWUnlock(hwp);

	depthcode = pScrn->depth;
	if (pScrn->bitsPerPixel == 32)
		depthcode = 32;

	if ((pAlp->CirRec.Chipset == PCI_CHIP_GD5480 && mode->Clock > 135100) ||
		(pAlp->CirRec.Chipset == PCI_CHIP_GD5446 && mode->Clock >  85500)) {
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
	pAlp->ModeReg.ExtVga[SR12] = 0;
#if 1
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
#else
	outw(0x3C4, (pAlp->ModeReg.ExtVga[SR12] << 8) | 0x12);
#endif

	if(VDiv2)
		hwp->ModeReg.CRTC[0x17] |= 0x04;

#ifdef ALP_DEBUG
	ErrorF("SynthClock = %d\n", mode->SynthClock);
#endif
	AlpSetClock(&pAlp->CirRec, hwp, mode->SynthClock);

	/* Disable DCLK pin driver, interrupts. */
	pAlp->ModeReg.ExtVga[GR17] |= 0x08;
	pAlp->ModeReg.ExtVga[GR17] &= ~0x04;
#if 1
	hwp->writeGr(hwp, 0x17, pAlp->ModeReg.ExtVga[GR17]);
#else
	outw(0x3CE, (pAlp->ModeReg.ExtVga[GR17] << 8) | 0x17);
#endif

	vgaReg = &hwp->ModeReg;

	pAlp->ModeReg.ExtVga[HDR] = 0;
	/* Enable linear mode and high-res packed pixel mode */
	pAlp->ModeReg.ExtVga[SR07] &= 0xe0;
#ifdef ALP_DEBUG
	ErrorF("depthcode = %d\n", depthcode);
#endif

	if (pScrn->bitsPerPixel == 1) {
		hwp->IOBase = 0x3B0;
		hwp->ModeReg.MiscOutReg &= ~0x01;
	} else {
		hwp->IOBase = 0x3D0;
		hwp->ModeReg.MiscOutReg |= 0x01;
	}

	switch (depthcode) {
	case 1:
	case 4:
		pAlp->ModeReg.ExtVga[SR07] |= 0x10;
		break;
	case 8:
		pAlp->ModeReg.ExtVga[SR07] |= 0x11;
		break;
	case 64+8:
		pAlp->ModeReg.ExtVga[SR07] |= 0x17;
		break;
	case 15:
		pAlp->ModeReg.ExtVga[SR07] |= 0x17;
		pAlp->ModeReg.ExtVga[HDR ]  = 0xC0;
		break;
	case 64+15:
		pAlp->ModeReg.ExtVga[SR07] |= 0x19;
		pAlp->ModeReg.ExtVga[HDR ]  = 0xC0;
		break;
	case 16:
		pAlp->ModeReg.ExtVga[SR07] |= 0x17;
		pAlp->ModeReg.ExtVga[HDR ]  = 0xC1;
		break;
	case 64+16:
		pAlp->ModeReg.ExtVga[SR07] |= 0x19;
		pAlp->ModeReg.ExtVga[HDR ]  = 0xC1;
		break;
	case 24:
		pAlp->ModeReg.ExtVga[SR07] |= 0x15;
		pAlp->ModeReg.ExtVga[HDR ]  = 0xC5;
		break;
	case 32:
		pAlp->ModeReg.ExtVga[SR07] |= 0x19;
		pAlp->ModeReg.ExtVga[HDR ]  = 0xC5;
		break;
	default:
		ErrorF("X11: Internal error: AlpModeInit: Cannot Initialize display to requested mode\n");
#ifdef ALP_DEBUG
		ErrorF("AlpModeInit returning FALSE on depthcode %d\n", depthcode);
#endif
		return FALSE;
	}
	if (HDiv2)
		pAlp->ModeReg.ExtVga[GR18] |= 0x20;
	else
		pAlp->ModeReg.ExtVga[GR18] &= ~0x20;

	/* No support for interlace (yet) */
	pAlp->ModeReg.ExtVga[CR1A] = 0x00;

#if 1
	hwp->writeGr(hwp, 0x18, pAlp->ModeReg.ExtVga[GR18]);
#else
	outw(0x3CE, (pAlp->ModeReg.ExtVga[GR18] << 8) | 0x18);
#endif

#if 1
	hwp->writeMiscOut(hwp, hwp->ModeReg.MiscOutReg);
#else
	outb(0x3C2, hwp->ModeReg.MiscOutReg);
#endif

#if 1
	hwp->writeSeq(hwp, 0x07, pAlp->ModeReg.ExtVga[SR07]);
	hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp);
	hwp->writeDacMask(hwp, pAlp->ModeReg.ExtVga[HDR ]);
#else
	outw(0x3C4, (pAlp->ModeReg.ExtVga[SR07] << 8) | 0x07);
	inb(0x3C6); inb(0x3C6); inb(0x3C6); inb(0x3C6);
	outb(0x3C6, pAlp->ModeReg.ExtVga[HDR ]);
#endif

	width = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
	if (pScrn->bitsPerPixel == 1)
		width <<= 2;
	hwp->ModeReg.CRTC[0x13] = width >> 3;
	/* Offset extension (see CR13) */
	pAlp->ModeReg.ExtVga[CR1B] &= 0xAF;
	pAlp->ModeReg.ExtVga[CR1B] |= (width >> (3+4)) & 0x10;
	pAlp->ModeReg.ExtVga[CR1B] |= (width >> (3+3)) & 0x40;
	pAlp->ModeReg.ExtVga[CR1B] |= 0x22;

#if 1
	hwp->writeCrtc(hwp, 0x1A, pAlp->ModeReg.ExtVga[CR1A]);
	hwp->writeCrtc(hwp, 0x1B, pAlp->ModeReg.ExtVga[CR1B]);
#else
	outw(hwp->IOBase + 4, (pAlp->ModeReg.ExtVga[CR1A] << 8) | 0x1A);
	outw(hwp->IOBase + 4, (pAlp->ModeReg.ExtVga[CR1B] << 8) | 0x1B);
#endif

	/* Programme the registers */
	vgaHWRestore(pScrn, &hwp->ModeReg, VGA_SR_MODE | VGA_SR_CMAP);

	/* XXX */
	if (pScrn->bitsPerPixel == 1)
		AlpFix1bppColorMap(pScrn);

	vgaHWProtect(pScrn, FALSE);

	return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void
AlpRestore(ScrnInfoPtr pScrn)
{
	vgaHWPtr hwp;
	vgaRegPtr vgaReg;
	AlpPtr pAlp;
	AlpRegPtr alpReg;

#ifdef ALP_DEBUG
	ErrorF("AlpRestore\n");
#endif

	hwp = VGAHWPTR(pScrn);
	pAlp = ALPPTR(pScrn);
	vgaReg = &hwp->SavedReg;
	alpReg = &pAlp->SavedReg;

	vgaHWProtect(pScrn, TRUE);

#if 1
	hwp->writeCrtc(hwp, 0x1A, alpReg->ExtVga[CR1A]);
	hwp->writeCrtc(hwp, 0x1B, alpReg->ExtVga[CR1B]);
	hwp->writeCrtc(hwp, 0x1D, alpReg->ExtVga[CR1D]);
	hwp->writeSeq(hwp, 0x07, alpReg->ExtVga[SR07]);
	hwp->writeSeq(hwp, 0x0E, alpReg->ExtVga[SR0E]);
	hwp->writeSeq(hwp, 0x12, alpReg->ExtVga[SR12]);
	hwp->writeSeq(hwp, 0x13, alpReg->ExtVga[SR13]);
	hwp->writeSeq(hwp, 0x1E, alpReg->ExtVga[SR1E]);
	hwp->writeGr(hwp, 0x17, alpReg->ExtVga[GR17]);
	hwp->writeGr(hwp, 0x18, alpReg->ExtVga[GR18]);
	/* The first 4 reads are for the pixel mask register. After 4 times that
	   this register is accessed in succession reading/writing this address
	   accesses the HDR. */
	hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp); hwp->readDacMask(hwp);
	hwp->writeDacMask(hwp, pAlp->SavedReg.ExtVga[HDR]);
#else
	outw(hwp->IOBase + 4, (alpReg->ExtVga[CR1A] << 8) | 0x1A);
	outw(hwp->IOBase + 4, (alpReg->ExtVga[CR1B] << 8) | 0x1B);
	outw(hwp->IOBase + 4, (alpReg->ExtVga[CR1D] << 8) | 0x1D);
	outw(0x3C4, (alpReg->ExtVga[SR07] << 8) | 0x07);
	outw(0x3C4, (alpReg->ExtVga[SR0E] << 8) | 0x0E);
	outw(0x3C4, (alpReg->ExtVga[SR12] << 8) | 0x12);
	outw(0x3C4, (alpReg->ExtVga[SR13] << 8) | 0x13);
	outw(0x3C4, (alpReg->ExtVga[SR1E] << 8) | 0x1E);
	outw(0x3CE, (alpReg->ExtVga[GR17] << 8) | 0x17);
	outw(0x3CE, (alpReg->ExtVga[GR18] << 8) | 0x18);
	/* The first 4 reads are for the pixel mask register. After 4 times that
	   this register is accessed in succession reading/writing this address
	   accesses the HDR. */
	inb(0x3C6); inb(0x3C6); inb(0x3C6); inb(0x3C6);
	outb(0x3C6, pAlp->SavedReg.ExtVga[HDR]);
#endif

	if (xf86IsPrimaryPci(pAlp->CirRec.PciInfo))
		vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
	else
		vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
	vgaHWProtect(pScrn, FALSE);
}

/* Mandatory */

/* This gets called at the start of each server generation */

Bool
AlpScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
	/* The vgaHW references will disappear one day */
	ScrnInfoPtr pScrn;
	vgaHWPtr hwp;
	AlpPtr pAlp;
	int i, ret;
	VisualPtr visual;

#ifdef ALP_DEBUG
	ErrorF("AlpScreenInit\n");
#endif

	/*
	 * First get the ScrnInfoRec
	 */
	pScrn = xf86Screens[pScreen->myNum];

	hwp = VGAHWPTR(pScrn);
	pAlp = ALPPTR(pScrn);

	/* Map the VGA memory when the primary video */
	if (xf86IsPrimaryPci(pAlp->CirRec.PciInfo)) {
		hwp->MapSize = 0x10000;		/* Standard 64k VGA window */
		if (!vgaHWMapMem(pScrn))
			return FALSE;
	}

	/* Map the Alp memory and MMIO areas */
	if (!CirMapMem(&pAlp->CirRec, pScrn->scrnIndex))
		return FALSE;

	if(pAlp->CirRec.UseMMIO)
		vgaHWSetMmioFuncs(hwp, pAlp->CirRec.IOBase, -0x3C0);

	vgaHWGetIOBase(hwp);

	/* Save the current state */
	AlpSave(pScrn);

	/* Initialise the first mode */
	if (!AlpModeInit(pScrn, pScrn->currentMode))
		return FALSE;

	/* Set the viewport */
	AlpAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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
#ifdef ALP_DEBUG
	ErrorF("AlpScreenInit before miSetVisualTypes\n");
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
#ifdef ALP_DEBUG
	ErrorF("AlpScreenInit after miSetVisualTypes\n");
#endif

	/*
	 * Call the framebuffer layer's ScreenInit function, and fill in other
	 * pScreen fields.
	 */

	switch (pScrn->bitsPerPixel) {
	case 1:
		ret = xf1bppScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		break;
	case 4:
		ret = xf4bppScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		break;
	case 8:
		ret = cfbScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		break;
	case 16:
		ret = cfb16ScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		break;
	case 24:
		if (pix24bpp == 24)
			ret = cfb24ScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		else
			ret = cfb24_32ScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		break;
	case 32:
		ret = cfb32ScreenInit(pScreen, pAlp->CirRec.FbBase,
								pScrn->virtualX, pScrn->virtualY,
								pScrn->xDpi, pScrn->yDpi,
								pScrn->displayWidth);
		break;
	default:
		xf86DrvMsg(scrnIndex, X_ERROR,
			"X11: Internal error: invalid bpp (%d) in AlpScreenInit\n",
			pScrn->bitsPerPixel);
		ret = FALSE;
		break;
	}
	if (!ret)
		return FALSE;

#ifdef ALP_DEBUG
	ErrorF("AlpScreenInit after depth dependent init\n");
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

	if (!pAlp->CirRec.NoAccel) { /* Initialize XAA functions */
		if (!(pAlp->CirRec.UseMMIO ? AlpXAAInitMMIO(pScreen) :
									AlpXAAInit(pScreen)))
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Could not initialize XAA\n");
	}

	/* Initialise cursor functions */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	if (pAlp->CirRec.HWCursor) { /* Initialize HW cursor layer */
		if (!AlpHWCursorInit(pScreen))
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				"Hardware cursor initialization failed\n");
	}
#if 0
	if (!AlpDGAInit(pScreen))
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "DGA initialization failed\n");
#endif

	if (!AlpI2CInit(pScrn))
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "I2C initialization failed\n");
	else
		xf86PrintEDID(xf86DoEDID_DDC2(pScrn->scrnIndex, pAlp->CirRec.I2CPtr1));

#ifdef ALPPROBEI2C
	AlpProbeI2C(pScrn->scrnIndex);
#endif

	/* Initialise default colourmap */
	if (!miCreateDefColormap(pScreen))
		return FALSE;

	if (pScrn->bitsPerPixel > 1 && pScrn->bitsPerPixel <= 8)
		vgaHWHandleColormaps(pScreen);

#ifdef DPMSExtension
	xf86DPMSInit(pScreen, AlpDisplayPowerManagementSet, 0);
#endif

	pScrn->memPhysBase = pAlp->CirRec.FbAddress;
	pScrn->fbOffset = 0;

#ifdef XvExtension
	{
		XF86VideoAdaptorPtr *ptr;
		int n;

		n = xf86XVListGenericAdaptors(&ptr);
		if (n)
			xf86XVScreenInit(pScreen, ptr, n);
	}
#endif

	/*
	 * Wrap the CloseScreen vector and set SaveScreen.
	 */
	pScreen->SaveScreen = AlpSaveScreen;
	pAlp->CirRec.CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = AlpCloseScreen;

	/* Report any unused options (only for the first generation) */
	if (serverGeneration == 1)
		xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

	/* Done */
	return TRUE;
}


/* Usually mandatory */
Bool
AlpSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
	return AlpModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
void
AlpAdjustFrame(int scrnIndex, int x, int y, int flags)
{
	ScrnInfoPtr pScrn;
	int Base, tmp;
	vgaHWPtr hwp;

	pScrn = xf86Screens[scrnIndex];
	hwp = VGAHWPTR(pScrn);

	Base = ((y * pScrn->displayWidth + x) / 8);
	if (pScrn->bitsPerPixel != 1)
		Base *= (pScrn->bitsPerPixel/4);

#ifdef ALP_DEBUG
	ErrorF("AlpAdjustFrame %d %d 0x%x %d %x\n", x, y, flags, Base, Base);
#endif

	if ((Base & ~0x000FFFFF) != 0) {
		ErrorF("X11: Internal error: AlpAdjustFrame: cannot handle overflow\n");
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
Bool
AlpEnterVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
#ifdef ALP_DEBUG
	ErrorF("AlpEnterVT\n");
#endif

	/* Should we re-save the text mode on each VT enter? */
	return AlpModeInit(pScrn, pScrn->currentMode);
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
void
AlpLeaveVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef ALP_DEBUG
	ErrorF("AlpLeaveVT\n");
#endif

	AlpRestore(pScrn);
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
AlpCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	AlpPtr pAlp = ALPPTR(pScrn);

	AlpRestore(pScrn);
	vgaHWLock(hwp);

	CirUnmapMem(&pAlp->CirRec, pScrn->scrnIndex);
	if (pAlp->CirRec.AccelInfoRec)
		XAADestroyInfoRec(pAlp->CirRec.AccelInfoRec);
	pAlp->CirRec.AccelInfoRec = NULL;
	if (pAlp->CirRec.CursorInfoRec)
		xf86DestroyCursorInfoRec(pAlp->CirRec.CursorInfoRec);
	pAlp->CirRec.CursorInfoRec = NULL;
#if 0
	if (pAlp->CirRec.DGAInfo)
		DGADestroyInfoRec(pAlp->CirRec.DGAInfo);
	pAlp->CirRec.DGAInfo = NULL;
#endif

	pScrn->vtSema = FALSE;

	pScreen->CloseScreen = pAlp->CirRec.CloseScreen;
	return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any persistent data structures */

/* Optional */
void
AlpFreeScreen(int scrnIndex, int flags)
{
#ifdef ALP_DEBUG
	ErrorF("AlpFreeScreen\n");
#endif
	/*
	 * This only gets called when a screen is being deleted.  It does not
	 * get called routinely at the end of a server generation.
	 */
	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
	AlpFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
int
AlpValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
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
AlpSaveScreen(ScreenPtr pScreen, Bool unblank)
{
	return vgaHWSaveScreen(pScreen, unblank);
}

/*
 * Set the clock to the requested frequency.  If the MCLK is very close
 * to the requested frequency, it sets a flag so that the MCLK can be used
 * as VCLK.  However this flag is not yet acted upon.
 */
static void
AlpSetClock(CirPtr pCir, vgaHWPtr hwp, int freq)
{
	int num, den, ffreq, usemclk, diff, mclk;
	CARD8 tmp;

	ErrorF("AlpSetClock freq=%d.%03dMHz\n", freq / 1000, freq % 1000);

	ffreq = freq;
	if (!CirrusFindClock(&ffreq, pCir->MaxClock, &num, &den))
		return;

	/* Calculate the MCLK. */
	mclk = 14318 * (hwp->readSeq(hwp, 0x1F) & 0x3F) / 8;	/* XXX */
	/*
	 * Favour MCLK as VLCK if it matches as good as the found clock,
	 * or if it is within 0.2 MHz of the request clock. A VCLK close
	 * to MCLK can cause instability.
	 */
	diff = abs(freq - ffreq);
	if (abs(mclk - ffreq) <= diff + 10 || abs(mclk - freq) <= 200)
		usemclk = TRUE;
	else
		usemclk = FALSE;

	ErrorF("AlpSetClock: nom=%x den=%x ffreq=%d.%03dMHz usemclk=%x\n",
		num, den, ffreq / 1000, ffreq % 1000, usemclk);

	/* So - how do we use MCLK here for the VCLK ? */

	/* Set VCLK3. */
	tmp = hwp->readSeq(hwp, 0x0E);
	hwp->writeSeq(hwp, 0x0E, (tmp & 0x80) | num);
	hwp->writeSeq(hwp, 0x1E, den);
}

/*
 * AlpDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
AlpDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
	unsigned char sr01, gr0e;
	vgaHWPtr hwp;

#ifdef ALP_DEBUG
	ErrorF("AlpDisplayPowerManagementSet\n");
#endif

	hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpDisplayPowerManagementSet: %d\n", PowerManagementMode);
#endif

	switch (PowerManagementMode) {
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

#ifdef ALPPROBEI2C
static void AlpProbeI2C(int scrnIndex)
{
	int i;
	I2CBusPtr b;

	b = xf86I2CFindBus(scrnIndex, "I2C bus 1");
	if (b == NULL)
		ErrorF("Could not find I2C bus \"%s\"\n", "I2C bus 1");
	else {
		for (i = 2; i < 256; i += 2)
			if (xf86I2CProbeAddress(b, i))
				ErrorF("Found device 0x%02x on bus \"%s\"\n", i, b->BusName);
	}
	b = xf86I2CFindBus(scrnIndex, "I2C bus 2");
	if (b == NULL)
		ErrorF("Could not find I2C bus \"%s\"\n", "I2C bus 2");
	else {
		for (i = 2; i < 256; i += 2)
			if (xf86I2CProbeAddress(b, i))
				ErrorF("Found device 0x%02x on bus \"%s\"\n", i, b->BusName);
	}
}
#endif
