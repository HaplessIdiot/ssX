/* $XConsortium: mga_driver.c /main/12 1996/10/28 05:13:26 kaleb $ */
/*
 * MGA Millennium (MGA2064W) with Ti3026 RAMDAC driver v.1.1
 *
 * The driver is written without any chip documentation. All extended ports
 * and registers come from tracing the VESA-ROM functions.
 * The BitBlt Engine comes from tracing the windows BitBlt function.
 *
 * Author:	Radoslaw Kapitan, Tarnow, Poland
 *			kapitan@student.uci.agh.edu.pl
 *		original source
 *
 * Now that MATROX has released documentation to the public, enhancing
 * this driver has become much easier. Nevertheless, this work continues
 * to be based on Radoslaw's original source
 *
 * Contributors:
 *		Andrew Vanderstock, Melbourne, Australia
 *			vanderaj@mail2.svhm.org.au
 *		additions, corrections, cleanups
 *
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		integrated into XFree86-3.1.2Gg
 *		fixed some problems with PCI probing and mapping
 *
 *		David Dawes
 *			dawes@XFree86.Org
 *		some cleanups, and fixed some problems
 *
 *		Andrew E. Mileski
 *			aem@ott.hookup.net
 *		RAMDAC timing, and BIOS stuff
 *
 *		Leonard N. Zubkoff
 *			lnz@dandelion.com
 *		Support for 8MB boards, RGB Sync-on-Green, and DPMS.
 *		Guy DESBIEF
 *			g.desbief@aix.pacwan.net
 *		RAMDAC MGA1064 timing,
 *		Doug Merritt
 *			doug@netcom.com
 *		Fixed 32bpp hires 8MB horizontal line glitch at middle right
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_driver.c,v 1.64 1999/01/03 08:06:36 dawes Exp $ */

/*
 * This is a first cut at a non-accelerated version to work with the
 * new server design (DHD).
 */


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers need this */
#include "xf86_ansic.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

#include "micmap.h"

#include "xf86DDC.h"

/*
 * If using cfb, cfb.h is required.  Select the others for the bpp values
 * the driver supports.
 */
#define PSZ 8	/* needed for cfb.h */
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#include "cfb8_32.h"

/* These need to be checked */
#if 0
#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif
#endif

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"
#include "mga_macros.h"

#include "xaa.h"
#include "xf86cmap.h"


/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */
static void	MGAIdentify(int flags);
static Bool	MGAProbe(DriverPtr drv, int flags);
static Bool	MGAPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	MGAScreenInit(int Index, ScreenPtr pScreen, int argc,
			      char **argv);
static Bool	MGAEnterVT(int scrnIndex, int flags);
static void	MGALeaveVT(int scrnIndex, int flags);
static Bool	MGACloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	MGASaveScreen(ScreenPtr pScreen, Bool unblank);

/* Required if the driver supports mode switching */
static Bool	MGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
/* Required if the driver supports moving the viewport */
static void	MGAAdjustFrame(int scrnIndex, int x, int y, int flags);

/* Optional functions */
static void	MGAFreeScreen(int scrnIndex, int flags);
static int	MGAValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
			     int flags);
#ifdef DPMSExtension
static void	MGADisplayPowerManagementSet(ScrnInfoPtr pScrn,
					     int PowerManagementMode,
					     int flags);
#endif

/* Internally used functions */
static Bool	MGAMapMem(ScrnInfoPtr pScrn);
static Bool	MGAUnmapMem(ScrnInfoPtr pScrn);
static void	MGASave(ScrnInfoPtr pScrn);
static void	MGARestore(ScrnInfoPtr pScrn);
static Bool	MGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

#define VERSION 4000
#define MGA_NAME "MGA"
#define MGA_DRIVER_NAME "mga"
#define MGA_MAJOR_VERSION 1
#define MGA_MINOR_VERSION 0
#define MGA_PATCHLEVEL 0

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec MGA = {
    VERSION,
    "accelerated driver for Matrox Millennium and Mystique cards",
    MGAIdentify,
    MGAProbe,
    NULL,
    0
};

/* Supported chipsets */
static SymTabRec MGAChipsets[] = {
    { PCI_CHIP_MGA2064,		"mga2064w" },
    { PCI_CHIP_MGA1064,		"mga1064sg" },
    { PCI_CHIP_MGA2164,		"mga2164w" },
    { PCI_CHIP_MGA2164_AGP,	"mga2164w AGP" },
    { PCI_CHIP_MGAG100,		"mgag100" },
    { PCI_CHIP_MGAG200,		"mgag200" },
    { PCI_CHIP_MGAG200_PCI,	"mgag200 PCI" },
    {-1,			NULL }
};

static PciChipsets MGAPciChipsets[] = {
    { PCI_CHIP_MGA2064,		PCI_CHIP_MGA2064,	RES_SHARED_VGA },
    { PCI_CHIP_MGA1064,		PCI_CHIP_MGA1064,	RES_SHARED_VGA },
    { PCI_CHIP_MGA2164,		PCI_CHIP_MGA2164,	RES_SHARED_VGA },
    { PCI_CHIP_MGA2164_AGP,	PCI_CHIP_MGA2164_AGP,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG100,		PCI_CHIP_MGAG100,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG200,		PCI_CHIP_MGAG200,	RES_SHARED_VGA },
    { PCI_CHIP_MGAG200_PCI,	PCI_CHIP_MGAG200_PCI,	RES_SHARED_VGA },
    { -1,			-1,			RES_UNDEFINED }
};

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_SYNC_ON_GREEN,
    OPTION_NOACCEL,
    OPTION_SHOWCACHE,
    OPTION_8_PLUS_24,
    OPTION_MGA_SDRAM
} MGAOpts;

static OptionInfoRec MGAOptions[] = {
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_TRI,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_SYNC_ON_GREEN,	"SyncOnGreen",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_8_PLUS_24,		"8Plus24",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MGA_SDRAM,		"MGASDRAM",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};


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
    NULL
};

