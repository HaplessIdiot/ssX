/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830_driver.c,v 1.17 2002/10/16 21:13:47 dawes Exp $ */
/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.
Copyright © 2002 by David Dawes

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Reformatted with GNU indent (2.2.8), using the following options:
 *
 *    -bad -bap -c41 -cd0 -ncdb -ci6 -cli0 -cp0 -ncs -d0 -di3 -i3 -ip3 -l78
 *    -lp -npcs -psl -sob -ss -br -ce -sc -hnl
 *
 * This provides a good match with the original i810 code and preferred
 * XFree86 formatting conventions.
 *
 * When editing this driver, please follow the existing formatting, and edit
 * with <TAB> characters expanded at 8-column intervals.
 */

/*
 * Authors: Jeff Hartmann <jhartmann@valinux.com>
 *          Abraham van der Merwe <abraham@2d3d.co.za>
 *          David Dawes <dawes@tungstengraphics.com>
 */

/*
 * Mode handling is based on the VESA driver written by:
 * Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 */

/*
 * Changes:
 *
 *    23/08/2001 Abraham van der Merwe <abraham@2d3d.co.za>
 *        - Fixed display timing bug (mode information for some
 *          modes were not initialized correctly)
 *        - Added workarounds for GTT corruptions (I don't adjust
 *          the pitches for 1280x and 1600x modes so we don't
 *          need extra memory)
 *        - The code will now default to 60Hz if LFP is connected
 *        - Added different refresh rate setting code to work
 *          around 0x4f02 BIOS bug
 *        - BIOS workaround for some mode sets (I use legacy BIOS
 *          calls for setting those)
 *        - Removed 0x4f04, 0x01 (save state) BIOS call which causes
 *          LFP to malfunction (do some house keeping and restore
 *          modes ourselves instead - not perfect, but at least the
 *          LFP is working now)
 *        - Several other smaller bug fixes
 *
 *    06/09/2001 Abraham van der Merwe <abraham@2d3d.co.za>
 *        - Preliminary local memory support (works without agpgart)
 *        - DGA fixes (the code were still using i810 mode sets, etc.)
 *        - agpgart fixes
 *
 *    18/09/2001
 *        - Proper local memory support (should work correctly now
 *          with/without agpgart module)
 *        - more agpgart fixes
 *        - got rid of incorrect GTT adjustments
 *
 *    09/10/2001
 *        - Changed the DPRINTF() variadic macro to an ANSI C compatible
 *          version
 *
 *    10/10/2001
 *        - Fixed DPRINTF_stub(). I forgot the __...__ macros in there
 *          instead of the function arguments :P
 *        - Added a workaround for the 1600x1200 bug (Text mode corrupts
 *          when you exit from any 1600x1200 mode and 1280x1024@85Hz. I
 *          suspect this is a BIOS bug (hence the 1280x1024@85Hz case)).
 *          For now I'm switching to 800x600@60Hz then to 80x25 text mode
 *          and then restoring the registers - very ugly indeed.
 *
 *    15/10/2001
 *        - Improved 1600x1200 mode set workaround. The previous workaround
 *          was causing mode set problems later on.
 *
 *    18/10/2001
 *        - Fixed a bug in I830BIOSLeaveVT() which caused a bug when you
 *          switched VT's
 */
/*
 *    07/2002 David Dawes
 *        - Cleanup code formatting.
 *        - Improve VESA mode selection, and fix refresh rate selection.
 *        - Don't duplicate functions provided in 4.2 vbe modules.
 *        - Don't duplicate functions provided in the vgahw module.
 *        - Rewrite memory allocation.
 *        - Rewrite initialisation and save/restore state handling.
 *        - Decouple the i810 support from i830 and later.
 *        - Remove various unnecessary hacks and workarounds.
 *        - Fix an 845G problem with the ring buffer not in pre-allocated
 *          memory.
 *        - Fix screen blanking.
 *        - Clear the screen at startup so you don't see the previous session.
 *        - Fix some HW cursor glitches, and turn HW cursor off at VT switch
 *          and exit.
 *
 *    08/2002 Keith Whitwell
 *        - Fix DRI initialisation.
 */
/*
 *    08/2002 Alan Hourihane and David Dawes
 *        - Add XVideo support.
 */

#define DEBUG

#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86RAC.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "mibstore.h"
#include "vgaHW.h"
#include "mipointer.h"
#include "micmap.h"

#include "fb.h"
#include "miscstruct.h"
#include "xf86xv.h"
#include "Xv.h"
#include "vbe.h"
#include "vbeModes.h"

#include "i830.h"

#ifdef XF86DRI
#include "dri.h"
#endif

#define BIT(x) (1 << (x))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define NB_OF(x) (sizeof (x) / sizeof (*x))

/* *INDENT-OFF* */
static SymTabRec I830BIOSChipsets[] = {
   {PCI_CHIP_I830_M,		"i830"},
   {PCI_CHIP_845_G,		"845G"},
   {-1,				NULL}
};

static PciChipsets I830BIOSPciChipsets[] = {
   {PCI_CHIP_I830_M,		PCI_CHIP_I830_M,	RES_SHARED_VGA},
   {PCI_CHIP_845_G,		PCI_CHIP_845_G,		RES_SHARED_VGA},
   {-1,				-1,			RES_UNDEFINED}
};

/*
 * Note: "ColorKey" is provided for compatibility with the i810 driver.
 * However, the correct option name is "VideoKey".  "ColorKey" usually
 * refers to the tranparency key for 8+24 overlays, not for video overlays.
 */

typedef enum {
   OPTION_NOACCEL,
   OPTION_SW_CURSOR,
   OPTION_CACHE_LINES,
   OPTION_DRI,
   OPTION_XVIDEO,
   OPTION_VIDEO_KEY,
   OPTION_COLOR_KEY,
   OPTION_STRETCH,
   OPTION_CENTER
} I830Opts;
   
static OptionInfoRec I830BIOSOptions[] = {
   {OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_CACHE_LINES,	"CacheLines",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_DRI,		"DRI",		OPTV_BOOLEAN,	{0},	TRUE},
   {OPTION_XVIDEO,	"XVideo",	OPTV_BOOLEAN,	{0},	TRUE},
   {OPTION_COLOR_KEY,	"ColorKey",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_VIDEO_KEY,	"VideoKey",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_STRETCH,	"Stretch",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_CENTER,	"Center",	OPTV_BOOLEAN,	{0},	FALSE},
   {-1,			NULL,		OPTV_NONE,	{0},	FALSE}
};
/* *INDENT-ON* */

static void I830DisplayPowerManagementSet(ScrnInfoPtr pScrn,
					  int PowerManagementMode, int flags);
static void I830BIOSAdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool I830BIOSCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool I830BIOSSaveScreen(ScreenPtr pScreen, int unblack);
static Bool I830BIOSEnterVT(int scrnIndex, int flags);
static Bool I830VESASetVBEMode(ScrnInfoPtr pScrn, int mode,
			       VbeCRTCInfoBlock *block);

static Bool OffsetFrame = FALSE;



#ifdef I830DEBUG
void
DPRINTF_stub(const char *filename, int line, const char *function,
	     const char *fmt, ...)
{
   va_list ap;

   ErrorF("\n##############################################\n"
	  "*** In function %s, on line %d, in file %s ***\n",
	  function, line, filename);
   va_start(ap, fmt);
   VErrorF(fmt, ap);
   va_end(ap);
   ErrorF("##############################################\n\n");
}
#else /* #ifdef I830DEBUG */
void
DPRINTF_stub(const char *filename, int line, const char *function,
	     const char *fmt, ...)
{
   /* do nothing */
}
#endif /* #ifdef I830DEBUG */

/* XXX Check if this is still needed. */
const OptionInfoRec *
I830BIOSAvailableOptions(int chipid, int busid)
{
   int i;

   for (i = 0; I830BIOSPciChipsets[i].PCIid > 0; i++) {
      if (chipid == I830BIOSPciChipsets[i].PCIid)
	 return I830BIOSOptions;
   }
   return NULL;
}

static Bool
I830BIOSGetRec(ScrnInfoPtr pScrn)
{
   I830Ptr pI830;

   if (pScrn->driverPrivate)
      return TRUE;
   pI830 = pScrn->driverPrivate = xnfcalloc(sizeof(I830Rec), 1);
   pI830->vesa = xnfcalloc(sizeof(VESARec), 1);
   return TRUE;
}

static void
I830BIOSFreeRec(ScrnInfoPtr pScrn)
{
   I830Ptr pI830;
   VESAPtr pVesa;
   DisplayModePtr mode;

   if (!pScrn)
      return;
   if (!pScrn->driverPrivate)
      return;

   pI830 = I830PTR(pScrn);
   mode = pScrn->modes;
   pVesa = pI830->vesa;

   if (mode) {
      do {
	 if (mode->Private) {
	    VbeModeInfoData *data = (VbeModeInfoData *) mode->Private;

	    if (data->block)
	       xfree(data->block);
	    xfree(data);
	    mode->Private = NULL;
	 }
	 mode = mode->next;
      } while (mode && mode != pScrn->modes);
   }

   if (pVesa->vbeInfo)
      VBEFreeVBEInfo(pVesa->vbeInfo);
   if (pVesa->pVbe)
      vbeFree(pVesa->pVbe);
   if (pVesa->monitor)
      xfree(pVesa->monitor);
   if (pVesa->savedPal)
      xfree(pVesa->savedPal);
   xfree(pScrn->driverPrivate);
   pScrn->driverPrivate = NULL;
}

static void
I830BIOSProbeDDC(ScrnInfoPtr pScrn, int index)
{
   vbeInfoPtr pVbe;

   /* The vbe module gets loaded in PreInit(), so no need to load it here. */

   pVbe = VBEInit(NULL, index);
   ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
}

