/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/p9000/p9000.c,v 3.10 1994/09/03 02:51:18 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1994 by Erik Nygren <nygren@mit.edu>
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
 * ERIK NYGREN, THOMAS ROELL, KEVIN E. MARTIN, RICKARD E. FAITH, SCOTT LAIRD,
 * DAVID MOEWS, AND TIAGO GONS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL ERIK NYGREN, THOMAS ROELL, KEVIN E. MARTIN, 
 * RICKARD E. FAITH, DAVID MOEWS SCOTT LAIRD, OR TIAGO GONS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Further modifications by Scott Laird (lair@kimbark.uchicago.edu)
 * and Tiago Gons (tiago@comosjn.hobby.nl)
 * Modified for the P9000 by Erik Nygren (nygren@mit.edu)
 * Bank switching code for P9000 by David Moews (dmoews@xraysgi.ims.uconn.edu)
 *
 * Header: /x/xc/programs/Xserver/hw/xfree86/accel/p9000/RCS/p9000.c,v 4.0 1994/05/28 01:24:17 nygren Exp
 */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "mipointer.h"
#include "cursorstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "p9000.h"
#include "p9000reg.h"
#include "p9000Bt485.h"
#include "p9000viper.h"
#include "p9000orchid.h"
#include "mi.h"
#include "cfb.h"

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

extern int p9000MaxClock;
extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed, xf86Verbose;

extern int defaultColorVisualClass;

