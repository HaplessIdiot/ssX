/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_driver.c,v 1.30 1998/07/25 16:56:03 dawes Exp $ 
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *          ET6000 and ET4000W32 16/24/32 bpp support by Koen Gadeyne
 */
/* $XConsortium: et4_driver.c /main/27 1996/10/28 04:48:15 kaleb $ */

/*** Generic includes ***/

#include "tseng.h"		       /* this includes most of the generic ones as well */
#include "tseng_acl.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

/*
 * If using cfb, cfb.h is required.  Select the others for the bpp values
 * the driver supports.
 */
#define PSZ 8			       /* needed for cfb.h */
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

/*** Chip-specific includes ***/

/* #include "tseng_acl.h" */

/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */
static void TsengIdentify(int flags);
static Bool TsengProbe(DriverPtr drv, int flags);
static Bool TsengPreInit(ScrnInfoPtr pScrn, int flags);
static Bool TsengScreenInit(int Index, ScreenPtr pScreen, int argc,
    char **argv);
static Bool TsengEnterVT(int scrnIndex, int flags);
static void TsengLeaveVT(int scrnIndex, int flags);
static Bool TsengCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool TsengSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool TsengSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

/* Required if the driver supports moving the viewport */
static void TsengAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void TsengFreeScreen(int scrnIndex, int flags);
static ModeStatus TsengValidMode(int scrnIndex, DisplayModePtr mode,
    Bool verbose, int flags);

/* If driver-specific config file entries are needed, this must be defined */
/*static Bool   TsengParseConfig(ParseInfoPtr raw); */

/* Internally used functions */
static Bool TsengMapMem(ScrnInfoPtr pScrn);
static Bool TsengUnmapMem(ScrnInfoPtr pScrn);
static void TsengSave(ScrnInfoPtr pScrn);
static void TsengRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, TsengRegPtr tsengReg);
static Bool TsengModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void TsengUnlock(void);
static void TsengLock(void);

#define VERSION 4000
#define TSENG_NAME "TSENG"
#define TSENG_DRIVER_NAME "tseng"
#define TSENG_DRIVER_VERSION 0x00010001

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec TSENG =
{
    VERSION,
    "unaccelerated driver for Tseng Labs ET4000, accelerated driver for Tseng Labs ET4000W32, W32i, W32p, ET6000 and ET6100 cards",
    TsengIdentify,
    TsengProbe,
    NULL,
    0
};

/* sub-revisions are now dealt with in the ChipRev variable */
static SymTabRec TsengChipsets[] =
{
    {TYPE_ET4000, "ET4000"},
    {TYPE_ET4000W32, "ET4000W32"},
    {TYPE_ET4000W32I, "ET4000W32i"},
    {TYPE_ET4000W32P, "ET4000W32p"},
    {TYPE_ET6000, "ET6000"},
    {TYPE_ET6100, "ET6100"},
    {-1, NULL}
};

/* List of PCI chipset names */
static char *TsengPciNames[] =
{
    "ET4000W32p",
    "ET6000",
    "ET6100",
    NULL
};

/* List of PCI IDs */
static unsigned int TsengPciIds[] =
{
    PCI_CHIP_ET4000_W32P_A,
    PCI_CHIP_ET4000_W32P_B,
    PCI_CHIP_ET4000_W32P_C,
    PCI_CHIP_ET4000_W32P_D,
    PCI_CHIP_ET6000,
    ~0
};

typedef enum {
    OPTION_HIBIT_HIGH,
    OPTION_HIBIT_LOW,
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_BURST_ON,
    OPTION_PCI_BURST_OFF,
    OPTION_SLOW_DRAM,
    OPTION_MED_DRAM,
    OPTION_FAST_DRAM,
    OPTION_W32_INTERLEAVE_ON,
    OPTION_W32_INTERLEAVE_OFF,
    OPTION_NOACCEL,
    OPTION_NOLINEAR,
    OPTION_LINEAR,
    OPTION_SHOWCACHE,
    OPTION_LEGEND,
    OPTION_PCI_RETRY
} TsengOpts;

static OptionInfoRec TsengOptions[] =
{
    {OPTION_HIBIT_HIGH, "hibit_high", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_HIBIT_LOW, "hibit_low", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_BURST_ON, "pci_burst_on", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_BURST_OFF, "pci_burst_off", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_SLOW_DRAM, "slow_dram", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_MED_DRAM, "med_dram", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_FAST_DRAM, "fast_dram", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_W32_INTERLEAVE_ON, "w32_interleave_on", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_W32_INTERLEAVE_OFF, "w32_interleave_off", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOLINEAR, "NoLinear", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_LINEAR, "Linear", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_SHOWCACHE, "ShowCache", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_LEGEND, "Legend", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_RETRY, "PciRetry", OPTV_BOOLEAN,
	{0}, FALSE},
    {-1, NULL, OPTV_NONE,
	{0}, FALSE}
};

#ifdef XFree86LOADER

MODULEINITPROTO(tsengModuleInit);
static MODULESETUPPROTO(tsengSetup);

static XF86ModuleVersionInfo tsengVersRec =
{
    "tseng_drv.o",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    TSENG_DRIVER_VERSION,
    ABI_CLASS_VIDEODRV,		       /* This is a video driver */
    ABI_VIDEODRV_VERSION,
    {0, 0, 0, 0}
};

/*
 * This function is the module init function for XFree86 modules.
 *
 * Its name has to be the driver name followed by ModuleInit()
 */
void
tsengModuleInit(XF86ModuleVersionInfo ** vers, ModuleSetupProc * setup,
    ModuleTearDownProc * teardown)
{
    *vers = &tsengVersRec;
    *setup = tsengSetup;
    *teardown = NULL;
}

static pointer
tsengSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&TSENG, module, 0);

	/*
	 * Modules that this driver always requires can be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * The return value must be non-NULL on success even though there
	 * is no TearDownProc.
	 */
	return (pointer) 1;
    } else {
	if (errmaj)
	    *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

#endif /* XFree86LOADER */

static Bool
TsengGetRec(ScrnInfoPtr pScrn)
{
    ErrorF("	TsengGetRec\n");
    /*
     * Allocate an TsengRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(TsengRec), 1);
    /* Initialise it here when needed (or possible) */

    return TRUE;
}

static void
TsengFreeRec(ScrnInfoPtr pScrn)
{
    ErrorF("	TsengFreeRec\n");
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static t_tseng_type
TsengPCI2Type(ScrnInfoPtr pScrn, int ChipID)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    switch (ChipID) {
    case PCI_CHIP_ET4000_W32P_A:
	pTseng->ChipType = TYPE_ET4000W32P;
	pTseng->ChipRev = W32REVID_A;
	break;
    case PCI_CHIP_ET4000_W32P_B:
	pTseng->ChipType = TYPE_ET4000W32P;
	pTseng->ChipRev = W32REVID_B;
	break;
    case PCI_CHIP_ET4000_W32P_C:
	pTseng->ChipType = TYPE_ET4000W32P;
	pTseng->ChipRev = W32REVID_C;
	break;
    case PCI_CHIP_ET4000_W32P_D:
	pTseng->ChipType = TYPE_ET4000W32P;
	pTseng->ChipRev = W32REVID_D;
	break;
    case PCI_CHIP_ET6000:
	pTseng->ChipType = TYPE_ET6000;
	pTseng->ChipRev = pTseng->PciInfo->chipRev;	/* ET6000 != ET6100 */
	if (pTseng->ChipRev >= ET6100REVID)
	    pTseng->ChipType = TYPE_ET6100;
	break;
    default:
	xf86Msg(X_ERROR, "%s: Unknown Tseng PCI ID: %X\n", TSENG_NAME, ChipID);
	return FALSE;
    }
    return TRUE;
}

static void
TsengIdentify(int flags)
{
    ErrorF("	TsengIdentify\n");
    xf86PrintChipsets(TSENG_NAME, "driver for Tseng Labs chipsets",
	TsengChipsets);
}

static void
TsengAssignFPtr(ScrnInfoPtr pScrn)
{
    pScrn->driverVersion = VERSION;
    pScrn->driverName = TSENG_DRIVER_NAME;
    pScrn->name = TSENG_NAME;
    pScrn->Probe = TsengProbe;
    pScrn->PreInit = TsengPreInit;
    pScrn->ScreenInit = TsengScreenInit;
    pScrn->SwitchMode = TsengSwitchMode;
    pScrn->AdjustFrame = TsengAdjustFrame;
    pScrn->EnterVT = TsengEnterVT;
    pScrn->LeaveVT = TsengLeaveVT;
    pScrn->FreeScreen = TsengFreeScreen;
    pScrn->ValidMode = TsengValidMode;
}

/* unlock ET4000 using KEY register */
static void
TsengUnlock(void)
{
    unsigned char temp;
    int iobase = VGAHW_GET_IOBASE();

    ErrorF("	TsengUnlock\n");
    outb(0x3BF, 0x03);
    outb(iobase + 8, 0xA0);
    outb(iobase + 4, 0x11);
    temp = inb(iobase + 5);
    outb(iobase + 5, temp & 0x7F);
}

/* lock ET4000 using KEY register. FIXME: should restore old lock status instead */
static void
TsengLock(void)
{
    unsigned char temp;
    int iobase = VGAHW_GET_IOBASE();

    ErrorF("	TsengLock\n");
    outb(iobase + 4, 0x11);
    temp = inb(iobase + 5);
    outb(iobase + 5, temp | 0x80);
    outb(iobase + 8, 0x00);
    outb(0x3D8, 0x29);
    outb(0x3BF, 0x01);
}

/*
 * ET4000AutoDetect -- Old-style autodetection code (by register probing)
 *
 * This code is only called when the chipset is not given beforehand,
 * and if the PCI code hasn't detected one previously.
 */
static Bool
ET4000MinimalProbe(void)
{
    unsigned char temp, origVal, newVal;
    int iobase;

    ErrorF("	ET4000MinimalProbe\n");
    /*
     * Note, the vgaHW module cannot be used here, but there are
     * some macros in vgaHW.h that can be used.
     */
    iobase = VGAHW_GET_IOBASE();

    /*
     * Check first that there is a ATC[16] register and then look at
     * CRTC[33]. If both are R/W correctly it's a ET4000 !
     */
    temp = inb(iobase + 0x0A);
    TsengUnlock();		       /* only ATC 0x16 is protected by KEY */
    outb(0x3C0, 0x16 | 0x20);
    origVal = inb(0x3C1);
    outb(0x3C0, origVal ^ 0x10);
    outb(0x3C0, 0x16 | 0x20);
    newVal = inb(0x3C1);
    outb(0x3C0, origVal);
    TsengLock();
    if (newVal != (origVal ^ 0x10)) {
	return (FALSE);
    }
    outb(iobase + 0x04, 0x33);
    origVal = inb(iobase + 0x05);
    outb(iobase + 0x05, origVal ^ 0x0F);
    newVal = inb(iobase + 0x05);
    outb(iobase + 0x05, origVal);
    if (newVal != (origVal ^ 0x0F)) {
	return (FALSE);
    }
    return TRUE;
}

static Bool
TsengProbe(DriverPtr drv, int flags)
{
    int i;
    pciVideoPtr pPci, *usedPci;
    GDevPtr *devSections;
    GDevPtr *usedDevs;
    int numDevSections;
    int numUsed;
    Bool foundScreen = FALSE;

    ErrorF("	TsengProbe\n");
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
     * If the HW/driver uses the standard VGA registers, first
     * check if another driver has already claimed that slot.
     * If so, return immediately.
     *
     * All Tseng devices require standard VGA registers to run...
     */
    if (xf86CheckIsaSlot(ISA_COLOR) == FALSE) {
	return FALSE;
    }
    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(TSENG_DRIVER_NAME,
		&devSections)) <= 0) {
	return FALSE;
    }
    /*
     * for the Tseng server, there can only be one matching
     * device section. So issue a warning if more than one show up.
     * Multiple Tseng cards in the same machine are not possible.
     */
    if (numDevSections > 1) {
	xf86Msg(X_ERROR, "%s: More than one matching \"Device\" section!\n",
	    TSENG_NAME);
	xfree(devSections);
	devSections = NULL;
	return FALSE;
    }
    /*
     * If this is a PCI card, "probing" just amounts to checking the PCI
     * data that the server has already collected.  If there is none,
     * check for non-PCI boards.
     *
     * The provided xf86MatchPciInstances() helper takes care of
     * the details.
     */
    numUsed = 0;
    if (xf86GetPciVideoInfo() != NULL) {
	numUsed = xf86MatchPciInstances(TSENG_NAME, PCI_VENDOR_TSENG,
	    TsengPciIds, TsengPciNames, devSections, numDevSections,
	    &usedDevs, &usedPci);
/*        return FALSE; */
    }
    if (numUsed > 0) {
	/* Found some Tseng PCI cards */
	for (i = 0; i < numUsed; i++) {
	    pPci = usedPci[i];
	    if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func,
		    PCI_VGA)) {
		ScrnInfoPtr pScrn;

		/* Allocate a ScrnInfoRec and claim the slot */
		pScrn = xf86AllocateScreen(drv, 0);
		if (!xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func,
			PCI_VGA, pScrn->scrnIndex)) {
		    /* This can't happen */
		    FatalError("someone claimed the free slot!\n");
		}
		TsengAssignFPtr(pScrn);
		pScrn->device = usedDevs[i];
		foundScreen = TRUE;
	    }
	}
	xfree(usedDevs);
	xfree(usedPci);
	if (foundScreen)
	    return TRUE;
    } else {
	/* Check for non-PCI cards */
	ScrnInfoPtr pScrn;

	if (devSections[0]->chipset == NULL /* FIXME: and no chipID set? */ ) {
	    /* do minimal probe for supported chips */
	    if (!ET4000MinimalProbe()) {
		return FALSE;
		xfree(devSections);
		devSections = NULL;
	    }
	} else {
	    if (xf86StringToToken(TsengChipsets, devSections[0]->chipset) == -1) {
		xfree(devSections);
		devSections = NULL;
		return FALSE;
	    }
	    /* FIXME: what about ChipID ? */
	}
	/* Allocate a ScrnInfoRec and claim the slot */
	pScrn = xf86AllocateScreen(drv, 0);
	if (!xf86ClaimIsaSlot(ISA_COLOR, pScrn->scrnIndex)) {
	    /* This can't happen: we already checked for availability of ISA */
	    FatalError("someone claimed the free ISA slot!\n");
	}
	TsengAssignFPtr(pScrn);
	pScrn->device = devSections[0];
	xfree(devSections);
	devSections = NULL;
	return TRUE;
    }

    /* end of line -- nothing found */
    xfree(devSections);
    devSections = NULL;
    return FALSE;
}