static Bool
I830DetectDisplayDevice(ScrnInfoPtr pScrn)
{
   I830Ptr pI830;
   vbeInfoPtr pVbe;
   int pipe, n;

   pI830 = I830PTR(pScrn);
   pVbe = pI830->vesa->pVbe;

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x0100;
   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);

   if (pVbe->pInt10->ax != 0x005f) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Failed to detect active display devices\n");
      return FALSE;
   }

   pI830->configuredDevices = pVbe->pInt10->cx;

   /* Check for active devices on pipes A and B */
   for (n = 0; n < 2; n++) {
      int shift;

      if (n == 0)
	 shift = PIPE_A_SHIFT;
      else
	 shift = PIPE_B_SHIFT;

      pipe = ((pVbe->pInt10->cx >> shift) & PIPE_ACTIVE_MASK);
      if (pipe) {
	 pI830->pipeEnabled[n] = TRUE;
	 pI830->pipeDevices[n] = pipe;
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Currently active displays on Pipe %c:\n", 'A' + n);
	 if (pipe & PIPE_CRT_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\tCRT\n");

	 if (pipe & PIPE_TV_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\tTV child device\n");

	 if (pipe & PIPE_DFP_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\tDFP child device\n");

	 if (pipe & PIPE_LCD_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "\tLFP (Local Flat Panel) child device\n");

	 if (pipe & PIPE_TV2_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\tSecond TV child device\n");

	 if (pipe & PIPE_DFP2_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\tSecond DFP child device\n");

	 if (pipe & PIPE_UNKNOWN_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "\tSome unknown display devices may also be present\n");

	 /* If a non-CRT device is attached, get its resolution. */

	 pI830->pipeDisplaySize[n].x1 = pI830->pipeDisplaySize[n].y1 = 0;
	 pI830->pipeDisplaySize[n].x2 = pI830->pipeDisplaySize[n].y2 = 4096;
	 if (pipe & ~PIPE_CRT_ACTIVE) {
	    static const unsigned char devices[] = {
	       PIPE_DFP_ACTIVE,
	       PIPE_LCD_ACTIVE,
	       PIPE_DFP2_ACTIVE
	    };
	    int numdevs = sizeof(devices) / sizeof(devices[0]);
	    int i, x, y;

	    for (i = 0; i < numdevs; i++) {
	       if (!(pipe & devices[i]))
		  continue;

	       pVbe->pInt10->num = 0x10;
	       pVbe->pInt10->ax = 0x5f64;
	       pVbe->pInt10->bx = 0x0300;
	       pVbe->pInt10->cx = devices[i] << shift;

	       xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);

	       if (pVbe->pInt10->ax != 0x005f) {
		     xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				"Get Display Device failed for %c:0x%x\n",
				'A' + n, devices[i]);
	       } else {
		  if (pVbe->pInt10->bx & 0x02) {
		     x = (pVbe->pInt10->cx >> 16) & 0xffff;
		     y = (pVbe->pInt10->cx & 0xffff);
		     xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			     "Size of device %c:0x%x is %d x %d\n",
			     'A' + n, devices[i], x, y);
		     if (x < pI830->pipeDisplaySize[n].x2)
			pI830->pipeDisplaySize[n].x2 = x;
		     if (y < pI830->pipeDisplaySize[n].y2)
			pI830->pipeDisplaySize[n].y2 = y;
		  }
	       }
	    }
	 }
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "No active displays on Pipe %c.\n", 'A' + n);
      }

      if (pI830->pipeDisplaySize[n].x2 == 4096)
	 pI830->pipeDisplaySize[n].x2 = 0;
      if (pI830->pipeDisplaySize[n].y2 == 4096)
	 pI830->pipeDisplaySize[n].y2 = 0;
      /* It seems that only x might be reported, so pick the best y. */
      if (pI830->pipeDisplaySize[n].x2 != 0 &&
	  pI830->pipeDisplaySize[n].y2 == 0) {
	 switch (pI830->pipeDisplaySize[n].x2) {
	 case 1280:
	    pI830->pipeDisplaySize[n].y2 = 1024;
	    break;
	 default:
	    pI830->pipeDisplaySize[n].y2 =
		pI830->pipeDisplaySize[n].x2 * 3 / 4;
	    break;
	 }
      }
      if (pI830->pipeDisplaySize[n].x2 != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Lowest common panel size for pipe %c is %d x %d\n",
		    'A' + n, pI830->pipeDisplaySize[n].x2,
		    pI830->pipeDisplaySize[n].y2);
      } else if (pI830->pipeEnabled[n] && pipe & ~PIPE_CRT_ACTIVE) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "No display size information available for pipe %c.\n",
		    'A' + n);
      }
   }

#if 0
   /* A quick hack to change the set of enabled devices. */
   pI830->configuredDevices = PIPE_CRT_ACTIVE;
   /* Turn on the configured displays */
   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x0001;
   pVbe->pInt10->cx = (CARD16)pI830->configuredDevices;
   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);

   if (pVbe->pInt10->ax != 0x005f) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Failed to switch to configured display devices\n");
   }
#endif

   return TRUE;
}

static int
I830DetectMemory(ScrnInfoPtr pScrn)
{
   I830Ptr pI830;
   vbeInfoPtr pVbe;
   PCITAG bridge;
   CARD16 gmch_ctrl;
   int memsize = 0;
   int local = 0;

   pI830 = I830PTR(pScrn);
   pVbe = pI830->vesa->pVbe;

   bridge = pciTag(0, 0, 0);		/* This is always the host bridge */
   gmch_ctrl = pciReadWord(bridge, I830_GMCH_CTRL);

   {
      switch (gmch_ctrl & I830_GMCH_GMS_MASK) {
      case I830_GMCH_GMS_STOLEN_512:
	 memsize = KB(512) - KB(132);
	 break;
      case I830_GMCH_GMS_STOLEN_1024:
	 memsize = MB(1) - KB(132);
	 break;
      case I830_GMCH_GMS_STOLEN_8192:
	 memsize = MB(8) - KB(132);
	 break;
      case I830_GMCH_GMS_LOCAL:
	 /*
	  * I'd like to use the VGA controller registers here, but
	  * MMIOBase isn't yet, so for now, we'll just use the
	  * BIOS instead...
	  */
	 /*
	  * XXX Local memory isn't really handled elsewhere.
	  * is it valid for the 830 and/or 845G?
	  */
	 pVbe->pInt10->num = 0x10;
	 pVbe->pInt10->ax = 0x5f10;
	 xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
	 memsize = pVbe->pInt10->cx * KB(1);
	 local = 1;
	 break;
      }
   }
   if (memsize > 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "detected %dK %s memory.\n", memsize / 1024,
		 local ? "local" : "stolen");
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "no video memory detected.\n");
   }
   return memsize;
}

static Bool
I830MapMMIO(ScrnInfoPtr pScrn)
{
   int mmioFlags;
   I830Ptr pI830 = I830PTR(pScrn);

#if !defined(__alpha__)
   mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
#else
   mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT | VIDMEM_SPARSE;
#endif

   pI830->MMIOBase = xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
				   pI830->PciTag,
				   pI830->MMIOAddr, I810_REG_SIZE);
   if (!pI830->MMIOBase)
      return FALSE;
   return TRUE;
}

static Bool
I830MapMem(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned i;

   for (i = 2; i < pI830->FbMapSize; i <<= 1) ;
   pI830->FbMapSize = i;

   if (!I830MapMMIO(pScrn))
      return FALSE;

   pI830->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pI830->PciTag,
				 pI830->LinearAddr, pI830->FbMapSize);
   if (!pI830->FbBase)
      return FALSE;

   pI830->LpRing.virtual_start = pI830->FbBase + pI830->LpRing.mem.Start;

   return TRUE;
}

static void
I830UnmapMMIO(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pI830->MMIOBase,
		   I810_REG_SIZE);
   pI830->MMIOBase = 0;
}

static Bool
I830UnmapMem(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pI830->FbBase,
		   pI830->FbMapSize);
   pI830->FbBase = 0;
   I830UnmapMMIO(pScrn);
   return TRUE;
}

/*
 * These three functions use a BIOS scratch register that holds the BIOS's
 * view of the (pre-reserved) memory size.
 */
static void
SaveBIOSMemSize(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   VESAPtr pVesa = pI830->vesa;

   if (IS_I830(pI830) || IS_845G(pI830)) {
      pVesa->saveSWF1 = INREG(SWF1) & 0x0f;
   }
}

static void
RestoreBIOSMemSize(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   VESAPtr pVesa = pI830->vesa;
   Bool mapped = (pI830->MMIOBase != NULL);
   CARD32 swf1;

   if (!pVesa || !pVesa->overrideBIOSMemSize)
      return;

   if ((IS_I830(pI830) || IS_845G(pI830))) {
      if (!mapped)
	 I830MapMMIO(pScrn);
      swf1 = INREG(SWF1);
      swf1 &= ~0x0f;
      swf1 |= (pVesa->saveSWF1 & 0x0f);
      OUTREG(SWF1, swf1);
      if (!mapped)
	 I830UnmapMMIO(pScrn);
   }
}

static void
SetBIOSMemSize(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   VESAPtr pVesa = pI830->vesa;
   Bool mapped = (pI830->MMIOBase != NULL);
   CARD32 swf1;

   if (!pVesa || !pVesa->overrideBIOSMemSize)
      return;

   if (IS_I830(pI830) || IS_845G(pI830)) {
      if (!mapped)
	 I830MapMMIO(pScrn);
      swf1 = INREG(SWF1);
      swf1 &= ~0x0f;
      swf1 |= (pVesa->newSWF1 & 0x0f);
      OUTREG(SWF1, swf1);
      if (!mapped)
	 I830UnmapMMIO(pScrn);
   }
}

/*
 * Use the native method instead of the vgahw method.  So far this is
 * only used for 8-bit mode.
 *
 * XXX Look into using the 10-bit gamma correction mode for 15/16/24 bit,
 * and see if a DirectColor visual can be offered.
 */
static void
I830LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		LOCO * colors, VisualPtr pVisual)
{
   I830Ptr pI830;
   int i, index;
   unsigned char r, g, b;
   CARD32 val, temp;

   DPRINTF(PFX, "I830LoadPalette: numColors: %d\n", numColors);
   pI830 = I830PTR(pScrn);

   if (pI830->pipeEnabled[0]) {
      /* It seems that an initial read is needed. */
      temp = INREG(PALETTE_A);
      for (i = 0; i < numColors; i++) {
	 index = indices[i];
	 r = colors[index].red;
	 g = colors[index].green;
	 b = colors[index].blue;
	 val = (r << 16) | (g << 8) | b;
	 OUTREG(PALETTE_A + index * 4, val);
      }
   }
   if (pI830->pipeEnabled[1]) {
      /* It seems that an initial read is needed. */
      temp = INREG(PALETTE_B);
      for (i = 0; i < numColors; i++) {
	 index = indices[i];
	 r = colors[index].red;
	 g = colors[index].green;
	 b = colors[index].blue;
	 val = (r << 16) | (g << 8) | b;
	 OUTREG(PALETTE_B + index * 4, val);
      }
   }
}

