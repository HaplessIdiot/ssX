/* $XFree86$ */
/*
 * $Workfile: geode_driver.c $
 * $Revision: 1.1 $
 *
 * File Contents: This is the main module configures the interfacing 
 *                with the X server. The individual modules will be 
 *                loaded based upon the options selected from the 
 *                XF86Config. This file also has modules for finding 
 *                supported modes, turning on the modes based on options.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 *
 */

/* 
 * NSC_LIC_ALTERNATIVE_PREAMBLE
 *
 * Revision 1.0
 *
 * National Semiconductor Alternative GPL-BSD License
 *
 * National Semiconductor Corporation licenses this software 
 * ("Software"):
 *
 * Geode Xfree frame buffer driver
 *
 * under one of the two following licenses, depending on how the 
 * Software is received by the Licensee.
 * 
 * If this Software is received as part of the Linux Framebuffer or
 * other GPL licensed software, then the GPL license designated 
 * NSC_LIC_GPL applies to this Software; in all other circumstances 
 * then the BSD-style license designated NSC_LIC_BSD shall apply.
 *
 * END_NSC_LIC_ALTERNATIVE_PREAMBLE */

/* NSC_LIC_BSD
 *
 * National Semiconductor Corporation Open Source License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (BSD License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer. 
 *
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution. 
 *
 *   * Neither the name of the National Semiconductor Corporation nor 
 *     the names of its contributors may be used to endorse or promote 
 *     products derived from this software without specific prior 
 *     written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE,
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * END_NSC_LIC_BSD */

/* NSC_LIC_GPL
 *
 * National Semiconductor Corporation Gnu General Public License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (GPL License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted under the terms of the GNU General 
 * Public License as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later version  
 *
 * In addition to the terms of the GNU General Public License, neither 
 * the name of the National Semiconductor Corporation nor the names of 
 * its contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE, 
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. See the GNU General Public License for more details. 
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this file; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 *
 * END_NSC_LIC_GPL */

/*
 * Lots of fixes & updates
 * Alan Hourihane <alanh@fairlite.demon.co.uk>
 */

#define DEBUG(x)

#define GEODE_TRACE 0
#define MAGIC 0
/* Flat Panel HACK!!!! */
extern int gfx_dont_program;

#if GEODE_TRACE
/* ANSI C does not allow var arg macros */
#define GeodeDebug(args) ErrorF##args
#else
#define GeodeDebug(args)
#endif

/* Includes that are used by all drivers */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Resources.h"

/* We may want inb() and outb() */
#include "compiler.h"

/* We may want to access the PCI config space */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

/* Colormap handling stuff */
#include "xf86cmap.h"

/* Frame buffer stuff */
#include "fb.h"
#include "shadowfb.h"

/* Machine independent stuff */
#include "mipointer.h"
#include "mibank.h"
#include "micmap.h"
/* All drivers implementing backing store need this */
#include "mibstore.h"
#include "vgaHW.h"
#include "vbe.h"

/* Check for some extensions */
#ifdef XFreeXDGA
#define _XF86_DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif /* XFreeXDGA */

#ifdef DPMSExtension
#include "globals.h"
#include "opaque.h"
#define DPMS_SERVER
#include "extensions/dpms.h"
#endif /* DPMSExtension */

/* Our private include file (this also includes the durango headers) */
#include "geode.h"

#define PCI_VENDOR_ID_CYRIX  0x1078
#define PCI_VENDOR_ID_NS     0x100b

#define PCI_CHIP_5530     0x0104
#define PCI_CHIP_SC1200   0x0504
#define PCI_CHIP_SC1400   0x0104

/* A few things all drivers should have */
#define GEODE_NAME     		"GEODE"
#define GEODE_DRIVER_NAME       "geode"

/* This should match the durango code version.
 * The patchlevel may be used to indicate changes in geode.c 
 */
#define GEODE_VERSION_NAME    "1.2.2"
#define GEODE_VERSION_MAJOR   1
#define GEODE_VERSION_MINOR   2
#define GEODE_PATCHLEVEL      2
#define GEODE_VERSION_CURRENT ((GEODE_VERSION_MAJOR << 24) | \
		(GEODE_VERSION_MINOR << 16) | GEODE_PATCHLEVEL)

typedef struct _MemOffset
{
   unsigned long xres;
   unsigned long yres;
   unsigned long bpp;
   unsigned long CBOffset;
   unsigned short CBPitch;
   unsigned short CBSize;
   unsigned long CurOffset;
   unsigned long OffScreenOffset;
}
MemOffset;

/* predefined memory address for compression and cursor offsets
 * if COMPRESSION enabled.
 */
MemOffset GeodeMemOffset[] = {
   {640, 480, 8, 640, 1024, 272, 0x78000, 0x78100},
   {640, 480, 16, 1280, 2048, 272, 0x610, 0xF0000},
   {800, 600, 8, 800, 1024, 208, 0x96000, 0x96100},
   {800, 600, 16, 1600, 2048, 272, 0x12C000, 0x12C100},
   {1024, 768, 8, 0xC0000, 272, 272, 0xF3000, 0xF3100},
   {1024, 768, 16, 0x180000, 272, 272, 0x1B3000, 0x1B3100},
   {1152, 864, 8, 1152, 2048, 272, 0x590, 0x1B0000},
   {1152, 864, 16, 2304, 4096, 272, 0xA10, 0x360000},
   {1280, 1024, 8, 1280, 2048, 272, 0x610, 0x200000},
   {1280, 1024, 16, 2560, 4096, 272, 0xB10, 0x400000},

   /* PAL TV modes */

   {704, 576, 16, 1408, 2048, 272, 0x690, 0x120000},
   {720, 576, 16, 1440, 2048, 272, 0x6B0, 0x120000},
   {768, 576, 16, 1536, 2048, 256, 0x700, 0x120000},

   /* NTSC TV modes */

   {704, 480, 16, 1408, 2048, 272, 0x690, 0xF0000},
   {720, 480, 16, 1440, 2048, 272, 0x6B0, 0xF0000}

};
static int MemIndex = 0;

/* Forward definitions */
static const OptionInfoRec *GeodeAvailableOptions(int chipid, int busid);
static void GeodeIdentify(int);
static Bool GeodeProbe(DriverPtr, int);
static Bool GeodePreInit(ScrnInfoPtr, int);
static Bool GeodeScreenInit(int, ScreenPtr, int, char **);
static Bool GeodeEnterVT(int, int);
static void GeodeLeaveVT(int, int);
static void GeodeFreeScreen(int, int);
void GeodeAdjustFrame(int, int, int, int);
Bool GeodeSwitchMode(int, DisplayModePtr, int);
static int GeodeValidMode(int, DisplayModePtr, Bool, int);
static void GeodeLoadPalette(ScrnInfoPtr pScreenInfo,
			     int numColors, int *indizes,
			     LOCO * colors, VisualPtr pVisual);
static Bool GeodeMapMem(ScrnInfoPtr);
static Bool GeodeUnmapMem(ScrnInfoPtr);

extern Bool GeodeAccelInit(ScreenPtr pScreen);
extern Bool GeodeHWCursorInit(ScreenPtr pScreen);
extern void GeodeHideCursor(ScrnInfoPtr pScreenInfo);
extern void GeodeShowCursor(ScrnInfoPtr pScreenInfo);
extern void GeodePointerMoved(int index, int x, int y);
extern void GeodeRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
extern void GeodeRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
extern void GeodeRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
extern void GeodeInitVideo(ScreenPtr pScreen);
extern Bool GeodeDGAInit(ScreenPtr pScreen);
extern void GeodeLoadCursorImage(ScrnInfoPtr pScreenInfo, unsigned char *src);

void get_tv_overscan_geom(const char *options, int *X, int *Y, int *W,
			  int *H);

#if !defined(STB_X)
extern unsigned char *XpressROMPtr;
#endif /* STB_X */

/* driver record contains the functions needed by the server after loading
 * the driver module.
 */
DriverRec GEODE = {
   GEODE_VERSION_CURRENT,
   GEODE_DRIVER_NAME,
   GeodeIdentify,
   GeodeProbe,
   GeodeAvailableOptions,
   NULL,
   0
};

/* Supported chipsets */
static SymTabRec GeodeChipsets[] = {
   {PCI_CHIP_5530, "5530"},
   {PCI_CHIP_SC1200, "SC1200"},
   {PCI_CHIP_SC1400, "SC1400"},
   {-1, NULL}
};

static PciChipsets GeodePCIchipsets[] = {
   {PCI_CHIP_5530, PCI_CHIP_5530, RES_SHARED_VGA},
   {PCI_CHIP_SC1200, PCI_CHIP_SC1200, RES_SHARED_VGA},
   {PCI_CHIP_SC1400, PCI_CHIP_SC1400, RES_SHARED_VGA},
   {-1, -1, RES_UNDEFINED},
};

/* option flags are self-explanatory */
typedef enum
{
   OPTION_SW_CURSOR,
   OPTION_HW_CURSOR,
   OPTION_COMPRESSION,
   OPTION_NOACCEL,
   OPTION_TV_SUPPORT,
   OPTION_TV_OUTPUT,
   OPTION_TV_OVERSCAN,
   OPTION_SHADOW_FB,
   OPTION_ROTATE,
   OPTION_FLATPANEL,
   OPTION_FLATPANEL_IN_BIOS,
   OPTION_OVERLAY,
   OPTION_COLOR_KEY,
   OPTION_VIDEO_KEY,
   OPTION_IMG_BUFS,
   OPTION_DONT_PROGRAM
}
GeodeOpts;

static OptionInfoRec GeodeOptions[] = {
   {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_COMPRESSION, "Compression", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_TV_SUPPORT, "TV", OPTV_ANYSTR, {0}, FALSE},
   {OPTION_TV_OUTPUT, "TV_Output", OPTV_ANYSTR, {0}, FALSE},
   {OPTION_TV_OVERSCAN, "TVOverscan", OPTV_ANYSTR, {0}, FALSE},
   {OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_ROTATE, "Rotate", OPTV_ANYSTR, {0}, FALSE},
   {OPTION_FLATPANEL, "FlatPanel", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_FLATPANEL_IN_BIOS, "FlatPanelInBios", OPTV_BOOLEAN, {0}, FALSE},
   {OPTION_OVERLAY, "Overlay", OPTV_ANYSTR, {0}, FALSE},
   {OPTION_COLOR_KEY, "ColorKey", OPTV_INTEGER, {0}, FALSE},
   {OPTION_VIDEO_KEY, "VideoKey", OPTV_INTEGER, {0}, FALSE},
   {OPTION_IMG_BUFS, "ImageWriteBuffers", OPTV_INTEGER,	{0}, FALSE },
   {OPTION_DONT_PROGRAM, "DontProgram", OPTV_BOOLEAN, {0}, FALSE},
   {-1, NULL, OPTV_NONE, {0}, FALSE}
};

/* List of symbols from other modules that this module references.The purpose
* is that to avoid unresolved symbol warnings
*/
static const char *vgahwSymbols[] = {
   "vgaHWGetHWRec",
   "vgaHWUnlock",
   "vgaHWInit",
   "vgaHWSave",
   "vgaHWRestore",
   "vgaHWProtect",
   "vgaHWGetIOBase",
   "vgaHWMapMem",
   "vgaHWLock",
   "vgaHWFreeHWRec",
   "vgaHWSaveScreen",
   NULL
};

static const char *vbeSymbols[] = {
   "VBEInit",
   "vbeDoEDID",
   "vbeFree",
   NULL
};

static const char *fbSymbols[] = {
   "fbScreenInit",
   "fbPictureInit",
   NULL
};

static const char *xaaSymbols[] = {
   "XAADestroyInfoRec",
   "XAACreateInfoRec",
   "XAAInit",
   "XAAScreenIndex",
   NULL
};

static const char *ramdacSymbols[] = {
   "xf86InitCursor",
   "xf86CreateCursorInfoRec",
   "xf86DestroyCursorInfoRec",
   NULL
};

static const char *shadowSymbols[] = {
   "ShadowFBInit",
   NULL
};

#ifdef XFree86LOADER

/* Module loader interface */

static MODULESETUPPROTO(GeodeSetup);

static XF86ModuleVersionInfo GeodeVersionRec = {
   "geode",
   MODULEVENDORSTRING,
   MODINFOSTRING1,
   MODINFOSTRING2,
   XF86_VERSION_CURRENT,
   GEODE_VERSION_MAJOR, GEODE_VERSION_MINOR, GEODE_PATCHLEVEL,
   ABI_CLASS_VIDEODRV,			/* This is a video driver */
   ABI_VIDEODRV_VERSION,
   MOD_CLASS_VIDEODRV,
   {0, 0, 0, 0}
};

/*
 * This data is accessed by the loader.  The name must be the module name
 * followed by "ModuleInit".
 */
XF86ModuleData geodeModuleData = { &GeodeVersionRec, GeodeSetup, NULL };

/*-------------------------------------------------------------------------
 * GeodeSetup.
 *
 * Description	:This function sets up the driver in X list and load the
 *               module symbols through xf86loader routines..
 *
 * Parameters.
 *    Module	:Pointer to the geode  module
 *    options	:Driver module options.
 *    ErrorMajor:Major no
 *    ErrorMinor:Minor no.
 *
 * Returns		:NULL on success
 *
 * Comments     :Module setup is done by this function
 *
 *-------------------------------------------------------------------------
*/
static pointer
GeodeSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
   static Bool Initialised = FALSE;

   if (!Initialised) {
      Initialised = TRUE;
      xf86AddDriver(&GEODE, Module, 0);
      /* Tell the loader about symbols from other modules that this
       * module might refer to.
       */
      LoaderRefSymLists(vgahwSymbols, fbSymbols, vbeSymbols,
			xaaSymbols, ramdacSymbols, shadowSymbols, NULL);
      return (pointer) TRUE;
   }
   /*The return value must be non-NULL on success */
   if (ErrorMajor)
      *ErrorMajor = LDR_ONCEONLY;
   return NULL;
}
#endif /*End of XFree86Loader */

