/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint.c,v 1.24 1998/03/20 21:05:31 hohndel Exp $ */
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
#include "xf86cursor.h"
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
    glintProbe,      	/* Bool (* Probe)() */
    FALSE,		/* Bool configured */
    -1,			/* int tmpIndex */
    -1,			/* int scrnIndex */
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
    0xA0000,		/* unsigned long VGAbase */
    40000,		/* int s3RefClk */
    -1,			/* int s3BlankDelay */
    0,			/* int textClockFreq */
    NULL,               /* char* DCConfig */
    NULL,               /* char* DCOptions */
    0,			/* int MemClk */
    0,			/* int busType */
    0,			/* PCITAG pciTag */
#ifdef XFreeXDGA
    0,			/* int directMode */
    NULL,		/* Set Vid Page */
    0,			/* unsigned long physBase */
    0,			/* int physSize */
#endif
#ifdef XF86SETUP
    NULL,		/* void *device */
#endif
    FALSE		/* hasDirectColor */
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
			* data = (pointer) "xaa24.o";
			* magic= MAGIC_LOAD;
			break;
		case 6:
			* data = (pointer) "xaa32.o";
			* magic= MAGIC_LOAD;
			break;
		case 7:
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

extern void XAAQueryBestSize();
extern void XAAWarpCursor();
extern void PM2DACShowCursor();
extern void PM2DACHideCursor();
extern void PM2DACSetCursorPosition();
extern void PM2DACSetCursorColors();
extern void PM2DACLoadCursorImage();
extern void glintIBMShowCursor();
extern void glintIBMHideCursor();
extern void glintIBMSetCursorPosition();
extern void glintIBMSetCursorColors();
extern void glintIBMLoadCursorImage();
extern int glintIBMRGB_Probe();

extern int Shiftbpp();
Bool glintDoubleBufferMode = FALSE;
extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern Bool xf86Exiting, xf86Resetting;
extern int VBlank;
extern int pprod;
extern int partprod500TX[];
extern int partprodPermedia[];
Bool VGAcore;
ScreenPtr savepScreen = NULL;
Bool glintReloadCursor, glintBlockCursor;
unsigned char glintSwapBits[256];
pointer glint_reg_base;
int glinthotX, glinthotY;
static PixmapPtr ppix = NULL;
int glintDisplayWidth;
volatile pointer glintVideoMem = NULL;
volatile pointer GLINTMMIOBase;
Bool UsePCIRetry = FALSE;

Bool AlreadyInited;
int coprotype = -1;
int glintLBBase;
int glintLBvideoRam;
int GLINTFrameBufferSize, GLINTLocalBufferSize;
int GLINTWindowBase;
int glintAdjustCursorXPos = 0;
static glintCRTCRegRec glintCRTCRegs;
extern int defaultColorVisualClass;
extern void PermediaSaveVGAInfo();
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
 * glintProbe --
 *      check up whether a GLINT based board is installed
 */