/* Not used yet
static const char *i2cSymbols[] = {
    "xf86CreateI2CBusRec",
    "xf86I2CBusInit",
    NULL
};
*/

#ifdef XFree86LOADER

MODULEINITPROTO(mgaModuleInit);
static MODULESETUPPROTO(mgaSetup);

static XF86ModuleVersionInfo mgaVersRec =
{
	"mga",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	MGA_MAJOR_VERSION, MGA_MINOR_VERSION, MGA_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	{0,0,0,0}
};

/*
 * This function is the module init function.
 * Its name has to be the driver name followed by ModuleInit()
 */
void
mgaModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &mgaVersRec;
    *setup = mgaSetup;
    *teardown = NULL;
}

static pointer
mgaSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&MGA, module, 0);

	/*
	 * Modules that this driver always requires may be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
	LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols, 
			  xf8_32bppSymbols, ramdacSymbols, ddcSymbols, NULL);

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

/* 
 * ramdac info structure initialization
 */
static MGARamdacRec DacInit = {
	FALSE, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL,
	90000, /* maxPixelClock */
	0, X_DEFAULT, X_DEFAULT, FALSE
}; 

static Bool
MGAGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate an MGARec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(MGARec), 1);
    /* Initialise it */

    MGAPTR(pScrn)->Dac = DacInit;
    return TRUE;
}

static void
MGAFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}


/* Mandatory */
static void
MGAIdentify(int flags)
{
    xf86PrintChipsets(MGA_NAME, "driver for Matrox chipsets", MGAChipsets);
}


/* Mandatory */
static Bool
MGAProbe(DriverPtr drv, int flags)
{
    int i;
    pciVideoPtr pPci, *usedPci;
    GDevPtr *devSections;
    GDevPtr *usedDevs;
    int *usedChips;
    int numDevSections;
    int numUsed;
    Bool foundScreen = FALSE;
    BusResource resource;

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
     * Check if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(MGA_DRIVER_NAME,
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

    numUsed = xf86MatchPciInstances(MGA_NAME, PCI_VENDOR_MATROX,
			MGAChipsets, MGAPciChipsets, devSections,
			numDevSections, &usedDevs, &usedPci, &usedChips);
    /* Free it since we don't need that list after this */
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];
	/* XXX Is this really needed since we already know what it is? */
	resource = xf86FindPciResource(usedChips[i], MGAPciChipsets);

	/*
	 * Check that nothing else has claimed the slots.
	 */
	
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func, resource)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func, resource,
			     &MGA, usedChips[i], pScrn->scrnIndex);

	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = MGA_DRIVER_NAME;
	    pScrn->name		 = MGA_NAME;
	    pScrn->Probe	 = MGAProbe;
	    pScrn->PreInit	 = MGAPreInit;
	    pScrn->ScreenInit	 = MGAScreenInit;
	    pScrn->SwitchMode	 = MGASwitchMode;
	    pScrn->AdjustFrame	 = MGAAdjustFrame;
	    pScrn->EnterVT	 = MGAEnterVT;
	    pScrn->LeaveVT	 = MGALeaveVT;
	    pScrn->FreeScreen	 = MGAFreeScreen;
	    pScrn->ValidMode	 = MGAValidMode;
	    pScrn->device	 = usedDevs[i];
	    foundScreen = TRUE;
	}
    }
    xfree(usedDevs);
    xfree(usedPci);
    return foundScreen;
}
	

/*
 * Should aim towards not relying on this.
 */

/*
 * MGAReadBios - Read the video BIOS info block.
 *
 * DESCRIPTION
 *   Warning! This code currently does not detect a video BIOS.
 *   In the future, support for motherboards with the mga2064w
 *   will be added (no video BIOS) - this is not a huge concern
 *   for me today though.  (XXX)
 *
 * EXTERNAL REFERENCES
 *   vga256InfoRec.BIOSbase	IN	Physical address of video BIOS.
 *   MGABios			OUT	The video BIOS info block.
 *
 * HISTORY
 *   August  31, 1997 - [ajv] Andrew van der Stock
 *   Fixed to understand Mystique and Millennium II
 * 
 *   January 11, 1997 - [aem] Andrew E. Mileski
 *   Set default values for GCLK (= MCLK / pre-scale ).
 *
 *   October 7, 1996 - [aem] Andrew E. Mileski
 *   Written and tested.
 */ 