static void
PreInitCleanup(ScrnInfoPtr pScrn)
{
   RestoreBIOSMemSize(pScrn);
   I830BIOSFreeRec(pScrn);
}

static Bool
I830BIOSPreInit(ScrnInfoPtr pScrn, int flags)
{
   vgaHWPtr hwp;
   I830Ptr pI830;
   MessageType from;
   rgb defaultWeight = { 0, 0, 0 };
   VESAPtr pVesa;
   vbeInfoPtr pVbe;
   DisplayModePtr p, *pp, tmp;
   EntityInfoPtr pEnt;
   int mem;
   int flags24;
   int i = 0;
   pointer pDDCModule, pVBEModule;

   if (pScrn->numEntities != 1)
      return FALSE;

   /* Load int10 module */
   if (!xf86LoadSubModule(pScrn, "int10"))
      return FALSE;
   xf86LoaderReqSymLists(I810int10Symbols, NULL);

   /* Load vbe module */
   if (!(pVBEModule = xf86LoadSubModule(pScrn, "vbe")))
      return FALSE;
   xf86LoaderReqSymLists(I810vbeSymbols, NULL);

   pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

   if (flags & PROBE_DETECT) {
      I830BIOSProbeDDC(pScrn, pEnt->index);
      return TRUE;
   }

   /* The vgahw module should be loaded here when needed */
   if (!xf86LoadSubModule(pScrn, "vgahw"))
      return FALSE;
   xf86LoaderReqSymLists(I810vgahwSymbols, NULL);


   /* Allocate a vgaHWRec */
   if (!vgaHWGetHWRec(pScrn))
      return FALSE;

   /* Allocate driverPrivate */
   if (!I830BIOSGetRec(pScrn))
      return FALSE;

   pI830 = I830PTR(pScrn);
   pI830->pEnt = pEnt;

   if (pI830->pEnt->location.type != BUS_PCI)
      return FALSE;

   pI830->PciInfo = xf86GetPciInfoForEntity(pI830->pEnt->index);
   pI830->PciTag = pciTag(pI830->PciInfo->bus, pI830->PciInfo->device,
			  pI830->PciInfo->func);

   if (xf86RegisterResources(pI830->pEnt->index, 0, ResNone)) {
      PreInitCleanup(pScrn);
      return FALSE;

   }

   pScrn->racMemFlags = RAC_FB | RAC_COLORMAP;
   pScrn->monitor = pScrn->confScreen->monitor;
   pScrn->progClock = TRUE;
   pScrn->rgbBits = 8;

   flags24 = Support32bppFb | PreferConvert24to32 | SupportConvert24to32;

   if (!xf86SetDepthBpp(pScrn, 0, 0, 0, flags24))
      return FALSE;

   switch (pScrn->depth) {
   case 8:
   case 15:
   case 16:
   case 24:
      break;
   default:
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Given depth (%d) is not supported by I830 driver\n",
		 pScrn->depth);
      return FALSE;
   }
   xf86PrintDepthBpp(pScrn);

   if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
      return FALSE;
   if (!xf86SetDefaultVisual(pScrn, -1))
      return FALSE;

   hwp = VGAHWPTR(pScrn);
   pI830->cpp = pScrn->bitsPerPixel / 8;

   /* Process the options */
   xf86CollectOptions(pScrn, NULL);
   if (!(pI830->Options = xalloc(sizeof(I830BIOSOptions))))
      return FALSE;
   memcpy(pI830->Options, I830BIOSOptions, sizeof(I830BIOSOptions));
   xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pI830->Options);

   /* We have to use PIO to probe, because we haven't mapped yet. */
   I830SetPIOAccess(pI830);

   pVesa = pI830->vesa;
   pVesa->useDefaultRefresh = FALSE;

   /* Initialize VBE record */

   if ((pVesa->pVbe = VBEInit(NULL, pI830->pEnt->index)) == NULL) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "VBE initialization failed.\n");
      return FALSE;
   }

   pVbe = pVesa->pVbe;

   pVesa->vbeInfo = VBEGetVBEInfo(pVbe);

   /* Set the Chipset and ChipRev, allowing config file entries to override. */
   if (pI830->pEnt->device->chipset && *pI830->pEnt->device->chipset) {
      pScrn->chipset = pI830->pEnt->device->chipset;
      from = X_CONFIG;
   } else if (pI830->pEnt->device->chipID >= 0) {
      pScrn->chipset = (char *)xf86TokenToString(I830BIOSChipsets,
						 pI830->pEnt->device->chipID);
      from = X_CONFIG;
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		 pI830->pEnt->device->chipID);
   } else {
      from = X_PROBED;
      pScrn->chipset = (char *)xf86TokenToString(I830BIOSChipsets,
						 pI830->PciInfo->chipType);
   }

   if (pI830->pEnt->device->chipRev >= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		 pI830->pEnt->device->chipRev);
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n",
	      (pScrn->chipset != NULL) ? pScrn->chipset : "Unknown i8xx");

   if (pI830->pEnt->device->MemBase != 0) {
      pI830->LinearAddr = pI830->pEnt->device->MemBase;
      from = X_CONFIG;
   } else {
      if (pI830->PciInfo->memBase[1] != 0) {
	 /* XXX Check mask. */
	 pI830->LinearAddr = pI830->PciInfo->memBase[0] & 0xFF000000;
	 from = X_PROBED;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid FB address in PCI config space\n");
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	      (unsigned long)pI830->LinearAddr);

   if (pI830->pEnt->device->IOBase != 0) {
      pI830->MMIOAddr = pI830->pEnt->device->IOBase;
      from = X_CONFIG;
   } else {
      if (pI830->PciInfo->memBase[1]) {
	 pI830->MMIOAddr = pI830->PciInfo->memBase[1] & 0xFFF80000;
	 from = X_PROBED;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid MMIO address in PCI config space\n");
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "IO registers at addr 0x%lX\n",
	      (unsigned long)pI830->MMIOAddr);

   if (IS_I830(pI830) || IS_845G(pI830)) {
      PCITAG bridge;
      CARD16 gmch_ctrl;

      bridge = pciTag(0, 0, 0);		/* This is always the host bridge */
      gmch_ctrl = pciReadWord(bridge, I830_GMCH_CTRL);
      if ((gmch_ctrl & I830_GMCH_MEM_MASK) == I830_GMCH_MEM_128M) {
	 pI830->FbMapSize = 0x8000000;
      } else {
	 pI830->FbMapSize = 0x4000000;
      }
   }

   /*
    * Get the pre-allocated (stolen) memory size.
    */
   pI830->StolenMemory.Size = I830DetectMemory(pScrn);
   pI830->StolenMemory.Start = 0;
   pI830->StolenMemory.End = pI830->StolenMemory.Size;

   /* Sanity check: compare with what the BIOS thinks. */
   if (pVesa->vbeInfo->TotalMemory != pI830->StolenMemory.Size / 1024 / 64) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Detected stolen memory (%dKB) doesn't match what the BIOS"
		 " reports (%dKB)\n",
		 pI830->StolenMemory.Size / 1024 / 64 * 64,
		 pVesa->vbeInfo->TotalMemory * 64);
   }

   /*
    * The "VideoRam" config file parameter specifies the total amount of
    * memory that will be used/allocated.  When agpgart support isn't
    * available (StolenOnly == TRUE), this is limited to the amount of
    * pre-allocated ("stolen") memory.
    */

   /*
    * Default to I830_DEFAULT_VIDEOMEM (8192KB).
    */
   if (!pI830->pEnt->device->videoRam) {
      from = X_DEFAULT;
      pScrn->videoRam = I830_DEFAULT_VIDEOMEM;
   } else {
      from = X_CONFIG;
      pScrn->videoRam = pI830->pEnt->device->videoRam;
   }

   /* Find the maximum amount of agpgart memory available. */
   mem = I830CheckAvailableMemory(pScrn);
   pI830->StolenOnly = FALSE;

   DPRINTF(PFX,
	   "Available memory: %dk\n"
	   "Requested memory: %dk\n", mem, pScrn->videoRam);

   if (mem <= 0) {
      if (pI830->StolenMemory.Size <= 0) {
	 /* Shouldn't happen. */
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "/dev/agpgart is either not available, or no memory "
		 "is available\nfor allocation, "
		 "and no pre-allocated memory is available.\n");
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "/dev/agpgart is either not available, or no memory "
		 "is available\nfor allocation.  "
		 "Using pre-allocated memory only.\n");
      mem = 0;
      pI830->StolenOnly = TRUE;
   }
   if (mem + (pI830->StolenMemory.Size / 1024) < pScrn->videoRam) {
      pScrn->videoRam = mem + (pI830->StolenMemory.Size / 1024);
      from = X_PROBED;
      if (mem + (pI830->StolenMemory.Size / 1024) <
	  pI830->pEnt->device->videoRam) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "VideoRam reduced to %dK\n", pScrn->videoRam);
      }
   }
   if (mem > 0) {
      /*
       * If the reserved (BIOS accessible) memory is less than the desired
       * mimimum, try to increase it.  We only know how to do this for the
       * 845G (and 830?) so far.
       */
      if (IS_I830(pI830) || IS_845G(pI830)) {
	 if (pVesa->vbeInfo->TotalMemory * 64 < pScrn->videoRam &&
	     pVesa->vbeInfo->TotalMemory * 64 < I830_MINIMUM_VBIOS_MEM) {
	    CARD32 swf1;

	    I830MapMMIO(pScrn);
	    SaveBIOSMemSize(pScrn);
	    swf1 = INREG(SWF1);
	    ErrorF("Before: SWF1 is 0x%08x\n", swf1);
	    pVesa->newSWF1 = 0x08;
	    pVesa->overrideBIOSMemSize = TRUE;
	    SetBIOSMemSize(pScrn);
	    swf1 = INREG(SWF1);
	    ErrorF("After: SWF1 is 0x%08x\n", swf1);
	    VBEFreeVBEInfo(pVesa->vbeInfo);
	    vbeFree(pVesa->pVbe);
	    pVesa->pVbe = VBEInit(NULL, pI830->pEnt->index);
	    pVbe = pVesa->pVbe;
	    pVesa->vbeInfo = VBEGetVBEInfo(pVbe);
	    I830UnmapMMIO(pScrn);
	 }
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Pre-allocated VideoRAM: %d kByte\n",
	      pI830->StolenMemory.Size / 1024);
   xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n", pScrn->videoRam);
   pI830->TotalVideoRam = KB(pScrn->videoRam);

   if (xf86ReturnOptValBool(pI830->Options, OPTION_NOACCEL, FALSE)) {
      pI830->noAccel = TRUE;
   }
   if (xf86ReturnOptValBool(pI830->Options, OPTION_SW_CURSOR, FALSE)) {
      pI830->SWCursor = TRUE;
   }

   pI830->directRenderingDisabled =
	!xf86ReturnOptValBool(pI830->Options, OPTION_DRI, TRUE);

#ifdef XF86DRI
   if (!pI830->directRenderingDisabled) {
      if (pI830->noAccel || pI830->SWCursor) {
	 xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "DRI is disabled because it "
		    "needs HW cursor and 2D acceleration.\n");
	 pI830->directRenderingDisabled = TRUE;
      }
   }
