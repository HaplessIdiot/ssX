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
 *           Dirk Hohndel, <hohndel@suse.de>
 *	     Stefan Dirsch, <sndirsch@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/glint_driver.c,v 1.32 1999/03/28 15:32:37 dawes Exp $ */

#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb24_32.h"
#include "cfb8_32.h"
#include "xf1bpp.h"
#include "xf4bpp.h"
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

#include "glint_regs.h"
#include "IBM.h"
#include "glint.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "globals.h"
#define DPMS_SERVER
#include "extensions/dpms.h"
#endif

static void	GLINTIdentify(int flags);
static Bool	GLINTProbe(DriverPtr drv, int flags);
static Bool	GLINTPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	GLINTScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	GLINTEnterVT(int scrnIndex, int flags);
static void	GLINTLeaveVT(int scrnIndex, int flags);
static Bool	GLINTCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	GLINTSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool	GLINTSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	GLINTAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	GLINTFreeScreen(int scrnIndex, int flags);
static int	GLINTValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);

/* Internally used functions */
static Bool	GLINTMapMem(ScrnInfoPtr pScrn);
static Bool	GLINTUnmapMem(ScrnInfoPtr pScrn);
static void	GLINTSave(ScrnInfoPtr pScrn);
static void	GLINTRestore(ScrnInfoPtr pScrn);
static Bool	GLINTModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;
 
#define VERSION 4000
#define GLINT_NAME "GLINT"
#define GLINT_DRIVER_NAME "glint"
#define GLINT_MAJOR_VERSION 1
#define GLINT_MINOR_VERSION 0
#define GLINT_PATCHLEVEL 0

/* 
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the SetupProc
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

DriverRec GLINT = {
    VERSION,
    "accelerated driver for 3dlabs and derived chipsets",
    GLINTIdentify,
    GLINTProbe,
    NULL,
    0
};

static SymTabRec GLINTChipsets[] = {
    { PCI_VENDOR_TI_CHIP_PERMEDIA2,		"ti_pm2" },
    { PCI_VENDOR_TI_CHIP_PERMEDIA,		"ti_pm" },
    { PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V,	"pm2v" },
    { PCI_VENDOR_3DLABS_CHIP_PERMEDIA2,		"pm2" },
    { PCI_VENDOR_3DLABS_CHIP_PERMEDIA,		"pm" },
    { PCI_VENDOR_3DLABS_CHIP_500TX,		"500tx" },
    { PCI_VENDOR_3DLABS_CHIP_MX,		"mx" },
    { -1,					NULL }
};

static PciChipsets GLINTPciChipsets[] = {
    { PCI_VENDOR_TI_CHIP_PERMEDIA2,	 PCI_VENDOR_TI_CHIP_PERMEDIA2,	    RES_SHARED_VGA },
    { PCI_VENDOR_TI_CHIP_PERMEDIA,	 PCI_VENDOR_TI_CHIP_PERMEDIA,	    RES_NONE },
    { PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V, PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V, RES_SHARED_VGA },
    { PCI_VENDOR_3DLABS_CHIP_PERMEDIA2,	 PCI_VENDOR_3DLABS_CHIP_PERMEDIA2,  RES_SHARED_VGA },
    { PCI_VENDOR_3DLABS_CHIP_PERMEDIA,	 PCI_VENDOR_3DLABS_CHIP_PERMEDIA,   RES_NONE },
    { PCI_VENDOR_3DLABS_CHIP_500TX,	 PCI_VENDOR_3DLABS_CHIP_500TX,	    RES_NONE },
    { PCI_VENDOR_3DLABS_CHIP_MX,	 PCI_VENDOR_3DLABS_CHIP_MX,	    RES_NONE },
    { -1,				 -1,				    -1 }
};


typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_NOACCEL,
    OPTION_BLOCK_WRITE,
    OPTION_FIREGL3000,
    OPTION_MEM_CLK,
    OPTION_OVERLAY
} GLINTOpts;

static OptionInfoRec GLINTOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_BLOCK_WRITE,	"BlockWrite",	OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_FIREGL3000,	"FireGL3000",   OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MEM_CLK,		"MemClock",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_OVERLAY,		"Overlay",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

static RamDacSupportedInfoRec PermediaRamdacs[] = {
    { IBM526DB_RAMDAC },
    { IBM526_RAMDAC },
    { -1 }
};

static RamDacSupportedInfoRec TXMXRamdacs[] = {
    { IBM526DB_RAMDAC },
    { IBM526_RAMDAC },
    { IBM640_RAMDAC },
    { -1 }
};


static const char *vgahwSymbols[] = {
    "vgaHWGetIndex",
    "vgaHWSave",
    "vgaHWRestore",
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
    "XAAPolyLines",
    "XAAPolySegment",
    NULL
};

static const char *fbSymbols[] = {
    "xf1bppScreenInit",
    "xf4bppScreenInit",
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb24_32ScreenInit",
    "cfb32ScreenInit",
    "cfb8_32ScreenInit",
    "cfbGCPrivateIndex",
    "cfb16GCPrivateIndex",
    "cfb32GCPrivateIndex",
    "cfbBresS",
    "cfb16BresS",
    "cfb32BresS",
    NULL
};

static const char *racSymbols[] = {
    "xf86RACInit",
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
    "xf86DestroyI2CBusRec",
    "xf86I2CBusInit",
    "xf86I2CDevInit",
    "xf86I2CProbeAddress",
    "xf86I2CWriteByte",
    "xf86I2CWriteVec",    
    NULL
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(glintSetup);

static XF86ModuleVersionInfo glintVersRec =
{
	"glint",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	GLINT_MAJOR_VERSION, GLINT_MINOR_VERSION, GLINT_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

XF86ModuleData glintModuleData = { &glintVersRec, glintSetup, NULL };

pointer
glintSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&GLINT, module, 0);
	LoaderRefSymLists(vgahwSymbols, fbSymbols, racSymbols,
	    ddcSymbols, i2cSymbols, xaaSymbols, xf8_32bppSymbols, NULL);
	return (pointer)TRUE;
    }

    if (errmaj) *errmaj = LDR_ONCEONLY;
    return NULL;
}

#endif /* XFree86LOADER */

#define PARTPROD(a,b,c) (((a)<<6) | ((b)<<3) | (c))

static char bppand[4] = { 0x03, /* 8bpp */
			  0x01, /* 16bpp */
			  0x00, /* 24bpp */
			  0x00  /* 32bpp */};