static void
MGAReadBios(ScrnInfoPtr pScrn)
{
	CARD8 tmp[ 64 ];
	CARD16 offset;
	CARD8	chksum;
	CARD8	*pPINSInfo; 
	MGAPtr pMga;
	MGABiosInfo *pBios;
	MGABios2Info *pBios2;
	Bool pciBIOS = FALSE;
	
	pMga = MGAPTR(pScrn);
	pBios = &pMga->Bios;
	pBios2 = &pMga->Bios2;

	/*
	 * If the BIOS address was probed, it was found from the PCI config
	 * space.  If it was given in the config file, try to guess when it
	 * looks like it might be controlled by the PCI config space.
	 */
	if (pMga->BiosFrom == X_PROBED)
	    pciBIOS = TRUE;
	else if (pMga->BiosFrom == X_CONFIG && pMga->BiosAddress > 0x100000)
	    pciBIOS = TRUE;

#define MGADoBIOSRead(offset, buf, len) \
    (pciBIOS ? \
      xf86ReadPciBIOS(pMga->BiosAddress, offset, pMga->PciTag, buf, len) : \
      xf86ReadBIOS(pMga->BiosAddress, offset, buf, len))
	
	MGADoBIOSRead(0, tmp, sizeof( tmp ));
	if (
		tmp[ 0 ] != 0x55
		|| tmp[ 1 ] != 0xaa
		|| strncmp(( char * )( tmp + 45 ), "MATROX", 6 )
	) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "Video BIOS info block not detected!\n");
		return;
	}

	/* Get the info block offset */
	MGADoBIOSRead(0x7ffc, ( CARD8 * ) & offset, sizeof( offset ));


	/* Let the world know what we are up to */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Video BIOS info block at 0x%08lX\n",
		   (long)(pMga->BiosAddress + offset));

	/* Copy the info block */
	switch (pMga->Chipset){
	   case PCI_CHIP_MGA2064:
		MGADoBIOSRead(offset,
			( CARD8 * ) & pBios->StructLen, sizeof( MGABiosInfo ));
		break;
	   default:
		MGADoBIOSRead(offset,
			( CARD8 * ) & pBios2->PinID, sizeof( MGABios2Info ));
	}

	
	/* matrox millennium-2 and mystique pins info */
	if ( pBios2->PinID == 0x412e ) {	
	    int i;
	    /* check that the pins info is correct */
	    if ( pBios2->StructLen != 0x40 ) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			"Video BIOS info block not detected!\n");
		return;
	    }
	    /* check that the chksum is correct */
	    chksum = 0;
	    pPINSInfo = (CARD8 *) &pBios2->PinID;

	    for (i=0; i < pBios2->StructLen; i++) {
		chksum += *pPINSInfo;
		pPINSInfo++;
	    }

	    if ( chksum ) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			"Video BIOS info block did not checksum!\n");
		pBios2->PinID = 0;
		return;
	    }

	    /* last check */
	    if ( pBios2->StructRev == 0 ) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		  "Video BIOS info block does not have a valid revision!\n");
		pBios2->PinID = 0;
		return;
	    }

	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		"Found and verified enhanced Video BIOS info block\n");

	   /* Set default MCLK values (scaled by 100 kHz) */
	   if ( pBios2->ClkMem == 0 )
		pBios2->ClkMem = 50;
	   if ( pBios2->Clk4MB == 0 )
		pBios2->Clk4MB = pBios->ClkBase;
	   if ( pBios2->Clk8MB == 0 )
		pBios2->Clk8MB = pBios->Clk4MB;
	   pBios->StructLen = 0; /* not in use */
	   return;
	} else {
	  /* Set default MCLK values (scaled by 10 kHz) */
	  if ( pBios->ClkBase == 0 )
		pBios->ClkBase = 4500;
  	  if ( pBios->Clk4MB == 0 )
		pBios->Clk4MB = pBios->ClkBase;
	  if ( pBios->Clk8MB == 0 )
		pBios->Clk8MB = pBios->Clk4MB;
	  pBios2->PinID = 0; /* not in use */
	  return;
	}
}

/*
 * MGASoftReset --
 *
 * Resets drawing engine
 */
static void
MGASoftReset(ScrnInfoPtr pScrn)
{
	MGAPtr pMga = MGAPTR(pScrn);
	int i;

	pMga->FbMapSize = 8192 * 1024;
	MGAMapMem(pScrn);

	/* set soft reset bit */
	OUTREG(MGAREG_Reset, 1);
	usleep(200);
	OUTREG(MGAREG_Reset, 0);

	/* reset memory */
	OUTREG(MGAREG_MACCESS, 1<<15);
	usleep(10);

	/* wait until drawing engine is ready */
	while ( MGAISBUSY() )
	    usleep(1000);
		
	/* flush FIFO */	
	i = 32;
	WAITFIFO(i);
	while ( i-- )
	    OUTREG(MGAREG_SHIFT, 0);

	MGAUnmapMem(pScrn);
}

/*
 * MGACountRAM --
 *
 * Counts amount of installed RAM 
 */