/* The PCI part of TsengPreInit() */
static Bool
TsengPreInitPCI(ScrnInfoPtr pScrn)
{
    MessageType from;
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengPreInitPCI\n");
    /*
     * * Set the ChipType and ChipRev, allowing config file entries to
     * * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	/* chipset given as a string in the config file */
	pScrn->chipset = pScrn->device->chipset;
	pTseng->ChipType = xf86StringToToken(TsengChipsets, pScrn->chipset);
	/* FIXME: still need to probe for W32p revision here */
	from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	/* chipset given as a PCI ID in the config file */
	if (!TsengPCI2Type(pScrn, pScrn->device->chipID))
	    return FALSE;
	pScrn->chipset = (char *)xf86TokenToString(TsengChipsets, pTseng->ChipType);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
	    pTseng->ChipType);
    } else {
	from = X_PROBED;
	if (!TsengPCI2Type(pScrn, pTseng->PciInfo->chipType))
	    return FALSE;
	pScrn->chipset = (char *)xf86TokenToString(TsengChipsets, pTseng->ChipType);
    }

    if (pScrn->device->chipRev >= 0) {
	pTseng->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
	    pTseng->ChipRev);
	if ((pTseng->ChipType == TYPE_ET6000) && (pTseng->ChipRev >= ET6100REVID))
	    pTseng->ChipType = TYPE_ET6100;
    } else {
	/* FIXME: W32p needs its own chiprev here */
	pTseng->ChipRev = pTseng->PciInfo->chipRev;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    pTseng->PciTag = pciTag(pTseng->PciInfo->bus, pTseng->PciInfo->device,
	pTseng->PciInfo->func);

    /* the masks just get rid of the crap in the lower bits */

    if (pScrn->device->IOBase != 0) {
	pTseng->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	if (Is_ET6K) {
	    if ((pTseng->PciInfo->ioBase[1]) != 0) {
		pTseng->IOAddress = pTseng->PciInfo->ioBase[1];
		from = X_PROBED;
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid PCI I/O address in PCI config space\n");
		return FALSE;
	    }
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "PCI I/O registers at 0x%lX\n",
	(unsigned long)pTseng->IOAddress);

    pTseng->LinFbAddressMask = 0xFF000000;

    return TRUE;
}

/*
 * This is a more detailed probe. We already know it's a Tseng chip from
 * TsengPreInit() and ET4000MinimalProbe().
 */

static Bool
ET4000DetailedProbe(t_tseng_type * chiptype, t_w32_revid * rev)
{
    unsigned char temp, origVal, newVal;

    ErrorF("	ET4000DetailedProbe\n");

    /*
     * We know it's an ET4000, now check for an ET4000/W32.
     * Test for writability of 0x3cb.
     */

    origVal = inb(0x3cb);
    outb(0x3cb, 0x33);		       /* Arbitrary value */
    newVal = inb(0x3cb);
    outb(0x3cb, origVal);
    if (newVal != 0x33) {	       /* not a W32; so it's a standard ET4000 */
	*chiptype = TYPE_ET4000;
	*rev = TSENGNOREV;
	return TRUE;
    }
    /* We have an ET4000/W32. Now determine the type. */
    outb(0x217a, 0xec);
    temp = inb(0x217b) >> 4;
    switch (temp) {
    case 0:			       /* ET4000/W32 */
	*chiptype = TYPE_ET4000W32;
	break;
    case 1:			       /* ET4000/W32i */
	*chiptype = TYPE_ET4000W32I;
	*rev = W32REVID_A;
	break;
    case 3:			       /* ET4000/W32i rev b */
	*chiptype = TYPE_ET4000W32I;
	*rev = W32REVID_B;
	break;
    case 11:			       /* ET4000/W32i rev c */
	*chiptype = TYPE_ET4000W32I;
	*rev = W32REVID_C;
	break;
    case 2:			       /* ET4000/W32p rev a */
	*chiptype = TYPE_ET4000W32P;
	*rev = W32REVID_A;
	break;
    case 5:			       /* ET4000/W32p rev b */
	*chiptype = TYPE_ET4000W32P;
	*rev = W32REVID_B;
	break;
    case 6:			       /* ET4000/W32p rev d */
	*chiptype = TYPE_ET4000W32P;
	*rev = W32REVID_D;
	break;
    case 7:			       /* ET4000/W32p rev c */
	*chiptype = TYPE_ET4000W32P;
	*rev = W32REVID_C;
	break;
    default:
	return (FALSE);
    }

    return (TRUE);
}

/*
 * TsengFindBusType --
 *      determine bus interface type
 *      (also determines Lin Mem address mask, because that depends on bustype)
 *
 * We don't need to bother with PCI busses here: TsengPreInitPCI() took care
 * of that. This code isn't called if it's a PCI bus anyway.
 */

static void
TsengFindNonPciBusType(ScrnInfoPtr pScrn)
{
    unsigned char bus;
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengFindNonPciBusType\n");
    pTseng->Bustype = BUS_ISA;
    pTseng->Linmem_1meg = FALSE;
    pTseng->LinFbAddressMask = 0;

    switch (pTseng->ChipType) {
    case TYPE_ET4000:
	break;
    case TYPE_ET4000W32:
    case TYPE_ET4000W32I:
	/*
	 * Notation: SMx = bit x of Segment Map Comparator (CRTC index 0x30)
	 *
	 * We assume the driver code disables the image port (which it does)
	 *
	 * ISA:      [ A23==SEGE, A22, A21, A20 ] ==      [ SM1, SM0, 0, 0 ]
	 * MCA: [ A24, A23, A22, A21, A20 ] == [ SM2, SM1, SM0, 0, 0 ]
	 * VLB: [ /A26, /A25, /A24, A23, A22, A21, A20 ] ==   ("/" means inverted!)
	 *       [ SM4,  SM3,  SM2, SM1, SM0, 0  , 0   ]
	 */
	outb(0x217A, 0xEF);
	bus = inb(0x217B) & 0x60;      /* Determine bus type */
	switch (bus) {
	case 0x40:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32/W32i bus type: MCA\n");
	    pTseng->Bustype = BUS_MCA;
	    pTseng->LinFbAddressMask = 0x01C00000;	/* MADE24, A23 and A22 are decoded */
	    break;
	case 0x60:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32/W32i bus type: Local Bus\n");
	    pTseng->Bustype = BUS_VLB;
	    pTseng->LinFbAddressMask = 0x07C00000;	/* A26..A22 are decoded */
	    break;
	case 0x00:
	case 0x20:
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32/W32i bus type (0x%02X): ISA\n", bus);
	    pTseng->Bustype = BUS_ISA;
	    pTseng->LinFbAddressMask = 0x00C00000;	/* SEGE and A22 are decoded */
	    break;
	}
	break;
    case TYPE_ET4000W32P:
	outb(0x217A, 0xEF);
	bus = inb(0x217B) >> 3;	       /* Determine bus type */
	switch (bus) {
	case 0x1C:		       /* PCI case already handled in TsengPreInitPCI() */
	    pTseng->Bustype = BUS_VLB;
	    pTseng->LinFbAddressMask = 0x3FC00000;	/* A29..A22 */
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32p bus type: Local Buffered Bus\n");
	    pTseng->Linmem_1meg = TRUE;		/* IMA bus support allows for only 1M linear memory */
	    break;
	case 0x13:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32p bus type: Local Bus (option 1a)\n");
	    pTseng->Bustype = BUS_VLB;
	    if (pTseng->ChipRev == W32REVID_A)
		pTseng->LinFbAddressMask = 0x07C00000;
	    else
		pTseng->LinFbAddressMask = 0x1FC00000;	/* SEGI,A27..A22 */
	    break;
	case 0x11:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32p bus type: Local Bus (option 1b)\n");
	    pTseng->Bustype = BUS_VLB;
	    pTseng->LinFbAddressMask = 0x00C00000;	/* SEGI,A22 */
	    pTseng->Linmem_1meg = TRUE;		/* IMA bus support allows for only 1M linear memory */
	    break;
	case 0x08:
	case 0x0B:
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected W32p bus type: Local Bus (option 2)\n");
	    pTseng->Bustype = BUS_VLB;
	    pTseng->LinFbAddressMask = 0x3FC00000;	/* A29..A22 */
	    break;
	}
	if (Is_W32p_cd && (pTseng->LinFbAddressMask = 0x3FC00000))
	    pTseng->LinFbAddressMask |= 0xC0000000;	/* A31,A30 decoded from PCI config space */
	break;
    case TYPE_ET6000:
    case TYPE_ET6100:
	pTseng->Bustype = BUS_PCI;
	pTseng->LinFbAddressMask = 0xFF000000;
	break;
    }
}

/* The TsengPreInit() part for non-PCI busses */
static Bool
TsengPreInitNoPCI(ScrnInfoPtr pScrn)
{
    MessageType from;
    t_w32_revid rev = TSENGNOREV;
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengPreInitNoPCI\n");
    /*
     * Set the ChipType and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	/* chipset given as a string in the config file */
	pScrn->chipset = pScrn->device->chipset;
	pTseng->ChipType = xf86StringToToken(TsengChipsets, pScrn->chipset);
	from = X_CONFIG;
    } else if (pScrn->device->chipID > 0) {
	/* chipset given as a PCI ID in the config file */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ChipID override only possible for PCI cards\n");
	return FALSE;
    } else {
	from = X_PROBED;
	if (!ET4000DetailedProbe(&(pTseng->ChipType), &rev)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"Unknown Tseng chip detected. Try chipset override.\n");
	    return FALSE;
	}
	pScrn->chipset = (char *)xf86TokenToString(TsengChipsets, pTseng->ChipType);
    }
    if (pScrn->device->chipRev >= 0) {
	pTseng->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
	    pTseng->ChipRev);
    } else {
	pTseng->ChipRev = rev;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    TsengFindNonPciBusType(pScrn);

    return TRUE;
}

/*
 * The 8*32kb ET6000 MDRAM granularity causes the more general probe to
 * detect too much memory in some configurations, because that code has a
 * 8-bank (=256k) granularity. E.g. it fails to recognize 2.25 MB of memory
 * (detects 2.5 instead). This function goes to check if the RAM is actually
 * there. MDRAM comes in multiples of 4 banks (16, 24, 32, 36, 40, 64, 72,
 * 80, ... 32kb-banks), so checking each 64k block should be enough granularity.
 *
 * No more than the amount of refreshed RAM is checked. Non-refreshed RAM
 * won't work anyway.
 *
 * The same code could be used on other Tseng chips, or even on ANY
 * VGA board, but probably only in case of trouble.
 *
 * FIXME: this should be done using linear memory
 */
#define VIDMEM ((volatile CARD32*)check_vgabase)
#define SEGSIZE (64)		       /* kb */

#define ET6K_SETSEG(seg) \
	outb(0x3CB, ((seg) & 0x30) | ((seg) >> 4)); \
	outb(0x3CD, ((seg) & 0x0f) | ((seg) << 4));

static int
et6000_check_videoram(ScrnInfoPtr pScrn, int ram)
{
    unsigned char oldSegSel1, oldSegSel2, oldGR5, oldGR6, oldSEQ2, oldSEQ4;
    int segment, i;
    int real_ram = 0;
    pointer check_vgabase;
    Bool fooled = FALSE;
    int save_vidmem;

    ErrorF("	et6000_check_videoram(%d)\n", ram);
    if (ram > 4096) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	    "Detected more than 4096 kb of video RAM. Clipped to 4096kb\n");
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	    "    (Tseng VGA chips can only use 4096kb).\n");
	ram = 4096;
    }
    if (!vgaHWMapMem(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	    "Could not map VGA memory to check for video memory.\n");
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	    "    Detected amount may be wrong.\n");
	return ram;
    }
    check_vgabase = (VGAHWPTR(pScrn)->Base);

    /*
     * We need to set the VGA controller in VGA graphics mode, or else we won't
     * be able to access the full 4MB memory range. First, we save the
     * registers we modify, of course.
     */

    /* first set the KEY, so we can access the bank select registers */
    TsengUnlock();

    oldSegSel1 = inb(0x3CD);
    oldSegSel2 = inb(0x3CB);
    outb(0x3CE, 5);
    oldGR5 = inb(0x3CF);
    outb(0x3CE, 6);
    oldGR6 = inb(0x3CF);
    outb(0x3C4, 2);
    oldSEQ2 = inb(0x3C5);
    outb(0x3C4, 4);
    oldSEQ4 = inb(0x3C5);

    /* set graphics mode */
    outb(0x3CE, 6);
    outb(0x3CF, 5);
    outb(0x3CE, 5);
    outb(0x3CF, 0x40);
    outb(0x3C4, 2);
    outb(0x3C5, 0x0f);
    outb(0x3C4, 4);
    outb(0x3C5, 0x0e);

    /*
     * count down from presumed amount of memory in SEGSIZE steps, and
     * look at each segment for real RAM.
     *
     * To select a segment, we cannot use ET4000W32SetReadWrite(), since
     * that requires the ScreenPtr, which we don't have here.
     */

    for (segment = (ram / SEGSIZE) - 1; segment >= 0; segment--) {
	/* select the segment */
	ET6K_SETSEG(segment);

	/* save contents of memory probing location */
	save_vidmem = *(VIDMEM);

	/* test with pattern */
	*VIDMEM = 0xAAAA5555;
	if (*VIDMEM != 0xAAAA5555) {
	    *VIDMEM = save_vidmem;
	    continue;
	}
	/* test with inverted pattern */
	*VIDMEM = 0x5555AAAA;
	if (*VIDMEM != 0x5555AAAA) {
	    *VIDMEM = save_vidmem;
	    continue;
	}
	/*
	 * If we get here, the memory seems to be writable/readable
	 * Now check if we aren't fooled by address wrapping (mirroring)
	 */
	fooled = FALSE;
	for (i = segment - 1; i >= 0; i--) {
	    /* select the segment */
	    ET6K_SETSEG(i);
	    outb(0x3CB, (i & 0x30) | (i >> 4));
	    outb(0x3CD, (i & 0x0f) | (i << 4));
	    if (*VIDMEM == 0x5555AAAA) {
		/*
		 * Seems like address wrap, but there could of course be
		 * 0x5555AAAA in here by accident, so we check with another
		 * pattern again.
		 */
		ET6K_SETSEG(segment);
		/* test with other pattern again */
		*VIDMEM = 0xAAAA5555;
		ET6K_SETSEG(i);
		if (*VIDMEM == 0xAAAA5555) {
		    /* now we're sure: this is not real memory */
		    fooled = TRUE;
		    break;
		}
	    }
	}
	if (!fooled) {
	    real_ram = (segment + 1) * SEGSIZE;
	    break;
	}
	/* restore old contents again */
	ET6K_SETSEG(segment);
	*VIDMEM = save_vidmem;
    }

    /* restore original register contents */
    outb(0x3CD, oldSegSel1);
    outb(0x3CB, oldSegSel2);
    outb(0x3CE, 5);
    outb(0x3CF, oldGR5);
    outb(0x3CE, 6);
    outb(0x3CF, oldGR6);
    outb(0x3C4, 2);
    outb(0x3C5, oldSEQ2);
    outb(0x3C4, 4);
    outb(0x3C5, oldSEQ4);