/* This functions may be used in future implementations */

#if 0
/* This is not used code and it may be used for vga setup */
static void
print_gxm_gfx_reg(GeodePtr pGeode, unsigned int count)
{
   int i, base = 0x8300;
   unsigned int *preg;

   GeodeDebug(("Geode Registers:\n"));
   for (i = 0; i <= count; i += 4) {
      preg = (unsigned int *)(gfx_regptr + base + i);
      DEBUGMSG(0, (0, X_NONE, "%x = %x\n", base + i, *preg));
   }
   GeodeDebug(("CS5530 Display Registers:\n"));
   base = 0x24;
   for (i = 0; i <= 8; i += 4) {
      preg = (unsigned int *)(gfx_vidptr + base + i);
      DEBUGMSG(0, (0, X_NONE, "%x = %x\n", base + i, *preg));
   }
}

static unsigned char
gfx_inb(unsigned short port)
{
   unsigned char value;
   __asm__ volatile ("inb %1,%0":"=a" (value):"d"(port));

   return value;
}

static void
gfx_outb(unsigned short port, unsigned char data)
{
   __asm__ volatile ("outb %0,%1"::"a" (data), "d"(port));
}

static void
print_gxm_vga_reg()
{
   int i, index, data;

   GeodeDebug(("Geode CRT Registers:\n"));

   index = 0x3d4;
   data = 0x3d5;
   for (i = 0; i < STD_CRTC_REGS; i++) {
      OUTB(index, (unsigned char)i);
      DEBUGMSG(0, (0, X_NONE, "CRT[%x] = %x\n", i, INB(data)));
   }
   for (i = 0; i < EXT_CRTC_REGS; i++) {
      OUTB(index, (unsigned char)0x40 + i);
      DEBUGMSG(0, (0, X_NONE, "CRT[%x] = %x\n", 0x40 + i, INB(data)));
   }

   index = 0x3c4;
   data = 0x3c5;
   for (i = 0; i <= 4; i++) {
      OUTB(index, (unsigned char)i);
      DEBUGMSG(0, (0, X_NONE, "SR[%x] = %x\n", i, INB(data)));
   }

   index = 0x3ce;
   data = 0x3cf;
   for (i = 0; i <= 8; i++) {
      OUTB(index, (unsigned char)i);
      DEBUGMSG(0, (0, X_NONE, "GR[%x] = %x\n", i, INB(data)));
   }

#if 0
   index = 0x3c0;
   data = 0x3c1;
   for (i = 0; i <= 0x14; i++) {
      INB(0x3da);
      OUTB(index, (unsigned char)i);
      DEBUGMSG(0, (0, X_NONE, "AR[%x] = %x\n", i, INB(data)));
   }

   DEBUGMSG(0, (0, X_NONE, "MISC = %x\n", INB(0x3CC)));
#endif
}
#endif /* end of Geode_reg_settings */

/*----------------------------------------------------------------------------
 * GeodeGetRec.
 *
 * Description	:This function allocate an GeodeRec and hooked into
 * pScreenInfo 	 str driverPrivate member of ScreeenInfo
 * 				 structure.
 * Parameters.
 * pScreenInfo 	:Pointer handle to the screenonfo structure.
 *
 * Returns		:allocated pScreeninfo structure.
 *
 * Comments     :none
 *
*----------------------------------------------------------------------------
*/
static GeodePtr
GeodeGetRec(ScrnInfoPtr pScreenInfo)
{
   if (!pScreenInfo->driverPrivate)
      pScreenInfo->driverPrivate = xnfcalloc(sizeof(GeodeRec), 1);
   return GEODEPTR(pScreenInfo);
}

/*----------------------------------------------------------------------------
 * GeodeFreeRec.
 *
 * Description	:This function deallocate an GeodeRec and freed from
 *               pScreenInfo str driverPrivate member of ScreeenInfo
 *               structure.
 * Parameters.
 * pScreenInfo	:Pointer handle to the screenonfo structure.
 *
 * Returns		:none
 *
 * Comments     :none
 *
*----------------------------------------------------------------------------
*/
static void
GeodeFreeRec(ScrnInfoPtr pScreenInfo)
{
   if (pScreenInfo->driverPrivate == NULL) {
      return;
   }
   xfree(pScreenInfo->driverPrivate);
   pScreenInfo->driverPrivate = NULL;
}

/*-------------------------------------------------------------------------
 * GeodeIdentify.
 *
 * Description  :	This function identify an Geodefamily version.
 *
 *
 * Parameters.
 *    flags		:	flags may be used in GeodePreInit*
 *
 * Returns		: 	none
 *
 * Comments     : 	none
 *
*------------------------------------------------------------------------
*/
static void
GeodeIdentify(int flags)
{
   xf86PrintChipsets(GEODE_NAME,
		     "Geode family driver (version " GEODE_VERSION_NAME ") "
		     "for chipsets", GeodeChipsets);
}

/*----------------------------------------------------------------------------
 * GeodeAvailableOptions.
 *
 * Description	:This function returns the geodeoptions set geodeoption
 *
 * Parameters.
 *    chipid	:This will identify the chipset.
 *    busid     :This will identify the PCI busid
 *
 * Returns		:ptr to GeodeOptions.
 *
 * Comments     :none
 *
*----------------------------------------------------------------------------
*/
static const OptionInfoRec *
GeodeAvailableOptions(int chipid, int busid)
{
   return GeodeOptions;
}

/*----------------------------------------------------------------------------
 * GeodeProbe.
 *
 * Description	:This is to find that hardware is claimed by another
 *		 driver if not claim the slot & allocate ScreenInfoRec.
 *
 * Parameters.
 *     drv	:a pointer to the geode driver
 *     flags    :flags may passed to check the config and probe detect
 * 												
 * Returns	:TRUE on success and FALSE on failure.
 *
 * Comments     :This should ne minimal probe and it should under no
 *               circumstances change the state of the hardware.Don't do
 *               any intiallizations other than the required
 *               ScreenInforec.
*----------------------------------------------------------------------------
*/

static Bool
GeodeProbe(DriverPtr drv, int flags)
{
   Bool foundScreen = FALSE;
   int numDevSections, numUsed;
   GDevPtr *devSections = NULL;
   int *usedChips = NULL;
   int i;

   GeodeDebug(("GeodeProbe: Probing for supported devices!\n"));
   /*
    * *Find the config file Device sections that match this
    * *driver, and return if there are none.
    */
   if ((numDevSections = xf86MatchDevice(GEODE_NAME, &devSections)) <= 0) {
      GeodeDebug(("GeodeProbe: failed 1!\n"));
      return FALSE;
   }
   /* PCI BUS */
   if (xf86GetPciVideoInfo()) {
      numUsed = xf86MatchPciInstances(GEODE_NAME, PCI_VENDOR_ID_NS,
				      GeodeChipsets, GeodePCIchipsets,
				      devSections, numDevSections,
				      drv, &usedChips);
      if (numUsed <= 0) {
	 /* Check for old CYRIX vendor ID (5530) */
	 numUsed = xf86MatchPciInstances(GEODE_NAME,
					 PCI_VENDOR_ID_CYRIX,
					 GeodeChipsets, GeodePCIchipsets,
					 devSections, numDevSections,
					 drv, &usedChips);
      }

      if (numUsed > 0) {
	 if (flags & PROBE_DETECT)
	    foundScreen = TRUE;
	 else {
	    /* Durango only supports one instance, */
	    /* so take the first one */
	    for (i = 0; i < numUsed; i++) {
	       /* Allocate a ScrnInfoRec  */
	       ScrnInfoPtr pScrn = xf86AllocateScreen(drv, 0);

	       pScrn->driverVersion = GEODE_VERSION_CURRENT;
	       pScrn->driverName = GEODE_DRIVER_NAME;
	       pScrn->name = GEODE_NAME;
	       pScrn->Probe = GeodeProbe;
	       pScrn->PreInit = GeodePreInit;
	       pScrn->ScreenInit = GeodeScreenInit;
	       pScrn->SwitchMode = GeodeSwitchMode;
	       pScrn->AdjustFrame = GeodeAdjustFrame;
	       pScrn->EnterVT = GeodeEnterVT;
	       pScrn->LeaveVT = GeodeLeaveVT;
	       pScrn->FreeScreen = GeodeFreeScreen;
	       pScrn->ValidMode = GeodeValidMode;
	       foundScreen = TRUE;
	       xf86ConfigActivePciEntity(pScrn,
					 usedChips[i],
					 GeodePCIchipsets,
					 NULL, NULL, NULL, NULL, NULL);
	    }
	 }
      }
   }

   if (usedChips)
      xfree(usedChips);
   if (devSections)
      xfree(devSections);
   GeodeDebug(("GeodeProbe: result (%d)!\n", foundScreen));
   return foundScreen;
}

/*----------------------------------------------------------------------------
 * GeodeSaveScreen.
 *
 * Description	:This is todo the screen blanking
 *
 * Parameters.
 *     pScreen	:Handle to ScreenPtr structure.
 *     mode		:mode is used by vgaHWSaveScren to check blnak os on.
 * 												
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments     :none
*----------------------------------------------------------------------------
*/
static Bool
GeodeSaveScreen(ScreenPtr pScreen, int mode)
{
   GeodeDebug(("GeodeSaveScreen!\n"));
   return vgaHWSaveScreen(pScreen, mode);
}

/*----------------------------------------------------------------------------
 * get_tv_overscan_geom.
 *
 * Description	:This is todo the screen blanking
 *
 *    Parameters:
 *     options	: Pointer to the display options.
 *             X: Pointer to the offset of the screen X-co-ordinate.
 *             Y: Pointer to the offset of the screen Y-co-ordinate.
 * 	       W: Pointer to the width of the screen.
 *             H: Pointer to the height of the screen. 
 * Returns	: none.
 *
 * Comments     :none
 *------------------------------------------------------------------------
 */
void
get_tv_overscan_geom(const char *options, int *X, int *Y, int *W, int *H)
{
   char *tv_opt;

   tv_opt = strtok((char *)options, ":");
   *X = strtoul(tv_opt, NULL, 0);
   tv_opt = strtok(NULL, ":");
   *Y = strtoul(tv_opt, NULL, 0);
   tv_opt = strtok(NULL, ":");
   *W = strtoul(tv_opt, NULL, 0);
   tv_opt = strtok(NULL, ":");
   *H = strtoul(tv_opt, NULL, 0);
}

static void
GeodeProbeDDC(ScrnInfoPtr pScrn, int index)
{
   vbeInfoPtr pVbe;

   if (xf86LoadSubModule(pScrn, "vbe")) {
      pVbe = VBEInit(NULL, index);
      ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
      vbeFree(pVbe);
   }
}

/*----------------------------------------------------------------------------
 * GeodePreInit.
 *
 * Description	:This function is called only once ate teh server startup
 *
 * Parameters.
 *  pScreenInfo :Handle to ScreenPtr structure.
 *  flags       :flags may be used to check the probeed one with config.
 * 												
 * Returns		:TRUE on success and FALSE on failure.
 *
 * Comments     :none.
 *----------------------------------------------------------------------------
 */