#endif

   if (xf86GetOptValInteger(pI830->Options, OPTION_CACHE_LINES,
			    &(pI830->CacheLines))) {
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Requested %d cache lines\n",
		 pI830->CacheLines);
   } else {
      pI830->CacheLines = -1;
   }

#ifdef I830_XV
   if (xf86GetOptValInteger(pI830->Options, OPTION_VIDEO_KEY,
			    &(pI830->colorKey))) {
      from = X_CONFIG;
   } else if (xf86GetOptValInteger(pI830->Options, OPTION_COLOR_KEY,
			    &(pI830->colorKey))) {
      from = X_CONFIG;
   } else {
      pI830->colorKey = (1 << pScrn->offset.red) |
			(1 << pScrn->offset.green) |
			(((pScrn->mask.blue >> pScrn->offset.blue) - 1) <<
			 pScrn->offset.blue);
      from = X_DEFAULT;
   }
   xf86DrvMsg(pScrn->scrnIndex, from, "video overlay key set to 0x%x\n",
	      pI830->colorKey);
#endif
   

   /* Check if the HW cursor needs physical address. */
   if (IS_MOBILE(pI830))
      pI830->CursorNeedsPhysical = TRUE;
   else
      pI830->CursorNeedsPhysical = FALSE;

   /*
    * XXX If we knew the pre-initialised GTT format for certain, we could
    * probably figure out the physical address even in the StolenOnly case.
    */
   if (pI830->StolenOnly && pI830->CursorNeedsPhysical && !pI830->SWCursor) {
      xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "HW Cursor disabled because it needs agpgart memory.\n");
      pI830->SWCursor = TRUE;
   }

   /*
    * Reduce the maximum videoram available for video modes by the ring buffer,
    * minimum scratch space and HW cursor amounts.
    */
   if (!pI830->SWCursor)
      pScrn->videoRam -= (HWCURSOR_SIZE / 1024);
   if (!pI830->noAccel) {
      pScrn->videoRam -= (PRIMARY_RINGBUFFER_SIZE / 1024);
      pScrn->videoRam -= (MIN_SCRATCH_BUFFER_SIZE / 1024);
   }

   xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	      "Maximum frambuffer space: %d kByte\n", pScrn->videoRam);

   /*
    * If the driver can do gamma correction, it should call xf86SetGamma() here.
    */

   {
      Gamma zeros = { 0.0, 0.0, 0.0 };

      if (!xf86SetGamma(pScrn, zeros))
	 return FALSE;
   }

   if (!I830DetectDisplayDevice(pScrn)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Couldn't detect display devices.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   if ((pDDCModule = xf86LoadSubModule(pScrn, "ddc")) == NULL) {
      PreInitCleanup(pScrn);
      return FALSE;
   } else {
      if ((pVesa->monitor = vbeDoEDID(pVbe, pDDCModule)) != NULL) {
	 xf86PrintEDID(pVesa->monitor);
      }
      xf86UnloadSubModule(pDDCModule);
   }

   /* XXX Move this to a header. */
#define VIDEO_BIOS_SCRATCH 0x18

   /*
    * XXX This should be in ScreenInit/EnterVT.  PreInit should not leave the
    * state changed.
    */
   /* Enable hot keys by writing the proper value to GR18 */
   {
      CARD8 gr18;

      gr18 = pI830->readControl(pI830, GRX, VIDEO_BIOS_SCRATCH);
      gr18 &= ~0x80;			/*
					 * Clear Hot key bit so that Video
					 * BIOS performs the hot key
					 * servicing
					 */
      pI830->writeControl(pI830, GRX, VIDEO_BIOS_SCRATCH, gr18);
   }

   if ((pScrn->monitor->DDC = pVesa->monitor) != NULL)
      xf86SetDDCproperties(pScrn, pVesa->monitor);

   for (i = 0; i < 2; i++) {
      if (pI830->pipeDevices[i] & (PIPE_ACTIVE_MASK & ~PIPE_CRT_ACTIVE)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		    "A non-CRT device is attached to pipe %c.\n"
		    "\tNo refresh rate overrides will be attempted.\n",
		    'A' + i);
	 pVesa->useDefaultRefresh = TRUE;
      }
   }

   /*
    * Some desktop platforms might not have 0x5f05.
    */
   pVesa->useExtendedRefresh = !pVesa->useDefaultRefresh;
   if (pVesa->useExtendedRefresh) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Will use BIOS call 0x5f05 to set non-default refresh "
		 "rates.\n");
   }

   /*
    * Calling 0x5f64 can reset the refresh rate, so only do this when
    * using 0x5f05, or when not overriding the default refresh rate.
    * Also, 0x5f64 doesn't work correctly in i830 platforms.
    */
   pVesa->enableDisplays = !IS_I830(pI830) &&
			   (pVesa->useDefaultRefresh ||
			    pVesa->useExtendedRefresh);
   if (pVesa->enableDisplays) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Will use BIOS call 0x5f64 to enable displays.\n");
   }

   /*
    * XXX This code is different from the previous 830/845 code because
    * it uses a new VBE module function (if present, or a local copy of
    * it if not) to get the list of modes.  It then uses the 5f28 extended
    * function as a final check if it's valid for the current display
    * device.
    */

   /*
    * Note: VBE modes (> 0x7f) won't work with Intel's extended BIOS functions.
    * For that reason it's important to set only V_MODETYPE_VGA in the
    * flags for VBEGetModePool().
    */
   pScrn->modePool = VBEGetModePool(pScrn, pVbe, pVesa->vbeInfo,
				    V_MODETYPE_VGA);

   if (pScrn->modePool == NULL) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "No Video BIOS modes for chosen depth.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

#if 1
   pp = &(pScrn->modePool);
   p = pScrn->modePool;
   while (p) {
      VbeModeInfoData *data = (VbeModeInfoData *) p->Private;

      if (!(data->mode & 0x100)) {
	 pVbe->pInt10->num = 0x10;
	 pVbe->pInt10->ax = 0x5f28;
	 pVbe->pInt10->bx = 0x8000 | (data->mode & 0x7f);
	 pVbe->pInt10->cx = 0x8000;	/* Current display device */
	 xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
      }
      tmp = p->next;
      if (!(data->mode & 0x100) && pVbe->pInt10->ax != 0x005f) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "BIOS mode %x is rejected by 0x5f28.\n", data->mode);
	 xfree(p->Private);
	 *pp = p->next;
	 xfree(p);
      } else {
	 pp = &(p->next);
      }
      p = tmp;
   }
#endif

   if (pScrn->modePool == NULL) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "No Video BIOS modes suitable for the display.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   VBESetModeNames(pScrn->modePool);

   /*
    * XXX DDC information: There's code in xf86ValidateModes
    * (VBEValidateModes) to set monitor defaults based on DDC information
    * where available.  If we need something that does better than this,
    * there's code in vesa/vesa.c.
    */

#ifndef USE_PITCHES
#define USE_PITCHES 0
#endif

   {
#if USE_PITCHES
      /* Good pitches to allow tiling.  Don't care about pitches < 256. */
      int i830_pitches[] = {
	 128 * 2,
	 128 * 4,
	 128 * 8,
	 128 * 16,
	 0
      };   
#endif

      int memsize;
      int *linePitches = NULL;

#if USE_PITCHES && defined(XF86DRI)
      if (!pI830->directRenderingDisabled)
	 linePitches = i830_pitches;
#endif

      /*
       * Limit videoram available for mode selection to what the video
       * BIOS can see.
       */
      if (pScrn->videoRam > (pVesa->vbeInfo->TotalMemory * 64))
	 memsize = pVesa->vbeInfo->TotalMemory * 64;
      else
	 memsize = pScrn->videoRam;
      xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Maximum space available for video modes: %d Kbyte\n",
		 memsize);

      i = VBEValidateModes(pScrn, NULL, pScrn->display->modes, NULL,
			   linePitches, 0, MAX_DISPLAY_PITCH, 1,
			   0, MAX_DISPLAY_HEIGHT,
			   pScrn->display->virtualX,
			   pScrn->display->virtualY,
			   memsize, LOOKUP_BEST_REFRESH);
   }

   if (i <= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   xf86PruneDriverModes(pScrn);

   pScrn->currentMode = pScrn->modes;

   if (pScrn->modes == NULL) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   VBEPrintModes(pScrn);

   if (!pVesa->useDefaultRefresh) {
      /*
       * This sets the parameters for the VBE modes according to the best
       * usable parameters from the Monitor sections modes (usually the
       * default VESA modes), allowing for better than default refresh rates.
       * This only works for VBE 3.0 and later.  Also, we only do this
       * if there are no non-CRT devices attached.
       */
      VBESetModeParameters(pScrn, pVbe);
   }

   /* PreInit shouldn't leave any state changes, so restore this. */
   RestoreBIOSMemSize(pScrn);

   /* Set display resolution */
   xf86SetDpi(pScrn, 0, 0);

   /* Load the required sub modules */
   if (!xf86LoadSubModule(pScrn, "fb")) {
      PreInitCleanup(pScrn);
      return FALSE;
   }

   xf86LoaderReqSymLists(I810fbSymbols, NULL);

   if (!pI830->noAccel) {
      if (!xf86LoadSubModule(pScrn, "xaa")) {
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86LoaderReqSymLists(I810xaaSymbols, NULL);
   }

   if (!pI830->SWCursor) {
      if (!xf86LoadSubModule(pScrn, "ramdac")) {
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86LoaderReqSymLists(I810ramdacSymbols, NULL);
   }

   /*  We won't be using the VGA access after the probe. */
   I830SetMMIOAccess(pI830);
   xf86SetOperatingState(resVgaIo, pI830->pEnt->index, ResUnusedOpr);
   xf86SetOperatingState(resVgaMem, pI830->pEnt->index, ResDisableOpr);

   VBEFreeVBEInfo(pVesa->vbeInfo);
   vbeFree(pVbe);

   return TRUE;
}

/*
 * As the name says.  Check that the initial state is reasonable.
 * If any unrecoverable problems are found, bail out here.
 */
static Bool
CheckInheritedState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int errors = 0, fatal = 0;
   unsigned long temp, head, tail;

   /* Check first for page table errors */
   temp = INREG(PGE_ERR);
   if (temp != 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "PGTBL_ER is 0x%08x\n", temp);
      errors++;
   }
   temp = INREG(PGETBL_CTL);
   if (!(temp & 1)) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "PGTBL_CTL (0x%08x) indicates GTT is disabled\n", temp);
      errors++;
   }
   temp = INREG(LP_RING + RING_LEN);
   if (temp & 1) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "PRB0_CTL (0x%08x) indicates ring buffer enabled\n", temp);
      errors++;
   }
   head = INREG(LP_RING + RING_HEAD);
   tail = INREG(LP_RING + RING_TAIL);
   if ((tail & I830_TAIL_MASK) != (head & I830_HEAD_MASK)) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "PRB0_HEAD (0x%08x) and PRB0_TAIL (0x%08x) indicate "
		 "ring buffer not flushed\n", head, tail);
      errors++;
   }