/*    TsengLock(); */
    vgaHWUnmapMem(pScrn);
    return real_ram;
}

/*
 * Handle amount of allowed memory: some combinations can't use all
 * available memory. Should we still allow the user to override this?
 *
 * This must be called AFTER the decision has been made to use linear mode
 * and/or acceleration, or the memory limit code won't be able to work.
 */

static int
TsengDoMemLimit(ScrnInfoPtr pScrn, int ram, int limit, char *reason)
{
    if (ram > limit) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Only %d kb of memory can be used %s.\n",
	    limit, reason);
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Reducing video memory to %d kb.\n", limit);
	ram = limit;
    }
    return ram;
}

static int
TsengLimitMem(ScrnInfoPtr pScrn, int ram)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    if (pTseng->UseLinMem && pTseng->Linmem_1meg) {
	TsengDoMemLimit(pScrn, ram, 1024, "in linear mode on this VGA board/bus configuration");
    }
    if (!pTseng->NoAccel && pTseng->UseLinMem) {
	if (Is_W32_any) {
	    /* <= W32p_ab :
	     *   2 MB direct access + 2*512kb via apertures MBP0 and MBP1
	     * == W32p_cd :
	     *   2*1MB via apertures MBP0 and MBP1
	     */
	    if (Is_W32p_cd)
		TsengDoMemLimit(pScrn, ram, 2048, "in linear + accelerated mode on W32p rev c and d");

	    TsengDoMemLimit(pScrn, ram, 2048 + 1024, "in linear + accelerated mode on W32/W32i/W32p");

	    /* upper 516kb of 4MB linear map used for "externally mapped registers" */
	    TsengDoMemLimit(pScrn, ram, 4096 - 516, "in linear + accelerated mode on W32/W32i/W32p");
	}
	if (Is_ET6K) {
	    /* upper 8kb used for externally mapped and memory mapped registers */
	    TsengDoMemLimit(pScrn, ram, 4096 - 8, "in linear + accelerated mode on ET6000/6100");
	}
    }
    TsengDoMemLimit(pScrn, ram, 4096, "on any Tseng card");
    return ram;
}

/*
 * TsengDetectMem --
 *      try to find amount of video memory installed.
 *
 */

static int
TsengDetectMem(ScrnInfoPtr pScrn)
{
    unsigned char config;
    int ramtype = 0;
    int ram = 0;
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengDetectMem\n");
    if (Is_ET6K) {
	ramtype = inb(0x3C2) & 0x03;
	switch (ramtype) {
	case 0x03:		       /* MDRAM */
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		"Video memory type: Multibank DRAM (MDRAM).\n");
	    ram = ((inb(pTseng->IOAddress + 0x47) & 0x07) + 1) * 8 * 32;	/* number of 8 32kb banks  */
	    if (inb(pTseng->IOAddress + 0x45) & 0x04) {
		ram <<= 1;
	    }
	    /*
	     * 8*32kb MDRAM refresh control granularity in the ET6000 fails to
	     * recognize 2.25 MB of memory (detects 2.5 instead)
	     */
	    ram = et6000_check_videoram(pScrn, ram);
	    break;
	case 0x00:		       /* DRAM -- VERY unlikely on ET6000 cards, IMPOSSIBLE on ET6100 */
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		"Video memory type: Standard DRAM.\n");
	    ram = 1024 << (inb(pTseng->IOAddress + 0x45) & 0x03);
	    break;
	default:		       /* unknown RAM type */
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		"Unknown ET6000 video memory type %d -- assuming 1 MB (unless specified)\n",
		ramtype);
	    ram = 1024;
	}
    } else {			       /* pre-ET6000 devices */
	int iobase = VGAHWPTR(pScrn)->IOBase;

	outb(iobase + 0x04, 0x37);
	config = inb(iobase + 0x05);

	ram = 128 << (config & 0x03);

	if (config & 0x80)
	    ram <<= 1;

	/* Check for interleaving on W32i/p. */
	if (Is_W32i || Is_W32p) {
	    outb(iobase + 0x04, 0x32);
	    config = inb(iobase + 0x05);
	    if (config & 0x80) {
		ram <<= 1;
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Video memory type: Interleaved DRAM.\n");
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Video memory type: Standard DRAM.\n");
	    }
	}
    }
    return ram;
}

static Bool
TsengProcessHibit(ScrnInfoPtr pScrn)
{
    MessageType from = X_CONFIG;
    int hibit_mode_width;
    int iobase;
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengProcessHibit\n");
    if (xf86IsOptionSet(TsengOptions, OPTION_HIBIT_HIGH)) {
	if (xf86IsOptionSet(TsengOptions, OPTION_HIBIT_LOW)) {
	    xf86Msg(X_ERROR, "\nOptions \"hibit_high\" and \"hibit_low\" are incompatible;\n");
	    xf86Msg(X_ERROR, "    specify only one (not both) in XFree86 configuration file\n");
	    return FALSE;
	}
	pTseng->save_divide = 0x40;
    } else if (xf86IsOptionSet(TsengOptions, OPTION_HIBIT_HIGH)) {
	pTseng->save_divide = 0;
    } else {
	from = X_PROBED;
	/* first check to see if hibit is probed from low-res mode */
	iobase = VGAHWPTR(pScrn)->IOBase;
	outb(iobase + 4, 1);
	hibit_mode_width = inb(iobase + 5) + 1;
	if (hibit_mode_width > 82) {
	    xf86Msg(X_WARNING, "Non-standard VGA text or graphics mode while probing for hibit:\n");
	    xf86Msg(X_WARNING, "    probed 'hibit' value may be wrong.\n");
	    xf86Msg(X_WARNING, "    Preferably run probe from 80x25 textmode,\n");
	    xf86Msg(X_WARNING, "    or specify correct value in XFree86 configuration file.\n");
	}
	/* Check for initial state of divide flag */
	outb(0x3C4, 7);
	pTseng->save_divide = inb(0x3C5) & 0x40;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Initial ET4000 hibit state: %s\n",
	pTseng->save_divide & 0x40 ? "high" : "low");
    return TRUE;
}

static Bool
TsengProcessOptions(ScrnInfoPtr pScrn)
{
    MessageType from;
    TsengPtr pTseng = TsengPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, TsengOptions);

    from = X_DEFAULT;
    pTseng->HWCursor = FALSE;	       /* default */
    if (xf86IsOptionSet(TsengOptions, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	pTseng->HWCursor = TRUE;
    }
    if (xf86IsOptionSet(TsengOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pTseng->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	pTseng->HWCursor ? "HW" : "SW");

    pTseng->NoAccel = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_NOACCEL)) {
	pTseng->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    pTseng->SlowDram = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_SLOW_DRAM)) {
	pTseng->SlowDram = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using slow DRAM access\n");
    }
    pTseng->FastDram = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_MED_DRAM)) {
	pTseng->MedDram = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using Medium-speed DRAM access\n");
    }
    pTseng->FastDram = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_FAST_DRAM)) {
	pTseng->FastDram = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using fast DRAM access\n");
    }
    pTseng->W32InterleaveOn = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_W32_INTERLEAVE_ON)) {
	pTseng->W32InterleaveOn = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing W32p memory interleave ON.\n");
    }
    pTseng->W32InterleaveOff = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_W32_INTERLEAVE_OFF)) {
	pTseng->W32InterleaveOff = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing W32p memory interleave OFF.\n");
    }
    from = X_DEFAULT;
    if (pTseng->PciTag)
	pTseng->UseLinMem = TRUE;      /* use linear memory by default on PCI systems */
    else
	pTseng->UseLinMem = FALSE;     /* ... and banked on non-PCI systems */
    if (xf86IsOptionSet(TsengOptions, OPTION_NOLINEAR)) {
	pTseng->UseLinMem = FALSE;
    }
    if (xf86IsOptionSet(TsengOptions, OPTION_LINEAR)) {
	pTseng->UseLinMem = TRUE;
	if (!CHIP_SUPPORTS_LINEAR) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Linear memory not supported on chipset \"%s\".\n",
		pScrn->chipset);
	} else if (!xf86LinearVidMem()) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"This operating system does not support a linear framebuffer.\n");
	    pTseng->UseLinMem = FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s Memory.\n",
	(pTseng->UseLinMem) ? "linear" : "banked");

    pTseng->ShowCache = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_SHOWCACHE)) {
	pTseng->ShowCache = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "(for debugging only:) Visible off-screen memory\n");
    }
    pTseng->Legend = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_LEGEND)) {
	pTseng->Legend = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using Legend pixel clock selection.\n");
    }
    pTseng->UsePCIRetry = FALSE;
    if (xf86IsOptionSet(TsengOptions, OPTION_PCI_RETRY)) {
	pTseng->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }
    return TRUE;
}

static Bool
TsengGetLinFbAddress(ScrnInfoPtr pScrn)
{
    MessageType from;
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengGetLinFbAddress\n");

    from = X_PROBED;

    /* let config file override Base address */
    if (pScrn->device->MemBase != 0) {
	pTseng->LinFbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
	/* check for possible errors in given linear base address */
	if ((pTseng->LinFbAddress & (~pTseng->LinFbAddressMask)) != 0) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"MemBase out of range. Must be <= 0x%x on 0x%x boundary.\n",
		pTseng->LinFbAddressMask, ~(pTseng->LinFbAddressMask | 0xFF000000) + 1);
	    pTseng->LinFbAddress &= ~pTseng->LinFbAddressMask;
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "    Clipping MemBase to: 0x%x.\n",
		pTseng->LinFbAddress);
	}
    } else {
	from = X_PROBED;
	if (pTseng->PciTag && Is_ET6K) {
	    /* PCI ET6000 boards only: PCI ET4000W32p doesn't use the PCI space.
	     * base0 is the framebuffer and base1 is the PCI IO space.
	     */
	    if ((pTseng->PciInfo->memBase[0]) != 0) {
		pTseng->LinFbAddress = pTseng->PciInfo->memBase[0];
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid Framebuffer address in PCI config space;\n");
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "    Falling back to banked mode.\n");
		pTseng->UseLinMem = FALSE;
		return TRUE;
	    }
	} else {		       /* non-PCI boards */
	    /* W32p cards can give us their Lin. memory address through the PCI
	     * configuration. For W32i, this is not possible (VL-bus, MCA or ISA). W32i
	     * cards have three extra external "address" lines, SEG2..SEG0 which _can_
	     * be connected to any set of address lines in addition to the already
	     * connected A23..A0. SEG2..SEG0 are either for three extra address lines
	     * or to connect an external address decoder (mostly an 74F27). It is NOT
	     * possible to know how SEG2..SEG0 are connected. We _assume_ they are
	     * connected to A26..A24 (most likely case). This means linear memory can
	     * be decoded into any 4MB block in the address range 0..128MB.
	     *
	     * For non-PCI cards (especially VLB), most motherboards don't decode all
	     * 32 address bits. The weird default memory base below will always end up
	     * at the end of the decoded address space -- independent of the number of
	     * address bits that are decoded.
	     */
#define DEFAULT_LIN_MEMBASE ( (256 + 128 + 64 + 32 + 16 + 8 + 4) * 1024*1024 )
#define DEFAULT_LIN_MEMBASE_PCI (DEFAULT_LIN_MEMBASE & 0xFF000000)

	    switch (pTseng->ChipType) {
	    case TYPE_ET4000W32:
	    case TYPE_ET4000W32I:
	    case TYPE_ET4000W32P:     /* A31,A30 are decoded as 00 (=always mapped below 512 MB) */
		pTseng->LinFbAddress = DEFAULT_LIN_MEMBASE;
		if (pTseng->LinFbAddress > pTseng->LinFbAddressMask)	/* ... except if we can't decode that much */
		    pTseng->LinFbAddress = pTseng->LinFbAddressMask - 4 * 1024 * 1024;	/* top of decodable memory */
		break;
	    default:
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "TsengNonPciLinMem(): Internal error. This should not happen: please report to XFree86@XFree86.Org\n");
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "    Falling back to banked mode.\n");
		pTseng->UseLinMem = FALSE;
		return TRUE;
	    }
	    pTseng->LinFbAddress &= pTseng->LinFbAddressMask;

	    /* One final check for a valid MemBase */
	    if (pTseng->LinFbAddress < 4096 * 1024) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Invalid MemBase for linear mode:\n");
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "    please define a non-zero MemBase in XF86Config.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "    See README.tseng or tseng.sgml for more information.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "    Using banked mode instead.\n");
		pTseng->UseLinMem = FALSE;
		return TRUE;
	    }
	}
    }

    /* The W32 linear map address space is always 4Mb (mainly because the
     * memory-mapped registers are located near the top of the 4MB area). 
     * The ET6000 maps out 16 Meg, but still uses only 4Mb of that. 
     * However, since all mmap()-ed space is also reflected in the "ps"
     * listing for the Xserver, many users will be worried by a server that
     * always eats 16MB of memory, even if it's not "real" memory, just
     * address space. Not mapping all of the 16M may be a potential problem
     * though: if another board is mapped on top of the remaining part of
     * the 16M... Boom!
     */
    if (Is_ET6K)
	pTseng->FbMapSize = 16384 * 1024;
    else
	pTseng->FbMapSize = 4096 * 1024;

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	(unsigned long)pTseng->LinFbAddress);

    return TRUE;
}