static Bool
GeodePreInit(ScrnInfoPtr pScreenInfo, int flags)
{
   static ClockRange GeodeClockRange =
	 { NULL, 25175, 135000, 0, FALSE, TRUE, 1, 1, 0 };
   MessageType from;
   int i = 0;
   GeodePtr pGeode;
   char *mod = NULL;
   char *reqSymbol = NULL;

#if defined(STB_X)
   GAL_ADAPTERINFO sAdapterInfo;
#endif /* STB_X */
   unsigned int PitchInc, minPitch, maxPitch;
   const char *s;
   char **modes;
   char **tvmodes_defa;

   GeodeDebug(("GeodePreInit!\n"));
   /* Allocate driver private structure */
   if (!(pGeode = GeodeGetRec(pScreenInfo)))
      return FALSE;

   /* This is the general case */
   for (i = 0; i < pScreenInfo->numEntities; i++) {
      pGeode->pEnt = xf86GetEntityInfo(pScreenInfo->entityList[i]);
      if (pGeode->pEnt->resources)
	 return FALSE;
      pGeode->Chipset = pGeode->pEnt->chipset;
      pScreenInfo->chipset = (char *)xf86TokenToString(GeodeChipsets,
						       pGeode->pEnt->chipset);
   }

   if (flags & PROBE_DETECT) {
      GeodeProbeDDC(pScreenInfo, pGeode->pEnt->index);
      return TRUE;
   }

   /* If the vgahw module would be needed it would be loaded here */
   if (!xf86LoadSubModule(pScreenInfo, "vgahw")) {
      return FALSE;
   }
   xf86LoaderReqSymLists(vgahwSymbols, NULL);
   GeodeDebug(("GeodePreInit(1)!\n"));
   /* Do the durango hardware detection */
#if defined(STB_X)
   if (!Gal_initialize_interface())
      return FALSE;
   if (Gal_get_adapter_info(&sAdapterInfo)) {
      pGeode->cpu_version = sAdapterInfo.dwCPUVersion;
      pGeode->vid_version = sAdapterInfo.dwVideoVersion;
      pGeode->FBSize = sAdapterInfo.dwFrameBufferSize;
      /* update the max clock from the one system suports  */
      GeodeClockRange.maxClock = sAdapterInfo.dwMaxSupportedPixelClock;
      pGeode->FBLinearAddr = sAdapterInfo.dwFrameBufferBase;

      if (!GeodeMapMem(pScreenInfo))
	 return FALSE;

      pGeode->HWVideo = pGeode->vid_version;
   } else {
      return FALSE;
   }
#else
   pGeode->cpu_version = gfx_detect_cpu();
   pGeode->vid_version = gfx_detect_video();
   pGeode->FBLinearAddr = gfx_get_frame_buffer_base();
   pGeode->FBSize = gfx_get_frame_buffer_size();
   /* update the max clock from the one system suports  */
   GeodeClockRange.maxClock = gfx_get_max_supported_pixel_clock();

   if (!GeodeMapMem(pScreenInfo))
      return FALSE;

   /* does this make sense!?!? */
   pGeode->HWVideo = gfx_detect_video();

   DEBUGMSG(1,
	    (0, X_INFO,
	     "Geode chip info: cpu:%x vid:%x size:%x base:%x, rom:%X\n",
	     pGeode->cpu_version, pGeode->vid_version, pGeode->FBSize,
	     pGeode->FBBase, XpressROMPtr));
#endif /* STB_X */

   /* Fill in the monitor field */
   pScreenInfo->monitor = pScreenInfo->confScreen->monitor;
   GeodeDebug(("GeodePreInit(2)!\n"));
   /* Determine depth, bpp, etc. */
   if (!xf86SetDepthBpp(pScreenInfo, 8, 8, 8, 0)) {
      return FALSE;
   } else {
      switch (pScreenInfo->depth) {
      case 8:
      case 16:
	 break;
      default:
	 /* Depth not supported */
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_ERROR,
		      "Given depth (%d bpp) is not supported by this driver\n",
		      pScreenInfo->depth));
	 return FALSE;
      }
   }

   /*This must happen after pScreenInfo->display has been set
    * * because xf86SetWeight references it.
    */
   if (pScreenInfo->depth > 8) {
      /* The defaults are OK for us */
      rgb zeros = { 0, 0, 0 };

      if (!xf86SetWeight(pScreenInfo, zeros, zeros)) {
	 return FALSE;
      } else {
	 /* XXX Check if the returned weight is supported */
      }
   }
   xf86PrintDepthBpp(pScreenInfo);
   GeodeDebug(("GeodePreInit(3)!\n"));
   if (!xf86SetDefaultVisual(pScreenInfo, -1))
      return FALSE;
   GeodeDebug(("GeodePreInit(4)!\n"));

   /* The new cmap layer needs this to be initialized */
   if (pScreenInfo->depth > 1) {
      Gamma zeros = { 0.0, 0.0, 0.0 };

      if (!xf86SetGamma(pScreenInfo, zeros)) {
	 return FALSE;
      }
   }
   GeodeDebug(("GeodePreInit(5)!\n"));
   /* We use a programmable clock */
   pScreenInfo->progClock = TRUE;

   /*Collect all of the relevant option flags
    * *(fill in pScreenInfo->options)
    */
   xf86CollectOptions(pScreenInfo, NULL);

   /*Process the options */
   xf86ProcessOptions(pScreenInfo->scrnIndex, pScreenInfo->options,
		      GeodeOptions);

   /*Set the bits per RGB for 8bpp mode */
   if (pScreenInfo->depth == 8) {
      /* Default to 8 */
      pScreenInfo->rgbBits = 8;
   }
   from = X_DEFAULT;
   pGeode->HWCursor = TRUE;
   /*
    * *The preferred method is to use the "hw cursor" option as a tri-state
    * *option, with the default set above.
    */
   if (xf86GetOptValBool(GeodeOptions, OPTION_HW_CURSOR, &pGeode->HWCursor)) {
      from = X_CONFIG;
   }
   /* For compatibility, accept this too (as an override) */
   if (xf86ReturnOptValBool(GeodeOptions, OPTION_SW_CURSOR, FALSE)) {
      from = X_CONFIG;
      pGeode->HWCursor = FALSE;
   }
   DEBUGMSG(1, (pScreenInfo->scrnIndex, from, "Using %s cursor\n",
		pGeode->HWCursor ? "HW" : "SW"));

   pGeode->Compression = FALSE;
   if (xf86ReturnOptValBool(GeodeOptions, OPTION_COMPRESSION, FALSE)) {
      pGeode->Compression = TRUE;
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG, "Compression \
			enabled\n"));
   }

   pGeode->NoAccel = FALSE;
   if (xf86ReturnOptValBool(GeodeOptions, OPTION_NOACCEL, FALSE)) {
      pGeode->NoAccel = TRUE;
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG, "Acceleration \
			disabled\n"));
   }

   pGeode->TVSupport = FALSE;
   if ((s = xf86GetOptValString(GeodeOptions, OPTION_TV_SUPPORT))) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG, "TV = %s\n", s));
      if (!xf86NameCmp(s, "PAL-768x576")) {
	 pGeode->TvParam.wStandard = TV_STANDARD_PAL;
	 pGeode->TvParam.wType = GFX_ON_TV_SQUARE_PIXELS;
	 pGeode->TvParam.wWidth = 768;
	 pGeode->TvParam.wHeight = 576;
	 pGeode->TVSupport = TRUE;
      } else if (!xf86NameCmp(s, "PAL-720x576")) {
	 pGeode->TvParam.wStandard = TV_STANDARD_PAL;
	 pGeode->TvParam.wType = GFX_ON_TV_NO_SCALING;
	 pGeode->TvParam.wWidth = 720;
	 pGeode->TvParam.wHeight = 576;
	 pGeode->TVSupport = TRUE;
      } else if (!xf86NameCmp(s, "NTSC-640x480")) {
	 pGeode->TvParam.wStandard = TV_STANDARD_NTSC;
	 pGeode->TvParam.wType = GFX_ON_TV_SQUARE_PIXELS;
	 pGeode->TvParam.wWidth = 640;
	 pGeode->TvParam.wHeight = 480;
	 pGeode->TVSupport = TRUE;
      } else if (!xf86NameCmp(s, "NTSC-720x480")) {
	 pGeode->TvParam.wStandard = TV_STANDARD_NTSC;
	 pGeode->TvParam.wType = GFX_ON_TV_NO_SCALING;
	 pGeode->TvParam.wWidth = 720;
	 pGeode->TvParam.wHeight = 480;
	 pGeode->TVSupport = TRUE;
      }

      if (pGeode->TVSupport == TRUE) {
	 unsigned int status = 0;

	 GFX(get_tv_enable(&status));

	 DEBUGMSG(1, (0, X_PROBED, "status %d \n", status));
	 if (status == 0) {
	    pGeode->TVSupport = FALSE;
	    DEBUGMSG(1, (0, X_NONE, "Not a TV Supported Platform\n"));
	 }
	 pGeode->TvParam.wOutput = TV_OUTPUT_S_VIDEO;	/* default */
	 /* Now find the output */
	 if (pGeode->TVSupport) {
	    if ((s = xf86GetOptValString(GeodeOptions, OPTION_TV_OUTPUT))) {
	       if (!xf86NameCmp(s, "COMPOSITE")) {
		  pGeode->TvParam.wOutput = TV_OUTPUT_COMPOSITE;
	       } else if (!xf86NameCmp(s, "SVIDEO")) {
		  pGeode->TvParam.wOutput = TV_OUTPUT_S_VIDEO;
	       } else if (!xf86NameCmp(s, "YUV")) {
		  pGeode->TvParam.wOutput = TV_OUTPUT_YUV;
	       } else if (!xf86NameCmp(s, "SCART")) {
		  pGeode->TvParam.wOutput = TV_OUTPUT_SCART;
	       }
	       DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
			    "TVOutput = %s %d\n", s,
			    pGeode->TvParam.wOutput));
	    }
	 }
      }

      /*TV can be turned on only in 16BPP mode */
      if ((pScreenInfo->depth == 8) && (pGeode->TVSupport == TRUE)) {
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		      "Warning TV Disabled, TV Can't be supported in 8Bpp !!!\n"));
	 pGeode->TVSupport = FALSE;
      }
   }

   /* If TV Supported then check for TVO support */
   if (pGeode->TVSupport == TRUE) {
      pGeode->TVOx = 0;
      pGeode->TVOy = 0;
      pGeode->TVOw = 0;
      pGeode->TVOh = 0;
      pGeode->TV_Overscan_On = FALSE;
      if ((s = xf86GetOptValString(GeodeOptions, OPTION_TV_OVERSCAN))) {
	 get_tv_overscan_geom(s, &(pGeode->TVOx),
			      &(pGeode->TVOy), &(pGeode->TVOw),
			      &(pGeode->TVOh));

	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		      "TVO %d %d %d %d\n", pGeode->TVOx, pGeode->TVOy,
		      pGeode->TVOw, pGeode->TVOh));

	 if ((pGeode->TVOx >= 0 && pGeode->TVOy >= 0) &&
	     (pGeode->TVOh > 0 && pGeode->TVOw > 0)) {
	    if (((pGeode->TVOx + pGeode->TVOw) <= pGeode->TvParam.wWidth) &&
		((pGeode->TVOy + pGeode->TVOh) <= pGeode->TvParam.wHeight)) {
	       pGeode->TV_Overscan_On = TRUE;
	    }
	 }
      }
   }

   /* If TV is not selected and currently TV is enabled, disable the TV out */
   if (pGeode->TVSupport == FALSE) {
      unsigned int status = 0;

      GFX(get_tv_enable(&status));
      if (status)
	 GFX(set_tv_enable(0));
   }

   pGeode->NoOfImgBuffers = DEFAULT_NUM_OF_BUF; /* default # of buffers */
   if(!xf86GetOptValInteger(GeodeOptions, OPTION_IMG_BUFS, 
						&(pGeode->NoOfImgBuffers))) {
      if(pGeode->NoOfImgBuffers < 0)
         pGeode->NoOfImgBuffers = DEFAULT_NUM_OF_BUF; /* default # of buffers */
   }

   pGeode->Panel = FALSE;
   if (xf86ReturnOptValBool(GeodeOptions, OPTION_FLATPANEL, FALSE)) {
      DEBUGMSG(0, (pScreenInfo->scrnIndex, X_CONFIG, "FlatPanel Selected\n"));
      pGeode->Panel = TRUE;
   }

   pGeode->FPInBios = FALSE;
   if (xf86ReturnOptValBool(GeodeOptions, OPTION_FLATPANEL_IN_BIOS, FALSE)) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		   "FlatPanel Support from BIOS\n"));
      pGeode->FPInBios = TRUE;
   }

   DEBUGMSG(0, (pScreenInfo->scrnIndex, X_CONFIG,
		"FP Bios %d %d\n", pGeode->FPInBios, pGeode->Panel));

   /* if FP not supported in BIOS, then turn off user option */
   if (pGeode->FPInBios) {
      int ret;

      /* check if bios supports FP */
#if defined(STB_X)
      Gal_get_softvga_state(&ret);
      if (!ret) {
	 /* its time to wakeup softvga */
	 Gal_set_softvga_state(TRUE);
	 Gal_vga_mode_switch(0);
      }
      Gal_pnl_enabled_in_bios(&pGeode->FPInBios);

      if (pGeode->FPInBios) {
	 Gal_pnl_info_from_bios(&pGeode->FPBX, &pGeode->FPBY,
				&pGeode->FPBB, &pGeode->FPBF);
      }
      if (!ret) {
	 /* its time to put softvga back to sleep */
	 Gal_set_softvga_state(FALSE);
	 Gal_vga_mode_switch(1);
      }
#else
      ret = gfx_get_softvga_active();
      if (!ret) {
	 /* its time to wakeup softvga */
	 gfx_enable_softvga();
	 gfx_vga_mode_switch(0);
      }
      pGeode->FPInBios = Pnl_IsPanelEnabledInBIOS();
      if (pGeode->FPInBios) {
	 Pnl_GetPanelInfoFromBIOS(&pGeode->FPBX, &pGeode->FPBY,
				  &pGeode->FPBB, &pGeode->FPBF);
      }
      if (!ret) {
	 /* its time to put softvga back to sleep */
	 gfx_disable_softvga();
	 gfx_vga_mode_switch(1);
      }
#endif
      if (pGeode->FPInBios == 0) {
	 pGeode->Panel = 0;
      } else {
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_PROBED,
		      "Flat Panel Dimensions (from BIOS) %dx%d\n",
		      pGeode->FPBX, pGeode->FPBY));
      }
   }

   DEBUGMSG(0, (pScreenInfo->scrnIndex, X_CONFIG,
		"FP Bios %d %d\n", pGeode->FPInBios, pGeode->Panel));

   /* if Panel enabled and Flatpanel support not from BIOS then 
    * detect if the platform can support a panel */

   if (pGeode->Panel && (!pGeode->FPInBios)) {
      pGeode->PnlParam.PanelPresent = 1;
#if defined(STB_X)
      Gal_pnl_get_params(PNL_PLATFORM | PNL_PANELCHIP | PNL_PANELSTAT,
			 &(pGeode->PnlParam));
#else
      pGeode->PnlParam.Flags = PNL_PLATFORM | PNL_PANELCHIP | PNL_PANELSTAT;

      Pnl_GetPanelParam(&(pGeode->PnlParam));
#endif /* STB_X */
      DEBUGMSG(0, (pScreenInfo->scrnIndex, X_CONFIG,
		   "chip = %X\n", pGeode->PnlParam.PanelChip));
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		   "%s Platform with %s Chip, %s Panel %dx%d %dBpp%sColor \n",
		   ((pGeode->PnlParam.Platform ==
		     CENTAURUS_PLATFORM) ? "Centaurus" : ((pGeode->PnlParam.
							   Platform ==
							   DORADO_PLATFORM) ?
							  "Dorado" :
							  "Unknown")),
		   ((pGeode->PnlParam.
		     PanelChip & PNL_9210) ? "9210" : ((pGeode->PnlParam.
							PanelChip &
							PNL_9211_A) ? "9211_A"
						       : ((pGeode->PnlParam.
							   PanelChip &
							   PNL_9211_C) ?
							  "9211_C" :
							  "Unknown"))),
		   ((pGeode->PnlParam.PanelStat.
		     Type & PNL_TFT) ? "TFT" : ((pGeode->PnlParam.PanelStat.
						 Type & PNL_SSTN) ? "SSTN"
						: ((pGeode->PnlParam.
						    PanelStat.
						    Type & PNL_DSTN) ? "DSTN"
						   : "Unknown"))),
		   pGeode->PnlParam.PanelStat.XRes,
		   pGeode->PnlParam.PanelStat.YRes,
		   pGeode->PnlParam.PanelStat.Depth,
		   ((pGeode->PnlParam.PanelStat.
		     MonoColor & PNL_MONO_PANEL) ? " Mono" : ((pGeode->
							       PnlParam.
							       PanelStat.
							       MonoColor &
							       PNL_COLOR_PANEL)
							      ? " " :
							      " Unknown"))
	       ));

      /* check if we found a vaild configuration */
      if ((pGeode->PnlParam.Platform == OTHER_PLATFORM) ||
	  (pGeode->PnlParam.PanelChip & PNL_UNKNOWN_CHIP) ||
	  (pGeode->PnlParam.PanelStat.Type & PNL_UNKNOWN_PANEL) ||
	  (pGeode->PnlParam.PanelStat.MonoColor & PNL_UNKNOWN_COLOR)) {
	 pGeode->Panel = FALSE;
      }
   }

   DEBUGMSG(0, (0, X_CONFIG, "FP Bios pGeode->Panel %d \n", pGeode->Panel));

   /* if panel not selected and Panel can be  supported. 
    * Power down the panel. 
    */
   if (!pGeode->Panel) {
#if defined(STB_X)
      Gal_pnl_powerdown();
#else
      Pnl_PowerDown();
#endif /* STB_X */
   }

   pGeode->ShadowFB = FALSE;
   if (xf86ReturnOptValBool(GeodeOptions, OPTION_SHADOW_FB, FALSE)) {
      pGeode->ShadowFB = TRUE;
      pGeode->NoAccel = TRUE;
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		   "Using \"Shadow Framebuffer\" - acceleration disabled\n"));
   }

   if (xf86ReturnOptValBool(GeodeOptions, OPTION_DONT_PROGRAM, FALSE)) {
      gfx_dont_program = 1;
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		   "Don't Program enabled\n"));
   } else
      gfx_dont_program = 0;

   pGeode->Rotate = 0;
   if ((s = xf86GetOptValString(GeodeOptions, OPTION_ROTATE))) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG, "Rotating - %s\n", s));
      if (!xf86NameCmp(s, "CW")) {
	 pGeode->ShadowFB = TRUE;
	 pGeode->NoAccel = TRUE;
	 pGeode->HWCursor = FALSE;
	 pGeode->Rotate = 1;
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		      "Rotating screen clockwise - acceleration disabled\n"));
      } else {
	 if (!xf86NameCmp(s, "CCW")) {
	    pGeode->ShadowFB = TRUE;
	    pGeode->NoAccel = TRUE;
	    pGeode->HWCursor = FALSE;
	    pGeode->Rotate = -1;
	    DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
			 "Rotating screen counter clockwise - acceleration \
					disabled\n"));
	 } else {
	    DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
			 "\"%s\" is not a valid value for Option \"Rotate\"\n",
			 s));
	    DEBUGMSG(1,
		     (pScreenInfo->scrnIndex, X_INFO,
		      "Valid options are \"CW\" or \"CCW\"\n"));
	 }
      }
   }

   if ((s = xf86GetOptValString(GeodeOptions, OPTION_OVERLAY))) {
      if (!*s || !xf86NameCmp(s, "8,16") || !xf86NameCmp(s, "16,8")) {
	 if (pScreenInfo->bitsPerPixel == 16) {
	    pGeode->HWOverlay = TRUE;
	    if (!xf86GetOptValInteger
		(GeodeOptions, OPTION_COLOR_KEY, &(pScreenInfo->colorKey)))
	       pScreenInfo->colorKey = TRANSPARENCY_KEY;
	    pScreenInfo->overlayFlags = OVERLAY_8_16_DUALFB;
	    DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
			 "PseudoColor overlay enabled.\n"));
	 } else {
	    DEBUGMSG(1, (pScreenInfo->scrnIndex, X_WARNING,
			 "Option \"Overlay\" is not supported in this configuration\n"));
	 }
      } else {
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_WARNING,
		      "\"%s\" is not a valid value for Option \"Overlay\"\n",
		      s));
      }
   }
   if (1 || pGeode->HWOverlay) {
      if (xf86GetOptValInteger(GeodeOptions, OPTION_VIDEO_KEY,
			       &(pGeode->videoKey))) {
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_CONFIG,
		      "video key set to 0x%x\n", pGeode->videoKey));
      } else {
	 pGeode->videoKey = (1 << pScreenInfo->offset.red) |
	       (1 << pScreenInfo->offset.green) |
	       (((pScreenInfo->mask.blue >> pScreenInfo->offset.blue) - 1)
		<< pScreenInfo->offset.blue);
      }
   }

   /* XXX Init further private data here */

   /*
    * * This shouldn't happen because such problems should be caught in
    * * GeodeProbe(), but check it just in case.
    */
   if (pScreenInfo->chipset == NULL) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pGeode->Chipset));
      return FALSE;
   }
   if (pGeode->Chipset < 0) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n",
		   pScreenInfo->chipset));
      return FALSE;
   }
   GeodeDebug(("GeodePreInit(6)!\n"));

   /*
    * * Init the screen with some values
    */
   DEBUGMSG(1, (pScreenInfo->scrnIndex, from,
		"Video I/O registers at 0x%08lX\n",
		(unsigned long)VGAHW_GET_IOBASE()));

   if (pScreenInfo->memPhysBase == 0) {
      from = X_PROBED;
#if defined(STB_X)
      pScreenInfo->memPhysBase = sAdapterInfo.dwFrameBufferBase;
#else
      pScreenInfo->memPhysBase = gfx_get_frame_buffer_base();
#endif /* STB_X */
   }
   DEBUGMSG(1, (pScreenInfo->scrnIndex, from,
		"Linear framebuffer at 0x%08lX\n",
		(unsigned long)pScreenInfo->memPhysBase));

   if (pGeode->pEnt->device->videoRam == 0) {
      from = X_PROBED;
      pScreenInfo->videoRam = pGeode->FBSize / 1024;
   } else {
      pScreenInfo->videoRam = pGeode->pEnt->device->videoRam;
      from = X_CONFIG;
   }
   DEBUGMSG(1, (pScreenInfo->scrnIndex, from,
		"VideoRam: %d kByte\n",
		(unsigned long)pScreenInfo->videoRam));

   GeodeDebug(("GeodePreInit(7)!\n"));

   /*
    * * xf86ValidateModes will check that the mode HTotal and VTotal values
    * * don't exceed the chipset's limit if pScreenInfo->maxHValue adn
    * * pScreenInfo->maxVValue are set. Since our GeodeValidMode()
    * * already takes care of this, we don't worry about setting them here.
    */
   /* Select valid modes from those available */
   /*
    * * min pitch 1024, max 2048 (Pixel count)
    * * min height 480, max 1024 (Pixel count)
    */
   minPitch = 1024;
   maxPitch = 2048;
   if (pScreenInfo->depth == 16) {
      PitchInc = 1024 << 4;		/* in bits */
   } else {
      PitchInc = 1024 << 3;		/* in bits */
   }

   /* by default use what user sets in the XF86Config file */
   modes = pScreenInfo->display->modes;
   if (pGeode->TVSupport == TRUE) {
      char str[20];

      sprintf(str, "%dx%d-%d",
	      pGeode->TvParam.wWidth,
	      pGeode->TvParam.wHeight,
	      ((pGeode->TvParam.wStandard == TV_STANDARD_PAL) ? 50 : 60));

      tvmodes_defa = (char **)malloc(sizeof(char *) * 2);
      tvmodes_defa[0] = (char *)malloc(strlen(str));
      tvmodes_defa[1] = NULL;
      strcpy(str, tvmodes_defa[0]);

      modes = tvmodes_defa;
   }

   i = xf86ValidateModes(pScreenInfo,
			 pScreenInfo->monitor->Modes,
			 modes,
			 &GeodeClockRange,
			 NULL, minPitch, maxPitch,
			 PitchInc, 480, 1024,
			 pScreenInfo->display->virtualX,
			 pScreenInfo->display->virtualY,
#if defined(STB_X)
			 sAdapterInfo.dwFrameBufferSize,
#else
			 gfx_get_frame_buffer_size(),
#endif /* STB_X */
			 LOOKUP_BEST_REFRESH);

   DEBUGMSG(1, (pScreenInfo->scrnIndex, from,
		"xf86ValidateModes: %d %d %d\n",
		pScreenInfo->virtualX,
		pScreenInfo->virtualY, pScreenInfo->displayWidth));
   if (i == -1) {
      GeodeFreeRec(pScreenInfo);
      return FALSE;
   }
   GeodeDebug(("GeodePreInit(8)!\n"));
   /* Prune the modes marked as invalid */
   xf86PruneDriverModes(pScreenInfo);
   GeodeDebug(("GeodePreInit(9)!\n"));
   if (i == 0 || pScreenInfo->modes == NULL) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_ERROR,
		   "No valid modes found\n"));
      GeodeFreeRec(pScreenInfo);
      return FALSE;
   }
   GeodeDebug(("GeodePreInit(10)!\n"));
   xf86SetCrtcForModes(pScreenInfo, 0);
   GeodeDebug(("GeodePreInit(11)!\n"));
   /* Set the current mode to the first in the list */
   pScreenInfo->currentMode = pScreenInfo->modes;
   GeodeDebug(("GeodePreInit(12)!\n"));
   /* Print the list of modes being used */
   xf86PrintModes(pScreenInfo);
   GeodeDebug(("GeodePreInit(13)!\n"));
   /* Set the display resolution */
   xf86SetDpi(pScreenInfo, 0, 0);
   GeodeDebug(("GeodePreInit(14)!\n"));
   /* Load bpp-specific modules */
   mod = NULL;
   switch (pScreenInfo->bitsPerPixel) {
   case 8:
   case 16:
      mod = "fb";
      reqSymbol = "fbScreenInit";
      break;
   }
   if (mod && !xf86LoadSubModule(pScreenInfo, mod)) {
      GeodeFreeRec(pScreenInfo);
      return FALSE;
   }
   xf86LoaderReqSymbols(reqSymbol, NULL);
   GeodeDebug(("GeodePreInit(15)!\n"));
   if (pGeode->NoAccel == FALSE) {
      if (!xf86LoadSubModule(pScreenInfo, "xaa")) {
	 GeodeFreeRec(pScreenInfo);
	 return FALSE;
      }
      xf86LoaderReqSymLists(xaaSymbols, NULL);
   }
   if (pGeode->HWCursor == TRUE) {
      if (!xf86LoadSubModule(pScreenInfo, "ramdac")) {
	 GeodeFreeRec(pScreenInfo);
	 return FALSE;
      }
      xf86LoaderReqSymLists(ramdacSymbols, NULL);
   }
   /* Load shadowfb if needed */
   if (pGeode->ShadowFB) {
      if (!xf86LoadSubModule(pScreenInfo, "shadowfb")) {
	 GeodeFreeRec(pScreenInfo);
	 return FALSE;
      }
      xf86LoaderReqSymLists(shadowSymbols, NULL);
   }
   if (xf86RegisterResources(pGeode->pEnt->index, NULL, ResExclusive)) {
      DEBUGMSG(1, (pScreenInfo->scrnIndex, X_ERROR,
		   "xf86RegisterResources() found resource conflicts\n"));
      GeodeFreeRec(pScreenInfo);
      return FALSE;
   }

   GeodeUnmapMem(pScreenInfo);

   GeodeDebug(("GeodePreInit(16)!\n"));
   GeodeDebug(("GeodePreInit(17)!\n"));
   GeodeDebug(("GeodePreInit ... done successfully!\n"));
   return TRUE;
}