static int
MGACountRam(ScrnInfoPtr pScrn)
{
	MGAPtr pMga = MGAPTR(pScrn);

	if (pMga->FbAddress)
	{
		volatile unsigned char* base;
		unsigned char tmp, tmp3, tmp5;
	
		pMga->FbMapSize = 8192 * 1024;
		MGAMapMem(pScrn);
		base = pMga->FbBase;
	
		/* turn MGA mode on - enable linear frame buffer (CRTCEXT3) */
		OUTREG8(0x1FDE, 3);
		tmp = INREG8(0x1FDF);
		OUTREG8(0x1FDF, tmp | 0x80);
	
		/* write, read and compare method */
		base[0x500000] = 0x55;
		base[0x300000] = 0x33;
		base[0x100000] = 0x11;
		tmp5 = base[0x500000];
		tmp3 = base[0x300000];

		/* restore CRTCEXT3 state */
		OUTREG8(0x1FDE, 3);
		OUTREG8(0x1FDF, tmp);
	
		MGAUnmapMem(pScrn);
	
		if(tmp5 == 0x55)
			return 8192;
		if(tmp3 == 0x33)
			return 4096;
	}
	return 2048;
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
    MGAPtr pMga = MGAPTR(pScrn);
	
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
	if (accelWidths[i] % pMga->Rounding == 0) {
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
MGAPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    MGAPtr pMga;
    MessageType from;
    int i;
    int bytesPerPixel;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *reqSym = NULL;

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

    bytesPerPixel = pScrn->bitsPerPixel / 8;

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the MGARec driverPrivate */
    if (!MGAGetRec(pScrn)) {
	return FALSE;
    }
    pMga = MGAPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, MGAOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* XXX This is here just to test options. */
	/* Default to 8 */
	pScrn->rgbBits = 8;
	if (xf86GetOptValInteger(MGAOptions, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
    }
    from = X_DEFAULT;
    pMga->HWCursor = TRUE;
    /*
     * The preferred method is to use the "hw cursor" option as a tri-state
     * option, with the default set above.
     */
    if (xf86GetOptValBool(MGAOptions, OPTION_HW_CURSOR, &pMga->HWCursor)) {
	from = X_CONFIG;
    }
    /* For compatibility, accept this too (as an override) */
    if (xf86IsOptionSet(MGAOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pMga->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pMga->HWCursor ? "HW" : "SW");
    if (xf86IsOptionSet(MGAOptions, OPTION_NOACCEL)) {
	pMga->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86IsOptionSet(MGAOptions, OPTION_PCI_RETRY)) {
	pMga->UsePCIRetry = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
    }
    if (xf86IsOptionSet(MGAOptions, OPTION_SYNC_ON_GREEN)) {
	pMga->SyncOnGreen = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Sync-on-Green enabled\n");
    }
    if (xf86IsOptionSet(MGAOptions, OPTION_SHOWCACHE)) {
	pMga->ShowCache = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ShowCache enabled\n");
    }
    if (xf86IsOptionSet(MGAOptions, OPTION_MGA_SDRAM)) {
	pMga->HasSDRAM = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Has SDRAM\n");
    }
    if (xf86IsOptionSet(MGAOptions, OPTION_8_PLUS_24)) {
	if(pScrn->bitsPerPixel == 32) {
#if 1
            int num = pScrn->numFormats;
            pScrn->formats[num].depth = pScrn->formats[num - 1].depth;
            pScrn->formats[num].bitsPerPixel =
                                 pScrn->formats[num - 1].bitsPerPixel;
            pScrn->formats[num].scanlinePad = 
                                pScrn->formats[num - 1].scanlinePad;
            pScrn->formats[num - 1].depth = 8;
            pScrn->formats[num - 1].bitsPerPixel = 8;
            pScrn->formats[num - 1].scanlinePad = BITMAP_SCANLINE_PAD;
            pScrn->numFormats++;
#endif
	    pMga->Overlay8Plus24 = TRUE;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
				"PseudoColor overlay enabled\n");
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Option \"8Plus24\" is not supported in this configuration\n");
	}
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
	MGAFreeRec(pScrn);
	if (i > 0)
	    xfree(pciList);
	return FALSE;
    }

    pMga->PciInfo = *pciList;
    xfree(pciList);
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        pMga->Chipset = xf86StringToToken(MGAChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	pMga->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(MGAChipsets, pMga->Chipset);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pMga->Chipset);
    } else {
	from = X_PROBED;
	pMga->Chipset = pMga->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(MGAChipsets, pMga->Chipset);
    }
    if (pScrn->device->chipRev >= 0) {
	pMga->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pMga->ChipRev);
    } else {
	pMga->ChipRev = pMga->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * MGAProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pMga->Chipset);
	return FALSE;
    }
    if (pMga->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);
	
    pMga->PciTag = pciTag(pMga->PciInfo->bus, pMga->PciInfo->device,
			  pMga->PciInfo->func);
    

    switch (pMga->Chipset) {
    case PCI_CHIP_MGA2064:
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
	MGA2064SetupFuncs(pScrn);
	break;
    case PCI_CHIP_MGA1064:
    case PCI_CHIP_MGAG100:
    case PCI_CHIP_MGAG200:
    case PCI_CHIP_MGAG200_PCI:
	MGAGSetupFuncs(pScrn);
	break;
    }

    /* ajv changes to reflect actual values. see sdk pp 3-2. */
    /* these masks just get rid of the crap in the lower bits */

    /*
     * For the 2064 and older rev 1064, base0 is the MMIO and base0 is
     * the framebuffer is base1.  Let the config file override these.
     */
    if (pScrn->device->MemBase != 0) {
	pMga->FbAddress = pScrn->device->MemBase;
	from = X_CONFIG;
    } else {
	/* details: mgabase2 sdk pp 4-12 */
	int i = ((pMga->Chipset == PCI_CHIP_MGA1064 && pMga->ChipRev < 3) ||
		    pMga->Chipset == PCI_CHIP_MGA2064) ? 1 : 0;
	if (pMga->PciInfo->memBase[i] != 0) {
	    pMga->FbAddress = pMga->PciInfo->memBase[i] & 0xff800000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "No valid FB address in PCI config space\n");
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pMga->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pMga->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	/* details: mgabase1 sdk pp 4-11 */
	int i = ((pMga->Chipset == PCI_CHIP_MGA1064 && pMga->ChipRev < 3) ||
		    pMga->Chipset == PCI_CHIP_MGA2064) ? 0 : 1;
	if (pMga->PciInfo->memBase[i] != 0) {
	    pMga->IOAddress = pMga->PciInfo->memBase[i] & 0xffffc000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"No valid MMIO address in PCI config space\n");
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pMga->IOAddress);

     
    pMga->ILOADAddress = 0;
    if ( pMga->Chipset != PCI_CHIP_MGA2064 ) {
	    if (pMga->PciInfo->memBase[2] != 0) {
	    	pMga->ILOADAddress = pMga->PciInfo->memBase[2] & 0xffffc000;
	        xf86DrvMsg(pScrn->scrnIndex, X_PROBED, 
			"Pseudo-DMA transfer window at 0x%lX\n",
	       		(unsigned long)pMga->ILOADAddress);
	    } 
    }


    /*
     * Find the BIOS base.  Get it from the PCI config if possible.  Otherwise
     * use the VGA default.  Allow the config file to override this.
     */

    if (pScrn->device->BiosBase != 0) {
	pMga->BiosAddress = pScrn->device->BiosBase;
	pMga->BiosFrom = X_CONFIG;
    } else {
	/* details: rombase sdk pp 4-15 */
	if (pMga->PciInfo->biosBase != 0) {
	    pMga->BiosAddress = pMga->PciInfo->biosBase & 0xffff0000;
	    pMga->BiosFrom = X_PROBED;
	} else {
	    /* Need to watch this when removing VGA depencencies */
	    pMga->BiosAddress = 0xc0000;
	    pMga->BiosFrom = X_DEFAULT;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, pMga->BiosFrom, "BIOS at 0x%lX\n",
	       (unsigned long)pMga->BiosAddress);

    /* Memory must be enabled to read the BIOS and probe the memory amount */
    xf86AddControlledResource(pScrn, MEM);
    xf86EnableAccess(&pScrn->Access);

    /*
     * Read the BIOS data struct
     */

    MGAReadBios(pScrn);

#ifdef DEBUG
    ErrorF("MGABios.RamdacType = 0x%x\n", pMga->Bios.RamdacType);
#endif
	
    /* HW bpp matches reported bpp */
    pMga->HwBpp = pScrn->bitsPerPixel;

    /*
     * Reset card if it isn't primary one
     */
    if (!xf86IsPrimaryPci(pMga->PciInfo))
        MGASoftReset(pScrn);
         
    /*
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else if((pMga->Chipset == PCI_CHIP_MGA2164) ||
	(pMga->Chipset == PCI_CHIP_MGA2164_AGP)){
	pScrn->videoRam = 4096;
	from = X_DEFAULT;
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
	 "Skipping memory probe due to hardware bug.  Assuming 4096 kBytes\n");
    } else {
	pScrn->videoRam = MGACountRam(pScrn);
	from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
	
    pMga->FbMapSize = pScrn->videoRam * 1024;

    /*
     * Access control can be relinquished now.  Leave memory disabled
     * when doing so.  It will get enabled again in ScreenInit.
     */
    xf86DelControlledResource(&pScrn->Access, FALSE);

    /* Set the bpp shift value */
    switch (pScrn->bitsPerPixel) {
    case 8:
	pMga->BppShift = 0;
	break;
    case 16:
	pMga->BppShift = 1;
	break;
    case 24:
	pMga->BppShift = 0;
	break;
    case 32:
	pMga->BppShift = 2;
	break;
    }

    /*
     * fill MGAdac struct
     * Warning: currently, it should be after RAM counting
     */
    (*pMga->PreInit)(pScrn);


    /* XXX Set HW cursor use */

    /* Set the min pixel clock */
    pMga->MinClock = 12000;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
	       pMga->MinClock / 1000);
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
	    pMga->MaxClock = pScrn->device->dacSpeeds[0];
	else
	    pMga->MaxClock = speed;
	from = X_CONFIG;
    } else {
	pMga->MaxClock = pMga->Dac.maxPixelClock;
	from = pMga->Dac.ClockFrom;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pMga->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = pMga->MinClock;
    clockRanges->maxClock = pMga->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = TRUE;

    /* Only set MemClk if appropriate for the ramdac */
    if (pMga->Dac.SetMemClk) {
	if (pScrn->device->MemClk != 0) {
	    pMga->MemClk = pScrn->device->MemClk;
	    from = X_CONFIG;
	} else {
	    pMga->MemClk = pMga->Dac.MemoryClock;
	    from = pMga->Dac.MemClkFrom;
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "MCLK used is %.1f MHz\n",
		   pMga->MemClk / 1000.0);
    }

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our MGAValidMode() already takes
     * care of this, we don't worry about setting them here.
     */

    /* Select valid modes from those available */
    if (pMga->NoAccel) {
	/*
	 * XXX Assuming min pitch 256, max 2048
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 2048,
			      pMga->Rounding * pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pMga->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    
    } else {
	/*
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      GetAccelPitchValues(pScrn), 0, 0,
			      pMga->Rounding * pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pMga->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }
    if (i == -1) {
	MGAFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	MGAFreeRec(pScrn);
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

    /*
     * Compute the byte offset into the linear frame buffer where the
     * frame buffer data should actually begin.  According to DDK misc.c
     * line 1023, if more than 4MB is to be displayed, YDSTORG must be set
     * appropriately to align memory bank switching, and this requires a
     * corresponding offset on linear frame buffer access.
     */

    pMga->YDstOrg = 0;
    if (pScrn->virtualX * pScrn->virtualY * bytesPerPixel > 4*1024*1024) {
	int offset, offset_modulo, ydstorg_modulo;

	offset = (4*1024*1024) % (pScrn->displayWidth * bytesPerPixel);
	offset_modulo = 4;
	ydstorg_modulo = 64;
	if (pScrn->bitsPerPixel == 24)
	    offset_modulo *= 3;
	if (pMga->Interleave)
	{
	    offset_modulo <<= 1;
	    ydstorg_modulo <<= 1;
	}
	pMga->YDstOrg = offset / bytesPerPixel;

	/*
	 * When this was unconditional, it caused a line of horizontal garbage
	 * at the middle right of the screen at the 4Meg boundary in 32bpp
	 * (and presumably any other modes that use more than 4M). But it's
	 * essential for 24bpp (it may not matter either way for 8bpp & 16bpp,
	 * I'm not sure; I didn't notice problems when I checked with and
	 * without.)
	 * DRM Doug Merritt 12/97, submitted to XFree86 6/98 (oops)
	 */
	if (bytesPerPixel < 4) {
	    while ((offset % offset_modulo) != 0 ||
		   (pMga->YDstOrg % ydstorg_modulo) != 0) {
		offset++;
		pMga->YDstOrg = offset / bytesPerPixel;
	    }
	}
    }
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "YDstOrg is set to %d\n",
		   pMga->YDstOrg);
    pMga->FbUsableSize = pMga->FbMapSize - pMga->YDstOrg * bytesPerPixel;
    /*
     * XXX This should be taken into account in some way in the mode valdation
     * section.
     */

    /* Allocate HW cursor buffer at the end of video ram */
    if( pMga->HWCursor && pMga->Dac.CursorOffscreenMemSize )
        if( pScrn->virtualY * pScrn->displayWidth * pScrn->bitsPerPixel / 8 <=
        	pMga->FbUsableSize - pMga->Dac.CursorOffscreenMemSize ) {
            pMga->FbUsableSize -= pMga->Dac.CursorOffscreenMemSize;
            pMga->FbCursorOffset =
                pMga->FbMapSize - pMga->Dac.CursorOffscreenMemSize;
        } else {
            pMga->HWCursor = FALSE;
            xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                "Too less offscreen memory for HW cursor; using SW cursor\n");
        }

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 8:
	mod = "cfb";
	reqSym = "cfbScreenInit";
	break;
    case 16:
	mod = "cfb16";
	reqSym = "cfb16ScreenInit";
	break;
    case 24:
	mod = "cfb24";
	reqSym = "cfb24ScreenInit";
	break;
    case 32:
	if (pMga->Overlay8Plus24) {
	    mod = "xf8_32bpp";
	    reqSym = "cfb8_32ScreenInit";
	    xf86LoaderReqSymLists(xf8_32bppSymbols, NULL);
	} else {
	    mod = "cfb32";
	    reqSym = "cfb32ScreenInit";
	    
	}
	break;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	MGAFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(reqSym, NULL);

    /* Load XAA if needed */
    if (!pMga->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    /* Load ramdac if needed */
    if (pMga->HWCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }
    /* Load DDC if needed */
    /* This gives us DDC1 - we should be able to get DDC2B using i2c */
    if (pMga->ddc1Read) {
	if (!xf86LoadSubModule(pScrn, "ddc")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(ddcSymbols, NULL);
    }

    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
MGAMapMem(ScrnInfoPtr pScrn)
{
    CARD32 save;
    MGAPtr pMga;

    pMga = MGAPTR(pScrn);

    /*
     * Disable memory and I/O before mapping the MMIO area.  This avoids
     * the MMIO area being read during the mapping (which happens on
     * some SVR4 versions), which will cause a lockup.
     */

    save = pciReadLong(pMga->PciTag, PCI_CMD_STAT_REG);
    pciWriteLong(pMga->PciTag, PCI_CMD_STAT_REG,
		 save & ~(PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));

    /*
     * Map IO registers to virtual address space
     */ 
#if !defined(__alpha__)
    pMga->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, pMga->PciTag,
				 (pointer)pMga->IOAddress, 0x4000);
#else
    /*
     * For Alpha, we need to map SPARSE memory, since we need
     * byte/short access.
     */
    pMga->IOBase = xf86MapPciMemSparse(pScrn->scrnIndex, VIDMEM_MMIO,
				       pMga->PciTag,
				       (pointer)pMga->IOAddress, 0x4000);
#endif
    if (pMga->IOBase == NULL)
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

	val = *(volatile CARD32 *)(pMga->IOBase+0);
	val = *(volatile CARD32 *)(pMga->IOBase+0x1000);
	val = *(volatile CARD32 *)(pMga->IOBase+0x2000);
	val = *(volatile CARD32 *)(pMga->IOBase+0x3000);
    }
#endif

#ifdef __alpha__
    /*
     * for Alpha, we need to map DENSE memory as well, for
     * setting CPUToScreenColorExpandBase.
     */
    pMga->IOBaseDense = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
					pMga->PciTag, (pointer)pMga->IOAddress,
					0x4000);
    if (pMga->IOBaseDense == NULL)
	return FALSE;
