/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3v_driver.c,v 1.3 1998/11/22 10:37:30 dawes Exp $ */

/*
 *
 * Copyright 1998 The XFree86 Project, Inc.
 *
 */


/*
 * s3v_driver.c
 * Port to 4.0 design level
 *
 * S3 ViRGE driver
 *
 * based largely on the SVGA driver,
 * Started 09/03/97 by S. Marineau
 *
 *
 */


	/* Most xf86 commons are already in s3v.h */
#include	"s3v.h"
		
  
/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */
static void S3VIdentify(int flags);
static Bool S3VProbe(DriverPtr drv, int flags);
static Bool S3VPreInit(ScrnInfoPtr pScrn, int flags);

static Bool S3VEnterVT(int scrnIndex, int flags);
static void S3VLeaveVT(int scrnIndex, int flags);
static void S3VSave (ScrnInfoPtr pScrn);
static void S3VWriteMode (ScrnInfoPtr pScrn, vgaRegPtr, S3VRegPtr);

static void S3VSaveSTREAMS(ScrnInfoPtr pScrn, unsigned int *streams);
static void S3VRestoreSTREAMS(ScrnInfoPtr pScrn, unsigned int *streams);
static void S3VDisableSTREAMS(ScrnInfoPtr pScrn);
static Bool S3VScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv);
static void S3VPrintRegs(ScrnInfoPtr);
static ModeStatus S3VValidMode(int index, DisplayModePtr mode, Bool verbose, int flags);

static Bool S3VMapMem(ScrnInfoPtr pScrn);
static void S3VUnmapMem(ScrnInfoPtr pScrn);
static Bool S3VModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool S3VCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool S3VSaveScreen(ScreenPtr pScreen, Bool unblank);
static void S3VInitSTREAMS(ScrnInfoPtr pScrn, unsigned int *streams, DisplayModePtr mode);
static void S3VAdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool S3VSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
static void S3VLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indicies, LOCO *colors, short visualClass);


/*
 * Externs needed here
 *
 * in s3v_pio.c
 */
extern void S3VEnableMmio(ScrnInfoPtr pScrn);
extern void S3VDisableMmio(ScrnInfoPtr pScrn);
extern void commonCalcClock(long freq, int min_m, int min_n1, int max_n1, int min_n2, int max_n2, 
		long freq_min, long freq_max,
		unsigned char * mdiv, unsigned char * ndiv);

/*
 * in s3v_accel.c
 */
extern Bool S3VAccelInit(ScreenPtr pScreen);
extern Bool S3VAccelInit32(ScreenPtr pScreen);


#define VERSION 4000
#define S3VIRGE_NAME "S3VIRGE"
#define S3VIRGE_DRIVER_NAME "s3virge"
				/* Version 0.1 */
				/* upper word is Major, lower is minor */
				/* 0000 0001 == 0.1 */
#define S3VIRGE_DRIVER_VERSION 0x00000001

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec S3VIRGE =
{
    VERSION,
    "driver for S3 ViRGE based cards, including /DX, /GX(2), /VX, & /MX",
    S3VIdentify,
    S3VProbe,
    NULL,
    0
};


/* Supported chipsets */
static SymTabRec S3VChipsets[] = {
				  	/* base (86C325) */
  { PCI_ViRGE,		"ViRGE" },
  					/* VX (86C988) */
  { PCI_ViRGE_VX,	"ViRGE/VX" },
  					/* DX (86C375) GX (86C385) */
  { PCI_ViRGE_DXGX,	"ViRGE/DX/GX" },
  					/* GX2 (86C357) */
  { PCI_ViRGE_GX2,	"ViRGE/GX2" },
  					/* MX (86C260) */
  { PCI_ViRGE_MX,	"ViRGE/MX" },
  					/* MX+ (86C280) */
  /* { PCI_ViRGE_MX+,	"ViRGE/ MX+" },  not yet, check 3.3.3 for PCI id */
  {-1,			NULL }
};

static PciChipsets S3VPciChipsets[] = {
  { PCI_ViRGE,      PCI_ViRGE,      RES_SHARED_VGA },
  { PCI_ViRGE_VX,   PCI_ViRGE_VX,   RES_SHARED_VGA },
  { PCI_ViRGE_DXGX, PCI_ViRGE_DXGX, RES_SHARED_VGA },
  { PCI_ViRGE_GX2,  PCI_ViRGE_GX2,  RES_SHARED_VGA },
  { PCI_ViRGE_MX,   PCI_ViRGE_MX,   RES_SHARED_VGA },
  /* add MX+ */
  { -1,                       -1,   RES_UNDEFINED }
};

typedef enum {		    
   OPTION_SLOW_EDODRAM, 	
   OPTION_FAST_DRAM, 		
   OPTION_FPM_VRAM, 		
   OPTION_PCI_BURST_ON, 	
   OPTION_FIFO_CONSERV, 	
   OPTION_FIFO_MODERATE, 	
   OPTION_FIFO_AGGRESSIVE, 	
   OPTION_PCI_RETRY, 		
   OPTION_NOACCEL, 		
 /*
   OPTION_HW_CURSOR, 		
   */
   OPTION_EARLY_RAS_PRECHARGE, 	
   OPTION_LATE_RAS_PRECHARGE
} S3VOpts;

static OptionInfoRec S3VOptions[] =
{  
   { OPTION_SLOW_EDODRAM, 	"slow_edodram",	OPTV_BOOLEAN,	{0}, FALSE },
 /* not yet ...
   { OPTION_FAST_DRAM, 		"fast_dram",	OPTV_BOOLEAN,	{0}, FALSE },
  */ 
   { OPTION_FPM_VRAM, 		"fpm_vram",	OPTV_BOOLEAN,	{0}, FALSE },
   { OPTION_PCI_BURST_ON, 	"pci_burst_on",	OPTV_BOOLEAN,	{0}, FALSE },
   { OPTION_FIFO_CONSERV, 	"fifo_conservative", OPTV_BOOLEAN, {0}, FALSE },
   { OPTION_FIFO_MODERATE, 	"fifo_moderate", OPTV_BOOLEAN, 	{0}, FALSE },
   { OPTION_FIFO_AGGRESSIVE, 	"fifo_aggressive", OPTV_BOOLEAN, {0}, FALSE },
   { OPTION_PCI_RETRY, 		"pci_retry",	OPTV_BOOLEAN,	{0}, FALSE  },
   { OPTION_NOACCEL, 		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE  },
 /*
   { OPTION_HW_CURSOR, 		},
  */ 
   { OPTION_EARLY_RAS_PRECHARGE, "early_ras_precharge",	OPTV_BOOLEAN, {0}, FALSE },
   { OPTION_LATE_RAS_PRECHARGE, "late_ras_precharge", OPTV_BOOLEAN, {0}, FALSE },
   {-1, NULL, OPTV_NONE,
	{0}, FALSE}
};


/*
 * Lists of symbols that may/may not be required by this driver.
 * This allows the loader to know which ones to issue warnings for.
 */

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWSetMmioFuncs",
    "vgaHWGetIOBase",
    "vgaHWSave",
    "vgaHWProtect",
    "vgaHWRestore",
    "vgaHWMapMem",
    "vgaHWUnmapMem",
    "vgaHWInit",
    "vgaHWSaveScreen",
    "vgaHWLock",
   /* not used by ViRGE (at the moment :( ) */
   /*
    "vgaHWUnlock",
    "vgaHWFreeHWRec",
    */
    NULL
};

static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    NULL
};

static const char *xaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAAInit",
   /*
    "XAAStippleScanlineFuncLSBFirst",
    "XAAOverlayFBfuncs",
    */
    NULL
};

static const char *ramdacSymbols[] = {
   /* so far, none */
   /*
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    */
    NULL
};


#ifdef XFree86LOADER

MODULEINITPROTO(s3virgeModuleInit);
static MODULESETUPPROTO(s3virgeSetup);

static XF86ModuleVersionInfo S3VVersRec =
{
    "s3virge",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    S3VIRGE_DRIVER_VERSION,
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
s3virgeModuleInit(XF86ModuleVersionInfo ** vers, ModuleSetupProc * setup,
    ModuleTearDownProc * teardown)
{
    *vers = &S3VVersRec;
    *setup = s3virgeSetup;
    *teardown = NULL;
}

static pointer
s3virgeSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&S3VIRGE, module, 0);

	/*
	 * Modules that this driver always requires can be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
	LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols,
			  ramdacSymbols, NULL);
			  
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
S3VGetRec(ScrnInfoPtr pScrn)
{
    PVERB5("	S3VGetRec\n");
    /*
     * Allocate an 'Chip'Rec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(S3VRec), 1);
    /* Initialise it here when needed (or possible) */

    return TRUE;
}

static void
S3VFreeRec(ScrnInfoPtr pScrn)
{
    PVERB5("	S3VFreeRec\n");
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static void
S3VIdentify(int flags)
{
    PVERB5("	S3VIdentify\n");
    xf86PrintChipsets(S3VIRGE_NAME, "driver for S3 ViRGE chipsets",
	S3VChipsets);
}


static Bool
S3VProbe(DriverPtr drv, int flags)
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
    
    PVERB5("	S3VProbe begin\n");
    
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
     */
     
    /*
     * Try for support without vgaHW...
    if (xf86CheckIsaSlot(ISA_COLOR) == FALSE) {
	return FALSE;
    }
     */
     

    /*
     * Next we check, if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(S3VIRGE_DRIVER_NAME,
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

    numUsed = xf86MatchPciInstances(S3VIRGE_NAME, PCI_S3_VENDOR_ID,
				    /*	S3VPciIds, S3VPciNames*/
			S3VChipsets, S3VPciChipsets, devSections, numDevSections,
			&usedDevs, &usedPci, &usedChips);
    /* Free it since we don't need that list after this */
    xfree(devSections);
    devSections = NULL;
    if (numUsed <= 0)
	return FALSE;

    for (i = 0; i < numUsed; i++) {
	pPci = usedPci[i];
	/* Another copy from MGA {KF} */
	resource = xf86FindPciResource(usedChips[i], S3VPciChipsets);
	/*
	 * PCI_ONLY for use without vgaHW, use PCI_VGA otherwise.
	 */
	/*  Obsolete ?! ^^-> resource {KF}*/
	if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func,resource)) {
	    ScrnInfoPtr pScrn;

	    /* Allocate a ScrnInfoRec and claim the slot */
	    pScrn = xf86AllocateScreen(drv, 0);
	    if (!xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func, 
				  resource, &S3VIRGE, usedChips[i], 
				  pScrn->scrnIndex)) {
		/* This can't happen */
	      /*Need this ?{KF}*/
		FatalError("someone claimed the free slot!\n");
	    }
	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion = VERSION;
	    pScrn->driverName	 = S3VIRGE_DRIVER_NAME;
	    pScrn->name		 = S3VIRGE_NAME;
	    pScrn->Probe	 = S3VProbe;
	    pScrn->PreInit	 = S3VPreInit;
	    pScrn->ScreenInit	 = S3VScreenInit;
	    pScrn->SwitchMode	 = S3VSwitchMode;
	    pScrn->AdjustFrame	 = S3VAdjustFrame;
	    pScrn->EnterVT	 = S3VEnterVT;
	    pScrn->LeaveVT	 = S3VLeaveVT;
	    pScrn->FreeScreen	 = NULL; /*S3VFreeScreen;*/
	    pScrn->ValidMode	 = S3VValidMode;
	    pScrn->device	 = usedDevs[i];
	    foundScreen = TRUE;
	}
    }
    xfree(usedDevs);
    xfree(usedPci);
    PVERB5("	S3VProbe end\n");
    return foundScreen;
}