#if 0
/*----------------------------------------------------------------------------
 * GeodeRestoreEx.
 *
 * Description	:This function restores the mode that was saved on server
                 entry
 * Parameters.
 * pScreenInfo 	:Handle to ScreenPtr structure.
 *  Pmode       :poits to screen mode
 * 												
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static void
GeodeRestoreEx(ScrnInfoPtr pScreenInfo, DisplayModePtr pMode)
{
   GeodePtr pGeode;

   GeodeDebug(("GeodeRestoreEx!\n"));
   /* Get driver private structure */
   if (!(pGeode = GeodeGetRec(pScreenInfo)))
      return;
   /* Restore the extended registers */
#if defined(STB_X)
   pGeode->FBgfxVgaRegs.dwFlags = GFX_VGA_FLAG_MISC_OUTPUT |
	 GFX_VGA_FLAG_STD_CRTC | GFX_VGA_FLAG_EXT_CRTC;
   Gal_vga_restore(&(pGeode->FBgfxVgaRegs));
#else
   gfx_vga_restore(&(pGeode->FBgfxVgaRegs),
		   GFX_VGA_FLAG_MISC_OUTPUT |
		   GFX_VGA_FLAG_STD_CRTC | GFX_VGA_FLAG_EXT_CRTC);
#endif /* STB_X */
}
#endif

/*----------------------------------------------------------------------------
 * GeodeCalculatePitchBytes.
 *
 * Description	:This function restores the mode that was saved on server
 *
 * Parameters.
 * pScreenInfo 	:Handle to ScreenPtr structure.
 *    Pmode     :Points to screenmode
 * 									
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static int
GeodeCalculatePitchBytes(ScrnInfoPtr pScreenInfo)
{
   GeodePtr pGeode;
   int lineDelta = pScreenInfo->virtualX * (pScreenInfo->bitsPerPixel >> 3);

   pGeode = GEODEPTR(pScreenInfo);

   /* needed in Rotate mode when in accel is turned off */
   if (1) {				/*!pGeode->NoAccel */
      /* Force to 1K or 2K if acceleration is enabled
       * we should check for pyramid here!!
       */
      if (lineDelta > 2048)
	 lineDelta = 4096;
      else if (lineDelta > 1024)
	 lineDelta = 2048;
      else
	 lineDelta = 1024;
   }

   DEBUGMSG(1,
	    (0, X_PROBED, "pitch %d %d\n", pScreenInfo->virtualX, lineDelta));

   return lineDelta;
}