#endif /* __alpha__ */

    pMga->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pMga->PciTag,
				 (pointer)((unsigned long)pMga->FbAddress),
				 pMga->FbMapSize);
    if (pMga->FbBase == NULL)
	return FALSE;

    pMga->FbStart = pMga->FbBase + pMga->YDstOrg * (pScrn->bitsPerPixel / 8);


    /* Map the ILOAD transfer window if there is one.  We only make
	DWORD access on DWORD boundaries to this window */
    if(pMga->ILOADAddress)
	pMga->ILOADBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, 
			pMga->PciTag, (pointer)pMga->ILOADAddress, 0x800000);
    else  pMga->ILOADBase = NULL;


    /* Re-enable I/O and memory */
    pciWriteLong(pMga->PciTag, PCI_CMD_STAT_REG,
		 save | (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE));
    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
MGAUnmapMem(ScrnInfoPtr pScrn)
{
    MGAPtr pMga;

    pMga = MGAPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
#ifndef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->IOBase, 0x4000);
#else
    xf86UnMapVidMemSparse(pScrn->scrnIndex, (pointer)pMga->IOBase, 0x4000);
#endif
    pMga->IOBase = NULL;

#ifdef __alpha__
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->IOBaseDense, 0x4000);
    pMga->IOBaseDense = NULL;