#if 0
   if (errors)
      I830PrintErrorState(pScrn);
#endif

   if (fatal)
      FatalError("CheckInheritedState: can't recover from the above\n");

   return (errors != 0);
}

/*
 * Reset registers that it doesn't make sense to save/restore to a sane state.
 * This is basically the ring buffer and fence registers.  Restoring these
 * doesn't make sense without restoring GTT mappings.  This is something that
 * whoever gets control next should do.
 */
static void
ResetState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int i;
   unsigned long temp;

   /* Reset the fence registers to 0 */
   for (i = 0; i < 8; i++)
      OUTREG(FENCE + i * 4, 0);

   /* Flush the ring buffer (if enabled), then disable it. */
   if (pI830->AccelInfoRec != NULL) {
      temp = INREG(LP_RING + RING_LEN);
      if (temp & 1) {
	 I830RefreshRing(pScrn);
	 I830Sync(pScrn);
	 DO_RING_IDLE();
      }
   }
   OUTREG(LP_RING + RING_LEN, 0);
   OUTREG(LP_RING + RING_HEAD, 0);
   OUTREG(LP_RING + RING_TAIL, 0);
   OUTREG(LP_RING + RING_START, 0);

   if (pI830->CursorInfoRec && pI830->CursorInfoRec->HideCursor)
       pI830->CursorInfoRec->HideCursor(pScrn);
}

static void
SetFenceRegs(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int i;

   for (i = 0; i < 8; i++) {
      OUTREG(FENCE + i * 4, pI830->ModeReg.Fence[i]);
      if (I810_DEBUG & DEBUG_VERBOSE_VGA)
	 ErrorF("Fence Register : %x\n", pI830->ModeReg.Fence[i]);
   }
}

static void
SetRingRegs(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int itemp;

   if (pI830->noAccel)
      return;

   OUTREG(LP_RING + RING_LEN, 0);
   OUTREG(LP_RING + RING_TAIL, 0);
   OUTREG(LP_RING + RING_HEAD, 0);

   if ((pI830->LpRing.mem.Start & I830_RING_START_MASK) !=
       pI830->LpRing.mem.Start) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "I830SetRingRegs: Ring buffer start (%x) violates its "
		 "mask (%x)\n", pI830->LpRing.mem.Start, I830_RING_START_MASK);
   }
   /* Don't care about the old value.  Reserved bits must be zero anyway. */
   itemp = pI830->LpRing.mem.Start & I830_RING_START_MASK;
   OUTREG(LP_RING + RING_START, itemp);

   if (((pI830->LpRing.mem.Size - 4096) & I830_RING_NR_PAGES) !=
       pI830->LpRing.mem.Size - 4096) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "I830SetRingRegs: Ring buffer size - 4096 (%x) violates its "
		 "mask (%x)\n", pI830->LpRing.mem.Size - 4096,
		 I830_RING_NR_PAGES);
   }
   /* Don't care about the old value.  Reserved bits must be zero anyway. */
   itemp = (pI830->LpRing.mem.Size - 4096) & I830_RING_NR_PAGES;
   itemp |= (RING_NO_REPORT | RING_VALID);
   OUTREG(LP_RING + RING_LEN, itemp);
}

/*
 * This should be called everytime the X server gains control of the screen,
 * before any video modes are programmed (ScreenInit, EnterVT).
 */
static void
SetHWOperatingState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "SetHWOperatingState\n");
   if (!pI830->noAccel)
      SetRingRegs(pScrn);
   SetFenceRegs(pScrn);
   if (!pI830->SWCursor)
      I830InitHWCursor(pScrn);
}

static Bool
SaveHWState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   VESAPtr pVesa = pI830->vesa;
   vbeInfoPtr pVbe = pVesa->pVbe;
   vgaHWPtr hwp = VGAHWPTR(pScrn);
   vgaRegPtr vgaReg = &hwp->SavedReg;

   DPRINTF(PFX, "SaveHWState\n");
   /* Make sure we save at least this information in case of failure. */
   VBEGetVBEMode(pVesa->pVbe, &pVesa->stateMode);

   vgaHWUnlock(hwp);
   vgaHWSave(pScrn, vgaReg, VGA_SR_FONTS);

#ifndef I845G_VBE_WORKAROUND
#define I845G_VBE_WORKAROUND 1
#endif
   /* This save/restore method doesn't work for 845G BIOS */
   /* XXX If it's fixed in production versions, this could be removed. */
   if (!I845G_VBE_WORKAROUND || !IS_845G(pI830)) {
      if (!VBESaveRestore(pVbe, MODE_SAVE, &pVesa->state, &pVesa->stateSize,
			  &pVesa->statePage)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "SaveHWState: VBESaveRestore(MODE_SAVE) failed\n");
	 return FALSE;
      }
   }
   pVesa->savedPal = VBESetGetPaletteData(pVbe, FALSE, 0, 256,
					  NULL, FALSE, FALSE);
   if (!pVesa->savedPal) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "SaveHWState: VBESetGetPaletteData(GET) failed\n");
      return FALSE;
   }
   return TRUE;
}

static Bool
RestoreHWState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   VESAPtr pVesa = pI830->vesa;
   vbeInfoPtr pVbe = pVesa->pVbe;
   vgaHWPtr hwp = VGAHWPTR(pScrn);
   vgaRegPtr vgaReg = &hwp->SavedReg;
   Bool restored = FALSE;

   DPRINTF(PFX, "RestoreHWState\n");
   if (pVesa->state && pVesa->stateSize) {
      /* Make a copy of the state.  Don't rely on it not being touched. */
      if (!pVesa->pstate) {
	 pVesa->pstate = xalloc(pVesa->stateSize);
	 if (pVesa->pstate)
	    memcpy(pVesa->pstate, pVesa->state, pVesa->stateSize);
      }
      restored = VBESaveRestore(pVbe, MODE_RESTORE, &pVesa->state,
				&pVesa->stateSize, &pVesa->statePage);
      if (!restored) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "RestoreHWState: VBESaveRestore failed\n");
      }
      /* Copy back */
      if (pVesa->pstate)
	 memcpy(pVesa->state, pVesa->pstate, pVesa->stateSize);
   }
   /* If that failed, restore the original mode. */
   if (!restored) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Setting the original video mode instead of restoring\n\t"
		 "the saved state\n");
      I830VESASetVBEMode(pScrn, pVesa->stateMode, NULL);
   }

   if (pVesa->savedPal)
      VBESetGetPaletteData(pVbe, TRUE, 0, 256, pVesa->savedPal, FALSE, TRUE);

   vgaHWRestore(pScrn, vgaReg, VGA_SR_FONTS);
   vgaHWLock(hwp);
   return TRUE;
}

static Bool
I830VESASetVBEMode(ScrnInfoPtr pScrn, int mode, VbeCRTCInfoBlock * block)
{
   I830Ptr pI830;
   Bool ret;

#ifdef DEBUGSWF
   CARD32 swf, offset;
#endif

   pI830 = I830PTR(pScrn);
#ifdef DEBUGSWF
   for (offset = SWF00; offset <= SWF06; offset += 4) {
      swf = INREG(offset);
      ErrorF("SWF0%d is 0x%08x\n", (offset - SWF00) / 4, swf);
   }
   for (offset = SWF10; offset <= SWF16; offset += 4) {
      swf = INREG(offset);
      ErrorF("SWF1%d is 0x%08x\n", (offset - SWF10) / 4, swf);
   }
   for (offset = SWF30; offset <= SWF32; offset += 4) {
      swf = INREG(offset);
      ErrorF("SWF3%d is 0x%08x\n", (offset - SWF30) / 4, swf);
   }
#endif
   DPRINTF(PFX, "Setting mode 0x%.8x\n", mode);
   ret = VBESetVBEMode(pI830->vesa->pVbe, mode, block);
#ifdef DEBUGSWF
   for (offset = SWF00; offset <= SWF06; offset += 4) {
      swf = INREG(offset);
      ErrorF("SWF0%d is 0x%08x\n", (offset - SWF00) / 4, swf);
   }
   for (offset = SWF10; offset <= SWF16; offset += 4) {
      swf = INREG(offset);
      ErrorF("SWF1%d is 0x%08x\n", (offset - SWF10) / 4, swf);
   }
   for (offset = SWF30; offset <= SWF32; offset += 4) {
      swf = INREG(offset);
      ErrorF("SWF3%d is 0x%08x\n", (offset - SWF30) / 4, swf);
   }
#endif
   return ret;
}

static Bool
I830VESASetMode(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
   I830Ptr pI830;
   VESAPtr pVesa;
   vbeInfoPtr pVbe;
   VbeModeInfoData *data;
   int mode;
   CARD32 planeA, planeB, temp;

   pI830 = I830PTR(pScrn);
   pVesa = pI830->vesa;
   pVbe = pI830->vesa->pVbe;

   data = (VbeModeInfoData *) pMode->Private;

   /* Always Enable Linear Addressing */
   mode = data->mode | (1 << 15) | (1 << 14);

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      DRILock(screenInfo.screens[pScrn->scrnIndex], 0);
      pI830->LockHeld = 1;
   }