static Bool
TsengPreInit(ScrnInfoPtr pScrn, int flags)
{
    TsengPtr pTseng;
    MessageType from;
    ClockRangePtr clockRanges;
    pciVideoPtr *pciList = NULL;
    int i;
    char *mod = NULL;

    ErrorF("	TsengPreInit\n");
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

    vgaHWGetIOBase(VGAHWPTR(pScrn));
    VGAHWPTR(pScrn)->MapSize = 0x10000;

    /*
     * Since, the capabilities are determined by the chipset, the very first
     * thing to do is to figure out the chipset and its capabilities.
     */

    /* Allocate the TsengRec driverPrivate */
    if (!TsengGetRec(pScrn)) {
	return FALSE;
    }
    pTseng = TsengPTR(pScrn);

    /*
     * Find the bus slot for this screen (PCI or other). This also finds the
     * exact chipset type.
     */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) == 0) {
	/* we didn't have a PCI card. Check for other busses */
	xfree(pciList);
	if (!TsengPreInitNoPCI(pScrn)) {
	    TsengFreeRec(pScrn);
	    return FALSE;
	}
    } else {
	/* found some PCI card(s) */
	if (i > 1) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"Expected one PCI card, but found %d\n", i);
	    xfree(pciList);
	    TsengFreeRec(pScrn);
	    return FALSE;
	} else {
	    pTseng->PciInfo = *pciList;
	    if (!TsengPreInitPCI(pScrn)) {
		TsengFreeRec(pScrn);
		return FALSE;
	    }
	}
    }

    /*
     * Find RAMDAC type and fill Tsengdac struct
     */
    Check_Tseng_Ramdac(pScrn);
    tseng_init_clockscale(pTseng);

    /* check for clockchip */
    if (!Tseng_check_clockchip(pScrn)) {
	return FALSE;
    }
    /*
     * Now we can check what depth we support.
     */

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     * Our preference for depth 24 is 24bpp, so tell it that too.
     */
    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, Support24bppFb | Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	Bool CanDo16bpp = FALSE, CanDo24bpp = FALSE, CanDo32bpp = FALSE;
	Bool CanDoThis = FALSE;

	switch (pTseng->DacInfo.DacType) {
	case ET6000_DAC:
	case ICS5341_DAC:
	case STG1703_DAC:
	case STG1702_DAC:
	    CanDo16bpp = TRUE;
	    CanDo24bpp = TRUE;
	    CanDo32bpp = TRUE;
	    break;
	case ATT20C490_DAC:
	case ATT20C491_DAC:
	case ATT20C492_DAC:
	case ATT20C493_DAC:
	case ICS5301_DAC:
	case MUSIC4910_DAC:
	    CanDo16bpp = TRUE;
	    CanDo24bpp = TRUE;
	    break;
	case CH8398_DAC:
	    CanDo16bpp = TRUE;
	    CanDo24bpp = TRUE;
	    break;
	case STG1700_DAC:	       /* can't do packed 24bpp over a 16-bit bus */
	    CanDo16bpp = TRUE;	       /* FIXME: can do it over 8 bit bus */
	    CanDo32bpp = TRUE;
	    break;
	default:		       /* default: only 1, 4, 8 bpp */
	    break;
	}

	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	    CanDoThis = TRUE;
	    break;
	case 16:
	    CanDoThis = CanDo16bpp;
	    break;
	case 24:
	    CanDoThis = (CanDo24bpp || CanDo32bpp);
	    break;
	}
	if (!CanDoThis) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"Given depth (%d) is not supported by this chipset/RAMDAC\n",
		pScrn->depth);
	    return FALSE;
	}
	if ((pScrn->bitsPerPixel == 32) && (!CanDo32bpp)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"Given bpp (%d) is not supported by this chipset/RAMDAC\n",
		pScrn->bitsPerPixel);
	    return FALSE;
	}
    }
    xf86PrintDepthBpp(pScrn);

    pTseng->Bytesperpixel = pScrn->bitsPerPixel / 8;

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */

    /* Set weight/mask/offset for depth > 8 */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros =
	{0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros)) {
	    return FALSE;
	} else {
	    /* XXX check that weight returned is supported */
	    ;
	}
    }
    /* Set the default visual. */
    if (!xf86SetDefaultVisual(pScrn, -1)) {
	return FALSE;
    } else {
	/* Tseng doesn't support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
		" (%s) is not supported at depth %d\n",
		xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    return FALSE;
	}
    }

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma()
     * here. But None of the Tseng devices does.
     */

    /* We use a programmable clock */
    pScrn->progClock = TRUE;	       /* FIXME */

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* Default to 6, because most Tseng chips/RAMDACs don't support it */
	pScrn->rgbBits = 6;
    }
    if (!TsengProcessOptions(pScrn))   /* must be done _after_ we know what chip this is */
	return FALSE;

    if (pTseng->UseLinMem) {
	if (!TsengGetLinFbAddress(pScrn))
	    return FALSE;
    }
    /* hibit processing (TsengProcessOptions() must have been called first) */
    pTseng->save_divide = 0x40;	       /* default */
    if (!Is_ET6K) {
	if (!TsengProcessHibit(pScrn))
	    return FALSE;
    }
    /*
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	from = X_PROBED;
	pScrn->videoRam = TsengDetectMem(pScrn);
    }
    pScrn->videoRam = TsengLimitMem(pScrn, pScrn->videoRam);
    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
	pScrn->videoRam);

/*    pTseng->FbMapSize = pScrn->videoRam * 1024; */

    /* TODO XXX Set HW cursor use */

    /* Set the min pixel clock */
    pTseng->MinClock = 12000;	       /* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	pTseng->MinClock / 1000);
    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->dacSpeeds[0]) {
	int speed;

	switch (pScrn->bitsPerPixel) {
	default:
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
	    pTseng->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pTseng->MaxClock = speed;
	from = X_CONFIG;
    } else {
	tseng_set_dacspeed(pScrn);
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	pTseng->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr) xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pTseng->MinClock;
    clockRanges->maxClock = pTseng->MaxClock;
    clockRanges->clockIndex = -1;      /* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = TRUE;

    pTseng->MClkInfo.Set = FALSE;
    /* Only set MemClk if appropriate for the ramdac */
    if (pTseng->MClkInfo.Programmable) {
	from = X_PROBED;
	if (pScrn->device->MemClk > 0) {
	    if ((pScrn->device->MemClk < pTseng->MClkInfo.min)
		|| (pScrn->device->MemClk > pTseng->MClkInfo.max)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "MCLK %d MHz out of range (=%d..%d); not changed!\n",
		    pScrn->device->MemClk / 1000,
		    pTseng->MClkInfo.min / 1000,
		    pTseng->MClkInfo.max / 1000);
	    } else {
		pTseng->MClkInfo.MemClk = pScrn->device->MemClk;
		pTseng->MClkInfo.Set = TRUE;
		from = X_CONFIG;
	    }
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "MCLK used is %d MHz\n",
	    pTseng->MClkInfo.MemClk / 1000);
    }
    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our TsengValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
	pScrn->display->modes, clockRanges,
	NULL, 32, 4096 / pTseng->Bytesperpixel,
	8, 0, 2048,
	pScrn->display->virtualX,
	pScrn->display->virtualY,
	pScrn->videoRam,	       /* was: pTseng->FbMapSize, */
	LOOKUP_BEST_REFRESH);

    if (i == -1) {
	TsengFreeRec(pScrn);
	return FALSE;
    }
    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	TsengFreeRec(pScrn);
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
    /* Set the required modules list */
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
	TsengFreeRec(pScrn);
	return FALSE;
    }
    /* Load XAA if needed */
    if (!pTseng->NoAccel || pTseng->HWCursor)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    TsengFreeRec(pScrn);
	    return FALSE;
	}
    return TRUE;
}

static Bool
TsengScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    TsengPtr pTseng;
    int ret;
    VisualPtr visual;
    int savedDefaultVisualClass;

    ErrorF("	TsengScreenInit\n");

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    pTseng = TsengPTR(pScrn);

    /* Map the Tseng memory areas */
    if (!TsengMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    TsengSave(pScrn);

    /* Initialise the first mode */
    TsengModeInit(pScrn, pScrn->currentMode);

    /* Darken the screen for aesthetic reasons and set the viewport */
#if TODO			       /* we want to see this now for non-aesthetic reasons */
    TsengSaveScreen(pScreen, FALSE);
#endif
    TsengAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    /* XXX Fill the screen with black */

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
    cfbClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.  To deal with this, call
     * cfbSetVisualTypes for each visual supported.
     */

    if (pScrn->bitsPerPixel > 8) {
	if (!cfbSetVisualTypes(pScrn->depth, 1 << TrueColor, pScrn->rgbBits))
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
	ret = cfbScreenInit(pScreen, pTseng->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pTseng->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pTseng->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pTseng->FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
	    "Internal error: invalid bpp (%d) in TsengScrnInit\n",
	    pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    xf86SetDefaultColorVisualClass(savedDefaultVisualClass);
    if (!ret)
	return FALSE;

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel == 8) {
	vgaHandleColormaps(pScreen, pScrn);
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
    }

    /*
     * If banking is needed, initialise an miBankInfoRec (defined in
     * "mibank.h"), and call miInitializeBanking().
     */

    if (!pTseng->UseLinMem) {
	xf86DrvMsg(scrnIndex, X_INFO, "Using banked (=non-linear) mode\n");
	if (!Is_stdET4K && (pScrn->videoRam > 1024)) {
	    pTseng->BankInfo.SetSourceBank = ET4000W32SetRead;
	    pTseng->BankInfo.SetDestinationBank = ET4000W32SetWrite;
	    pTseng->BankInfo.SetSourceAndDestinationBanks = ET4000W32SetReadWrite;
	} else {
	    pTseng->BankInfo.SetSourceBank = ET4000SetRead;
	    pTseng->BankInfo.SetDestinationBank = ET4000SetWrite;
	    pTseng->BankInfo.SetSourceAndDestinationBanks = ET4000SetReadWrite;
	}
	pTseng->BankInfo.pBankA = pTseng->FbBase;
	pTseng->BankInfo.pBankB = pTseng->FbBase;
	pTseng->BankInfo.BankSize = 0x10000;
	pTseng->BankInfo.nBankDepth = pScrn->depth;	/* depth for pixmap format for DDX */

	if (!miInitializeBanking(pScreen, pScrn->virtualX, pScrn->virtualY,
		pScrn->displayWidth, &pTseng->BankInfo))
	    return FALSE;
    }
    /*
     * Initialize the acceleration interface.
     */

    if ((!pTseng->NoAccel) && (pScrn->bitsPerPixel >= 8)) {
	int FBmem = (pScrn->virtualY * pScrn->displayWidth * pTseng->Bytesperpixel) / 1024;

	if ((pScrn->videoRam - FBmem) < 1) {
	    xf86Msg(X_WARNING, "Acceleration disabled. It requires AT LEAST 1kb of free video memory\n");
	    pTseng->NoAccel = TRUE;
	} else {
	    pScrn->videoRam -= 1;
	    tsengScratchVidBase = pScrn->videoRam * 1024;
	}

#ifdef TODO
	/*
	 * XAA ImageWrite support needs two entire line buffers + 3 and rounded
	 * up to the next DWORD boundary.
	 */
	if (tseng_use_ACL) {
	    int req_ram = (pScrn->displayWidth * pTseng->Bytesperpixel + 6) * 2;

	    req_ram = (req_ram + 1023) / 1024;	/* in kb */
	    if ((pScrn->videoRam - FBmem) > req_ram) {
		pScrn->videoRam -= req_ram;
		tsengImageWriteBase = pScrn->videoRam * 1024;
		ErrorF("%s %s: Using %dkb of unused display memory for extra acceleration functions.\n",
		    XCONFIG_PROBED, pScrn->name, req_ram);
	    }
	}
#endif
	tseng_init_acl(pScreen);       /* set up accelerator */
	if (!TsengXAAInit(pScreen)) {  /* set up XAA interface */
	    return FALSE;
	}
    }
    miInitializeBackingStore(pScreen);

#ifdef TODO
    if (!pTseng->NoAccel)
	TsengAclInit(pScreen);

    /* Initialise cursor functions */
    if (pTseng->HWCursor && 0) {
	/* Initialise HW cursor functions */
    } else
#endif
    {
	/* SW cursor */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());
    }
    /* Initialise default colourmap */
    if (!cfbCreateDefColormap(pScreen))
	return FALSE;

    /* Wrap the current CloseScreen and SaveScreen functions */
    pTseng->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = TsengCloseScreen;
    pScreen->SaveScreen = TsengSaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }
    /* Done */
    return TRUE;
}