static int partprod500TX[] = {
	-1,
	PARTPROD(0,0,1), PARTPROD(0,0,2), PARTPROD(0,1,2), PARTPROD(0,0,3),
	PARTPROD(0,1,3), PARTPROD(0,2,3), PARTPROD(1,2,3), PARTPROD(0,0,4),
	PARTPROD(0,1,4), PARTPROD(0,2,4), PARTPROD(1,2,4), PARTPROD(0,3,4),
	PARTPROD(1,3,4), PARTPROD(2,3,4),              -1, PARTPROD(0,0,5), 
	PARTPROD(0,1,5), PARTPROD(0,2,5), PARTPROD(1,2,5), PARTPROD(0,3,5), 
	PARTPROD(1,3,5), PARTPROD(2,3,5),              -1, PARTPROD(0,4,5), 
	PARTPROD(1,4,5), PARTPROD(2,4,5), PARTPROD(3,4,5),              -1,
	             -1,              -1,              -1, PARTPROD(0,0,6), 
	PARTPROD(0,1,6), PARTPROD(0,2,6), PARTPROD(1,2,6), PARTPROD(0,3,6), 
	PARTPROD(1,3,6), PARTPROD(2,3,6),              -1, PARTPROD(0,4,6), 
	PARTPROD(1,4,6), PARTPROD(2,4,6),              -1, PARTPROD(3,4,6),
	             -1,              -1,              -1, PARTPROD(0,5,6), 
	PARTPROD(1,5,6), PARTPROD(2,5,6),              -1, PARTPROD(3,5,6), 
	             -1,              -1,              -1, PARTPROD(4,5,6), 
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(0,0,7), 
	             -1, PARTPROD(0,2,7), PARTPROD(1,2,7), PARTPROD(0,3,7), 
	PARTPROD(1,3,7), PARTPROD(2,3,7),              -1, PARTPROD(0,4,7),
	PARTPROD(1,4,7), PARTPROD(2,4,7),              -1, PARTPROD(3,4,7), 
	             -1,              -1,              -1, PARTPROD(0,5,7),
	PARTPROD(1,5,7), PARTPROD(2,5,7),              -1, PARTPROD(3,5,7), 
	             -1,              -1,              -1, PARTPROD(4,5,7),
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(0,6,7), 
	PARTPROD(1,6,7), PARTPROD(2,6,7),              -1, PARTPROD(3,6,7),
	             -1,              -1,              -1, PARTPROD(4,6,7), 
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(5,6,7), 
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1,              -1,
		     -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(0,7,7),
		      0};

int partprodPermedia[] = {
	-1,
	PARTPROD(0,0,1), PARTPROD(0,1,1), PARTPROD(1,1,1), PARTPROD(1,1,2),
	PARTPROD(1,2,2), PARTPROD(1,2,2), PARTPROD(1,2,3), PARTPROD(2,2,3),
	PARTPROD(1,3,3), PARTPROD(2,3,3), PARTPROD(1,2,4), PARTPROD(3,3,3),
	PARTPROD(1,3,4), PARTPROD(2,3,4),              -1, PARTPROD(3,3,4), 
	PARTPROD(1,4,4), PARTPROD(2,4,4),              -1, PARTPROD(3,4,4), 
	             -1, PARTPROD(2,3,5),              -1, PARTPROD(4,4,4), 
	PARTPROD(1,4,5), PARTPROD(2,4,5), PARTPROD(3,4,5),              -1,
	             -1,              -1,              -1, PARTPROD(4,4,5), 
	PARTPROD(1,5,5), PARTPROD(2,5,5),              -1, PARTPROD(3,5,5), 
	             -1,              -1,              -1, PARTPROD(4,5,5), 
	             -1,              -1,              -1, PARTPROD(3,4,6),
	             -1,              -1,              -1, PARTPROD(5,5,5), 
	PARTPROD(1,5,6), PARTPROD(2,5,6),              -1, PARTPROD(3,5,6),
	             -1,              -1,              -1, PARTPROD(4,5,6),
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1, PARTPROD(5,5,6),
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
		     0};

#ifdef DPMSExtension
static void
GLINTDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
					int flags)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int videocontrol = 0, vtgpolarity = 0;
    
    if((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_500TX) ||
       (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_MX)) {
	vtgpolarity = GLINT_READ_REG(VTGPolarity) & 0xFFFFFFF0;
    } else {
        videocontrol = GLINT_READ_REG(PMVideoControl) & 0xFFFFFFD6;
    }

    switch (PowerManagementMode) {
	case DPMSModeOn:
	    /* Screen: On, HSync: On, VSync: On */
	    videocontrol |= 0x29;
	    vtgpolarity |= 0x05;
	    break;
	case DPMSModeStandby:
	    /* Screen: Off, HSync: Off, VSync: On */
	    videocontrol |= 0x20;
	    vtgpolarity |= 0x04;
	    break;
	case DPMSModeSuspend:
	    /* Screen: Off, HSync: On, VSync: Off */
	    videocontrol |= 0x08;
	    vtgpolarity |= 0x01;
	    break;
	case DPMSModeOff:
	    /* Screen: Off, HSync: Off, VSync: Off */
	    videocontrol |= 0x00;
	    vtgpolarity |= 0x00;
	    break;
	default:
	    return;
    }

    if((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_500TX) ||
       (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_MX))  {
    	GLINT_SLOW_WRITE_REG(vtgpolarity, VTGPolarity);
    } else {
    	GLINT_SLOW_WRITE_REG(videocontrol, PMVideoControl);
    }
}
#endif

static Bool
GLINTGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an GLINTRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(GLINTRec), 1);
    /* Initialise it */

    return TRUE;
}

static void
GLINTFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}


/* Mandatory */
static void
GLINTIdentify(int flags)
{
    xf86PrintChipsets(GLINT_NAME, "driver for 3Dlabs chipsets", GLINTChipsets);
}