Bool
glintProbe()
{
  int i, ibm_id;
  int tx, ty;
  int temp;
  DisplayModePtr pMode, pEnd;
  OFlagSet validOptions;
  pciConfigPtr pcrp = NULL;
  pciConfigPtr pcrpglint = NULL;
  pciConfigPtr pcrpdelta = NULL;
  pciConfigPtr *pcrpp;
  unsigned long glintdelta = 0;
  unsigned long glintcopro = 0;
  unsigned long basecopro = 0;
  unsigned long base3copro = 0;
  unsigned long basedelta = 0;
  unsigned long *delta_pci_basep = 0;
  int cardnum = -1;
  int offset;

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
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_300SX;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found GLINT 300SX at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_500TX:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_500TX;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found GLINT 500TX at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_MX:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_MX;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found GLINT MX at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_PERMEDIA:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_PERMEDIA;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found GLINT Permedia at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_PERMEDIA2:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_PERMEDIA2;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found GLINT Permedia 2 at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
	case PCI_CHIP_3DLABS_DELTA:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basedelta = pcrp->_base0;
		delta_pci_basep = &(pcrp->_base0);
		pcrpdelta = pcrp;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			coprotype = -1;
			glintcopro = 0;
			pcrpglint = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found GLINT Delta at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basedelta);
		}
		break;
	}
    }
    else if (pcrp->_vendor == PCI_VENDOR_TI) /* TI is producing the PM2 */
    {
        switch (pcrp->_device)
	{
	case PCI_CHIP_TI_PERMEDIA:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_3DLABS_PERMEDIA;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found TI chip GLINT Permedia at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
	case PCI_CHIP_TI_PERMEDIA2:
		glintcopro = PCI_MAKE_TAG(pcrp->_bus, pcrp->_cardnum, pcrp->_func);
		basecopro = pcrp->_base0;
		pcrpglint = pcrp;
		coprotype = PCI_CHIP_TI_PERMEDIA2;
		if( cardnum == -1 )
			cardnum = pcrp->_cardnum;
		else if( cardnum != pcrp->_cardnum )
		{
			ErrorF("%s %s: found second board based on GLINT "
			       "will use information from there\n",
			       XCONFIG_PROBED, glintInfoRec.name);
			glintdelta = 0;
			pcrpdelta = NULL;
			cardnum = pcrp->_cardnum;
		}
		if( xf86Verbose ) 
		{
			ErrorF("%s %s: found TI chip GLINT Permedia 2 at card #%d func #%d with "
			       "base 0x%x\n",XCONFIG_PROBED, glintInfoRec.name,
			       pcrp->_cardnum,pcrp->_func,basecopro);
		}
		break;
    	}
    }
  }

  if (pcrpglint == NULL)
    FatalError("No GLINT/PERMEDIA based card found\n");

  /*
   * next, we should enable memory and I/O on the card, just to be sure that 
   * the BIOS didn't try to be smart and disabled that for anything except 
   * the first VGA card (which would be the ViRGE chip here)
   */
  xf86writepci(glintInfoRec.scrnIndex, pcrpglint->_bus, pcrpglint->_cardnum,
  	       pcrpglint->_func, PCI_CMD_STAT_REG, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE);
  if (glintdelta) {
      xf86writepci(glintInfoRec.scrnIndex, pcrpdelta->_bus, 
  	       pcrpdelta->_cardnum,
  	       pcrpdelta->_func, PCI_CMD_STAT_REG, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE, 
	       PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE);
  }

  /*
   * due to a few bugs in the GLINT Delta we might have to relocate
   * the base address of config region of the Delta, if bit 17 of
   * the base addresses of config region of the Delta and the 500TX
   * or 300SX are different
   * We only handle config type 1 at this point
   */
  if( glintdelta && glintcopro)
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
	xf86EnableIOPorts(glintInfoRec.scrnIndex);
 	if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
 		offset = 0x20; /* base4 */
         } else {
 		offset = 0x1c; /* base3 */
 	}
	base3copro = pciReadLong(glintcopro, offset);
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
	temp = pciReadLong(glintdelta, 0x10);
	pciWriteLong(glintdelta, 0x10, 0xffffffff);
	temp = pciReadLong(glintdelta, 0x10);
	pciWriteLong(glintdelta, 0x10, base3copro);

	/*
	 * additionally, sometimes we see the baserom which might
	 * confuse the chip, so let's make sure that is disabled
	 */
	temp = pciReadLong(glintcopro, 0x30);
	pciWriteLong(glintcopro, 0x30, 0xffffffff);
	temp = pciReadLong(glintcopro, 0x30);
	pciWriteLong(glintcopro, 0x30, 0);

	/*
	 * now update our internal structure accordingly
	 */
	*delta_pci_basep = base3copro;
        xf86DisableIOPorts(glintInfoRec.scrnIndex);
    }
  }

  if (glintcopro) {
    unsigned long temp2;
    
    xf86EnableIOPorts(glintInfoRec.scrnIndex);

    temp = pciReadLong(glintcopro, 0x04);
    pciWriteLong(glintcopro, 0x04, temp | 0x04); /* Master enable */
    temp = pciReadLong(glintcopro, 0x04);

    /*
     * and now for the magic.
     * read old value
     * write fffffffff
     * read value
     * print size
     * write old value
     */
    if (glintdelta) {
	temp = pciReadLong(glintdelta, 0x10);
	pciWriteLong(glintdelta, 0x10, 0xffffffff);
	temp2 = pciReadLong(glintdelta, 0x10);
#ifdef DEBUG
	ErrorF("Delta - Control Register size: 0x%08x\n", ~temp2);
#endif
	pciWriteLong(glintdelta, 0x10, temp & 0xfffffff0);
    }
    
    temp = pciReadLong(glintcopro, 0x10);
    pciWriteLong(glintcopro, 0x10, 0xffffffff);
    temp2 = pciReadLong(glintcopro, 0x10);
#ifdef DEBUG
    ErrorF("500TX - Control Register size: 0x%08x\n", ~temp2);
#endif
    pciWriteLong(glintcopro, 0x10, temp);
    
    temp = pciReadLong(glintcopro, 0x14);
    pciWriteLong(glintcopro, 0x14, 0xffffffff);
    temp2 = pciReadLong(glintcopro, 0x14);
#ifdef DEBUG
    ErrorF("500TX - Localbuffer0 size: 0x%08x\n", ~temp2);
#endif
    pciWriteLong(glintcopro, 0x14, temp);
    
    temp = pciReadLong(glintcopro, 0x18);
    pciWriteLong(glintcopro, 0x18, 0xffffffff);
    temp2 = pciReadLong(glintcopro, 0x18);
#ifdef DEBUG
    ErrorF("500TX - Framebuffer0 size: 0x%08x\n", ~temp2);
#endif
    pciWriteLong(glintcopro, 0x18, temp);
    
    temp = pciReadLong(glintcopro, 0x1c);
    pciWriteLong(glintcopro, 0x1c, 0xffffffff);
    temp2 = pciReadLong(glintcopro, 0x1c);
#ifdef DEBUG
    ErrorF("500TX - Localbuffer1 size: 0x%08x\n", ~temp2);
#endif 
    pciWriteLong(glintcopro, 0x1c, temp);
   
    temp = pciReadLong(glintcopro, 0x20);
    pciWriteLong(glintcopro, 0x20, 0xffffffff);
    temp2 = pciReadLong(glintcopro, 0x20);
#ifdef DEBUG
    ErrorF("500TX - Framebuffer1 size: 0x%08x\n", ~temp2);
#endif
    pciWriteLong(glintcopro, 0x20, temp);
    
    xf86DisableIOPorts(glintInfoRec.scrnIndex);
  }
  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	  GLINTMMIOBase = xf86MapVidMem(0,MMIO_REGION,
  				(pointer)pcrpdelta->_base0,0x20000);
	  glintLBBase = pcrpglint->_base1;
	  ErrorF("%s %s: Localbuffer address at 0x%x\n",XCONFIG_PROBED,
			glintInfoRec.name, glintLBBase);
	  glintInfoRec.MemBase = pcrpglint->_base2;
  } else if (IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
      GLINTMMIOBase = xf86MapVidMem(0, MMIO_REGION, (pointer)pcrpdelta->_base0,
				    0x40000);
      glintLBBase = 0; /* no local buffer on PerMedia cards */
      glintInfoRec.MemBase = pcrpglint->_base2;
  } else if (IS_3DLABS_PM2_CLASS(coprotype)) {
      GLINTMMIOBase = xf86MapVidMem(0, MMIO_REGION, (pointer)pcrpglint->_base0,
                                    0x40000);
      glintLBBase = 0; /* no local buffer on PerMedia cards */
      glintInfoRec.MemBase = pcrpglint->_base2;
  }
  ErrorF("%s %s: Framebuffer address at 0x%x\n",XCONFIG_PROBED,
		glintInfoRec.name, glintInfoRec.MemBase);

  if (!glintInfoRec.videoRam) {
  	if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
		GLINTFrameBufferSize = GLINT_READ_REG(FBMemoryCtl);
		glintInfoRec.videoRam = 1024 * (1 << ((GLINTFrameBufferSize &
						0xE0000000) >> 29));
	} else if (IS_3DLABS_PM_FAMILY(coprotype)) {
	  GLINTFrameBufferSize = GLINT_READ_REG(PMMemConfig);
	  /* ErrorF("Memconfig register 0x%x\n", GLINTFrameBufferSize); */
	  glintInfoRec.videoRam = 2048 * (((GLINTFrameBufferSize >> 29) & 0x03) + 1);
	}
	ErrorF("%s %s: videoram : %dk\n", XCONFIG_PROBED, 
		glintInfoRec.name, glintInfoRec.videoRam);
  }
  else
  {
    ErrorF("%s %s: videoram : %dk\n", XCONFIG_GIVEN, glintInfoRec.name,
	   glintInfoRec.videoRam);
  }

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	  GLINTLocalBufferSize = GLINT_READ_REG(LBMemoryCtl);
	  glintLBvideoRam = 1024 * 
	  		    (1 << ((GLINTLocalBufferSize & 0x07000000) >> 24));
	  ErrorF("%s %s: localbuffer ram : %dk\n", XCONFIG_PROBED, 
	  				glintInfoRec.name, glintLBvideoRam);
  }

  if (IS_3DLABS_TX_MX_CLASS(coprotype) ||
      IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
        /*
	 * TX/MX and Permedia are equipped with the IBM RAMDAC
	 */
	ibm_id = glintIBMRGB_Probe();
	if (ibm_id) {
	    ErrorF("%s %s: ", XCONFIG_PROBED, glintInfoRec.name);
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
		case 0x121C:
			ErrorF("Detected IBM 640 Ramdac\n");
			break;
		default:
			ErrorF("Detected unknown Ramdac with id 0x%x\n");
	    }
	}
	else {
	    ErrorF("%s %s: GLINT TX/MX and Permedia are only supported with\n",
	           XCONFIG_PROBED,glintInfoRec.name);
	    ErrorF("%s %s: IBM RGB 526,526DB,524,624 or 640 RAMDAC\n",
	           XCONFIG_PROBED,glintInfoRec.name);
	    return (FALSE);
	}
      }
  if (IS_3DLABS_PM2_CLASS(coprotype)) {
  	/*
	 * PM2 has a builtin RAMDAC
	 */
	ErrorF("%s %s: Using builtin RAMDAC of Permedia 2 chip\n",
	           XCONFIG_PROBED,glintInfoRec.name);
  }
  if (IS_3DLABS_PM_FAMILY(coprotype)) {
	ErrorF("%s %s: Fitted Memory type is : %s\n", XCONFIG_PROBED,
		glintInfoRec.name, GLINT_READ_REG(PMRomControl) & 0x10 ? 
					"SDRAM" : "SGRAM");
	ErrorF("%s %s: VGA core is : %s\n", XCONFIG_PROBED,
		glintInfoRec.name, GLINT_READ_REG(ChipConfig) & 0x2 ?
					"Enabled" : "Disabled");
	if (GLINT_READ_REG(ChipConfig) & 0x2) {
		VGAcore = TRUE;
	} else {
		VGAcore = FALSE;
	}
  }