static Bool
TsengEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    ErrorF("	TsengEnterVT\n");

    vgaHWUnlock(VGAHWPTR(pScrn));
    TsengUnlock();

    return TsengModeInit(pScrn, pScrn->currentMode);
}

static void
TsengLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    TsengPtr pTseng = TsengPTR(pScrn);

#ifdef TODO
#ifdef XFreeXDGA
    if (pScrn->bitsPerPixel >= 8) {
	if (pScrn->directMode & XF86DGADirectGraphics) {
	    if (vgaHWCursor.Initialized == TRUE)
		TsengHideCursor();
	    return;
	}
    }
#endif
#endif

    ErrorF("	TsengLeaveVT\n");
    TsengRestore(pScrn, &(VGAHWPTR(pScrn)->SavedReg), &pTseng->SavedReg);

    TsengLock();
    vgaHWLock(VGAHWPTR(pScrn));
}

static Bool
TsengCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengCloseScreen\n");

    TsengRestore(pScrn, &(VGAHWPTR(pScrn)->SavedReg), &(pTseng->SavedReg));
    TsengUnmapMem(pScrn);
    if (pTseng->AccelInfoRec)
	XAADestroyInfoRec(pTseng->AccelInfoRec);

    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = pTseng->CloseScreen;
    return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

/*
 * SaveScreen --
 *
 *   perform a sequencer reset.
 *
 * The ET4000 "Video System Configuration 1" register (CRTC index 0x36),
 * which is used to set linear memory mode and MMU-related stuff, is
 * partially reset to "0" when TS register index 0 bit 1 is set (synchronous
 * reset): bits 3..5 are reset during a sync. reset. The problem is that
 * sync reset is active during the register setup (ET4000Restore()), and
 * thus ExtCRTC[0x36] never gets written...).
 *
 * IS THIS STILL TRUE FOR ND? 
 *
 * We hook this function so that we can remember/restore ExtCRTC[0x36].
 */

static Bool
TsengSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    ErrorF("	TsengSaveScreen\n");
#ifdef TODO
#ifndef PC98_NEC480
#ifndef PC98_EGC
    if (start == SS_START) {
	if (Is_W32_any) {
	    outb(iobase + 4, 0x36);
	    save_ExtCRTC[0x36] = inb(iobase + 5);
	}
	vgaHWSaveScreen(start);
    } else {
	vgaHWSaveScreen(start);
	if (Is_W32_any) {
	    outw(iobase + 4, (save_ExtCRTC[0x36] << 8) | 0x36);
	}
    }
#endif
#endif
#endif
    return vgaHWSaveScreen(pScreen, unblank);
}

static Bool
TsengMapMem(ScrnInfoPtr pScrn)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengMapMem\n");

    /* Map the VGA memory */

    if (!vgaHWMapMem(pScrn))
	return FALSE;

    if (pTseng->UseLinMem) {
	pTseng->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
	    pTseng->PciTag,
	    (pointer) ((unsigned long)pTseng->LinFbAddress),
	    pTseng->FbMapSize);
    } else {
	pTseng->FbBase = VGAHWPTR(pScrn)->Base;
    }

    if (pTseng->FbBase == NULL)
	return FALSE;

    return TRUE;
}

static Bool
TsengUnmapMem(ScrnInfoPtr pScrn)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengUnmapMem\n");

    if (pTseng->UseLinMem) {
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pTseng->FbBase, pScrn->videoRam);
    }
    vgaHWUnmapMem(pScrn);

    pTseng->FbBase = NULL;

    return TRUE;
}

static void
TsengFreeScreen(int scrnIndex, int flags)
{
    ErrorF("	TsengFreeScreen\n");
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    TsengFreeRec(xf86Screens[scrnIndex]);
}

#define BASE_FREQ         14.31818     /* MHz */
static void
commonCalcClock(long freq, int min_m, int min_n1, int max_n1, int min_n2, int max_n2,
    long freq_min, long freq_max,
    unsigned char *mdiv, unsigned char *ndiv)
{
    double ffreq, ffreq_min, ffreq_max;
    double div, diff, best_diff;
    unsigned int m;
    unsigned char n1, n2;
    unsigned char best_n1 = 16 + 2, best_n2 = 2, best_m = 125 + 2;

    ErrorF("	commonCalcClock\n");
    ffreq = freq / 1000.0 / BASE_FREQ;
    ffreq_min = freq_min / 1000.0 / BASE_FREQ;
    ffreq_max = freq_max / 1000.0 / BASE_FREQ;

    if (ffreq < ffreq_min / (1 << max_n2)) {
	ErrorF("invalid frequency %1.3f MHz  [freq >= %1.3f MHz]\n",
	    ffreq * BASE_FREQ, ffreq_min * BASE_FREQ / (1 << max_n2));
	ffreq = ffreq_min / (1 << max_n2);
    }
    if (ffreq > ffreq_max / (1 << min_n2)) {
	ErrorF("invalid frequency %1.3f MHz  [freq <= %1.3f MHz]\n",
	    ffreq * BASE_FREQ, ffreq_max * BASE_FREQ / (1 << min_n2));
	ffreq = ffreq_max / (1 << min_n2);
    }
    /* work out suitable timings */

    best_diff = ffreq;

    for (n2 = min_n2; n2 <= max_n2; n2++) {
	for (n1 = min_n1 + 2; n1 <= max_n1 + 2; n1++) {
	    m = (int)(ffreq * n1 * (1 << n2) + 0.5);
	    if (m < min_m + 2 || m > 127 + 2)
		continue;
	    div = (double)(m) / (double)(n1);
	    if ((div >= ffreq_min) &&
		(div <= ffreq_max)) {
		diff = ffreq - div / (1 << n2);
		if (diff < 0.0)
		    diff = -diff;
		if (diff < best_diff) {
		    best_diff = diff;
		    best_m = m;
		    best_n1 = n1;
		    best_n2 = n2;
		}
	    }
	}
    }

#if EXTENDED_DEBUG
    ErrorF("Clock parameters for %1.6f MHz: m=%d, n1=%d, n2=%d\n",
	((double)(best_m) / (double)(best_n1) / (1 << best_n2)) * BASE_FREQ,
	best_m - 2, best_n1 - 2, best_n2);
#endif

    if (max_n1 == 63)
	*ndiv = (best_n1 - 2) | (best_n2 << 6);
    else
	*ndiv = (best_n1 - 2) | (best_n2 << 5);
    *mdiv = best_m - 2;
}

static Bool
TsengModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    TsengPtr pTseng = TsengPTR(pScrn);
    TsengRegPtr new = &(pTseng->ModeReg);
    TsengRegPtr initial = &(pTseng->SavedReg);
    int row_offset;
    int temp1, temp2, temp3;

    ErrorF("	TsengModeInit\n");

    /* prepare standard VGA register contents */
    hwp = VGAHWPTR(pScrn);
    if (!vgaHWInit(pScrn, mode))
	return (FALSE);
    pScrn->vtSema = TRUE;

    /* prepare extended (Tseng) register contents */
    /* 
     * Start by copying all the saved registers in the "new" data, so we
     * only have to modify those that need to change.
     */

    memcpy(new, initial, sizeof(TsengRegRec));

#ifdef TODO
    /*
     * this is a kludge, but the ET4000Validate() code should already have
     * done this, and it _has_ (supposing we call tseng_validate_mode in
     * there. I tried that, with no effect). But we seem to be getting a
     * different mode structure (a copy?) at this point.
     *
     * Another weirdness is that _here_, mode->Clock points to the clock
     * _index_ in the array off clocks (usually 2 for a programmable clock),
     * while in ET4000Validate() mode->Clock is the actual pixel clock (in
     * kHZ).
     */
    tseng_validate_mode(mode, TRUE);
#endif

    if (pScrn->bitsPerPixel < 8) {
	/* Don't ask me why this is needed on the ET6000 and not on the others */
	if (Is_ET6K)
	    hwp->ModeReg.Sequencer[1] |= 0x04;
	row_offset = hwp->ModeReg.CRTC[19];
    } else {
	hwp->ModeReg.Attribute[16] = 0x01;	/* use the FAST 256 Color Mode */
	row_offset = pScrn->displayWidth >> 3;	/* overruled by 16/24/32 bpp code */
    }

    hwp->ModeReg.CRTC[20] = 0x60;
    hwp->ModeReg.CRTC[23] = 0xAB;
    new->ExtTS[6] = 0x00;
    new->ExtTS[7] = 0xBC;
    new->ExtCRTC[0x33] = 0x00;

    new->ExtCRTC[0x35] = (mode->Flags & V_INTERLACE ? 0x80 : 0x00)
	| 0x10
	| ((mode->CrtcVSyncStart & 0x400) >> 7)
	| (((mode->CrtcVDisplay - 1) & 0x400) >> 8)
	| (((mode->CrtcVTotal - 2) & 0x400) >> 9)
	| (((mode->CrtcVBlankStart - 1) & 0x400) >> 10);

    if (pScrn->bitsPerPixel < 8)
	new->ExtATC = 0x00;
    else
	new->ExtATC = 0x80;

    if (pScrn->bitsPerPixel >= 8) {
	if (pTseng->FastDram && !Is_ET6K) {
	    /*
	     *  make sure Trsp is no more than 75ns
	     *            Tcsw is 25ns
	     *            Tcsp is 25ns
	     *            Trcd is no more than 50ns
	     * Timings assume SCLK = 40MHz
	     *
	     * Note, this is experimental, but works for me (DHD)
	     */
	    /* Tcsw, Tcsp, Trsp */
	    new->ExtCRTC[0x32] &= ~0x1F;
	    if (initial->ExtCRTC[0x32] & 0x18)
		new->ExtCRTC[0x32] |= 0x08;
	    /* Trcd */
	    new->ExtCRTC[0x32] &= ~0x20;
	}
    }
    /*
     * Here we make sure that CRTC regs 0x34 and 0x37 are untouched, except for 
     * some bits we want to change. 
     * Notably bit 7 of CRTC 0x34, which changes RAS setup time from 4 to 0 ns 
     * (performance),
     * and bit 7 of CRTC 0x37, which changes the CRTC FIFO low treshold control.
     * At really high pixel clocks, this will avoid lots of garble on the screen 
     * when something is being drawn. This only happens WAY beyond 80 MHz 
     * (those 135 MHz ramdac's...)
     */
    if (Is_W32i || Is_W32p) {
	if (!pTseng->SlowDram)
	    new->ExtCRTC[0x34] = (initial->ExtCRTC[0x34] & 0x7F) | 0x80;
	if ((mode->Clock * pTseng->Bytesperpixel) > 80000)
	    new->ExtCRTC[0x37] = (new->ExtCRTC[0x37] & 0x7f) | 0x80;
	/*
	 * now on to the memory interleave setting (CR32 bit 7)
	 */
	if (pTseng->W32InterleaveOff)
	    new->ExtCRTC[0x32] &= 0x7F;
	if (pTseng->W32InterleaveOn)
	    new->ExtCRTC[0x32] |= 0x80;
    }
    if (Is_W32p) {
	/*
	 * next, we check the PCI Burst option and turn that on or off
	 * which is done with bit 4 in CR34
	 */
	if (pTseng->PCIBurstOff)
	    new->ExtCRTC[0x34] &= 0xEF;
	if (pTseng->PCIBurstOn)
	    new->ExtCRTC[0x34] |= 0x10;
    }
#ifdef TODO
    /* prepare clock-related registers when not Legend.
     * cannot really SET the clock here yet, since the ET4000Save()
     * is called LATER, so it would save the wrong state...
     * ET4000Restore() is used to actually SET vga regs.
     */

    if (STG170x_programmable_clock || Gendac_programmable_clock) {
	/* for pixmux: must use post-div of 4 on ICS GenDAC clock generator!
	 */

	if (mode->Flags & V_PIXMUX) {
	    commonCalcClock(mode->SynthClock / 2, 1, 1, 31, 2, 3, 100000, pScrn->device->dacSpeeds[0] * 2 + 1,
		&(new->pll.f2_M), &(new->pll.f2_N));
	} else {
	    commonCalcClock(mode->SynthClock, 1, 1, 31, 0, 3, 100000, pScrn->device->dacSpeeds[0] * 2 + 1,
		&(new->pll.f2_M), &(new->pll.f2_N));
	}
	new->pll.w_idx = 0;
	new->pll.r_idx = 0;

	/* the programmed clock will be on clock index 2 */
	/* disable MCLK/2 and MCLK/4 */
	new->ExtTS[7] = (new->ExtTS[7] & 0xBE);
	/* clear CS2: we need clock #2 */
	new->ExtCRTC[0x34] = (new->ExtCRTC[0x34] & 0xFD);
	hwp->ModeReg.MiscOutReg = (hwp->ModeReg.MiscOutReg & 0xF3) | 0x08;
	mode->ClockIndex = 2;

	/* memory clock */
	if (Gendac_programmable_clock && pTseng->MClkInfo.Set) {
	    commonCalcClock(pTseng->MClkInfo.MemClk, 1, 1, 31, 1, 3, 100000, pScrn->device->dacSpeeds[0] * 2 + 1,
		&(new->pll.MClkM), &(new->pll.MClkN));
	}
    } else if (ICD2061a_programmable_clock) {
	/* the programmed clock will be on clock index 2 */
	/* disable MCLK/2 and MCLK/4 */
	new->ExtTS[7] = (new->ExtTS[7] & 0xBE);
	/* clear CS2: we need clock #2 */
	new->ExtCRTC[0x34] = (new->ExtCRTC[0x34] & 0xFD);
	hwp->ModeReg.MiscOutReg = (hwp->ModeReg.MiscOutReg & 0xF3) | 0x08;
	mode->ClockIndex = 2;
	/* FIXME: icd2061_dwv not used anywhere ... */
	pTseng->icd2061_dwv = AltICD2061CalcClock(mode->SynthClock * 1000);
	/* Tseng_ICD2061AClockSelect(mode->SynthClock); */
    } else if (CH8398_programmable_clock) {
	/* Let's call common_hw/Ch8391clk.c ! */
	if (mode->Flags & V_PIXMUX)
	    Chrontel8391CalcClock(mode->SynthClock / 2, &temp1, &temp2, &temp3);
	else
	    Chrontel8391CalcClock(mode->SynthClock, &temp1, &temp2, &temp3);
	new->pll.f2_N = (unsigned char)(temp2);
	new->pll.f2_M = (unsigned char)(temp1 | (temp3 << 6));
	if (xf86GetVerbosity())
	    ErrorF("CH8398 or CH8398A PLL set to %fMhz\n",
		14.31818 * (temp2 + 8.0) / (temp1 + 2.0) / (1 << temp3));
	/* ok LSB=f2_N and MSB=f2_M            */
	/* now set the Clock Select Register(CSR)      */
	new->pll.ctrl = (new->pll.ctrl | 0x90) & 0xF0;
	new->pll.timingctrl &= 0x1F;
	new->pll.r_idx = 0;
	new->pll.w_idx = 0;
	mode->ClockIndex = 2;
	new->ExtCRTC[0x31] &= 0x3F;
	/* clear CS2: we need clock #2 */
	new->ExtCRTC[0x34] = (new->ExtCRTC[0x34] & 0xFD);
	hwp->ModeReg.MiscOutReg = (hwp->ModeReg.MiscOutReg & 0xF3) | 0x08;
	/* disable MCLK/2 and MCLK/4, they don't seem to work in 24bpp 
	 * anyway */
	new->ExtTS[7] = (new->ExtTS[7] & 0xBE);
    } else