/* Mandatory */
static Bool
S3VPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    S3VPtr ps3v;
    MessageType from;
    int i;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *reqSym = NULL;

    unsigned char config1, config2, m, n, n1, n2, cr66;
    int mclk;
    
  vgaHWPtr hwp;
  int vgaCRIndex, vgaCRReg, vgaIOBase;
    
    			      
    PVERB5("	S3VPreInit 1\n");
    	      
  
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
    } else {		/* editme - from MGA, does ViRGE? */
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
     * here. (from MGA, no ViRGE gamma support yet, but needed for 
     * xf86HandleColormaps support. )
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Allocate the S3VRec driverPrivate */
    if (!S3VGetRec(pScrn)) {
	return FALSE;
    }
    ps3v = S3VPTR(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);
	       
    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
    				/* ViRGE supports 6 RGB bits in depth 8 */
				/* modes (with 256 entry LUT) */
      pScrn->rgbBits = 6;
    }
    
    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, S3VOptions);


    if (xf86IsOptionSet(S3VOptions, OPTION_PCI_BURST_ON)) {
	ps3v->pci_burst_on = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: pci_burst_on - PCI burst read enabled\n");
    } else
   	ps3v->pci_burst_on = FALSE;
					/* default */
    ps3v->NoPCIRetry = 1;
   					/* Set option */
    if (xf86IsOptionSet(S3VOptions, OPTION_PCI_RETRY))
      if (xf86IsOptionSet(S3VOptions, OPTION_PCI_BURST_ON)) {
      	ps3v->NoPCIRetry = 0;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: pci_retry\n");
	}
      else {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"pci_retry\" option requires \"pci_burst_on\".\n");
	}

    if (xf86IsOptionSet(S3VOptions, OPTION_FIFO_CONSERV)) {
	ps3v->fifo_conservative = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: fifo_conservative set\n");
    } else
   	ps3v->fifo_conservative = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_FIFO_MODERATE)) {
	ps3v->fifo_moderate = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: fifo_moderate set\n");
    } else
   	ps3v->fifo_moderate = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_FIFO_AGGRESSIVE)) {
	ps3v->fifo_aggressive = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: fifo_aggressive set\n");
    } else
   	ps3v->fifo_aggressive = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_SLOW_EDODRAM)) {
	ps3v->slow_edodram = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: slow_edodram set\n");
    } else
   	ps3v->slow_edodram = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_FAST_DRAM)) {
	ps3v->fast_dram = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: fast_dram set\n");
    } else
   	ps3v->fast_dram = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_FPM_VRAM)) {
	ps3v->fpm_vram = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: fpm_vram set\n");
    } else
   	ps3v->fpm_vram = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_NOACCEL)) {
	ps3v->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: NoAccel - Acceleration disabled\n");
    } else
   	ps3v->NoAccel = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_EARLY_RAS_PRECHARGE)) {
	ps3v->early_ras_precharge = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: early_ras_precharge set\n");
    } else
   	ps3v->early_ras_precharge = FALSE;

    if (xf86IsOptionSet(S3VOptions, OPTION_LATE_RAS_PRECHARGE)) {
	ps3v->late_ras_precharge = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: late_ras_precharge set\n");
    } else
   	ps3v->late_ras_precharge = FALSE;
	       
    /* Find the PCI slot for this screen */
    /*
     * XXX Ignoring the Type list for now.  It might be needed when
     * multiple cards are supported.
     */
    if ((i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL)) != 1) {
	/* This shouldn't happen */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Expected one PCI card, but found %d\n", i);
	S3VFreeRec(pScrn);
	if (i > 0)
	    xfree(pciList);
	return FALSE;
    }

    ps3v->PciInfo = *pciList;
    xfree(pciList);
    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pScrn->device->chipset && *pScrn->device->chipset) {
	pScrn->chipset = pScrn->device->chipset;
        ps3v->Chipset = xf86StringToToken(S3VChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pScrn->device->chipID >= 0) {
	ps3v->Chipset = pScrn->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(S3VChipsets, ps3v->Chipset);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   ps3v->Chipset);
    } else {
	from = X_PROBED;
	ps3v->Chipset = ps3v->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(S3VChipsets, ps3v->Chipset);
    }								    
    
    if (pScrn->device->chipRev >= 0) {
	ps3v->ChipRev = pScrn->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   ps3v->ChipRev);
    } else {
	ps3v->ChipRev = ps3v->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * S3VProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", ps3v->Chipset);
	return FALSE;
    }
    if (ps3v->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);
	
    ps3v->PciTag = pciTag(ps3v->PciInfo->bus, ps3v->PciInfo->device,
			  ps3v->PciInfo->func);
			  
    					/* Map the ViRGE register space */
					/* Starts with PCI registers */
					/* around 0x18000 from MemBase */
    ps3v->IOBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, ps3v->PciTag,
			(pointer) (ps3v->PciInfo->memBase[0] + S3_NEWMMIO_VGABASE),
			S3V_MMIO_REGSIZE );
  		   
					/* Add me to resource management */
    xf86AddControlledResource(pScrn, MEM_IO);
    					/* Enable card from server side  */
    xf86EnableAccess(&pScrn->Access);
					/* Enable MMIO on the ViRGE */
    S3VEnableMmio( pScrn);

		/********************************************************/
		/* Aaagh...  So many locations!  On my machine (KJB) the*/
		/* following is true. 					*/
		/* PciInfo->memBase[0] returns e400 0000 		*/
		/* From my ViRGE manual, the memory map looks like 	*/
		/* Linear mem - 16M  	000 0000 - 0ff ffff 		*/
		/* Image xfer - 32k  	100 0000 - 100 7fff 		*/
		/* PCI cnfg    		100 8000 - 100 8043 		*/
		/* ...				   			*/
		/* CRT VGA 3b? reg	100 83b0 - 			*/
		
		/* And S3_NEWMMIO_VGABASE = S3_NEWMMIO_REGBASE + 0x8000	*/
		/* where S3_NEWMMIO_REGBASE = 0x100 0000  ( 16MB )      */
		/* S3_NEWMMIO_REGSIZE = 0x1 0000  ( 64KB )		*/
		/* S3V_MMIO_REGSIZE = 0x8000 ( 32KB ) - above includes	*/
		/* the image transfer area, so this one is used instead.*/
		
		/* ps3v->IOBase is assinged the virtual address returned*/
		/* from MapPciMem, it is the address to base all 	*/
		/* register access. (It is a pointer.)  		*/
		
		/* hwp->MemBase is a CARD32, containing the register	*/
		/* base. (It's a conversion from IOBase above.) 	*/


  hwp = VGAHWPTR(pScrn);
		  			/* Note that for convience during */
					/* Init we have mapped at 0 offset*/
  vgaHWSetMmioFuncs( hwp, ps3v->IOBase, 0 );
  
    xf86ErrorFVerb(VERBLEV, 
	"	S3VPreInit hwp=%p, hwp->MMIOBase=%x\n",
		hwp, hwp->MMIOBase );
	
  					/* assigns hwp->IOBase to 3D0 or 3B0 */
					/* needs hwp->MMIOBase to work */
  vgaHWGetIOBase(hwp);
  vgaIOBase = hwp->IOBase;
  vgaCRIndex = vgaIOBase + 4;
  vgaCRReg = vgaIOBase + 5;

    xf86ErrorFVerb(VERBLEV, 
	"	S3VPreInit vgaCRIndex=%x, vgaIOBase=%x, MMIOBase=%x\n", 
	vgaCRIndex, vgaIOBase, hwp->MMIOBase );


   #if 0	/* Not needed in 4.0 flavors */
   /* Unlock sys regs */
   OUTREG8(vgaCRIndex, 0x38);
   OUTREG8(vgaCRReg, 0x48);
   #endif
 
   /* Next go on to detect amount of installed ram */

   OUTREG8(vgaCRIndex, 0x36);              /* for register CR36 (CONFG_REG1), */
   config1 = INREG8(vgaCRReg);              /* get amount of vram installed */

   OUTREG8(vgaCRIndex, 0x37);              /* for register CR37 (CONFG_REG2), */
   config2 = INREG8(vgaCRReg);             /* get amount of off-screen ram  */

   /* And compute the amount of video memory and offscreen memory */
   ps3v->MemOffScreen = 0;
   if (!pScrn->videoRam) {
      if (ps3v->Chipset == S3_ViRGE_VX) {
         switch((config2 & 0x60) >> 5) {
         case 1:
            ps3v->MemOffScreen = 4 * 1024;
            break;
         case 2:
            ps3v->MemOffScreen = 2 * 1024;
            break;
         }
         switch ((config1 & 0x60) >> 5) {
         case 0:
            ps3v->videoRamKbytes = 2 * 1024;
            break;
         case 1:
            ps3v->videoRamKbytes = 4 * 1024;
            break;
         case 2:
            ps3v->videoRamKbytes = 6 * 1024;
            break;
         case 3:
            ps3v->videoRamKbytes = 8 * 1024;
            break;
         }
         ps3v->videoRamKbytes -= ps3v->MemOffScreen;
      }
      else {
         switch((config1 & 0xE0) >> 5) {
         case 0:
            ps3v->videoRamKbytes = 4 * 1024;
            break;
         case 4:
            ps3v->videoRamKbytes = 2 * 1024;
            break;
         case 6:
            ps3v->videoRamKbytes = 1 * 1024;
            break;
         }
      }
      					/* And save a byte value also */
      ps3v->videoRambytes = ps3v->videoRamKbytes * 1024;
      				       	/* Make sure the screen also */
					/* has correct videoRam setting */
      pScrn->videoRam = ps3v->videoRamKbytes;

      if (ps3v->MemOffScreen)
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	    	"videoram:  %dk (plus %dk off-screen)\n",
                ps3v->videoRamKbytes, ps3v->MemOffScreen);
      else
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "videoram:  %dk\n",
                ps3v->videoRamKbytes);
   } else {
   					/* Note: if ram is not probed then */
					/* ps3v->videoRamKbytes will not be init'd */
					/* should we? can do it here... */
					
					
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "videoram:  %dk\n",
              	ps3v->videoRamKbytes);
   }


   /* reset S3 graphics engine to avoid memory corruption */
   if (ps3v->Chipset != S3_ViRGE_VX) {
      OUTREG8(vgaCRIndex, 0x66);
      cr66 = INREG8(vgaCRReg);
      OUTREG8(vgaCRReg, cr66 | 0x02);
      usleep(10000);  /* wait a little bit... */
   }