/*----------------------------------------------------------------------------
 * GeodeGetRefreshRate.
 *
 * Description	:This function restores the mode that saved on server
 *
 * Parameters.
 *     Pmode    :Pointer to the screen modes
 * 												
 * Returns		:It returns the selected refresh rate.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static int
GeodeGetRefreshRate(DisplayModePtr pMode)
{
#define THRESHOLD 2
   int i;
   static int validRates[] = { 50, 56, 60, 70, 72, 75, 85 };	/* Hz */
   unsigned long dotClock;
   int refreshRate;
   int selectedRate;

   dotClock = pMode->SynthClock * 1000;
   refreshRate = dotClock / pMode->CrtcHTotal / pMode->CrtcVTotal;
   selectedRate = validRates[0];
   for (i = 0; i < (sizeof(validRates) / sizeof(validRates[0])); i++) {
      if (validRates[i] < (refreshRate + THRESHOLD)) {
	 selectedRate = validRates[i];
      }
   }
   return selectedRate;
}

#if 1
static void
clear_screen(int width, int height)
{
   /* clean up the frame buffer memory */

   GFX(set_solid_pattern(0));
   GFX(set_raster_operation(0xF0));
   GFX(pattern_fill(0, 0, width, height));
}
#endif

/*----------------------------------------------------------------------------
 * GeodeSetMode.
 *
 * Description	:This function sets parametrs for screen mode
 *
 * Parameters.
 * pScreenInfo 	:Pointer to the screenInfo structure.
 *	 Pmode      :Pointer to the screen modes
 * 												
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/

static Bool
GeodeSetMode(ScrnInfoPtr pScreenInfo, DisplayModePtr pMode)
{
   GeodePtr pGeode;

   /* unsigned int compOffset, compPitch, compSize; */
   GeodeDebug(("GeodeSetMode!\n"));
   pGeode = GEODEPTR(pScreenInfo);

   /* Set the VT semaphore */
   pScreenInfo->vtSema = TRUE;

   /* The timing will be adjusted later */
   GeodeDebug(("Set display mode: %dx%d-%d (%dHz)\n",
	       pMode->CrtcHDisplay,
	       pMode->CrtcVDisplay,
	       pScreenInfo->bitsPerPixel, GeodeGetRefreshRate(pMode)));
   GeodeDebug(("Before setting the mode\n"));
#if 0
   if ((pMode->CrtcHDisplay >= 640) && (pMode->CrtcVDisplay >= 480))
      if (1) {
#endif
	 GFX(set_display_bpp(pScreenInfo->bitsPerPixel));

	 if (pGeode->TVSupport == TRUE) {
	    pGeode->TvParam.bState = 1;	/* enable */
	    /* wWidth and wHeight already set in init. */
#if defined(STB_X)
	    Gal_tv_set_params(GAL_TVSTATE | GAL_TVOUTPUT |
			      GAL_TVFORMAT | GAL_TVRESOLUTION,
			      &(pGeode->TvParam));
#else
	    /* sequence might be important */
	    gfx_set_tv_display(pGeode->TvParam.wWidth,
			       pGeode->TvParam.wHeight);
	    gfx_set_tv_format(pGeode->TvParam.wStandard,
			      pGeode->TvParam.wType);
	    gfx_set_tv_output(pGeode->TvParam.wOutput);
	    gfx_set_tv_enable(pGeode->TvParam.bState);

#endif /* STB_X */
	 } else {			/* TV not selected */

	    DEBUGMSG(0, (0, X_PROBED, "Setting Display for CRT or TFT\n"));

	    if (pGeode->Panel) {
	       DEBUGMSG(0, (0, X_PROBED, "Setting Display for TFT\n"));
	       if (gfx_dont_program) {
		  if (pGeode->FPBX != pMode->CrtcHDisplay &&
		      pGeode->FPBY != pMode->CrtcVDisplay) {
		     gfx_dont_program = 0;
		     DEBUGMSG(1,
			      (1, X_PROBED,
			       "Disabling Dont Program - Panel/Res mismatch %dx%d != %dx%d\n",
			       pGeode->FPBX, pGeode->FPBY,
			       pMode->CrtcHDisplay, pMode->CrtcVDisplay));
		  }
	       }
#if defined(STB_X)
	       if (pGeode->FPInBios) {
		  DEBUGMSG(0, (0, X_PROBED, "Restore Panel %d %d %d %d %d\n",
			       pGeode->FPBX,
			       pGeode->FPBY,
			       pMode->CrtcHDisplay,
			       pMode->CrtcVDisplay,
			       pScreenInfo->bitsPerPixel));
		  Gal_set_fixed_timings(pGeode->FPBX, pGeode->FPBY,
					pMode->CrtcHDisplay,
					pMode->CrtcVDisplay,
					pScreenInfo->bitsPerPixel);
	       } else {
		  DEBUGMSG(0, (0, X_PROBED, "Restore Panel %d %d %d %d %d\n",
			       pGeode->PnlParam.PanelStat.XRes,
			       pGeode->PnlParam.PanelStat.YRes,
			       pMode->CrtcHDisplay,
			       pMode->CrtcVDisplay,
			       pScreenInfo->bitsPerPixel));

		  Gal_set_fixed_timings(pGeode->PnlParam.PanelStat.XRes,
					pGeode->PnlParam.PanelStat.YRes,
					pMode->CrtcHDisplay,
					pMode->CrtcVDisplay,
					pScreenInfo->bitsPerPixel);
		  Gal_pnl_init(&(pGeode->PnlParam));
	       }
#else
	       GeodeDebug(("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", READ_REG32(DC_OUTPUT_CFG), READ_REG32(DC_TIMING_CFG), READ_REG32(DC_GENERAL_CFG), READ_REG32(DC_H_TIMING_1), READ_REG32(DC_H_TIMING_2), READ_REG32(DC_H_TIMING_3), READ_REG32(DC_FP_H_TIMING), READ_REG32(DC_V_TIMING_1), READ_REG32(DC_V_TIMING_2), READ_REG32(DC_V_TIMING_3), READ_REG32(DC_FP_V_TIMING)));

	       if (pGeode->FPInBios) {
		  DEBUGMSG(0, (0, X_PROBED, "Restore Panel %d %d %d %d %d\n",
			       pGeode->FPBX,
			       pGeode->FPBY,
			       pMode->CrtcHDisplay,
			       pMode->CrtcVDisplay,
			       pScreenInfo->bitsPerPixel));

#ifdef MAGIC
		  /* XXX: We do this twice - magic ! */
		  gfx_set_fixed_timings(pGeode->FPBX, pGeode->FPBY,
					pMode->CrtcHDisplay,
					pMode->CrtcVDisplay,
					pScreenInfo->bitsPerPixel);
#endif
		  gfx_set_fixed_timings(pGeode->FPBX, pGeode->FPBY,
					pMode->CrtcHDisplay,
					pMode->CrtcVDisplay,
					pScreenInfo->bitsPerPixel);
	       } else {
		  DEBUGMSG(0, (0, X_PROBED, "Restore Panel %d %d %d %d %d\n",
			       pGeode->PnlParam.PanelStat.XRes,
			       pGeode->PnlParam.PanelStat.YRes,
			       pMode->CrtcHDisplay,
			       pMode->CrtcVDisplay,
			       pScreenInfo->bitsPerPixel));
#ifdef MAGIC
		  /* XXX: We do this twice - magic ! */
		  gfx_set_fixed_timings(pGeode->PnlParam.PanelStat.XRes,
					pGeode->PnlParam.PanelStat.YRes,
					pMode->CrtcHDisplay,
					pMode->CrtcVDisplay,
					pScreenInfo->bitsPerPixel);
#endif
		  gfx_set_fixed_timings(pGeode->PnlParam.PanelStat.XRes,
					pGeode->PnlParam.PanelStat.YRes,
					pMode->CrtcHDisplay,
					pMode->CrtcVDisplay,
					pScreenInfo->bitsPerPixel);
		  Pnl_InitPanel(&(pGeode->PnlParam));
	       }
	       GeodeDebug(("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", READ_REG32(DC_OUTPUT_CFG), READ_REG32(DC_TIMING_CFG), READ_REG32(DC_GENERAL_CFG), READ_REG32(DC_H_TIMING_1), READ_REG32(DC_H_TIMING_2), READ_REG32(DC_H_TIMING_3), READ_REG32(DC_FP_H_TIMING), READ_REG32(DC_V_TIMING_1), READ_REG32(DC_V_TIMING_2), READ_REG32(DC_V_TIMING_3), READ_REG32(DC_FP_V_TIMING)));
#endif /* STB_X */
	    } else {
	       /* display is crt */
	       DEBUGMSG(0, (0, X_PROBED, "Setting Display for CRT\n"));
#if defined(STB_X)
	       Gal_set_display_mode(pMode->CrtcHDisplay,
				    pMode->CrtcVDisplay,
				    pScreenInfo->bitsPerPixel,
				    GeodeGetRefreshRate(pMode));

	       /* adjust the pitch */
	       Gal_set_display_pitch(pGeode->Pitch);
#else
#ifdef MAGIC
	       /* XXX: We do this twice - magic ! */
	       gfx_set_display_mode(pMode->CrtcHDisplay,
				    pMode->CrtcVDisplay,
				    pScreenInfo->bitsPerPixel,
				    GeodeGetRefreshRate(pMode));
#endif
	       gfx_set_display_mode(pMode->CrtcHDisplay,
				    pMode->CrtcVDisplay,
				    pScreenInfo->bitsPerPixel,
				    GeodeGetRefreshRate(pMode));

	       /* adjust the pitch */
	       gfx_set_display_pitch(pGeode->Pitch);
#endif /* STB_X */
	    }
	    /* enable crt */
#if defined(STB_X)
	    Gal_set_crt_state(CRT_ENABLE);
#else
#ifdef MAGIC
	    /* XXX: We do this twice - magic ! */
	    gfx_set_crt_enable(CRT_ENABLE);
#endif
	    gfx_set_crt_enable(CRT_ENABLE);
#endif /* STB_X */
	 }

	 GFX(set_display_offset(0L));
	 GFX(wait_vertical_blank());

	 pGeode->CursorSize = 256;

	 /* enable compression if option selected */
	 if (pGeode->Compression) {
	    pGeode->CursorStartOffset = GeodeMemOffset[MemIndex].CurOffset;
	    /* set the compression parameters,and it will be turned on later. */
#if defined(STB_X)
	    Gal_set_compression_parameters(GAL_COMPRESSION_ALL,
					   GeodeMemOffset[MemIndex].CBOffset,
					   GeodeMemOffset[MemIndex].CBPitch,
					   GeodeMemOffset[MemIndex].CBSize -
					   16);

	    /* set the compression buffer, all parameters already set */
	    Gal_set_compression_state(GAL_COMPRESSION_ENABLE);
#else
	    gfx_set_compression_offset(GeodeMemOffset[MemIndex].CBOffset);
	    gfx_set_compression_pitch(GeodeMemOffset[MemIndex].CBPitch);
	    gfx_set_compression_size(GeodeMemOffset[MemIndex].CBSize - 16);

	    /* set the compression buffer, all parameters already set */
	    gfx_set_compression_enable(1);
#endif /* STB_X */

	 } else {
	    /* Default cursor offset, end of the frame buffer */
	    pGeode->CursorStartOffset = pGeode->FBSize - pGeode->CursorSize;
	 }
	 if (pGeode->HWCursor) {
	    /* Load blank cursor */
	    GeodeLoadCursorImage(pScreenInfo, NULL);
	    GFX(set_cursor_position(pGeode->CursorStartOffset, 0, 0, 0, 0));
	    GFX(set_cursor_enable(1));
	 }
#if 0
      } else {
	 GeodeDebug(("GeodeRestoreEx ... "));
	 GeodeRestoreEx(pScreenInfo, pMode);
	 GeodeDebug(("done.\n"));
      }
#endif

   GeodeDebug(("done.\n"));
   /* Reenable the hardware cursor after the mode switch */
   if (pGeode->HWCursor == TRUE) {
      GeodeDebug(("GeodeShowCursor ... "));
      GeodeShowCursor(pScreenInfo);
      GeodeDebug(("done.\n"));
   }
   /* Restore the contents in the screen info */
   GeodeDebug(("After setting the mode\n"));
   return TRUE;
}

