/* $XFree86: $ */
/*
 * Copyright 1997 by Alan Hourihane, Wigan, England.
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

#include "misc.h"
#include "cfb.h"
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

#include "xf86Procs.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86_PCI.h"
#include "xf86Version.h"
#include "glint_regs.h"
#include "glint.h"
#define GLINT_SERVER
#include "IBMRGB.h"

#include "xf86xaa.h"
#include "xf86scrin.h"
#include "xf86_Config.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef DPMSExtension
#include "opaque.h"
#include "extensions/dpms.h"
#endif

static int glintValidMode(
#if NeedFunctionPrototypes
    DisplayModePtr,
    Bool,
    int
#endif
);

#if defined(XFree86LOADER)

#define _NO_XF86_PROTOTYPES
#include "xf86.h"

#define GLINT_MAX_CLOCK	220000

int glintMaxClock = GLINT_MAX_CLOCK;

ScrnInfoPtr xf86Screens[] = 
{
  &glintInfoRec,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  ACCEL,
  -1
};

int glintValidTokens[] =
{
  STATICGRAY,
  GRAYSCALE,
  STATICCOLOR,
  PSEUDOCOLOR,
  TRUECOLOR,
  DIRECTCOLOR,
  CHIPSET,
  CLOCKS,
  MODES,
  OPTION,
  VIDEORAM,
  VIEWPORT,
  VIRTUAL,
  CLOCKPROG,
  BIOSBASE,
  MEMBASE,
  -1
};
#endif

ScrnInfoRec glintInfoRec = {
    FALSE,		/* Bool configured */
    -1,			/* int tmpIndex */
    -1,			/* int scrnIndex */
    glintProbe,      	/* Bool (* Probe)() */
    glintInitialize,	/* Bool (* Init)() */
    glintValidMode,	/* Bool (* ValidMode)() */
    glintEnterLeaveVT,	/* void (* EnterLeaveVT)() */
    (void (*)())NoopDDA,/* void (* EnterLeaveMonitor)() */
    (void (*)())NoopDDA,/* void (* EnterLeaveCursor)() */
    glintAdjustFrame,	/* void (* AdjustFrame)() */
    glintSwitchMode,	/* Bool (* SwitchMode)() */
    glintDPMSSet,	/* void (* DPMSSet)() */
    glintPrintIdent,	/* void (* PrintIdent)() */
    8,			/* int depth */
    {5, 6, 5},          /* xrgb weight */
    8,			/* int bitsPerPixel */
    PseudoColor,       	/* int defaultVisual */
    -1, -1,		/* int virtualX,virtualY */
    -1,                 /* int displayWidth */
    -1, -1, -1, -1,	/* int frameX0, frameY0, frameX1, frameY1 */
    {0, },	       	/* OFlagSet options */
    {0, },	       	/* OFlagSet clockOptions */
    {0, },	       	/* OFlagSet xconfigFlag */
    NULL,	       	/* char *chipset */
    NULL,	       	/* char *ramdac */
    {0, 0, 0, 0},	/* int dacSpeeds[MAXDACSPEEDS] */
    0,			/* int dacSpeedBpp */
    0,			/* int clocks */
    {0, },		/* int clock[MAXCLOCKS] */
    0,			/* int maxClock */
    0,			/* int videoRam */
    0, 		        /* int BIOSbase */   
    0,			/* unsigned long MemBase */
    240, 180,		/* int width, height */
    0,                  /* unsigned long  speedup */
    NULL,	       	/* DisplayModePtr modes */
    NULL,	       	/* MonPtr monitor */
    NULL,               /* char *clockprog */
    -1,                 /* int textclock */   
    FALSE,              /* Bool bankedMono */
    "GLINT",            /* char *name */
    {0, },		/* xrgb blackColour */
    {0, },		/* xrgb whiteColour */
    glintValidTokens,	/* int *validTokens */
    GLINT_PATCHLEVEL,	/* char *patchlevel */
    0,			/* int IObase */
    0,			/* int PALbase */
    0,			/* int COPbase */
    0,			/* int POSbase */
    0,			/* int instance */
    0,			/* int s3Madjust */
    0,			/* int s3Nadjust */
    0,			/* int s3MClk */
    0,			/* int chipID */
    0,			/* int chipRev */
    0,			/* unsigned long VGAbase */
    40000,		/* int s3RefClk */
    -1,			/* int s3BlankDelay */
    0,			/* int textClockFreq */
    NULL,               /* char* DCConfig */
    NULL,               /* char* DCOptions */
    0			/* int MemClk */