/*cep*/
#if 0		/* write find_bios_string... */
   if (find_bios_string(vga256InfoRec.BIOSbase,"S3 86C325",
			"MELCO WGP-VG VIDEO BIOS") != NULL) {
      if (xf86Verbose)
	 ErrorF("%s %s: MELCO BIOS found\n",
		XCONFIG_PROBED, vga256InfoRec.name);
      if (vga256InfoRec.MemClk <= 0)       vga256InfoRec.MemClk       =  74000;
      if (pScrn->clock[0] <= 0) pScrn->clock[0] = 191500;
      if (pScrn->clock[1] <= 0) pScrn->clock[1] = 162500;
      if (pScrn->clock[2] <= 0) pScrn->clock[2] = 111500;
      if (pScrn->clock[3] <= 0) pScrn->clock[3] =  83500;
   }
#endif

   if (ps3v->Chipset != S3_ViRGE_VX) {
      OUTREG8(vgaCRIndex, 0x66);
      OUTREG8(vgaCRReg, cr66 & ~0x02);  /* clear reset flag */
      usleep(10000);  /* wait a little bit... */
   }

   /* ViRGE built-in ramdac speeds */
   					/* ViRGE has four default clocks */
   pScrn->numClocks = 4;
   
   if (pScrn->clock[3] <= 0 && pScrn->clock[2] > 0)
      pScrn->clock[3] = pScrn->clock[2];

   if (ps3v->Chipset == S3_ViRGE_VX) {
      if (pScrn->clock[0] <= 0) pScrn->clock[0] = 220000;
      if (pScrn->clock[1] <= 0) pScrn->clock[1] = 220000;
      if (pScrn->clock[2] <= 0) pScrn->clock[2] = 135000;
      if (pScrn->clock[3] <= 0) pScrn->clock[3] = 135000;
   }
   else if (ps3v->Chipset == S3_ViRGE_DXGX || ps3v->Chipset == S3_ViRGE_GX2) {
      if (pScrn->clock[0] <= 0) pScrn->clock[0] = 170000;
      if (pScrn->clock[1] <= 0) pScrn->clock[1] = 170000;
      if (pScrn->clock[2] <= 0) pScrn->clock[2] = 135000;
      if (pScrn->clock[3] <= 0) pScrn->clock[3] = 135000;
   }
   else {
      if (pScrn->clock[0] <= 0) pScrn->clock[0] = 135000;
      if (pScrn->clock[1] <= 0) pScrn->clock[1] =  95000;
      if (pScrn->clock[2] <= 0) pScrn->clock[2] =  57000;
      if (pScrn->clock[3] <= 0) pScrn->clock[3] =  57000;
   }
   
   if (ps3v->dacSpeedBpp <= 0)
      if (pScrn->bitsPerPixel > 24 && pScrn->clock[3] > 0)
	 ps3v->dacSpeedBpp = pScrn->clock[3];
      else if (pScrn->bitsPerPixel >= 24 && pScrn->clock[2] > 0)
	 ps3v->dacSpeedBpp = pScrn->clock[2];
      else if (pScrn->bitsPerPixel > 8 && pScrn->bitsPerPixel < 24 && pScrn->clock[1] > 0)
	 ps3v->dacSpeedBpp = pScrn->clock[1];
      else if (pScrn->bitsPerPixel <= 8 && pScrn->clock[0] > 0)
	 ps3v->dacSpeedBpp = pScrn->clock[0];
	 
/*cep*/
#if 0
   if (xf86Verbosity()) {
      ErrorF("%s %s: Ramdac speed: %d MHz",
	     OFLG_ISSET(XCONFIG_DACSPEED, &vga256InfoRec.xconfigFlag) ?
	     XCONFIG_GIVEN : XCONFIG_PROBED, vga256InfoRec.name,
	     pScrn->clock[0] / 1000);
      if (ps3v->dacSpeedBpp != pScrn->clock[0])
	 ErrorF("  (%d MHz for %d bpp)",ps3v->dacSpeedBpp / 1000, pScrn->bitsPerPixel);
      ErrorF("\n");
   }
#endif

   /* Now set RAMDAC limits */
   /*ps3v->maxClock = ps3v->dacSpeedBpp;*/
  if (ps3v->Chipset == S3_ViRGE_VX ) {
    ps3v->maxClock = 440000;
  } else {
    ps3v->maxClock = 270000;
  }
    

   /* Detect current MCLK and print it for user */
   OUTREG8(0x3c4, 0x08);
   OUTREG8(0x3c5, 0x06); 
   OUTREG8(0x3c4, 0x10);
   n = INREG8(0x3c5);
   OUTREG8(0x3c4, 0x11);
   m = INREG8(0x3c5);
   m &= 0x7f;
   n1 = n & 0x1f;
   n2 = (n>>5) & 0x03;
   mclk = ((1431818 * (m+2)) / (n1+2) / (1 << n2) + 50) / 100;
   xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected current MCLK value of %1.3f MHz\n",
	     mclk / 1000.0);

  S3VDisableMmio(pScrn);
  
  xf86UnMapVidMem(pScrn->scrnIndex, (pointer)ps3v->IOBase, S3V_MMIO_REGSIZE);

#if 0
   /* Now check if the user has specified "set_memclk" value in XConfig */
   if (vga256InfoRec.MemClk > 0) {
      if(vga256InfoRec.MemClk <= 100000) {
         ErrorF("%s %s: Using Memory Clock value of %1.3f MHz\n",
		OFLG_ISSET(XCONFIG_DACSPEED, &vga256InfoRec.xconfigFlag) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, vga256InfoRec.name, 
		vga256InfoRec.MemClk/1000.0);
         s3vPriv.MCLK = vga256InfoRec.MemClk;
         }
      else {
         ErrorF("%s %s: Memory Clock value of %1.3f MHz is larger than limit of 100 MHz\n",
              XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.MemClk/1000.0);
         s3vPriv.MCLK = 0;
         }
      }
   else s3vPriv.MCLK = 0;
#endif

   /* Set scale factors for mode timings */

   if (ps3v->Chipset == S3_ViRGE_VX){
      ps3v->HorizScaleFactor = 1;
      }
   else if (pScrn->bitsPerPixel == 8){
      ps3v->HorizScaleFactor = 1;
      }
   else if (pScrn->bitsPerPixel == 16){
      ps3v->HorizScaleFactor = 2;
      }
   else {     
      ps3v->HorizScaleFactor = 1;
      }


   /* And finally set various possible option flags */

   ps3v->bankedMono = FALSE;


#if 0
#ifdef XFreeXDGA
   vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif


	/* find BIOS base?  See above, do MELCO bios detect */
	
	

/* s3v_driver.c.old above */   
/****************************************************/
/* converted mgastuff */

  #if 0
  if (ps3v->Chipset == S3_ViRGE_VX ) {
    ps3v->minClock = 220000;
  } else {
    ps3v->minClock = 135000;
  }
  #else
    ps3v->minClock = 20000;  /* cep */
  #endif
  
    xf86ErrorFVerb(VERBLEV, 
	"	S3VPreInit minClock=%d, maxClock=%d\n",
		ps3v->minClock,
		ps3v->maxClock
		 );
  
    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  
     */
			 	/* true for all ViRGE? */
  pScrn->maxHValue = 2048;
  pScrn->maxVValue = 2048;

    				/* Lower depths default to config file */
  pScrn->virtualX = pScrn->display->virtualX;
				/* Adjust the virtualX to meet ViRGE hardware */
				/* limits for depth 24, bpp 24 & 32.  This is */
				/* mostly for 32 bpp as 1024x768 is one pixel */
				/* larger than supported. */
  if (pScrn->depth == 24)
      if ( ((pScrn->bitsPerPixel/8) * pScrn->display->virtualX) > 4095 ) {
        pScrn->virtualX = 4095 / (pScrn->bitsPerPixel / 8);
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
	   "Virtual width adjusted, max for this depth & bpp is %d.\n",
	   pScrn->virtualX );
      }
  
    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->minClock = ps3v->minClock;
    clockRanges->maxClock = ps3v->maxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;	/* yes, S3V SVGA 3.3.2 */
    clockRanges->doubleScanAllowed = FALSE;	/* no, S3V SVGA 3.3.2 */
  
  					/* Screen pointer 		*/
    i = xf86ValidateModes(pScrn, 
  					/* Available monitor modes 	*/
					/* (DisplayModePtr availModes)  */
		pScrn->monitor->Modes,
					/* req mode names for screen 	*/
					/* (char **modesNames)  	*/
		pScrn->display->modes, 
					/* list of clock ranges allowed */
					/* (ClockRangePtr clockRanges) 	*/
		clockRanges,
					/* list of driver line pitches, */
					/* supply or NULL and use min/	*/
					/* max below			*/
					/* (int *linePitches)		*/
		NULL, 
					/* min lin pitch (width)	*/
					/* (int minPitch)		*/
		256, 
					/* max line pitch (width)	*/
					/* (int maxPitch)		*/
		2048,
					/* bits of granularity for line	*/
					/* pitch (width) above, reguired*/
					/* (int pitchInc)		*/
		pScrn->bitsPerPixel,
					/* min virt height, 0 no limit	*/
					/* (int minHeight)		*/
		128, 
					/* max virt height, 0 no limit	*/
					/* (int maxHeight)		*/
		2048,
					/* force virtX, 0 for auto 	*/
					/* (int VirtualX) 		*/
					/* value is adjusted above for  */
					/* hardware limits */
		pScrn->virtualX,
					/* force virtY, 0 for auto	*/
					/* (int VirtualY)		*/
		pScrn->display->virtualY,
					/* size (bytes) of aper used to	*/
					/* access video memory		*/
					/* (unsigned long apertureSize)	*/
		ps3v->videoRambytes,
					/* how to pick mode */
					/* (LookupModeFlags strategy)	*/
		LOOKUP_BEST_REFRESH);
  
    if (i == -1) {
	S3VFreeRec(pScrn);
	return FALSE;
    }
    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	S3VFreeRec(pScrn);
	return FALSE;
    }

    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);
    /*xf86SetCrtcForModes(pScrn, 0);*/

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);
    					/* When running the STREAMS processor */
					/* the max. stride is limited to 4096-1 */
					/* so this is the virtualX limit. */
					/* STREAMS is needed for 24 & 32 bpp, */
					/* (all depth 24 modes) */
					/* This should never happen... we */
					/* checked it before ValidateModes */
    if ( (pScrn->depth == 24) && 
         ((pScrn->bitsPerPixel/8) * pScrn->virtualX > 4095) ) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Virtual width to large for ViRGE\n");
      S3VFreeRec(pScrn);
      return FALSE;
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
	mod = "cfb32";
	reqSym = "cfb32ScreenInit";
	break;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	S3VFreeRec(pScrn);
	return FALSE;
    }	       
    
    xf86LoaderReqSymbols(reqSym, NULL);
	     
    /* Load XAA if needed */
    if (!ps3v->NoAccel /*|| pMga->HWCursor*/ ) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    S3VFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    return TRUE;
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */


/* Mandatory */
static Bool
S3VEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    /*vgaHWPtr hwp = VGAHWPTR(pScrn);*/

    PVERB5("	S3VEnterVT\n");
    /*vgaHWUnlockMMIO(hwp);*/
					/* Add me to resource management */
    xf86AddControlledResource(pScrn, MEM_IO);
    					/* Enable card from server side  */
    xf86EnableAccess(&pScrn->Access);
    				/* Enable MMIO and map memory */
    S3VMapMem(pScrn);
    S3VSave(pScrn);
    return S3VModeInit(pScrn, pScrn->currentMode);
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 *
 */

/* Mandatory */
static void
S3VLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    S3VPtr ps3v = S3VPTR(pScrn);
    vgaRegPtr vgaSavePtr = &hwp->SavedReg;
    S3VRegPtr S3VSavePtr = &ps3v->SavedReg;

    PVERB5("	S3VLeaveVT\n");
    					/* Like S3VRestore, but uses passed */
					/* mode registers.		    */
    S3VWriteMode(pScrn, vgaSavePtr, S3VSavePtr);
    					/* Restore standard register access */
					/* and unmap memory.		    */
    S3VUnmapMem(pScrn);
    xf86DelControlledResource(&pScrn->Access, FALSE);
    /*vgaHWLockMMIO(hwp);*/
}


/* 
 * This function performs the inverse of the restore function: It saves all
 * the standard and extended registers that we are going to modify to set
 * up a video mode. Again, we also save the STREAMS context if it is needed.
 *
 * prototype
 *   void ChipSave(ScrnInfoPtr pScrn)
 *
 */

static void
S3VSave (ScrnInfoPtr pScrn)
{
  unsigned char cr3a, /*cr53,*/ cr66;
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  vgaRegPtr vgaSavePtr = &hwp->SavedReg;
  S3VPtr ps3v = S3VPTR(pScrn);
  S3VRegPtr save = &ps3v->SavedReg;
  void *s3vMmioMem = ps3v->MapBase;
  int vgaCRIndex, vgaCRReg, vgaIOBase;
  /*vgaIOBase = VGAHW_GET_IOBASE();*/
  vgaIOBase = hwp->IOBase;
  vgaCRIndex = vgaIOBase + 4;
  vgaCRReg = vgaIOBase + 5;

/*vgaS3VPtr save;  pre-4.0 version saved to passed struct */

    PVERB5("	S3VSave\n");

   /*
    * This function will handle creating the data structure and filling
    * in the generic VGA portion.
    */
	 
    /* Hmmm, memory settings might not be needed for 4.0... */
    /* If not, remove resetting at end also. */
   #if 1
   OUTREG8(vgaCRIndex, 0x66);
   cr66 = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr66 | 0x80);
   OUTREG8(vgaCRIndex, 0x3a);
   cr3a = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr3a | 0x80);
   #endif

   /* save = (vgaS3VPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaS3VRec));*/
   /* Save VGA info, False indicates no fontinfo save */
   /* VGA_SR_MODE saves mode info only, no fonts, no colormap */
					/* Save all for primary, anything */
					/* for secondary cards?, do MODE */
					/* for the moment. */
   if (xf86IsPrimaryPci(ps3v->PciInfo))
   	vgaHWSave(pScrn, vgaSavePtr, VGA_SR_ALL);
   else
   	vgaHWSave(pScrn, vgaSavePtr, VGA_SR_MODE);
   
   #if 1
   OUTREG8(vgaCRIndex, 0x66);
   OUTREG8(vgaCRReg, cr66);
   OUTREG8(vgaCRIndex, 0x3a);             
   OUTREG8(vgaCRReg, cr3a);

   /* First unlock extended sequencer regs */
   OUTREG8(0x3c4, 0x08);
   OUTREG8(0x3c5, 0x06); 
   #endif

   /* Now we save all the s3 extended regs we need */
   OUTREG8(vgaCRIndex, 0x31);             
   save->CR31 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x34);             
   save->CR34 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x36);             
   save->CR36 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x3a);             
   save->CR3A = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x42);             
   save->CR42 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x51);             
   save->CR51 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x53);             
   save->CR53 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x54);             
   save->CR54 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x58);             
   save->CR58 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x63);
   save->CR63 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x66);             
   save->CR66 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x67);             
   save->CR67 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x68);             
   save->CR68 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x69);
   save->CR69 = INREG8(vgaCRReg);

   OUTREG8(vgaCRIndex, 0x33);             
   save->CR33 = INREG8(vgaCRReg);
   if (ps3v->Chipset == S3_ViRGE_DXGX || ps3v->Chipset == S3_ViRGE_GX2) {
      OUTREG8(vgaCRIndex, 0x86);
      save->CR86 = INREG8(vgaCRReg);
      OUTREG8(vgaCRIndex, 0x90);
      save->CR90 = INREG8(vgaCRReg);
   }

   /* Extended mode timings regs */

   OUTREG8(vgaCRIndex, 0x3b);             
   save->CR3B = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x3c);             
   save->CR3C = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x43);             
   save->CR43 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x5d);             
   save->CR5D = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x5e);
   save->CR5E = INREG8(vgaCRReg);  
   OUTREG8(vgaCRIndex, 0x65);             
   save->CR65 = INREG8(vgaCRReg);
   OUTREG8(vgaCRIndex, 0x6d);
   save->CR6D = INREG8(vgaCRReg);


   /* Save sequencer extended regs for DCLK PLL programming */

   OUTREG8(0x3c4, 0x10);
   save->SR10 = INREG8(0x3c5);
   OUTREG8(0x3c4, 0x11);
   save->SR11 = INREG8(0x3c5);

   OUTREG8(0x3c4, 0x12);
   save->SR12 = INREG8(0x3c5);
   OUTREG8(0x3c4, 0x13);
   save->SR13 = INREG8(0x3c5);

   OUTREG8(0x3c4, 0x15);
   save->SR15 = INREG8(0x3c5);
   OUTREG8(0x3c4, 0x18);
   save->SR18 = INREG8(0x3c5);


   /* And if streams is to be used, save that as well */
						      
 #if 0	/* should be using mmio already for 4.0 server */
   /*I hope I solved it {KF}*/
      OUTREG8(vgaCRIndex, 0x53);
   cr53 = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr53 | 0x08);  /* Enable NEWMMIO to save MIU context */
 #endif
 #if 1
   OUTREG8(vgaCRIndex, 0x66);
   cr66 = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr66 | 0x80);
   OUTREG8(vgaCRIndex, 0x3a);
   cr3a = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr3a | 0x80);
 #endif

   if(ps3v->NeedSTREAMS) {
      S3VSaveSTREAMS(pScrn, save->STREAMS);
      }

   /* Now save Memory Interface Unit registers, enable MMIO for this */
   /* 4.0 note: mmio should be enabled for this entire function */
   save->MMPR0 = ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control;
   save->MMPR1 = ((mmtr)s3vMmioMem)->memport_regs.regs.miu_control;
   save->MMPR2 = ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout;
   save->MMPR3 = ((mmtr)s3vMmioMem)->memport_regs.regs.misc_timeout;

   if (xf86GetVerbosity() > 1) {
      /* Debug */
      ErrorF("MMPR regs: %08x %08x %08x %08x\n",
         ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control,
         ((mmtr)s3vMmioMem)->memport_regs.regs.miu_control,
         ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout,
         ((mmtr)s3vMmioMem)->memport_regs.regs.misc_timeout );

      PVERB5("\n\nViRGE driver: saved current video mode. Register dump:\n\n");
   }
 #if 0
   OUTREG8(vgaCRIndex, 0x53);
   OUTREG8(vgaCRReg, cr53);   /* Restore CR53 to original for MMIO */
 #endif
 #if 1
   OUTREG8(vgaCRIndex, 0x3a);
   OUTREG8(vgaCRReg, cr3a);
   OUTREG8(vgaCRIndex, 0x66);
   OUTREG8(vgaCRReg, cr66);
 #endif

   				/* Dup the VGA & S3V state to the */
				/* new mode state, but only first time. */
   if( !ps3v->ModeStructInit ) {
     memcpy( &hwp->ModeReg, vgaSavePtr, sizeof(vgaRegRec) );
     memcpy( &ps3v->ModeReg, save, sizeof(S3VRegRec) );
     ps3v->ModeStructInit = TRUE;
   }
   			 
   if (xf86GetVerbosity() > 1) S3VPrintRegs(pScrn);

   return;
}


/* This function saves the STREAMS registers to our private structure */

static void
S3VSaveSTREAMS(ScrnInfoPtr pScrn, unsigned int *streams)
{
  S3VPtr ps3v = S3VPTR(pScrn);
  void *s3vMmioMem = ps3v->MapBase;

   streams[0] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_cntl;
   streams[1] = ((mmtr)s3vMmioMem)->streams_regs.regs.col_chroma_key_cntl;
   streams[2] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_cntl;
   streams[3] = ((mmtr)s3vMmioMem)->streams_regs.regs.chroma_key_upper_bound;
   streams[4] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stretch;
   streams[5] = ((mmtr)s3vMmioMem)->streams_regs.regs.blend_cntl;
   streams[6] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0;
   streams[7] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr1;
   streams[8] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_stride;
   streams[9] = ((mmtr)s3vMmioMem)->streams_regs.regs.double_buffer;
   streams[10] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr0;
   streams[11] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr1;
   streams[12] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stride;
   streams[13] = ((mmtr)s3vMmioMem)->streams_regs.regs.opaq_overlay_cntl;
   streams[14] = ((mmtr)s3vMmioMem)->streams_regs.regs.k1;
   streams[15] = ((mmtr)s3vMmioMem)->streams_regs.regs.k2;
   streams[16] = ((mmtr)s3vMmioMem)->streams_regs.regs.dda_vert;
   streams[17] = ((mmtr)s3vMmioMem)->streams_regs.regs.streams_fifo;
   streams[18] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_start_coord;
   streams[19] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_window_size;
   streams[20] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_start_coord;
   streams[21] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_window_size;

}
       