#endif
    if (Is_ET6K) {
	/* setting min_n2 to "1" will ensure a more stable clock ("0" is allowed though) */
	commonCalcClock(mode->Clock, 1, 1, 31, 1, 3, 100000,
	    pScrn->device->dacSpeeds[0] * 2,
	    &(new->pll.f2_M), &(new->pll.f2_N));

	/* ErrorF("M=0x%x ; N=0x%x\n",new->pll.f2_M, new->pll.f2_N); */
	/* above 130MB/sec, we enable the "LOW FIFO threshold" */
	if (mode->Clock * pTseng->Bytesperpixel > 130000) {
	    new->ExtET6K[0x41] = initial->ExtET6K[0x41] | 0x10;
	    if (Is_ET6100)
		new->ExtET6K[0x46] = initial->ExtET6K[0x46] | 0x04;
	} else {
	    new->ExtET6K[0x41] = initial->ExtET6K[0x41] & ~0x10;
	    if (Is_ET6100)
		new->ExtET6K[0x46] = initial->ExtET6K[0x46] & ~0x04;
	}

	if (pTseng->MClkInfo.Set) {
	    /* according to Tseng Labs, N1 must be <= 4, and N2 should always be 1 for MClk */
	    commonCalcClock(pTseng->MClkInfo.MemClk, 1, 1, 4, 1, 1,
		100000, pScrn->device->dacSpeeds[0] * 2,
		&(new->pll.MClkM), &(new->pll.MClkN));
	}
	/* 
	 * Even when we don't allow setting the MClk value as described
	 * above, we can use the FAST/MED/SLOW DRAM options to set up
	 * the RAS/CAS delays as decided by the value of ExtET6K[0x44].
	 * This is also a more correct use of the flags, as it describes
	 * how fast the RAM works. [HNH].
	 */
	if (pTseng->FastDram)
	    new->ExtET6K[0x44] = 0x04; /* Fastest speed(?) */
	else if (pTseng->MedDram)
	    new->ExtET6K[0x44] = 0x15; /* Medium speed */
	else if (pTseng->SlowDram)
	    new->ExtET6K[0x44] = 0x35; /* Slow speed */
	else
	    new->ExtET6K[0x44] = initial->ExtET6K[0x44];	/* keep current value */

	/* force clock #2 */
	new->ExtCRTC[0x34] = (new->ExtCRTC[0x34] & 0xFD);
	hwp->ModeReg.MiscOutReg = (hwp->ModeReg.MiscOutReg & 0xF3) | 0x08;
	mode->ClockIndex = 2;
    } else if (mode->ClockIndex >= 0) {
	/* CS2 */
	new->ExtCRTC[0x34] = (new->ExtCRTC[0x34] & 0xFD) |
	    ((mode->ClockIndex & 0x04) >> 1);
#ifndef OLD_CLOCK_SCHEME
	/* clock select bit 3 = divide-by-2 disable/enable */
	new->ExtTS[7] = (pTseng->save_divide ^ ((mode->ClockIndex & 0x08) << 3)) |
	    (new->ExtTS[7] & 0xBF);
	/* clock select bit 4 = CS3 */
	new->ExtCRTC[0x31] = ((mode->ClockIndex & 0x10) << 2) | (new->ExtCRTC[0x31] & 0x3F);
#else
	new->ExtTS[7] = (pTseng->save_divide ^ ((mode->ClockIndex & 0x10) << 2)) |
	    (new->ExtTS[7] & 0xBF);
#endif
    }
    /*
     * linear mode handling
     */

    if (Is_ET6K) {
	if (pTseng->UseLinMem) {
	    new->ExtET6K[0x13] = pTseng->LinFbAddress >> 24;
	    new->ExtET6K[0x40] |= 0x09;
	} else {
	    new->ExtET6K[0x40] &= ~0x09;
	    new->ExtET6K[0x13] = initial->ExtET6K[0x13];
	}
    } else {			       /* et4000 style linear memory */
	if (pTseng->UseLinMem) {
	    new->ExtCRTC[0x36] |= 0x10;
	    if (Is_W32p || Is_ET6K)
		new->ExtCRTC[0x30] = (pTseng->LinFbAddress >> 22) & 0xFF;
	    else
		new->ExtCRTC[0x30] = ((pTseng->LinFbAddress >> 22) & 0x1F) ^ 0x1c;	/* invert bits 4..2 */
	    hwp->ModeReg.Graphics[6] &= ~0x0C;
	    new->ExtIMACtrl &= ~0x01;  /* disable IMA port (to get >1MB lin mem) */
	} else {
	    new->ExtCRTC[0x36] &= ~0x10;
	    if (pTseng->ChipType < TYPE_ET4000W32P)
		new->ExtCRTC[0x30] = 0x1C;	/* default value */
	    else
		new->ExtCRTC[0x30] = 0x00;
	}
    }

    /*
     * 16/24/32 bpp handling.
     */

    if (pScrn->bitsPerPixel >= 8) {
	tseng_set_ramdac_bpp(pScrn, mode);
	row_offset *= pTseng->Bytesperpixel;
    }
    /*
     * Horizontal overflow settings: for modes with > 2048 pixels per line
     */

    hwp->ModeReg.CRTC[19] = row_offset;
    new->ExtCRTC[0x3F] = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8)
	| ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7)
	| ((((mode->CrtcHBlankStart >> 3) - 1) & 0x100) >> 6)
	| (((mode->CrtcHSyncStart >> 3) & 0x100) >> 4)
	| ((row_offset & 0x200) >> 3)
	| ((row_offset & 0x100) >> 1);

    /*
     * Enable memory mapped IO registers when acceleration is needed.
     */

    if (!pTseng->NoAccel) {
	if (Is_ET6K) {
	    if (pTseng->UseLinMem)
		new->ExtET6K[0x40] |= 0x02;	/* MMU can't be used here (causes system hang...) */
	    else
		new->ExtET6K[0x40] |= 0x06;	/* MMU is needed in banked accelerated mode */
	} else {
	    new->ExtCRTC[0x36] |= 0x28;
	}
    }
    vgaHWUnlock(hwp);		       /* TODO: is this needed (tsengEnterVT does this) */
    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    TsengRestore(pScrn, &hwp->ModeReg, new);
    return TRUE;
}

static Bool
TsengSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    ErrorF("	TsengSwitchMode\n");
    return TsengModeInit(xf86Screens[scrnIndex], mode);
}

/*
 * adjust the current video frame (viewport) to display the mousecursor.
 */
static void
TsengAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    TsengPtr pTseng = TsengPTR(pScrn);
    int iobase = VGAHWPTR(pScrn)->IOBase;
    int Base;

    ErrorF("	TsengAdjustFrame\n");

    if (pTseng->ShowCache) {
	if (y)
	    y += 256;
    }
    if (pScrn->bitsPerPixel < 8)
	Base = (y * pScrn->displayWidth + x + 3) >> 3;
    else {
	Base = ((y * pScrn->displayWidth + x + 1) * pTseng->Bytesperpixel) >> 2;
	/* adjust Base address so it is a non-fractional multiple of pTseng->Bytesperpixel */
	Base -= (Base % pTseng->Bytesperpixel);
    }

    outw(iobase + 4, (Base & 0x00FF00) | 0x0C);
    outw(iobase + 4, ((Base & 0x00FF) << 8) | 0x0D);
    outw(iobase + 4, ((Base & 0x0F0000) >> 8) | 0x33);

#ifdef TODO
#ifdef XFreeXDGA
    if (pScrn->directMode & XF86DGADirectGraphics) {
	/* Wait until vertical retrace is in progress. */
	while (inb(iobase + 0xA) & 0x08) ;
	while (!(inb(iobase + 0xA) & 0x08)) ;
    }
#endif
#endif
}

ModeStatus
TsengValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
#define Tseng_HMAX (4096-8)
#define Tseng_VMAX (2048-1)
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    TsengPtr pTseng = TsengPTR(pScrn);

    ErrorF("	TsengValidMode\n");
    /*
     * Check for pixmux modes. We try not to change the mode clock here
     * (although that would be the easiest way), because that confuses all the
     * rest of the code. E.g. clock limit checking is done outside this
     * driver, and it would be made to think the clock is only half of what it
     * actually is, and thus allow clocks that should be banned.
     *
     * For the time being, cards with PIXMUX capabilities but without
     * programmable clockchips use this "bad" method until a better solution
     * is found.
     */
    if ((pTseng->DacInfo.pixMuxPossible) &&
	(mode->Clock > pTseng->DacInfo.nonMuxMaxClock)
    /* && (!(mode->Flags & V_INTERLACE)) *//* PIXMUX+interlace seem to work now */
	) {
	mode->Flags |= V_PIXMUX;
	mode->Flags |= V_DBLCLK;
	if (verbose)
	    xf86Msg(X_PROBED, "Mode \"%s\" will use pixel multiplexing.\n", mode->name);
	if (!pScrn->progClock) {
	    mode->Clock /= 2;
	    if (verbose)
		xf86Msg(X_PROBED, " (clock will be reported as 1/2 of clock in modeline)\n");
	}
    }
    /* Check for CRTC timing bits overflow. */
    if (mode->HTotal > Tseng_HMAX) {
	if (verbose)
	    xf86Msg(X_ERROR, "Horizontal mode timing overflow (=%d, max=%d)\n",
		mode->HTotal, Tseng_HMAX);
	return MODE_BAD;
    }
    if (mode->VTotal > Tseng_VMAX) {
	if (verbose)
	    xf86Msg(X_ERROR, "Vertical mode timing overflow (=%d, max=%d)\n",
		mode->VTotal, Tseng_VMAX);
	return MODE_BAD;
    }
    return MODE_OK;
}

/*
 * TsengSave --
 *      save the current video mode
 */