/* Mandatory */
static Bool
GLINTProbe(DriverPtr drv, int flags)
{
    int i;
    pciVideoPtr pPci, *usedPci, *checkusedPci;
    PCITAG deltatag = 0, chiptag = 0;
    GDevPtr *devSections;
    GDevPtr *usedDevs;
    int numDevSections;
    int numUsed;
    int *usedChips;
    Bool foundScreen = FALSE;
    BusResource resource;
    unsigned long glintbase = 0, glintbase3 = 0, deltabase = 0;
    unsigned long *delta_pci_base = 0 ;

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

    if ((numDevSections = xf86MatchDevice(GLINT_DRIVER_NAME,
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
    checkusedPci = xf86GetPciVideoInfo();

    if (checkusedPci == NULL) {
	/*
	 * We won't let anything in the config file override finding no
	 * PCI video cards at all.  This seems reasonable now, but we'll see.
	 */
	return FALSE;
    }

    numUsed = xf86MatchPciInstances(GLINT_NAME, 0,
		   GLINTChipsets, GLINTPciChipsets, devSections,
		   numDevSections, &usedDevs, &usedPci, &usedChips);
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];
	resource = xf86FindPciResource(usedChips[i], GLINTPciChipsets);

	/*
	 * Check that nothing else has claimed the slots.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func, resource)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    glintbase = pPci->memBase[0];
	    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func, resource,
			     &GLINT, usedChips[i], pScrn->scrnIndex);

	    chiptag = pciTag(pPci->bus, pPci->device, pPci->func);

	    /* Need to claim Glint Delta for PERMEDIA & 500TX */
	    /* and for the moment we claim all other chips on the same */
	    /* bus/device number */
	    if ( (pPci->chipType == PCI_CHIP_500TX) ||
	         (pPci->chipType == PCI_CHIP_MX) ||
		 (pPci->chipType == PCI_CHIP_PERMEDIA) ) {

    		while(*checkusedPci != NULL) {

		    /* make sure we claim all but our source device */
		    if ((pPci->bus == (*checkusedPci)->bus && 
			pPci->device == (*checkusedPci)->device) &&
			pPci->func != (*checkusedPci)->func) {

		      /* Find that Delta chip, and give us the tag value */
		      if ( (((*checkusedPci)->vendor == PCI_VENDOR_TI) || 
		         ((*checkusedPci)->vendor == PCI_VENDOR_3DLABS)) &&
			 ((*checkusedPci)->chipType == PCI_CHIP_DELTA) ) {
			    deltabase = (*checkusedPci)->memBase[0];
			    delta_pci_base = &((*checkusedPci)->memBase[0]);
		    	    deltatag = pciTag((*checkusedPci)->bus, 
					      (*checkusedPci)->device, 
					      (*checkusedPci)->func);
		      }

		      /* Claim our devices */
	    	      if (!xf86ClaimPciSlot((*checkusedPci)->bus, 
					  (*checkusedPci)->device, 
					  (*checkusedPci)->func,
			  		  RES_NONE, &GLINT, usedChips[i],
					  pScrn->scrnIndex)) {
		          /* This can't happen */
			  FatalError("someone claimed the free slot!\n");
		      }

		    }
		    checkusedPci++;
		}
	    }

	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = GLINT_DRIVER_NAME;
	    pScrn->name		 = GLINT_NAME;
	    pScrn->Probe	 = GLINTProbe;
	    pScrn->PreInit	 = GLINTPreInit;
	    pScrn->ScreenInit	 = GLINTScreenInit;
	    pScrn->SwitchMode	 = GLINTSwitchMode;
	    pScrn->AdjustFrame	 = GLINTAdjustFrame;
	    pScrn->EnterVT	 = GLINTEnterVT;
	    pScrn->LeaveVT	 = GLINTLeaveVT;
	    pScrn->FreeScreen	 = GLINTFreeScreen;
	    pScrn->ValidMode	 = GLINTValidMode;
	    pScrn->device	 = usedDevs[i];
	    foundScreen = TRUE;

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* NEED TO MOVE THIS OUT OF THE PROBE CODE */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
	    {
		int temp;
		int bugbase = 0;
  		/*
   		 * due to a few bugs in the GLINT Delta we might have to
   		 * relocate the base address of config region of the Delta, if
   		 * bit 17 of the base addresses of config region of the Delta
   		 * and the 500TX or 300SX are different
   		 * We only handle config type 1 at this point
   		 */
  		if (deltatag && chiptag) {
    		    if ((deltabase & 0x20000) ^ (glintbase & 0x20000)) {
		/*
	 	 * if the base addresses are different at bit 17,
	 	 * we have to remap the base0 for the delta;
	 	 * as wrong as this looks, we can use the base3 of the
	 	 * 300SX/500TX for this. The delta is working as a bridge
	  	 * here and gives its own addresses preference. And we
	  	 * don't need to access base3, as this one is the bytw
	 	 * swapped local buffer which we don't need.
	  	 * Using base3 we know that the space is
	 	 * a) large enough
	 	 * b) free (well, almost)
	 	 *
	 	 * to be able to do that we need to enable IO
	 	 */
 		    if (pPci->chipType == PCI_CHIP_PERMEDIA) {
		    	glintbase3 = pciReadLong(chiptag, 0x20); /* base4 */
         	    } else {
		    	glintbase3 = pciReadLong(chiptag, 0x1c); /* base3 */
 		    }
		    if ((glintbase & 0x20000) ^ (glintbase3 & 0x20000)) {
	    	/*
	     	 * oops, still different; we know that base3 is at least
	     	 * 16 MB, so we just take 128k offset into it
	     	 */
	    	    	glintbase3 += 0x20000;
		    }
		/*
	 	 * and now for the magic.
	 	 * read old value
	 	 * write fffffffff
	 	 * read value
	 	 * write new value
	 	 */
		    bugbase = pciReadLong(deltatag, 0x10);
		    pciWriteLong(deltatag, 0x10, 0xffffffff);
		    temp = pciReadLong(deltatag, 0x10);
		    pciWriteLong(deltatag, 0x10, glintbase3);

		/* Update PCI tables */
		    *delta_pci_base = glintbase3;

		/*
	 	 * additionally, sometimes we see the baserom which might
	 	 * confuse the chip, so let's make sure that is disabled
	 	 */
		    temp = pciReadLong(chiptag, 0x30);
		    pciWriteLong(chiptag, 0x30, 0xffffffff);
		    temp = pciReadLong(chiptag, 0x30);
		    pciWriteLong(chiptag, 0x30, 0);
		    }
    	        }
  	        if (bugbase)
    	    	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
			"Glint Delta BUG, fixing.....old = 0x%x, new = 0x%x\n", 
				bugbase, glintbase3);
	    } 
	}
	/*
	 * ok, now let's forget about the Delta, in case we found one
	 */
	deltatag = deltabase = 0;
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
    GLINTPtr pGlint = GLINTPTR(pScrn);
	
    switch (pGlint->Chipset) {
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	linep = &partprodPermedia[0];
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	linep = &partprod500TX[0];
	break;
    }

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
GLINTPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    GLINTPtr pGlint;
    MessageType from;
    int i;
    Bool Overlay;
    int maxwidth = 0, maxheight = 0;
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

    xf86AddControlledResource(pScrn, MEM);
    xf86EnableAccess(&pScrn->Access);

    /* The ramdac module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "ramdac"))
	return FALSE;

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /* Allocate the GLINTRec driverPrivate */
    if (!GLINTGetRec(pScrn)) {
	return FALSE;
    }
    pGlint = GLINTPTR(pScrn);

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     * We support both 24bpp and 32bpp layouts, so indicate that.
     */
    if (!xf86SetDepthBpp(pScrn, 8, 0, 0, /*Support24bppFb |*/ Support32bppFb 
	 	/*| SupportConvert32to24 | PreferConvert32to24*/)) {
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
	case 30:
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

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, GLINTOptions);

    /* Default to 8bits per RGB */
    if (pScrn->depth == 30)  pScrn->rgbBits = 10;	
    			else pScrn->rgbBits = 8;
    if (xf86GetOptValInteger(GLINTOptions, OPTION_RGB_BITS, &pScrn->rgbBits)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
    }

    if (xf86IsOptionSet(GLINTOptions, OPTION_MEM_CLK)) {
	if (xf86GetOptValInteger(GLINTOptions, OPTION_MEM_CLK, 
				&pGlint->MemClock)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Memory Clock override enabled, set to %dMHz\n",
			pGlint->MemClock);
	}
    }
    from = X_DEFAULT;
    pGlint->HWCursor = TRUE; /* ON by default */
    if (xf86GetOptValBool(GLINTOptions, OPTION_HW_CURSOR, &pGlint->HWCursor))
	from = X_CONFIG;
    if (xf86ReturnOptValBool(GLINTOptions, OPTION_SW_CURSOR, FALSE)) {
	from = X_CONFIG;
	pGlint->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pGlint->HWCursor ? "HW" : "SW");
    if (xf86ReturnOptValBool(GLINTOptions, OPTION_NOACCEL, FALSE)) {
	pGlint->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    pGlint->UsePCIRetry = FALSE;
    from = X_DEFAULT;
    if (xf86GetOptValBool(GLINTOptions, OPTION_PCI_RETRY, &pGlint->UsePCIRetry))
	from = X_CONFIG;
    if (pGlint->UsePCIRetry)
	xf86DrvMsg(pScrn->scrnIndex, from, "PCI retry enabled\n");
    pScrn->overlayFlags = 0;
    from = X_DEFAULT;
    if (xf86GetOptValBool(GLINTOptions, OPTION_OVERLAY, &Overlay))
	from = X_CONFIG;
    if (Overlay) {
	if ((pScrn->depth == 24) && (pScrn->bitsPerPixel == 32)) {
	    pScrn->colorKey = 255; /* we should let the user change this */
	    pScrn->overlayFlags = OVERLAY_8_32_PLANAR;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "32/8bpp overlay enabled\n");
	}
    }

    i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL);

    pGlint->PciInfo = pciList[0];
    i--;
    while (i>0) {
	if ( ((pciList[i]->vendor == PCI_VENDOR_3DLABS) ||
	      (pciList[i]->vendor == PCI_VENDOR_TI)) ) {
	    if ( ((pciList[i]->chipType == PCI_CHIP_DELTA) ||
	      (pciList[i]->chipType == PCI_CHIP_GAMMA)) ) 
		pGlint->PciInfoGeometry = pciList[i];
	}
	i--;
    }

    pGlint->DoubleBuffer = FALSE;
    pGlint->RamDac = NULL;
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pGlint->Chipset = xf86StringToToken(GLINTChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pGlint->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(GLINTChipsets, pGlint->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pGlint->Chipset);
    } else {
	from = X_PROBED;
	pGlint->Chipset = pGlint->PciInfo->vendor << 16 | 
			  pGlint->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(GLINTChipsets, pGlint->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pGlint->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pGlint->ChipRev);
    } else {
	pGlint->ChipRev = pGlint->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * GLINTProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pGlint->Chipset);
	return FALSE;
    }
    if (pGlint->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    if ((pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA2) ||
	(pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V) ||
	(pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2)) {
    	if (xf86ReturnOptValBool(GLINTOptions, OPTION_BLOCK_WRITE, FALSE)) {
	    pGlint->UseBlockWrite = TRUE;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Block Writes enabled\n");
    	}
    }

    if (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_500TX) {
    	if (xf86ReturnOptValBool(GLINTOptions, OPTION_FIREGL3000, FALSE)) {
	    /* Can't we detect a Fire GL 3000 ????? and remove this ? */
	    pGlint->UseFireGL3000 = TRUE;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Diamond FireGL3000 mode enabled\n");
    	}
    }

    pGlint->PciTag = pciTag(pGlint->PciInfo->bus, pGlint->PciInfo->device,
			  pGlint->PciInfo->func);
    if ((pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) &&
	(pGlint->Chipset != PCI_VENDOR_TI_CHIP_PERMEDIA2) &&
	(pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V)) {
    	pGlint->PciTagGeometry = pciTag(pGlint->PciInfoGeometry->bus, 
					pGlint->PciInfoGeometry->device,
			  		pGlint->PciInfoGeometry->func);
    }
    
    if (pScrn->device->MemBase != 0) {
	pGlint->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	pGlint->FbAddress = pGlint->PciInfo->memBase[2] & 0xFF800000;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pGlint->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pGlint->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	if (pGlint->PciTagGeometry)
	    pGlint->IOAddress = pGlint->PciInfoGeometry->memBase[0] &0xFFFFC000;
	else
	    pGlint->IOAddress = pGlint->PciInfo->memBase[0] & 0xFFFFC000;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pGlint->IOAddress);

    /* HW bpp matches reported bpp */
    pGlint->HwBpp = pScrn->bitsPerPixel;

    pGlint->FbBase = NULL;
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	pGlint->FbMapSize = 0; /* Need to set FbMapSize for MMIO access */
	/* Need to access MMIO to determine videoRam */
        GLINTMapMem(pScrn);
	if( (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_500TX) ||
	   (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_MX) )
	    pScrn->videoRam = 1024 * (1 << ((GLINT_READ_REG(FBMemoryCtl) & 
							0xE0000000)>>29));
	else 
	    pScrn->videoRam = 2048 * (((GLINT_READ_REG(PMMemConfig) >> 29) &
							0x03) + 1);
        GLINTUnmapMem(pScrn);
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    pGlint->FbMapSize = pScrn->videoRam * 1024;

    if((pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_500TX) &&
       (pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_MX)) {
	pGlint->VGAcore = TRUE; /* chip has a vga core */
    	/* The vgahw module should be loaded here when needed */
    	if (!xf86LoadSubModule(pScrn, "vgahw"))
		return FALSE;

    	xf86LoaderReqSymLists(vgahwSymbols, NULL);
    	/*
         * Allocate a vgaHWRec
         */
        if (!vgaHWGetHWRec(pScrn))
	    return FALSE;
    } else {
	pGlint->VGAcore = FALSE;
    }

    if((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) ||
       (pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA2) ||
       (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V)) {
	GLINTMapMem(pScrn);
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Chip %s AGP Capable\n",
		((GLINT_READ_REG(ChipConfig) & 0x200) ? "is" : "is not"));
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "AGP Sidebanding is %sabled\n",
		((GLINT_READ_REG(ChipConfig) & 0x100) ? "en" : "dis"));
	GLINTUnmapMem(pScrn);
    }

    /* Let's check what type of DAC we have and reject if necessary */
    switch (pGlint->Chipset)
    {
	case PCI_VENDOR_TI_CHIP_PERMEDIA2:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	    maxheight = 2048;
	    maxwidth = 2048;
	    pGlint->RefClock = 14318;
	    pGlint->RamDacRec = RamDacCreateInfoRec();
	    pGlint->RamDacRec->ReadDAC = Permedia2InIndReg;
	    pGlint->RamDacRec->WriteDAC = Permedia2OutIndReg;
	    pGlint->RamDacRec->ReadAddress = Permedia2ReadAddress;
	    pGlint->RamDacRec->WriteAddress = Permedia2WriteAddress;
	    pGlint->RamDacRec->ReadData = Permedia2ReadData;
	    pGlint->RamDacRec->WriteData = Permedia2WriteData;
	    if(!RamDacInit(pScrn, pGlint->RamDacRec)) {
		RamDacDestroyInfoRec(pGlint->RamDacRec);
		return FALSE;
	    }
	    break;
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	    maxheight = 2048;
	    maxwidth = 2048;
	    pGlint->RefClock = 14318;
	    pGlint->RamDacRec = RamDacCreateInfoRec();
	    pGlint->RamDacRec->ReadDAC = Permedia2vInIndReg;
	    pGlint->RamDacRec->WriteDAC = Permedia2vOutIndReg;
	    pGlint->RamDacRec->ReadAddress = Permedia2ReadAddress;
	    pGlint->RamDacRec->WriteAddress = Permedia2WriteAddress;
	    pGlint->RamDacRec->ReadData = Permedia2ReadData;
	    pGlint->RamDacRec->WriteData = Permedia2WriteData;
	    if(!RamDacInit(pScrn, pGlint->RamDacRec)) {
		RamDacDestroyInfoRec(pGlint->RamDacRec);
		return FALSE;
	    }
	    break;
	case PCI_VENDOR_TI_CHIP_PERMEDIA:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	    maxheight = 1024;
	    maxwidth = 1536;
	    pGlint->RefClock = 14318;
	    pGlint->RamDacRec = RamDacCreateInfoRec();
	    pGlint->RamDacRec->ReadDAC = glintInIBMRGBIndReg;
	    pGlint->RamDacRec->WriteDAC = glintOutIBMRGBIndReg;
	    pGlint->RamDacRec->ReadAddress = glintIBMReadAddress;
	    pGlint->RamDacRec->WriteAddress = glintIBMWriteAddress;
	    pGlint->RamDacRec->ReadData = glintIBMReadData;
	    pGlint->RamDacRec->WriteData = glintIBMWriteData;
	    if(!RamDacInit(pScrn, pGlint->RamDacRec)) {
		RamDacDestroyInfoRec(pGlint->RamDacRec);
		return FALSE;
	    }
            GLINTMapMem(pScrn);
	    pGlint->RamDac = IBMramdacProbe(pScrn, PermediaRamdacs);
            GLINTUnmapMem(pScrn);
	    if (pGlint->RamDac == NULL)
		return FALSE;
	    break;
	case PCI_VENDOR_3DLABS_CHIP_500TX:
	case PCI_VENDOR_3DLABS_CHIP_MX:
	    maxheight = 4096;
	    maxwidth = 4096;
	    if (pGlint->PciInfoGeometry->chipType == PCI_CHIP_GAMMA)
		pGlint->RefClock = 28322;
	    else
	    if (pGlint->PciInfoGeometry->chipType == PCI_CHIP_DELTA)
		pGlint->RefClock = 40000;
	    else {
    		xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Undefined RefClock\n");
		return FALSE;
	    }
	    pGlint->RamDacRec = RamDacCreateInfoRec();
	    pGlint->RamDacRec->ReadDAC = glintInIBMRGBIndReg;
	    pGlint->RamDacRec->WriteDAC = glintOutIBMRGBIndReg;
	    pGlint->RamDacRec->ReadAddress = glintIBMReadAddress;
	    pGlint->RamDacRec->WriteAddress = glintIBMWriteAddress;
	    pGlint->RamDacRec->ReadData = glintIBMReadData;
	    pGlint->RamDacRec->WriteData = glintIBMWriteData;
	    if(!RamDacInit(pScrn, pGlint->RamDacRec)) {
		RamDacDestroyInfoRec(pGlint->RamDacRec);
		return FALSE;
	    }
            GLINTMapMem(pScrn);
	    pGlint->RamDac = IBMramdacProbe(pScrn, TXMXRamdacs);
            GLINTUnmapMem(pScrn);
	    if (pGlint->RamDac == NULL)
		return FALSE;
	    break;
    }

    /* Set the min pixel clock */
    pGlint->MinClock = 16250;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pGlint->MinClock / 1000);

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
	    pGlint->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pGlint->MaxClock = speed;
	from = X_CONFIG;
    } else {
	if((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_500TX)||
	   (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_MX))
		pGlint->MaxClock = 220000;
	if ( (pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA) ||
	     (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA) ) {
		switch (pScrn->bitsPerPixel) {
		    case 1:
		    case 4:
		    case 8:
			pGlint->MaxClock = 200000;
			break;
		    case 16:
			pGlint->MaxClock = 100000;
			break;
		    case 24:
			pGlint->MaxClock = 50000;
			break;
		    case 32:
			pGlint->MaxClock = 50000;
			break;
		}
	}
	if ( (pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA2) ||
	     (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) ||
	     (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V) ) {
		switch (pScrn->bitsPerPixel) {
		    case 1:
		    case 4:
		    case 8:
			pGlint->MaxClock = 230000;
			break;
		    case 16:
			pGlint->MaxClock = 230000;
			break;
		    case 24:
			pGlint->MaxClock = 150000;
			break;
		    case 32:
			pGlint->MaxClock = 110000;
			break;
		}
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pGlint->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pGlint->MinClock;
    clockRanges->maxClock = pGlint->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our GLINTValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
    if (pGlint->NoAccel) {
	/*
	 * XXX Assuming min pitch 256, max <maxwidth>
	 * XXX Assuming min height 128, max <maxheight>
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, maxwidth,
			      pScrn->bitsPerPixel, 128, maxheight,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pGlint->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    } else {
	/*
	 * Minimum width 32, Maximum width <maxwidth>
	 * Minimum height 128, Maximum height <maxheight>
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      GetAccelPitchValues(pScrn), 32, maxwidth,
			      pScrn->bitsPerPixel, 128, maxheight,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pGlint->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }

    if (i == -1) {
	GLINTFreeRec(pScrn);
	return FALSE;
    }

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	GLINTFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    /* Only allow a single mode for MX and TX chipsets */
    if ((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_500TX) ||
        (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_MX)) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
	    "GLINT TX/MX chipsets only support one modeline, using first.\n");
	pScrn->modes->next = NULL;
	pScrn->virtualX = pScrn->modes->HDisplay;
	pScrn->virtualY = pScrn->modes->VDisplay;
	pScrn->displayWidth = pScrn->virtualX;
	if (partprod500TX[pScrn->displayWidth >> 5] == -1) {
	    i = -1;
	    do {
	        pScrn->displayWidth += 32;
	        i = partprod500TX[pScrn->displayWidth >> 5];
	    } while (i == -1);
	}
    }

    switch (pGlint->Chipset)
    { /* Now we know displaywidth, so set linepitch data */
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	pGlint->pprod = partprodPermedia[pScrn->displayWidth >> 5];
	pGlint->bppalign = bppand[(pScrn->bitsPerPixel>>3)-1];
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	pGlint->pprod = partprod500TX[pScrn->displayWidth >> 5];
	pGlint->bppalign = 0;
	break;
    }

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 1:
	mod = "xf1bpp";
	break;
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
	if (pix24bpp == 24)
	    mod = "cfb24";
	else
	    mod = "xf24_32bpp";
	break;
    case 32:
	if (pScrn->overlayFlags & OVERLAY_8_32_PLANAR) {
	    mod = "xf8_32bpp";
	} else {
	    mod = "cfb32";
	}
	break;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	GLINTFreeRec(pScrn);
	return FALSE;
    }

    /* Load XAA if needed */
    if (!pGlint->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    GLINTFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    /* Load DDC */
    if (!xf86LoadSubModule(pScrn, "ddc")) {
	GLINTFreeRec(pScrn);
	return FALSE;
    }

    /* Load I2C if needed */
    if ((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) ||
	(pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V) ||
	(pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA2)) {
	if (xf86LoadSubModule(pScrn, "i2c")) {
	    I2CBusPtr pBus;

	    if ((pBus = xf86CreateI2CBusRec())) {
		pBus->BusName = "DDC";
		pBus->scrnIndex = pScrn->scrnIndex;
		pBus->I2CUDelay = Permedia2I2CUDelay;
		pBus->I2CPutBits = Permedia2I2CPutBits;
		pBus->I2CGetBits = Permedia2I2CGetBits;
		pBus->DriverPrivate.ptr = pGlint;
		if (!xf86I2CBusInit(pBus)) {
		    xf86DestroyI2CBusRec(pBus, TRUE, TRUE);
		} else
		    pGlint->DDCBus = pBus; 
	    }

	    if ((pBus = xf86CreateI2CBusRec())) {
	        pBus->BusName = "Video";
	        pBus->scrnIndex = pScrn->scrnIndex;
		pBus->I2CUDelay = Permedia2I2CUDelay;
		pBus->I2CPutBits = Permedia2I2CPutBits;
		pBus->I2CGetBits = Permedia2I2CGetBits;
		pBus->DriverPrivate.ptr = pGlint;
		if (!xf86I2CBusInit(pBus)) {
		    xf86DestroyI2CBusRec(pBus, TRUE, TRUE);
		} else
		    pGlint->VSBus = pBus;
	    }
	}
    }

    xf86DelControlledResource(&pScrn->Access, FALSE);
    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
GLINTMapMem(ScrnInfoPtr pScrn)
{
    CARD32 save = 0, saved = 0;
    GLINTPtr pGlint;

    pGlint = GLINTPTR(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

#if 0 /* 500TX doesn't like this - FIX ME */
    save = pciReadLong(pGlint->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pGlint->PciTag, PCI_CMD_STAT_REG,
		 save & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));
    if (pGlint->PciTagGeometry) {
	saved = pciReadLong(pGlint->PciTagGeometry, PCI_CMD_STAT_REG);
	pciWriteLong(pGlint->PciTagGeometry, PCI_CMD_STAT_REG,
		 saved & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));
    }
#endif

    /*
     * Map IO registers to virtual address space
     */ 
    if (pGlint->PciTagGeometry)
	pGlint->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
		pGlint->PciTagGeometry, pGlint->IOAddress, 0x20000);
    else
	pGlint->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
		pGlint->PciTag, pGlint->IOAddress, 0x20000);

    if (pGlint->IOBase == NULL)
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

	val = *(volatile CARD32 *)(pGlint->IOBase+0);
	val = *(volatile CARD32 *)(pGlint->IOBase+0x1000);
	val = *(volatile CARD32 *)(pGlint->IOBase+0x2000);
	val = *(volatile CARD32 *)(pGlint->IOBase+0x3000);
    }