#ifdef XFreeXDGA
    ,0,			/* int directMode */
    NULL,		/* Set Vid Page */
    0,			/* unsigned long physBase */
    0,			/* int physSize */
#endif
};

#if defined(XFree86LOADER)
XF86ModuleVersionInfo glintVersRec = 
{
	"libglint.a",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}
};

ScrnInfoRec *
ServerInit()
{
return &glintInfoRec;
}

void
ModuleInit(data,magic)
	pointer	* data;
	INT32	* magic;
{
	static int cnt = 0;

	switch(cnt++)
	{
		case 0:
			* data = (pointer) &glintVersRec;
			* magic= MAGIC_VERSION;
			break;
		case 1:
			* data = (pointer) &glintInfoRec;
			* magic= MAGIC_ADD_VIDEO_CHIP_REC;
			break;
		case 2:
			* data = (pointer) "libmfb.a";
			* magic= MAGIC_LOAD;
			break;
		case 3:
			* data = (pointer) "xaa8.o";
			* magic= MAGIC_LOAD;
			break;
		case 4:
			* data = (pointer) "xaa16.o";
			* magic= MAGIC_LOAD;
			break;
		case 5:
			* data = (pointer) "xaa32.o";
			* magic= MAGIC_LOAD;
			break;
		case 6:
			* data = (pointer) "libxaa.a";
			* magic= MAGIC_LOAD;
			break;
		default:
			* magic= MAGIC_DONE;
			break;
	}
	return;
}
#endif /* XFree86LOADER */

Bool glintDoubleBufferMode = FALSE;
extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed;
ScreenPtr savepScreen = NULL;
Bool glintReloadCursor, glintBlockCursor;
unsigned char glintSwapBits[256];
pointer glint_reg_base;
int glinthotX, glinthotY;
static PixmapPtr ppix = NULL;
int glintDisplayWidth;
volatile pointer glintVideoMem = NULL;
volatile pointer GLINTMMIOBase;
volatile pointer PERMEDIAMMIOBase;
Bool AlreadyInited;
int coprotype = -1;
int glintLBBase;
int glintLBvideoRam;
int GLINTFrameBufferSize, GLINTLocalBufferSize;
int GLINTWindowBase;
extern unsigned char *glintVideoMemSave;
int glintAdjustCursorXPos = 0;
static glintCRTCRegRec glintCRTCRegs;
extern int defaultColorVisualClass;
volatile unsigned long *VidBase;
Bool xf86issvgatype;
#define glintReorderSwapBits(a,b)		b = \
		(a & 0x80) >> 7 | \
		(a & 0x40) >> 5 | \
		(a & 0x20) >> 3 | \
		(a & 0x10) >> 1 | \
		(a & 0x08) << 1 | \
		(a & 0x04) << 3 | \
		(a & 0x02) << 5 | \
		(a & 0x01) << 7;

/*
 * glintPrintIdent
 */

void
glintPrintIdent()
{
	ErrorF("  %s: accelerated server for 3DLabs GLINT graphics adapters\n",
			glintInfoRec.name);
	ErrorF("(Patchlevel %s)\n", glintInfoRec.patchLevel);
}

/*
 * IBMRGBClockSelect --
 * 	Set the programmable clock
 */
Bool
IBMRGBClockSelect(int freq)
{
	Bool result = TRUE;

	switch(freq)
	{
		case CLK_REG_SAVE:
		case CLK_REG_RESTORE:
			break;
		default:
		{
			if (freq < 16250) {
				ErrorF("%s %s: Specified dot clock (%.3f) too"
				       "low for IBM Ramdac", XCONFIG_PROBED,
				       glintInfoRec.name, freq / 1000.0);
				result = FALSE;
				break;
			}			
			(void)IBMRGBGlintSetClock(freq, 2,
					glintInfoRec.dacSpeeds[0],
					glintInfoRec.s3RefClk);
		}
	}

	usleep(5000);

	return(result);
}

static void
glintEnableIO(int scrnIndex)
{
    /* This is enough to ensure that full I/O is enabled */
    unsigned pciIOPorts[] = { PCI_MODE1_ADDRESS_REG };
    int numPciIOPorts = sizeof(pciIOPorts) / sizeof(pciIOPorts[0]);

    xf86ClearIOPortList(scrnIndex);
    xf86AddIOPorts(scrnIndex, numPciIOPorts, pciIOPorts);
    xf86EnableIOPorts(scrnIndex);
}

static void
glintDisableIO(int scrnIndex)
{
    xf86DisableIOPorts(scrnIndex);
    xf86ClearIOPortList(scrnIndex);
}

/*
 * glintProbe --
 *      check up whether a GLINT based board is installed
 */