/*----------------------------------------------------------------------------
 * GeodeEnterGraphics.
 *
 * Description	:This function will intiallize the displaytiming
				 structure for nextmode and switch to VGA mode.
 *
 * Parameters.
 *    pScreen   :Screen information will be stored in this structure.
 * 	pScreenInfo :Pointer to the screenInfo structure.
 *													
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments     :gfx_vga_mode_switch() will start and end the
 *				switching based on the arguments 0 or 1.soft_vga
 *				is disabled in this function.
*----------------------------------------------------------------------------
*/
static Bool
GeodeEnterGraphics(ScreenPtr pScreen, ScrnInfoPtr pScreenInfo)
{
   GeodePtr pGeode;
   vgaHWPtr hwp = VGAHWPTR(pScreenInfo);

   GeodeDebug(("GeodeEnterGraphics!\n"));
   vgaHWUnlock(hwp);
   /* Should we re-save the text mode on each VT enter? */
   pGeode = GeodeGetRec(pScreenInfo);
#if 0
   print_gxm_gfx_reg(pGeode, 0x4C);
   print_gxm_vga_reg();
#endif
   /* This routine saves the current VGA state in Durango VGA structure */
#if defined(STB_X)
   Gal_get_softvga_state(&pGeode->FBSoftVGAActive);
   pGeode->FBgfxVgaRegs.dwFlags = GFX_VGA_FLAG_MISC_OUTPUT |
	 GFX_VGA_FLAG_STD_CRTC | GFX_VGA_FLAG_EXT_CRTC;
   Gal_vga_save(&(pGeode->FBgfxVgaRegs));
#else
   pGeode->FBSoftVGAActive = gfx_get_softvga_active();
   gfx_vga_save(&(pGeode->FBgfxVgaRegs),
		GFX_VGA_FLAG_MISC_OUTPUT |
		GFX_VGA_FLAG_STD_CRTC | GFX_VGA_FLAG_EXT_CRTC);
#endif /* STB_X */
   DEBUGMSG(0, (0, X_PROBED, "VSA = %d\n", pGeode->FBSoftVGAActive));

   vgaHWSave(pScreenInfo, &VGAHWPTR(pScreenInfo)->SavedReg,
	     VGA_SR_ALL);

#if defined(STB_X)
   Gal_get_display_timing(&pGeode->FBgfxdisplaytiming);
   Gal_tv_get_timings(0, &pGeode->FBgfxtvtiming);

   /* Save Display offset */
   Gal_get_display_offset(&(pGeode->FBDisplayOffset));

   /* Save the current Compression state */
   Gal_get_compression_state(&(pGeode->FBCompressionEnable));
   Gal_get_compression_parameters(GAL_COMPRESSION_ALL,
				  &(pGeode->FBCompressionOffset),
				  &(pGeode->FBCompressionPitch),
				  &(pGeode->FBCompressionSize));

   /* Save Cursor offset */
   {
      unsigned short x, y, xhot, yhot;

      Gal_get_cursor_position(&(pGeode->FBCursorOffset),
			      &x, &y, &xhot, &yhot);
   }
   /* Save the Panel state */
   Gal_pnl_save();

   /* its time to put softvga to sleep */
   Gal_set_softvga_state(FALSE);
   Gal_vga_mode_switch(1);

#else
   /* Save TV State */
   gfx_get_tv_enable(&(pGeode->FBTVEnabled));
   if (pGeode->FBTVEnabled) {
      /* TV Format */
      pGeode->FBtvtiming.HorzTim = READ_VID32(SC1200_TVOUT_HORZ_TIM);
      pGeode->FBtvtiming.HorzSync = READ_VID32(SC1200_TVOUT_HORZ_SYNC);
      pGeode->FBtvtiming.VertSync = READ_VID32(SC1200_TVOUT_VERT_SYNC);
      pGeode->FBtvtiming.LineEnd = READ_VID32(SC1200_TVOUT_LINE_END);
      pGeode->FBtvtiming.VertDownscale =
	    READ_VID32(SC1200_TVOUT_VERT_DOWNSCALE);
      pGeode->FBtvtiming.HorzScaling = READ_VID32(SC1200_TVOUT_HORZ_SCALING);
      pGeode->FBtvtiming.TimCtrl1 = READ_VID32(SC1200_TVENC_TIM_CTRL_1);
      pGeode->FBtvtiming.TimCtrl2 = READ_VID32(SC1200_TVENC_TIM_CTRL_2);
      pGeode->FBtvtiming.Subfreq = READ_VID32(SC1200_TVENC_SUB_FREQ);
      pGeode->FBtvtiming.DispPos = READ_VID32(SC1200_TVENC_DISP_POS);
      pGeode->FBtvtiming.DispSize = READ_VID32(SC1200_TVENC_DISP_SIZE);
      /* TV Output */
      pGeode->FBtvtiming.TimCtrl2 = READ_VID32(SC1200_TVENC_TIM_CTRL_2);
      pGeode->FBtvtiming.Debug = READ_VID32(SC1200_TVOUT_DEBUG);
      /* TV Enable */
      pGeode->FBtvtiming.DacCtrl = READ_VID32(SC1200_TVENC_DAC_CONTROL);
   }

   /* Save CRT State */
   pGeode->FBgfxdisplaytiming.dwDotClock = gfx_get_clock_frequency();
   pGeode->FBgfxdisplaytiming.wPitch = gfx_get_display_pitch();
   pGeode->FBgfxdisplaytiming.wBpp = gfx_get_display_bpp();
   pGeode->FBgfxdisplaytiming.wHTotal = gfx_get_htotal();
   pGeode->FBgfxdisplaytiming.wHActive = gfx_get_hactive();
   pGeode->FBgfxdisplaytiming.wHSyncStart = gfx_get_hsync_start();
   pGeode->FBgfxdisplaytiming.wHSyncEnd = gfx_get_hsync_end();
   pGeode->FBgfxdisplaytiming.wHBlankStart = gfx_get_hblank_start();
   pGeode->FBgfxdisplaytiming.wHBlankEnd = gfx_get_hblank_end();
   pGeode->FBgfxdisplaytiming.wVTotal = gfx_get_vtotal();
   pGeode->FBgfxdisplaytiming.wVActive = gfx_get_vactive();
   pGeode->FBgfxdisplaytiming.wVSyncStart = gfx_get_vsync_start();
   pGeode->FBgfxdisplaytiming.wVSyncEnd = gfx_get_vsync_end();
   pGeode->FBgfxdisplaytiming.wVBlankStart = gfx_get_vblank_start();
   pGeode->FBgfxdisplaytiming.wVBlankEnd = gfx_get_vblank_end();
   pGeode->FBgfxdisplaytiming.wPolarity = gfx_get_sync_polarities();

   /* Save Display offset */
   pGeode->FBDisplayOffset = gfx_get_display_offset();

   /* Save the current Compression state */
   pGeode->FBCompressionEnable = gfx_get_compression_enable();
   pGeode->FBCompressionOffset = gfx_get_compression_offset();
   pGeode->FBCompressionPitch = gfx_get_compression_pitch();
   pGeode->FBCompressionSize = gfx_get_compression_size();

   /* Save Cursor offset */
   pGeode->FBCursorOffset = gfx_get_cursor_offset();

   /* Save the Panel state */
   Pnl_SavePanelState();

   /* its time to put softvga to sleep */
   gfx_disable_softvga();
   gfx_vga_mode_switch(1);

#endif /* STB_X */

   if (!GeodeSetMode(pScreenInfo, pScreenInfo->currentMode)) {
      return FALSE;
   }
#if 1
   /* clear the frame buffer, for annoying noise during mode switch */
   clear_screen(pScreenInfo->currentMode->CrtcHDisplay,
		pScreenInfo->currentMode->CrtcVDisplay);
#endif
   return TRUE;
}

/*----------------------------------------------------------------------------
 * GeodeLeaveGraphics:
 *
 * Description	:This function will restore the displaymode parameters
 * 				 and switches the VGA mode
 *
 * Parameters.
 *    pScreen   :Screen information will be stored in this structure.
 * 	pScreenInfo :Pointer to the screenInfo structure.
 *	
 * 												
 * Returns		:none.
 *
 * Comments		: gfx_vga_mode_switch() will start and end the switching
 *			      based on the arguments 0 or 1.soft_vga is disabled in
 *                    this function.
*----------------------------------------------------------------------------
*/
static void
GeodeLeaveGraphics(ScrnInfoPtr pScreenInfo)
{
   GeodePtr pGeode;

   GeodeDebug(("GeodeLeaveGraphics!\n"));
   pGeode = GEODEPTR(pScreenInfo);

#if 1
   /* clear the frame buffer, when leaving X */
   clear_screen(pScreenInfo->virtualX, pScreenInfo->virtualY);
#endif

#if defined(STB_X)
   Gal_set_display_timing(&pGeode->FBgfxdisplaytiming);
   Gal_tv_set_timings(0, &pGeode->FBgfxtvtiming);
   Gal_set_display_offset(pGeode->FBDisplayOffset);

   /* Restore Panel */
   if (pGeode->PnlParam.PanelPresent == PNL_PANELPRESENT) {
      Gal_pnl_restore();
   }

   /* Restore Cursor */
   Gal_set_cursor_position(pGeode->FBCursorOffset, 0, 0, 0, 0);

   /* Restore the previous Compression state */
   if (pGeode->FBCompressionEnable) {
      Gal_set_compression_parameters(GAL_COMPRESSION_ALL,
				     pGeode->FBCompressionOffset,
				     pGeode->FBCompressionPitch,
				     pGeode->FBCompressionSize);

      Gal_set_compression_state(GAL_COMPRESSION_ENABLE);
   }
#else
   GeodeDebug(("GeodeLeaveGraphics(1)!\n"));
   /* Restore Panel */
   if (pGeode->PnlParam.PanelPresent == PNL_PANELPRESENT) {
      Pnl_RestorePanelState();
   }

   GeodeDebug(("GeodeLeaveGraphics(2)!\n"));
   /* Restore TV */
   if (pGeode->FBTVEnabled) {
      /* TV Format */
      WRITE_VID32(SC1200_TVOUT_HORZ_TIM, pGeode->FBtvtiming.HorzTim);
      WRITE_VID32(SC1200_TVOUT_HORZ_SYNC, pGeode->FBtvtiming.HorzSync);
      WRITE_VID32(SC1200_TVOUT_VERT_SYNC, pGeode->FBtvtiming.VertSync);
      WRITE_VID32(SC1200_TVOUT_LINE_END, pGeode->FBtvtiming.LineEnd);
      WRITE_VID32(SC1200_TVOUT_VERT_DOWNSCALE,
		  pGeode->FBtvtiming.VertDownscale);
      WRITE_VID32(SC1200_TVOUT_HORZ_SCALING, pGeode->FBtvtiming.HorzScaling);
      WRITE_VID32(SC1200_TVENC_TIM_CTRL_1, pGeode->FBtvtiming.TimCtrl1);
      WRITE_VID32(SC1200_TVENC_TIM_CTRL_2, pGeode->FBtvtiming.TimCtrl2);
      WRITE_VID32(SC1200_TVENC_SUB_FREQ, pGeode->FBtvtiming.Subfreq);
      WRITE_VID32(SC1200_TVENC_DISP_POS, pGeode->FBtvtiming.DispPos);
      WRITE_VID32(SC1200_TVENC_DISP_SIZE, pGeode->FBtvtiming.DispSize);
      /* TV Output */
      WRITE_VID32(SC1200_TVENC_TIM_CTRL_2, pGeode->FBtvtiming.TimCtrl2);
      /*  WRITE_VID32(SC1200_TVENC_DAC_CONTROL, tmp); */
      WRITE_VID32(SC1200_TVOUT_DEBUG, pGeode->FBtvtiming.Debug);
      /* TV Enable */
      WRITE_VID32(SC1200_TVENC_DAC_CONTROL, pGeode->FBtvtiming.DacCtrl);
   }
   GeodeDebug(("GeodeLeaveGraphics(3)!\n"));

   /* Restore CRT */
   gfx_set_display_timings(pGeode->FBgfxdisplaytiming.wBpp,
			   pGeode->FBgfxdisplaytiming.wPolarity,
			   pGeode->FBgfxdisplaytiming.wHActive,
			   pGeode->FBgfxdisplaytiming.wHBlankStart,
			   pGeode->FBgfxdisplaytiming.wHSyncStart,
			   pGeode->FBgfxdisplaytiming.wHSyncEnd,
			   pGeode->FBgfxdisplaytiming.wHBlankEnd,
			   pGeode->FBgfxdisplaytiming.wHTotal,
			   pGeode->FBgfxdisplaytiming.wVActive,
			   pGeode->FBgfxdisplaytiming.wVBlankStart,
			   pGeode->FBgfxdisplaytiming.wVSyncStart,
			   pGeode->FBgfxdisplaytiming.wVSyncEnd,
			   pGeode->FBgfxdisplaytiming.wVBlankEnd,
			   pGeode->FBgfxdisplaytiming.wVTotal,
			   pGeode->FBgfxdisplaytiming.dwDotClock);

   gfx_set_display_pitch(pGeode->FBgfxdisplaytiming.wPitch);

   GeodeDebug(("GeodeLeaveGraphics(4)!\n"));
   gfx_set_display_offset(pGeode->FBDisplayOffset);
   GeodeDebug(("GeodeLeaveGraphics(5)!\n"));

   /* Restore Cursor */
   gfx_set_cursor_position(pGeode->FBCursorOffset, 0, 0, 0, 0);
   GeodeDebug(("GeodeLeaveGraphics(6)!\n"));

   /* Restore the previous Compression state */
   if (pGeode->FBCompressionEnable) {
      gfx_set_compression_offset(pGeode->FBCompressionOffset);
      gfx_set_compression_pitch(pGeode->FBCompressionPitch);
      gfx_set_compression_size(pGeode->FBCompressionSize);
      gfx_set_compression_enable(1);
   }
   GeodeDebug(("GeodeLeaveGraphics(7)!\n"));
#endif /* STB_X */

   /* We need this to get back to vga mode when soft-vga
    * * kicks in.
    * * We actually need to examine the attribute Ctlr to find out
    * * what kinda crap (grafix or text) we need to enter in
    * * For now just lookout for 720x400
    */
#if 0
   if ((pGeode->FBgfxdisplaytiming.wHActive == 720) &&
       (pGeode->FBgfxdisplaytiming.wVActive == 400))
#else
   if (pGeode->FBSoftVGAActive)
#endif
   {
      /* VSA was active before we started. Since we disabled it 
       * we got to enable it */
#if defined(STB_X)
      Gal_set_softvga_state(TRUE);
      Gal_vga_mode_switch(1);
      Gal_vga_clear_extended();
      pGeode->FBgfxVgaRegs.dwFlags = GFX_VGA_FLAG_MISC_OUTPUT |
	    GFX_VGA_FLAG_STD_CRTC | GFX_VGA_FLAG_EXT_CRTC;
      Gal_vga_restore(&(pGeode->FBgfxVgaRegs));
      Gal_vga_mode_switch(0);
#else
      GeodeDebug(("GeodeLeaveGraphics(8)!\n"));
      gfx_enable_softvga();
      GeodeDebug(("GeodeLeaveGraphics(9)!\n"));
      gfx_vga_mode_switch(1);
      GeodeDebug(("GeodeLeaveGraphics(10)!\n"));
      gfx_vga_clear_extended();
      GeodeDebug(("GeodeLeaveGraphics(11)!\n"));
      gfx_vga_restore(&(pGeode->FBgfxVgaRegs),
		      GFX_VGA_FLAG_MISC_OUTPUT |
		      GFX_VGA_FLAG_STD_CRTC | GFX_VGA_FLAG_EXT_CRTC);
      GeodeDebug(("GeodeLeaveGraphics(12)!\n"));
      gfx_vga_mode_switch(0);
#endif /* STB_X */
      vgaHWRestore(pScreenInfo, &VGAHWPTR(pScreenInfo)->SavedReg,
		   VGA_SR_ALL);
      GeodeDebug(("GeodeLeaveGraphics(13)!\n"));
   }
#if 0
   print_gxm_gfx_reg(pGeode, 0x4C);
   print_gxm_vga_reg();
#endif
   GeodeDebug(("GeodeLeaveGraphics Done!\n"));
}