ScrnInfoRec p9000InfoRec = {
    FALSE,		/* Bool configured */
    -1,			/* int tmpIndex */
    -1,			/* int scrnIndex */
    p9000Probe,      	/* Bool (* Probe)() */
    p9000Initialize,	/* Bool (* Init)() */
    p9000EnterLeaveVT,	/* void (* EnterLeaveVT)() */
    (void (*)())NoopDDA,/* void (* EnterLeaveMonitor)() */
    (void (*)())NoopDDA,/* void (* EnterLeaveCursor)() */
    (void (*)())NoopDDA,/* void (* AdjustFrame)() */
    p9000SwitchMode,	/* Bool (* SwitchMode)() */
    p9000PrintIdent,	/* void (* PrintIdent)() */
    8,			/* int depth */
    {0, 0, 0},		/* xrgb weight */
    8,			/* int bitsPerPixel */
    PseudoColor,       	/* int defaultVisual */
    -1, -1,		/* int virtualX,virtualY */
    -1,			/* int displayWidth */
    -1, -1, -1, -1,	/* int frameX0, frameY0, frameX1, frameY1 */
    {0, },              /* OFlagSet options */
    {0, },              /* OFlagSet clockOptions */
    {0, },              /* OFlagSet xconfigFlag */
    NULL,	       	/* char *chipset */
    NULL,	       	/* char *ramdac */
    0,			/* int dacSpeed */
    0,			/* int clocks */
    {0, },		/* int clock[MAXCLOCKS] */
    0,			/* int maxClock */
    0,			/* int videoRam */
    0,                  /* int BIOSbase, 1.3 new */
    80000000L,          /* unsigned long MemBase, 2.1 new */
    240, 180,		/* int width, height */
    0,                  /* unsigned long  speedup */
    NULL,	       	/* DisplayModePtr modes */
    NULL,	       	/* DisplayModePtr pModes */
    NULL,               /* char *clockprog */
    -1,                 /* int textclock, 1.3 new */
    FALSE,              /* Bool bankedMono */
    "P9000",            /* char *name */
    {0, },		/* xrgb blackColour */
    {0, },		/* xrgb whiteColour */
    p9000ValidTokens,	/* int *validTokens */
    P9000_PATCHLEVEL,	/* char *patchlevel */
    0,			/* int IObase */
    0,			/* int PALbase */
    0,			/* int COPbase */
    0,			/* int POSbase */
    0,			/* int instance */
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;

ScreenPtr p9000savepScreen;

Bool p9000BlockCursor, p9000ReloadCursor;
int p9000hotX, p9000hotY;

static int AlreadyInited = FALSE;

p9000CRTCRegRec p9000CRTCRegs;

p9000MiscRegRec  p9000MiscReg;

p9000VendorRec *p9000VendorPtr = NULL;  /* The probed vendor */

/* A list of vendors to be probed */
static p9000VendorRec *p9000VendorList[] = {
  &(p9000ViperVlbVendor),
#ifdef P9000_VPRPCI_SUP
  &(p9000ViperPciVendor),
#endif
#ifdef P9000_ORCHID_SUP
  &(p9000OrchidVendor),
#endif
};

static int Num_p9000_Vendors = (sizeof(p9000VendorList)/
				sizeof(p9000VendorList[0]));

static ScreenPtr savepScreen = NULL;

/* P9000 control register base */
volatile unsigned long *CtlBase; 
/* P9000 linear mapped frame buffer base */
volatile unsigned long *VidBase; 

volatile pointer p9000VideoMem;

unsigned p9000BytesPerPixel;  /* The number of bytes per pixel */

/* Options that may be specified to Xconfig */
Bool p9000SWCursor = FALSE;         /* Use a software cursor */
Bool p9000DACSyncOnGreen = FALSE;   /* Enables syncing on green */
Bool p9000DAC8Bit = TRUE;           /* Use 8 bits for cmap entry (the
				     * alternative is not implemented
				     * and may never be */
Bool p9000NoAccel = FALSE;          /* Disables hardware acceleration */

unsigned char p9000SwapBits[256];

static unsigned p9000_IOPorts[] = {
  /* VGA Registers */
  SEQ_INDEX_REG,    SEQ_PORT,         MISC_OUT_REG,     MISC_IN_REG,
  GRA_I,            GRA_D,            CRT_IC,           CRT_DC,
  IS1_RC,           ATT_IW,           ATT_R,            VGA_BANK_REG,

  /* BT484/485 Register Defines */  
  BT_PIXEL_MASK,    BT_READ_ADDR,     BT_WRITE_ADDR,    BT_RAMDAC_DATA,
  BT_COMMAND_REG_0, BT_CURS_RD_ADDR,  BT_CURS_WR_ADDR,  BT_CURS_DATA ,
  BT_STATUS_REG,    BT_COMMAND_REG_3, BT_CURS_RAM_DATA, BT_COMMAND_REG_1,
  BT_COMMAND_REG_2, BT_CURS_Y_LOW,    BT_CURS_Y_HIGH,   BT_CURS_X_LOW, 
  BT_CURS_X_HIGH,
};

static int Num_p9000_IOPorts = (sizeof(p9000_IOPorts)/
				sizeof(p9000_IOPorts[0]));

/* p9000WeightMasks must match p9000weights[], below */
static short p9000WeightMasks[] = { RGB16_565, RGB16_555, RGB32_888 };
short p9000WeightMask;

#define p9000ReorderSwapBits(a,b)    b = \
        (a & 0x80) >> 7 | \
        (a & 0x40) >> 5 | \
        (a & 0x20) >> 3 | \
        (a & 0x10) >> 1 | \
        (a & 0x08) << 1 | \
        (a & 0x04) << 3 | \
        (a & 0x02) << 5 | \
        (a & 0x01) << 7;

/* Raster operation (alu) -> minterm mapping */
unsigned int p9000alu[16];

/*
 * p9000Probe --
 *     probe and initialize the hardware driver
 */
Bool
p9000Probe()
{
    int            memavail, rounding;
    DisplayModePtr pMode, pEnd,
                   pLastAddedMode = NULL,
                   pNextMode,
                   pNewModeRing = NULL,  /* The start of a new ring of modes */
                   pmaxX = NULL, pmaxY = NULL;
    int            numValidModes = 0;
    int            maxX, maxY;
    int            curvendor;
    OFlagSet       validOptions;
    xrgb           p9000weights[] = { { 5, 6, 5 }, { 5, 5, 5 }, /* 16bpp */
				      { 8, 8, 8 } };            /* 32bpp */
    int            i;
    unsigned long  tmpsysconfig;  /* dummy value */

    p9000InfoRec.maxClock = p9000MaxClock;

    if (!xf86LinearVidMem())
      {
	ErrorF("This operating system does not support memory mapping of linear regions.\nAs a result, it can not be used with this server\n");
	return(FALSE);
      }

    xf86ClearIOPortList(p9000InfoRec.scrnIndex);
    xf86AddIOPorts(p9000InfoRec.scrnIndex, Num_p9000_IOPorts, p9000_IOPorts);

    if (p9000InfoRec.chipset)
      {
	for (curvendor = 0; curvendor < Num_p9000_Vendors; curvendor++)
	  {
	    if (0 == strcmp(p9000VendorList[curvendor]->Vendor,
			    p9000InfoRec.chipset))
	      {
		p9000VendorPtr = p9000VendorList[curvendor];
		break;
	      }
	  }
	if (curvendor >= Num_p9000_Vendors)
	  {
	    ErrorF("Chipset specified in Xconfig (%s) is not recognized!\n",
		   p9000InfoRec.chipset);
	    return(FALSE);
	  }
      }
    else
      {
	for (curvendor = 0; curvendor < Num_p9000_Vendors; curvendor++)
	  {
	    if (p9000VendorList[curvendor]->Probe())
	      {
		p9000VendorPtr = p9000VendorList[curvendor];
		break;
	      }
	  }
	if (curvendor >= Num_p9000_Vendors)
	  {
	    ErrorF("Autodetection of chipset failed.  Specify explicitly in Xconfig.\n");
	    return(FALSE);
	  }
      }
    if (xf86Verbose)
      ErrorF("%s %s: Vendor/chipset is %s (%s)\n",
	     (p9000InfoRec.chipset ? XCONFIG_GIVEN : XCONFIG_PROBED),
	     p9000InfoRec.name, p9000VendorPtr->Vendor, p9000VendorPtr->Desc);
    p9000InfoRec.chipset = p9000VendorPtr->Vendor;

    xf86EnableIOPorts(p9000InfoRec.scrnIndex);
    
    OFLG_ZERO(&validOptions);
    xf86VerifyOptions(&validOptions, &p9000InfoRec);
    
    if (!p9000InfoRec.clocks)
      {
	ErrorF("Autodetection of clocks is not supported.\n    Explicitly specify in Xconfig file on a Clocks line.\n");
	return(FALSE);
      }
    
    if (!p9000InfoRec.videoRam)
      {
	ErrorF("Autodetection of video RAM is not yet supported.\n    Explicitly specify VideoRAM in Xconfig file.\n");
	return(FALSE);	
      }
    memavail = p9000InfoRec.videoRam*1024;

    /* Validate the vendor specific things like MemBase */
    if (!p9000VendorPtr->Validate())
      return(FALSE);

    if (xf86bpp < 0) {
        xf86bpp = p9000InfoRec.depth;
    }
    if (p9000InfoRec.weight.red != 0 && p9000InfoRec.weight.green != 0 &&
        p9000InfoRec.weight.blue != 0) {
        xf86weight = p9000InfoRec.weight;
    }
    switch (xf86bpp)
      {
      case 8:
	break;
      case 16:
	p9000InfoRec.depth = 16;  /* if 555, set to 15 below */
	p9000InfoRec.bitsPerPixel = 16;
	p9000InfoRec.defaultVisual = TrueColor;
	defaultColorVisualClass = TrueColor;
	break;
      case 24:
      case 32:
	p9000InfoRec.depth = 24;
	p9000InfoRec.bitsPerPixel = 32;     /* Use sparse 24 bpp (RGBX) */
	p9000InfoRec.defaultVisual = TrueColor;
	defaultColorVisualClass = TrueColor;
	p9000MaxClock = P9000_MAX_32BPP_CLOCK;
	break;
      default:
	ErrorF("Invalid value for bpp.  Valid values are 8, 16, and 32.\n");
	return(FALSE);
      }

    if (xf86bpp == 16) 
      {
	for (i = 0; i < 2; i++) 
	  {
	    if (xf86weight.red == p9000weights[i].red
		&& xf86weight.green == p9000weights[i].green
		&& xf86weight.blue == p9000weights[i].blue)
	      break;
	  }
	if (i == 2) 
	  {
	    ErrorF("Invalid color weighting\n");
	    return(FALSE);
	  }
	if (i == RGB16_555)
	  p9000InfoRec.depth = 15;
	p9000WeightMask = p9000WeightMasks[i];
      }
    else if (xf86bpp == 32)
      {
	xf86weight.red =  xf86weight.green = xf86weight.blue = 8;
	p9000WeightMask = p9000WeightMasks[RGB32_888];
      }

    xf86ProbeFailed = FALSE;

    if (xf86Verbose)
    {
        ErrorF("%s %s: (mem: %dk numclocks: %d vendor: %s membase: 0x%lx)\n",
               OFLG_ISSET(XCONFIG_VIDEORAM,&p9000InfoRec.xconfigFlag) ?
               XCONFIG_GIVEN : XCONFIG_PROBED,
               p9000InfoRec.name,
               p9000InfoRec.videoRam,
               p9000InfoRec.clocks,
	       p9000VendorPtr->Vendor,
	       p9000InfoRec.MemBase);
      }

    /* All modes must have the same width and height as the first valid mode.
     * Any invalid modes are removed from the mode ring.
     */
    pMode = pEnd = p9000InfoRec.modes;
    do
      {
	xf86LookupMode(pMode, &p9000InfoRec);
	pNextMode = pMode->next;
	
	if (p9000InfoRec.clock[pMode->Clock] > 110000)
	  {
	    ErrorF("**********************************************************\n");
	    ErrorF("WARNING: The use of clocks over 110 MHz is not\n");
	    ErrorF("  recommended or supported and may damage your video card!\n");
	    ErrorF("**********************************************************\n");
	  }

	/* Now for a bunch of tests to see if the mode is possible and/or
	 * safe.  It it fails, the mode is removed.
	 */
        if (pMode->HDisplay * pMode->VDisplay * (p9000InfoRec.bitsPerPixel / 8)
	    > memavail)
	  ErrorF("%s: Too little memory for mode %s\n", p9000InfoRec.name,
		 pMode->name);
	else if ((pNewModeRing) && (pNewModeRing->VDisplay != pMode->VDisplay)
		 && (pNewModeRing->HDisplay != pMode->HDisplay))
	  ErrorF("%s: The dimensions of mode %s do not match those of\n\tthe first valid mode (%s)\n",
		 p9000InfoRec.name, pMode->name, pNewModeRing->name);
	/* These size limits are in effect until 1600x1200 is tested *TO*DO* */
	else if (pMode->HDisplay > 1280) 
	  ErrorF("%s: The width of mode %s is too large (max 1280)\n",
		 p9000InfoRec.name, pMode->name);
	else if (pMode->VDisplay > 1024) 
	  ErrorF("%s: The height of mode %s is too large (max 1024)\n",
		 p9000InfoRec.name, pMode->name);
	else if (p9000InfoRec.clock[pMode->Clock] > p9000MaxClock)
	  ErrorF("%s: The clock speed of mode %s is too high (max %ld MHz)\n",
		 p9000InfoRec.name, pMode->name, p9000MaxClock/1000);	  
	/* See if the horizontal resolution is valid.  This is constrained
	 * by possible values for sysconfig */
	else if (!p9000CalcSysconfigHres(pMode->HDisplay, 
					 p9000InfoRec.bitsPerPixel/8,
					 &tmpsysconfig))
	  ErrorF("%s: The width of mode %s is not valid\n",
		 p9000InfoRec.name, pMode->name);
	else   /* The mode has passed and is valid */
	  {
	    /* Link in the mode */
	    if (pNewModeRing == NULL)
	      {
		pNewModeRing = pLastAddedMode = pMode;
	      }
	    else
	      {
		pLastAddedMode->next = pMode;
		pLastAddedMode = pMode;
	      }
	  }
        pMode = pNextMode;
      } while (pMode != pEnd);
    
    if (pLastAddedMode == NULL)
      {
	ErrorF("No valid modes were found!\n");
	return(FALSE);
      }

    /* Finish and install the new mode ring */
    pLastAddedMode->next = pNewModeRing;
    p9000InfoRec.modes = pNewModeRing;
    /* Now link the ring the other way around. */
    pMode = pEnd = p9000InfoRec.modes;
    do
      {
	pMode->next->prev = pMode;
	pMode = pMode->next;
      } while (pMode != pEnd);

    p9000InfoRec.virtualX = pNewModeRing->HDisplay;
    p9000InfoRec.virtualY = pNewModeRing->VDisplay;
    
    if (xf86Verbose)
      ErrorF("%s %s: Virtual resolution set to %dx%d\n",
	     OFLG_ISSET(XCONFIG_VIRTUAL,&p9000InfoRec.xconfigFlag) ?
	     XCONFIG_GIVEN : XCONFIG_PROBED, p9000InfoRec.name,
	     p9000InfoRec.virtualX, p9000InfoRec.virtualY);

    if (OFLG_ISSET(OPTION_SYNC_ON_GREEN, &p9000InfoRec.options))
      {
	p9000DACSyncOnGreen = TRUE;
	if (xf86Verbose)
	  ErrorF("%s %s: Putting RAMDAC into sync-on-green mode\n",
		 XCONFIG_GIVEN, p9000InfoRec.name);
      }

    if (OFLG_ISSET(OPTION_NOACCEL, &p9000InfoRec.options))
      {
	p9000NoAccel = TRUE;
	if (xf86Verbose)
	  ErrorF("%s %s: Disabling hardware acceleration\n",
		 XCONFIG_GIVEN, p9000InfoRec.name);
      }

    p9000SWCursor = OFLG_ISSET(OPTION_SW_CURSOR, &p9000InfoRec.options);
    if (xf86Verbose)
      ErrorF("%s %s: Using %s cursor\n", p9000SWCursor ? XCONFIG_GIVEN :
	     XCONFIG_PROBED, p9000InfoRec.name,
	     p9000SWCursor ? "software" : "hardware");

    if (xf86Verbose) 
      {
	if (p9000InfoRec.bitsPerPixel == 8)
	  ErrorF("%s %s: Using %d bits per RGB value\n",
		 XCONFIG_PROBED, p9000InfoRec.name,
		 p9000DAC8Bit ?  8 : 6);
	else if (p9000InfoRec.bitsPerPixel == 16)
	  ErrorF("%s %s: Using 16 bpp.  Color weight: %1d%1d%1d\n", 
		 XCONFIG_GIVEN, p9000InfoRec.name, xf86weight.red,
		 xf86weight.green, xf86weight.blue);
	else if (p9000InfoRec.bitsPerPixel == 32)
	  ErrorF("%s %s: Using sparse 32 bpp.  Color weight: %1d%1d%1d\n", 
		 XCONFIG_GIVEN, p9000InfoRec.name, xf86weight.red,
		 xf86weight.green, xf86weight.blue);
      }

    xf86DisableIOPorts(p9000InfoRec.scrnIndex);   
#ifdef DEBUG
    ErrorF("Reached end of p9000Probe...\n");
#endif

    return(TRUE);
}

void
p9000PrintIdent()
{
  int curvendor;

  ErrorF("  %s: accelerated server for ", p9000InfoRec.name);
  ErrorF("Weitek P9000 graphics adaptors (Patchlevel %s)\n", 
	 p9000InfoRec.patchLevel);
  ErrorF("  Supported vendors (specify on chipset line):\n");
  for (curvendor = 0; curvendor < Num_p9000_Vendors; curvendor++)
    {
      ErrorF("\t%s\t%s\n", p9000VendorList[curvendor]->Vendor,
	     p9000VendorList[curvendor]->Desc);
    }
}

/*
 * p9000Initialize --
 *      Attempt to find and initialize a VGA framebuffer
 *      Most of the elements of the ScreenRec are filled in.  The
 *      video is enabled for the frame buffer...
 */

Bool
p9000Initialize (scr_index, pScreen, argc, argv)
    int            scr_index;    /* The index of pScreen in the ScreenInfo */
    ScreenPtr      pScreen;      /* The Screen to initialize */
    int            argc;         /* The number of the Server's arguments. */
    char           **argv;       /* The arguments themselves. Don't change! */
{
  int i;

  xf86EnableIOPorts(scr_index);
#ifdef DEBUG
    ErrorF("Entered p9000Initialize...\n");
#endif

#ifdef P9000_ACCEL
  /* Prepare GC stuff for use */
  p9000InitGC();
#endif

  /* Maps P9000 linear address space into video memory */
  p9000InitAperture(scr_index);

  /* Do vendor specific initialization */
  p9000VendorPtr->Initialize(scr_index, pScreen, argc, argv);

  /* Set up swap bits table */
  for (i = 0; i < 256; i++)
    {
      p9000ReorderSwapBits(i, p9000SwapBits[i]);
    }

  /* Save the VT's colormap and such before anyone messes with it */
  p9000SaveVT();

  /* Calculates registers for this mode */
  p9000CalcCRTCRegs(&p9000CRTCRegs, p9000InfoRec.modes);

  /* Enables the P9000 */
  p9000VendorPtr->Enable(&p9000CRTCRegs);

  /* Calculates registers specific to the P9000 and its memory size */
  p9000CalcMiscRegs();  /* Enable must be run before this! */

#ifdef DEBUG
    ErrorF("About to enter p9000SetCRTCRegs\n");
#endif

  /* Load in the P9000 registers to set this resolution */
  p9000SetCRTCRegs(&p9000CRTCRegs); 

#ifdef DEBUG
    ErrorF("Done with p9000SetCRTCRegs.. Entering p9000InitEnvironment\n");
#endif

  p9000InitEnvironment();
  AlreadyInited = TRUE;

#ifdef DEBUG
    ErrorF("Done with p9000InitEnv... About to enter p9000ScreenInit\n");
#endif
  
  if (!p9000ScreenInit(pScreen,
		       (volatile pointer) VidBase,
		       p9000InfoRec.virtualX, p9000InfoRec.virtualY,
		       75, 75,
		       p9000InfoRec.virtualX))
    return(FALSE);
  
  pScreen->CloseScreen = p9000CloseScreen;
  pScreen->SaveScreen = p9000SaveScreen;
  
#ifdef DEBUG
    ErrorF("pScreen being set up...\n");
#endif

#if 0
  mfbRegisterCopyPlaneProc (pScreen, miCopyPlane);
#endif

  switch (p9000InfoRec.bitsPerPixel) 
    {
    case 8:
      pScreen->InstallColormap = p9000InstallColormap;
      pScreen->UninstallColormap = p9000UninstallColormap;
      pScreen->ListInstalledColormaps = p9000ListInstalledColormaps;
      pScreen->StoreColors = p9000StoreColors;
      break;
    case 16:
    case 32:
      pScreen->InstallColormap = cfbInstallColormap;
      pScreen->UninstallColormap = cfbUninstallColormap;
      pScreen->ListInstalledColormaps = cfbListInstalledColormaps;
      pScreen->StoreColors = (void (*)())NoopDDA;
    }

  if (p9000SWCursor)
    miDCInitialize (pScreen, &xf86PointerScreenFuncs);
  else
    {
      xf86PointerScreenFuncs.WarpCursor = p9000WarpCursor;
      (void)p9000CursorInit(0, pScreen); 
      pScreen->QueryBestSize = p9000QueryBestSize;
    }
  
  savepScreen = pScreen;

  p9000savepScreen = pScreen;

#ifdef DEBUG
    ErrorF("Leaving p9000Initialize...\n");
#endif

#ifdef P9000_ACCEL
  p9000ImageInit();
#endif

  return (cfbCreateDefColormap(pScreen));
}

/*
 * p9000EnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
p9000EnterLeaveVT(enter, screen_idx)
     Bool enter;
     int screen_idx;
{

#ifdef DEBUG
  ErrorF("Entered EnterLeaveVT: ");
#endif
  if (enter)
    {
#ifdef DEBUG
      ErrorF("Entering\n");
#endif
      xf86EnableIOPorts(p9000InfoRec.scrnIndex);
      p9000SaveVT();
      p9000VendorPtr->Enable(&p9000CRTCRegs);
      p9000SetCRTCRegs(&p9000CRTCRegs); 
      p9000RestoreDACvalues();
      AlreadyInited = TRUE;
    } 
  else
    {
#ifdef DEBUG
      ErrorF("Leaving\n");
#endif
      p9000CleanUp();     /* This does p9000RestoreVT() */
      xf86DisableIOPorts(p9000InfoRec.scrnIndex);
    }
}