Bool
glintProbe()
{
  int i, ibm_id;
  int tx, ty;
  int temp;
  Bool pModeInited = FALSE;
  DisplayModePtr pMode, pEnd;
  OFlagSet validOptions;
  pciConfigPtr pcrp = NULL;
  pciConfigPtr pcrpglint = NULL;
  pciConfigPtr pcrpdelta = NULL;
  pciConfigPtr *pcrpp;
  unsigned long glintdelta = 0;
  unsigned long glintcopro = 0;
  unsigned long basecopro = 0;
  unsigned long base3copro;
  unsigned long basedelta;
  unsigned long *delta_pci_basep;
  int cardnum = -1;

  pcrpp = xf86scanpci(glintInfoRec.scrnIndex);
 
  if (!pcrpp)
	return(FALSE);

  i = -1;
  while ((pcrp = pcrpp[++i]) != (pciConfigPtr)NULL) {
    if (pcrp->_vendor == PCI_VENDOR_3DLABS)
    {
        switch (pcrp->_device)
	{
	case PCI_CHIP_3DLABS_300SX:
		glintcopro = PCI_EN |
			(pcrp->_bus << 16) |
			(pcrp->_cardnum << 11) | (pcrp->_func << 8);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_300SX;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("found second board based on GLINT "
			       "will use information from there\n");
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose > 1 ) 
		{
			ErrorF("found GLINT 300SX at card #%d func #%d with "
			       "base 0x%x\n",pcrp->_cardnum,pcrp->_func,
			       basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_500TX:
		glintcopro = PCI_EN |
			(pcrp->_bus << 16) |
			(pcrp->_cardnum << 11) | (pcrp->_func << 8);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_500TX;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("found second board based on GLINT "
			       "will use information from there\n");
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose > 1 ) 
		{
			ErrorF("found GLINT 500TX at card #%d func #%d with "
			       "base 0x%x\n",pcrp->_cardnum,pcrp->_func,
			       basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_PERMEDIA:
		glintcopro = PCI_EN |
			(pcrp->_bus << 16) |
			(pcrp->_cardnum << 11) | (pcrp->_func << 8);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_PERMEDIA;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("found second board based on GLINT "
			       "will use information from there\n");
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose > 1 ) 
		{
			ErrorF("found GLINT PerMedia at card #%d func #%d with "
			       "base 0x%x\n",pcrp->_cardnum,pcrp->_func,
			       basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_DELTA:
		glintdelta = PCI_EN |
			(pcrp->_bus << 16) |
			(pcrp->_cardnum << 11) | (pcrp->_func << 8);
		basedelta = pcrp->_base0;
		delta_pci_basep = &(pcrp->_base0);
		pcrpdelta = pcrp;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("found second board based on GLINT "
			       "will use information from there\n");
			coprotype = -1;
			glintcopro = 0;
			pcrpglint = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose > 1 ) 
		{
			ErrorF("found GLINT Delta at card #%d func #%d with "
			       "base 0x%x\n",pcrp->_cardnum,pcrp->_func,
			       basedelta);
		}
		break;
	}
    }
  }
  if (!pcrpdelta)
  {
      /*
       * we only deal with cards that have a Delta installed as well
       */
      return FALSE;
  }
  /*
   * next, we should enable memory and I/O on the card, just to be sure that 
   * the BIOS didn't try to be smart and disabled that for anything except 
   * the first VGA card (which would be the ViRGE chip here)
   */
  xf86writepci(glintInfoRec.scrnIndex, pcrpglint->_bus, pcrpglint->_cardnum,
  	       pcrpglint->_func, PCI_CMD_STAT_REG, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE);
  xf86writepci(glintInfoRec.scrnIndex, pcrpdelta->_bus, pcrpdelta->_cardnum,
  	       pcrpdelta->_func, PCI_CMD_STAT_REG, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE);

  /*
   * due to a few bugs in the GLINT Delta we might have to relocate
   * the base address of config region of the Delta, if bit 17 of
   * the base addresses of config region of the Delta and the 500TX
   * or 300SX are different
   * We only handle config type 1 at this point
   */
  if( glintdelta && glintcopro )
  {
    if( (basedelta & 0x20000) ^ (basecopro & 0x20000) )
    {
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
	glintEnableIO(glintInfoRec.scrnIndex);
	outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x1c); /* base3 */
	base3copro  = inl(PCI_MODE1_DATA_REG);
	if( (basecopro & 0x20000) ^ (base3copro & 0x20000) )
	{
	    /*
	     * oops, still different; we know that base3 is at least
	     * 16 MB, so we just take 128k offset into it
	     */
	    base3copro += 0x20000;
	}
	/*
	 * and now for the magic.
	 * read old value
	 * write fffffffff
	 * read value
	 * write new value
	 */
	outl(PCI_MODE1_ADDRESS_REG, glintdelta | 0x10);
	temp = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, glintdelta | 0x10);
	outl(PCI_MODE1_DATA_REG,0xffffffff);
	outl(PCI_MODE1_ADDRESS_REG, glintdelta | 0x10);
	temp = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, glintdelta | 0x10);
	outl(PCI_MODE1_DATA_REG,base3copro);
	/*
	 * additionally, sometimes we see the baserom which might
	 * confuse the chip, so let's make sure that is disabled
	 */
	outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x30);
	temp = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x30);
	outl(PCI_MODE1_DATA_REG,0xffffffff);
	outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x30);
	temp = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x30);
	outl(PCI_MODE1_DATA_REG,0x0);

	glintDisableIO(glintInfoRec.scrnIndex);
	/*
	 * now update our internal structure accordingly
	 */
	*delta_pci_basep = base3copro;
    }
  }
  if (glintcopro) {
	glintEnableIO(glintInfoRec.scrnIndex);
	outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x04);
	temp = inl(PCI_MODE1_DATA_REG);
       	outl(PCI_MODE1_DATA_REG, temp | 0x04); /* Master enable */
	temp = inl(PCI_MODE1_DATA_REG);
	glintDisableIO(glintInfoRec.scrnIndex);
  }

  if (coprotype == PCI_CHIP_3DLABS_500TX) {
	  GLINTMMIOBase = xf86MapVidMem(0,MMIO_REGION,
  				(pointer)pcrpdelta->_base0,0x20000);
	  glintLBBase = pcrpglint->_base1;
	  ErrorF("%s %s: Localbuffer address at 0x%x\n",XCONFIG_PROBED,
			glintInfoRec.name, glintLBBase);
	  glintInfoRec.MemBase = pcrpglint->_base2;
  } else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
	  GLINTMMIOBase = xf86MapVidMem(0,MMIO_REGION,
  				(pointer)pcrpdelta->_base0,0x20000);
	  PERMEDIAMMIOBase = xf86MapVidMem(0,MMIO_REGION,
  				(pointer)pcrpglint->_base0,0x20000);
  	  glintLBBase = 0; /* no local buffer on PerMedia cards */
	  glintInfoRec.MemBase = pcrpglint->_base1;
  }
  ErrorF("%s %s: Framebuffer address at 0x%x\n",XCONFIG_PROBED,
		glintInfoRec.name, glintInfoRec.MemBase);

  if (!glintInfoRec.videoRam)
  {
  	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		GLINTFrameBufferSize = GLINT_READ_REG(FBMemoryCtl);
		glintInfoRec.videoRam = 1024 * (1 << ((GLINTFrameBufferSize &
						0xE0000000) >> 29));
	} else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		/*
		 * we need to get this field from the PerMedia, not the 
		 * Delta chip */
		GLINTFrameBufferSize = 
			*(unsigned int *)((char*)PERMEDIAMMIOBase+PMMemConfig);
		glintInfoRec.videoRam = 2048 * (1 + ((GLINTFrameBufferSize &
						0x60000000) >> 29));
	}
	ErrorF("%s %s: videoram : %dk\n", XCONFIG_PROBED, 
		glintInfoRec.name, glintInfoRec.videoRam);
  }
  else
  {
  	ErrorF("%s %s: videoram : %dk\n", XCONFIG_GIVEN, glintInfoRec.name,
		glintInfoRec.videoRam);
  }

  if (coprotype == PCI_CHIP_3DLABS_500TX) {
	  GLINTLocalBufferSize = GLINT_READ_REG(LBMemoryCtl);
	  glintLBvideoRam = 1024 * 
	  		    (1 << ((GLINTLocalBufferSize & 0x07000000) >> 24));
	  ErrorF("%s %s: localbuffer ram : %dk\n", XCONFIG_PROBED, 
	  				glintInfoRec.name, glintLBvideoRam);
  }
  ErrorF("%s %s: ", XCONFIG_PROBED, glintInfoRec.name);

  ibm_id = glintIBMRGB_Probe();
  if (ibm_id) {
	switch (ibm_id) {
		case 0x280:
			ErrorF("Detected IBM 526DB Ramdac\n");
			break;
		case 0x2C0:
			ErrorF("Detected IBM 526 Ramdac\n");
			break;
		case 0x2F0:
		case 0x2E0:
			ErrorF("Detected IBM 524 Ramdac\n");
			break;
		case 0x30C0:
			ErrorF("Detected IBM 624 Ramdac\n");
			break;
	}
#if DEBUG
	glintDumpRegs();
#endif

	glintIBMRGB52x_PreInit();
  } 

  if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
  	/* we need to turn off the VGA part as the very first thing 
	 * and make sure that the Aperture is set up correctly 
	 * additionally, we want to set a few other magic values */
	GLINT_WRITE_REG(0x05,0x63c4);
	GLINT_WRITE_REG(0x00,0x63c4);
	GLINT_WRITE_REG(0x0f,ErrorFlags);
	GLINT_WRITE_REG(0x00,IntEnable);
	GLINT_WRITE_REG(0x8000003f,IntFlags);
	GLINT_WRITE_REG(0xffffffff,PMBypassWriteMask);
	GLINT_WRITE_REG(0x7ff,PMInterruptLine);
	GLINT_WRITE_REG(0x01,FBWriteMode);
	GLINT_WRITE_REG(0x00,Aperture0);
	GLINT_WRITE_REG(0x7ff0000,PackedDataLimits);
	GLINT_WRITE_REG(0x7ff0000,XLimits);
	GLINT_WRITE_REG(0x7ff0000,YLimits);
  }
  /* Initialize options that reflect the GLINT */
  OFLG_ZERO(&validOptions);

  OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &validOptions);
  OFLG_SET(CLOCK_OPTION_IBMRGB, &validOptions);
  OFLG_SET(OPTION_IBMRGB_CURS, &validOptions);
  OFLG_SET(OPTION_XAA_BENCHMARK, &validOptions);
  OFLG_SET(OPTION_NOACCEL, &validOptions);

  OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &glintInfoRec.clockOptions);
  OFLG_SET(CLOCK_OPTION_IBMRGB, &glintInfoRec.clockOptions);
  OFLG_SET(OPTION_IBMRGB_CURS, &glintInfoRec.options);

  xf86VerifyOptions(&validOptions, &glintInfoRec);
  glintInfoRec.chipset = "glint";
  xf86ProbeFailed = FALSE;

  glintInfoRec.dacSpeeds[0] = 220000; 
  glintInfoRec.maxClock = 220000;

  /* Let's grab the basic mode lines */