/*----------------------------------------------------------------------------
 * GeodeCloseScreen.
 *
 * Description	:This function will restore the original mode
 *				 and also it unmap video memory
 *
 * Parameters.
 *    ScrnIndex	:Screen index value of the screen will be closed.
 * 	pScreen    	:Pointer to the screen structure.
 *	
 * 												
 * Returns		:TRUE on success and FALSE on Failure.
 *
 * Comments		:none.
*----------------------------------------------------------------------------
*/
static Bool
GeodeCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
   ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
   GeodePtr pGeode;

   GeodeDebug(("GeodeCloseScreen!\n"));
   pGeode = GEODEPTR(pScreenInfo);
   /* Shouldn't try to restore graphics, if we're already in textmode */
   if (pScreenInfo->vtSema)
      GeodeLeaveGraphics(pScreenInfo);
   if (pGeode->AccelInfoRec)
      XAADestroyInfoRec(pGeode->AccelInfoRec);
   pScreenInfo->vtSema = FALSE;
   if (pGeode->DGAModes)
      xfree(pGeode->DGAModes);
   pGeode->DGAModes = 0;
   if (pGeode->ShadowPtr)
      xfree(pGeode->ShadowPtr);
   if (pGeode->AccelImageWriteBufferOffsets)
      xfree(pGeode->AccelImageWriteBufferOffsets);

   GeodeUnmapMem(pScreenInfo);

   pScreen->CloseScreen = pGeode->CloseScreen;
   return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

#ifdef DPMSExtension
/*----------------------------------------------------------------------------
 * GeodeDPMSet.
 *
 * Description	:This function sets geode into Power Management
 *               Signalling mode.				
 *
 * Parameters.
 * 	pScreenInfo	 :Pointer to screen info strucrure.
 * 	mode         :Specifies the power management mode.
 *	 												
 * Returns		 :none.
 *
 * Comments      :none.
*----------------------------------------------------------------------------
*/
static void
GeodeDPMSSet(ScrnInfoPtr pScreenInfo, int mode, int flags)
{
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   GeodeDebug(("GeodeDPMSSet!\n"));

   /* Check if we are actively controlling the display */
   if (!pScreenInfo->vtSema) {
      ErrorF("GeodeDPMSSet called when we not controlling the VT!\n");
      return;
   }
   switch (mode) {
   case DPMSModeOn:
      /* Screen: On; HSync: On; VSync: On */
#if defined(STB_X)
      Gal_set_crt_state(CRT_ENABLE);
      if (pGeode->Panel)
	 Gal_pnl_powerup();
#else
      gfx_set_crt_enable(CRT_ENABLE);
      if (pGeode->Panel)
	 Pnl_PowerUp();
#endif /* STB_X */
      if (pGeode->TVSupport)
	 GFX(set_tv_enable(1));
      break;

   case DPMSModeStandby:
      /* Screen: Off; HSync: Off; VSync: On */
#if defined(STB_X)
      Gal_set_crt_state(CRT_STANDBY);
      if (pGeode->Panel)
	 Gal_pnl_powerdown();
#else
      gfx_set_crt_enable(CRT_STANDBY);
      if (pGeode->Panel)
	 Pnl_PowerDown();
#endif /* STB_X */
      if (pGeode->TVSupport)
	 GFX(set_tv_enable(0));
      break;

   case DPMSModeSuspend:
      /* Screen: Off; HSync: On; VSync: Off */
#if defined(STB_X)
      Gal_set_crt_state(CRT_SUSPEND);
      if (pGeode->Panel)
	 Gal_pnl_powerdown();
#else
      gfx_set_crt_enable(CRT_SUSPEND);
      if (pGeode->Panel)
	 Pnl_PowerDown();
#endif /* STB_X */
      if (pGeode->TVSupport)
	 GFX(set_tv_enable(0));
      break;
   case DPMSModeOff:
      /* Screen: Off; HSync: Off; VSync: Off */
#if defined(STB_X)
      Gal_set_crt_state(CRT_DISABLE);
      if (pGeode->Panel)
	 Gal_pnl_powerdown();
#else
      gfx_set_crt_enable(CRT_DISABLE);
      if (pGeode->Panel)
	 Pnl_PowerDown();
#endif /* STB_X */
      if (pGeode->TVSupport)
	 GFX(set_tv_enable(0));
      break;
   }
}
#endif

/*----------------------------------------------------------------------------
 * GeodeScreenInit.
 *
 * Description	:This function will be called at the each ofserver
 *   			 generation.				
 *
 * Parameters.
 *   scrnIndex   :Specfies the screenindex value during generation.
 *    pScreen	 :Pointer to screen info strucrure.
 * 	argc         :parameters for command line arguments count
 *	argv         :command line arguments if any it is not used.  												
 *
 * Returns		 :none.
 *
 * Comments      :none.
*----------------------------------------------------------------------------
*/
static Bool
GeodeScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
   ScrnInfoPtr pScreenInfo = xf86Screens[pScreen->myNum];
   GeodePtr pGeode;
   int i;
   unsigned char *FBStart;
   unsigned int req_offscreenmem;
   int width, height, displayWidth;
   VisualPtr visual;

   GeodeDebug(("GeodeScreenInit!\n"));

   /* Get driver private */
   pGeode = GeodeGetRec(pScreenInfo);
   GeodeDebug(("GeodeScreenInit(0)!\n"));

   /*
    * * Allocate a vgaHWRec
    */
   if (!vgaHWGetHWRec(pScreenInfo))
      return FALSE;
   VGAHWPTR(pScreenInfo)->MapSize = 0x10000;	/* Standard 64k VGA window */
   if (!vgaHWMapMem(pScreenInfo))
      return FALSE;

   if (!GeodeMapMem(pScreenInfo))
      return FALSE;

   vgaHWGetIOBase(VGAHWPTR(pScreenInfo));
   pGeode->Pitch = GeodeCalculatePitchBytes(pScreenInfo);

   /* find the index to our operating mode the offsets are located */
   for (i = 0; i < (sizeof(GeodeMemOffset) / sizeof(MemOffset)); i++) {
      if ((pScreenInfo->virtualX == GeodeMemOffset[i].xres) &&
	  (pScreenInfo->virtualY == GeodeMemOffset[i].yres) &&
	  (pScreenInfo->bitsPerPixel == GeodeMemOffset[i].bpp)) {
	 MemIndex = i;
	 break;
      }
   }
   if (MemIndex == -1)			/* no match */
      return FALSE;
   pGeode->CursorSize = 8 * 32;		/* 32 DWORDS */

   /* Compute cursor buffer */
   if (pGeode->Compression) {
      pGeode->CursorStartOffset = GeodeMemOffset[MemIndex].CurOffset;
   } else {
      /* Default cursor offset, end of the frame buffer */
      pGeode->CursorStartOffset = pGeode->FBSize - pGeode->CursorSize;
   }

   if (pGeode->HWCursor) {
      if ((pGeode->CursorStartOffset + pGeode->CursorSize) > pGeode->FBSize) {
	 /* Not enough memory for HW cursor disable it */
	 pGeode->HWCursor = FALSE;
	 pGeode->CursorSize = 0;
	 DEBUGMSG(1, (scrnIndex, X_ERROR,
		      "Not enough memory for HW cursor. HW cursor disabled.\n"));
      } else {
	 DEBUGMSG(1, (scrnIndex, X_PROBED,
		      "HW cursor offset: 0x%08lX (%d bytes allocated)\n",
		      pGeode->CursorStartOffset, pGeode->CursorSize));
      }
   } else {
      pGeode->CursorSize = 0;
   }

   /* set the offscreen offset accordingly */
   if (pGeode->Compression) {
      pGeode->OffscreenStartOffset =
	    (GeodeMemOffset[MemIndex].OffScreenOffset + pGeode->Pitch - 1)
	    & (~(pGeode->Pitch - 1));
      pGeode->OffscreenSize = pGeode->FBSize - pGeode->OffscreenStartOffset;
   } else {
      pGeode->OffscreenStartOffset = pGeode->Pitch * pScreenInfo->virtualY;
      pGeode->OffscreenSize = pGeode->FBSize -
	    pGeode->OffscreenStartOffset - pGeode->CursorSize;
   }

   if (pGeode->OffscreenSize < 0)
      pGeode->OffscreenSize = 0;
   if ((pGeode->OffscreenStartOffset + pGeode->OffscreenSize) >
       pGeode->FBSize) {
      pGeode->OffscreenSize = 0;
      DEBUGMSG(1, (scrnIndex, X_ERROR, "No offscreen memory available.\n"));
   } else {
      DEBUGMSG(1, (scrnIndex, X_PROBED,
		   "Offscreen memory offset: 0x%08lX (0x%08lX bytes available)\n",
		   pGeode->OffscreenStartOffset, pGeode->OffscreenSize));
   }

   /* XAA Image Write support needs two entire line buffers. */
   req_offscreenmem = pGeode->NoOfImgBuffers * pGeode->Pitch;
   pGeode->AccelImageWriteBufferOffsets = NULL;
   if (req_offscreenmem > pGeode->OffscreenSize) {
      DEBUGMSG(1, (scrnIndex, X_PROBED,
		   "Offscreen memory not adequate for acceleration\n"));
   } else {
      pGeode->OffscreenSize -= req_offscreenmem;
      pGeode->AccelImageWriteBufferOffsets =
	    xalloc(sizeof(unsigned long) * pGeode->NoOfImgBuffers);

      pGeode->AccelImageWriteBufferOffsets[0] = (unsigned char *)(((unsigned
								    long)
								   pGeode->
								   FBBase) +
								  pGeode->
								  OffscreenStartOffset);

      for (i = 1; i < pGeode->NoOfImgBuffers; i++) {
	 pGeode->AccelImageWriteBufferOffsets[i] =
	       pGeode->AccelImageWriteBufferOffsets[i - 1] + pGeode->Pitch;
      }

      for (i = 0; i < pGeode->NoOfImgBuffers; i++) {
	 DEBUGMSG(0, (scrnIndex, X_PROBED,
		      "memory  %x %x\n", pGeode->FBBase,
		      pGeode->AccelImageWriteBufferOffsets[i]));
      }
   }

   /* Initialise graphics mode */
   if (!GeodeEnterGraphics(pScreen, pScreenInfo))
      return FALSE;

   GeodeAdjustFrame(scrnIndex, pScreenInfo->frameX0, pScreenInfo->frameY0, 0);
   GeodeDebug(("GeodeScreenInit(1)!\n"));

   /* Reset visual list */
   miClearVisualTypes();
   GeodeDebug(("GeodeScreenInit(2)!\n"));

   /* Setup the visual we support */
   if (pScreenInfo->bitsPerPixel > 8) {
      if (!miSetVisualTypes(pScreenInfo->depth,
			    TrueColorMask,
			    pScreenInfo->rgbBits,
			    pScreenInfo->defaultVisual)) {
	 return FALSE;
      }
   } else {
      if (!miSetVisualTypes(pScreenInfo->depth,
			    miGetDefaultVisualMask(pScreenInfo->depth),
			    pScreenInfo->rgbBits,
			    pScreenInfo->defaultVisual)) {
	 return FALSE;
      }
   }
   GeodeDebug(("GeodeScreenInit(3)!\n"));

   /* Set this for RENDER extension */
   miSetPixmapDepths();

   /* Call the framebuffer layer's ScreenInit function, and fill in other
    * * pScreen fields.
    */

   if (pGeode->TV_Overscan_On) {
      width = pGeode->TVOw;
      height = pGeode->TVOh;
      GeodeDebug(("width : %d , height : %d", width, height));
   } else {
      width = pScreenInfo->virtualX;
      height = pScreenInfo->virtualY;
   }

   displayWidth = pScreenInfo->displayWidth;
   if (pGeode->Rotate) {
      if (pGeode->TV_Overscan_On) {
	 width = pGeode->TVOh;
	 height = pGeode->TVOw;
      } else {
	 width = pScreenInfo->virtualY;
	 height = pScreenInfo->virtualX;
      }
   }
   if (pGeode->ShadowFB) {
      pGeode->ShadowPitch = BitmapBytePad(pScreenInfo->bitsPerPixel * width);
      pGeode->ShadowPtr = xalloc(pGeode->ShadowPitch * height);
      displayWidth = pGeode->ShadowPitch / (pScreenInfo->bitsPerPixel >> 3);
      FBStart = pGeode->ShadowPtr;
   } else {
      pGeode->ShadowPtr = NULL;

      if (pGeode->TV_Overscan_On) {
	 GeodeDebug(("pGeode->Pitch (%d)!\n", pGeode->Pitch));
	 FBStart = pGeode->FBBase + (pGeode->Pitch * pGeode->TVOy) +
	       (pGeode->TVOx << ((pScreenInfo->depth >> 3) - 1));
      } else {
	 FBStart = pGeode->FBBase;
      }
      DEBUGMSG(1, (0, X_PROBED, "FBStart %X \n", FBStart));
   }

   /* Initialise the framebuffer */
   switch (pScreenInfo->depth) {
   case 8:
   case 16:
      if (!fbScreenInit(pScreen, FBStart, width, height,
			pScreenInfo->xDpi, pScreenInfo->yDpi,
			displayWidth, pScreenInfo->bitsPerPixel))
	 return FALSE;
      break;
   default:
      DEBUGMSG(1, (scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) "
		   "in GeodeScreenInit. Geode only supports 8 and 16 bpp.\n",
		   pScreenInfo->bitsPerPixel));
      return FALSE;
   }
   GeodeDebug(("GeodeScreenInit(4)!\n"));
   xf86SetBlackWhitePixels(pScreen);

   if (!pGeode->ShadowFB && (!pGeode->TV_Overscan_On)) {
      GeodeDGAInit(pScreen);
   }
   GeodeDebug(("GeodeScreenInit(5)!\n"));
   if (pScreenInfo->bitsPerPixel > 8) {
      /* Fixup RGB ordering */
      visual = pScreen->visuals + pScreen->numVisuals;
      while (--visual >= pScreen->visuals) {
	 if ((visual->class | DynamicClass) == DirectColor) {
	    visual->offsetRed = pScreenInfo->offset.red;
	    visual->offsetGreen = pScreenInfo->offset.green;
	    visual->offsetBlue = pScreenInfo->offset.blue;
	    visual->redMask = pScreenInfo->mask.red;
	    visual->greenMask = pScreenInfo->mask.green;
	    visual->blueMask = pScreenInfo->mask.blue;
	 }
      }
   }

   /* must be after RGB ordering fixed */
   fbPictureInit(pScreen, 0, 0);

   GeodeDebug(("GeodeScreenInit(6)!\n"));
   if (!pGeode->NoAccel) {
      GeodeAccelInit(pScreen);
   }
   GeodeDebug(("GeodeScreenInit(7)!\n"));
   miInitializeBackingStore(pScreen);
   xf86SetBackingStore(pScreen);
   GeodeDebug(("GeodeScreenInit(8)!\n"));
   /* Initialise software cursor */
   miDCInitialize(pScreen, xf86GetPointerScreenFuncs());
   /* Initialize HW cursor layer.
    * * Must follow software cursor initialization
    */
   if (pGeode->HWCursor) {
      if (!GeodeHWCursorInit(pScreen))
	 DEBUGMSG(1, (pScreenInfo->scrnIndex, X_ERROR,
		      "Hardware cursor initialization failed\n"));
   }
   GeodeDebug(("GeodeScreenInit(9)!\n"));
   /* Setup default colourmap */
   if (!miCreateDefColormap(pScreen)) {
      return FALSE;
   }
   GeodeDebug(("GeodeScreenInit(10)!\n"));
   /* Initialize colormap layer.
    * * Must follow initialization of the default colormap
    */
   if (!xf86HandleColormaps(pScreen, 256, 8,
			    GeodeLoadPalette, NULL,
			    CMAP_PALETTED_TRUECOLOR |
			    CMAP_RELOAD_ON_MODE_SWITCH)) {
      return FALSE;
   }
   GeodeDebug(("GeodeScreenInit(11)!\n"));

   if (pGeode->ShadowFB) {
      RefreshAreaFuncPtr refreshArea = GeodeRefreshArea;

      if (pGeode->Rotate) {
	 if (!pGeode->PointerMoved) {
	    pGeode->PointerMoved = pScreenInfo->PointerMoved;
	    pScreenInfo->PointerMoved = GeodePointerMoved;
	 }
	 switch (pScreenInfo->bitsPerPixel) {
	 case 8:
	    refreshArea = GeodeRefreshArea8;
	    break;
	 case 16:
	    refreshArea = GeodeRefreshArea16;
	    break;
	 }
      }
      ShadowFBInit(pScreen, refreshArea);
   }