/* 
 * This function is used to restore a video mode. It writes out all  
 * of the standart VGA and extended S3 registers needed to setup a 
 * video mode.
 *
 * Note that our life is made more difficult because of the STREAMS
 * processor which must be used for 24bpp. We need to disable STREAMS
 * before we switch video modes, or we risk locking up the machine. 
 * We also have to follow a certain order when reenabling it. 
 */

static void
S3VWriteMode (ScrnInfoPtr pScrn, vgaRegPtr vgaSavePtr, S3VRegPtr restore)
/* vgaS3VPtr restore; */
{
  unsigned char tmp, cr3a, /*cr53,*/ cr66, cr67;
  
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  S3VPtr ps3v = S3VPTR(pScrn);
  void *s3vMmioMem = ps3v->MapBase;
  int vgaCRIndex, vgaCRReg, vgaIOBase;
  vgaIOBase = hwp->IOBase;
  vgaCRIndex = vgaIOBase + 4;
  vgaCRReg = vgaIOBase + 5;
  
    PVERB5("	S3VWriteMode\n");

  vgaHWProtect(pScrn, TRUE);
   
   /* Are we going to reenable STREAMS in this new mode? */
   ps3v->STREAMSRunning = restore->CR67 & 0x0c; 

   /* First reset GE to make sure nothing is going on */
   if(ps3v->Chipset == S3_ViRGE_VX) {
      OUTREG8(vgaCRIndex, 0x63);
      if(INREG8(vgaCRReg) & 0x01) S3VGEReset(pScrn);
      }
   else {
      OUTREG8(vgaCRIndex, 0x66);
      if(INREG8(vgaCRReg) & 0x01) S3VGEReset(pScrn);
      }

   /* As per databook, always disable STREAMS before changing modes */
   OUTREG8(vgaCRIndex, 0x67);
   cr67 = INREG8(vgaCRReg);
   if ((cr67 & 0x0c) == 0x0c) {
      S3VDisableSTREAMS(pScrn);     /* If STREAMS was running, disable it */
      }

   /* Restore S3 extended regs */
   OUTREG8(vgaCRIndex, 0x63);             
   OUTREG8(vgaCRReg, restore->CR63);
   OUTREG8(vgaCRIndex, 0x66);             
   OUTREG8(vgaCRReg, restore->CR66);
   OUTREG8(vgaCRIndex, 0x3a);             
   OUTREG8(vgaCRReg, restore->CR3A);
   OUTREG8(vgaCRIndex, 0x31);    
   OUTREG8(vgaCRReg, restore->CR31);
   OUTREG8(vgaCRIndex, 0x58);             
   OUTREG8(vgaCRReg, restore->CR58);

   /* Extended mode timings registers */  
   OUTREG8(vgaCRIndex, 0x53);             
   OUTREG8(vgaCRReg, restore->CR53); 
   OUTREG8(vgaCRIndex, 0x5d);     
   OUTREG8(vgaCRReg, restore->CR5D);
   OUTREG8(vgaCRIndex, 0x5e);             
   OUTREG8(vgaCRReg, restore->CR5E);
   OUTREG8(vgaCRIndex, 0x3b);             
   OUTREG8(vgaCRReg, restore->CR3B);
   OUTREG8(vgaCRIndex, 0x3c);             
   OUTREG8(vgaCRReg, restore->CR3C);
   OUTREG8(vgaCRIndex, 0x43);             
   OUTREG8(vgaCRReg, restore->CR43);
   OUTREG8(vgaCRIndex, 0x65);             
   OUTREG8(vgaCRReg, restore->CR65);
   OUTREG8(vgaCRIndex, 0x6d);
   OUTREG8(vgaCRReg, restore->CR6D);


   /* Restore the desired video mode with CR67 */
        
   OUTREG8(vgaCRIndex, 0x67);             
   cr67 = INREG8(vgaCRReg) & 0xf; /* Possible hardware bug on VX? */
   OUTREG8(vgaCRReg, 0x50 | cr67); 
   usleep(10000);
   OUTREG8(vgaCRIndex, 0x67);             
   OUTREG8(vgaCRReg, restore->CR67 & ~0x0c); /* Don't enable STREAMS yet */

   /* Other mode timing and extended regs */
   OUTREG8(vgaCRIndex, 0x34);             
   OUTREG8(vgaCRReg, restore->CR34);
   OUTREG8(vgaCRIndex, 0x42);             
   OUTREG8(vgaCRReg, restore->CR42);
   OUTREG8(vgaCRIndex, 0x51);             
   OUTREG8(vgaCRReg, restore->CR51);
   OUTREG8(vgaCRIndex, 0x54);             
   OUTREG8(vgaCRReg, restore->CR54);
   
   /* Memory timings */
   OUTREG8(vgaCRIndex, 0x36);             
   OUTREG8(vgaCRReg, restore->CR36);
   OUTREG8(vgaCRIndex, 0x68);             
   OUTREG8(vgaCRReg, restore->CR68);
   OUTREG8(vgaCRIndex, 0x69);
   OUTREG8(vgaCRReg, restore->CR69);

   OUTREG8(vgaCRIndex, 0x33);
   OUTREG8(vgaCRReg, restore->CR33);
   if (ps3v->Chipset == S3_ViRGE_DXGX || ps3v->Chipset == S3_ViRGE_GX2) {
      OUTREG8(vgaCRIndex, 0x86);
      OUTREG8(vgaCRReg, restore->CR86);
      OUTREG8(vgaCRIndex, 0x90);
      OUTREG8(vgaCRReg, restore->CR90);
   }

 #if 1		/* not for MMIO ? */
   /* Unlock extended sequencer regs */
   OUTREG8(0x3c4, 0x08);
   OUTREG8(0x3c5, 0x06); 
 #endif


   /* Restore extended sequencer regs for MCLK. SR10 == 255 indicates that 
    * we should leave the default SR10 and SR11 values there.
    */

   if (restore->SR10 != 255) {   
       OUTREG8(0x3c4, 0x10);
       OUTREG8(0x3c5, restore->SR10);
       OUTREG8(0x3c4, 0x11);
       OUTREG8(0x3c5, restore->SR11);
       }

   /* Restore extended sequencer regs for DCLK */
   OUTREG8(0x3c4, 0x12);
   OUTREG8(0x3c5, restore->SR12);
   OUTREG8(0x3c4, 0x13);
   OUTREG8(0x3c5, restore->SR13);

   OUTREG8(0x3c4, 0x18);
   OUTREG8(0x3c5, restore->SR18); 

   /* Load new m,n PLL values for DCLK & MCLK */
   OUTREG8(0x3c4, 0x15);
   tmp = INREG8(0x3c5) & ~0x21;

   OUTREG8(0x3c5, tmp | 0x03);
   OUTREG8(0x3c5, tmp | 0x23);
   OUTREG8(0x3c5, tmp | 0x03);
   OUTREG8(0x3c5, restore->SR15);

   /* Now write out CR67 in full, possibly starting STREAMS */

   VerticalRetraceWait();
   OUTREG8(vgaCRIndex, 0x67);    
   OUTREG8(vgaCRReg, 0x50);   /* For possible bug on VX?! */          
   usleep(10000);
   OUTREG8(vgaCRIndex, 0x67);
   OUTREG8(vgaCRReg, restore->CR67); 


   /* And finally, we init the STREAMS processor if we have CR67 indicate 24bpp
    * We also restore FIFO and TIMEOUT memory controller registers.
    */
	 
 #if 0		/* not for MMIO version */
      OUTREG8(vgaCRIndex, 0x53);
   cr53 = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr53 | 0x08);  /* Enable NEWMMIO temporarily */
 #endif
 #if 1
   OUTREG8(vgaCRIndex, 0x66);
   cr66 = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr66 | 0x80);
   OUTREG8(vgaCRIndex, 0x3a);
   cr3a = INREG8(vgaCRReg);
   OUTREG8(vgaCRReg, cr3a | 0x80);
 #endif

   if (ps3v->NeedSTREAMS) {
      if(ps3v->STREAMSRunning) S3VRestoreSTREAMS(pScrn, restore->STREAMS);
      }

   /* Now, before we continue, check if this mode has the graphic engine ON 
    * If yes, then we reset it. 
    * This fixes some problems with corruption at 24bpp with STREAMS
    * Also restore the MIU registers. 
    */

#ifndef MetroLink
   if(ps3v->Chipset == S3_ViRGE_VX) {
      if(restore->CR63 & 0x01) S3VGEReset(pScrn);
      }
   else {
      if(restore->CR66 & 0x01) S3VGEReset(pScrn);
      }
#else
   S3VGEReset(pScrn);
#endif

   VerticalRetraceWait();
   ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control = restore->MMPR0;
   WaitIdle();                  /* Don't ask... */
   ((mmtr)s3vMmioMem)->memport_regs.regs.miu_control = restore->MMPR1;
   WaitIdle();                  
   ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout = restore->MMPR2;
   WaitIdle();
   ((mmtr)s3vMmioMem)->memport_regs.regs.misc_timeout = restore->MMPR3;

 #if 0
   OUTREG8(vgaCRIndex, 0x53);
   OUTREG8(vgaCRReg, cr53);   /* Restore CR53 to original for MMIO */
 #endif
 #if 0
   OUTREG8(vgaCRIndex, 0x66);             
   OUTREG8(vgaCRReg, cr66);
   OUTREG8(vgaCRIndex, 0x3a);             
   OUTREG8(vgaCRReg, cr3a);
 #endif

   OUTREG8(0x3c4, 0x08);
   OUTREG8(0x3c5, 0x06); 
   OUTREG8(0x3c4, 0x12);
   OUTREG8(0x3c5, restore->SR12);
   OUTREG8(0x3c4, 0x13);
   OUTREG8(0x3c5, restore->SR13);
   OUTREG8(0x3c4, 0x15);
   OUTREG8(0x3c5, restore->SR15); 

   /* Restore the standard VGA registers */
   /* False indicates no fontinfo restore. */
   /* VGA_SR_MODE restores mode info only, no font, no colormap */
   					/* Do all for primary video */
   if (xf86IsPrimaryPci(ps3v->PciInfo))
     vgaHWRestore(pScrn, vgaSavePtr, VGA_SR_ALL);
   					/* Mode only for non-primary? */
   else
     vgaHWRestore(pScrn, vgaSavePtr, VGA_SR_MODE);

 #if 1		/* moved from before vgaHWRestore, to prevent segfault? */
   OUTREG8(vgaCRIndex, 0x66);             
   OUTREG8(vgaCRReg, cr66);
   OUTREG8(vgaCRIndex, 0x3a);             
   OUTREG8(vgaCRReg, cr3a);
 #endif

   if (xf86GetVerbosity() > 1) {
      ErrorF("\n\nViRGE driver: done restoring mode, dumping CR registers:\n\n");
      S3VPrintRegs(pScrn);
   }
   
   vgaHWProtect(pScrn, FALSE);
   return;

}

			    
/* This function restores the saved STREAMS registers */