#if 0
  tx = glintInfoRec.virtualX;
  ty = glintInfoRec.virtualY;
#else
  if (glintInfoRec.virtualX > 0) {
	ErrorF("%s %s: Virtual coordinates - Not yet supported. "
		   "Ignoring.\n", XCONFIG_GIVEN, glintInfoRec.name);
  }
#endif
  pMode = glintInfoRec.modes;
  if (pMode == NULL)
  { 
	ErrorF("No modes specified in the XF86Config file.\n");
	return (FALSE);
  }
  pEnd = (DisplayModePtr)NULL;
  do
  {
	DisplayModePtr pModeSv;

	pModeSv = pMode->next;
	
	/* Delete any invalid ones */
	if (xf86LookupMode(pMode, &glintInfoRec, LOOKUP_DEFAULT) == FALSE) {
		pModeSv = pMode->next;
		xf86DeleteMode(&glintInfoRec, pMode);
		pMode = pModeSv;
#if 0
	} else if (((tx > 0) && (pMode->HDisplay > tx)) ||
		   ((ty > 0) && (pMode->VDisplay > ty))) {
		ErrorF("%s %s: Resolution %dx%d too large for virtual %dx%d\n",
			XCONFIG_PROBED, glintInfoRec.name,
			pMode->HDisplay, pMode->VDisplay, tx, ty);
		xf86DeleteMode(&glintInfoRec, pMode);
#endif
	} else {
		if (pEnd == (DisplayModePtr) NULL)
			pEnd = pMode;

#if 0
		glintInfoRec.virtualX = max(glintInfoRec.virtualX,
						pMode->HDisplay);
		glintInfoRec.virtualY = max(glintInfoRec.virtualY,
						pMode->VDisplay);
#else
		if (pMode->HDisplay % 4)
		{
			pModeSv = pMode->next;
			ErrorF("%s %s: Horizontal Resolution %d not divisible"
				" by a factor of 4, removing modeline.\n",
				XCONFIG_GIVEN, glintInfoRec.name,
				pMode->HDisplay);
			xf86DeleteMode(&glintInfoRec, pMode);
			pMode = pModeSv;
		}
		else
		{
			glintInfoRec.virtualX = pMode->HDisplay;
			glintInfoRec.virtualY = pMode->VDisplay;
			pModeInited = TRUE; /* We have a mode - only 1 supported */
		}
#endif
	}
	pMode = pModeSv;
  } while (pModeInited == FALSE) /* (pMode != pEnd) */; 