#endif

#ifndef MODESWITCH_RESET_STATE
#define MODESWITCH_RESET_STATE 0
#endif
#if MODESWITCH_RESET_STATE
   ResetState(pScrn);
#endif

   /* XXX Add macros for the various mode parameter bits. */

   if (pVesa->useDefaultRefresh)
      mode &= ~(1 << 11);

#if 0
   /* Turn on the configured displays */
   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x0001;
   pVbe->pInt10->cx = (CARD16)pI830->configuredDevices;
   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);

   if (pVbe->pInt10->ax != 0x005f) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Failed to switch to configured display devices\n");
   }
#endif

   if (I830VESASetVBEMode(pScrn, mode, data->block) == FALSE) {
      if ((data->block && (mode & (1 << 11))) &&
	  I830VESASetVBEMode(pScrn, (mode & ~(1 << 11)), NULL) == TRUE) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Set VBE Mode rejected this modeline. "
		    "Trying standard mode instead!\n");
	 DPRINTF(PFX, "OOPS!\n");
	 xfree(data->block);
	 data->block = NULL;
	 data->mode &= ~(1 << 11);
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Set VBE Mode failed!\n");
	 return FALSE;
      }
   }

   if (data->data->XResolution != pScrn->displayWidth)
      VBESetLogicalScanline(pVbe, pScrn->displayWidth);

   if (pScrn->bitsPerPixel >= 8 && pVesa->vbeInfo->Capabilities[0] & 0x01)
      VBESetGetDACPaletteFormat(pVbe, 8);

   /*
    * Turn on the configured displays.  This has the effect of resetting
    * the default refresh rates to values that the configured displays
    * can handle.  This seems to be the safest way to make sure that this
    * happens.  When it's safe to set higher values, we do that after this.
    *
    * Note: When a DFP is connected to an 830, this causes the mode setting
    * to be trashed.  So, we don't do it on the 830.
    *
    * XXX Need to test an 830 with a LFP.
    */
   if (pVesa->enableDisplays) {
      pVbe->pInt10->num = 0x10;
      pVbe->pInt10->ax = 0x5f64;
      pVbe->pInt10->bx = 0x0001;
      pVbe->pInt10->cx = (CARD16)pI830->configuredDevices;
      xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);

      if (pVbe->pInt10->ax != 0x005f) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Failed to switch to configured display devices\n");
      }
   }

   /*
    * When it's OK to set better than default refresh rates, set them here.
    * NOTE: pVesa->useDefaultRefresh is used redundantly here and in some
    * other places.  That redundancy may be removed one day.  For now,
    * keep it to be safe.
    */
   if (pVesa->useExtendedRefresh && !pVesa->useDefaultRefresh &&
       (mode & (1 << 11)) && data && data->data && data->block) {
      static const int refreshes[] = {
	 43, 56, 60, 70, 72, 75, 85, 100, 120
      };
      int i, nrefreshes = sizeof(refreshes) / sizeof(refreshes[0]);

      for (i = nrefreshes - 1; i >= 0; i--) {
	 /*
	  * Look for the highest value that the requested (refresh + 2) is
	  * greater than or equal to.
	  */
	 if (refreshes[i] <= (data->block->RefreshRate / 100 + 2))
	    break;
      }
      /* i can be 0 if the requested refresh was higher than the max. */
      if (i == 0) {
	 if (data->block->RefreshRate / 100 >= refreshes[nrefreshes - 1])
	    i = nrefreshes - 1;
      }
      DPRINTF(PFX, "Setting refresh rate to %dHz for mode %d\n",
	      refreshes[i], mode & 0xff);
      pVbe->pInt10->num = 0x10;
      pVbe->pInt10->ax = 0x5f05;
      pVbe->pInt10->bx = mode & 0xff;
      pVbe->pInt10->cx = 1 << i;
      xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);

      if (pVbe->pInt10->ax != 0x5f) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Failed to set refresh rate to %dHz.\n",
		    refreshes[i]);
      }
   }


   planeA = INREG(DSPACNTR);
   planeB = INREG(DSPBCNTR);

   pI830->planeEnabled[0] = ((planeA & DISPLAY_PLANE_ENABLE) != 0);
   pI830->planeEnabled[1] = ((planeB & DISPLAY_PLANE_ENABLE) != 0);

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Display plane A is %s.\n",
	      pI830->planeEnabled[0] ? "enabled" : "disabled");
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Display plane B is %s.\n",
	      pI830->planeEnabled[1] ? "enabled" : "disabled");

   /*
    * Sometimes it seems that no display planes are enabled at this point.
    * For mobile platforms pick the plane(s) connected to enabled pipes.
    * For others choose plane A.
    */
   if (!pI830->planeEnabled[0] && !pI830->planeEnabled[1]) {
      if (IS_MOBILE(pI830)) {
	 if ((pI830->pipeEnabled[0] &&
	      ((planeA & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_A)) ||
	     (pI830->pipeEnabled[1] &&
	      ((planeA & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_B))) {
	    pI830->planeEnabled[0] = TRUE;
	 }
	 if ((pI830->pipeEnabled[0] &&
	      ((planeB & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_A)) ||
	     (pI830->pipeEnabled[1] &&
	      ((planeB & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_B))) {
	    pI830->planeEnabled[1] = TRUE;
	 }
      } else {
	 pI830->planeEnabled[0] = TRUE;
      }
      if (pI830->planeEnabled[0]) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Enabling plane A.\n");
	 planeA |= DISPLAY_PLANE_ENABLE;
	 OUTREG(DSPACNTR, planeA);
	 /* flush the change. */
	 temp = INREG(DSPABASE);
	 OUTREG(DSPABASE, temp);
      }
      if (pI830->pipeEnabled[1]) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Enabling plane B.\n");
	 planeB |= DISPLAY_PLANE_ENABLE;
	 OUTREG(DSPBCNTR, planeB);
	 /* flush the change. */
	 temp = INREG(DSPBADDR);
	 OUTREG(DSPBADDR, temp);
      }
   }

   /* XXX Plane C is ignored for now (overlay). */

   /*
    * Print out the PIPEACONF and PIPEBCONF registers.
    */
   temp = INREG(PIPEACONF);
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "PIPEACONF is 0x%08x\n", temp);
   if (IS_MOBILE(pI830)) {
      temp = INREG(PIPEBCONF);
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "PIPEBCONF is 0x%08x\n", temp);
   }

#if MODESWITCH_RESET_STATE
   ResetState(pScrn);
   SetHWOperatingState(pScrn);
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      DRIUnlock(screenInfo.screens[pScrn->scrnIndex]);
      pI830->LockHeld = 0;
   }
#endif

   pScrn->vtSema = TRUE;
   return TRUE;
}

static void
InitRegisterRec(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   I830RegPtr i830Reg = &pI830->ModeReg;
   int i;

   for (i = 0; i < 8; i++)
      i830Reg->Fence[i] = 0;
}

/* Famous last words
 */
void
I830PrintErrorState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   ErrorF("pgetbl_ctl: 0x%lx pgetbl_err: 0x%lx\n",
	  INREG(PGETBL_CTL), INREG(PGE_ERR));

   ErrorF("ipeir: %lx iphdr: %lx\n", INREG(IPEIR), INREG(IPEHR));

   ErrorF("LP ring tail: %lx head: %lx len: %lx start %lx\n",
	  INREG(LP_RING + RING_TAIL),
	  INREG(LP_RING + RING_HEAD) & HEAD_ADDR,
	  INREG(LP_RING + RING_LEN), INREG(LP_RING + RING_START));

   ErrorF("eir: %x esr: %x emr: %x\n",
	  INREG16(EIR), INREG16(ESR), INREG16(EMR));

   ErrorF("instdone: %x instpm: %x\n", INREG16(INST_DONE), INREG8(INST_PM));

   ErrorF("memmode: %lx instps: %lx\n", INREG(MEMMODE), INREG(INST_PS));

   ErrorF("hwstam: %x ier: %x imr: %x iir: %x\n",
	  INREG16(HWSTAM), INREG16(IER), INREG16(IMR), INREG16(IIR));
}

#ifdef I830DEBUG
static void
dump_DSPACNTR(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int tmp;

   /* Display A Control */
   tmp = INREG(0x70180);
   ErrorF("Display A Plane Control Register (0x%.8x)\n", tmp);

   if (tmp & BIT(31))
      ErrorF("   Display Plane A (Primary) Enable\n");
   else
      ErrorF("   Display Plane A (Primary) Disabled\n");

   if (tmp & BIT(30))
      ErrorF("   Display A pixel data is gamma corrected\n");
   else
      ErrorF("   Display A pixel data bypasses gamma correction logic (default)\n");

   switch ((tmp & 0x3c000000) >> 26) {	/* bit 29:26 */
   case 0x00:
   case 0x01:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   case 0x02:
      ErrorF("   8-bpp Indexed\n");
      break;
   case 0x04:
      ErrorF("   15-bit (5-5-5) pixel format (Targa compatible)\n");
      break;
   case 0x05:
      ErrorF("   16-bit (5-6-5) pixel format (XGA compatible)\n");
      break;
   case 0x06:
      ErrorF("   32-bit format (X:8:8:8)\n");
      break;
   case 0x07:
      ErrorF("   32-bit format (8:8:8:8)\n");
      break;
   default:
      ErrorF("   Unknown - Invalid register value maybe?\n");
   }

   if (tmp & BIT(25))
      ErrorF("   Stereo Enable\n");
   else
      ErrorF("   Stereo Disable\n");

   if (tmp & BIT(24))
      ErrorF("   Display A, Pipe B Select\n");
   else
      ErrorF("   Display A, Pipe A Select\n");

   if (tmp & BIT(22))
      ErrorF("   Source key is enabled\n");
   else
      ErrorF("   Source key is disabled\n");

   switch ((tmp & 0x00300000) >> 20) {	/* bit 21:20 */
   case 0x00:
      ErrorF("   No line duplication\n");
      break;
   case 0x01:
      ErrorF("   Line/pixel Doubling\n");
      break;
   case 0x02:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   }

   if (tmp & BIT(18))
      ErrorF("   Stereo output is high during second image\n");
   else
      ErrorF("   Stereo output is high during first image\n");
}