/*
 * p9000CloseScreen --
 *      called to ensure video is enabled when server exits.
 */

Bool
p9000CloseScreen(screen_idx, pScreen)
     int screen_idx;
     ScreenPtr pScreen;
{
  /*
   * Hmm... The server may shut down even if it is not running on the
   * current vt. Let's catch this case here.
   */
  xf86Exiting = TRUE;
  if (xf86VTSema) 
    p9000EnterLeaveVT(LEAVE, screen_idx);
  return(TRUE);
}

/*
 * p9000SaveScreen --
 *      blank the screen.
 *      Only supported for 8bpp.  *TO*DO*  
 *      For now, use the screen saver extension.
 */

Bool
p9000SaveScreen (pScreen, on)
     ScreenPtr     pScreen;
     Bool          on;
{
  if (on) 
    {
      SetTimeSinceLastInputEvent();
      if (p9000InfoRec.bitsPerPixel == 8)
	{
	  if (xf86VTSema && !p9000SWCursor)
	    p9000BtCursorOn();
	  p9000UnblankScreen(pScreen);
	}
    }
  else
    {
      if (p9000InfoRec.bitsPerPixel == 8)
	{
	  if (xf86VTSema && !p9000SWCursor)
	    p9000BtCursorOff();
	  p9000BlankScreen(pScreen);
	}
    }
  return(TRUE);
}

/*
 * p9000SwitchMode --
 *     Set a new display mode
 */
Bool
p9000SwitchMode(mode)
     DisplayModePtr mode;
{
  p9000CalcCRTCRegs(&p9000CRTCRegs, mode);
  p9000VendorPtr->Enable(&p9000CRTCRegs);
  p9000SetCRTCRegs(&p9000CRTCRegs); 
  
  return(TRUE);
}