#if 1
  ErrorF("%s %s: GLINT chipset currently only supports one modeline.\n",
		XCONFIG_PROBED, glintInfoRec.name);
#endif

  glintInfoRec.displayWidth = glintInfoRec.virtualX;

  if (xf86bpp < 0)
	xf86bpp = glintInfoRec.depth;
  if (xf86weight.red == 0 || xf86weight.green == 0 || xf86weight.blue == 0)
	xf86weight = glintInfoRec.weight;
  switch (xf86bpp) {
	case 8:
		/* XAA uses this */
		xf86weight.green = 8;
		break;
	case 16:
		glintInfoRec.depth = 15;
		xf86weight.green = xf86weight.red = xf86weight.blue = 5;
		glintInfoRec.bitsPerPixel = 16;
		if (glintInfoRec.defaultVisual < 0)
			glintInfoRec.defaultVisual = TrueColor;
		if (defaultColorVisualClass < 0)
			defaultColorVisualClass = glintInfoRec.defaultVisual;
		break;
	case 32:
		glintInfoRec.bitsPerPixel = 32;
		glintInfoRec.depth = 32;
		xf86weight.red = xf86weight.blue = xf86weight.green = 8;
		if (glintInfoRec.defaultVisual < 0)
			glintInfoRec.defaultVisual = TrueColor;
		if (defaultColorVisualClass < 0)
			defaultColorVisualClass = glintInfoRec.defaultVisual;
		break;
	default:
		ErrorF("Invalid bpp.\n");
		return (FALSE);
		break;
  }