static void
TsengSave(ScrnInfoPtr pScrn)
{
    unsigned char temp, saveseg1 = 0, saveseg2 = 0;
    TsengPtr pTseng = TsengPTR(pScrn);
    vgaRegPtr vgaReg;
    TsengRegPtr tsengReg;
    int iobase = VGAHWPTR(pScrn)->IOBase;

    ErrorF("	TsengSave\n");

    vgaReg = &VGAHWPTR(pScrn)->SavedReg;
    tsengReg = &pTseng->SavedReg;

    /*
     * This function will handle creating the data structure and filling
     * in the generic VGA portion.
     */
    vgaHWSave(pScrn, vgaReg, TRUE);

    /*
     * we need this here , cause we MUST disable the ROM SYNC feature
     * this bit changed with W32p_rev_c...
     */
    outb(iobase + 4, 0x34);
    temp = inb(iobase + 5);
    if (Is_stdET4K || Is_W32_W32i || Is_W32p_ab) {
	outb(iobase + 5, temp & 0x1F);
    }
    tsengReg->ExtCRTC[0x34] = temp;

    saveseg1 = inb(0x3CD);
    outb(0x3CD, 0x00);		       /* segment select 1 */
    if (!Is_stdET4K) {
	saveseg2 = inb(0x3CB);
	outb(0x3CB, 0x00);	       /* segment select 2 */
    }
    tsengReg->ExtSegSel[0] = saveseg1;
    tsengReg->ExtSegSel[1] = saveseg2;

    outb(iobase + 4, 0x33);
    tsengReg->ExtCRTC[0x33] = inb(iobase + 5);
    outb(iobase + 4, 0x35);
    tsengReg->ExtCRTC[0x35] = inb(iobase + 5);
    if (Is_W32_any) {
	outb(iobase + 4, 0x36);
	tsengReg->ExtCRTC[0x36] = inb(iobase + 5);
	outb(iobase + 4, 0x37);
	tsengReg->ExtCRTC[0x37] = inb(iobase + 5);
	outb(0x217a, 0xF7);
	tsengReg->ExtIMACtrl = inb(0x217b);
    }
    if (!Is_ET6K) {
	outb(iobase + 4, 0x32);
	tsengReg->ExtCRTC[0x32] = inb(iobase + 5);
    }
    outb(0x3C4, 6);
    tsengReg->ExtTS[6] = inb(0x3C5);
    outb(0x3C4, 7);
    tsengReg->ExtTS[7] = inb(0x3C5);
    tsengReg->ExtTS[7] |= 0x14;
    temp = inb(iobase + 0x0A);	       /* reset flip-flop */
    outb(0x3C0, 0x36);
    tsengReg->ExtATC = inb(0x3C1);
    outb(0x3C0, tsengReg->ExtATC);
#ifdef TODO
    if (DAC_is_GenDAC) {
	/* Save GenDAC Command and PLL registers */
	outb(iobase + 4, 0x31);
	temp = inb(iobase + 5);
	outb(iobase + 5, temp | 0x40);

	tsengReg->pll.cmd_reg = inb(0x3c6);	/* Enhanced command register */
	if (Gendac_programmable_clock) {
	    tsengReg->pll.w_idx = inb(0x3c8);	/* PLL write index */
	    tsengReg->pll.r_idx = inb(0x3c7);	/* PLL read index */
	    outb(0x3c7, 2);	       /* index to f2 reg */
	    tsengReg->pll.f2_M = inb(0x3c9);	/* f2 PLL M divider */
	    tsengReg->pll.f2_N = inb(0x3c9);	/* f2 PLL N1/N2 divider */
	    outb(0x3c7, 10);	       /* index to Mclk reg */
	    tsengReg->MClkM = inb(0x3c9);	/* MClk PLL M divider */
	    tsengReg->MClkN = inb(0x3c9);	/* MClk PLL N1/N2 divider */
	    outb(0x3c7, 0x0e);	       /* index to PLL control */
	    tsengReg->pll.ctrl = inb(0x3c9);	/* PLL control */
	}
	outb(iobase + 4, 0x31);
	outb(iobase + 5, temp & ~0x40);
    }
    if ((pTseng->DacInfo.DacType == STG1702_DAC) || (pTseng->DacInfo.DacType == STG1703_DAC)
	|| (pTseng->DacInfo.DacType == STG1700_DAC)) {
	/* Save STG 1703 GenDAC Command and PLL registers 
	 * unfortunately we reuse the gendac data structure, so the 
	 * field names are not really good.
	 */

	xf86dactopel();
	tsengReg->pll.cmd_reg = xf86getdaccomm();	/* Enhanced command register */
	if (STG170x_programmable_clock) {
	    tsengReg->pll.f2_M = STG1703getIndex(0x24);		/* f2 PLL M divider */
	    tsengReg->pll.f2_N = inb(0x3c6);	/* f2 PLL N1/N2 divider */
	}
	tsengReg->pll.ctrl = STG1703getIndex(0x03);	/* pixel mode select control */
	tsengReg->pll.timingctrl = STG1703getIndex(0x05);	/* pll timing control */
    }
    if (pTseng->DacInfo.DacType == CH8398_DAC) {
	xf86dactopel();
	tsengReg->pll.cmd_reg = xf86getdaccomm();
    }
    if (CH8398_programmable_clock) {
	inb(0x3c8);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	tsengReg->pll.timingctrl = inb(0x3c6);
	/* Save PLL */
	outb(iobase + 4, 0x31);
	temp = inb(iobase + 5);
	outb(iobase + 5, temp | (1 << 6));	/* set RS2 through CS3 */
	/* We are in ClockRAM mode 0x3c7 = CRA, 0x3c8 = CWA, 0x3c9 = CDR */
	tsengReg->pll.r_idx = inb(0x3c7);
	tsengReg->pll.w_idx = inb(0x3c8);
	outb(0x3c7, 10);
	tsengReg->pll.f2_N = inb(0x3c9);
	tsengReg->pll.f2_M = inb(0x3c9);
	outb(0x3c7, tsengReg->pll.r_idx);
	inb(0x3c8);		       /* loop to Clock Select Register */
	inb(0x3c8);
	inb(0x3c8);
	inb(0x3c8);
	tsengReg->pll.ctrl = inb(0x3c8);
	outb(iobase + 4, 0x31);
	outb(iobase + 5, temp);
    }
#endif
    if (ET6000_programmable_clock) {
	/* Save ET6000 CLKDAC PLL registers */
	temp = inb(pTseng->IOAddress + 0x67);	/* remember old CLKDAC index register pointer */
	outb(pTseng->IOAddress + 0x67, 2);
	tsengReg->pll.f2_M = inb(pTseng->IOAddress + 0x69);
	tsengReg->pll.f2_N = inb(pTseng->IOAddress + 0x69);
	/* save MClk values */
	outb(pTseng->IOAddress + 0x67, 10);
	tsengReg->pll.MClkM = inb(pTseng->IOAddress + 0x69);
	tsengReg->pll.MClkN = inb(pTseng->IOAddress + 0x69);
	/* restore old index register */
	outb(pTseng->IOAddress + 0x67, temp);
    }
    if (DAC_IS_ATT49x)
	tsengReg->ATTdac_cmd = tseng_getdaccomm();

    if (Is_ET6K) {
	tsengReg->ExtET6K[0x13] = inb(pTseng->IOAddress + 0x13);
	tsengReg->ExtET6K[0x40] = inb(pTseng->IOAddress + 0x40);
	tsengReg->ExtET6K[0x58] = inb(pTseng->IOAddress + 0x58);
	tsengReg->ExtET6K[0x41] = inb(pTseng->IOAddress + 0x41);
	tsengReg->ExtET6K[0x44] = inb(pTseng->IOAddress + 0x44);
	tsengReg->ExtET6K[0x46] = inb(pTseng->IOAddress + 0x46);
    }
    outb(iobase + 4, 0x30);
    tsengReg->ExtCRTC[0x30] = inb(iobase + 5);
    outb(iobase + 4, 0x31);
    tsengReg->ExtCRTC[0x31] = inb(iobase + 5);
    outb(iobase + 4, 0x3F);
    tsengReg->ExtCRTC[0x3F] = inb(iobase + 5);
}

/*
 * TsengRestore --
 *      restore a video mode
 */

static void
TsengRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, TsengRegPtr tsengReg)
{
    vgaHWPtr hwp;
    TsengPtr pTseng;
    unsigned char tmp;
    int iobase = VGAHWPTR(pScrn)->IOBase;

    ErrorF("	TsengRestore\n");

    hwp = VGAHWPTR(pScrn);
    pTseng = TsengPTR(pScrn);

    vgaHWProtect(pScrn, TRUE);

    outb(0x3CD, 0x00);		       /* segment select bits 0..3 */
    if (!Is_stdET4K)
	outb(0x3CB, 0x00);	       /* segment select bits 4,5 */

#if TODO
    if (DAC_is_GenDAC) {
	/* Restore GenDAC Command and PLL registers */
	outb(iobase + 4, 0x31);
	tmp = inb(iobase + 5);
	outb(iobase + 5, tmp | 0x40);

	outb(0x3c6, tsengReg->pll.cmd_reg);	/* Enhanced command register */

	if (Gendac_programmable_clock) {
	    outb(0x3c8, 2);	       /* index to f2 reg */
	    outb(0x3c9, tsengReg->pll.f2_M);	/* f2 PLL M divider */
	    outb(0x3c9, tsengReg->pll.f2_N);	/* f2 PLL N1/N2 divider */

	    if (pTseng->MClkInfo.Set) {
		outb(0x3c7, 10);       /* index to Mclk reg */
		outb(0x3c9, tsengReg->MClkM);	/* MClk PLL M divider */
		outb(0x3c9, tsengReg->MClkN);	/* MClk PLL N1/N2 divider */
	    }
	    outb(0x3c8, 0x0e);	       /* index to PLL control */
	    outb(0x3c9, tsengReg->pll.ctrl);	/* PLL control */
	    outb(0x3c8, tsengReg->pll.w_idx);	/* PLL write index */
	    outb(0x3c7, tsengReg->pll.r_idx);	/* PLL read index */
	}
	outb(iobase + 4, 0x31);
	outb(iobase + 5, i & ~0x40);
    }
    if (DAC_is_STG170x) {
	/* Restore STG 170x GenDAC Command and PLL registers 
	 * we share one data structure with the gendac code, so the names
	 * are not too good.
	 */

	if (STG170x_programmable_clock) {
	    STG1703setIndex(0x24, tsengReg->pll.f2_M);
	    outb(0x3c6, tsengReg->pll.f2_N);	/* use autoincrement */
	}
	STG1703setIndex(0x03, tsengReg->pll.ctrl);	/* primary pixel mode */
	outb(0x3c6, tsengReg->pll.ctrl);	/* secondary pixel mode */
	outb(0x3c6, tsengReg->pll.timingctrl);	/* pipeline timing control */
	usleep(500);		       /* 500 usec PLL settling time required */

	STG1703magic(0);
	xf86dactopel();
	tseng_setdaccomm(tsengReg->pll.cmd_reg);	/* write enh command reg */
    }
    if (pTseng->DacInfo.DacType == CH8398_DAC) {
	xf86dactopel();
	tseng_setdaccomm(tsengReg->pll.cmd_reg);
    }
    if (CH8398_programmable_clock) {
	inb(0x3c8);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	inb(0x3c6);
	outb(0x3c6, tsengReg->pll.timingctrl);
	outb(iobase + 4, 0x31);
	i = inb(iobase + 5);
	outb(iobase + 5, i | (1 << 6));		/* Set RS2 through CS3 */
	/* We are in ClockRAM mode 0x3c7 = CRA, 0x3c8 = CWA, 0x3c9 = CDR */
	outb(0x3c7, tsengReg->pll.r_idx);
	outb(0x3c8, 10);
	outb(0x3c9, tsengReg->pll.f2_N);
	outb(0x3c9, tsengReg->pll.f2_M);
	outb(0x3c8, tsengReg->pll.w_idx);
	usleep(500);
	inb(0x3c7);		       /* reset sequence */
	inb(0x3c8);		       /* loop to Clock Select Register */
	inb(0x3c8);
	inb(0x3c8);
	inb(0x3c8);
	outb(0x3c8, tsengReg->pll.ctrl);
	outb(iobase + 4, 0x31);
	outb(iobase + 5, (i & 0x3F));
	/* Make sure CS3 isn't set */
    }
#endif
    if (ET6000_programmable_clock) {
	/* Restore ET6000 CLKDAC PLL registers */
	tmp = inb(pTseng->IOAddress + 0x67);	/* remember old CLKDAC index register pointer */
	outb(pTseng->IOAddress + 0x67, 2);
	outb(pTseng->IOAddress + 0x69, tsengReg->pll.f2_M);
	outb(pTseng->IOAddress + 0x69, tsengReg->pll.f2_N);
	/* set MClk values if needed, but don't touch them if not needed */
	if (pTseng->MClkInfo.Set) {
	    /*
	     * Since setting the MClk to highly illegal value results in a
	     * total system crash, we'd better play it safe here.
	     * N1 must be <= 4, and N2 should always be 1
	     */
	    if ((tsengReg->pll.MClkN & 0xf8) != 0x20) {
		xf86Msg(X_ERROR, "Internal Error in MClk registers: MClkM=0x%x, MClkN=0x%x\n",
		    tsengReg->pll.MClkM, tsengReg->pll.MClkN);
	    } else {
		outb(pTseng->IOAddress + 0x67, 10);
		outb(pTseng->IOAddress + 0x69, tsengReg->pll.MClkM);
		outb(pTseng->IOAddress + 0x69, tsengReg->pll.MClkN);
	    }
	}
	/* restore old index register */
	outb(pTseng->IOAddress + 0x67, tmp);
    }
    if (DAC_IS_ATT49x)
	tseng_setdaccomm(tsengReg->ATTdac_cmd);

    if (Is_ET6K) {
	outb(pTseng->IOAddress + 0x13, tsengReg->ExtET6K[0x13]);
	outb(pTseng->IOAddress + 0x40, tsengReg->ExtET6K[0x40]);
	outb(pTseng->IOAddress + 0x58, tsengReg->ExtET6K[0x58]);
	outb(pTseng->IOAddress + 0x41, tsengReg->ExtET6K[0x41]);
	outb(pTseng->IOAddress + 0x44, tsengReg->ExtET6K[0x44]);
	outb(pTseng->IOAddress + 0x46, tsengReg->ExtET6K[0x46]);
    }
    outw(iobase + 4, (tsengReg->ExtCRTC[0x3F] << 8) | 0x3F);
    outw(iobase + 4, (tsengReg->ExtCRTC[0x30] << 8) | 0x30);
    outw(iobase + 4, (tsengReg->ExtCRTC[0x31] << 8) | 0x31);

    vgaHWRestore(pScrn, vgaReg, TRUE); /* TODO: does this belong HERE, in the middle? */

    outw(0x3C4, (tsengReg->ExtTS[6] << 8) | 0x06);
    outw(0x3C4, (tsengReg->ExtTS[7] << 8) | 0x07);
    tmp = inb(iobase + 0x0A);	       /* reset flip-flop */
    outb(0x3C0, 0x36);
    outb(0x3C0, tsengReg->ExtATC);
    outw(iobase + 4, (tsengReg->ExtCRTC[0x33] << 8) | 0x33);
#ifdef TODO
    if (tsengReg->std.ClockIndex >= 0)
	outw(iobase + 4, (tsengReg->ExtCRTC[0x34] << 8) | 0x34);
#endif
    outw(iobase + 4, (tsengReg->ExtCRTC[0x35] << 8) | 0x35);
#ifdef TODO
    if (Is_W32_any) {
	outw(iobase + 4, (tsengReg->ExtCRTC[0x36] << 8) | 0x36);
	/* 
	 * We must also save ExtCRTC[0x36] in save_ExtCRTC[0x36], because we are at
	 * this moment in the middle of a sync reset, and we will have
	 * saved the OLD value, which we want to change now.
	 */
	save_ExtCRTC[0x36] = tsengReg->ExtCRTC[0x36];
	outw(iobase + 4, (tsengReg->ExtCRTC[0x37] << 8) | 0x37);
	outw(0x217a, (tsengReg->ExtIMACtrl << 8) | 0xF7);
    }
#endif
    if (!Is_ET6K) {
	outw(iobase + 4, (tsengReg->ExtCRTC[0x32] << 8) | 0x32);
    }
    outb(0x3CD, tsengReg->ExtSegSel[0]);
    if (pTseng->ChipType > TYPE_ET4000)
	outb(0x3CB, tsengReg->ExtSegSel[1]);

#ifdef TODO
    /*
     * This might be required for the Legend clock setting method, but
     * should not be used for the "normal" case because the high order
     * bits are not set in ClockIndex when returning to text mode.
     */
    if (OFLG_ISSET(OPTION_LEGEND, &vga256InfoRec.options))
	if (tsengReg->std.ClockIndex >= 0) {
	    vgaProtect(TRUE);
	    (ClockSelect) (tsengReg->std.ClockIndex);
	}
#endif
#ifdef TODO
    if (tseng_use_ACL & (tsengReg->std.Attribute[16] & 1))	/* are we going to graphics mode? */
	tseng_init_acl();	       /* initialize acceleration hardware */
#endif

    vgaHWProtect(pScrn, FALSE);
}