static void
S3VRestoreSTREAMS(ScrnInfoPtr pScrn, unsigned int *streams)
{
  S3VPtr ps3v = S3VPTR(pScrn);
  void *s3vMmioMem = ps3v->MapBase;


/* For now, set most regs to their default values for 24bpp 
 * Restore only those that are needed for width/height/stride
 * Otherwise, we seem to get lockups because some registers 
 * when saved have some reserved bits set.
 */

   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_cntl = 
         streams[0] & 0x77000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.col_chroma_key_cntl = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_cntl = 0x03000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.chroma_key_upper_bound = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stretch = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.blend_cntl = 0x01000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr1 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_stride = 
         streams[8] & 0x0fff;
   ((mmtr)s3vMmioMem)->streams_regs.regs.double_buffer = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr0 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr1 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stride = 0x01;
   ((mmtr)s3vMmioMem)->streams_regs.regs.opaq_overlay_cntl = 0x40000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.k1 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.k2 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.dda_vert = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_start_coord = 0x00010001;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_window_size = 
         streams[19] & 0x07ff07ff;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_start_coord = 0x07ff07ff;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_window_size = 0x00010001;


}




/* And this function disables the STREAMS processor as per databook.
 * This is usefull before we do a mode change 
 */

static void
S3VDisableSTREAMS(ScrnInfoPtr pScrn)
{
unsigned char tmp;
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  S3VPtr ps3v = S3VPTR(pScrn);
  void *s3vMmioMem = ps3v->MapBase;
  int vgaCRIndex, vgaCRReg, vgaIOBase;
  vgaIOBase = hwp->IOBase;
  vgaCRIndex = vgaIOBase + 4;
  vgaCRReg = vgaIOBase + 5;

   VerticalRetraceWait();
   ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control = 0xC000;
   OUTREG8(vgaCRIndex, 0x67);
   tmp = INREG8(vgaCRReg);
                         /* Disable STREAMS processor */
   OUTREG8( vgaCRReg, tmp & ~0x0C );

   return;
}



/* MapMem - contains half of pre-4.0 EnterLeave function */
/* The EnterLeave function which en/dis access to IO ports and ext. regs */

static Bool
S3VMapMem(ScrnInfoPtr pScrn)
{    
  S3VPtr ps3v;
  vgaHWPtr hwp;

    PVERB5("	S3VMapMem\n");
    
  ps3v = S3VPTR(pScrn);
   
    					/* Map the ViRGE register space */
					/* Starts with Image Transfer area */
					/* so that we can use registers map */
					/* structure - see newmmio.h */
					/* around 0x10000 from MemBase */
  ps3v->MapBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, ps3v->PciTag,
			(pointer) (ps3v->PciInfo->memBase[0] + S3_NEWMMIO_REGBASE),
			S3_NEWMMIO_REGSIZE );
					/* IOBase starts at PCI registers */
  ps3v->IOBase = ps3v->MapBase + S3V_MMIO_REGSIZE;
  
  if( !ps3v->MapBase ) {
    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
	"Internal error: could not map registers.\n");
    return FALSE;
  }
					/* Map the framebuffer */
  ps3v->FBBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER, 
			ps3v->PciTag,
			(pointer) ps3v->PciInfo->memBase[0],
			ps3v->videoRambytes );
  if( !ps3v->FBBase ) {
    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
	"Internal error: could not map framebuffer.\n");
    return FALSE;
  }
  			       		/* Initially the visual display start */
					/* is the same as the mapped start. */
  ps3v->FBStart = ps3v->FBBase;
  
  S3VEnableMmio( pScrn);
   					/* Assign hwp->MemBase & IOBase here */
  hwp = VGAHWPTR(pScrn);
					/* Sets MMIOBase and Offset, assigns */
					/* functions. Offset from map area   */
					/* to VGA reg area is 0x8000. */
  vgaHWSetMmioFuncs( hwp, ps3v->MapBase, S3V_MMIO_REGSIZE );
  					/* assigns hwp->IOBase to 3D0 or 3B0 */
					/* needs hwp->MMIOBase to work */
  vgaHWGetIOBase(hwp);
    					/* Map the VGA memory when the */
					/* primary video */
  if (xf86IsPrimaryPci(ps3v->PciInfo)) {
    hwp->MapSize = 0x10000;
    if (!vgaHWMapMem(pScrn))
      return FALSE;
    ps3v->PrimaryVidMapped = TRUE;
  }
  
  return TRUE;
}



/* UnMapMem - contains half of pre-4.0 EnterLeave function */
/* The EnterLeave function which en/dis access to IO ports and ext. regs */

static void 
S3VUnmapMem(ScrnInfoPtr pScrn)
{
  S3VPtr ps3v;

  ps3v = S3VPTR(pScrn);
					/* Unmap VGA mem if mapped. */
  if( ps3v->PrimaryVidMapped ) {
    vgaHWUnmapMem( pScrn );
    ps3v->PrimaryVidMapped = FALSE;
  }

  S3VDisableMmio(pScrn);
  xf86UnMapVidMem(pScrn->scrnIndex, (pointer)ps3v->MapBase, S3_NEWMMIO_REGSIZE);
  xf86UnMapVidMem(pScrn->scrnIndex, (pointer)ps3v->FBBase, ps3v->videoRambytes);

  return;
}




/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
S3VScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
  ScrnInfoPtr pScrn;
  S3VPtr ps3v;
  int ret;
  					/* First get the ScrnInfoRec */
  pScrn = xf86Screens[pScreen->myNum];
  					/* Get S3V rec */
  ps3v = S3VPTR(pScrn);
  					/* Make sure we have card access */
  xf86EnableAccess(&pScrn->Access);
   					/* Map MMIO regs and framebuffer */
  if( !S3VMapMem(pScrn) )
    return FALSE;
    					/* Save the chip/graphics state */
  S3VSave(pScrn);
				 	/* Blank the screen during init */
  vgaHWBlankScreen(pScrn, TRUE );  
    					/* Initialise the first mode */
  if (!S3VModeInit(pScrn, pScrn->currentMode))
    return FALSE;
   					/* Adjust the viewport */
  S3VAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    
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
	ret = cfbScreenInit(pScreen, ps3v->FBStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, ps3v->FBStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, ps3v->FBStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, ps3v->FBStart,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in S3VScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
  } /*switch*/
  if (!ret)
    return FALSE;
      
  xf86SetBlackWhitePixels(pScreen);
	 
  if (pScrn->pixmapBPP > 8) {
    	VisualPtr visual;
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
  	      				/* Initialize acceleration layer */
  if (!ps3v->NoAccel) {
    if(pScrn->bitsPerPixel == 32) {
      if (!S3VAccelInit32(pScreen))
        return FALSE;
    } else 
      if (!S3VAccelInit(pScreen))
        return FALSE;
  }
	
  miInitializeBackingStore(pScreen);

    					/* Initialise cursor functions */
  miDCInitialize(pScreen, xf86GetPointerScreenFuncs());
   
    					/* Initialise default colourmap */
  if (!miCreateDefColormap(pScreen))
    return FALSE;
  					/* Initialize colormap layer.   */
					/* Must follow initialization   */
					/* of the default colormap. 	*/
					/* And SetGamma call, else it 	*/
					/* will load palette with solid */
					/* white. */
  if(!xf86HandleColormaps(pScreen, 256, 6, S3VLoadPalette, 
			CMAP_RELOAD_ON_MODE_SWITCH ))
	return FALSE;
				    	/* All the ugly stuff is done, 	*/
					/* so re-enable the screen. 	*/
  vgaHWBlankScreen(pScrn, FALSE );  
    
  pScreen->SaveScreen = S3VSaveScreen;

    					/* Wrap the current CloseScreen function */
  ps3v->CloseScreen = pScreen->CloseScreen;
  pScreen->CloseScreen = S3VCloseScreen;

    /* Report any unused options (only for the first generation) */
  if (serverGeneration == 1) {
    xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
  }
    					/* Done */
  return TRUE;
}





/* Checks if a mode is suitable for the selected chipset. */

static ModeStatus
S3VValidMode(int index, DisplayModePtr mode, Bool verbose, int flags)
{

  return MODE_OK;
}



