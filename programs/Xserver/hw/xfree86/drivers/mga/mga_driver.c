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
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_driver.c,v 1.34 1998/08/02 05:16:58 dawes Exp $ */

/*
 * This is a first cut at a non-accelerated version to work with the
 * new server design (DHD).
 */


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

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"
#include "xaa.h"


/* Leave out Mystique for testing */
#define NO_MYSTIQUE 1

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
/*
 * The major version is the 0xffff0000 part and the minor version is the
 * 0x0000ffff part.
 */
#define MGA_DRIVER_VERSION 0x00010001

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
    {-1,			NULL }
};

/* List of PCI chipset names */
static char *MGAPciNames[] = {
    "mga2064w",
    "mga1064sg",
    "mga2164w",
    "mga2164w AGP",
    NULL
};

/* List of PCI IDs */
static unsigned int MGAPciIds[] = {
    PCI_CHIP_MGA2064,
    PCI_CHIP_MGA1064,
    PCI_CHIP_MGA2164,
    PCI_CHIP_MGA2164_AGP,
    ~0
};

typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_SYNC_ON_GREEN,
    OPTION_NOACCEL,
    OPTION_SHOWCACHE
} MGAOpts;

static OptionInfoRec MGAOptions[] = {
#if 0
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
#endif
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_TRI,	{0}, FALSE },
    { OPTION_PCI_RETRY,		"PciRetry",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_SYNC_ON_GREEN,	"SyncOnGreen",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};


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
	MGA_DRIVER_VERSION,
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

/* 
 * ramdac info structure initialization
 */