static void
dump_DSPBCNTR(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int tmp;

   /* Display B/Sprite Control */
   tmp = INREG(0x71180);
   ErrorF("Display B/Sprite Plane Control Register (0x%.8x)\n", tmp);

   if (tmp & BIT(31))
      ErrorF("   Display B/Sprite Enable\n");
   else
      ErrorF("   Display B/Sprite Disable\n");

   if (tmp & BIT(30))
      ErrorF("   Display B pixel data is gamma corrected\n");
   else
      ErrorF("   Display B pixel data bypasses gamma correction logic (default)\n");

   switch ((tmp & 0x3c000000) >> 26) {	/* bit 29:26 */
   case 0x00:
   case 0x01:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   case 0x02:
      ErrorF("   8-bpp Indexed\n");
      break;
   case 0x04:
      ErrorF("   15-bit (5-5-5) pixel format (Targa compatible)\n");
      break;
   case 0x05:
      ErrorF("   16-bit (5-6-5) pixel format (XGA compatible)\n");
      break;
   case 0x06:
      ErrorF("   32-bit format (X:8:8:8)\n");
      break;
   case 0x07:
      ErrorF("   32-bit format (8:8:8:8)\n");
      break;
   default:
      ErrorF("   Unknown - Invalid register value maybe?\n");
   }

   if (tmp & BIT(25))
      ErrorF("   Stereo is enabled and both start addresses are used in a two frame sequence\n");
   else
      ErrorF("   Stereo disable and only a single start address is used\n");

   if (tmp & BIT(24))
      ErrorF("   Display B/Sprite, Pipe B Select\n");
   else
      ErrorF("   Display B/Sprite, Pipe A Select\n");

   if (tmp & BIT(22))
      ErrorF("   Sprite source key is enabled\n");
   else
      ErrorF("   Sprite source key is disabled (default)\n");

   switch ((tmp & 0x00300000) >> 20) {	/* bit 21:20 */
   case 0x00:
      ErrorF("   No line duplication\n");
      break;
   case 0x01:
      ErrorF("   Line/pixel Doubling\n");
      break;
   case 0x02:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   }

   if (tmp & BIT(18))
      ErrorF("   Stereo output is high during second image\n");
   else
      ErrorF("   Stereo output is high during first image\n");

   if (tmp & BIT(15))
      ErrorF("   Alpha transfer mode enabled\n");
   else
      ErrorF("   Alpha transfer mode disabled\n");

   if (tmp & BIT(0))
      ErrorF("   Sprite is above overlay\n");
   else
      ErrorF("   Sprite is above display A (default)\n");
}

void
I830_dump_registers(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int i;

   ErrorF("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");

   dump_DSPACNTR(pScrn);
   dump_DSPBCNTR(pScrn);

   ErrorF("0x71400 == 0x%.8x\n", INREG(0x71400));
   ErrorF("0x70008 == 0x%.8x\n", INREG(0x70008));
   for (i = 0x71410; i <= 0x71428; i += 4)
      ErrorF("0x%x == 0x%.8x\n", i, INREG(i));

   ErrorF("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
}
#endif

static Bool
I830BIOSScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
   ScrnInfoPtr pScrn;
   vgaHWPtr hwp;
   I830Ptr pI830;
   VESAPtr pVesa;
   VisualPtr visual;
#ifdef XF86DRI
   Bool driDisabled;
#endif

   pScrn = xf86Screens[pScreen->myNum];
   pI830 = I830PTR(pScrn);
   pVesa = pI830->vesa;
   hwp = VGAHWPTR(pScrn);

   /*
    * If we're changing the BIOS's view of the video memory size, do that
    * first, then re-initialise the VBE information.
    */
   SetBIOSMemSize(pScrn);
   pVesa->pVbe = VBEInit(NULL, pI830->pEnt->index);
   if (!pVesa->pVbe)
      return FALSE;
   pVesa->vbeInfo = VBEGetVBEInfo(pVesa->pVbe);

   miClearVisualTypes();
   if (!xf86SetDefaultVisual(pScrn, -1))
      return FALSE;
   if (pScrn->bitsPerPixel > 8) {
      if (!miSetVisualTypes(pScrn->depth, TrueColorMask,
			    pScrn->rgbBits, TrueColor))
	 return FALSE;
   } else {
      if (!miSetVisualTypes(pScrn->depth,
			    miGetDefaultVisualMask(pScrn->depth),
			    pScrn->rgbBits, pScrn->defaultVisual))
	 return FALSE;
   }
   if (!miSetPixmapDepths())
      return FALSE;

   pI830->XvEnabled =
	xf86ReturnOptValBool(pI830->Options, OPTION_XVIDEO, TRUE);

#ifdef I830_XV
   if (pI830->XvEnabled) {
      if (pI830->noAccel || pI830->StolenOnly) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Xv is disabled because it "
		    "needs 2D accel and AGPGART.\n");
	 pI830->XvEnabled = FALSE;
      }
   }
#else
   pI830->XvEnabled = FALSE;
#endif

   /* Allocate 2D memory */
   pI830->FreeMemory = pI830->TotalVideoRam - pI830->StolenMemory.Size;
   pI830->MemoryAperture.Start = pI830->StolenMemory.End;
   pI830->MemoryAperture.End = pI830->FbMapSize;
   pI830->MemoryAperture.Size = pI830->FbMapSize - pI830->StolenMemory.Size;
   pI830->StolenPool.Fixed = pI830->StolenMemory;
   pI830->StolenPool.Free = pI830->StolenMemory;
   pI830->StolenPool.Total = pI830->StolenMemory;

   /*
    * Force ring buffer to be in low memory for the 845G.
    */
   if (IS_845G(pI830))
      pI830->NeedRingBufferLow = TRUE;

   I830Allocate2DMemory(pScrn, TRUE);

   if (!pI830->noAccel) {
      if (pI830->LpRing.mem.Size == 0) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		     "Disabling acceleration because the ring buffer "
		      "allocation failed.\n");
	   pI830->noAccel = TRUE;
      }
   }

   if (!pI830->SWCursor) {
      if (pI830->CursorMem.Key == -1) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		     "Disabling HW cursor because the cursor memory "
		      "allocation failed.\n");
	   pI830->SWCursor = TRUE;
      }
   }

#ifdef I830_XV
   if (pI830->XvEnabled) {
      if (pI830->noAccel) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Disabling Xv because it "
		    "needs 2D acceleration.\n");
	 pI830->XvEnabled = FALSE;
      }
      if (pI830->OverlayMem.Physical == 0) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		     "Disabling Xv because the overlay register buffer "
		      "allocation failed.\n");
	 pI830->XvEnabled = FALSE;
      }
   }
#endif

   InitRegisterRec(pScrn);

#ifdef XF86DRI
   /*
    * pI830->directRenderingDisabled is set once in PreInit.  Reinitialise
    * pI830->directRenderingEnabled based on it each generation.
    */
   pI830->directRenderingEnabled = !pI830->directRenderingDisabled;
   /*
    * Setup DRI after visuals have been established, but before fbScreenInit
    * is called.   fbScreenInit will eventually call into the drivers
    * InitGLXVisuals call back.
    */

   if (pI830->directRenderingEnabled) {
      if (pI830->noAccel || pI830->SWCursor || pI830->StolenOnly) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DRI is disabled because it "
		    "needs HW cursor, 2D accel and AGPGART.\n");
	 pI830->directRenderingEnabled = FALSE;
      }
   }


   driDisabled = !pI830->directRenderingEnabled;

   if (pI830->directRenderingEnabled)
      pI830->directRenderingEnabled = I830DRIScreenInit(pScreen);

   if (pI830->directRenderingEnabled)
      if (!(pI830->directRenderingEnabled = I830Allocate3DMemory(pScrn)))
	  I830DRICloseScreen(pScreen);
 
   
#else
   pI830->directRenderingEnabled = FALSE;
#endif

   /*
    * After the 3D allocations have been done, see if there's any free space
    * that can be added to the framebuffer allocation.
    */
   I830Allocate2DMemory(pScrn, FALSE);

   DPRINTF(PFX, "assert(if(!I830DoPoolAllocation(pScrn, pI830->StolenPool)))\n");
   if (!I830DoPoolAllocation(pScrn, &(pI830->StolenPool)))
      return FALSE;

   DPRINTF(PFX, "assert( if(!I830FixupOffsets(pScrn)) )\n");
   if (!I830FixupOffsets(pScrn))
      return FALSE;

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      I830SetupMemoryTiling(pScrn);
      pI830->directRenderingEnabled = I830DRIDoMappings(pScreen);
   }
#endif

   DPRINTF(PFX, "assert( if(!I830MapMem(pScrn)) )\n");
   if (!I830MapMem(pScrn))
      return FALSE;

   pScrn->memPhysBase = (unsigned long)pI830->FbBase;
   pScrn->fbOffset = pI830->FrontBuffer.Start;

   vgaHWSetMmioFuncs(hwp, pI830->MMIOBase, 0);
   vgaHWGetIOBase(hwp);
   DPRINTF(PFX, "assert( if(!vgaHWMapMem(pScrn)) )\n");
   if (!vgaHWMapMem(pScrn))
      return FALSE;

   DPRINTF(PFX, "assert( if(!I830BISEnterVT(scrnIndex, 0)) )\n");
   if (!I830BIOSEnterVT(scrnIndex, 0))
      return FALSE;

   DPRINTF(PFX, "assert( if(!fbScreenInit(pScreen, ...) )\n");
   if (!fbScreenInit(pScreen, pI830->FbBase + pScrn->fbOffset,
		     pScrn->virtualX, pScrn->virtualY,
		     pScrn->xDpi, pScrn->yDpi,
		     pScrn->displayWidth, pScrn->bitsPerPixel))
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

   fbPictureInit(pScreen, 0, 0);

   xf86SetBlackWhitePixels(pScreen);

#if 1
   I830DGAInit(pScreen);
#endif

   DPRINTF(PFX,
	   "assert( if(!xf86InitFBManager(pScreen, &(pI830->FbMemBox))) )\n");
   if (!xf86InitFBManager(pScreen, &(pI830->FbMemBox))) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Failed to init memory manager\n");
      return FALSE;
   }

   if (!pI830->noAccel) {
      if (!I830AccelInit(pScreen)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Hardware acceleration initialization failed\n");
      }
   }

   miInitializeBackingStore(pScreen);
   xf86SetBackingStore(pScreen);
   xf86SetSilkenMouse(pScreen);
   miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

   if (!pI830->SWCursor) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Initializing HW Cursor\n");
      if (!I830CursorInit(pScreen))
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Hardware cursor initialization failed\n");
   } else
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Initializing SW Cursor!\n");

   DPRINTF(PFX, "assert( if(!miCreateDefColormap(pScreen)) )\n");
   if (!miCreateDefColormap(pScreen))
      return FALSE;

   DPRINTF(PFX, "assert( if(!xf86HandleColormaps(pScreen, ...)) )\n");
   if (!xf86HandleColormaps(pScreen, 256, 8, I830LoadPalette, 0,
			    CMAP_RELOAD_ON_MODE_SWITCH)) {
      return FALSE;
   }