#endif /* __alpha__ */

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->FbBase, pScrn->videoRam);
    pMga->FbBase = NULL;
    pMga->FbStart = NULL;

    if(pMga->ILOADBase)
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pMga->ILOADBase, 0x800000);
    pMga->ILOADBase = NULL;
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
MGASave(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->SavedReg;
    MGAPtr pMga = MGAPTR(pScrn);
    MGARegPtr mgaReg = &pMga->SavedReg;

    mgaReg->VgaEnable =
		(pciReadLong(pMga->PciTag, PCI_OPTION_REG) & 0x100) != 0;

    /* Enable VGA core for primary card */
    if (xf86IsPrimaryPci(pMga->PciInfo))
	pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x100);
    else
	pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x000);

    xf86AddControlledResource(pScrn, MEM);
    xf86EnableAccess(&pScrn->Access);

    /* Only save text mode fonts/text for the primary card */
    (*pMga->Save)(pScrn, vgaReg, mgaReg, xf86IsPrimaryPci(pMga->PciInfo));

    /* Disable VGA core, and leave memory access on */
    pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x000);
    xf86DelControlledResource(&pScrn->Access, TRUE);
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
MGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    MGAPtr pMga = MGAPTR(pScrn);
    MGARegPtr mgaReg;

    vgaHWUnlock(hwp);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    if (!(*pMga->ModeInit)(pScrn, mode))
	return FALSE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    mgaReg = &pMga->ModeReg;

    (*pMga->Restore)(pScrn, vgaReg, mgaReg, FALSE);

    MGAStormSync(pScrn);
    MGAStormEngineInit(pScrn);
    vgaHWProtect(pScrn, FALSE);

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
MGARestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->SavedReg;
    MGAPtr pMga = MGAPTR(pScrn);
    MGARegPtr mgaReg = &pMga->SavedReg;

    MGAStormSync(pScrn);

    /* Enable VGA core for primary card */
    if (xf86IsPrimaryPci(pMga->PciInfo))
	pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100, 0x100);

    xf86AddControlledResource(pScrn, MEM);
    xf86EnableAccess(&pScrn->Access);

    /* Only restore text mode fonts/text for the primary card */
    vgaHWProtect(pScrn, TRUE);
    if (xf86IsPrimaryPci(pMga->PciInfo))
        (*pMga->Restore)(pScrn, vgaReg, mgaReg, TRUE);
    else
        vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
    vgaHWProtect(pScrn, FALSE);

    xf86DelControlledResource(&pScrn->Access, FALSE);

    /* Restore VgaEnable setting */
    pciSetBitsLong(pMga->PciTag, PCI_OPTION_REG, 0x100,
		    mgaReg->VgaEnable ? 0x100 : 0x000);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
MGAScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    MGAPtr pMga;
    MGARamdacPtr MGAdac;
    int ret;
    VisualPtr visual;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);
    pMga = MGAPTR(pScrn);
    MGAdac = &pMga->Dac;

    /* Map the MGA memory and MMIO areas */
    if (!MGAMapMem(pScrn))
	return FALSE;

    /* Initialise the MMIO vgahw functions */
    vgaHWSetMmioFuncs(hwp, pMga->IOBase, PORT_OFFSET);
    vgaHWGetIOBase(hwp);

    /* Map the VGA memory when the primary video */
    if (xf86IsPrimaryPci(pMga->PciInfo)) {
	hwp->MapSize = 0x10000;
	if (!vgaHWMapMem(pScrn))
	    return FALSE;
    }

    /* Save the current state */
    MGASave(pScrn);

    /* Initialize DDC and output Monitor info */
    /* This gives us DDC1 - we should be able to get DDC2B using i2c */
    /* Needs to be done before ModeInit as it changes some registers */
    if (pMga->ddc1Read) {
	xf86PrintEDID( xf86DoEDID_DDC1(pScrn->scrnIndex, vgaHWddc1SetSpeed, pMga->ddc1Read ) );
    }

    /* Initialise the first mode */
    if (!MGAModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    /* Darken the screen for aesthetic reasons and set the viewport */
    MGASaveScreen(pScreen, FALSE);
    MGAAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

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

    /* All MGA support DirectColor and can do overlays in 32bpp */
    if(pMga->Overlay8Plus24 && (pScrn->bitsPerPixel == 32)) {
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
    case 8:
	ret = cfbScreenInit(pScreen, pMga->FbStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pMga->FbStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pMga->FbStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	if(pMga->Overlay8Plus24)
	    ret = cfb8_32ScreenInit(pScreen, pMga->FbStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth, TRANSPARENCY_KEY);
	else 
	    ret = cfb32ScreenInit(pScreen, pMga->FbStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in MGAScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;


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

    xf86SetBlackWhitePixels(pScreen);

    if (!pMga->NoAccel)
	MGAStormAccelInit(pScreen);

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    if(pMga->Overlay8Plus24) {
	if(!xf86Overlay8Plus32Init(pScreen))
	    return FALSE;
    }

    /* Initialize software cursor.  
	Must precede creation of the default colormap */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* Initialize HW cursor layer. 
	Must follow software cursor initialization*/
    if (pMga->HWCursor) { 
	if(!MGAHWCursorInit(pScreen))
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
		"Hardware cursor initialization failed\n");
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    /* Initialize colormap layer.  
	Must follow initialization of the default colormap */
    if(!xf86HandleColormaps(pScreen, 256, 8, MGAdac->LoadPalette, NULL,
	(pMga->Overlay8Plus24 ? 0 : CMAP_PALETTED_TRUECOLOR) |
			CMAP_RELOAD_ON_MODE_SWITCH))	
	return FALSE;

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, MGADisplayPowerManagementSet, 0);
#endif

    pScreen->SaveScreen = MGASaveScreen;

    /* Wrap the current CloseScreen function */
    pMga->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = MGACloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    /* Done */
    return TRUE;
}


/* Usually mandatory */
static Bool
MGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return MGAModeInit(xf86Screens[scrnIndex], mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
static void 
MGAAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    int Base, tmp, count;
    MGAPtr pMga;
    vgaHWPtr hwp;

    pScrn = xf86Screens[scrnIndex];
    hwp = VGAHWPTR(pScrn);
    pMga = MGAPTR(pScrn);

    if(pMga->ShowCache && y) {
	int lastline = pMga->FbUsableSize / 
		(pScrn->displayWidth * pScrn->bitsPerPixel/8);

	if(lastline > 4095) lastline = 4095; /* is that right ? */
	lastline -= pScrn->currentMode->VDisplay;
	y += pScrn->virtualY - 1;
        if(y > lastline) y = lastline;
    }

    Base = (y * pScrn->displayWidth + x + pMga->YDstOrg) >>
			(3 - pMga->BppShift);

    if (pScrn->bitsPerPixel == 24)
	Base *= 3;

    /* find start of retrace */
    while (INREG8(0x1FDA) & 0x08);
    while (!(INREG8(0x1FDA) & 0x08)); 
    /* wait until we're past the start (fixseg.c in the DDK) */
    count = INREG(MGAREG_VCOUNT) + 2;
    while(INREG(MGAREG_VCOUNT) < count);
    
    OUTREG16(0x1FD4, (Base & 0x00FF00) | 0x0C);
    OUTREG16(0x1FD4, ((Base & 0x0000FF) << 8) | 0x0D);
    OUTREG8(0x1FDE, 0x00);
    tmp = INREG8(0x1FDF);
    OUTREG8(0x1FDF, (tmp & 0xF0) | ((Base & 0x0F0000) >> 16));

}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
MGAEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    if (!MGAModeInit(pScrn, pScrn->currentMode))
	return FALSE;
    MGAAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
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
MGALeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    MGARestore(pScrn);
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
MGACloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    MGAPtr pMga = MGAPTR(pScrn);

    MGARestore(pScrn);
    vgaHWLock(hwp);
    MGAUnmapMem(pScrn);
    if (pMga->AccelInfoRec)
	XAADestroyInfoRec(pMga->AccelInfoRec);
    if (pMga->CursorInfoRec)
    	xf86DestroyCursorInfoRec(pMga->CursorInfoRec);
    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = pMga->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}


/* Free up any persistent data structures */

/* Optional */
static void
MGAFreeScreen(int scrnIndex, int flags)
{
    /*
     * This only gets called when a screen is being deleted.  It does not
     * get called routinely at the end of a server generation.
     */
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    MGAFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
MGAValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
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
MGASaveScreen(ScreenPtr pScreen, Bool unblank)
{
    return vgaHWSaveScreen(pScreen, unblank);
}


/*
 * MGADisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
MGADisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
	MGAPtr pMga = MGAPTR(pScrn);
	unsigned char seq1 = 0, crtcext1 = 0;

ErrorF("MGADisplayPowerManagementSet: %d\n", PowerManagementMode);

	switch (PowerManagementMode)
	{
	case DPMSModeOn:
	    /* Screen: On; HSync: On, VSync: On */
	    seq1 = 0x00;
	    crtcext1 = 0x00;
	    break;
	case DPMSModeStandby:
	    /* Screen: Off; HSync: Off, VSync: On */
	    seq1 = 0x20;
	    crtcext1 = 0x10;
	    break;
	case DPMSModeSuspend:
	    /* Screen: Off; HSync: On, VSync: Off */
	    seq1 = 0x20;
	    crtcext1 = 0x20;
	    break;
	case DPMSModeOff:
	    /* Screen: Off; HSync: Off, VSync: Off */
	    seq1 = 0x20;
	    crtcext1 = 0x30;
	    break;
	}
	/* XXX Prefer an implementation that doesn't depend on VGA specifics */
	OUTREG8(0x1FC4, 0x01);	/* Select SEQ1 */
	seq1 |= INREG8(0x1FC5) & ~0x20;
	OUTREG8(0x1FC5, seq1);
	OUTREG8(0x1FDE, 0x01);	/* Select CRTCEXT1 */
	crtcext1 |= INREG8(0x1FDF) & ~0x30;
	OUTREG8(0x1FDF, crtcext1);
}
#endif