#if DEBUG
	if( xf86Verbose > 3)					
	  glintDumpRegs();
#endif

  OFLG_ZERO(&validOptions);

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) 
    OFLG_SET(OPTION_FIREGL3000, &validOptions);

  OFLG_SET(OPTION_XAA_BENCHMARK, &validOptions);
  OFLG_SET(OPTION_BLOCK_WRITE, &validOptions);
  OFLG_SET(OPTION_NO_PIXMAP_CACHE, &validOptions);
  OFLG_SET(OPTION_NOACCEL, &validOptions);
  OFLG_SET(OPTION_PCI_RETRY, &validOptions);
  OFLG_SET(OPTION_XAA_NO_COL_EXP, &validOptions);
  OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &validOptions);
  OFLG_SET(OPTION_SW_CURSOR, &validOptions);
  OFLG_SET(OPTION_FIREGL3000, &validOptions);

  OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &glintInfoRec.clockOptions);

  if (OFLG_ISSET(OPTION_PCI_RETRY, &glintInfoRec.options))
	UsePCIRetry = TRUE;

  xf86VerifyOptions(&validOptions, &glintInfoRec);
  glintInfoRec.chipset = "glint";
  xf86ProbeFailed = FALSE;

  if (xf86bpp < 0)
	xf86bpp = glintInfoRec.depth;

  if (xf86weight.red == 0 || xf86weight.green == 0 || xf86weight.blue == 0)
	xf86weight = glintInfoRec.weight;

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	if (xf86bpp == 16) {
		ErrorF("%s %s: Glint 500TX/MX does not support 16bpp. "
		   "Using 15bpp mode.\n", XCONFIG_GIVEN, glintInfoRec.name);
		xf86bpp = 15;
	}
	if (xf86bpp == 24) {
		ErrorF("%s %s: Glint 500TX/MX does not support 24bpp. "
		   "Using 32bpp mode.\n", XCONFIG_GIVEN, glintInfoRec.name);
		xf86bpp = 32;
	}
  }

  if (IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
	if (xf86bpp == 24) {
	  	ErrorF("%s %s: Permedia does not support 24bpp. "
	     	   "Using 32bpp mode.\n", XCONFIG_GIVEN, glintInfoRec.name);
	  	xf86bpp = 32;
	}
  }

  switch (xf86bpp) {
	case 8:
		/* XAA uses this */
		xf86weight.green = 8;
		break;
	case 15:
		glintInfoRec.depth = 15;
		xf86weight.green = xf86weight.red = xf86weight.blue = 5;
		glintInfoRec.bitsPerPixel = 16;
		if (glintInfoRec.defaultVisual < 0)
			glintInfoRec.defaultVisual = TrueColor;
		if (defaultColorVisualClass < 0)
			defaultColorVisualClass = glintInfoRec.defaultVisual;
		break;
	case 16:
		glintInfoRec.depth = 16;
		xf86weight.green = 6;
		xf86weight.red = xf86weight.blue = 5;
		glintInfoRec.bitsPerPixel = 16;
		if (glintInfoRec.defaultVisual < 0)
			glintInfoRec.defaultVisual = TrueColor;
		if (defaultColorVisualClass < 0)
			defaultColorVisualClass = glintInfoRec.defaultVisual;
		break;
	case 24:
		glintInfoRec.bitsPerPixel = 24;
		glintInfoRec.depth = 24;
		xf86weight.red = xf86weight.blue = xf86weight.green = 8;
		if (glintInfoRec.defaultVisual < 0)
			glintInfoRec.defaultVisual = TrueColor;
		if (defaultColorVisualClass < 0)
			defaultColorVisualClass = glintInfoRec.defaultVisual;
		OFLG_SET(OPTION_NOACCEL, &glintInfoRec.options);
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

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	if (!glintInfoRec.dacSpeeds[0])
		glintInfoRec.dacSpeeds[0] = 220000; 
	if (!glintInfoRec.dacSpeeds[1])
		glintInfoRec.dacSpeeds[1] = 220000; 
	if (!glintInfoRec.dacSpeeds[2])
		glintInfoRec.dacSpeeds[2] = 220000; /* Never used */
	if (!glintInfoRec.dacSpeeds[3])
		glintInfoRec.dacSpeeds[3] = 220000; 
  }
  else if (IS_3DLABS_PERMEDIA_CLASS(coprotype)){
	if (!glintInfoRec.dacSpeeds[0])
		glintInfoRec.dacSpeeds[0] = 200000; 
	if (!glintInfoRec.dacSpeeds[1])
		glintInfoRec.dacSpeeds[1] = 100000; 
	if (!glintInfoRec.dacSpeeds[2])
		glintInfoRec.dacSpeeds[2] = 50000;
	if (!glintInfoRec.dacSpeeds[3])
		glintInfoRec.dacSpeeds[3] = 50000; 
  }
  else if (IS_3DLABS_PM2_CLASS(coprotype)){
	if (!glintInfoRec.dacSpeeds[0])
		glintInfoRec.dacSpeeds[0] = 230000; 
	if (!glintInfoRec.dacSpeeds[1])
		glintInfoRec.dacSpeeds[1] = 170000; 
	if (!glintInfoRec.dacSpeeds[2])
		glintInfoRec.dacSpeeds[2] = 110000; 
	if (!glintInfoRec.dacSpeeds[3])
		glintInfoRec.dacSpeeds[3] = 110000; 
  }

  glintInfoRec.maxClock = glintInfoRec.dacSpeeds
					[(glintInfoRec.bitsPerPixel/8)-1];

  /* Let's grab the basic mode lines */
  tx = glintInfoRec.virtualX;
  ty = glintInfoRec.virtualY;

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
  	if (glintInfoRec.virtualX > 0) {
		ErrorF("%s %s: Virtual coordinates - Not supported. "
		   "Ignoring.\n", XCONFIG_GIVEN, glintInfoRec.name);
  	}
  }

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
	} else if (((tx > 0) && (pMode->HDisplay > tx)) ||
		   ((ty > 0) && (pMode->VDisplay > ty))) {
		ErrorF("%s %s: Resolution %dx%d too large for virtual %dx%d\n",
			XCONFIG_PROBED, glintInfoRec.name,
			pMode->HDisplay, pMode->VDisplay, tx, ty);
		xf86DeleteMode(&glintInfoRec, pMode);
	} else {
		if (pEnd == (DisplayModePtr) NULL)
			pEnd = pMode;

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
			glintInfoRec.virtualX = max(glintInfoRec.virtualX, 
							pMode->HDisplay);
			glintInfoRec.virtualY = max(glintInfoRec.virtualY, 
							pMode->VDisplay);
			if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
				pMode = pEnd; /*  only one mode supported */
			}
		}
	}
	if (IS_3DLABS_PM_FAMILY(coprotype)) {
		pMode = pModeSv;
	}
  } while (pMode != pEnd); 

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
  	ErrorF("%s %s: 3Dlabs TX/MX only supports one modeline.\n",
		XCONFIG_PROBED, glintInfoRec.name);
  }

  glintInfoRec.displayWidth = glintInfoRec.virtualX;

  if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	pprod = partprod500TX[glintInfoRec.displayWidth >> 5];
  } else {
	pprod = partprodPermedia[glintInfoRec.displayWidth >> 5];
  }

  if (pprod == -1) {
	/*
	 * Houston, we have a problem
	 * 
	 * we need to figure out how to handle pitches we
	 * can't handle. Later, comma, much.
	 */
	FatalError("Can't handle pitch %d\n", glintInfoRec.displayWidth);
  }