#endif

    if (pGlint->FbMapSize != 0) {
    	pGlint->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pGlint->PciTag,
				 pGlint->FbAddress,
				 pGlint->FbMapSize);
        if (pGlint->FbBase == NULL)
	    return FALSE;
    }

#if 0 /* 500TX doesn't like this - FIX ME */
    /* Re-enable I/O and memory */
    pciWriteLong(pGlint->PciTag, PCI_CMD_STAT_REG,
		 save | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));
    if (pGlint->PciTagGeometry)
	pciWriteLong(pGlint->PciTag, PCI_CMD_STAT_REG,
		 saved | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));
#endif

  /* Due to bugs in the Glint Delta/Permedia/500TX chips we need to do this */
  /* Bizarre, but it works. */
  if ((pGlint->Chipset != PCI_VENDOR_TI_CHIP_PERMEDIA2) &&
      (pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) &&
      (pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V)) {
   if (pGlint->PciTag) {
    unsigned long temp, temp2;
    
    /*
     * and now for the magic.
     * read old value
     * write fffffffff
     * read value
     * write old value
     */
    if (pGlint->PciTagGeometry) {
	temp = pciReadLong(pGlint->PciTagGeometry, 0x10);
	pciWriteLong(pGlint->PciTagGeometry, 0x10, 0xffffffff);
	temp2 = pciReadLong(pGlint->PciTagGeometry, 0x10);
	pciWriteLong(pGlint->PciTagGeometry, 0x10, temp & 0xfffffff0);
    }
    
    temp = pciReadLong(pGlint->PciTag, 0x10);
    pciWriteLong(pGlint->PciTag, 0x10, 0xffffffff);
    temp2 = pciReadLong(pGlint->PciTag, 0x10);
    pciWriteLong(pGlint->PciTag, 0x10, temp);
    
    temp = pciReadLong(pGlint->PciTag, 0x14);
    pciWriteLong(pGlint->PciTag, 0x14, 0xffffffff);
    temp2 = pciReadLong(pGlint->PciTag, 0x14);
    pciWriteLong(pGlint->PciTag, 0x14, temp);
    
    temp = pciReadLong(pGlint->PciTag, 0x18);
    pciWriteLong(pGlint->PciTag, 0x18, 0xffffffff);
    temp2 = pciReadLong(pGlint->PciTag, 0x18);
    pciWriteLong(pGlint->PciTag, 0x18, temp);
    
    temp = pciReadLong(pGlint->PciTag, 0x1c);
    pciWriteLong(pGlint->PciTag, 0x1c, 0xffffffff);
    temp2 = pciReadLong(pGlint->PciTag, 0x1c);
    pciWriteLong(pGlint->PciTag, 0x1c, temp);
   
    temp = pciReadLong(pGlint->PciTag, 0x20);
    pciWriteLong(pGlint->PciTag, 0x20, 0xffffffff);
    temp2 = pciReadLong(pGlint->PciTag, 0x20);
    pciWriteLong(pGlint->PciTag, 0x20, temp);
   }
  }
  return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