#ifdef DPMSExtension
   xf86DPMSInit(pScreen, GeodeDPMSSet, 0);
#endif
   GeodeDebug(("GeodeScreenInit(12)!\n"));

   if (pGeode->TV_Overscan_On) {
      GeodeDebug(("pGeode->Pitch (%d)!\n", pGeode->Pitch));

      pScreenInfo->memPhysBase = (unsigned long)(pGeode->FBBase +
						 (pGeode->Pitch *
						  pGeode->TVOy) +
						 (pGeode->
						  TVOx <<
						  ((pScreenInfo->depth >> 3) -
						   1)));
      GeodeDebug(("->memPhysBase (%X)!\n", pScreenInfo->memPhysBase));
   } else {
      pScreenInfo->memPhysBase = (unsigned long)pGeode->FBBase;
   }
   pScreenInfo->fbOffset = 0;

   GeodeDebug(("GeodeScreenInit(13)!\n"));
   GeodeInitVideo(pScreen);		/* needed for video */
   /* Wrap the screen's CloseScreen vector and set its
    * SaveScreen vector 
    */
   pGeode->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = GeodeCloseScreen;
   pScreen->SaveScreen = GeodeSaveScreen;
   GeodeDebug(("GeodeScreenInit(14)!\n"));
   /* Report any unused options */
   if (serverGeneration == 1) {
      xf86ShowUnusedOptions(pScreenInfo->scrnIndex, pScreenInfo->options);
   }
   return TRUE;
}

/*----------------------------------------------------------------------------
 * GeodeSwitchMode.
 *
 * Description	:This function will switches the screen mode
 *   			    				
 * Parameters:
 *    scrnIndex	:Specfies the screen index value.
 *    pMode		:pointer to the mode structure.
 * 	flags       :may be used for status check?.
 *	  												
 * Returns		:Returns TRUE on success and FALSE on failure.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
Bool
GeodeSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
   GeodeDebug(("GeodeSwitchMode!\n"));
   return GeodeSetMode(xf86Screens[scrnIndex], pMode);
}

/*----------------------------------------------------------------------------
 * GeodeAdjustFrame.
 *
 * Description	:This function is used to intiallize the start
 *				 address of the memory.
 * Parameters.
 *    scrnIndex	:Specfies the screen index value.
 *     x     	:x co-ordinate value interms of pixels.
 * 	 y          :y co-ordinate value interms of pixels.
 *	  												
 * Returns		:none.
 *
 * Comments    	:none.
*----------------------------------------------------------------------------
*/
void
GeodeAdjustFrame(int scrnIndex, int x, int y, int flags)
{
   ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
   GeodePtr pGeode;
   unsigned long offset;

   pGeode = GeodeGetRec(pScreenInfo);
   offset = (unsigned long)y *(unsigned long)pGeode->Pitch;

   offset += x;
   if (pScreenInfo->bitsPerPixel > 8) {
      offset += x;
   }
   GFX(set_display_offset(offset));
}

/*----------------------------------------------------------------------------
 * GeodeEnterVT.
 *
 * Description	:This is called when VT switching back to the X server
 *			
 * Parameters.
 *    scrnIndex	:Specfies the screen index value.
 *     flags   	:Not used inside the function.
 * 	 						
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static Bool
GeodeEnterVT(int scrnIndex, int flags)
{
   GeodeDebug(("GeodeEnterVT!\n"));
   return GeodeEnterGraphics(NULL, xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * GeodeLeaveVT.
 *
 * Description	:This is called when VT switching  X server text mode.
 *			
 * Parameters.
 *    scrnIndex	:Specfies the screen index value.
 *     flags    :Not used inside the function.
 * 	 						
 * Returns		:none.
 *
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static void
GeodeLeaveVT(int scrnIndex, int flags)
{
   GeodeDebug(("GeodeLeaveVT!\n"));
   GeodeLeaveGraphics(xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * GeodeFreeScreen.
 *
 * Description	:This is called to free any persistent data structures.
 *			
 * Parameters.
 *    scrnIndex :Specfies the screen index value.
 *     flags   	:Not used inside the function.
 * 	 						
 * Returns		:none.
 *
 * Comments     :This will be called only when screen being deleted..
*----------------------------------------------------------------------------
*/
static void
GeodeFreeScreen(int scrnIndex, int flags)
{
   GeodeDebug(("GeodeFreeScreen!\n"));
   if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
      vgaHWFreeHWRec(xf86Screens[scrnIndex]);
   GeodeFreeRec(xf86Screens[scrnIndex]);
}

/*----------------------------------------------------------------------------
 * GeodeValidMode.
 *
 * Description	:This function checks if a mode is suitable for selected
 *                   		chipset.
 * Parameters.
 *    scrnIndex :Specfies the screen index value.
 *     pMode	:Pointer to the screen mode structure..
 * 	 verbose    :not used for implementation.						
 *     flags    :not used for implementation
 *
 * Returns		:MODE_OK if the specified mode is supported or
 *                    		MODE_NO_INTERLACE.
 * Comments     :none.
*----------------------------------------------------------------------------
*/
static int
GeodeValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
   ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
   int ret = -1;
   GeodePtr pGeode;

   pGeode = GeodeGetRec(pScreenInfo);
   DEBUGMSG(0, (0, X_NONE, "GeodeValidateMode: %dx%d %d %d\n",
		pMode->CrtcHDisplay, pMode->CrtcVDisplay,
		pScreenInfo->bitsPerPixel, GeodeGetRefreshRate(pMode)));
   if (pGeode->TVSupport == TRUE) {
      if ((pGeode->TvParam.wWidth == pMode->CrtcHDisplay) &&
	  (pGeode->TvParam.wHeight == pMode->CrtcVDisplay)) {
	 DEBUGMSG(0, (0, X_NONE, "TV mode"));
#if defined(STB_X)
	 Gal_is_tv_mode_supported(0, &(pGeode->TvParam), &ret);
#else
	 ret = gfx_is_tv_display_mode_supported(pMode->CrtcHDisplay,
						pMode->CrtcVDisplay,
						pGeode->TvParam.wStandard);
#endif
      }
   } else {
      DEBUGMSG(0, (0, X_NONE, "CRT mode"));
      if (pMode->Flags & V_INTERLACE)
	 return MODE_NO_INTERLACE;
#if defined(STB_X)
      Gal_is_display_mode_supported(pMode->CrtcHDisplay, pMode->CrtcVDisplay,
				    pScreenInfo->bitsPerPixel,
				    GeodeGetRefreshRate(pMode), &ret);
#else
      ret = gfx_is_display_mode_supported(pMode->CrtcHDisplay,
					  pMode->CrtcVDisplay,
					  pScreenInfo->bitsPerPixel,
					  GeodeGetRefreshRate(pMode));
#endif /* STB_X */
   }

   DEBUGMSG(0, (0, X_NONE, "ret = %d\n", ret));
   if (ret == -1) {			/* mode not supported */
      return MODE_NO_INTERLACE;
   }

   return MODE_OK;
}

/*----------------------------------------------------------------------------
 * GeodeLoadPalette.
 *
 * Description	:This function sets the  palette entry used for graphics data
 *
 * Parameters.
 *   pScreenInfo:Points the screeninfo structure.
 *     numColors:Specifies the no of colors it supported.
 * 	 indizes    :This is used get index value .						
 *     LOCO     :to be added.
 *     pVisual  :to be added.
 *
 * Returns		:MODE_OK if the specified mode is supported or
 *          	 MODE_NO_INTERLACE.
 * Comments     :none.
*----------------------------------------------------------------------------
*/

static void
GeodeLoadPalette(ScrnInfoPtr pScreenInfo,
		 int numColors, int *indizes, LOCO * colors,
		 VisualPtr pVisual)
{
   int i, index, color;

   for (i = 0; i < numColors; i++) {
      index = indizes[i] & 0xFF;
      color = (((unsigned long)(colors[index].red & 0xFF)) << 16) |
	    (((unsigned long)(colors[index].green & 0xFF)) << 8) |
	    ((unsigned long)(colors[index].blue & 0xFF));
      DEBUGMSG(0, (0, X_NONE, "GeodeLoadPalette: %d %d %X\n",
		   numColors, index, color));
      GFX(set_display_palette_entry(index, color));
   }
}

static Bool
GeodeMapMem(ScrnInfoPtr pScreenInfo)
{
   GeodePtr pGeode = GEODEPTR(pScreenInfo);

#if defined(STB_X)
   pGeode->FBBase = (unsigned char *)xf86MapVidMem(pScreenInfo->scrnIndex,
						   VIDMEM_FRAMEBUFFER,
						   pGeode->FBLinearAddr,
						   pGeode->FBSize);

#else
   /* SET DURANGO REGISTER POINTERS
    * * The method of mapping from a physical address to a linear address
    * * is operating system independent.  Set variables to linear address.
    */
   gfx_virt_regptr = (unsigned char *)xf86MapVidMem(pScreenInfo->scrnIndex,
						    VIDMEM_MMIO,
						    (unsigned int)
						    gfx_get_cpu_register_base
						    (), 0x9000);
   gfx_virt_spptr = gfx_virt_regptr;
   gfx_virt_vidptr = (unsigned char *)xf86MapVidMem(pScreenInfo->scrnIndex,
						    VIDMEM_MMIO,
						    (unsigned int)
						    gfx_get_vid_register_base
						    (), 0x1000);
   gfx_virt_fbptr =
	 (unsigned char *)xf86MapVidMem(pScreenInfo->scrnIndex,
					VIDMEM_FRAMEBUFFER,
					pGeode->FBLinearAddr, pGeode->FBSize);

   /* CHECK IF REGISTERS WERE MAPPED SUCCESSFULLY */
   if ((!gfx_virt_regptr) ||
       (!gfx_virt_spptr) || (!gfx_virt_vidptr) || (!gfx_virt_fbptr)) {
      DEBUGMSG(1, (0, X_NONE, "Could not map hardware registers.\n"));
      return (FALSE);
   }

   /* Map the XpressROM ptr to read what platform are we on */
   XpressROMPtr = (unsigned char *)xf86MapVidMem(pScreenInfo->scrnIndex,
						 VIDMEM_FRAMEBUFFER, 0xF0000,
						 0x10000);

   if (!XpressROMPtr)
      return FALSE;

   pGeode->FBBase = gfx_virt_fbptr;
#endif

   return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
GeodeUnmapMem(ScrnInfoPtr pScreenInfo)
{
   GeodePtr pGeode = GEODEPTR(pScreenInfo);

#if !defined(STB_X)
   /* unmap all the memory map's */
   xf86UnMapVidMem(pScreenInfo->scrnIndex, gfx_virt_regptr, 0x9000);
   xf86UnMapVidMem(pScreenInfo->scrnIndex, gfx_virt_vidptr, 0x1000);
   xf86UnMapVidMem(pScreenInfo->scrnIndex, gfx_virt_fbptr, pGeode->FBSize);
   xf86UnMapVidMem(pScreenInfo->scrnIndex, XpressROMPtr, 0x10000);
#endif /* STB_X */

   return TRUE;
}

/* End of file */