/* 
 * The rest below is stuff from the old driver, which still needs to be
 * checked and integrated in the ND
 */

#ifdef OLD_DRIVER

#include "tseng_cursor.h"
extern vgaHWCursorRec vgaHWCursor;

/*
 * ET4000Probe --
 *      check whether a Et4000 based board is installed
 */

static Bool
ET4000Probe()
{
    int numClocks;
    Bool autodetect = TRUE;

    ...

	if (pScrn->bitsPerPixel >= 8) {

	...

	/*
	 * Acceleration is only supported on W32 or newer chips.
	 *
	 * Also, some bus configurations only allow for a 1MB linear memory
	 * aperture instead of the default 4M aperture used on all Tseng devices.
	 * If acceleration is also enabled, you only get 512k + (with some aperture
	 * tweaking) 2*128k for a total of max 768 kb of memory. This just isn't
	 * worth having a lot of conditionals in the accelerator code (the
	 * memory-mapped registers move to the top of the 1M aperture), so we
	 * simply don't allow acceleration and linear mode combined on these cards.
	 * 
	 */

	    if ((pTseng->ChipType < TYPE_ET4000W32) || (pTseng->Linmem_1meg && pTseng->UseLinMem)) {
	    tseng_use_ACL = FALSE;
	} else {
	    /* enable acceleration-related options */
	    OFLG_SET(OPTION_NOACCEL, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_PCI_RETRY, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_SHOWCACHE, &TSENG.ChipOptionFlags);

	    tseng_use_ACL = !OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options);
	}

	...

	/* Hardware Cursor support */
#ifdef W32_HW_CURSOR_FIXED
	    if (pTseng->ChipType >= TYPE_ET4000W32P)
#else
	    if (Is_ET6K)
#endif
	{
	    /* Set HW Cursor option valid */
	    OFLG_SET(OPTION_HW_CURSOR, &TSENG.ChipOptionFlags);
	}
    }
    /* if (pScrn->bitsPerPixel >= 8) */
    else {
	OFLG_CLR(OPTION_HW_CURSOR, &vga256InfoRec.options);
	pTseng->UseLinMem = FALSE;
	tseng_use_ACL = FALSE;
    }

    if (!Is_ET6K) {
	/* Initialize option flags allowed for this driver */
	OFLG_SET(OPTION_LEGEND, &TSENG.ChipOptionFlags);
	OFLG_SET(OPTION_HIBIT_HIGH, &TSENG.ChipOptionFlags);
	OFLG_SET(OPTION_HIBIT_LOW, &TSENG.ChipOptionFlags);
	if (pScrn->bitsPerPixel >= 8) {
	    OFLG_SET(OPTION_PCI_BURST_ON, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_PCI_BURST_OFF, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_W32_INTERLEAVE_ON, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_W32_INTERLEAVE_OFF, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_SLOW_DRAM, &TSENG.ChipOptionFlags);
	    OFLG_SET(OPTION_FAST_DRAM, &TSENG.ChipOptionFlags);
	}
/*
 * because of some problems with W32 cards, SLOW_DRAM is _always_ enabled
 * for those cards
 */
	if (pTseng->ChipType <= TYPE_ET4000W32) {
	    ErrorF("%s %s: option \"slow_dram\" is enabled by default on this card.\n",
		XCONFIG_PROBED, vga256InfoRec.name);
	    OFLG_SET(OPTION_SLOW_DRAM, &vga256InfoRec.options);
	}
	if (pScrn->progClock) {
	    if (Gendac_programmable_clock) {
		ClockSelect = Tseng_GenDACClockSelect;
		numClocks = 3;
	    } else if (STG170x_programmable_clock) {
		ClockSelect = Tseng_STG1703ClockSelect;
		numClocks = 3;
	    } else if (ICD2061a_programmable_clock) {
		ClockSelect = Tseng_ICD2061AClockSelect;
		numClocks = 3;
	    } else if (CH8398_programmable_clock) {
		ClockSelect = Tseng_ET4000ClockSelect;
		numClocks = 3;
	    } else {
		ErrorF("Unsuported programmable Clock chip.\n");
		ET4000EnterLeave(LEAVE);
		return (FALSE);
	    }
	} else {
	    if (OFLG_ISSET(OPTION_LEGEND, &vga256InfoRec.options)) {
		ClockSelect = Tseng_LegendClockSelect;
		numClocks = 32;
	    } else {
		ClockSelect = Tseng_ET4000ClockSelect;
		/*
		 * The CH8398 RAMDAC uses CS3 for register selection (RS2), not for clock selection.
		 * The GenDAC family only has 8 clocks. Together with MCLK/2, that's 16 clocks.
		 */
		if ((pTseng->ChipType > TYPE_ET4000)
		    && (!DAC_is_GenDAC) && (pTseng->DacInfo.DacType != CH8398_DAC))
		    numClocks = 32;
		else
		    numClocks = 16;
	    }
	}

	/* Save initial ExtCRTC[0x32] value */
	outb(iobase + 4, 0x32);
	initialExtCRTC[0x32] = inb(iobase + 5);
    }				       /* pTseng->ChipType < ET6000 */
    if (pTseng->ChipType > TYPE_ET4000) {
	/* Save initial Auxctrl (CRTC 0x34) value */
	outb(iobase + 4, 0x34);
	initialExtCRTC[0x34] = inb(iobase + 5);
	if (!Is_ET6K) {
	    /* Save initial ExtCRTC[0x36] (CRTC 0x36) value */
	    outb(iobase + 4, 0x36);
	    initialExtCRTC[0x36] = inb(iobase + 5);
	    /* Save initial ExtCRTC[0x37] (CRTC 0x37) value */
	    outb(iobase + 4, 0x37);
	    initialExtCRTC[0x37] = inb(iobase + 5);
	    /* Save initial ExtIMACtrl value */
	    outb(0x217a, 0xF7);
	    initialExtIMACtrl = inb(0x217b);
	}
    }
    if (Is_ET6K) {
	int tmp = inb(pTseng->IOAddress + 0x67);

	...

	    outb(pTseng->IOAddress + 0x67, 10);
	initialET6KMclkM = inb(pTseng->IOAddress + 0x69);
	initialET6KMclkN = inb(pTseng->IOAddress + 0x69);
	outb(pTseng->IOAddress + 0x67, tmp);
	ErrorF("%s %s: %s: MClk: %3.2f MHz, R/C: 0x%x\n", XCONFIG_PROBED,
	    vga256InfoRec.name, vga256InfoRec.chipset,
	    gendacMNToClock(initialET6KMclkM, initialET6KMclkN) / 1000.0,
	    initialExtET6K[0x44]);
	OFLG_SET(OPTION_SLOW_DRAM, &TSENG.ChipOptionFlags);
	OFLG_SET(OPTION_MED_DRAM, &TSENG.ChipOptionFlags);
	OFLG_SET(OPTION_FAST_DRAM, &TSENG.ChipOptionFlags);
    }
    if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions)) {
	if (!vga256InfoRec.clocks)
	    vgaGetClocks(numClocks, ClockSelect);
    }
    vga256InfoRec.bankedMono = TRUE;
#ifdef XFreeXDGA
    if (pScrn->bitsPerPixel >= 8)
	vga256InfoRec.directMode = XF86DGADirectPresent;
#endif

#ifdef DPMSExtension
    /* Support for DPMS, the ET4000W32Pc and newer uses a different and
     * simpler method than the older cards.
     */
    if (pTseng->ChipType >= TYPE_ET4000W32Pc) {
	vga256InfoRec.DPMSSet = TsengCrtcDPMSSet;
    } else {
	vga256InfoRec.DPMSSet = TsengHVSyncDPMSSet;
    }
#endif

    return (TRUE);
}

/*
 * ET4000FbInit --
 *      initialise the cfb SpeedUp functions
 */

static void
ET4000FbInit()
{
    int useSpeedUp;
    int FBmem = (vga256InfoRec.virtualY * pScrn->displayWidth * pTseng->Bytesperpixel) / 1024;

    if (pScrn->bitsPerPixel < 8)
	return;

    if (xf86GetVerbosity() && pTseng->UseLinMem)
	ErrorF("%s %s: %s: Using linear framebuffer at 0x%08X.\n",
	    XCONFIG_PROBED, vga256InfoRec.name,
	    vga256InfoRec.chipset, TSENG.ChipLinearBase);

    if (pScrn->videoRam > 1024)
	useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYCHIPSET;
    else
	useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYWIDTH;
    if (useSpeedUp && xf86GetVerbosity()) {
	ErrorF("%s %s: ET4000: SpeedUps selected (Flags=0x%x)\n",
	    OFLG_ISSET(XCONFIG_SPEEDUP, &vga256InfoRec.xconfigFlag) ?
	    XCONFIG_GIVEN : XCONFIG_PROBED,
	    vga256InfoRec.name, useSpeedUp);
    }
    if (!pTseng->UseLinMem) {
	if (useSpeedUp & SPEEDUP_FILLRECT) {
	    vga256LowlevFuncs.fillRectSolidCopy = speedupvga256FillRectSolidCopy;
	}
	if (useSpeedUp & SPEEDUP_BITBLT) {
	    vga256LowlevFuncs.doBitbltCopy = speedupvga256DoBitbltCopy;
	}
	if (useSpeedUp & SPEEDUP_LINE) {
	    vga256LowlevFuncs.lineSS = speedupvga256LineSS;
	    vga256LowlevFuncs.segmentSS = speedupvga256SegmentSS;
	    vga256TEOps1Rect.Polylines = speedupvga256LineSS;
	    vga256TEOps1Rect.PolySegment = speedupvga256SegmentSS;
	    vga256TEOps.Polylines = speedupvga256LineSS;
	    vga256TEOps.PolySegment = speedupvga256SegmentSS;
	    vga256NonTEOps1Rect.Polylines = speedupvga256LineSS;
	    vga256NonTEOps1Rect.PolySegment = speedupvga256SegmentSS;
	    vga256NonTEOps.Polylines = speedupvga256LineSS;
	    vga256NonTEOps.PolySegment = speedupvga256SegmentSS;
	}
	if (useSpeedUp & SPEEDUP_FILLBOX) {
	    vga256LowlevFuncs.fillBoxSolid = speedupvga256FillBoxSolid;
	}
    }
    /* Hardware cursor setup */
    if (OFLG_ISSET(OPTION_HW_CURSOR, &vga256InfoRec.options) &&
#ifdef W32_HW_CURSOR_FIXED
	(pTseng->ChipType >= TYPE_ET4000W32P)
#else
	(Is_ET6K)
#endif
	) {
	if ((pScrn->videoRam - FBmem) < 1) {
	    ErrorF("%s %s: Hardware cursor disabled. It requires 1kb of free video memory\n",
		XCONFIG_PROBED, vga256InfoRec.name);
	    OFLG_CLR(OPTION_HW_CURSOR, &vga256InfoRec.options);
	} else {
	    pScrn->videoRam -= 1;      /* 1kb reserved for hardware cursor */
	    tsengCursorAddress = pScrn->videoRam * 1024;

	    tsengCursorWidth = 64;
	    tsengCursorHeight = 64;
	    vgaHWCursor.Initialized = TRUE;
	    vgaHWCursor.Init = TsengCursorInit;
	    vgaHWCursor.Restore = TsengRestoreCursor;
	    vgaHWCursor.Warp = TsengWarpCursor;
	    vgaHWCursor.QueryBestSize = TsengQueryBestSize;
	    if (xf86GetVerbosity())
		ErrorF("%s %s: %s: Using hardware cursor\n",
		    XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
	}
    }
    if (OFLG_ISSET(OPTION_PCI_RETRY, &vga256InfoRec.options)) {
	ErrorF("%s %s: Using PCI retrys.\n", XCONFIG_PROBED, vga256InfoRec.name);
	tseng_use_PCI_Retry = 1;
    }
    if ((pScrn->bitsPerPixel >= 8) && (tseng_use_ACL)) {
	if ((pScrn->videoRam - FBmem) < 1) {
	    ErrorF("%s %s: Acceleration disabled. It requires AT LEAST 1kb of free video memory\n",
		XCONFIG_PROBED, vga256InfoRec.name);
	    tseng_use_ACL = FALSE;
	} else {
	    pScrn->videoRam -= 1;
	    tsengScratchVidBase = pScrn->videoRam * 1024;
	    /* initialize the XAA interface software */
	    /* TsengAccelInit();
	     * This relies on variables that are setup later, so it's called there */
	}

	/*
	 * XAA ImageWrite support needs two entire line buffers + 3 and rounded
	 * up to the next DWORD boundary.
	 */
	if (tseng_use_ACL) {
	    int req_ram = (pScrn->displayWidth * pTseng->Bytesperpixel + 6) * 2;

	    req_ram = (req_ram + 1023) / 1024;	/* in kb */
	    if ((pScrn->videoRam - FBmem) > req_ram) {
		pScrn->videoRam -= req_ram;
		tsengImageWriteBase = pScrn->videoRam * 1024;
		ErrorF("%s %s: Using %dkb of unused display memory for extra acceleration functions.\n",
		    XCONFIG_PROBED, vga256InfoRec.name, req_ram);
	    }
	}
    }
}

#endif