#if defined (DEBUG)
/*
 * some functions to track input/output in the server
 */

CARD8
dbg_inreg8(ScrnInfoPtr pScrn,int addr,int verbose)
{
    MGAPtr pMga;
    CARD8 ret;

    pMga = MGAPTR(pScrn);
    ret = *(volatile CARD8 *)(pMga->IOBase + (addr));
    if(verbose)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "inreg8 : 0x%8x = 0x%x\n",addr,ret);
    return ret;
}

CARD16
dbg_inreg16(ScrnInfoPtr pScrn,int addr,int verbose)
{
    MGAPtr pMga;
    CARD16 ret;

    pMga = MGAPTR(pScrn);
    ret = *(volatile CARD16 *)(pMga->IOBase + (addr));
    if(verbose)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "inreg16: 0x%8x = 0x%x\n",addr,ret);
    return ret;
}

CARD32
dbg_inreg32(ScrnInfoPtr pScrn,int addr,int verbose)
{
    MGAPtr pMga;
    CARD32 ret;

    pMga = MGAPTR(pScrn);
    ret = *(volatile CARD32 *)(pMga->IOBase + (addr));
    if(verbose)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "inreg32: 0x%8x = 0x%x\n",addr,ret);
    return ret;
}

void
dbg_outreg8(ScrnInfoPtr pScrn,int addr,int val)
{
    MGAPtr pMga;
    CARD8 ret;

    pMga = MGAPTR(pScrn);
#if 0
    if( addr = 0x1fdf )
    	return;
#endif
    if( addr != 0x3c00 ) {
	ret = dbg_inreg8(pScrn,addr,0);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg8 : 0x%8x = 0x%x was 0x%x\n",addr,val,ret);
    }
    else {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg8 : index 0x%x\n",val);
    }
    *(volatile CARD8 *)(pMga->IOBase + (addr)) = (val);
}

void
dbg_outreg16(ScrnInfoPtr pScrn,int addr,int val)
{
    MGAPtr pMga;
    CARD16 ret;

#if 0
    if (addr == 0x1fde)
    	return;
#endif
    pMga = MGAPTR(pScrn);
    ret = dbg_inreg16(pScrn,addr,0);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg16 : 0x%8x = 0x%x was 0x%x\n",addr,val,ret);
    *(volatile CARD16 *)(pMga->IOBase + (addr)) = (val);
}

void
dbg_outreg32(ScrnInfoPtr pScrn,int addr,int val)
{
    MGAPtr pMga;
    CARD32 ret;

    if (((addr & 0xff00) == 0x1c00) 
    	&& (addr != 0x1c04)
/*    	&& (addr != 0x1c1c) */
    	&& (addr != 0x1c20)
    	&& (addr != 0x1c24)
    	&& (addr != 0x1c80)
    	&& (addr != 0x1c8c)
    	&& (addr != 0x1c94)
    	&& (addr != 0x1c98)
    	&& (addr != 0x1c9c)
	 ) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "refused address 0x%x\n",addr);
    	return;
    }
    pMga = MGAPTR(pScrn);
    ret = dbg_inreg32(pScrn,addr,0);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "outreg32 : 0x%8x = 0x%x was 0x%x\n",addr,val,ret);
    *(volatile CARD32 *)(pMga->IOBase + (addr)) = (val);
}
#endif /* DEBUG */