#ifdef XFreeXDGA
#ifdef NOTYET
  glintInfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

  return(TRUE);
}

/*
 * glintInitialize --
 *
 */

Bool
glintInitialize (int scr_index, ScreenPtr pScreen, int argc, char **argv)
{
	int displayResolution = 75; 	/* default to 75dpi */
	int i;
	extern int monitorResolution;
	Bool (*ScreenInitFunc)(register ScreenPtr,pointer,int,int,int,int,int);

	/* Init the screen */
	
	glintInitAperture(scr_index);
	glintInit(glintInfoRec.modes);
	glintInitEnvironment();
	AlreadyInited = TRUE;
	glintCalcCRTCRegs(&glintCRTCRegs, glintInfoRec.modes);
	glintSetCRTCRegs(&glintCRTCRegs);
	for (i = 0; i < 256; i++)
	{ 
		glintReorderSwapBits(i, glintSwapBits[i]);
	}

	/*
	 * Take display resolution from the -dpi flag 
	 */
	if (monitorResolution)
		displayResolution = monitorResolution;

  	/* Let's use the new XAA Architecture.....*/
	
	if (OFLG_ISSET(OPTION_NOACCEL, &glintInfoRec.options))
	{
		switch (glintInfoRec.bitsPerPixel) {
			case 8:
				ScreenInitFunc = &cfbScreenInit;
				break;
			case 16:
				ScreenInitFunc = &cfb16ScreenInit;
				break;
			case 32:
				ScreenInitFunc = &cfb32ScreenInit;
				break;
		}
	} else {
		GLINTAccelInit();
		switch (glintInfoRec.bitsPerPixel) {
			case 8:
				ScreenInitFunc = &xf86XAAScreenInit8bpp;
				break;
			case 16:
				ScreenInitFunc = &xf86XAAScreenInit16bpp;
				break;
			case 32:
				ScreenInitFunc = &xf86XAAScreenInit32bpp;
				break;
		}
	}

	if (!ScreenInitFunc(pScreen,
			(pointer) glintVideoMem,
			glintInfoRec.virtualX, glintInfoRec.virtualY,
			displayResolution, displayResolution,
			glintInfoRec.displayWidth))
		return(FALSE);

	pScreen->whitePixel = (Pixel) 1;
	pScreen->blackPixel = (Pixel) 0;
	XF86FLIP_PIXELS();
	pScreen->CloseScreen = glintCloseScreen;
	pScreen->SaveScreen = glintSaveScreen;

	switch(glintInfoRec.bitsPerPixel) {
		case 8:
			pScreen->InstallColormap = glintInstallColormap;
			pScreen->UninstallColormap = glintUninstallColormap;
			pScreen->ListInstalledColormaps = 
						glintListInstalledColormaps;
			pScreen->StoreColors = glintStoreColors;
			break;
		case 16:
		case 32:
			pScreen->InstallColormap = cfbInstallColormap;
			pScreen->UninstallColormap = cfbUninstallColormap;
			pScreen->ListInstalledColormaps = 
						cfbListInstalledColormaps;
			pScreen->StoreColors = (void (*)())NoopDDA;
			break;
	}

	if (OFLG_ISSET(OPTION_IBMRGB_CURS, &glintInfoRec.options)) {
		pScreen->QueryBestSize = glintQueryBestSize;
		xf86PointerScreenFuncs.WarpCursor = glintWarpCursor;
		(void)glintCursorInit(0, pScreen);
	} else 
	miDCInitialize (pScreen, &xf86PointerScreenFuncs);

	savepScreen = pScreen;
	return (cfbCreateDefColormap(pScreen));
}