#ifdef DPMSExtension
   xf86DPMSInit(pScreen, I830DisplayPowerManagementSet, 0);
#endif

#ifdef I830_XV
   /* Init video */
   if (pI830->XvEnabled)
      I830InitVideo(pScreen);
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      pI830->directRenderingEnabled = I830DRIFinishScreenInit(pScreen);
   }
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      pI830->directRenderingOpen = TRUE;
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Enabled\n");
      /* What about doing this on EnterVT too? */
      /* Setup 3D engine */
#if 1
      I830EmitInvarientState(pScrn);
#endif
#if 0
      I830EmitInvarientState2(pScrn);
#endif
   } else {
      if (driDisabled)
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Disabled\n");
      else
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Failed\n");
   }
#else
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Not available\n");
#endif

   pScreen->SaveScreen = I830BIOSSaveScreen;
   pI830->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = I830BIOSCloseScreen;

   if (serverGeneration == 1)
      xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
#if 0
#ifdef I830DEBUG
   I830_dump_registers(pScrn);
#endif
#endif
   return TRUE;
}

static void
I830BIOSAdjustFrame(int scrnIndex, int x, int y, int flags)
{
   ScrnInfoPtr pScrn;
   I830Ptr pI830;
   vbeInfoPtr pVbe;
   static int xoffset = 0, yoffset = 0;
   static unsigned long adjustGeneration = 0;

   pScrn = xf86Screens[scrnIndex];
   pI830 = I830PTR(pScrn);
   pVbe = pI830->vesa->pVbe;

   /* Calculate the offsets once per server generation. */
   if (adjustGeneration != serverGeneration) {
      adjustGeneration = serverGeneration;
      xoffset = (pScrn->fbOffset / pI830->cpp) % pScrn->displayWidth;
      yoffset = (pScrn->fbOffset / pI830->cpp) / pScrn->displayWidth;
   }

   DPRINTF(PFX, "y = %d (+ %d), x = %d (+ %d)\n", x, xoffset, y, yoffset);

   if (OffsetFrame) {
      y = (pI830->FbMemBox.y2 - pScrn->currentMode->VDisplay);
      ErrorF("AdjustFrame: OffsetFrame is set, setting y to %d\n", y);
   }
   x += xoffset;
   y += yoffset;
#if 0
   x >>= 4;
   x <<= 4;
#endif
   VBESetDisplayStart(pVbe, x, y, TRUE);
}

static void
I830BIOSFreeScreen(int scrnIndex, int flags)
{
   I830BIOSFreeRec(xf86Screens[scrnIndex]);
   if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
      vgaHWFreeHWRec(xf86Screens[scrnIndex]);
}

static void
I830BIOSLeaveVT(int scrnIndex, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
#ifdef XF86DRI
   I830Ptr pI830 = I830PTR(pScrn);
#endif

   DPRINTF(PFX, "Leave VT\n");

#ifdef XF86DRI
   if (pI830->directRenderingOpen) {
      DPRINTF(PFX, "calling dri lock\n");
      DRILock(screenInfo.screens[scrnIndex], 0);
      pI830->LockHeld = 1;
   }
#endif

   ResetState(pScrn);
   RestoreHWState(pScrn);
   RestoreBIOSMemSize(pScrn);
   I830UnbindGARTMemory(pScrn);
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
I830BIOSEnterVT(int scrnIndex, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);
   static unsigned long SaveGeneration = 0;

   DPRINTF(PFX, "Enter VT\n");

   if (!I830BindGARTMemory(pScrn))
      return FALSE;

   CheckInheritedState(pScrn);
   SetBIOSMemSize(pScrn);
   /*
    * Only save state once per server generation since that's what most
    * drivers do.  Could change this to save state at each VT enter.
    */
   if (SaveGeneration != serverGeneration) {
      SaveGeneration = serverGeneration;
      SaveHWState(pScrn);
   }
   ResetState(pScrn);
   SetHWOperatingState(pScrn);
   
#if 1
   /* Clear the framebuffer */
   memset(pI830->FbBase + pScrn->fbOffset, 0,
	  pScrn->virtualY * pScrn->displayWidth * pI830->cpp);
#endif

   if (!I830VESASetMode(pScrn, pScrn->currentMode))
      return FALSE;

   ResetState(pScrn);
   SetHWOperatingState(pScrn);

   pScrn->AdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      DPRINTF(PFX, "calling dri unlock\n");
      DRIUnlock(screenInfo.screens[scrnIndex]);
      pI830->LockHeld = 0;
   }
#endif

   return TRUE;
}

static Bool
I830BIOSSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{

   int _head;
   int _tail;
   I830Ptr pI830;
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   int ret;

   pI830 = I830PTR(pScrn);

   if (!pI830->noAccel && (1 || IS_845G(pI830))) {	/*Stops head pointer freezes for 845G */
      do {
	 _head = INREG(LP_RING + RING_HEAD) & I830_HEAD_MASK;
	 _tail = INREG(LP_RING + RING_TAIL) & I830_TAIL_MASK;
	 DELAY(1000);
      } while (_head != _tail);
   }
   DPRINTF(PFX, "mode == %p\n", mode);

#if 0
   OffsetFrame = !OffsetFrame;
   pScrn->AdjustFrame(scrnIndex, 0, 0, 0);
#endif

#ifndef BINDUNBIND
#define BINDUNBIND 0
#endif
#if BINDUNBIND
   I830UnbindGARTMemory(pScrn);
#endif
   ret = I830VESASetMode(xf86Screens[scrnIndex], mode);
#if BINDUNBIND
   I830BindGARTMemory(pScrn);
#endif
   return ret;
}

static Bool
I830BIOSSaveScreen(ScreenPtr pScreen, int mode)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I830Ptr pI830 = I830PTR(pScrn);
   Bool on = xf86IsUnblank(mode);
   CARD32 temp, ctrl, base, i;

   DPRINTF(PFX, "I830BIOSSaveScreen: %d, on is %s\n", mode, BOOLTOSTRING(on));

   for (i = 0; i < 2; i++) {
      if (i == 0) {
	 ctrl = DSPACNTR;
	 base = DSPABASE;
      } else {
	 ctrl = DSPBCNTR;
	 base = DSPBADDR;
      }
      if (pI830->planeEnabled[i]) {
	 temp = INREG(ctrl);
	 if (on)
	    temp |= DISPLAY_PLANE_ENABLE;
	 else
	    temp &= ~DISPLAY_PLANE_ENABLE;
	 OUTREG(ctrl, temp);
	 /* Flush changes */
	 temp = INREG(base);
	 OUTREG(base, temp);
      }
   }

   if (pI830->CursorInfoRec && !pI830->SWCursor) {
      if (on && pI830->cursorOn)
	 pI830->CursorInfoRec->ShowCursor(pScrn);
      else
	 pI830->CursorInfoRec->HideCursor(pScrn);
   }

   return TRUE;
}

/* Use the VBE version when available. */
static void
I830DisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			      int flags)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->vesa->pVbe;

   if (xf86LoaderCheckSymbol("VBEDPMSSet")) {
      VBEDPMSSet(pVbe, PowerManagementMode);
   } else {
      pVbe->pInt10->num = 0x10;
      pVbe->pInt10->ax = 0x4f10;
      pVbe->pInt10->bx = 0x01;

      switch (PowerManagementMode) {
      case DPMSModeOn:
	 break;
      case DPMSModeStandby:
	 pVbe->pInt10->bx |= 0x0100;
	 break;
      case DPMSModeSuspend:
	 pVbe->pInt10->bx |= 0x0200;
	 break;
      case DPMSModeOff:
	 pVbe->pInt10->bx |= 0x0400;
	 break;
      }
      xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   }
}

static Bool
I830BIOSCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);
   XAAInfoRecPtr infoPtr = pI830->AccelInfoRec;

#ifdef XF86DRI
   if (pI830->directRenderingOpen) {
      pI830->directRenderingOpen = FALSE;
      I830DRICloseScreen(pScreen);
   }
#endif

   if (pScrn->vtSema == TRUE) {
      I830BIOSLeaveVT(scrnIndex, 0);
   }

   DPRINTF(PFX, "\nUnmapping memory\n");
   I830UnmapMem(pScrn);
   vgaHWUnmapMem(pScrn);

   if (pI830->ScanlineColorExpandBuffers) {
      xfree(pI830->ScanlineColorExpandBuffers);
      pI830->ScanlineColorExpandBuffers = 0;
   }

   if (infoPtr) {
      if (infoPtr->ScanlineColorExpandBuffers)
	 xfree(infoPtr->ScanlineColorExpandBuffers);
      XAADestroyInfoRec(infoPtr);
      pI830->AccelInfoRec = NULL;
   }

   if (pI830->CursorInfoRec) {
      xf86DestroyCursorInfoRec(pI830->CursorInfoRec);
      pI830->CursorInfoRec = 0;
   }

   xf86GARTCloseScreen(scrnIndex);

   pScrn->vtSema = FALSE;
   pScreen->CloseScreen = pI830->CloseScreen;
   return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

static int
I830ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
   if (mode->Flags & V_INTERLACE) {
      if (verbose) {
	 xf86DrvMsg(scrnIndex, X_PROBED,
		    "Removing interlaced mode \"%s\"\n", mode->name);
      }
      return MODE_BAD;
   }
   return MODE_OK;
}

void
I830InitpScrn(ScrnInfoPtr pScrn)
{
   pScrn->PreInit = I830BIOSPreInit;
   pScrn->ScreenInit = I830BIOSScreenInit;
   pScrn->SwitchMode = I830BIOSSwitchMode;
   pScrn->AdjustFrame = I830BIOSAdjustFrame;
   pScrn->EnterVT = I830BIOSEnterVT;
   pScrn->LeaveVT = I830BIOSLeaveVT;
   pScrn->FreeScreen = I830BIOSFreeScreen;
   pScrn->ValidMode = I830ValidMode;
}