static MGARamdacRec DacInit = {
	FALSE, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL,
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

    if ((numDevSections = xf86MatchDevice(MGA_DRIVER_NAME,
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

    numUsed = xf86MatchPciInstances(MGA_NAME, PCI_VENDOR_MATROX,
			MGAPciIds, MGAPciNames, devSections, numDevSections,
			&usedDevs, &usedPci);
    /* Free it since we don't need that list after this */
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];

	/*
	 * Check that nothing else has claimed the slots.
	 * For now we're checking for PCI_VGA, but that will change
	 * to PCI_ONLY when we remove the vgaHW dependence.
	 */
	
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
	
	pMga = MGAPTR(pScrn);
	pBios = &pMga->Bios;
	pBios2 = &pMga->Bios2;

	/* Make sure the BIOS is present */
	xf86ReadBIOS(pMga->BiosAddress, 0, tmp, sizeof( tmp ));
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
	xf86ReadBIOS( pMga->BiosAddress, 0x7ffc,
		( CARD8 * ) & offset, sizeof( offset ));


	/* Let the world know what we are up to */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Video BIOS info block at 0x%08lX\n",
		   (long)(pMga->BiosAddress + offset));

	/* Copy the info block */
	switch (pMga->Chipset){
	   case PCI_CHIP_MGA2064:
		xf86ReadBIOS( pMga->BiosAddress, offset,
			( CARD8 * ) & pBios->StructLen, sizeof( MGABiosInfo ));
		break;
	   default:
		xf86ReadBIOS( pMga->BiosAddress, offset,
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
 * MGACountRAM --
 *
 * Counts amount of installed RAM 
 */

/* XXX We need to get rid of this PIO (MArk) */
static int
MGACountRam(ScrnInfoPtr pScrn)
{
	MGAPtr pMga = MGAPTR(pScrn);

	if (pMga->FbAddress)
	{
		volatile unsigned char* base;
		unsigned char tmp, tmp3, tmp5;
	
		base = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				pMga->PciTag,
				(pointer)((unsigned long)pMga->FbAddress),
				8192 * 1024);
	
		/* turn MGA mode on - enable linear frame buffer (CRTCEXT3) */
		outb(0x3DE, 3);
		tmp = inb(0x3DF);
		outb(0x3DF, tmp | 0x80);
	
		/* write, read and compare method */
		base[0x500000] = 0x55;
		base[0x300000] = 0x33;
		base[0x100000] = 0x11;
		tmp5 = base[0x500000];
		tmp3 = base[0x300000];

		/* restore CRTCEXT3 state */
		outb(0x3DE, 3);
		outb(0x3DF, tmp);
	
		xf86UnMapVidMem(pScrn->scrnIndex, (pointer)base, 8192 * 1024);
	
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

#if SAMPLE
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
    pMga->HWCursor = FALSE;
    if (xf86GetOptValBool(MGAOptions, OPTION_HW_CURSOR, &pMga->HWCursor)) {
	from = X_CONFIG;
    }
#if 0
    if (xf86IsOptionSet(MGAOptions, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	pMga->HWCursor = FALSE;
    }
#endif
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
	if ( (pMga->Chipset == PCI_CHIP_MGA1064 && pMga->ChipRev >= 3) ||
		pMga->Chipset == PCI_CHIP_MGA2164 ||
		pMga->Chipset == PCI_CHIP_MGA2164_AGP) {
	    if (pMga->PciInfo->memBase[0] != 0) {
	    	pMga->FbAddress = pMga->PciInfo->memBase[0] & 0xff800000;
	    	from = X_PROBED;
	     } else {
	    	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "No valid FB address in PCI config space\n");
	    	MGAFreeRec(pScrn);
	    	return FALSE;
	     }
	} else {
	    if (pMga->PciInfo->memBase[1] != 0) {
	    	pMga->FbAddress = pMga->PciInfo->memBase[1] & 0xff800000;
	    	from = X_PROBED;
	     } else {
	    	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "No valid FB address in PCI config space\n");
	    	MGAFreeRec(pScrn);
	    	return FALSE;
	     }
        }

    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pMga->FbAddress);

    if (pScrn->device->IOBase != 0) {
	pMga->IOAddress = pScrn->device->IOBase;
	from = X_CONFIG;
    } else {
	/* details: mgabase1 sdk pp 4-11 */
	if ( (pMga->Chipset == PCI_CHIP_MGA1064 && pMga->ChipRev >= 3) ||
		pMga->Chipset == PCI_CHIP_MGA2164 ||
		pMga->Chipset == PCI_CHIP_MGA2164_AGP) {
	    if (pMga->PciInfo->memBase[1] != 0) {
	    	pMga->IOAddress = pMga->PciInfo->memBase[1] & 0xffffc000;
	    	from = X_PROBED;
	    } else {
	    	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"No valid MMIO address in PCI config space\n");
	    	MGAFreeRec(pScrn);
	    	return FALSE;
	    }
	} else {
	    if (pMga->PciInfo->memBase[0] != 0) {
	    	pMga->IOAddress = pMga->PciInfo->memBase[0] & 0xffffc000;
	    	from = X_PROBED;
	    } else {
	    	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"No valid MMIO address in PCI config space\n");
	    	MGAFreeRec(pScrn);
	    	return FALSE;
	    }
        }
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pMga->IOAddress);

    /*
     * Find the BIOS base.  Get it from the PCI config if possible.  Otherwise
     * use the VGA default.  Allow the config file to override this.
     */

    if (pScrn->device->BiosBase != 0) {
	pMga->BiosAddress = pScrn->device->BiosBase;
	from = X_CONFIG;
    } else {
	/* details: rombase sdk pp 4-15 */
	if (pMga->PciInfo->biosBase != 0) {
	    pMga->BiosAddress = pMga->PciInfo->biosBase & 0xffff0000;
	    from = X_PROBED;
	} else {
	    /* Need to watch this when removing VGA depencencies */
	    pMga->BiosAddress = 0xc0000;
	    from = X_DEFAULT;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "BIOS at 0x%lX\n",
	       (unsigned long)pMga->BiosAddress);

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
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	from = X_CONFIG;
    } else {
	pScrn->videoRam = MGACountRam(pScrn);
	from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
	
    pMga->FbMapSize = pScrn->videoRam * 1024;


    /* Setup the chipset and ramdac specific function pointers */
    switch (pMga->Chipset) {
    case PCI_CHIP_MGA2064:
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
	MGA2064SetupChipFuncs(pScrn);
	break;
    case PCI_CHIP_MGA1064:
#if !NO_MYSTIQUE
	MGA1064SetupChipFuncs(pScrn);
#endif
	break;
    default: return FALSE;
    }
	
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
	int speed;

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
    clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

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

    /* Check if interleaving can be used */
    switch (pMga->Chipset) {
    case PCI_CHIP_MGA1064:
	/* we can't use interleave on Mystique */
	pMga->Interleave = FALSE;
	break;
    case PCI_CHIP_MGA2064:
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
	if (pScrn->videoRam > 2048)
	    pMga->Interleave = TRUE;
	else
	    pMga->Interleave = FALSE;
	break;
    default:
	/* Shouldn't get here, but choose a safe default */
	pMga->Interleave = FALSE;
	break;
    }

    /* Set the rounding and bpp shift values */
    switch (pScrn->bitsPerPixel) {
    case 8:
	if (pMga->Interleave)
	    pMga->BppShift = 0;
	else
	    pMga->BppShift = 1;
	break;
    case 16:
	if (pMga->Interleave)
	    pMga->BppShift = 1;
	else
	    pMga->BppShift = 2;
	break;
    case 24:
	if (pMga->Interleave)
	    pMga->BppShift = 0;
	else
	    pMga->BppShift = 1;
	break;
    case 32:
	if (pMga->Interleave)
	    pMga->BppShift = 2;
	else
	    pMga->BppShift = 3;
	break;
    }
    pMga->Rounding = 128 >> pMga->BppShift;
    if (pMga->Chipset == PCI_CHIP_MGA1064) {
	pMga->BppShift--;
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
	while ((offset % offset_modulo) != 0 ||
	       (pMga->YDstOrg % ydstorg_modulo) != 0)
	{
	    offset++;
	    pMga->YDstOrg = offset / bytesPerPixel;
	}
    }

    /* Set Fast bitblt flag */
    if (pMga->Chipset == PCI_CHIP_MGA1064) {
	pMga->HasFBitBlt = FALSE;
    } else {
	pMga->HasFBitBlt = !(pMga->Bios.FeatFlag & 0x00000001);
    }

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
	MGAFreeRec(pScrn);
	return FALSE;
    }

    /* Load XAA if needed */
    if (!pMga->NoAccel || pMga->HWCursor)
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    MGAFreeRec(pScrn);
	    return FALSE;
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
					pMga->PciTag, (pointer)pMga->IOAddr,
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
    return TRUE;
}