static Bool
S3VModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  S3VPtr ps3v = S3VPTR(pScrn);
  int width, dclk;
  int i, j;
  unsigned char tmp = 0;
  
  		      		/* Store values to current mode register structs */
  S3VRegPtr new = &ps3v->ModeReg;
  vgaRegPtr vganew = &hwp->ModeReg;
  int vgaCRIndex, vgaCRReg, vgaIOBase;

  vgaIOBase = hwp->IOBase;
  vgaCRIndex = vgaIOBase + 4;
  vgaCRReg = vgaIOBase + 5;

    PVERB5("	S3VModeInit\n");   

   /* First we adjust the horizontal timings if needed */

   if(ps3v->HorizScaleFactor != 1)
      if (!mode->CrtcHAdjusted) {
             mode->CrtcHDisplay *= ps3v->HorizScaleFactor;
             mode->CrtcHSyncStart *= ps3v->HorizScaleFactor;
             mode->CrtcHSyncEnd *= ps3v->HorizScaleFactor;
             mode->CrtcHTotal *= ps3v->HorizScaleFactor;
             mode->CrtcHSkew *= ps3v->HorizScaleFactor;
             mode->CrtcHAdjusted = TRUE;
             }

   if(!vgaHWInit (pScrn, mode))
      return FALSE;
      
   /* Now we fill in the rest of the stuff we need for the virge */
   /* Start with MMIO, linear addr. regs */

   OUTREG8(vgaCRIndex, 0x3a);
   tmp = INREG8(vgaCRReg);
   if(ps3v->pci_burst_on)
      new->CR3A = (tmp & 0x7f) | 0x15; /* ENH 256, PCI burst */
   else 
      new->CR3A = tmp | 0x95;      /* ENH 256, no PCI burst! */
	   
   new->CR53 = 0x08;     /* Enables MMIO */
   new->CR31 = 0x8c;     /* Dis. 64k window, en. ENH maps */    

   /* Enables S3D graphic engine and PCI disconnects */
   if(ps3v->Chipset == S3_ViRGE_VX){
      new->CR66 = 0x90;  
      new->CR63 = 0x09;
      }
   else {
      new->CR66 = 0x89; 
      new->CR63 = 0;
      }    

  /* Now set linear addr. registers */
  /* LAW size: we have 2 cases, 2MB, 4MB or >= 4MB for VX */
   OUTREG8(vgaCRIndex, 0x58);
   new->CR58 = INREG8(vgaCRReg) & 0x80;
   if(pScrn->videoRam == 2048){   
      new->CR58 |= 0x02 | 0x10; 
      }
   else {
      new->CR58 |= 0x03 | 0x10; /* 4MB window on virge, 8MB on VX */
      } 
   if(ps3v->Chipset == S3_ViRGE_VX)
      new->CR58 |= 0x40;
   if (ps3v->early_ras_precharge)
      new->CR58 |= 0x80;
   if (ps3v->late_ras_precharge)
      new->CR58 &= 0x7f;
      
  /* ** On PCI bus, no need to reprogram the linear window base address */
  
  /* Now do clock PLL programming. Use the s3gendac function to get m,n */
  /* Also determine if we need doubling etc. */

   dclk = mode->Clock;
   new->CR67 = 0x00;             /* Defaults */
   new->SR15 = 0x03 | 0x80; 
   new->SR18 = 0x00;
   new->CR43 = 0x00;
   /*new->CR65 = 0x00;*/
   				/* Enable MMIO to RAMDAC registers */
   new->CR65 = 0x04;
   /*new->CR65 = 0x02;  3.9Nm */
   new->CR54 = 0x00;
   
    /*cep*/  
    xf86ErrorFVerb(VERBLEV, "	S3VModeInit dclk=%i \n", 
   	dclk
	);
   
   /* Memory controller registers. Optimize for better graphics engine 
    * performance. These settings are adjusted/overridden below for other bpp/
    * XConfig options.The idea here is to give a longer number of contiguous
    * MCLK's to both refresh and the graphics engine, to diminish the 
    * relative penalty of 3 or 4 mclk's needed to setup memory transfers. 
    */
   new->MMPR0 = 0x010400; /* defaults */
   new->MMPR1 = 0x00;   
   new->MMPR2 = 0x0808;  
   new->MMPR3 = 0x08080810; 
			 
   if( ps3v->fifo_aggressive || ps3v->fifo_moderate || 
       ps3v->fifo_conservative ) {
         new->MMPR1 = 0x0200;   /* Low P. stream waits before filling */
         new->MMPR2 = 0x1808;   /* Let the FIFO refill itself */
         new->MMPR3 = 0x08081810; /* And let the GE hold the bus for a while */
      }

  #if 0
   /* And setup here the new value for MCLK. We use the XConfig 
    * option "set_mclk", whose value gets stored in vga256InfoRec.s3MClk.
    * I'm not sure what the maximum "permitted" value should be, probably
    * 100 MHz is more than enough for now.  
    */

   if(ps3v->MCLK> 0) {
       commonCalcClock(ps3v->MCLK, 1, 1, 31, 0, 3,
	   135000, 270000, &new->SR11, &new->SR10);
       }
   else {
       new->SR10 = 255; /* This is a reserved value, so we use as flag */
       new->SR11 = 255; 
       }
  #endif

   					/* most modes don't need STREAMS */
					/* processor, preset FALSE */
   ps3v->NeedSTREAMS = FALSE;
   
   if(ps3v->Chipset == S3_ViRGE_VX){
       if (pScrn->bitsPerPixel == 8) {
          if (dclk <= 110000) new->CR67 = 0x00; /* 8bpp, 135MHz */
          else new->CR67 = 0x10;                /* 8bpp, 220MHz */
          }
       else if ((pScrn->bitsPerPixel == 16) && (pScrn->weight.green == 5)) {
          if (dclk <= 110000) new->CR67 = 0x20; /* 15bpp, 135MHz */
          else new->CR67 = 0x30;                /* 15bpp, 220MHz */
          } 
       else if (pScrn->bitsPerPixel == 16) {
          if (dclk <= 110000) new->CR67 = 0x40; /* 16bpp, 135MHz */
          else new->CR67 = 0x50;                /* 16bpp, 220MHz */
          }
       else if ((pScrn->bitsPerPixel == 24) || (pScrn->bitsPerPixel == 32)) {
          new->CR67 = 0xd0 | 0x0c;              /* 24bpp, 135MHz, STREAMS */
	  					/* Flag STREAMS proc. required */
          ps3v->NeedSTREAMS = TRUE;
          S3VInitSTREAMS(pScrn, new->STREAMS, mode);
          new->MMPR0 = 0xc098;            /* Adjust FIFO slots */
          }
       commonCalcClock(dclk, 1, 1, 31, 0, 4, 
	   220000, 440000, &new->SR13, &new->SR12);

      }
   else {           /* Is this correct for DX/GX as well? */
      if (pScrn->bitsPerPixel == 8) {
         if(dclk > 80000) {                     /* We need pixmux */
            new->CR67 = 0x10;
            new->SR15 |= 0x10;                   /* Set DCLK/2 bit */
            new->SR18 = 0x80;                   /* Enable pixmux */
            }
         }
      else if ((pScrn->bitsPerPixel == 16) && (pScrn->weight.green == 5)) {
         new->CR67 = 0x30;                       /* 15bpp */
         }
      else if (pScrn->bitsPerPixel == 16) {
         new->CR67 = 0x50;
         }
      else if (pScrn->bitsPerPixel == 24) { 
         new->CR67 = 0xd0 | 0x0c;
	  					/* Flag STREAMS proc. required */
         ps3v->NeedSTREAMS = TRUE;
         S3VInitSTREAMS(pScrn, new->STREAMS, mode);
         new->MMPR0 = 0xc000;            /* Adjust FIFO slots */
         }
      else if (pScrn->bitsPerPixel == 32) { 
         new->CR67 = 0xd0 | 0x0c;
	  					/* Flag STREAMS proc. required */
         ps3v->NeedSTREAMS = TRUE;
         S3VInitSTREAMS(pScrn, new->STREAMS, mode);
         new->MMPR0 = 0x10000;            /* Still more FIFO slots */
         }
      commonCalcClock(dclk, 1, 1, 31, 0, 3, 
	135000, 270000, &new->SR13, &new->SR12);
      }

   /* Now adjust the value of the FIFO based upon options specified */
   if( ps3v->fifo_moderate ) {
      if(pScrn->bitsPerPixel < 24)
         new->MMPR0 -= 0x8000;
      else 
         new->MMPR0 -= 0x4000;
      }
   else if( ps3v->fifo_aggressive ) {
      if(pScrn->bitsPerPixel < 24)
         new->MMPR0 -= 0xc000;
      else 
         new->MMPR0 -= 0x6000;
      }
	     
   /* If we have an interlace mode, set the interlace bit. Note that mode
    * vertical timings are already adjusted by the standard VGA code 
    */
   if(mode->Flags & V_INTERLACE) {
        new->CR42 = 0x20; /* Set interlace mode */
        }
   else {
        new->CR42 = 0x00;
        }

   /* Set display fifo */
   new->CR34 = 0x10;  

   /* Now we adjust registers for extended mode timings */
   /* This is taken without change from the accel/s3_virge code */

   i = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8) |
       ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7) |
       ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6) |
       ((mode->CrtcHSyncStart & 0x800) >> 7);

   if ((mode->CrtcHSyncEnd >> 3) - (mode->CrtcHSyncStart >> 3) > 64)
      i |= 0x08;   /* add another 64 DCLKs to blank pulse width */

   if ((mode->CrtcHSyncEnd >> 3) - (mode->CrtcHSyncStart >> 3) > 32)
      i |= 0x20;   /* add another 32 DCLKs to hsync pulse width */

   j = (  vganew->CRTC[0] + ((i&0x01)<<8)
        + vganew->CRTC[4] + ((i&0x10)<<4) + 1) / 2;

   if (j-(vganew->CRTC[4] + ((i&0x10)<<4)) < 4)
      if (vganew->CRTC[4] + ((i&0x10)<<4) + 4 <= vganew->CRTC[0]+ ((i&0x01)<<8))
         j = vganew->CRTC[4] + ((i&0x10)<<4) + 4;
      else
         j = vganew->CRTC[0]+ ((i&0x01)<<8) + 1;

   new->CR3B = j & 0xFF;
   i |= (j & 0x100) >> 2;
   new->CR3C = (vganew->CRTC[0] + ((i&0x01)<<8))/2;
   new->CR5D = i;

   new->CR5E = (((mode->CrtcVTotal - 2) & 0x400) >> 10)  |
               (((mode->CrtcVDisplay - 1) & 0x400) >> 9) |
               (((mode->CrtcVSyncStart) & 0x400) >> 8)   |
               (((mode->CrtcVSyncStart) & 0x400) >> 6)   | 0x40;

   
   width = (pScrn->displayWidth * (pScrn->bitsPerPixel / 8))>> 3;
   vganew->CRTC[19] = 0xFF & width;
   new->CR51 = (0x300 & width) >> 4; /* Extension bits */
   
   /* And finally, select clock source 2 for programmable PLL */
   vganew->MiscOutReg |= 0x0c;      


   new->CR33 = 0x20;
   if (ps3v->Chipset == S3_ViRGE_DXGX || ps3v->Chipset == S3_ViRGE_GX2) {
      new->CR86 = 0x80;  /* disable DAC power saving to avoid bright left edge */
      new->CR90 = 0x00;  /* disable the stream display fetch length control */
   }
	 

   /* Now we handle various XConfig memory options and others */
   
   /* option "slow_edodram" sets EDO to 2 cycle mode on ViRGE */
   if (ps3v->Chipset == S3_ViRGE) {
      OUTREG8(vgaCRIndex, 0x36);
      new->CR36 = INREG8(vgaCRReg);
      if( ps3v->slow_edodram )
         new->CR36 = (new->CR36 & 0xf3) | 0x08;
      else  
         new->CR36 &= 0xf3;
      }
   
   /* Option "fpm_vram" for ViRGE_VX sets memory in fast page mode */
   if (ps3v->Chipset == S3_ViRGE_VX) {
      OUTREG8(vgaCRIndex, 0x36);
      new->CR36 = INREG8(vgaCRReg);
      if( ps3v->fpm_vram )
         new->CR36 |=  0x0c;
      else 
         new->CR36 &= ~0x0c;
      }
      
  #if 0
   if (mode->Private) {
      if (mode->Private[0] & (1 << S3_INVERT_VCLK)) {
	 if (mode->Private[S3_INVERT_VCLK])
	    new->CR67 |= 1;
	 else
	    new->CR67 &= ~1;
      }
      if (mode->Private[0] & (1 << S3_BLANK_DELAY)) {
	 if (s3vPriv.chip == S3_ViRGE_VX)
	    new->CR6D = mode->Private[S3_BLANK_DELAY];
	 else {
	    new->CR65 = (new->CR65 & ~0x38) 
	       | (mode->Private[S3_BLANK_DELAY] & 0x07) << 3;
	    OUTREG8(vgaCRIndex, 0x6d);
	    new->CR6D = INREG8(vgaCRReg);
	 }
      }
      if (mode->Private[0] & (1 << S3_EARLY_SC)) {
	 if (mode->Private[S3_EARLY_SC])
	    new->CR65 |= 2;
	 else
	    new->CR65 &= ~2;
      }
   }
   else {
      OUTREG8(vgaCRIndex, 0x6d);
      new->CR6D = INREG8(vgaCRReg);
   }
  #endif
  
   OUTREG8(vgaCRIndex, 0x68);
   new->CR68 = INREG8(vgaCRReg);
   new->CR69 = 0;
   
   pScrn->vtSema = TRUE;
   			    
   					/* Do it!  Write the mode registers */
					/* to hardware, start STREAMS if    */
					/* needed, etc.		    	    */
   S3VWriteMode( pScrn, vganew, new );

  return TRUE;
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should also unmap the video memory, and free
 * any per-generation data allocated by the driver.  It should finish
 * by unwrapping and calling the saved CloseScreen function.
 */