/*
 *      Assign a new serial number to the window.
 *      Used to force GC validation on VT switch.
 */

/*ARGSUSED*/
static int
glintNewSerialNumber(WindowPtr pWin, pointer data)
{
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    return WT_WALKCHILDREN;
}


/*
 * glintEnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
glintEnterLeaveVT(Bool enter, int screen_idx)
{
    PixmapPtr pspix;
    ScreenPtr pScreen = savepScreen;

    if (!xf86Exiting && !xf86Resetting) {
        switch (glintInfoRec.bitsPerPixel) {
        case 8:
            pspix = (PixmapPtr)pScreen->devPrivate;
            break;
	case 16:
	    pspix = (PixmapPtr)pScreen->devPrivates[cfb16ScreenPrivateIndex].ptr;
	    break;
        case 32:
	    pspix = (PixmapPtr)pScreen->devPrivates[cfb32ScreenPrivateIndex].ptr;
            break;
        }
    }

    if (pScreen && !xf86Exiting && !xf86Resetting)
        WalkTree(pScreen, glintNewSerialNumber, 0);

    if (enter) {
	xf86MapDisplay(screen_idx, LINEAR_REGION);
	if (!xf86Resetting) {
	    ScrnInfoPtr pScr = (ScrnInfoPtr)XF86SCRNINFO(pScreen);

	    glintInit(glintInfoRec.modes);
	    glintInitEnvironment();
	    AlreadyInited = TRUE;
	    glintCalcCRTCRegs(&glintCRTCRegs, glintInfoRec.modes);
	    glintSetCRTCRegs(&glintCRTCRegs);
	    glintRestoreDACvalues();
	    glintRestoreColor0(pScreen);
	    (void)glintCursorInit(0, pScreen);
	    glintRestoreCursor(pScreen);
	    glintAdjustFrame(pScr->frameX0, pScr->frameY0);

	    if (pspix->devPrivate.ptr != glintVideoMem && ppix) {
		pspix->devPrivate.ptr = glintVideoMem;
#if 1
		ppix->devPrivate.ptr = (pointer)glintVideoMem;
#else
		(glintImageWriteFunc)(0, 0, pScreen->width, pScreen->height,
				 ppix->devPrivate.ptr,
				 PixmapBytePad(pScreen->width,
					       pScreen->rootDepth),
				 0, 0, MIX_SRC, ~0);
#endif
	    }
	}
	if (ppix) {
	    (pScreen->DestroyPixmap)(ppix);
	    ppix = NULL;
	}
    } else {
	xf86MapDisplay(screen_idx, LINEAR_REGION);
	if (!xf86Exiting) {
	    ppix = (pScreen->CreatePixmap)(pScreen, glintInfoRec.displayWidth,
					    pScreen->height,
					    pScreen->rootDepth);

	    if (ppix) {
#if 1
		ppix->devPrivate.ptr = (pointer)glintVideoMemSave;
#else
		(glintImageReadFunc)(0, 0, pScreen->width, pScreen->height,
				ppix->devPrivate.ptr,
				PixmapBytePad(pScreen->width,
					      pScreen->rootDepth),
				0, 0, ~0);
#endif
		pspix->devPrivate.ptr = ppix->devPrivate.ptr;
	    }
	}

	xf86InvalidatePixmapCache();

	if (!xf86Resetting) {
		if (AlreadyInited) {
#ifdef XFreeXDGA
	    if (!(glintInfoRec.directMode & XF86DGADirectGraphics))
#endif
		glintCleanUp();
		AlreadyInited = FALSE;
		}
	}
	xf86UnMapDisplay(screen_idx, LINEAR_REGION);
    }
}

/*
 * glintCloseScreen --
 *      called to ensure video is enabled when server exits.
 */