/*
 * This function saves the video state.
 */
static void
MGASave(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);

    (*pMga->Save)(pScrn);
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
MGAModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    MGAPtr pMga = MGAPTR(pScrn);

    return((*pMga->ModeInit)(pScrn, mode));
}

/*
 * Restore the initial (text) mode.
 */
static void 
MGARestore(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);

    (*pMga->Restore)(pScrn);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
MGAScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    /* The vgaHW references will disappear one day */
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    MGAPtr pMga;
    int ret;
    VisualPtr visual;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    hwp = VGAHWPTR(pScrn);

    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    pMga = MGAPTR(pScrn);

    /* Map the VGA memory and get the VGA IO base */
    if (!vgaHWMapMem(pScrn))
	return FALSE;
    vgaHWGetIOBase(hwp);

    /* Map the MGA memory and MMIO areas */
    if (!MGAMapMem(pScrn))
	return FALSE;

    /* Save the current state */
    MGASave(pScrn);

    /* Initialise the first mode */
    if (!MGAModeInit(pScrn, pScrn->currentMode))
	return FALSE;

    /* Darken the screen for aesthetic reasons and set the viewport */
    MGASaveScreen(pScreen, FALSE);
    MGAAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    /* XXX Fill the screen with black */
#if 0
    MGASaveScreen(pScreen, TRUE);
#endif

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
    case 8:
	ret = cfbScreenInit(pScreen, pMga->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, pMga->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, pMga->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, pMga->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in MGAScrnInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->bitsPerPixel == 8) {
	/* Another VGA dependency to remove */
	MGAHandleColormaps(pScreen, pScrn);
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

    if (!pMga->NoAccel)
	MGAStormAccelInit(pScreen);

    miInitializeBackingStore(pScreen);

    /* Initialise cursor functions */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    if (pMga->HWCursor) { /* Initialize HW cursor layer */
	if(!MGAHWCursorInit(pScreen))
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
		"Hardware cursor initialization failed\n");
    }


    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
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
	int lastline = pMga->FbMapSize / 
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
    while (inb(hwp->IOBase + 0x0A) & 0x08);
    while (!(inb(hwp->IOBase + 0xA) & 0x08)); 
    /* wait until we're past the start (fixseg.c in the DDK) */
    count = INREG(MGAREG_VCOUNT) + 2;
    while(INREG(MGAREG_VCOUNT) < count);
    
    outw(hwp->IOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(hwp->IOBase + 4, ((Base & 0x0000FF) << 8) | 0x0D);
    outb(0x3DE, 0x00);
    tmp = inb(0x3DF);
    outb(0x3DF, (tmp & 0xF0) | ((Base & 0x0F0000) >> 16));

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

    /* Should we re-save the text mode on each VT enter? */
    return MGAModeInit(pScrn, pScrn->currentMode);
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
    	XAADestroyCursorInfoRec(pMga->CursorInfoRec);
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
	unsigned char seq1, crtcext1;

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
	outb(0x3C4, 0x01);	/* Select SEQ1 */
	seq1 |= inb(0x3C5) & ~0x20;
	outb(0x3C5, seq1);
	outb(0x3DE, 0x01);	/* Select CRTCEXT1 */
	crtcext1 |= inb(0x3DF) & ~0x30;
	outb(0x3DF, crtcext1);
}
#endif