#ifdef XFreeXDGA
  glintInfoRec.directMode = XF86DGADirectPresent;
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
        xf86EnableIOPorts(scr_index);
	glintInitAperture(scr_index);
	saveGLINTstate();
	if (IS_3DLABS_PM_FAMILY(coprotype)) {
		if (VGAcore) 
			PermediaSaveVGAInfo();
	}
	glintInit(glintInfoRec.modes);
	glintCalcCRTCRegs(&glintCRTCRegs, glintInfoRec.modes);
	glintSetCRTCRegs(&glintCRTCRegs);
	glintInitEnvironment();
	AlreadyInited = TRUE;

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
	
	xf86AccelInfoRec.ServerInfoRec = &glintInfoRec;

	if (OFLG_ISSET(OPTION_NOACCEL, &glintInfoRec.options)) {
		OFLG_SET(OPTION_SW_CURSOR, &glintInfoRec.options);
	} else {
  		if (IS_3DLABS_TX_MX_CLASS(coprotype)) 
			GLINTAccelInit();
		else
  		if (IS_3DLABS_PERMEDIA_CLASS(coprotype))
			PermediaAccelInit();
		else
  		if (IS_3DLABS_PM2_CLASS(coprotype)) {
#if 1
			OFLG_SET(OPTION_SW_CURSOR, &glintInfoRec.options);
#endif
			Permedia2AccelInit();
		}
	}

	switch (glintInfoRec.depth) {
		case 8:
			ScreenInitFunc = &xf86XAAScreenInit8bpp;
			break;
		case 15: case 16:
			ScreenInitFunc = &xf86XAAScreenInit16bpp;
			break;
		case 24:
			ScreenInitFunc = &xf86XAAScreenInit24bpp;
			break;
		case 32:
			ScreenInitFunc = &xf86XAAScreenInit32bpp;
			break;
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

	switch(glintInfoRec.bitsPerPixel) {
		case 8:
			pScreen->InstallColormap = glintInstallColormap;
			pScreen->UninstallColormap = glintUninstallColormap;
			pScreen->ListInstalledColormaps = 
						glintListInstalledColormaps;
			pScreen->StoreColors = glintStoreColors;
			break;
		case 16:
		case 24:
		case 32:
			pScreen->InstallColormap = cfbInstallColormap;
			pScreen->UninstallColormap = cfbUninstallColormap;
			pScreen->ListInstalledColormaps = 
						cfbListInstalledColormaps;
			pScreen->StoreColors = (void (*)())NoopDDA;
			break;
	}

	if (!OFLG_ISSET(OPTION_SW_CURSOR, &glintInfoRec.options)) {
	   if (IS_3DLABS_PM2_CLASS(coprotype)) {
		XAACursorInfoRec.MaxHeight = 64;
		XAACursorInfoRec.MaxWidth = 64;
		XAACursorInfoRec.Flags = USE_HARDWARE_CURSOR |
					 HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
					 HARDWARE_CURSOR_CHAR_BIT_FORMAT |
					 HARDWARE_CURSOR_PROGRAMMED_BITS;
		XAACursorInfoRec.ShowCursor = PM2DACShowCursor;
		XAACursorInfoRec.HideCursor = PM2DACHideCursor;
		XAACursorInfoRec.SetCursorColors = PM2DACSetCursorColors;
		XAACursorInfoRec.SetCursorPosition = PM2DACSetCursorPosition;
		XAACursorInfoRec.LoadCursorImage = PM2DACLoadCursorImage;
		pScreen->QueryBestSize = XAAQueryBestSize;
		xf86PointerScreenFuncs.WarpCursor = XAAWarpCursor;
		(void)XAACursorInit(0, pScreen);
	    } else {
		XAACursorInfoRec.MaxHeight = 64;
		XAACursorInfoRec.MaxWidth = 64;
		XAACursorInfoRec.Flags = USE_HARDWARE_CURSOR |
					 HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
					 HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
					 HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE |
					 HARDWARE_CURSOR_CHAR_BIT_FORMAT |
					 HARDWARE_CURSOR_PROGRAMMED_BITS |
					 HARDWARE_CURSOR_PROGRAMMED_ORIGIN;
		XAACursorInfoRec.ShowCursor = glintIBMShowCursor;
		XAACursorInfoRec.HideCursor = glintIBMHideCursor;
		XAACursorInfoRec.SetCursorColors = glintIBMSetCursorColors;
		XAACursorInfoRec.SetCursorPosition = glintIBMSetCursorPosition;
		XAACursorInfoRec.LoadCursorImage = glintIBMLoadCursorImage;
		pScreen->QueryBestSize = XAAQueryBestSize;
		xf86PointerScreenFuncs.WarpCursor = XAAWarpCursor;
		(void)XAACursorInit(0, pScreen);
	    }
	} else 
	miDCInitialize (pScreen, &xf86PointerScreenFuncs);

	pScreen->CloseScreen = glintCloseScreen;
	pScreen->SaveScreen = glintSaveScreen;

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
  BoxRec pixBox;
  RegionRec pixReg;
  DDXPointRec pixPt;
  PixmapPtr pspix;
  ScreenPtr pScreen = savepScreen;

    if (!xf86Exiting && !xf86Resetting) {
	pixBox.x1 = 0; pixBox.x2 = pScreen->width;
	pixBox.y1 = 0; pixBox.y2 = pScreen->height;
	pixPt.x = 0; pixPt.y = 0;
	(pScreen->RegionInit)(&pixReg, &pixBox, 1);
        switch (glintInfoRec.bitsPerPixel) {
        case 8:
            pspix = (PixmapPtr)pScreen->devPrivate;
            break;
	case 16:
	    pspix = (PixmapPtr)pScreen->devPrivates[cfb16ScreenPrivateIndex].ptr;
	    break;
        case 24:
	    pspix = (PixmapPtr)pScreen->devPrivates[cfb24ScreenPrivateIndex].ptr;
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
	    xf86EnableIOPorts(screen_idx);
	    if (!AlreadyInited) {
	      saveGLINTstate();
	      if (IS_3DLABS_PM_FAMILY(coprotype)) {
		if (VGAcore)
			PermediaSaveVGAInfo();
	      }
	    }
	    glintInit(glintInfoRec.modes);
	    glintCalcCRTCRegs(&glintCRTCRegs, glintInfoRec.modes);
	    glintSetCRTCRegs(&glintCRTCRegs);
	    glintInitEnvironment();
	    AlreadyInited = TRUE;

	    glintRestoreDACvalues();
	    glintRestoreColor0(pScreen);

    	    if (!OFLG_ISSET(OPTION_SW_CURSOR, &glintInfoRec.options))
	    	XAARestoreCursor(pScreen);
	    glintAdjustFrame(pScr->frameX0, pScr->frameY0);

	    if (pspix->devPrivate.ptr != glintVideoMem && ppix) {
		pspix->devPrivate.ptr = glintVideoMem;

		xf86AccelInfoRec.ImageWriteFallBack(&ppix->drawable,
			&pspix->drawable, GXcopy, &pixReg, &pixPt, ~0);
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
		xf86AccelInfoRec.ImageWriteFallBack(&pspix->drawable,
			&ppix->drawable, GXcopy, &pixReg, &pixPt, ~0);

		pspix->devPrivate.ptr = ppix->devPrivate.ptr;
	    }
	}
#ifdef XFreeXDGA
    if (glintInfoRec.directMode & XF86DGADirectGraphics) {
	if (!OFLG_ISSET(OPTION_SW_CURSOR, &glintInfoRec.options))
		XAACursorInfoRec.HideCursor();
    }
#endif

	xf86InvalidatePixmapCache();

	if (!xf86Resetting) {
#ifdef XFreeXDGA
	    if (!(glintInfoRec.directMode & XF86DGADirectGraphics)) {
#endif
		if (AlreadyInited) {
		    glintCleanUp();
		    xf86DisableIOPorts(screen_idx);
		    AlreadyInited = FALSE;
		}
#ifdef XFreeXDGA
	    }
#endif
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
    case 24:
        cfb24CloseScreen(screen_idx, savepScreen);
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
	if (IS_3DLABS_PM_FAMILY(coprotype)) {
		GLINT_SLOW_WRITE_REG(Shiftbpp((y*glintInfoRec.displayWidth+x)>>1), PMScreenBase);
	}
#ifdef XFreeXDGA
	if (glintInfoRec.directMode & XF86DGADirectGraphics) {
		while ( (GLINT_READ_REG(VTGVLineNumber) >= 1) &&
			(GLINT_READ_REG(VTGVLineNumber) <= VBlank) );
	}
#endif
}

/*
 * glintSwitchMode --
 *      Reinitialize the CRTC registers for the new mode.
 */
Bool
glintSwitchMode(mode)
    DisplayModePtr mode;
{
    xf86MapDisplay(glintInfoRec.scrnIndex, LINEAR_REGION);

    glintCalcCRTCRegs(&glintCRTCRegs, mode);
    glintSetCRTCRegs(&glintCRTCRegs);

    if (!OFLG_ISSET(OPTION_SW_CURSOR, &glintInfoRec.options))
    	XAACursorInfoRec.ShowCursor();

    return(TRUE);
}

/*
 * glintValidMode --
 *
 */
static int
glintValidMode(DisplayModePtr mode, Bool verbose, int flag)
{
  if (mode->Flags & V_INTERLACE) {
      ErrorF("%s %s: Cannot support interlaced modes, deleting.\n",
	     XCONFIG_GIVEN, glintInfoRec.name);
      return MODE_BAD;
  }

  if (IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
    if (mode->CrtcHDisplay > 1536) {
        ErrorF("HDisplay is %d, cannot support HDisplay > 1536, deleting.\n",
	     mode->CrtcHDisplay);
        return MODE_BAD;
    }
  }

  return MODE_OK;
}