/*ARGSUSED*/
Bool
glintCloseScreen(int screen_idx, ScreenPtr pScreen)
{
    extern void glintClearSavedCursor();

    /*
     * Hmm... The server may shut down even if it is not running on the
     * current vt. Let's catch this case here.
     */
    xf86Exiting = TRUE;
    if (xf86VTSema)
	glintEnterLeaveVT(LEAVE, screen_idx);
    else if (ppix) {
    /* 7-Jan-94 CEG: The server is not running on the current vt.
     * Free the screen snapshot taken when the server vt was left.
     */
	    (savepScreen->DestroyPixmap)(ppix);
	    ppix = NULL;
    }

#ifdef NOTYET
    glintClearSavedCursor(screen_idx);
#endif

    switch (glintInfoRec.bitsPerPixel) {
    case 8:
        cfbCloseScreen(screen_idx, savepScreen);
	break;
    case 16:
	cfb16CloseScreen(screen_idx, savepScreen);
	break;
    case 32:
        cfb32CloseScreen(screen_idx, savepScreen);
	break;
    }

    savepScreen = NULL;
    return(TRUE);
}

/*
 * glintSaveScreen --
 *      blank the screen.
 */
Bool
glintSaveScreen (ScreenPtr pScreen, Bool on)
{
    if (on)
	SetTimeSinceLastInputEvent();

    if (xf86VTSema) {

#ifdef NOTYET
	if (on) {
	    if (glintHWCursorSave != -1) {
		glintSetRamdac(glintCRTCRegs.color_depth, TRUE,
				glintCRTCRegs.dot_clock);
		regwb(GEN_TEST_CNTL, glintHWCursorSave);
		glintHWCursorSave = -1;
	    }
	    glintRestoreColor0(pScreen);
	    if (glintRamdacSubType != DAC_ATI68875)
		outb(ioDAC_REGS+2, 0xff);
	} else {
	    outb(ioDAC_REGS, 0);
	    outb(ioDAC_REGS+1, 0);
	    outb(ioDAC_REGS+1, 0);
	    outb(ioDAC_REGS+1, 0);
	    outb(ioDAC_REGS+2, 0x00);

	    glintSetRamdac(CRTC_PIX_WIDTH_8BPP, TRUE,
			    glintCRTCRegs.dot_clock);
	    glintHWCursorSave = regrb(GEN_TEST_CNTL);
	    regwb(GEN_TEST_CNTL, glintHWCursorSave & ~HWCURSOR_ENABLE);

	    if (glintRamdacSubType != DAC_ATI68875)
		outb(ioDAC_REGS+2, 0x00);
	}
#endif
    }

    return(TRUE);
}

/*
 * glintDPMSSet -- Sets VESA Display Power Management Signaling (DPMS) Mode
 *
 * Only the Off and On modes are currently supported.
 */

void
glintDPMSSet(int PowerManagementMode)
{
#if 0
#ifdef DPMSExtension
    int crtcGenCntl;
    if (!xf86VTSema) return;
#if 0
    crtcGenCntl = GLINT_READ_REG(GLINT_VALID_REG);
#endif
    switch (PowerManagementMode)
    {
    case DPMSModeOn:
	/* HSync: On, VSync: On */
	crtcGenCntl |= 0x0001;
	break;
    case DPMSModeStandby:
	/* HSync: Off, VSync: On -- Not Supported */
	break;
    case DPMSModeSuspend:
	/* HSync: On, VSync: Off -- Not Supported */
	break;
    case DPMSModeOff:
	/* HSync: Off, VSync: Off */
	crtcGenCntl &= 0xFFFE;
	break;
    }
    usleep(10000);
#if 0
    GLINT_WRITE_REG(crtcGenCntl, GLINT_VALID_REG);
#endif
#endif
#endif
}

/*
 * glintAdjustFrame --
 *      Modify the CRT_OFFSET for panning the display.
 */
void
glintAdjustFrame(x, y)
    int x, y;
{
	GLINTWindowBase = y * glintInfoRec.displayWidth + x;
	GLINT_WRITE_REG(GLINTWindowBase, FBWindowBase);
}

/*
 * glintSwitchMode --
 *      Reinitialize the CRTC registers for the new mode.
 */
Bool
glintSwitchMode(mode)
    DisplayModePtr mode;
{
#if 0
    glintCalcCRTCRegs(&glintCRTCRegs, mode);
    glintSetCRTCRegs(&glintCRTCRegs);

    return(TRUE);
#else
    return(FALSE);
#endif
}

/*
 * glintValidMode --
 *
 */
static int
glintValidMode(DisplayModePtr mode, Bool verbose, int flag)
{
    if (mode->Flags & V_INTERLACE)
    {
	ErrorF("%s %s: Cannot support interlaced modes, deleting.\n",
			XCONFIG_GIVEN, glintInfoRec.name);
	return MODE_BAD;
    }
    return MODE_OK;
}