GLINTUnmapMem(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint;

    pGlint = GLINTPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pGlint->IOBase, 0x20000);
    pGlint->IOBase = NULL;

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pGlint->FbBase, pScrn->videoRam);
    pGlint->FbBase = NULL;
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
GLINTSave(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint;
    GLINTRegPtr glintReg;
    RamDacHWRecPtr pRAMDAC;
    RamDacRegRecPtr RAMDACreg;

    pGlint = GLINTPTR(pScrn);
    pRAMDAC = RAMDACHWPTR(pScrn);
    glintReg = &pGlint->SavedReg;
    RAMDACreg = &pRAMDAC->SavedReg;

    xf86AddControlledResource(pScrn, MEM);
    xf86EnableAccess(&pScrn->Access);

    if (pGlint->VGAcore) {
    	vgaRegPtr vgaReg;
    	vgaReg = &VGAHWPTR(pScrn)->SavedReg;
	vgaHWSave(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_FONTS);
    }

    switch (pGlint->Chipset)
    {
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	Permedia2Save(pScrn, glintReg);
	break;
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	Permedia2VSave(pScrn, glintReg);
	break;
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	PermediaSave(pScrn, glintReg);
	(*pGlint->RamDac->Save)(pScrn, pGlint->RamDacRec, RAMDACreg);
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	TXSave(pScrn, glintReg);
	(*pGlint->RamDac->Save)(pScrn, pGlint->RamDacRec, RAMDACreg);
	break;
    }

    xf86DelControlledResource(&pScrn->Access, TRUE);
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
GLINTModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int ret = -1;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINTRegPtr glintReg;
    RamDacHWRecPtr pRAMDAC = RAMDACHWPTR(pScrn);
    RamDacRegRecPtr RAMDACreg;

    if (pGlint->VGAcore) {
    	vgaHWPtr hwp = VGAHWPTR(pScrn);
    	vgaHWUnlock(hwp);

    	/* Initialise the ModeReg values */
    	if (!vgaHWInit(pScrn, mode))
	    return FALSE;
    }


    pScrn->vtSema = TRUE;

    switch (pGlint->Chipset) {
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	ret = Permedia2Init(pScrn, mode);
	break;
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	ret = Permedia2VInit(pScrn, mode);
	break;
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	ret = PermediaInit(pScrn, mode);
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	ret = TXInit(pScrn, mode);
	break;
    }

    if (!ret)
	return FALSE;

    /* Program the registers */
    if (pGlint->VGAcore) {
    	vgaHWPtr hwp = VGAHWPTR(pScrn);
    	vgaRegPtr vgaReg = &hwp->ModeReg;
    	vgaHWProtect(pScrn, TRUE);
	vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
    }

    glintReg = &pGlint->ModeReg;
    RAMDACreg = &pRAMDAC->ModeReg;

    switch (pGlint->Chipset) {
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	Permedia2Restore(pScrn, glintReg);
	break;
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	Permedia2VRestore(pScrn, glintReg);
	break;
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	PermediaRestore(pScrn, glintReg);
	(*pGlint->RamDac->Restore)(pScrn, pGlint->RamDacRec, RAMDACreg);
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	TXRestore(pScrn, glintReg);
	(*pGlint->RamDac->Restore)(pScrn, pGlint->RamDacRec, RAMDACreg);
	break;
    }

    if (pGlint->VGAcore) {
    	vgaHWProtect(pScrn, FALSE);
    }

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
GLINTRestore(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint;
    GLINTRegPtr glintReg;
    RamDacHWRecPtr pRAMDAC;
    RamDacRegRecPtr RAMDACreg;

    pGlint = GLINTPTR(pScrn);
    pRAMDAC = RAMDACHWPTR(pScrn);
    glintReg = &pGlint->SavedReg;
    RAMDACreg = &pRAMDAC->SavedReg;

    xf86AddControlledResource(pScrn, MEM);
    xf86EnableAccess(&pScrn->Access);

    if (pGlint->VGAcore) {
    	vgaHWProtect(pScrn, TRUE);
    }

    switch (pGlint->Chipset) {
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	Permedia2VideoReset(pScrn);
	Permedia2Restore(pScrn, glintReg);
	break;
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	Permedia2VideoReset(pScrn);
	Permedia2VRestore(pScrn, glintReg);
	break;
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	PermediaRestore(pScrn, glintReg);
	(*pGlint->RamDac->Restore)(pScrn, pGlint->RamDacRec, RAMDACreg);
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	TXRestore(pScrn, glintReg);
	(*pGlint->RamDac->Restore)(pScrn, pGlint->RamDacRec, RAMDACreg);
	break;
    }

    if (pGlint->VGAcore) {
    	vgaHWPtr hwp = VGAHWPTR(pScrn);
    	vgaRegPtr vgaReg = &hwp->SavedReg;
	vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_FONTS);
    	vgaHWProtect(pScrn, FALSE);
    }

    xf86DelControlledResource(&pScrn->Access, FALSE);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
GLINTScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int ret;
    VisualPtr visual;

    /* Map the GLINT memory and MMIO areas */
    if (!GLINTMapMem(pScrn))
	return FALSE;

    /* Initialize the MMIO vgahw functions */
    if (pGlint->VGAcore) {
    	vgaHWPtr hwp;
    	hwp = VGAHWPTR(pScrn);
	if (xf86IsPrimaryPci(pGlint->PciInfo)) {
    	    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */
    	    if (!vgaHWMapMem(pScrn))
	   	 return FALSE;
	}
    	vgaHWSetMmioFuncs(hwp, pGlint->IOBase, 0x6000);
    	vgaHWGetIOBase(hwp);
    }

    /* Save the current state */
    GLINTSave(pScrn);

    /* DDC */
    {
	xf86MonPtr pMon = NULL;
	
	if (pGlint->DDCBus)
	    pMon = xf86DoEDID_DDC2(pScrn->scrnIndex, pGlint->DDCBus);
	    
	if (!pMon)
	    /* Try DDC1 */;
	    
	xf86PrintEDID(pMon);
    }
    
    /* Initialise the first mode */
    GLINTModeInit(pScrn, pScrn->currentMode);

    /* Darken the screen for aesthetic reasons and set the viewport */
    GLINTSaveScreen(pScreen, FALSE);
    GLINTAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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

    if((pScrn->overlayFlags & OVERLAY_8_32_PLANAR) && 
						(pScrn->bitsPerPixel == 32)) {
	if (!miSetVisualTypes(8, PseudoColorMask | GrayScaleMask,
			      pScrn->rgbBits, PseudoColor))
		return FALSE;
	if (!miSetVisualTypes(24, TrueColorMask, pScrn->rgbBits, TrueColor))
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
	ret = xf1bppScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 8:
	ret = cfbScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	if (pix24bpp == 24)
	    ret = cfb24ScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	else
	    ret = cfb24_32ScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	if(pScrn->overlayFlags & OVERLAY_8_32_PLANAR)
	    ret = cfb8_32ScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	else 
	    ret = cfb32ScreenInit(pScreen, pGlint->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in GLINTScrnInit\n",
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
    }

    if (!pGlint->NoAccel) {
        switch (pGlint->Chipset)
        {
        case PCI_VENDOR_TI_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	    Permedia2AccelInit(pScreen);
	    break;
	case PCI_VENDOR_TI_CHIP_PERMEDIA:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	    PermediaAccelInit(pScreen);
	    break;
	case PCI_VENDOR_3DLABS_CHIP_500TX:
	case PCI_VENDOR_3DLABS_CHIP_MX:
	    TXAccelInit(pScreen);
	    break;
        }
    }

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Initialise cursor functions */
    if (pGlint->HWCursor) {
	if ((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) || 
	    (pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA2))
	    Permedia2HWCursorInit(pScreen);
	else
	if (pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V)
	    Permedia2vHWCursorInit(pScreen);
	else
	if ( ((pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) &&
	      (pGlint->Chipset != PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V) &&
	      (pGlint->Chipset != PCI_VENDOR_TI_CHIP_PERMEDIA2)) &&
	     ((pGlint->RamDac->RamDacType == (IBM526DB_RAMDAC)) ||
	      (pGlint->RamDac->RamDacType == (IBM526_RAMDAC)) ||
	      (pGlint->RamDac->RamDacType == (IBM640_RAMDAC))) )
	    		glintIBMHWCursorInit(pScreen);
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    if ((pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V) ||
	(pGlint->Chipset == PCI_VENDOR_3DLABS_CHIP_PERMEDIA2) || 
	(pGlint->Chipset == PCI_VENDOR_TI_CHIP_PERMEDIA2)) {
    	if (!xf86HandleColormaps(pScreen, 256, pScrn->rgbBits, 
	    (pScrn->depth == 16) ? Permedia2LoadPalette16:Permedia2LoadPalette,
	    NULL,
	    CMAP_RELOAD_ON_MODE_SWITCH |
	    ((pScrn->overlayFlags & OVERLAY_8_32_PLANAR) 
					? 0 : CMAP_PALETTED_TRUECOLOR)))
	return FALSE;
    } else {
	if (pScrn->rgbBits == 10) {
    	if (!RamDacHandleColormaps(pScreen, 1024, pScrn->rgbBits, 
	    CMAP_RELOAD_ON_MODE_SWITCH | CMAP_PALETTED_TRUECOLOR)) 
	return FALSE;
	} else {
    	if (!RamDacHandleColormaps(pScreen, 256, pScrn->rgbBits, 
	    CMAP_RELOAD_ON_MODE_SWITCH | 
	    ((pScrn->overlayFlags & OVERLAY_8_32_PLANAR) 
					? 0 : CMAP_PALETTED_TRUECOLOR)))
	return FALSE;
	}
    }

    if((pScrn->overlayFlags & OVERLAY_8_32_PLANAR) && 
						(pScrn->bitsPerPixel == 32)) {
	if(!xf86Overlay8Plus32Init(pScreen))
	    return FALSE;
    }

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, (DPMSSetProcPtr)GLINTDisplayPowerManagementSet, 0);
#endif

    pGlint->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = GLINTCloseScreen;
    pScreen->SaveScreen = GLINTSaveScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    switch (pGlint->Chipset) {
        case PCI_VENDOR_TI_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	    Permedia2VideoInit(pScreen);
    }

#if 0
    /* Enable the screen */
    GLINTSaveScreen(pScreen, TRUE);
#endif

    /* Done */
    return TRUE;
}


/* Usually mandatory */
static Bool
GLINTSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return GLINTModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
GLINTAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    CARD32 base;
    GLINTPtr pGlint;

    pScrn = xf86Screens[scrnIndex];
    pGlint = GLINTPTR(pScrn);
    if (pGlint->VGAcore) {
    	vgaHWPtr hwp;
    	hwp = VGAHWPTR(pScrn);
    }

    base = ((y * pScrn->displayWidth + x) >> 1) >> pGlint->BppShift;
    if (pScrn->bitsPerPixel == 24) base *= 3;
 
    switch (pGlint->Chipset)
    {
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	GLINT_SLOW_WRITE_REG(base, PMScreenBase);
	break;
    }
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
GLINTEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    /* Should we re-save the text mode on each VT enter? */
    if (!GLINTModeInit(pScrn, pScrn->currentMode))
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
GLINTLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    GLINTPtr pGlint = GLINTPTR(pScrn);

    GLINTRestore(pScrn);
    if (pGlint->VGAcore)
    	vgaHWLock(VGAHWPTR(pScrn));
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
GLINTCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    GLINTPtr pGlint = GLINTPTR(pScrn);

    switch (pGlint->Chipset) {
        case PCI_VENDOR_TI_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	    Permedia2VideoUninit(xf86Screens[scrnIndex]);
    }

    if (pScrn->vtSema) {
        GLINTRestore(pScrn);
	if (pGlint->VGAcore)
       	    vgaHWLock(VGAHWPTR(pScrn));
        GLINTUnmapMem(pScrn);
    }
    if(pGlint->AccelInfoRec)
	XAADestroyInfoRec(pGlint->AccelInfoRec);
    if(pGlint->CursorInfoRec)
	xf86DestroyCursorInfoRec(pGlint->CursorInfoRec);
    pScrn->vtSema = FALSE;
    
    pScreen->CloseScreen = pGlint->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any per-generation data structures */

/* Optional */
static void
GLINTFreeScreen(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    GLINTPtr pGlint = GLINTPTR(pScrn);

    if (pGlint->VGAcore)
    	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    if (pGlint->RamDacRec)
    	RamDacFreeRec(xf86Screens[scrnIndex]);
    GLINTFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
GLINTValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    GLINTPtr pGlint = GLINTPTR(pScrn);

    if (mode->Flags & V_INTERLACE)
	return(MODE_NO_INTERLACE);
    
    if (pScrn->bitsPerPixel == 24) {
	/* A restriction on the PM2 where a black strip on the left hand
	 * side appears if not aligned properly */
        switch (pGlint->Chipset) {
        case PCI_VENDOR_TI_CHIP_PERMEDIA2:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
        case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
          if (mode->HDisplay % 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
	      "HDisplay %d not divisible by 8, fixing...\n", mode->HDisplay);
	    mode->HDisplay -= (mode->HDisplay % 8);
          }
	
          if (mode->HSyncStart % 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
	     "HSyncStart %d not divisible by 8, fixing...\n", mode->HSyncStart);
	    mode->HSyncStart -= (mode->HSyncStart % 8);
          }

          if (mode->HSyncEnd % 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
	      "HSyncEnd %d not divisible by 8, fixing...\n", mode->HSyncEnd);
	    mode->HSyncEnd -= (mode->HSyncEnd % 8);
          }

          if (mode->HTotal % 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
	      "HTotal %d not divisible by 8, fixing...\n", mode->HTotal);
	    mode->HTotal -= (mode->HTotal % 8);
          }
          break;
	}
    }

    return(MODE_OK);
}