/* Mandatory */
static Bool
S3VCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  S3VPtr ps3v = S3VPTR(pScrn);
  vgaRegPtr vgaSavePtr = &hwp->SavedReg;
  S3VRegPtr S3VSavePtr = &ps3v->SavedReg;

    					/* Like S3VRestore, but uses passed */
					/* mode registers.		    */
  S3VWriteMode(pScrn, vgaSavePtr, S3VSavePtr);
  vgaHWLock(hwp);
  S3VUnmapMem(pScrn);
  if (ps3v->AccelInfoRec)
    XAADestroyInfoRec(ps3v->AccelInfoRec);
  #if 0  /* from MGA */
    if (pMga->CursorInfoRec)
    	xf86DestroyCursorInfoRec(pMga->CursorInfoRec);
  #endif
  pScrn->vtSema = FALSE;

  pScreen->CloseScreen = ps3v->CloseScreen;
  return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}




/* Do screen blanking */

/* Mandatory */
static Bool
S3VSaveScreen(ScreenPtr pScreen, Bool unblank)
{
  return vgaHWSaveScreen(pScreen, unblank);
}





/* This function inits the STREAMS processor variables. 
 * This has essentially been taken from the accel/s3_virge code and the databook.
 */
static void
S3VInitSTREAMS(ScrnInfoPtr pScrn, unsigned int *streams, DisplayModePtr mode)
{
  
   if ( pScrn->bitsPerPixel == 24 ) {
                         /* data format 8.8.8 (24 bpp) */
      streams[0] = 0x06000000;
      } 
   else if (pScrn->bitsPerPixel == 32) {
                         /* one more bit for X.8.8.8, 32 bpp */
      streams[0] = 0x07000000;
   }
                         /* NO chroma keying... */
   streams[1] = 0x0;
                         /* Secondary stream format KRGB-16 */
                         /* data book suggestion... */
   streams[2] = 0x03000000;

   streams[3] = 0x0;

   streams[4] = 0x0;
                         /* use 0x01000000 for primary over second. */
                         /* use 0x0 for second over prim. */
   streams[5] = 0x01000000;

   streams[6] = 0x0;

   streams[7] = 0x0;
                                /* Stride is 3 bytes for 24 bpp mode and */
                                /* 4 bytes for 32 bpp. */
   if ( pScrn->bitsPerPixel == 24 ) {
      streams[8] = 
             pScrn->displayWidth * 3;
      } 
   else {
      streams[8] = 
             pScrn->displayWidth * 4;
      }
                                /* Choose fbaddr0 as stream source. */
   streams[9] = 0x0;
   streams[10] = 0x0;
   streams[11] = 0x0;
   streams[12] = 0x1;

                                /* Set primary stream on top of secondary */
                                /* stream. */
   streams[13] = 0xc0000000;
                               /* Vertical scale factor. */
   streams[14] = 0x0;

   streams[15] = 0x0;
                                /* Vertical accum. initial value. */
   streams[16] = 0x0;
                                /* X and Y start coords + 1. */
   streams[18] =  0x00010001;

         /* Specify window Width -1 and Height of */
         /* stream. */
   streams[19] =
         (mode->HDisplay - 1) << 16 |
         (mode->VDisplay);
   
                                /* Book says 0x07ff07ff. */
   streams[20] = 0x07ff07ff;

   streams[21] = 0x00010001;
                            
}


   

/* Used to adjust start address in frame buffer. We use the new 
 * CR69 reg for this purpose instead of the older CR31/CR51 combo.
 * If STREAMS is running, we program the STREAMS start addr. registers. 
 */

static void
S3VAdjustFrame(int scrnIndex, int x, int y, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   vgaHWPtr hwp = VGAHWPTR(pScrn);
   S3VPtr ps3v = S3VPTR(pScrn);
   void *s3vMmioMem = ps3v->MapBase;
   int Base;
   int vgaCRIndex, vgaCRReg, vgaIOBase;
   vgaIOBase = hwp->IOBase;
   vgaCRIndex = vgaIOBase + 4;
   vgaCRReg = vgaIOBase + 5;

   if(ps3v->STREAMSRunning == FALSE) {
      /*
      Base = ((y * vga256InfoRec.displayWidth + x)
		* (vgaBitsPerPixel / 8)) >> 2;
	*/
      Base = ((y * pScrn->displayWidth + x)
		* (pScrn->bitsPerPixel / 8)) >> 2;

      /* Now program the start address registers */
      OUTREG16(vgaCRIndex, (Base & 0x00FF00) | 0x0C);
      OUTREG16(vgaCRIndex, ((Base & 0x00FF) << 8) | 0x0D);
      OUTREG8(vgaCRIndex, 0x69);
      OUTREG8(vgaCRReg, (Base & 0x0F0000) >> 16);   
      }
   else {          /* Change start address for STREAMS case */
      VerticalRetraceWait();
      if(ps3v->Chipset == S3_ViRGE_VX)
	((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0 =
	  ((y * pScrn->displayWidth + (x & ~7)) * pScrn->bitsPerPixel / 8);
      else
	((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0 =
	((y * pScrn->displayWidth + (x & ~3)) * pScrn->bitsPerPixel / 8);
      }

#if 0
#ifdef XFreeXDGA
   if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
      /* Wait until vertical retrace is in progress. */
      VerticalRetraceWait();
   }
#endif
#endif

   return;
}




/* Usually mandatory */
static Bool
S3VSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return S3VModeInit(xf86Screens[scrnIndex], mode);
}



void S3VLoadPalette(
    ScrnInfoPtr pScrn, 
    int numColors, 
    int *indicies,
    LOCO *colors,
    short visualClass
){
    S3VPtr ps3v = S3VPTR(pScrn);
    int i, index;

    for(i = 0; i < numColors; i++) {
	index = indicies[i];
        OUTREG8(0x3c8, index);
        OUTREG8(0x3c9, colors[index].red);
        OUTREG8(0x3c9, colors[index].green);
        OUTREG8(0x3c9, colors[index].blue);
    }
}





/* This function is used to debug, it prints out the contents of s3 regs */

static void
S3VPrintRegs(ScrnInfoPtr pScrn)
{
    unsigned char tmp1, tmp2;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    S3VPtr ps3v = S3VPTR(pScrn);
    int vgaCRIndex, vgaCRReg, vgaIOBase;
    vgaIOBase = hwp->IOBase;
    vgaCRIndex = vgaIOBase + 4;
    vgaCRReg = vgaIOBase + 5;
       
/* All registers */
    OUTREG8(0x3c4, 0x10);
    tmp1 = INREG8(0x3c5);
    OUTREG8(0x3c4, 0x11);
    tmp2 = INREG8(0x3c5);
    ErrorF("SR10: %02x SR11: %02x\n", tmp1, tmp2);

    OUTREG8(0x3c4, 0x12);
    tmp1 = INREG8(0x3c5);
    OUTREG8(0x3c4, 0x13);
    tmp2 = INREG8(0x3c5);
    ErrorF("SR12: %02x SR13: %02x\n", tmp1, tmp2);

    OUTREG8(0x3c4, 0x0a);
    tmp1 = INREG8(0x3c5);
    OUTREG8(0x3c4, 0x15);
    tmp2 = INREG8(0x3c5);
    ErrorF("SR0A: %02x SR15: %02x\n", tmp1, tmp2);

    /* Now load and print a whole rnage of other regs */
    for(tmp1=0x0;tmp1<=0x0f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x10;tmp1<=0x1f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x20;tmp1<=0x2f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x30;tmp1<=0x3f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x40;tmp1<=0x4f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x50;tmp1<=0x59;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x5d;tmp1<=0x67;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }
    ErrorF("\n");
    for(tmp1=0x68;tmp1<=0x6f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("CR%02x:%02x ",tmp1,INREG8(vgaCRReg));
    }

    ErrorF("\n\n");
 #if 0
/* New formatted registers, matches s3rc */
    ErrorF("Misc Out\n");
    ErrorF("  %02x\n",INREG8(0x3cc));

    ErrorF("CR[00-2f]\n");
    for(tmp1=0x0;tmp1<=0x2f;tmp1++){
	OUTREG8(vgaCRIndex, tmp1);
	ErrorF("%02x ",INREG8(vgaCRReg));
	if((tmp1 & 0x3) == 0x3) ErrorF(" ");
	if((tmp1 & 0xf) == 0xf) ErrorF("\n");
    }
    
    ErrorF("SR[00-0f]\n");
    for(tmp1=0x0;tmp1<=0x0f;tmp1++){
	OUTREG8(0x3c4, tmp1);
	ErrorF("%02x ",INREG8(0x3c5));
	if((tmp1 & 0x3) == 0x3) ErrorF(" ");
	if((tmp1 & 0xf) == 0xf) ErrorF("\n");
    }

    ErrorF("Gr Cont GR[00-0f]\n");
    for(tmp1=0x0;tmp1<=0x0f;tmp1++){
	OUTREG8(0x3ce, tmp1);
	ErrorF("%02x ",INREG8(0x3cf));
	if((tmp1 & 0x3) == 0x3) ErrorF(" ");
	if((tmp1 & 0xf) == 0xf) ErrorF("\n");
    }
    ErrorF("\n\n");
 #endif
}

/*EOF*/