/* Do screen blanking */

/* Mandatory */
static Bool
GLINTSaveScreen(ScreenPtr pScreen, Bool unblank)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CARD32 temp;

    switch (pGlint->Chipset) {
    case PCI_VENDOR_TI_CHIP_PERMEDIA2:
    case PCI_VENDOR_TI_CHIP_PERMEDIA:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
    case PCI_VENDOR_3DLABS_CHIP_PERMEDIA:
	temp = GLINT_READ_REG(PMVideoControl);
	if (unblank) temp |= 1;
	else	     temp &= 0xFFFFFFFE;
	GLINT_WRITE_REG(temp, PMVideoControl);
	break;
    case PCI_VENDOR_3DLABS_CHIP_500TX:
    case PCI_VENDOR_3DLABS_CHIP_MX:
	break;
    }

    return TRUE;
}

#ifdef DEBUG
void
GLINT_VERB_WRITE_REG(GLINTPtr pGlint, CARD32 v, int r, char *file, int line)
{
    if (xf86GetVerbosity() > 2)
	ErrorF("[0x%04x] <- 0x%08x (%s, %d)\n",	r, v, file, line);
    *(volatile CARD32 *)((char *) pGlint->IOBase + r) = v;
}

CARD32
GLINT_VERB_READ_REG(GLINTPtr pGlint, CARD32 r, char *file, int line)
{
    CARD32 v = *(volatile CARD32 *)((char *) pGlint->IOBase + r);

    if (xf86GetVerbosity() > 2)
	ErrorF("[0x%04x] -> 0x%08x (%s, %d)\n", r, v, file, line);
    return v;
}
#endif
