/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_driver.c,v 1.34 1998/09/20 06:01:19 dawes Exp $ */

/*
 * Copyright 1993 by Jon Block <block@frc.com>
 * Modified by Mike Hollick <hollick@graphics.cis.upenn.edu>
 * Modified 1994 by Régis Cridlig <cridlig@dmi.ens.fr>
 *
 * Major Contributors to XFree86 3.2
 *   Modified 1995/6 by Nozomi Ytow
 *   Modified 1996 by Egbert Eich <Egbert.Eich@Physik.TH-Darmstadt.DE>
 *   Modified 1996 by David Bateman <dbateman@ee.uts.edu.au>
 *   Modified 1996 by Xavier Ducoin <xavier@rd.lectra.fr>
 *
 * Contributors to XFree86 3.2
 *   Modified 1995/6 by Ken Raeburn <raeburn@raeburn.org>
 *   Modified 1996 by Shigehiro Nomura <nomura@sm.sony.co.jp>
 *   Modified 1996 by Marc de Courville <courvill@sig.enst.fr>
 *   Modified 1996 by Adam Sulmicki <adam@cfar.umd.edu>
 *   Modified 1996 by Jens Maurer <jmaurer@cck.uni-kl.de>
 *
 * Large parts rewritten for XFree86 4.0
 *   Modified 1998 by David Bateman <dbateman@eng.uts.edu.au>
 *   Modified 1998 by Egbert Eich <Egbert.Eich@Physik.TH-Darmstadt.DE>
 *   Modified 1998 by Nozomi Ytow
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these 
 * conditions, you may download, copy, install, use, modify and distribute 
 * this software in source and/or binary form. No title or ownership is 
 * transferred hereby.
 * 1) Any source code used, modified or distributed must reproduce and retain 
 *    this copyright notice and list of conditions as they appear in the 
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation" name 
 *    nor any trademark or logo of Digital Equipment Corporation may be used 
 *    to endorse or promote products derived from this software without the 
 *    prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed. In
 *    no event shall DIGITAL be liable for any damages whatsoever, and in 
 *    particular, DIGITAL shall not be liable for special, indirect, 
 *    consequential, or incidental damages or damages for lost profits, loss 
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even if
 *    advised of the possibility of such damage. 
 */

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* This is used for module versioning */
#include "xf86Version.h"

/* All drivers using the vgahw module need this */
#include "vgaHW.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

/* All drivers using the mi banking wrapper need this */
#include "mibank.h"

/* All drivers using the mi colormap manipulation need this */
#include "micmap.h"

/* If using cfb, cfb.h is required. */
#define PSZ 8
#include "cfb.h"  
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

/* Needed for the 1 and 4 bpp framebuffers */
#include "xf1bpp.h"
#include "xf4bpp.h"

/* Needed by Resources Access Control (RAC) */
#include "xf86RAC.h"

/* These need to be checked */
#if 0
#ifdef XFreeXDGA 
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif
#endif

/* Driver specific headers */
#include "ct_driver.h"

/* Mandatory functions */
static void     CHIPSIdentify(int flags);
static Bool     CHIPSProbe(DriverPtr drv, int flags);
static Bool     CHIPSPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     CHIPSScreenInit(int Index, ScreenPtr pScreen, int argc,
                                  char **argv);
static Bool     CHIPSSwitchMode(int scrnIndex, DisplayModePtr mode,
                                  int flags);
static void     CHIPSAdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool     CHIPSEnterVT(int scrnIndex, int flags);
static void     CHIPSLeaveVT(int scrnIndex, int flags);
static Bool     CHIPSCloseScreen(int scrnIndex, ScreenPtr pScreen);
static void     CHIPSFreeScreen(int scrnIndex, int flags);
static int      CHIPSValidMode(int scrnIndex, DisplayModePtr mode,
                                 Bool verbose, int flags);
static Bool	CHIPSSaveScreen(ScreenPtr pScreen, Bool unblank);

/* Internally used functions */
static int      chipsFindIsaDevice(void);
static Bool     chipsClockSelect(ScrnInfoPtr pScrn, int no);
static Bool     chipsModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void     chipsSave(ScrnInfoPtr pScrn);
static void     chipsRestore(ScrnInfoPtr pScrn, vgaRegPtr VgaReg,
				 CHIPSRegPtr ChipsReg, Bool restoreFonts);
static void     chipsLock(ScrnInfoPtr pScrn);
static void     chipsUnlock(ScrnInfoPtr pScrn);
static void     chipsClockSave(ScrnInfoPtr pScrn, CHIPSClockPtr Clock);
static void     chipsClockLoad(ScrnInfoPtr pScrn, CHIPSClockPtr Clock);
static Bool     chipsClockFind(ScrnInfoPtr pScrn, int no, CHIPSClockPtr Clock);
static void     chipsCalcClock(ScrnInfoPtr pScrn, int Clock,
				 unsigned char *vclk);
static int      chipsGetHWClock(ScrnInfoPtr pScrn);
static Bool     chipsPreInit655xx(ScrnInfoPtr pScrn, int flags);
static Bool     chipsPreInitHiQV(ScrnInfoPtr pScrn, int flags);
static Bool     chipsPreInitWingine(ScrnInfoPtr pScrn, int flags);
static int      chipsSetMonitor(ScrnInfoPtr pScrn);
static Bool	chipsMapMem(ScrnInfoPtr pScrn);
static Bool	chipsUnmapMem(ScrnInfoPtr pScrn);
static void     chipsProtect(ScrnInfoPtr pScrn, Bool on);
static void	chipsBlankScreen(ScrnInfoPtr pScrn, Bool unblank);
static void     chipsRestoreExtendedRegs(CHIPSPtr cPtr, CHIPSRegPtr Regs);
static void     chipsRestoreStretching(ScrnInfoPtr pScrn,
				unsigned char ctHorizontalStretch,
				unsigned char ctVerticalStretch);
static Bool     chipsModeInitHiQV(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     chipsModeInitWingine(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     chipsModeInit655xx(ScrnInfoPtr pScrn, DisplayModePtr mode);
static int      chipsVideoMode(int vgaBitsPerPixel, int weightGreen,
				int displayHSize, int displayVSize);
static void     chipsDisplayPowerManagementSet(ScrnInfoPtr pScrn,
				int PowerManagementMode, int flags);
static void     chipsHWCursorOn(CHIPSPtr cPtr);
static void     chipsHWCursorOff(CHIPSPtr cPtr);
static void     chipsFixResume(CHIPSPtr cPtr);

/*
 * Initialise some arrays that are used in multiple instances of the
 * acceleration code. Set them up here as its a convenient place to do it.
 */
/* alu to C&T conversion for use with source data */
int ChipsAluConv[] =
{
    0x00,			/* dest =  0; GXclear, 0 */
    0x88,			/* dest &= src; GXand, 0x1 */
    0x44,			/* dest =  src & ~dest; GXandReverse, 0x2 */
    0xCC,			/* dest =  src; GXcopy, 0x3 */
    0x22,			/* dest &= ~src; GXandInverted, 0x4 */
    0xAA,			/* dest =  dest; GXnoop, 0x5 */
    0x66,			/* dest =  ^src; GXxor, 0x6 */
    0xEE,			/* dest |= src; GXor, 0x7 */
    0x11,			/* dest =  ~src & ~dest;GXnor, 0x8 */
    0x99,			/* dest ^= ~src ;GXequiv, 0x9 */
    0x55,			/* dest =  ~dest; GXInvert, 0xA */
    0xDD,			/* dest =  src|~dest ;GXorReverse, 0xB */
    0x33,			/* dest =  ~src; GXcopyInverted, 0xC */
    0xBB,			/* dest |= ~src; GXorInverted, 0xD */
    0x77,			/* dest =  ~src|~dest ;GXnand, 0xE */
    0xFF,			/* dest =  0xFF; GXset, 0xF */
};

/* alu to C&T conversion for use with pattern data */
int ChipsAluConv2[] =
{
    0x00,			/* dest =  0; GXclear, 0 */
    0xA0,			/* dest &= src; GXand, 0x1 */
    0x50,			/* dest =  src & ~dest; GXandReverse, 0x2 */
    0xF0,			/* dest =  src; GXcopy, 0x3 */
    0x0A,			/* dest &= ~src; GXandInverted, 0x4 */
    0xAA,			/* dest =  dest; GXnoop, 0x5 */
    0x5A,			/* dest =  ^src; GXxor, 0x6 */
    0xFA,			/* dest |= src; GXor, 0x7 */
    0x05,			/* dest =  ~src & ~dest;GXnor, 0x8 */
    0xA5,			/* dest ^= ~src ;GXequiv, 0x9 */
    0x55,			/* dest =  ~dest; GXInvert, 0xA */
    0xF5,			/* dest =  src|~dest ;GXorReverse, 0xB */
    0x0F,			/* dest =  ~src; GXcopyInverted, 0xC */
    0xAF,			/* dest |= ~src; GXorInverted, 0xD */
    0x5F,			/* dest =  ~src|~dest ;GXnand, 0xE */
    0xFF,			/* dest =  0xFF; GXset, 0xF */
};

/* alu to C&T conversion for use with pattern data as a planemask */
int ChipsAluConv3[] =
{
    0x0A,			/* dest =  0; GXclear, 0 */
    0x8A,			/* dest &= src; GXand, 0x1 */
    0x4A,			/* dest =  src & ~dest; GXandReverse, 0x2 */
    0xCA,			/* dest =  src; GXcopy, 0x3 */
    0x2A,			/* dest &= ~src; GXandInverted, 0x4 */
    0xAA,			/* dest =  dest; GXnoop, 0x5 */
    0x6A,			/* dest =  ^src; GXxor, 0x6 */
    0xEA,			/* dest |= src; GXor, 0x7 */
    0x1A,			/* dest =  ~src & ~dest;GXnor, 0x8 */
    0x9A,			/* dest ^= ~src ;GXequiv, 0x9 */
    0x5A,			/* dest =  ~dest; GXInvert, 0xA */
    0xDA,			/* dest =  src|~dest ;GXorReverse, 0xB */
    0x3A,			/* dest =  ~src; GXcopyInverted, 0xC */
    0xBA,			/* dest |= ~src; GXorInverted, 0xD */
    0x7A,			/* dest =  ~src|~dest ;GXnand, 0xE */
    0xFA,			/* dest =  0xFF; GXset, 0xF */
};

/* The addresses of the acceleration registers */
unsigned int ChipsReg32HiQV[] =
{
    0x00,		/* BR00 Source and Destination offset register */
    0x04,		/* BR01 Color expansion background color       */
    0x08,		/* BR02 Color expansion foreground color       */
    0x0C,		/* BR03 Monochrome source control register     */
    0x10,		/* BR04 BitBLT control register                */
    0x14,		/* BR05 Pattern address register               */
    0x18,		/* BR06 Source address register                */
    0x1C,		/* BR07 Destination  address register          */
    0x20		/* BR08 Destination width and height register  */
};

unsigned int ChipsReg32[] =
{
  /*BitBLT */
    0x83D0,			       /*DR0 src/dest offset                 */
    0x87D0,			       /*DR1 BitBlt. address of freeVram?    */
    0x8BD0,			       /*DR2 BitBlt. paintBrush, or tile pat.*/
    0x8FD0,                            /*DR3                                 */
    0x93D0,			       /*DR4 BitBlt.                         */
    0x97D0,			       /*DR5 BitBlt. srcAddr, or 0 in VRAM   */
    0x9BD0,			       /*DR6 BitBlt. dest?                   */
    0x9FD0,			       /*DR7 BitBlt. width << 16 | height    */
  /*H/W cursor */
    0xA3D0,			       /*DR8 write/erase cursor              */
		                       /*bit 0-1 if 0  cursor is not shown
		                        * if 1  32x32 cursor
					* if 2  64x64 cursor
					* if 3  128x128 cursor
					*/
                                        /* bit 7 if 1  cursor is not shown   */
		                        /* bit 9 cursor expansion in X       */
		                        /* bit 10 cursor expansion in Y      */
    0xA7D0,			        /* DR9 foreGroundCursorColor         */
    0xABD0,			        /* DR0xA backGroundCursorColor       */
    0xAFD0,			        /* DR0xB cursorPosition              */
		                        /* bit 0-7       x coordinate        */
		                        /* bit 8-14      0                   */
		                        /* bit 15        x signum            */
		                        /* bit 16-23     y coordinate        */
		                        /* bit 24-30     0                   */
		                        /* bit 31        y signum            */
    0xB3D0,			        /* DR0xC address of cursor pattern   */
};

#if defined(__arm32__) && defined(__NetBSD__)
/*
 * Built in TV output modes: These modes have been tested on NetBSD with
 * CT65550 and StrongARM. They give what seems to be the best output for
 * a roughly 640x480 display. To enable one of the built in modes, add 
 * the identifier "NTSC" or "PAL" to the list of modes in the appropriate
 * "Display" subsection of the "Screen" section in the XF86Config file.
 * Note that the call to xf86SetTVOut(), which tells the kernel to enable
 * TV output results in hardware specific actions. There must be code to
 * support this in the kernel or TV output won't work.
 */
static DisplayModeRec ChipsPALMode = {
	NULL, NULL,     /* prev, next */
	"PAL",          /* identifier of this mode */
	MODE_OK,        /* mode status */
	15000,		/* Clock frequency */
	776,		/* HDisplay */
	800,		/* HSyncStart */
	872,		/* HSyncEnd */
	960,		/* HTotal */
	0,		/* HSkew */
	585,		/* VDisplay */
	590,		/* VSyncStart */
	595,		/* VSyncEnd */
	625,		/* VTotal */
	0,		/* VScan */
	V_INTERLACE,	/* Flags */
	-1,		/* ClockIndex */
	15000,		/* SynthClock */
	776,		/* CRTC HDisplay */
	800,            /* CRTC HBlankStart */
	800,            /* CRTC HSyncStart */
	872,            /* CRTC HSyncEnd */
	872,            /* CRTC HBlankEnd */
	960,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	585,		/* CRTC VDisplay */
	590,		/* CRTC VBlankStart */
	590,		/* CRTC VSyncStart */
	595,		/* CRTC VSyncEnd */
	595,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL		/* Private */
};


static DisplayModeRec ChipsNTSCMode = {
	NULL, NULL,     /* prev, next */
	"NTSC",          /* identifier of this mode */
	MODE_OK,        /* mode status */
	11970,		/* Clock frequency */
	584,		/* HDisplay */
	640,		/* HSyncStart */
	696,		/* HSyncEnd */
	760,		/* HTotal */
	0,		/* HSkew */
	450,		/* VDisplay */
	479,		/* VSyncStart */
	485,		/* VSyncEnd */
	525,		/* VTotal */
	0,		/* VScan */
	V_INTERLACE | V_NVSYNC | V_NHSYNC ,	/* Flags */
	-1,		/* ClockIndex */
	11970,		/* SynthClock */
	584,		/* CRTC HDisplay */
	640,            /* CRTC HBlankStart */
	640,            /* CRTC HSyncStart */
	696,            /* CRTC HSyncEnd */
	696,            /* CRTC HBlankEnd */
	760,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	450,		/* CRTC VDisplay */
	479,		/* CRTC VBlankStart */
	479,		/* CRTC VSyncStart */
	485,		/* CRTC VSyncEnd */
	485,		/* CRTC VBlankEnd */
	525,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL		/* Private */
};
#endif

#define VERSION 4000
#define CHIPS_NAME "CHIPS"
#define CHIPS_DRIVER_NAME "chips"
/*
 * The major version is the 0xffff0000 part and the minor version is the
 * 0x0000ffff part.
 */
#define CHIPS_DRIVER_VERSION 0x00010001

/*
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the ModuleInit
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

DriverRec CHIPS = {
	VERSION,
	"Driver for the Chips and Technologies chipsets",
	CHIPSIdentify,
	CHIPSProbe,
	NULL,
	0
};


/* Supported chipsets */
typedef enum {
    CHIPS_CT65520,
    CHIPS_CT65525,
    CHIPS_CT65530,
    CHIPS_CT65535,
    CHIPS_CT65540,
    CHIPS_CT65545,
    CHIPS_CT65546,
    CHIPS_CT65548,
    CHIPS_CT65550,
    CHIPS_CT65554,
    CHIPS_CT65555,
    CHIPS_CT68554,
    CHIPS_CT69000,
    CHIPS_CT64200,
    CHIPS_CT64300
} CHIPSType;

static SymTabRec CHIPSChipsets[] = {
    { CHIPS_CT65520,		"ct65520" },
    { CHIPS_CT65525,		"ct65525" },
    { CHIPS_CT65530,		"ct65530" },
    { CHIPS_CT65535,		"ct65535" },
    { CHIPS_CT65540,		"ct65540" },
    { CHIPS_CT65545,		"ct65545" },
    { CHIPS_CT65546,		"ct65546" },
    { CHIPS_CT65548,		"ct65548" },
    { CHIPS_CT65550,		"ct65550" },
    { CHIPS_CT65554,		"ct65554" },
    { CHIPS_CT65555,		"ct65555" },
    { CHIPS_CT68554,		"ct68554" },
    { CHIPS_CT69000,		"ct69000" },
    { CHIPS_CT64200,		"ct64200" },
    { CHIPS_CT64300,		"ct64300" },
    { -1,			NULL }
};

/* Conversion PCI ID to chipset name */
static PciChipsets CHIPSPCIchipsets[] = {
    { CHIPS_CT65545, PCI_CHIP_65545, RES_SHARED_VGA },
    { CHIPS_CT65548, PCI_CHIP_65548, RES_SHARED_VGA },
    { CHIPS_CT65550, PCI_CHIP_65550, RES_SHARED_VGA },
    { CHIPS_CT65554, PCI_CHIP_65554, RES_SHARED_VGA },
    { CHIPS_CT65555, PCI_CHIP_65555, RES_SHARED_VGA },
    { CHIPS_CT68554, PCI_CHIP_68554, RES_SHARED_VGA },
    { CHIPS_CT69000, PCI_CHIP_69000, RES_SHARED_VGA },
    { -1,	     -1,	     RES_UNDEFINED}
};

static IsaChipsets CHIPSISAchipsets[] = {
    { CHIPS_CT65520,		RES_VGA },
    { CHIPS_CT65525,		RES_VGA },
    { CHIPS_CT65530,		RES_VGA },
    { CHIPS_CT65535,		RES_VGA },
    { CHIPS_CT65540,		RES_VGA },
    { CHIPS_CT65545,		RES_VGA },
    { CHIPS_CT65546,		RES_VGA },
    { CHIPS_CT65548,		RES_VGA },
    { CHIPS_CT65550,		RES_VGA },
    { CHIPS_CT65554,		RES_VGA },
    { CHIPS_CT65555,		RES_VGA },
    { CHIPS_CT68554,		RES_VGA },
    { CHIPS_CT69000,		RES_VGA },
    { CHIPS_CT64200,		RES_VGA },
    { CHIPS_CT64300,		RES_VGA },
    { -1,			RES_UNDEFINED }
};

/* The options supported by the Chips and Technologies Driver */
typedef enum {
    OPTION_FORCE_VCLK1,
    OPTION_LINEAR,
    OPTION_NOACCEL,
    OPTION_HW_CLKS,
    OPTION_NOLINEAR_MODE,
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_STN,
    OPTION_USE_MODELINE,
    OPTION_LCD_STRETCH,
    OPTION_LCD_CENTER,
    OPTION_MMIO,
    OPTION_SUSPEND_HACK,
    OPTION_RGB_BITS,
    OPTION_SYNC_ON_GREEN,
    OPTION_PANEL_SIZE,
    OPTION_18_BIT_BUS,
    OPTION_SHOWCACHE
} CHIPSOpts;

static OptionInfoRec Chips655xxOptions[] = {
    { OPTION_LINEAR,		"Linear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CLKS,		"HWclocks",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOLINEAR_MODE,	"NoLinear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_STN,		"STN",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_USE_MODELINE,	"UseModeline",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_STRETCH,	"NoStretch",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_CENTER,	"LcdCenter",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SUSPEND_HACK,	"SuspendHack",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PANEL_SIZE,	"FixPanelSize",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_18_BIT_BUS,	"18BitBus",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

static OptionInfoRec ChipsWingineOptions[] = {
    { OPTION_LINEAR,		"Linear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CLKS,		"HWclocks",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOLINEAR_MODE,	"NoLinear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

static OptionInfoRec ChipsHiQVOptions[] = {
    { OPTION_FORCE_VCLK1,	"UseVclk1",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LINEAR,		"Linear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CLKS,		"HWclocks",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOLINEAR_MODE,	"NoLinear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_STN,		"STN",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_USE_MODELINE,	"UseModeline",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_STRETCH,	"NoStretch",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_CENTER,	"LcdCenter",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SUSPEND_HACK,	"SuspendHack",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PANEL_SIZE,	"FixPanelSize",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_SYNC_ON_GREEN,	"SyncOnGreen",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_18_BIT_BUS,	"18BitBus",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
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
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    NULL
};

static const char *cfbSymbols[] = {
    "xf1bppScreenInit",
    "xf4bppScreenInit",
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
    "XAAStippleScanlineFuncMSBFirst",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

static const char *racSymbols[] = {
    "xf86RACInit",
    NULL
};

#ifdef XFree86LOADER

MODULEINITPROTO(chipsModuleInit);
static MODULESETUPPROTO(chipsSetup);

static XF86ModuleVersionInfo chipsVersRec =
{
	"chips",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	CHIPS_DRIVER_VERSION,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	{0,0,0,0}
};

/*
 * This function is the module init function.
 * Its name has to be the driver name followed by ModuleInit()
 */
void
chipsModuleInit(XF86ModuleVersionInfo **vers, ModuleSetupProc *setup,
	      ModuleTearDownProc *teardown)
{
    *vers = &chipsVersRec;
    *setup = chipsSetup;
    *teardown = NULL;  
}

static pointer
chipsSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
        xf86AddDriver(&CHIPS, module, 0);

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
	return (pointer)1;
    } else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

#endif /* XFree86LOADER */

static Bool
CHIPSGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate a CHIPSRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(CHIPSRec), 1);

    if (pScrn->driverPrivate == NULL)
	return FALSE;
    
    return TRUE;
}

static void
CHIPSFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

/* Mandatory */
static void
CHIPSIdentify(int flags)
{
    xf86PrintChipsets(CHIPS_NAME, "Driver for Chips and Technologies chipsets",
			CHIPSChipsets);
}

/* Mandatory */
static Bool
CHIPSProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections, *usedDevs;
    GDevPtr usedDev;
    pciVideoPtr *usedPci;
    int *usedChips;
    int usedChip;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(CHIPS_DRIVER_NAME,
					  &devSections)) <= 0) {
	return FALSE;
    }
  
    /* PCI BUS */
    if (xf86GetPciVideoInfo() ) {
	numUsed = xf86MatchPciInstances(CHIPS_NAME, PCI_VENDOR_CHIPSTECH,
					CHIPSChipsets, CHIPSPCIchipsets, 
					devSections,numDevSections,
					&usedDevs, &usedPci, &usedChips);
	if (numUsed > 0){
	    int i;
	    for (i = 0; i < numUsed; i++) {
		pciVideoPtr pPci;
		BusResource Resource;
		pPci = usedPci[i];
		Resource = xf86FindPciResource(usedChips[i],CHIPSPCIchipsets);
		if (xf86CheckPciSlot(pPci->bus, pPci->device, pPci->func,
				     Resource)) {  
		    ScrnInfoPtr pScrn;
		    /* Allocate a ScrnInfoRec and claim the slot */
		    pScrn = xf86AllocateScreen(drv,0);
		    xf86ClaimPciSlot(pPci->bus, pPci->device, pPci->func,
				     Resource, &CHIPS, usedChips[i],
				     pScrn->scrnIndex);
		    pScrn->driverVersion = VERSION;
		    pScrn->driverName    = CHIPS_DRIVER_NAME;
		    pScrn->name          = CHIPS_NAME;
		    pScrn->Probe         = CHIPSProbe;
		    pScrn->PreInit       = CHIPSPreInit;
		    pScrn->ScreenInit    = CHIPSScreenInit;
		    pScrn->SwitchMode    = CHIPSSwitchMode;
		    pScrn->AdjustFrame   = CHIPSAdjustFrame;
		    pScrn->EnterVT       = CHIPSEnterVT;
		    pScrn->LeaveVT       = CHIPSLeaveVT;
		    pScrn->FreeScreen    = CHIPSFreeScreen;
		    pScrn->ValidMode     = CHIPSValidMode;
		    pScrn->device        = usedDevs[i];
		    foundScreen = TRUE;
		}
	    }
      
	    xfree(usedDevs);
	    xfree(usedPci);
	}
    }
    /* Isa Bus */

    usedChip = xf86MatchIsaInstances(CHIPS_NAME,CHIPSChipsets,CHIPSISAchipsets,
				     chipsFindIsaDevice,devSections,
				     numDevSections,&usedDev);    /*XXX*/
    if(usedChip >= 0)
    {
	ScrnInfoPtr pScrn;
	int Resource = xf86FindIsaResource(usedChip,CHIPSISAchipsets); /*XXX*/
	  
	pScrn = xf86AllocateScreen(drv,0);
	xf86ClaimIsaSlot(Resource,&CHIPS,usedChip,pScrn->scrnIndex);
	pScrn->driverVersion = VERSION;
	pScrn->driverName    = CHIPS_DRIVER_NAME;
	pScrn->name          = "CHIPS";
	pScrn->Probe         = CHIPSProbe;
	pScrn->PreInit       = CHIPSPreInit;
	pScrn->ScreenInit    = CHIPSScreenInit;
	pScrn->SwitchMode    = CHIPSSwitchMode;
	pScrn->AdjustFrame   = CHIPSAdjustFrame;
	pScrn->EnterVT       = CHIPSEnterVT;
	pScrn->LeaveVT       = CHIPSLeaveVT;
	pScrn->FreeScreen    = CHIPSFreeScreen;
	pScrn->ValidMode     = CHIPSValidMode;
	pScrn->device        = usedDev;
	foundScreen = TRUE;
    }

    xfree(devSections);
    return foundScreen;
}

static int
chipsFindIsaDevice(void)
{
    int found = -1;
    unsigned char tmp;
    /*if ((testinx(0x3D6, 0x18) && (testinx2(0x3D6, 0x7E, 0x3F))))*/ {
	/* Chips and Technologies chipset. Check if its supported */
	read_xr(0x00, tmp);
	switch (tmp & 0xF0) {
	case 0x70: 		/* CT65520 */
	    found = CHIPS_CT65520; break;
	case 0x80:		/* CT65525 or CT65530 */
	    found = CHIPS_CT65530; break;
	case 0xA0:		/* CT64200 */
	    found = CHIPS_CT64200; break;
	case 0xB0:		/* CT64300 */
	    found = CHIPS_CT64300; break;
	case 0xC0:		/* CT65535 */
	    found = CHIPS_CT65535; break;
	default:
	    switch (tmp & 0xF8) {
	    case 0xD0:		/* CT65540 */
		found = CHIPS_CT65540; break;
	    case 0xD8:		/* CT65545 or CT65546 or CT65548 */
		switch (tmp & 7) {
		case 3:
		    found = CHIPS_CT65546; break;
		case 4:
		    found = CHIPS_CT65548; break;
		default:
		    found = CHIPS_CT65545; break;

		}
		break;
	    default:
		if (tmp == 0x2C) {
		    read_xr(0x01,tmp);
		    if (tmp != 0x10) break;
		    read_xr(0x02,tmp);
		    switch (tmp) {
		    case 0xE0:		/* CT65550 */
			found = CHIPS_CT65550; break;
		    case 0xE4:		/* CT65554 */
			found = CHIPS_CT65554; break;
		    case 0xE5:		/* CT65555 */
			found = CHIPS_CT65555; break;
		    case 0xF4:		/* CT68554 */
			found = CHIPS_CT68554; break;
		    case 0xC0:		/* CT69000 */
			found = CHIPS_CT69000; break;
		    default:
			break;
		    }
		}
		break;
	    }
	    break;
	}
    }
    /* We only want ISA/VL Bus - so check for PCI Bus */
    if(found > CHIPS_CT65548) {
	read_xr(0x08,tmp);
	if(tmp & 0x01) found = -1; 
    } else if(found > CHIPS_CT65535) {
	read_xr(0x01,tmp);
	if ((tmp & 0x07) == 0x06) found = -1;
    }
    return found;
}

/* Mandatory */
Bool
CHIPSPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr *pciList = NULL;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    int i;
    CHIPSPtr cPtr;
    int *numChipsets;
    const char *reqSym = NULL;

    xf86AddControlledResource(pScrn,IO);

    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    /* Allocate the ChipsRec driverPrivate */
    if (!CHIPSGetRec(pScrn)) {
	return FALSE;
    }
    cPtr = CHIPSPTR(pScrn);

    /* Since, the capabilities are determined by the chipset the very
     * first thing to do is, figure out the chipset and its capabilities
     */
    if(xf86FindChipsetsForScreen(pScrn->scrnIndex,&CHIPS,&numChipsets) > 1)
	return FALSE;
    cPtr->Chipset = *numChipsets;
    pScrn->chipset = (char *)xf86TokenToString(CHIPSChipsets,
					       *numChipsets);

    if ((cPtr->Chipset == CHIPS_CT64200) ||
	(cPtr->Chipset == CHIPS_CT64300)) cPtr->Flags |= ChipsWingine;
    if ((cPtr->Chipset >= CHIPS_CT65550) &&
	(cPtr->Chipset <= CHIPS_CT69000)) cPtr->Flags |= ChipsHiQV;

    if(xf86IsPciBus(pScrn->scrnIndex)){
	i = xf86GetPciInfoForScreen(pScrn->scrnIndex, &pciList, NULL);
	if (i > 1) {
	    /* This shouldn't happen */
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Expected one PCI card, but found %d\n", i);
	    CHIPSFreeRec(pScrn);
	    xfree(pciList);
	    return FALSE;
	}
	cPtr->PciInfo = *pciList;
	xfree(pciList);
	      
	cPtr->PciTag = pciTag(cPtr->PciInfo->bus, 
			      cPtr->PciInfo->device,
			      cPtr->PciInfo->func);
    }
	
    /* Now that we've identified the chipset, setup the capabilities flags */
    switch (cPtr->Chipset) {
    case CHIPS_CT69000:
    case CHIPS_CT68554:
    case CHIPS_CT65555:
	cPtr->Flags |= ChipsTMEDSupport;
	/* Fall through */
    case CHIPS_CT65554:
    case CHIPS_CT65550:
    case CHIPS_CT65548:
    case CHIPS_CT65546:
    case CHIPS_CT65545:
	cPtr->Flags |= ChipsMMIOSupport;
	/* Fall through */
    case CHIPS_CT64300:
	cPtr->Flags |= ChipsAccelSupport;
	/* Fall through */
    case CHIPS_CT65540:
	cPtr->Flags |= ChipsHDepthSupport;
	cPtr->Flags |= ChipsDPMSSupport;
	/* Fall through */
    case CHIPS_CT65535:
    case CHIPS_CT65530:
    case CHIPS_CT65525:
	cPtr->Flags |= ChipsLinearSupport;
	/* Fall through */
    case CHIPS_CT64200:
    case CHIPS_CT65520:
	break;
    }

    xf86EnableAccess(&pScrn->Access);

    /* Call the device specific PreInit */
    if (IS_HiQV(cPtr)) {
	if (!chipsPreInitHiQV(pScrn, flags)) {
	    return FALSE;
	}
    } else if (IS_Wingine(cPtr)) {
	if (!chipsPreInitWingine(pScrn, flags)) {
	    return FALSE;
	}
    } else {
	if (!chipsPreInit655xx(pScrn, flags)) {
	    return FALSE;
	}
    }

    /*
     * Allocate a vgaHWRec.
     */
    if (!vgaHWGetHWRec(pScrn))
        return FALSE;
    vgaHWGetIOBase(VGAHWPTR(pScrn));

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfalloc(sizeof(ClockRange));
    clockRanges->next = NULL;
    clockRanges->ClockMulFactor = cPtr->ClockMulFactor;
    clockRanges->minClock = cPtr->MinClock;
    clockRanges->maxClock = cPtr->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    if (cPtr->PanelType & ChipsLCD)
	clockRanges->interlaceAllowed = FALSE;
    else
	clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = FALSE;

    cPtr->Rounding = 8;
    if (pScrn->bitsPerPixel >= 8)
	cPtr->Rounding *= pScrn->bitsPerPixel;

    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			  pScrn->display->modes, clockRanges,
			  NULL, 256, 2048, cPtr->Rounding,
			  128, 2048, pScrn->display->virtualX,
			  pScrn->display->virtualY, cPtr->FbMapSize,
			  LOOKUP_BEST_REFRESH);

    if (i == -1) {
	CHIPSFreeRec(pScrn);
	return FALSE;
    }
    
    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	CHIPSFreeRec(pScrn);
	return FALSE;
    }

    /*
     * If we are using acceleration then we want a display pitch that is
     * a multiple of 64 pixels. This allows for alignment issues for the
     * 8x8 pattern fills. Try to widen the display pitch to 64 if necessary
     */
    if ((cPtr->Flags & ChipsAccelSupport) && (pScrn->displayWidth % 64)) {
	int pitch = ((pScrn->displayWidth + 63) / 64) * 64;

	/* Don't adjust pitch if less than 16kb free. What's the point ! */
	if ((pScrn->videoRam << 10) - cPtr->FrameBufferSize < pitch *
	    pScrn->virtualY * (pScrn->bitsPerPixel >> 3) + 16 * 1024) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "not enough free memory to adjust display pitch.\n");
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "some acceleration may be disabled.\n");
	} else {
	    pScrn->displayWidth = pitch;
	}
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

    /* If monitor resolution is set on the command line, use it */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 1:
	mod = "xf1bpp";
	reqSym = "xf1bppScreenInit";
	break;
    case 4:
	mod = "xf4bpp";
	reqSym = "xf4bppScreenInit";
	break;
    case 8:
	mod = "cfb";
	reqSym = "cfbScreenInit";
	break;
    case 16:
	mod = "cfb16";
	reqSym = "cfb26ScreenInit";
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
	CHIPSFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(reqSym, NULL);

    if (cPtr->Flags & ChipsAccelSupport) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    CHIPSFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    if (cPtr->Flags & ChipsHWCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    CHIPSFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }
    
    if (!xf86LoadSubModule(pScrn, "rac")){
        CHIPSFreeRec(pScrn);
	return FALSE;
    }
    xf86LoaderReqSymLists(racSymbols, NULL);

    return TRUE;
}

static Bool
chipsPreInitHiQV(ScrnInfoPtr pScrn, int flags)
{
    int bytesPerPixel;
    unsigned char tmp;
    MessageType from;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSPanelSizePtr Size = &cPtr->PanelSize;
    CHIPSMemClockPtr MemClk = &cPtr->MemClock;
    CHIPSClockPtr SaveClk = &(cPtr->SavedReg.Clock);

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /* All HiQV chips support 16/24/32 bpp */
    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, Support24bppFb | Support32bppFb))
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
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

    /* Store register values that might be messed up by a suspend resume */
    /* Do this early as some of the other code in PreInit relies on it   */
    cPtr->SuspendHack.vgaIOBaseFlag = (inb(0x3CC) & 0x01);
    cPtr->IOBase = (unsigned int)(cPtr->SuspendHack.vgaIOBaseFlag ?
				  0x3D0 : 0x3B0);

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);
    /* Process the options */
    cPtr->Options = (OptionInfoPtr)ChipsHiQVOptions;
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, cPtr->Options);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* Default to 6, is this right for HiQV?? */
	pScrn->rgbBits = 6;
	if (xf86GetOptValInteger(cPtr->Options, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
    }

    if ((cPtr->Flags & ChipsAccelSupport) &&
	    (xf86IsOptionSet(cPtr->Options, OPTION_NOACCEL))) {
	cPtr->Flags &= ~ChipsAccelSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8) {
	/* Default to SW cursor for 1/4 bpp */
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsHWCursor;
    } else {
	cPtr->Accel.UseHWCursor = TRUE;
	cPtr->Flags |= ChipsHWCursor;
    }
    if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = TRUE;
	cPtr->Flags |= ChipsHWCursor;
    }
    if (xf86IsOptionSet(cPtr->Options, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsHWCursor;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	   (cPtr->Flags & ChipsHWCursor) ? "HW" : "SW");

    if ((pScrn->bitsPerPixel < 8) && 
	    !xf86IsOptionSet(cPtr->Options, OPTION_LINEAR)) {
	cPtr->Flags &= ~ChipsLinearSupport;
    } else if ((cPtr->Flags & ChipsLinearSupport) && 
	    xf86IsOptionSet(cPtr->Options, OPTION_LINEAR)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Enabling linear addressing\n");
    }
    
    if (xf86IsOptionSet(cPtr->Options, OPTION_NOLINEAR_MODE)) {
	cPtr->Flags &= ~ChipsLinearSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Disabling linear addressing\n");
    } else
	cPtr->UseMMIO = TRUE;

    /* monitor info */
    cPtr->Monitor = chipsSetMonitor(pScrn);

    /* memory size */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    } else {
        /* not given, probe it    */
	switch (cPtr->Chipset) {
	case CHIPS_CT69000:
	    /* The ct69000 has 2Mb of SGRAM integrated */
	    pScrn->videoRam = 2048;
	    break;
	case CHIPS_CT65550:
	    /* XR43: DRAM interface   */
	    /* bit 2-1: memory size   */
	    /*          0: 1024 kB    */
	    /*          1: 2048 kB    */
	    /*          2:  reserved  */
	    /*          3: reserved   */
            outb(0x3D6, 0x43);
	    switch ((inb(0x3D7) & 0x06) >> 1) {
	    case 0:
		pScrn->videoRam = 1024;
		break;
	    case 1:
	    case 2:
	    case 3:
		pScrn->videoRam = 2048;
		break;
	    }
	default:
	    /* XRE0: Software reg     */
	    /* bit 3-0: memory size   */
	    /*          0: 512k       */
	    /*          1: 1024k      */
	    /*          2: 1536k(1.5M)*/
	    /*          3: 2048k      */
	    /*          7: 4096k      */
	    outb(0x3D6, 0xE0);
	    tmp = inb(0x3D7) & 0xF;
	    switch (tmp) {
	    case 0:
		pScrn->videoRam = 512;
		break;
	    case 1:
		pScrn->videoRam = 1024;
		break;
	    case 2:
		pScrn->videoRam = 1536;
		break;
	    case 3:
		pScrn->videoRam = 2048;
		break;
	    case 7:
		pScrn->videoRam = 4096;
		break;
	    default:
		pScrn->videoRam = 1024;
		break;
	    }
	}
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    }
    cPtr->FbMapSize = pScrn->videoRam * 1024;

    /*test STN / TFT */
    outb(0x3D0, 0x10);
    tmp = inb(0x3D1);

    /* XR51 or FR10: DISPLAY TYPE REGISTER                      */
    /* XR51[1-0] or FR10[1:0] for ct65550 : PanelType,          */
    /* 0 = Single Panel Single Drive, 3 = Dual Panel Dual Drive */
    switch (tmp & 0x3) {
    case 0:
	if (xf86IsOptionSet(cPtr->Options, OPTION_STN)) {
	    cPtr->PanelType |= ChipsSS;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "SS-STN probed\n");
	} else {
	    cPtr->PanelType |= ChipsTFT;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "TFT probed\n");
	}
	break;
    case 2:
	cPtr->PanelType |= ChipsDS;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DS-STN probed\n");
    case 3:
	cPtr->PanelType |= ChipsDD;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DD-STN probed\n");
	break;
    default:
	break;
    }

    /* test LCD */
    /* FR01: DISPLAY TYPE REGISTER                         */
    /* FR01[1:0]:   Display Type, 01 = CRT, 10 = FlatPanel */
    /* LCD                                                 */
    outb(0x3D0, 0x01);
    tmp = inb(0x3D1);
    if ((tmp & 0x03) == 0x02) {
	cPtr->PanelType |= ChipsLCD;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "LCD\n");
    } else {
	cPtr->PanelType |= ChipsCRT;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CRT\n");
    }

    /* screen size */
    /* 
     * In LCD mode / dual mode we want to derive the timing values from
     * the ones preset by bios
     */
    if (cPtr->PanelType & ChipsLCD) {

	/* for 65550 we only need H/VDisplay values for screen size */
	unsigned char fr25, tmp1;
#ifdef DEBUG
	unsigned char fr26;
	char tmp2;
#endif
	read_fr(0x25, fr25);
	read_fr(0x20, tmp);
	Size->HDisplay = ((tmp + ((fr25 & 0x0F) << 8)) + 1) << 3;
	read_fr(0x30, tmp);
	read_fr(0x35, tmp1);
	Size->VDisplay = ((tmp1 & 0x0F) << 8) + tmp + 1;
#ifdef DEBUG
	read_fr(0x21, tmp);
	Size->HRetraceStart = ((tmp + ((fr25 & 0xF0) << 4)) + 1) << 3;
	read_fr(0x22, tmp1);
	tmp2 = (tmp1 & 0x1F) - (tmp & 0x3F);
	Size->HRetraceEnd = ((((tmp2 < 0) ? (tmp2 + 0x40) : tmp2) << 3)
			      + Size->HRetraceStart);
	read_fr(0x23, tmp);
	read_fr(0x26, fr26);
	Size->HTotal = ((tmp + ((fr26 & 0x0F) << 8)) + 5) << 3;
	ErrorF("x=%i, y=%i; xSync=%i, xSyncEnd=%i, xTotal=%i\n",
		Size->HDisplay, Size->VDisplay,
		Size->HRetraceStart,Size->HRetraceEnd,
		Size->HTotal);
#endif
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Display Size: x=%i; y=%i\n",
		Size->HDisplay, Size->VDisplay);
	/* Warn the user if the panel size has been overridden by
	 * the modeline values
	 */
	if (xf86IsOptionSet(cPtr->Options, OPTION_PANEL_SIZE)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Display size overridden by modelines.\n");
	}
    }

    /* Frame Buffer */                 /* for LCDs          */ 
    if (IS_STN(cPtr->PanelType)) {
	outb(0x3D0, 0x1A);	       /*Frame Buffer Ctrl. */
	tmp = inb(0x3D1);
	if (tmp & 1) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Frame Buffer used\n");
	    if (!(tmp & 0x80)) {
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using embedded Frame Buffer\n");
		/* Formula for calculating the size of the framebuffer. 3
		 * bits per pixel 10 pixels per 32 bit dword. If frame
		 * acceleration is enabled the size can be halved.
		 */
		cPtr->FrameBufferSize = ( Size->HDisplay * 
			Size->VDisplay / 5 ) * ((tmp & 2) ? 1 : 2);
	    } else
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using external Frame Buffer used\n");
	}
	if (tmp & 2)
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Frame accelerator enabled\n");
    }

    /* bus type */
    outb(0x3D6, 0x08);
    tmp = inb(0x3D7) & 1;
    if (tmp == 1) {	       /*PCI */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "PCI Bus\n");
	cPtr->Bus = ChipsPCI;
    } else {   /* XR08: Linear addressing base, not for PCI */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
	cPtr->Bus = ChipsVLB;
    }

    /* disable acceleration for 1 and 4 bpp */
    if (pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Disabling acceleration for %d bpp\n", pScrn->bitsPerPixel);
	cPtr->Flags &= ~ChipsAccelSupport;
    }
    
    /* linear base */
    if (cPtr->Flags & ChipsLinearSupport) {
	if (pScrn->device->MemBase) {
	    cPtr->FbAddress = pScrn->device->MemBase;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "base address is set at 0x%X.\n", cPtr->FbAddress);
	} else {
	    if (cPtr->Bus == ChipsPCI) {
		cPtr->FbAddress =  cPtr->PciInfo->memBase[0] & 0xff800000;
	    } else {
		outb(0x3D6, 0x6);
		cPtr->FbAddress = ((unsigned int)inb(0x3D7)) << 24;
		outb(0x3D6, 0x5);
		cPtr->FbAddress |= ((unsigned int)(0x80 & inb(0x3D7))) << 16;
	    }
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "base address is set at 0x%X.\n", cPtr->FbAddress);
	}
    }

    /* If we support MMIO, setup the MMIO address */
    if (cPtr->Flags & ChipsMMIOSupport)
	cPtr->IOAddress = cPtr->FbAddress + 0x400000L;

    /* Set the flags for Colour transparency. This is dependent
     * on the revision on the chip. Until exactly which chips
     * have this bug are found, only allow 8bpp Colour transparency */
    if (pScrn->bitsPerPixel == 8) 
	cPtr->Flags |= ChipsColorTransparency;
    else
	cPtr->Flags &= ~ChipsColorTransparency;

#if 0
#ifdef XFreeXDGA 
    /* Setup DGA */
    if ( pScrn->depth > 1)
        vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

    /* DAC info */
    outb(0x3D6, 0xD0);
    if (!(inb(0x3D7) & 0x01))
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Internal DAC disabled\n");

    /* MMIO address offset */
    cPtr->Regs32 = ChipsReg32HiQV;
  
    /* sync reset ignored on this chipset */
    cPtr->SyncResetIgn = TRUE;   /* !! */

    /* We use a programamble clock */
    pScrn->numClocks = 26;		/* Some number */
    pScrn->progClock = TRUE;
    cPtr->ClockType = HiQV_STYLE | TYPE_PROGRAMMABLE;

    if (pScrn->device->textClockFreq > 0) {
	SaveClk->Clock = pScrn->device->textClockFreq;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using textclock freq: %7.3f.\n",
		   SaveClk->Clock/1000.0);
    } else
	SaveClk->Clock = 0;

    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using programmable clocks\n");

    if (xf86IsOptionSet(cPtr->Options, OPTION_FORCE_VCLK1)) {
        cPtr->Flags |= ChipsUseVClk1;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using VCLK1 as programmable clocks\n");
    }

    /* Set the maximum memory clock. */
    switch (cPtr->Chipset) {
    case CHIPS_CT65550:
	outb(0x3D6, 0x04);
	if ((inb(0x3D7) & 0xF) < 6)
	    MemClk->Max = 38000; /* Revision A chips */
	else
	    MemClk->Max = 50000; /* Revision B chips */
	break;
    case CHIPS_CT65554:
    case CHIPS_CT65555:
    case CHIPS_CT68554:
	MemClk->Max = 55000;
	break;
    case CHIPS_CT69000:
	MemClk->Max = 100000;
	break;
    }

    /* Probe the memory clock currently in use */
    outb(0x3D6,0xCC);
    MemClk->xrCC = inb(0x3D7);
    MemClk->M = (MemClk->xrCC  & 0x7F) + 2;
    outb(0x3D6,0xCD);
    MemClk->xrCD = inb(0x3D7);
    MemClk->N = (MemClk->xrCD & 0x7F) + 2;
    outb(0x3D6,0xCE);
    MemClk->xrCE = inb(0x3D7);
    MemClk->PSN = (MemClk->xrCE & 0x1) ? 1 : 4;
    MemClk->P = ((MemClk->xrCE & 0x70) >> 4);
    /* Be careful with the calculation of ProbeClk as it can overflow */ 
    MemClk->ProbedClk = 4 * Fref / MemClk->N;
    MemClk->ProbedClk = MemClk->ProbedClk * MemClk->M / (MemClk->PSN * 
	(1 << MemClk->P));
    MemClk->ProbedClk = MemClk->ProbedClk / 1000;
    MemClk->Clk = MemClk->ProbedClk;

    if (pScrn->device->MemClk > 0) {
	if (pScrn->device->MemClk <= MemClk->Max) {
	   xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		      "Using memory clock of %7.3f MHz\n",
		      (float)(pScrn->device->MemClk/1000.));

	    /* Only alter the memory clock if the desired memory clock differs
	     * by 50kHz from the one currently being used.
	     */
	   if (abs(pScrn->device->MemClk - MemClk->ProbedClk) > 50) {
		unsigned char vclk[3];

		MemClk->Clk = pScrn->device->MemClk;
		chipsCalcClock(pScrn, MemClk->Clk, vclk);
		MemClk->M = vclk[1] + 2;
		MemClk->N = vclk[2] + 2;
		MemClk->P = (vclk[0] & 0x70) >> 4;
		MemClk->PSN = (vclk[0] & 0x1) ? 1 : 4;
		MemClk->xrCC = vclk[1];
		MemClk->xrCD = vclk[2];
		MemClk->xrCE = 0x80 || vclk[0];
	   }
	} else
	   xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		      "Memory clock of %7.3f MHz exceeds limit of %7.3 MHz\n",
		      (float)(pScrn->device->MemClk/1000.), 
		      (float)(MemClk->Max/1000.));
    } else 
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Probed memory clock of %7.3f MHz\n",
		   (float)(MemClk->ProbedClk/1000.));
    
    cPtr->ClockMulFactor = 1;

    /* Set the min pixel clock */
    cPtr->MinClock = 11000;	/* XXX Guess, need to check this */
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %7.3f MHz\n",
	       (float)(cPtr->MinClock / 1000.));
    /* Set the max pixel clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT69000:
	cPtr->MaxClock = 220000;
	break;
    case CHIPS_CT68554:
    case CHIPS_CT65555:
	cPtr->MaxClock = 110000;
	break;
    case CHIPS_CT65554:
	cPtr->MaxClock = 95000;
	break;
    case CHIPS_CT65550:
	outb(0x3D6, 0x04);
	if ((inb(0x3D7) & 0xF) < 6) {
	    outb(0x3D0, 0x0A);
	    if (inb(0x3D1) & 2) {
		/*5V Vcc */
		cPtr->MaxClock = 100000;
	    } else {
		/*3.3V Vcc */
		cPtr->MaxClock = 80000;
	    }
	} else
	    cPtr->MaxClock = 95000; /* Revision B */
	break;
    }
    
    /* Check if maxClock is limited by the MemClk. Only 70% to allow for */
    /* RAS/CAS. Extra byte per memory clock needed if framebuffer used */
    if (cPtr->FrameBufferSize)
	cPtr->MaxClock = min(cPtr->MaxClock,
			 MemClk->Clk * 4 * 0.7 / (bytesPerPixel + 1));
    else
	cPtr->MaxClock = min(cPtr->MaxClock, 
			     MemClk->Clk * 4 * 0.7 / bytesPerPixel);
    
    if (pScrn->device->dacSpeeds[0]) {
	int speed = 0;
	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
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
	    cPtr->MaxClock = pScrn->device->dacSpeeds[0];
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	    "User max pixel clock of %7.3f MHz overrides %7.3f MHz limit\n",
	    (float)(cPtr->MaxClock / 1000.), (float)(speed / 1000.));
	cPtr->MaxClock = speed;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		"Max pixel clock is %7.3f MHz\n",
		(float)(cPtr->MaxClock / 1000.));
    }

#if defined(__arm32__) && defined(__NetBSD__)
    /* 
     ** If PAL. SECAM or NTSC is specified as the first mode in 
     ** /etc/XF86Config, add it as the Built in mode.
     */
    if (strlen(pScrn->modes->name) == 3
		&& strcmp(pScrn->modes->name, "PAL") == 0) {
	pScrn->virtualX     = 776;
	pScrn->virtualY     = 585;
	pScrn->displayWidth = 776;
	pScrn->modes = &ChipsPALMode;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using built-in PAL TV mode\n");
	cPtr->TVMode = XMODE_PAL;
    } else if (strlen(pScrn->modes->name) == 4
		&& strcmp(pScrn->modes->name, "NTSC") == 0) {
	pScrn->virtualX     = 584;
	pScrn->virtualY     = 450;
	pScrn->displayWidth = 584;
	pScrn->modes = &ChipsNTSCMode;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using built-in NTSC TV mode\n");
	cPtr->TVMode = XMODE_NTSC;
    } else if (strlen(pScrn->modes->name) == 5
		&& strcmp(pScrn->modes->name, "SECAM") == 0) {
	/*
	** So far, it looks like SECAM uses the same values as PAL
	*/
	pScrn->virtualX     = 776;
	pScrn->virtualY     = 585;
	pScrn->displayWidth = 776;
	pScrn->modes = &ChipsPALMode;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using built-in SECAM TV mode\n");
	cPtr->TVMode = XMODE_SECAM;
    } else
	 cPtr->TVMode = XMODE_RGB;
  
    if (cPtr->TVMode != XMODE_RGB) {
	/*
	 ** These are normally set in xf86LookupMode, but that's not called
	 ** for a built in mode, so do it here.
	 */
	pScrn->numClocks = 3;
	pScrn->clock[0] = 25175;
	pScrn->clock[1] = 28322;
	pScrn->clock[2] = pScrn->modes->SynthClock;
	pScrn->modes->Clock = 2;
    }
#endif

    return TRUE;
}

static Bool
chipsPreInitWingine(ScrnInfoPtr pScrn, int flags)
{
    int i, bytesPerPixel, NoClocks = 0;
    unsigned char tmp;
    MessageType from;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSClockPtr SaveClk = &(cPtr->SavedReg.Clock);

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    if (cPtr->Flags & ChipsHDepthSupport)
	i = xf86SetDepthBpp(pScrn, 8, 8, 8, Support24bppFb );
    else
	i = xf86SetDepthBpp(pScrn, 8, 8, 8, NoDepth24Support );

    if (!i)
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	    /* OK */
	    break;
	case 15:
	case 16:
	case 24:
	    if (cPtr->Flags & ChipsHDepthSupport) 
		break; /* OK */
	    /* fall through */
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

    /* Store register values that might be messed up by a suspend resume */
    /* Do this early as some of the other code in PreInit relies on it   */
    outb(0x3D6, 0x02);
    cPtr->SuspendHack.xr02 = inb(0x3D7) & 0x18;
    outb(0x3D6, 0x03);
    cPtr->SuspendHack.xr03 = inb(0x3D7) & 0x0A;
    outb(0x3D6, 0x14);
    cPtr->SuspendHack.xr14 = inb(0x3D7) & 0x20;
    outb(0x3D6, 0x15);
    cPtr->SuspendHack.xr15 = inb(0x3D7);
    cPtr->SuspendHack.vgaIOBaseFlag = (inb(0x3CC) & 0x01);
    cPtr->IOBase = (unsigned int)(cPtr->SuspendHack.vgaIOBaseFlag ?
				  0x3D0 : 0x3B0);

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    cPtr->Options = (OptionInfoPtr)ChipsWingineOptions;
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, cPtr->Options);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* Default to 6, is this right?? */
	pScrn->rgbBits = 6;
	if (xf86GetOptValInteger(cPtr->Options, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
    }

    if ((cPtr->Flags & ChipsAccelSupport) &&
	    (xf86IsOptionSet(cPtr->Options, OPTION_NOACCEL))) {
	cPtr->Flags &= ~ChipsAccelSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8) {
	/* Default to SW cursor for 1/4 bpp */
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsHWCursor;
    } else {
	cPtr->Accel.UseHWCursor = TRUE;
	cPtr->Flags |= ChipsHWCursor;
    }
    if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = TRUE;
	cPtr->Flags |= ChipsHWCursor;
    }
    if (xf86IsOptionSet(cPtr->Options, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsHWCursor;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	   (cPtr->Flags & ChipsHWCursor) ? "HW" : "SW");

    if ((pScrn->bitsPerPixel < 8) && 
	    !xf86IsOptionSet(cPtr->Options, OPTION_LINEAR)) {
	cPtr->Flags &= ~ChipsLinearSupport;
    } else if ((cPtr->Flags & ChipsLinearSupport) && 
	    xf86IsOptionSet(cPtr->Options, OPTION_LINEAR)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Enabling linear addressing\n");
    }
    
    if (cPtr->Flags & ChipsLinearSupport) {
	if (xf86IsOptionSet(cPtr->Options, OPTION_NOLINEAR_MODE)) {
	    cPtr->Flags &= ~ChipsLinearSupport;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Disabling linear addressing\n");
	}
    }

    /* monitor info */
    cPtr->Monitor = chipsSetMonitor(pScrn);

    /* memory size */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    } else {
	/* not given, probe it    */
	/* XR0F: Software flags 0 */
	/* bit 1-0: memory size   */
	/*          0: 256 kB     */
	/*          1: 512 kB     */
	/*          2: 1024 kB    */
	/*          3: 1024 kB    */

	outb(0x3D6, 0x0F);
	switch (inb(0x3D7) & 3) {
	case 0:
	    pScrn->videoRam = 256;
	    break;
	case 1:
	    pScrn->videoRam = 512;
	    break;
	case 2:
	    pScrn->videoRam = 1024;
	    break;
	case 3:
	    pScrn->videoRam = 2048;
	    break;
	}
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    }
    cPtr->FbMapSize = pScrn->videoRam * 1024;

    cPtr->PanelType |= ChipsCRT;
    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CRT\n");

    /* bus type */
    outb(0x3D6, 0x01);
    tmp = inb(0x3D7) & 3;
    switch (tmp) {
    case 0:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "ISA Bus\n");
	cPtr->Bus = ChipsISA;
	break;
    case 3:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
	cPtr->Bus = ChipsVLB;
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Unknown Bus\n");
	cPtr->Bus = ChipsUnknown;
	break;
    }

    /* disable acceleration for 1 and 4 bpp */
    if (pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Disabling acceleration for %d bpp\n", pScrn->bitsPerPixel);
	cPtr->Flags &= ~ChipsAccelSupport;
    }

    /* linear base */
    if (cPtr->Flags & ChipsLinearSupport) {
	unsigned char mask = 0xF8;
	if (pScrn->videoRam == 1024)
	    mask = 0xF0;
	else if (pScrn->videoRam == 2048)
	    mask = 0xE0;
	if (pScrn->device->MemBase) {
	    cPtr->FbAddress = pScrn->device->MemBase
		& ((0xFF << 24) | (mask << 16));
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "base address is set at 0x%X.\n", cPtr->FbAddress);
	} else {
	    outb(0x3D6, 0x9);
	    cPtr->FbAddress = (( 0xFF & inb(0x3D6)) << 24);
	    outb(0x3D6, 0x8);
	    cPtr->FbAddress |= (( mask  & inb(0x3D7)) << 16);
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "base address is set at 0x%X.\n", cPtr->FbAddress);
	}
    }

    /* If we support MMIO, setup the MMIO address */
    if (cPtr->Flags & ChipsMMIOSupport)
	cPtr->IOAddress = cPtr->FbAddress + 0x200000L;

    /* 32bit register address offsets */
    cPtr->Regs32 = (unsigned int *)xnfalloc(sizeof(ChipsReg32));
    if ((cPtr->Flags & ChipsAccelSupport) ||
	    (cPtr->Flags & ChipsHWCursor)) {
	outb(0x3D6,0x07);
	tmp =  inb(0x3D7);
	for( i = 0; i < (sizeof(ChipsReg32) / sizeof(ChipsReg32[0])); i++) {
	    cPtr->Regs32[i] =  ((ChipsReg32[i] & 0x7E03)) | ((tmp & 0x80)
		<< 8)| ((tmp & 0x7F) << 2);
#ifdef DEBUG
	    ErrorF("DR[%X] = %X\n",i,cPtr->Regs32[i]);
#endif
	}
    }

#if 0
#ifdef XFreeXDGA 
    /* Setup DGA */
    if( pScrn->depth > 1 )
        vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

    cPtr->ClockMulFactor = ((pScrn->bitsPerPixel >= 8) ? bytesPerPixel : 1);
    if (cPtr->ClockMulFactor != 1)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	       "Clocks scaled by %d\n", cPtr->ClockMulFactor);

    /* Clock type */
    switch (cPtr->Chipset) {
    case CHIPS_CT64200:
	NoClocks = 4;
	cPtr->ClockType = WINGINE_1_STYLE | TYPE_HW;
	break;
    default:
	outb(0x3D6,0x01);
	if (!(inb(0x3D7) & 0x10)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Using external clock generator\n");
	    NoClocks = 4;
	    cPtr->ClockType = WINGINE_1_STYLE | TYPE_HW;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Using internal clock generator\n");
	    if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CLKS)) {
		NoClocks = 3;
		cPtr->ClockType = WINGINE_2_STYLE | TYPE_HW;
	    } else {
		NoClocks = 26; /* some number */
		cPtr->ClockType = WINGINE_2_STYLE | TYPE_PROGRAMMABLE;
		pScrn->progClock = TRUE;
	    }
	}
    }

    if (cPtr->ClockType & TYPE_PROGRAMMABLE) {
	pScrn->numClocks = NoClocks;
	if(pScrn->device->textClockFreq > 0) {
	    SaveClk->Clock = pScrn->device->textClockFreq;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Using textclock freq: %7.3f.\n",
		       SaveClk->Clock/1000.0);
	} else
	   SaveClk->Clock = CRT_TEXT_CLK_FREQ;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using programmable clocks\n");
    } else {  /* TYPE_PROGRAMMABLE */
	SaveClk->Clock = chipsGetHWClock(pScrn);
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using textclock clock %i.\n",
	       SaveClk->Clock);
	if (!pScrn->device->numclocks) {
	    pScrn->numClocks = NoClocks;
	    xf86GetClocks(pScrn, NoClocks, chipsClockSelect,
			  chipsProtect, chipsBlankScreen,
			  cPtr->IOBase + 0x0A, 0x08, 1, 28322);
	    from = X_PROBED;
	} else {
	    pScrn->numClocks = pScrn->device->numclocks;
	    if (pScrn->numClocks > NoClocks) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Too many Clocks specified in configuration file.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "\t\tAt most %d clocks may be specified\n", NoClocks);
		pScrn->numClocks= NoClocks;
	    }
	    for (i = 0; i < pScrn->numClocks; i++)
		pScrn->clock[i] = pScrn->device->clock[i];
	    from = X_CONFIG;
	}
#if 0
	for (i = 0; i < pScrn->numClocks; i++)
	    pScrn->clock[i] /= cPtr->ClockMulFactor;
#endif
	xf86ShowClocks(pScrn, from);
    }
      
    /* Set the min pixel clock */
    /* XXX Guess, need to check this */
    cPtr->MinClock = 11000 / cPtr->ClockMulFactor;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %7.3f MHz\n",
	       (float)(cPtr->MinClock / 1000.));
    /* maximal clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT64200:
	cPtr->MaxClock = 80000 / cPtr->ClockMulFactor;
	break;
    case CHIPS_CT64300:
	cPtr->MaxClock = 85000 / cPtr->ClockMulFactor;
	break;
    }

    if (pScrn->device->dacSpeeds[0]) {
	int speed = 0;
	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	case 8:
	   speed = pScrn->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = pScrn->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = pScrn->device->dacSpeeds[DAC_BPP24];
	   break;
	}
	if (speed == 0)
	    cPtr->MaxClock = pScrn->device->dacSpeeds[0];
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	    "User max pixel clock of %7.3f MHz overrides %7.3f MHz limit\n",
	    (float)(cPtr->MaxClock / 1000.), (float)(speed / 1000.));
	cPtr->MaxClock = speed;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		"Max pixel clock is %7.3f MHz\n",
		(float)(cPtr->MaxClock / 1000.));
    }

    return TRUE;
}

static Bool
chipsPreInit655xx(ScrnInfoPtr pScrn, int flags)
{
    int i, bytesPerPixel, NoClocks = 0;
    unsigned char tmp;
    MessageType from;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSPanelSizePtr Size = &cPtr->PanelSize;
    CHIPSClockPtr SaveClk = &(cPtr->SavedReg.Clock);

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    if (cPtr->Flags & ChipsHDepthSupport)
	i = xf86SetDepthBpp(pScrn, 8, 8, 8, Support24bppFb );
    else
	i = xf86SetDepthBpp(pScrn, 8, 8, 8, NoDepth24Support );

    if (!i)
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	    /* OK */
	    break;
	case 15:
	case 16:
	case 24:
	    if (cPtr->Flags & ChipsHDepthSupport) 
		break; /* OK */
	    /* fall through */
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

    /* Store register values that might be messed up by a suspend resume */
    /* Do this early as some of the other code in PreInit relies on it   */
    outb(0x3D6, 0x02);
    cPtr->SuspendHack.xr02 = inb(0x3D7) & 0x18;
    outb(0x3D6, 0x03);
    cPtr->SuspendHack.xr03 = inb(0x3D7) & 0x0A;
    outb(0x3D6, 0x14);
    cPtr->SuspendHack.xr14 = inb(0x3D7) & 0x20;
    outb(0x3D6, 0x15);
    cPtr->SuspendHack.xr15 = inb(0x3D7);
    cPtr->SuspendHack.vgaIOBaseFlag = (inb(0x3CC) & 0x01);
    cPtr->IOBase = cPtr->SuspendHack.vgaIOBaseFlag ? 0x3D0 : 0x3B0;

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    cPtr->Options = (OptionInfoPtr)Chips655xxOptions;
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, cPtr->Options);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8) {
	/* Default to 6, is this right for HiQV?? */
	pScrn->rgbBits = 6;
	if (xf86GetOptValInteger(cPtr->Options, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
    }

    if ((cPtr->Flags & ChipsAccelSupport) &&
	    (xf86IsOptionSet(cPtr->Options, OPTION_NOACCEL))) {
	cPtr->Flags &= ~ChipsAccelSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8) {
	/* Default to SW cursor for 1/4 bpp */
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsHWCursor;
    } else {
	cPtr->Accel.UseHWCursor = TRUE;
	cPtr->Flags |= ChipsHWCursor;
    }
    
    if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CURSOR)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = TRUE;
	cPtr->Flags |= ChipsHWCursor;
    }
    if (xf86IsOptionSet(cPtr->Options, OPTION_SW_CURSOR)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsHWCursor;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	   (cPtr->Flags & ChipsHWCursor) ? "HW" : "SW");

    if ((pScrn->bitsPerPixel < 8) && 
	    !xf86IsOptionSet(cPtr->Options, OPTION_LINEAR)) {
	cPtr->Flags &= ~ChipsLinearSupport;
    } else if ((cPtr->Flags & ChipsLinearSupport) && 
	    xf86IsOptionSet(cPtr->Options, OPTION_LINEAR)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Enabling linear addressing\n");
    }
    
    if (cPtr->Flags & ChipsLinearSupport) {
	if (xf86IsOptionSet(cPtr->Options, OPTION_NOLINEAR_MODE)) {
	    cPtr->Flags &= ~ChipsLinearSupport;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Disabling linear addressing\n");
	}
    }
    if (xf86IsOptionSet(cPtr->Options, OPTION_MMIO) &&
	    (cPtr->Flags & ChipsLinearSupport) &&
	    (cPtr->Flags & ChipsMMIOSupport)) {
	cPtr->UseMMIO = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Enabling MMIO\n");
    }

    /* monitor info */
    cPtr->Monitor = chipsSetMonitor(pScrn);

    /* memory size */
    if (pScrn->device->videoRam != 0) {
	pScrn->videoRam = pScrn->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    } else {
        /* not given, probe it    */
	/* XR0F: Software flags 0 */
	/* bit 1-0: memory size   */
	/*          0: 256 kB     */
	/*          1: 512 kB     */
	/*          2: 1024 kB    */
	/*          3: 1024 kB    */
	outb(0x3D6, 0x0F);
	switch (inb(0x3D7) & 3) {
	case 0:
	    pScrn->videoRam = 256;
	    break;
	case 1:
	    pScrn->videoRam = 512;
	    break;
	case 2:
	case 3:
	    pScrn->videoRam = 1024;
	    break;
	}
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    }
    cPtr->FbMapSize = pScrn->videoRam * 1024;

    /*test STN / TFT */
    outb(0x3D6, 0x51);
    tmp = inb(0x3D7);

    /* XR51 or FR10: DISPLAY TYPE REGISTER                      */
    /* XR51[1-0] or FR10[1:0] for ct65550 : PanelType,          */
    /* 0 = Single Panel Single Drive, 3 = Dual Panel Dual Drive */
    switch (tmp & 0x3) {
    case 0:
	if (xf86IsOptionSet(cPtr->Options, OPTION_STN)) {
	    cPtr->PanelType |= ChipsSS;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "SS-STN probed\n");
	} else {
	    cPtr->PanelType |= ChipsTFT;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "TFT probed\n");
	}
	break;
    case 2:
	cPtr->PanelType |= ChipsDS;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DS-STN probed\n");
    case 3:
	cPtr->PanelType |= ChipsDD;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DD-STN probed\n");
	break;
    default:
	break;
    }

    /* test LCD */
    /* XR51: DISPLAY TYPE REGISTER                     */
    /* XR51[2]:   Display Type, 0 = CRT, 1 = FlatPanel */
    if (tmp & 0x04) {
	cPtr->PanelType |= ChipsLCD;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "LCD\n");
    } else {
	cPtr->PanelType |= ChipsCRT;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CRT\n");
    }

    /* screen size */
    /* 
     * In LCD mode / dual mode we want to derive the timing values from
     * the ones preset by bios
     */
    if (cPtr->PanelType & ChipsLCD) {
	unsigned char xr17, tmp1;
	char tmp2;

	read_xr(0x17, xr17);
	read_xr(0x1B, tmp);
	Size->HTotal =((tmp + ((xr17 & 0x01) << 8)) + 5) << 3;
	read_xr(0x1C, tmp);
	Size->HDisplay = ((tmp + ((xr17 & 0x02) << 7)) + 1) << 3;
	read_xr(0x19, tmp);
	Size->HRetraceStart = ((tmp + ((xr17 & 0x04) << 9)) + 1) << 3;
	read_xr(0x1A, tmp1);
	tmp2 = (tmp1 & 0x1F) + ((xr17 & 0x08) << 2) - (tmp & 0x3F);
	Size->HRetraceEnd = ((((tmp2 < 0) ? (tmp2 + 0x40) : tmp2) << 3)
		+ Size->HRetraceStart);
	read_xr(0x65, tmp1);
	read_xr(0x68, tmp);
	Size->VDisplay = ((tmp1 & 0x02) << 7) 
	      + ((tmp1 & 0x40) << 3) + tmp + 1;
	read_xr(0x66, tmp);
	Size->VRetraceStart = ((tmp1 & 0x04) << 6) 
	      + ((tmp1 & 0x80) << 2) + tmp + 1;
	read_xr(0x64, tmp);
	Size->VTotal = ((tmp1 & 0x01) << 8)
	      + ((tmp1 & 0x20) << 4) + tmp + 2;
#ifdef DEBUG
	ErrorF("x=%i, y=%i; xSync=%i, xSyncEnd=%i, xTotal=%i\n",
	       Size->HDisplay, Size->VDisplay,
	       Size->HRetraceStart, Size->HRetraceEnd,
	       Size->HTotal);
#endif
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Display Size: x=%i; y=%i\n",
		Size->HDisplay, Size->VDisplay);
	/* Warn the user if the panel size has been overridden by
	 * the modeline values
	 */
	if (xf86IsOptionSet(cPtr->Options, OPTION_PANEL_SIZE)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Display size overridden by modelines.\n");
	}
    }

    /* Frame Buffer */                 /* for LCDs          */ 
    if (IS_STN(cPtr->PanelType)) {
	outb(0x3D6, 0x6F);	       /*Frame Buffer Ctrl. */
	tmp = inb(0x3D7);
	if (tmp & 1) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Frame Buffer used\n");
	    if ((cPtr->Chipset > CHIPS_CT65530) && !(tmp & 0x80)) {
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using embedded Frame Buffer\n");
		/* Formula for calculating the size of the framebuffer. 3
		 * bits per pixel 10 pixels per 32 bit dword. If frame
		 * acceleration is enabled the size can be halved.
		 */
		cPtr->FrameBufferSize = ( Size->HDisplay *
			Size->VDisplay / 5 ) * ((tmp & 2) ? 1 : 2);
	    } else
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using external Frame Buffer used\n");
	}
	if (tmp & 2)
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Frame accelerator enabled\n");
    }

    /* bus type */
    outb(0x3D6, 0x01);
    if (cPtr->Chipset > CHIPS_CT65535) {
	tmp = inb(0x3D7) & 7;
	if (tmp == 6) {	       /*PCI */
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "PCI Bus\n");
	    cPtr->Bus = ChipsPCI;
	    if ((cPtr->Chipset == CHIPS_CT65545) ||
		    (cPtr->Chipset == CHIPS_CT65546)) {
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "32Bit IO not supported on 65545 PCI\n");
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "\tenabling MMIO\n");
		cPtr->UseMMIO = TRUE;
	    }
	    
	} else {   /* XR08: Linear addressing base, not for PCI */
	    switch (tmp) {
	    case 3:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CPU Direct\n");
		cPtr->Bus = ChipsCPUDirect;
		break;
	    case 5:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "ISA Bus\n");
		cPtr->Bus = ChipsISA;
		break;
	    case 7:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
		cPtr->Bus = ChipsVLB;
		break;
	    default:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Unknown Bus\n");
	    }
	}
    } else {
	tmp = inb(0x3D7) & 3;
	switch (tmp) {
	case 0:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "PI Bus\n");
	    cPtr->Bus = ChipsPIB;
	    break;
	case 1:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "MC Bus\n");
	    cPtr->Bus = ChipsMCB;
	    break;
	case 2:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
	    cPtr->Bus = ChipsVLB;
	    break;
	case 3:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "ISA Bus\n");
	    cPtr->Bus = ChipsISA;
	    break;
	}
    }

    if (!(cPtr->Bus == ChipsPCI) && (cPtr->UseMMIO)) {
	cPtr->UseMMIO = FALSE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "MMIO only supported on PCI Bus. Disabling MMIO\n");
    }

    /* disable acceleration for 1 and 4 bpp */
    if (pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Disabling acceleration for %d bpp\n", pScrn->bitsPerPixel);
	cPtr->Flags &= ~ChipsAccelSupport;
    }

    if ((cPtr->Chipset == CHIPS_CT65530) &&
	    (cPtr->Flags & ChipsLinearSupport)) {
	/* linear mode is no longer default on ct65530 since it */
	/* requires additional hardware which some manufacturers*/
	/* might not provide.                                   */
	if (!xf86IsOptionSet(cPtr->Options, OPTION_LINEAR))
	    cPtr->Flags &= ~ChipsLinearSupport;
	
	/* Test wether linear addressing is possible on 65530 */
	/* on the 65530 only the A19 select scheme can be used*/
	/* for linear addressing since MEMW is used on ISA bus*/
	/* systems.                                           */
	/* A19 however is used if video memory is > 512 Mb    */   
	if ((cPtr->Bus == ChipsISA) && (pScrn->videoRam > 512)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "User selected linear fb not supported by HW!\n");
	    cPtr->Flags &= ~ChipsLinearSupport;
	}
    }

    /* linear base */
    if (cPtr->Flags & ChipsLinearSupport) {
	unsigned char mask;
	if (cPtr->Chipset == CHIPS_CT65535) {
	    mask = (pScrn->videoRam > 512) ? 0xF8 :0xFC;
	    if (cPtr->Bus == ChipsISA)
	    mask &= 0x7F;
	} else if (cPtr->Bus == ChipsISA) {
	    mask = 0x0F;
	} else {
	    mask = 0xFF;
	    read_xr(0x01,tmp);
	    if(tmp & 0x40)
		mask &= 0x3F;
	    if(!(tmp & 0x80))
		mask &= 0xCF;
	}
	if (pScrn->device->MemBase) {
	    cPtr->FbAddress = pScrn->device->MemBase;
	    if (cPtr->Chipset == CHIPS_CT65535)
		cPtr->FbAddress &= (mask << 17);
	    else if (cPtr->Chipset > CHIPS_CT65535)
		cPtr->FbAddress &= (mask << 20);
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "base address is set at 0x%X.\n", cPtr->FbAddress);
	} else { 
	    if (cPtr->Chipset <= CHIPS_CT65530) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "base address assumed at  0xC00000!\n");
		cPtr->FbAddress = 0xC00000;
	    } else if (cPtr->Chipset == CHIPS_CT65535) {
		outb(0x3D6, 0x8);
		cPtr->FbAddress = ((mask & inb(0x3D7)) << 17);	
	    } else if (cPtr->Bus == ChipsPCI) {
		cPtr->FbAddress =  cPtr->PciInfo->memBase[0] & 0xff800000;
	    } else {
		outb(0x3D6, 0x8);
		cPtr->FbAddress = ((mask & inb(0x3D7)) << 20);
	    }
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "base address is set at 0x%X.\n", cPtr->FbAddress);
	}
    }

    /* If we support MMIO, setup the MMIO address */
    if (cPtr->Flags & ChipsMMIOSupport)
	cPtr->IOAddress = cPtr->FbAddress + 0x200000L;

#if 0
#ifdef XFreeXDGA 
    /* Setup DGA */
    if( pScrn->depth > 1 )
        vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

    /* DAC info */
    outb(0x3D6, 0x06);
    if (inb(0x3D7) & 0x02)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Internal DAC disabled\n");

    /* MMIO address offset */
    if (cPtr->UseMMIO)
	cPtr->Regs32 = ChipsReg32;
    else {
	cPtr->Regs32 = (unsigned int *)xnfalloc(sizeof(ChipsReg32));
	if ((cPtr->Flags & ChipsAccelSupport) ||
		(cPtr->Flags & ChipsHWCursor)) {
	    outb(0x3D6,0x07);
	    tmp =  inb(0x3D7);
	    for (i = 0; i < (sizeof(ChipsReg32)/sizeof(ChipsReg32[0])); i++) {
		cPtr->Regs32[i] =  ((ChipsReg32[i] & 0x7E03)) | ((tmp & 0x80)
		    << 8)| ((tmp & 0x7F) << 2);
#ifdef DEBUG
		ErrorF("DR[%X] = %X\n",i,cPtr->Regs32[i]);
#endif
	    }
	}
    }

    /* sync reset ignored on this chipset */
    if (cPtr->Chipset > CHIPS_CT65530) {
	read_xr(0x0E,tmp);
	if (tmp & 0x80)
	    cPtr->SyncResetIgn = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Synchronous reset %signored.\n",
		   (cPtr->SyncResetIgn ? "" : "not ")); 
    }

    cPtr->ClockMulFactor = ((pScrn->bitsPerPixel >= 8) ? bytesPerPixel : 1);
    if (cPtr->ClockMulFactor != 1)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	       "Clocks scaled by %d\n", cPtr->ClockMulFactor);
    /* We use a programamble clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT65520:
    case CHIPS_CT65525:
    case CHIPS_CT65530:
	NoClocks = 4;		/* Some number */
	cPtr->ClockType = OLD_STYLE | TYPE_HW;
	break;
    default:
	if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CLKS)) {
	    NoClocks = 5;		/* Some number */
	    cPtr->ClockType = NEW_STYLE | TYPE_HW;
	} else {
	    NoClocks = 26;		/* Some number */
	    cPtr->ClockType = NEW_STYLE | TYPE_PROGRAMMABLE;
	    pScrn->progClock = TRUE;
	}
    }

    if (cPtr->ClockType & TYPE_PROGRAMMABLE) {
	pScrn->numClocks = NoClocks;
	if (pScrn->device->textClockFreq > 0) {
	    SaveClk->Clock = pScrn->device->textClockFreq;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Using textclock freq: %7.3f.\n",
		       SaveClk->Clock/1000.0);
	} else
	   SaveClk->Clock = ((cPtr->PanelType & ChipsLCD) ? 
				 LCD_TEXT_CLK_FREQ : CRT_TEXT_CLK_FREQ);
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using programmable clocks\n");
    } else {  /* TYPE_PROGRAMMABLE */
	SaveClk->Clock = chipsGetHWClock(pScrn);
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using textclock clock %i.\n",
	       SaveClk->Clock);
	if (!pScrn->device->numclocks) {
	    pScrn->numClocks = NoClocks;
	    xf86GetClocks(pScrn, NoClocks, chipsClockSelect,
			  chipsProtect, chipsBlankScreen,
			  cPtr->IOBase + 0x0A, 0x08, 1, 28322);
	    from = X_PROBED;
	} else { 
	    pScrn->numClocks = pScrn->device->numclocks;
	    if (pScrn->numClocks > NoClocks) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Too many Clocks specified in configuration file.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "\t\tAt most %d clocks may be specified\n", NoClocks);
		pScrn->numClocks = NoClocks;
	    }
	    for (i = 0; i < pScrn->numClocks; i++)
		pScrn->clock[i] = pScrn->device->clock[i];
	    from = X_CONFIG;
	}
#if 0
	for (i = 0; i < pScrn->numClocks; i++)
	    pScrn->clock[i] /= cPtr->ClockMulFactor;
#endif
	xf86ShowClocks(pScrn, from);
    }
    /* Set the min pixel clock */
    /* XXX Guess, need to check this */
    cPtr->MinClock = 11000 / cPtr->ClockMulFactor;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %7.3f MHz\n",
	       (float)(cPtr->MinClock / 1000.));
    /* Set the max pixel clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT65546:
    case CHIPS_CT65548:
	/* max VCLK is 80 MHz, max MCLK is 75 MHz for CT65548 */
	/* It is not sure for CT65546, but it works with 60 nsec EDODRAM */
	cPtr->MaxClock = 80000 / cPtr->ClockMulFactor;
	break;
    default:
	outb(0x3D6, 0x6C);
	if (inb(0x3D7) & 2) {
	    /*5V Vcc */
	    cPtr->MaxClock = 68000 / cPtr->ClockMulFactor;
	} else {
	    /*3.3V Vcc */
	    cPtr->MaxClock = 56000 / cPtr->ClockMulFactor;
	}
    }

    if (pScrn->device->dacSpeeds[0]) {
	int speed = 0;
	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	case 8:
	   speed = pScrn->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = pScrn->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = pScrn->device->dacSpeeds[DAC_BPP24];
	   break;
	}
	if (speed == 0)
	    cPtr->MaxClock = pScrn->device->dacSpeeds[0];
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	    "User max pixel clock of %7.3f MHz overrides %7.3f MHz limit\n",
	    (float)(cPtr->MaxClock / 1000.), (float)(speed / 1000.));
	cPtr->MaxClock = speed;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Max pixel clock is %7.3f MHz\n",
		   (float)(cPtr->MaxClock / 1000.));
    }

    return TRUE;
}
    

/* Mandatory */
static Bool
CHIPSEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    
    xf86EnableAccess(&pScrn->Access);
    chipsHWCursorOn(cPtr);
    /* Should we re-save the text mode on each VT enter? */
    if(!chipsModeInit(pScrn, pScrn->currentMode))
      return FALSE;
    CHIPSAdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);    
    return TRUE;
}

/* Mandatory */
static void
CHIPSLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    xf86EnableAccess(&pScrn->Access);
#if 0
#ifdef XFreeXDGA
    if (vga256InfoRec.directMode&XF86DGADirectGraphics) {
	/* 
	 * Disable HW cursor. I hope DGA can't call this function twice
	 * in a row, without calling EnterVT in between. Otherwise the
	 * effect will be to hide the cursor, perhaps permanently!!
	 */
        chipsHWCursorOff(cPtr);
	return;
    }
#endif
#endif

    /* Invalidate the cached acceleration registers */
    cAcl->planemask = -1;
    cAcl->fgColor = -1;
    cAcl->bgColor = -1;
    chipsHWCursorOff(cPtr);
    chipsRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &cPtr->SavedReg, TRUE);
    chipsLock(pScrn);
}

/* Mandatory */
static Bool
CHIPSScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    CHIPSPtr cPtr;
    CHIPSACLPtr cAcl;
    int ret;
    VisualPtr visual;
    int savedDefaultVisualClass;
    int allocatebase, freespace, currentaddr;
    unsigned int racflag = 0;

    /*
     * we need to get the ScrnInfoRec for this screen, so let's allocate
     * one first thing
     */
    pScrn = xf86Screens[pScreen->myNum];
    cPtr = CHIPSPTR(pScrn);
    cAcl = CHIPSACLPTR(pScrn);
    
    hwp = VGAHWPTR(pScrn);
    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

    /* Map the VGA memory */
    if (!vgaHWMapMem(pScrn))
	return FALSE;

    /* Map the Chips memory and possible MMIO areas */
    if (!chipsMapMem(pScrn))
	return FALSE;

    xf86EnableAccess(&pScrn->Access);

    /*
     * next we save the current state and setup the first mode
     */
    chipsSave(pScrn);
    if (!chipsModeInit(pScrn,pScrn->currentMode))
	return FALSE;
    CHIPSSaveScreen(pScreen,FALSE);
    CHIPSAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    
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
     * Reset visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.  To deal with this, call
     * miSetVisualTypes for each visual supported.
     */

    if (pScrn->depth > 8) {
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
    case 1:
	ret = xf1bppScreenInit(pScreen, cPtr->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, cPtr->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 8:
	ret = cfbScreenInit(pScreen, cPtr->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, cPtr->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 24:
	ret = cfb24ScreenInit(pScreen, cPtr->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, cPtr->FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Internal error: invalid bpp (%d) in CHIPSScreenInit\n",
		   pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    xf86SetDefaultColorVisualClass(savedDefaultVisualClass);
    if (!ret)
	return FALSE;

    xf86SetBlackWhitePixels(pScreen);

    if (pScrn->pixmapBPP == 8) {	/* Both xf4bpp & cfb */
	/* Another VGA dependency to remove */
	vgaHandleColormaps(pScreen, pScrn);
    } else if (pScrn->depth > 8) {
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

    if (!(cPtr->Flags & ChipsLinearSupport)) {
	miBankInfoPtr pBankInfo;

	/* Setup the vga banking variables */
	pBankInfo = (miBankInfoPtr)xnfcalloc(sizeof(miBankInfoRec),1);
	if (pBankInfo == NULL)
	    return FALSE;
	
#if defined(__arm32__)
	cPtr->Bank = -1;
#endif
	pBankInfo->pBankA = hwp->Base;
	pBankInfo->pBankB = (unsigned char *)hwp->Base + 0x08000;
	pBankInfo->BankSize = 0x08000;
	pBankInfo->nBankDepth = (pScrn->depth == 4) ? 1 : pScrn->depth;
	xf86AddControlledResource(pScrn,MEM_IO);
	xf86EnableAccess(&pScrn->Access);

	if (IS_HiQV(cPtr)) {
	    pBankInfo->pBankB = hwp->Base;
	    pBankInfo->BankSize = 0x10000;
	    if (pScrn->bitsPerPixel < 8) {
		pBankInfo->SetSourceBank =
			(miBankProcPtr)CHIPSHiQVSetReadWritePlanar;
		pBankInfo->SetDestinationBank =
			(miBankProcPtr)CHIPSHiQVSetReadWritePlanar;
		pBankInfo->SetSourceAndDestinationBanks =
			(miBankProcPtr)CHIPSHiQVSetReadWritePlanar;
	    } else {
		pBankInfo->SetSourceBank =
			(miBankProcPtr)CHIPSHiQVSetReadWrite;
		pBankInfo->SetDestinationBank =
			(miBankProcPtr)CHIPSHiQVSetReadWrite;
		pBankInfo->SetSourceAndDestinationBanks =
			(miBankProcPtr)CHIPSHiQVSetReadWrite;
	    }
	} else {
	    if (IS_Wingine(cPtr)) {
		if (pScrn->bitsPerPixel < 8) {
		    pBankInfo->SetSourceBank =
			    (miBankProcPtr)CHIPSWINSetReadPlanar;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSWINSetWritePlanar;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSWINSetReadWritePlanar;
		} else {
		    pBankInfo->SetSourceBank = (miBankProcPtr)CHIPSWINSetRead;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSWINSetWrite;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSWINSetReadWrite;
		}
	    } else {
		if (pScrn->bitsPerPixel < 8) {
		    pBankInfo->SetSourceBank =
			    (miBankProcPtr)CHIPSSetReadPlanar;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSSetWritePlanar;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSSetReadWritePlanar;
		} else {
		    pBankInfo->SetSourceBank = (miBankProcPtr)CHIPSSetRead;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSSetWrite;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSSetReadWrite;
		}
	    }
	}
	if (!miInitializeBanking(pScreen, pScrn->virtualX, pScrn->virtualY,
				 pScrn->displayWidth, pBankInfo)) {
	    xfree(pBankInfo);
	    pBankInfo = NULL;
	    return FALSE;
	}
    }

    /* !!! Only support linear addressing for now. This might change */
    if (cPtr->Flags & ChipsLinearSupport) {
	/* Setup pointers to free space in video ram */
#define CHIPSALIGN(size, align) (currentaddr - ((currentaddr - size) & ~align))
	allocatebase = (pScrn->videoRam<<10) - cPtr->FrameBufferSize;
	if (pScrn->bitsPerPixel < 8)
	    freespace = allocatebase - pScrn->displayWidth * 
		    pScrn->virtualY / 2;
	else
	    freespace = allocatebase - pScrn->displayWidth * 
		    pScrn->virtualY * (pScrn->bitsPerPixel >> 3);
	currentaddr = allocatebase;
	xf86DrvMsg(scrnIndex, X_PROBED,
		   "%d bytes off-screen memory available\n", freespace);

	/* 
	 * Allocate video memory to store the hardware cursor. Allocate 1kB
	 * vram to the cursor, with 1kB alignment for 6554x's and 4kb alignment
	 * for 65550's. Wingine cursor is stored in registers and so no memory
	 * is needed.
	 */
	if (cPtr->Flags & ChipsHWCursor) {
	    cAcl->CursorAddress = -1;
	    if (IS_HiQV(cPtr)) {
		if (CHIPSALIGN(1024, 0xFFF) <= freespace) {
		    currentaddr -= CHIPSALIGN(1024, 0xFFF);
		    freespace -= CHIPSALIGN(1024, 0xFFF);
		    cAcl->CursorAddress = currentaddr;
		}
	    } else if (IS_Wingine(cPtr)) {
		cAcl->CursorAddress = 0;
	    } else if (CHIPSALIGN(1024, 0x3FF) <= freespace) {
		currentaddr -= CHIPSALIGN(1024, 0x3FF);
		freespace -= CHIPSALIGN(1024, 0x3FF);
		cAcl->CursorAddress = currentaddr;
	    }
	    if (cAcl->CursorAddress == -1)
		xf86DrvMsg(scrnIndex, X_ERROR,
		       "Too little space for H/W cursor.\n");
	}
    
	/* Setup the acceleration primitives */
	if (cPtr->Flags & ChipsAccelSupport) {
	    /* 
	     * A scratch area is now allocated in the video ram. This is used
	     * at 8 and 16 bpp to simulate a planemask with a complex ROP, and 
	     * at 24 and 32 bpp to aid in accelerating solid fills
	     */
	    cAcl->ScratchAddress = -1;
	    switch  (pScrn->bitsPerPixel) {
	    case 8:
		if (CHIPSALIGN(64, 0x3F) <= freespace) {
		    currentaddr -= CHIPSALIGN(64, 0x3F);
		    freespace -= CHIPSALIGN(64, 0x3F);
		    cAcl->ScratchAddress = currentaddr;
		}
		break;
	    case 16:
		if (CHIPSALIGN(128, 0x7F) <= freespace) {
		    currentaddr -= CHIPSALIGN(128, 0x7F);
		    freespace -= CHIPSALIGN(128, 0x7F);
		    cAcl->ScratchAddress = currentaddr;
		}
		break;
	    case 24:
		/* One scanline of data used for solid fill */
		if (!IS_HiQV(cPtr)) {
		    if (CHIPSALIGN(3 * (pScrn->displayWidth + 4), 0x3)
			<= freespace) {
			currentaddr -= CHIPSALIGN(3 * (pScrn->displayWidth
					       + 4), 0x3);
			freespace -= CHIPSALIGN(3 * (pScrn->displayWidth + 4),
						0x3);
			cAcl->ScratchAddress = currentaddr;
		    }
		}
		break;
	    case 32:
		/* 16bpp 8x8 mono pattern fill for solid fill. QWORD aligned */
		if (IS_HiQV(cPtr)) {
		    if (CHIPSALIGN(8, 0x7) <= freespace) {
			currentaddr -= CHIPSALIGN(8, 0x7);
			freespace -= CHIPSALIGN(8, 0x7);
			cAcl->ScratchAddress = currentaddr;
		    }
		}
		break;
	    }

	    /* Setup the boundaries of the pixmap cache */
	    cAcl->CacheStart = currentaddr - freespace;
	    cAcl->CacheEnd = currentaddr;

	    if (cAcl->CacheStart >= cAcl->CacheEnd) {
		xf86DrvMsg(scrnIndex, X_ERROR,
		       "Too little space for pixmap cache.\n");
		cAcl->CacheStart = 0;
		cAcl->CacheEnd = 0;
	    }

	    if (IS_HiQV(cPtr)) {
		cAcl->BltDataWindow = (unsigned char *)cPtr->MMIOBase + 
				0x10000L;
	    } else {
		cAcl->BltDataWindow = cPtr->FbBase;
	    }
	    
	    if (IS_HiQV(cPtr)) {
		CHIPSHiQVAccelInit(pScreen);
	    } else if (cPtr->UseMMIO) {
		CHIPSMMIOAccelInit(pScreen);
	    } else {
		CHIPSAccelInit(pScreen);
	    }
	}
    
	miInitializeBackingStore(pScreen);

	/* Initialise cursor functions */
	miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

	if ((cPtr->Flags & ChipsHWCursor) && (cAcl->CursorAddress != -1)) {
	    /* HW cursor functions */
	    if (!CHIPSCursorInit(pScreen)) {
		xf86DrvMsg(scrnIndex, X_ERROR,
		       "Hardware cursor initialization failed\n");
		return FALSE;
	    }
	}
    } else {
	miInitializeBackingStore(pScreen);

	/* Initialise cursor functions */
	miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    }
    
    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    if (pScrn->bitsPerPixel <= 8)
        racflag = RAC_COLORMAP;
    if (!(cPtr->Flags & ChipsLinearSupport))
        racflag |= RAC_FB;
    if (cPtr->Flags & ChipsHWCursor)
        racflag |= RAC_CURSOR;
    xf86RACInit(pScreen, racflag);

    pScreen->SaveScreen = CHIPSSaveScreen;

#ifdef DPMSExtension
    /* Setup DPMS mode */
    if (cPtr->Flags & ChipsDPMSSupport)
	xf86DPMSInit(pScreen, (DPMSSetProcPtr)chipsDisplayPowerManagementSet,
		     0);
#endif

    /* Wrap the current CloseScreen function */
    cPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CHIPSCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    return TRUE;
}

/* Mandatory */
static Bool
CHIPSSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    xf86EnableAccess(&xf86Screens[scrnIndex]->Access);
    return chipsModeInit(xf86Screens[scrnIndex], mode);
}

/* Mandatory */
static void
CHIPSAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn;
    int Base;
    CHIPSPtr cPtr;
    vgaHWPtr hwp;
    unsigned char tmp;

    pScrn = xf86Screens[scrnIndex];
    hwp = VGAHWPTR(pScrn);
    cPtr = CHIPSPTR(pScrn);

    if (xf86IsOptionSet(cPtr->Options, OPTION_SHOWCACHE) && y) {
	int lastline = cPtr->FbMapSize / 
		((pScrn->displayWidth * pScrn->bitsPerPixel) / 8);
	lastline -= pScrn->currentMode->VDisplay;
	y += pScrn->virtualY - 1;
        if (y > lastline) y = lastline;
    }
    
    Base = y * pScrn->displayWidth + x;

    /* calculate base bpp dep. */
    switch (pScrn->bitsPerPixel) {
    case 1:
    case 4:
	Base >>= 3;
	break;
    case 16:
	Base >>= 1;
	break;
    case 24:
	if (!IS_HiQV(cPtr))
	    Base = (Base >> 2) * 3;
	else
	    Base = (Base >> 3) * 6;  /* 65550 seems to need 64bit alignment */
	break;
    case 32:
	break;
    default:			     /* 8bpp */
	Base >>= 2;
	break;
    }

    /* write base to chip */
    /*
     * These are the generic starting address registers.
     */
    xf86EnableAccess(&pScrn->Access);
    chipsFixResume(cPtr);
    outw(hwp->IOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(hwp->IOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
    if (IS_HiQV(cPtr)) {
	outb(0x3D6, 0x09);
	if ((inb(0x3D7) & 0x1) == 0x1)
	    outw(hwp->IOBase + 4, ((Base & 0x0F0000) >> 8) | 0x8000 | 0x40);
    } else {
	outb(0x3D6, 0x0C);
	tmp = inb(0x3D7);
	outb(0x3D7, ((Base & (IS_Wingine(cPtr) ? 0x0F0000 : 0x030000)) >> 16) 
	     | (tmp & 0xF8));
    }
#if 0
#ifdef XFreeXDGA
    if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
	/* Wait until vertical retrace is in progress. */
	while (inb(hwp->IOBase + 0xA) & 0x08);
	while (!(inb(hwp->IOBase + 0xA) & 0x08));
    }
#endif
#endif
}

/* Mandatory */
static Bool
CHIPSCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    if(pScrn->vtSema){   /*§§§*/
      xf86EnableAccess(&pScrn->Access);
      chipsHWCursorOff(cPtr);
      chipsRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &cPtr->SavedReg, TRUE);
      chipsLock(pScrn);
      chipsUnmapMem(pScrn);
    }
    if (cPtr->AccelInfoRec)
	XAADestroyInfoRec(cPtr->AccelInfoRec);
    if (cPtr->CursorInfoRec)
	xf86DestroyCursorInfoRec(cPtr->CursorInfoRec);

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = cPtr->CloseScreen; /*§§§*/
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);/*§§§*/
}

/* Optional */
static void
CHIPSFreeScreen(int scrnIndex, int flags)
{
    vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    CHIPSFreeRec(xf86Screens[scrnIndex]);
}

/* Optional */
static int
CHIPSValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    /* The tests here need to be expanded */
    if ((mode->Flags & V_INTERLACE) && (cPtr->Flags & ChipsLCD))
	return MODE_BAD;

    return MODE_OK;
}

#ifdef DPMSExtension
/*
 * DPMS Control registers
 *
 * XR73 6554x and 64300 (what about 65535?)
 * XR61 6555x
 *    0   HSync Powerdown data
 *    1   HSync Select 1=Powerdown
 *    2   VSync Powerdown data
 *    3   VSync Select 1=Powerdown
 */

static void
chipsDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			       int flags)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char dpmsreg, seqreg, lcdoff, tmp;
    
    xf86EnableAccess(&pScrn->Access);
    switch (PowerManagementMode) {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	dpmsreg = 0x00;
	seqreg = 0x00;
	lcdoff = 0x0;
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	dpmsreg = 0x02;
	seqreg = 0x20;
	lcdoff = 0x0;
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	dpmsreg = 0x08;
	seqreg = 0x20;
	lcdoff = 0x1;
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	dpmsreg = 0x0A;
	seqreg = 0x20;
	lcdoff = 0x1;
	break;
    default:
	return;
    }

    outb(0x3C4, 0x01);
    seqreg |= inb(0x3C5) & ~0x20;
    outb(0x3C5, seqreg);
    if (IS_HiQV(cPtr))
	outb(0x3D6,0x61);
    else
	outb(0x3D6,0x73);
    tmp = inb(0x3D7);
    outb(0x3D7,((tmp & 0xF0) | dpmsreg));
    /* Turn off the flat panel */
    if (cPtr->PanelType & ChipsLCD) {
	if (IS_HiQV(cPtr)) {
	    outb(0x3D0,0x5);
	    tmp = inb(0x3D1);
	    if (lcdoff)
	        outb(0x3D1, (tmp | 0x8));
	    else
	        outb(0x3D1, (tmp & 0xF7));
	} else {
	    outb(0x3D6,0x52);
	    tmp = inb(0x3D7);
	    if (lcdoff)
	        outb(0x3D7, (tmp | 0x8));
	    else
	        outb(0x3D7, (tmp & 0xF7));
	}
    }
}
#endif

static Bool
CHIPSSaveScreen(ScreenPtr pScreen, Bool unblank)
{
   ScrnInfoPtr pScrn = NULL;            /* §§§ */

   if (pScreen != NULL)
      pScrn = xf86Screens[pScreen->myNum];

   if (unblank)
      SetTimeSinceLastInputEvent();

   if ((pScrn != NULL) && pScrn->vtSema) { /* §§§ */
     xf86EnableAccess(&pScrn->Access);
     chipsBlankScreen(pScrn, unblank);
   }
   return (TRUE);
}

static Bool
chipsClockSelect(ScrnInfoPtr pScrn, int no)
{
    CHIPSClockReg TmpClock;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    switch (no) {
    case CLK_REG_SAVE:
	chipsClockSave(pScrn, &cPtr->SaveClock);
	break;

    case CLK_REG_RESTORE:
	chipsClockLoad(pScrn, &cPtr->SaveClock);
	break;

    default:
	if (!chipsClockFind(pScrn, no, &TmpClock))
	    return (FALSE);
	chipsClockLoad(pScrn, &TmpClock);
    }
    return (TRUE);
}

/*
 * 
 * Fout = (Fref * 4 * M) / (PSN * N * (1 << P) )
 * Fvco = (Fref * 4 * M) / (PSN * N)
 * where
 * M = XR31+2
 * N = XR32+2
 * P = XR30[3:1]
 * PSN = XR30[0]? 1:4
 * 
 * constraints:
 * 4 MHz <= Fref <= 20 MHz (typ. 14.31818 MHz)
 * 150 kHz <= Fref/(PSN * N) <= 2 MHz
 * 48 MHz <= Fvco <= 220 MHz
 * 2 < M < 128
 * 2 < N < 128
 */

static void
chipsClockSave(ScrnInfoPtr pScrn, CHIPSClockPtr Clock)
{
    unsigned char tmp;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;

    Clock->msr = (inb(0x3CC) & 0xFE);	/* save standard VGA clock registers */
    switch (Type & GET_STYLE) {
    case HiQV_STYLE:
	read_fr(0x03, Clock->fr03);  /* save alternate clock select reg.  */
	if (!Clock->Clock) {   /* save HiQV console clock           */
	    if (cPtr->Flags & ChipsUseVClk1) {
		tmp = 1;
	    } else {
		tmp = (Clock->fr03 & 0xC) >> 2;
		if (tmp == 3)
		    tmp = 2;
	    }
	    tmp = tmp << 2;
	    read_xr(0xC0 + tmp, cPtr->ConsoleClk[0]);
	    read_xr(0xC1 + tmp, cPtr->ConsoleClk[1]);
	    read_xr(0xC2 + tmp, cPtr->ConsoleClk[2]);
	    read_xr(0xC3 + tmp, cPtr->ConsoleClk[3]);
	}
	break;
    case OLD_STYLE: 
	Clock->fcr = inb(0x3CA);
	read_xr(0x02, Clock->xr02);  
	read_xr(0x54, Clock->xr54);    /* save alternate clock select reg.   */
	break;
    case WINGINE_1_STYLE:
    case WINGINE_2_STYLE:
	break;
    case NEW_STYLE:
	read_xr(0x54, Clock->xr54);    /* save alternate clock select reg.   */
	read_xr(0x33, Clock->xr33);    /* get status of MCLK/VCLK select reg.*/
	break;
    }
#ifdef DEBUG
    ErrorF("saved \n");
#endif
}

static Bool
chipsClockFind(ScrnInfoPtr pScrn, int no, CHIPSClockPtr Clock)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;

    if (no > (pScrn->numClocks - 1))
	return (FALSE);

    switch (Type & GET_STYLE) {
    case HiQV_STYLE:
	if (cPtr->Flags & ChipsUseVClk1)
	    Clock->msr = 1 << 2;
	else
	    Clock->msr = 3 << 2;
	Clock->fr03 = Clock->msr;
	Clock->Clock = pScrn->currentMode->Clock;
	break;
    case NEW_STYLE:
	if (Type & TYPE_HW) {
	    Clock->msr = (no == 4 ? 3 << 2: (no & 0x01) << 2);
	    Clock->xr54 = Clock->msr;               
	    Clock->xr33 = no > 1 ? 0x80 : 0;	
	} else {
	    Clock->msr = 3 << 2;
	    Clock->xr33 = 0;
	    Clock->xr54 = Clock->msr;
	    Clock->Clock = pScrn->currentMode->SynthClock;
#if 0
	    Clock->Clock *= cPtr->ClockMulFactor;
#endif
	}
	break;
    case OLD_STYLE:
	if (no > 3) {
	    Clock->msr = 3 << 2;
	    Clock->fcr = no & 0x03;
	    Clock->xr02 = 0;
	    Clock->xr54 = Clock->msr & (Clock->fcr << 4);
	} else {
	    Clock->msr = (no << 2) & 0x4;
	    Clock->fcr = 0;
	    Clock->xr02 = no & 0x02;
	    Clock->xr54 = Clock->msr;
	}
	break;
    case WINGINE_1_STYLE:
	Clock->msr = no << 2;
    case WINGINE_2_STYLE:
	if (Type & TYPE_HW) {
	    Clock->msr = (no == 2 ? 3 << 2: (no & 0x01) << 2);
	    Clock->xr33 = 0;	
	} else {
	    Clock->msr = 3 << 2;
	    Clock->xr33 = 0;
	    Clock->Clock = pScrn->currentMode->SynthClock;
#if 0
	    Clock->Clock *= cPtr->ClockMulFactor;
#endif
	}
	break;
    }
    Clock->msr |= (inb(0x3CC) & 0xF2);

#ifdef DEBUG
    ErrorF("found\n");
#endif
    return (TRUE);
}

static int
chipsGetHWClock(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;
    unsigned char tmp, tmp1;

    if (!(Type & TYPE_HW))
        return 0;		/* shouldn't happen                   */

    switch (Type & GET_STYLE) {
    case WINGINE_1_STYLE:
	return ((inb(0x3CC) & 0x0C) >> 2);
    case WINGINE_2_STYLE:
	tmp = ((inb(0x3CC) & 0x04) >> 2);
	return (tmp > 2) ? 2 : tmp;
    case OLD_STYLE:
	if (!(cPtr->PanelType & ChipsLCD))
	    tmp = inb(0x3CC);
	else
	    read_xr(0x54,tmp);
	if (tmp & 0x08) {
	    if (!(cPtr->PanelType & ChipsLCD))
		tmp = inb(0x3CA) & 0x03;
	    else 
		tmp = (tmp >> 4) & 0x03;
	    return (tmp + 4);
	} else {
	    tmp = (tmp >> 2) & 0x01;
	    read_xr(0x02,tmp1);
	    return (tmp + (tmp1 & 0x02));
	}
    case NEW_STYLE:
	if (cPtr->PanelType & ChipsLCD) {
	    read_xr(0x54, tmp);
	} else
	    tmp = inb(0x3CC);
	tmp = (tmp & 0x0C) >> 2;
	if (tmp > 1) return 4;
	read_xr(0x33, tmp1);
	tmp1 = (tmp1 & 0x80) >> 6; /* iso mode 25.175/28.322 or 32/36 MHz  */
	return (tmp + tmp1);       /*            ^=0    ^=1     ^=4 ^=5    */
    default:		       /* we should never get here              */
	return (0);
    }
}

static void
chipsClockLoad(ScrnInfoPtr pScrn, CHIPSClockPtr Clock)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;
    volatile unsigned char tmp, tmpmsr, tmpfcr, tmp02;
    volatile unsigned char tmp33, tmp54, tmpf03;
    unsigned char vclk[3];       

    tmpmsr = inb(0x3CC);  /* read msr, we need it for all clock styles */

    switch (Type & GET_STYLE) {
    case HiQV_STYLE:
	read_fr(0x03, tmpf03);   /* save alternate clock select reg.  */
	/* select fixed clock 0  before tampering with VCLK select */
	outb(0x3C2, (tmpmsr & ~0x0D) | cPtr->SuspendHack.vgaIOBaseFlag);
	write_fr(0x03, (tmpf03 & ~0x0C) | 0x04);
	if (!Clock->Clock) {      /* Hack to load saved console clock  */
	    if (cPtr->Flags & ChipsUseVClk1) {
		tmp = 1;
	    } else {
		tmp = (Clock->fr03 & 0xC) >> 2;
		if (tmp == 3)
		    tmp = 2;
	    }
	    tmp = tmp << 2;
	    write_xr(0xC0 + tmp, (cPtr->ConsoleClk[0] & 0xFF));
	    write_xr(0xC1 + tmp, (cPtr->ConsoleClk[1] & 0xFF));
	    write_xr(0xC2 + tmp, (cPtr->ConsoleClk[2] & 0xFF));
	    write_xr(0xC3 + tmp, (cPtr->ConsoleClk[3] & 0xFF));
	} else {
	    /* 
	     * Don't use the extra 2 bits in the M, N registers available
	     * on the HiQV, so write zero to 0xCA 
	     */
	    chipsCalcClock(pScrn, Clock->Clock, vclk);
	    if (cPtr->Flags & ChipsUseVClk1) { 
		write_xr(0xC4, (vclk[1] & 0xFF));
		write_xr(0xC5, (vclk[2] & 0xFF));
		write_xr(0xC6, 0x0);
		write_xr(0xC7, (vclk[0] & 0xFF));
	    } else {
		write_xr(0xC8, (vclk[1] & 0xFF));
		write_xr(0xC9, (vclk[2] & 0xFF));
		write_xr(0xCA, 0x0);
		write_xr(0xCB, (vclk[0] & 0xFF));
	    }
	}
	usleep(10000);		         /* Let VCO stabilise    */
	write_fr(0x03, ((tmpf03 & ~0x0C) | (Clock->fr03 & 0x0C)));
	break;
    case WINGINE_1_STYLE:
	break;
    case WINGINE_2_STYLE:
	/* Only write to soft clock registers if we really need to */
	if ((Type & GET_TYPE) == TYPE_PROGRAMMABLE) {
	    /* select fixed clock 0  before tampering with VCLK select */
	    outb(0x3C2, (tmpmsr & ~0x0D) | cPtr->SuspendHack.vgaIOBaseFlag);
	    chipsCalcClock(pScrn, Clock->Clock, vclk);
	    read_xr(0x33, tmp33);     /* get status of MCLK/VCLK select reg */
	    write_xr(0x33, tmp33 & ~0x20);
	    write_xr(0x30, vclk[0]);
	    write_xr(0x31, vclk[1]);     /* restore VCLK regs.   */
	    write_xr(0x32, vclk[2]);
	    /*  write_xr(0x33, tmp33 & ~0x20);*/
	    usleep(10000);		     /* Let VCO stabilise    */
	}
	break;
    case OLD_STYLE:
	read_xr(0x02, tmp02);
	read_xr(0x54, tmp54);
	tmpfcr = inb(0x3CA);
	write_xr(0x02, ((tmp02 & ~0x02) | (Clock->xr02 & 0x02)));
	write_xr(0x54, ((tmp54 & 0xF0) | (Clock->xr54 & ~0xF0)));
	outb((int)(cPtr->IOBase + 0xA),(tmpfcr & ~0x03) & Clock->fcr);
	break;
    case NEW_STYLE:
	read_xr(0x33, tmp33);       /* get status of MCLK/VCLK select reg */
	read_xr(0x54, tmp54);
	/* Only write to soft clock registers if we really need to */
	if ((Type & GET_TYPE) == TYPE_PROGRAMMABLE) {
	    /* select fixed clock 0  before tampering with VCLK select */
	    outb(0x3C2, (tmpmsr & ~0x0D) | cPtr->SuspendHack.vgaIOBaseFlag);
	    write_xr(0x54, (tmp54 & 0xF3));
	    chipsCalcClock(pScrn, Clock->Clock, vclk);
	    write_xr(0x33, tmp33 & ~0x20);
	    write_xr(0x30, vclk[0]);
	    write_xr(0x31, vclk[1]);     /* restore VCLK regs.   */
	    write_xr(0x32, vclk[2]);
	    /*  write_xr(0x33, tmp33 & ~0x20);*/
	    usleep(10000);		         /* Let VCO stabilise    */
	}
	write_xr(0x33, ((tmp33 & ~0x80) | (Clock->xr33 & 0x80))); 
	write_xr(0x54, ((tmp54 & 0xF3) | (Clock->xr54 & ~0xF3)));
	break;
    }

    outb(0x3C2, (Clock->msr & 0xFE) | cPtr->SuspendHack.vgaIOBaseFlag);
#ifdef DEBUG
    ErrorF("restored\n");
#endif
}
   
/* 
 * This is Ken Raeburn's <raeburn@raeburn.org> clock
 * calculation code just modified a little bit to fit in here.
 */

static void
chipsCalcClock(ScrnInfoPtr pScrn, int Clock, unsigned char *vclk)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int M, N, P, PSN, PSNx;

    int bestM = 0, bestN = 0, bestP = 0, bestPSN = 0;
    double bestError, abest = 42, bestFout = 0;
    double target;

    double Fvco, Fout;
    double error, aerror;

    int M_min = 3;

    /* Hack to deal with problem of Toshiba 720CDT clock */
    int M_max = IS_HiQV(cPtr) ? 63 : 127;


    /* Other parameters available on the 65548 but not the 65545, and
     * not documented in the Clock Synthesizer doc in rev 1.0 of the
     * 65548 datasheet:
     * 
     * + XR30[4] = 0, VCO divider loop uses divide by 4 (same as 65545)
     * 1, VCO divider loop uses divide by 16
     * 
     * + XR30[5] = 1, reference clock is divided by 5
     * 
     * Other parameters available on the 65550 and not on the 65545
     * 
     * + XRCB[2] = 0, VCO divider loop uses divide by 4 (same as 65545)
     * 1, VCO divider loop uses divide by 16
     * 
     * + XRCB[1] = 1, reference clock is divided by 5
     * 
     * + XRCB[7] = Vclk = Mclk
     * 
     * + XRCA[0:1] = 2 MSB of a 10 bit M-Divisor
     * 
     * + XRCA[4:5] = 2 MSB of a 10 bit N-Divisor
     * 
     * I haven't put in any support for those here.  For simplicity,
     * they should be set to 0 on the 65548, and left untouched on
     * earlier chips.  */

    target = Clock * 1000;

    for (PSNx = 0; PSNx <= 1; PSNx++) {
	int low_N, high_N;
	double Fref4PSN;

	PSN = PSNx ? 1 : 4;

	low_N = 3;
	high_N = 127;

	while (Fref / (PSN * low_N) > 2.0e6)
	    low_N++;
	while (Fref / (PSN * high_N) < 150.0e3)
	    high_N--;

	Fref4PSN = Fref * 4 / PSN;
	for (N = low_N; N <= high_N; N++) {
	    double tmp = Fref4PSN / N;

	    for (P = IS_HiQV(cPtr) ? 1 : 0; P <= 5; P++) {	
	        /* to force post divisor on Toshiba 720CDT */
		double Fvco_desired = target * (1 << P);
		double M_desired = Fvco_desired / tmp;

		/* Which way will M_desired be rounded?  Do all three just to
		 * be safe.  */
		int M_low = M_desired - 1;
		int M_hi = M_desired + 1;

		if (M_hi < M_min || M_low > M_max)
		    continue;

		if (M_low < M_min)
		    M_low = M_min;
		if (M_hi > M_max)
		    M_hi = M_max;

		for (M = M_low; M <= M_hi; M++) {
		    Fvco = tmp * M;
		    if (Fvco <= 48.0e6)
			continue;
		    if (Fvco > 220.0e6)
			break;

		    Fout = Fvco / (1 << P);

		    error = (target - Fout) / target;

		    aerror = (error < 0) ? -error : error;
		    if (aerror < abest) {
			abest = aerror;
			bestError = error;
			bestM = M;
			bestN = N;
			bestP = P;
			bestPSN = PSN;
			bestFout = Fout;
		    }
		}
	    }
	}
    }
    vclk[0] = (bestP << (IS_HiQV(cPtr) ? 4 : 1)) + (bestPSN == 1);
    vclk[1] = bestM - 2;
    vclk[2] = bestN - 2;
#ifdef DEBUG
    ErrorF("Freq. selected: %.2f MHz, vclk[0]=%X, vclk[1]=%X, vclk[2]=%X\n",
	(float)(Clock / 1000.), vclk[0], vclk[1], vclk[2]);
    ErrorF("Freq. set: %.2f MHz\n", bestFout / 1.0e6);
#endif
}

static void
chipsSave(ScrnInfoPtr pScrn)
{
    vgaRegPtr VgaSave = &VGAHWPTR(pScrn)->SavedReg;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsSave;
    int i;
    unsigned char tmp;

    ChipsSave = &cPtr->SavedReg;

    /* set registers that we can program the controller */
    /* bank 0 */
    if (IS_HiQV(cPtr)) {
	outw(0x3D6, 0x0E);
    } else {
	outw(0x3D6, 0x10);
	outw(0x3D6, 0x11);
	outb(0x3D6, 0x0C);     /* WINgine stores MSB here */
	tmp = inb(0x3D7) & ~0x50;
	outb(0x3D7, tmp);
    }
    chipsFixResume(cPtr);
    outb(0x3D6,0x02);
    tmp = inb(0x3D7);
    outb(0x3D7,tmp & ~0x18);

    /* get generic registers */
    vgaHWSave(pScrn, VgaSave, VGA_SR_ALL);

    /* save clock */
    chipsClockSave(pScrn, &ChipsSave->Clock);

    /* save extended registers */
    if (IS_HiQV(cPtr)) {
	for (i = 0; i < 0xFF; i++) {
	    outb(0x3D6, i);
	    ChipsSave->XR[i] = inb(0x3D7);
#ifdef DEBUG
	    ErrorF("XS%X - %X\n", i, ChipsSave->XR[i]);
#endif
	}
	for (i = 0; i < 0x80; i++) {
	    outb(0x3D0, i);
	    ChipsSave->FR[i] = inb(0x3D1);
#ifdef DEBUG
	    ErrorF("FS%X - %X\n", i, ChipsSave->FR[i]);
#endif
	}
	/* Save CR0-CR40 even though we don't use them, so they can be 
	 *  printed */
	for (i = 0x0; i < 0x80; i++) {
	    outb(cPtr->IOBase + 4, i);
	    ChipsSave->CR[i] = inb(cPtr->IOBase + 5);
#ifdef DEBUG
	    ErrorF("CS%X - %X\n", i, ChipsSave->CR[i]);
#endif
	}
    } else {
	for (i = 0; i < 0x7D; i++) { /* don't touch XR7D and XR7F on WINGINE */
	    outb(0x3D6, i);
	    ChipsSave->XR[i] = inb(0x3D7);
#ifdef DEBUG
	    ErrorF("XS%X - %X\n", i, ChipsSave->XR[i]);
#endif
	}
    }
}

static Bool
chipsModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    chipsUnlock(pScrn);
    chipsFixResume(cPtr);

    if (IS_HiQV(cPtr))
	return chipsModeInitHiQV(pScrn, mode);
    else if (IS_Wingine(cPtr))
        return chipsModeInitWingine(pScrn, mode);
    else
      return chipsModeInit655xx(pScrn, mode);
}

static Bool
chipsModeInitHiQV(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i;
    int lcdHTotal, lcdHDisplay;
    int lcdVTotal, lcdVDisplay;
    int lcdHRetraceStart, lcdHRetraceEnd;
    int lcdVRetraceStart, lcdVRetraceEnd;
    int lcdHSyncStart;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsNew;
    vgaRegPtr ChipsStd;
    unsigned int tmp;
    
    ChipsNew = &cPtr->ModeReg;
    ChipsStd = &hwp->ModeReg;

    /* generic init */
    if (!vgaHWInit(pScrn, mode)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }
    pScrn->vtSema = TRUE;

    /* init clock */
    if (!chipsClockFind(pScrn, mode->ClockIndex, &ChipsNew->Clock)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }
   
    /* get C&T Specific Registers */
    for (i = 0; i < 0xFF; i++) {
	outb(0x3D6, i);
	ChipsNew->XR[i] = inb(0x3D7);
    }
    for (i = 0; i < 0x80; i++) {
	outb(0x3D0, i);
	ChipsNew->FR[i] = inb(0x3D1);
    }
    for (i = 0x30; i < 0x80; i++) {    /* These are the CT extended CRT regs */
	outb(cPtr->IOBase + 4, i);
	ChipsNew->CR[i] = inb(cPtr->IOBase + 5);
    }

    /*
     * Here all of the other fields of 'ChipsNew' get filled in, to
     * handle the SVGA extended registers.  It is also allowable
     * to override generic registers whenever necessary.
     */

    /* some generic settings */
    if (pScrn->depth == 1) {
	ChipsStd->Attribute[0x10] = 0x03;   /* mode */
    } else {
	ChipsStd->Attribute[0x10] = 0x01;   /* mode */
    }
    ChipsStd->Attribute[0x11] = 0x00;   /* overscan (border) color */
    ChipsStd->Attribute[0x12] = 0x0F;   /* enable all color planes */
    ChipsStd->Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

    ChipsStd->Graphics[0x05] = 0x00;    /* normal read/write mode */

    /* set virtual screen width */
    tmp = pScrn->displayWidth >> 3;
    if (pScrn->bitsPerPixel == 16) {
	tmp <<= 1;		       /* double the width of the buffer */
    } else if (pScrn->bitsPerPixel == 24) {
	tmp += tmp << 1;
    } else if (pScrn->bitsPerPixel == 32) {
	tmp <<= 2;
    } else if (pScrn->bitsPerPixel < 8) {
	tmp >>= 1;
    }
    ChipsStd->CRTC[0x13] = tmp & 0xFF;
    ChipsNew->CR[0x41] = (tmp >> 8) & 0x0F;

    /* Set paging mode on the HiQV32 architecture, if required */
    if ((xf86IsOptionSet(cPtr->Options, OPTION_NOLINEAR_MODE)) ||
	    (pScrn->bitsPerPixel < 8))
	ChipsNew->XR[0x0A] |= 0x1;

    ChipsNew->XR[0x09] |= 0x1;	       /* Enable extended CRT registers */
    ChipsNew->XR[0x0E] = 0;           /* Single map */
    ChipsNew->XR[0x40] |= 0x2;	       /* Don't wrap at 256kb */
    ChipsNew->XR[0x81] &= 0xF8;
    if (pScrn->bitsPerPixel >= 8) {
        ChipsNew->XR[0x40] |= 0x1;    /* High Resolution. XR40[1] reserved? */
	ChipsNew->XR[0x81] |= 0x2;    /* 256 Color Video */
    }
    ChipsNew->XR[0x80] |= 0x10;       /* Enable cursor output on P0 and P1 */

    if (abs(cPtr->MemClock.Clk - cPtr->MemClock.ProbedClk) > 50) {
	/* set mem clk */
	ChipsNew->XR[0xCC] = cPtr->MemClock.xrCC;
	ChipsNew->XR[0xCD] = cPtr->MemClock.xrCD;
	ChipsNew->XR[0xCE] = cPtr->MemClock.xrCE;
    }

    /* linear specific */
    if (cPtr->Flags & ChipsLinearSupport) {
	ChipsNew->XR[0x0A] |= 0x02;   /* Linear Addressing Mode */
	ChipsNew->XR[0x20] = 0x0;     /*BitBLT Draw Mode for 8 */
	ChipsNew->XR[0x05] =
	    (unsigned char)((cPtr->FbAddress >> 16) & 0xFF);
	ChipsNew->XR[0x06] = 
	    (unsigned char)((cPtr->FbAddress >> 24) & 0xFF);
    }

    /* panel timing */
    /* By default don't set panel timings, but allow it as an option */
    if (xf86IsOptionSet(cPtr->Options, OPTION_USE_MODELINE)) {
	lcdHTotal = (mode->CrtcHTotal >> 3) - 5;
	lcdHDisplay = (cPtr->PanelSize.HDisplay >> 3) - 1;
	lcdHRetraceStart = (mode->CrtcHSyncStart >> 3);
	lcdHRetraceEnd = (mode->CrtcHSyncEnd >> 3);
	lcdHSyncStart = lcdHRetraceStart - 2;

	lcdVTotal = mode->CrtcVTotal - 2;
	lcdVDisplay = cPtr->PanelSize.VDisplay - 1;
	lcdVRetraceStart = mode->CrtcVSyncStart;
	lcdVRetraceEnd = mode->CrtcVSyncEnd;

	ChipsNew->FR[0x20] = lcdHDisplay & 0xFF;
	ChipsNew->FR[0x21] = lcdHRetraceStart & 0xFF;
	ChipsNew->FR[0x25] = ((lcdHRetraceStart & 0xF00) >> 4) |
	    ((lcdHDisplay & 0xF00) >> 8);
	ChipsNew->FR[0x22] = lcdHRetraceEnd & 0x1F;
	ChipsNew->FR[0x23] = lcdHTotal & 0xFF;
	ChipsNew->FR[0x24] = (lcdHSyncStart >> 3) & 0xFF;
	ChipsNew->FR[0x26] = (ChipsNew->FR[0x26] & ~0x1F)
	    | ((lcdHTotal & 0xF00) >> 8)
	    | (((lcdHSyncStart >> 3) & 0x100) >> 4);
	ChipsNew->FR[0x27] &= 0x7F;

	ChipsNew->FR[0x30] = lcdVDisplay & 0xFF;
	ChipsNew->FR[0x31] = lcdVRetraceStart & 0xFF;
	ChipsNew->FR[0x35] = ((lcdVRetraceStart & 0xF00) >> 4)
	    | ((lcdVDisplay & 0xF00) >> 8);
	ChipsNew->FR[0x32] = lcdVRetraceEnd & 0x0F;
	ChipsNew->FR[0x33] = lcdVTotal & 0xFF;
	ChipsNew->FR[0x34] = (lcdVTotal - lcdVRetraceStart) & 0xFF;
	ChipsNew->FR[0x36] = ((lcdVTotal & 0xF00) >> 8) |
	    (((lcdVTotal - lcdVRetraceStart) & 0x700) >> 4);
	ChipsNew->FR[0x37] |= 0x80;
    }

    /* Set up the extended CRT registers of the HiQV32 chips */
    ChipsNew->CR[0x30] = ((mode->CrtcVTotal - 2) & 0xF00) >> 8;
    ChipsNew->CR[0x31] = ((mode->CrtcVDisplay - 1) & 0xF00) >> 8;
    ChipsNew->CR[0x32] = (mode->CrtcVSyncStart & 0xF00) >> 8;
    ChipsNew->CR[0x33] = (mode->CrtcVBlankStart & 0xF00) >> 8;
    if (cPtr->Chipset == CHIPS_CT69000) {
	/* The 69000 has overflow bits for the horizontal values as well */
	ChipsNew->CR[0x38] = (((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8;
	ChipsNew->CR[0x3C] = ((mode->CrtcHSyncEnd >> 3) & 0xC0);
    }
    ChipsNew->CR[0x40] |= 0x80;

    /* centering/stretching */
    if (!xf86IsOptionSet(cPtr->Options, OPTION_SUSPEND_HACK)) {
	if (xf86IsOptionSet(cPtr->Options, OPTION_LCD_STRETCH)) {
	    ChipsNew->FR[0x40] &= 0xDF;    /* Disable Horizontal stretching */
	    ChipsNew->FR[0x48] &= 0xFB;    /* Disable vertical stretching */
	    ChipsNew->XR[0xA0] = 0x10;     /* Disable cursor stretching */
	    cPtr->Accel.UseHWCursor = TRUE;
	} else {
	    ChipsNew->FR[0x40] |= 0x21;    /* Enable Horizontal stretching */
	    ChipsNew->FR[0x48] |= 0x05;    /* Enable vertical stretching */
	    ChipsNew->XR[0xA0] = 0x70;     /* Enable cursor stretching */
	    if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CURSOR))
		cPtr->Accel.UseHWCursor = TRUE;      /* H/W  cursor forced */
	    else
		cPtr->Accel.UseHWCursor = FALSE;     /* Possible H/W bug? */
	}
    }

    if (!xf86IsOptionSet(cPtr->Options, OPTION_LCD_CENTER)) {
	ChipsNew->FR[0x40] &= 0xFD;    /* Disable Horizontal centering */
	ChipsNew->FR[0x48] &= 0xFD;    /* Disable Vertical centering */
    } else {
	ChipsNew->FR[0x40] |= 0x3;    /* Enable Horizontal centering */
	ChipsNew->FR[0x48] |= 0x3;    /* Enable Vertical centering */
    }

    /* sync on green */
    if (xf86IsOptionSet(cPtr->Options, OPTION_SYNC_ON_GREEN))
	ChipsNew->XR[0x82] |=0x02;

    /* software mode flag */
    ChipsNew->XR[0xE2] = chipsVideoMode(pScrn->bitsPerPixel,
	pScrn->weight.green, (cPtr->PanelType & ChipsLCD) ?
	min(mode->CrtcHDisplay, cPtr->PanelSize.HDisplay) :
	mode->CrtcHDisplay, mode->CrtcVDisplay);
#ifdef DEBUG
    ErrorF("VESA Mode: %Xh\n", ChipsNew->XR[0xE2]);
#endif

    /* sync. polarities */
    if ((mode->Flags & (V_PHSYNC | V_NHSYNC))
	&& (mode->Flags & (V_PVSYNC | V_NVSYNC))) {
	if (mode->Flags & (V_PHSYNC | V_NHSYNC)) {
	    if (mode->Flags & V_PHSYNC)
		ChipsNew->FR[0x08] &= 0xBF;	/* Alt. CRT Hsync positive */
	    else
		ChipsNew->FR[0x08] |= 0x40;	/* Alt. CRT Hsync negative */
	}
	if (mode->Flags & (V_PVSYNC | V_NVSYNC)) {
	    if (mode->Flags & V_PVSYNC)
		ChipsNew->FR[0x08] &= 0x7F;	/* Alt. CRT Vsync positive */
	    else
		ChipsNew->FR[0x08] |= 0x80;	/* Alt. CRT Vsync negative */
	}
    }

    /* bpp depend */
    if (pScrn->bitsPerPixel == 16) {
	ChipsNew->XR[0x81] = (ChipsNew->XR[0x81] & 0xF0) | 0x4;
	/* 16bpp = 5-5-5             */
	ChipsNew->FR[0x10] |= 0x0C;   /*Colour Panel               */
	ChipsNew->XR[0x20] = 0x10;    /*BitBLT Draw Mode for 16 bpp */
	if (pScrn->weight.green != 5)
	    ChipsNew->XR[0x81] |= 0x01;	/*16bpp */
    } else if (pScrn->bitsPerPixel == 24) {
	ChipsNew->XR[0x81] = (ChipsNew->XR[0x81] & 0xF0) | 0x6;
	/* 24bpp colour              */
	ChipsNew->XR[0x20] = 0x20;    /*BitBLT Draw Mode for 24 bpp */
    } else if (pScrn->bitsPerPixel == 32) {
	ChipsNew->XR[0x81] = (ChipsNew->XR[0x81] & 0xF0) | 0x7;
	/* 32bpp colour              */
	ChipsNew->XR[0x20] = 0x10;    /*BitBLT Draw Mode for 16 bpp */
    }

    /*CRT only */
    if (!(cPtr->PanelType & ChipsLCD)) {
	if (mode->Flags & V_INTERLACE) {
	    ChipsNew->CR[0x70] = 0x80          /*   set interlace */
	      | (((((mode->CrtcHDisplay >> 3) - 1) >> 1) - 6) & 0x7F);
	    /* 
	     ** Double VDisplay to get back the full screen value, otherwise
	     ** you only see half the picture.
	     */
	    mode->CrtcVDisplay = mode->VDisplay;
	    tmp = ChipsStd->CRTC[7] & ~0x42;
	    ChipsStd->CRTC[7] = (tmp | 
				((((mode->CrtcVDisplay -1) & 0x100) >> 7 ) |
				 (((mode->CrtcVDisplay -1) & 0x200) >> 3 )));
	    ChipsStd->CRTC[0x12] = (mode->CrtcVDisplay -1) & 0xFF;
	    ChipsNew->CR[0x31] = ((mode->CrtcVDisplay - 1) & 0xF00) >> 8;
	} else {
	    ChipsNew->CR[0x70] &= ~0x80;	/* unset interlace */
	}
    }
    
#if defined(__arm32__) && defined(__NetBSD__)
    if (cPtr->TVMode != XMODE_RGB) {
	/*
	 * Put the console into TV Out mode.
	 */
	xf86SetTVOut(cPtr->TVMode);
	
	ChipsNew->CR[0x72] = (mode->CrtcHTotal >> 1) >> 3;/* First horizontal
							   * serration pulse */
	ChipsNew->CR[0x73] = mode->CrtcHTotal >> 3; /* Second pulse */
	ChipsNew->CR[0x74] = (((mode->HSyncEnd - mode->HSyncStart) >> 3) - 1)
					& 0x1F; /* equalization pulse */
	
	if (cPtr->TVMode == XMODE_PAL || cPtr->TVMode == XMODE_SECAM) {
	    ChipsNew->CR[0x71] = 0xA0; /* PAL support with blanking delay */
	} else {
	    ChipsNew->CR[0x71] = 0x20; /* NTSC support with blanking delay */
	}
    } else {	/* XMODE_RGB */
	/*
	 * Put the console into RGB Out mode.
	 */
	xf86SetRGBOut();
    }
#endif

    /* STN specific */
    if (IS_STN(cPtr->PanelType)) {
	ChipsNew->FR[0x11] &= ~0x03;	/* FRC clear                    */
	ChipsNew->FR[0x11] &= ~0x8C;	/* Dither clear                 */
	ChipsNew->FR[0x11] |= 0x01;	/* 16 frame FRC                 */
	ChipsNew->FR[0x11] |= 0x84;	/* Dither                       */
	if (cPtr->Flags & ChipsTMEDSupport) {
	    ChipsNew->FR[0x73] &= 0x4F; /* Clear TMED                   */
	    ChipsNew->FR[0x73] |= 0x80; /* Enable TMED                  */
	    ChipsNew->FR[0x73] |= 0x30; /* TMED 256 Shades of RGB       */
	}
	if (cPtr->PanelType & ChipsDD)	/* Shift Clock Mask. Use to get */
	    ChipsNew->FR[0x12] |= 0x4;	/* rid of line in DSTN screens  */
    }

    /* Program the registers */
    /*vgaHWProtect(pScrn, TRUE);*/
    chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
    /*vgaHWProtect(pScrn, FALSE);*/
    
    return(TRUE);
}

static Bool
chipsModeInitWingine(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i, bytesPerPixel;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsNew;
    vgaRegPtr ChipsStd;
    unsigned int tmp;

    ChipsNew = &cPtr->ModeReg;
    ChipsStd = &hwp->ModeReg;

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /* 
     * This chipset seems to have problems if 
     * HBlankEnd is choosen equals HTotal
     */
    if (!mode->CrtcHAdjusted)
      mode->CrtcHBlankEnd = min(mode->CrtcHSyncEnd, mode->CrtcHTotal - 2);

    /* correct the timings for 16/24 bpp */
    if (pScrn->bitsPerPixel == 16) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay <<= 1;
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart <<= 1;
	    mode->CrtcHSyncEnd <<= 1;
	    mode->CrtcHBlankStart <<= 1;
	    mode->CrtcHBlankEnd <<= 1;
	    mode->CrtcHTotal <<= 1;
	    mode->CrtcHAdjusted = TRUE;
	}
    } else if (pScrn->bitsPerPixel == 24) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay += ((mode->CrtcHDisplay) << 1);
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart += ((mode->CrtcHSyncStart) << 1);
	    mode->CrtcHSyncEnd += ((mode->CrtcHSyncEnd) << 1);
	    mode->CrtcHBlankStart += ((mode->CrtcHBlankStart) << 1);
	    mode->CrtcHBlankEnd += ((mode->CrtcHBlankEnd) << 1);
	    mode->CrtcHTotal += ((mode->CrtcHTotal) << 1);
	    mode->CrtcHAdjusted = TRUE;
	}
    }

    /* generic init */
    if (!vgaHWInit(pScrn, mode)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }
    pScrn->vtSema = TRUE;
    
    /* init clock */
    if (!chipsClockFind(pScrn, mode->ClockIndex, &ChipsNew->Clock)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }

    /* get  C&T Specific Registers */
    for (i = 0; i < 0x7D; i++) {   /* don't touch XR7D and XR7F on WINGINE */
	outb(0x3D6, i);
	ChipsNew->XR[i] = inb(0x3D7);
    }

    /* some generic settings */
    if (pScrn->bitsPerPixel == 1) {
	ChipsStd->Attribute[0x10] = 0x03;   /* mode */
    } else {
	ChipsStd->Attribute[0x10] = 0x01;   /* mode */
    }
    ChipsStd->Attribute[0x11] = 0x00;   /* overscan (border) color */
    ChipsStd->Attribute[0x12] = 0x0F;   /* enable all color planes */
    ChipsStd->Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

    ChipsStd->Graphics[0x05] = 0x00;    /* normal read/write mode */

    /* set virtual screen width */
    if (pScrn->bitsPerPixel >= 8)
	ChipsStd->CRTC[0x13] = (pScrn->displayWidth * bytesPerPixel) >> 3;
    else
	ChipsStd->CRTC[0x13] = pScrn->displayWidth >> 4;

    
    /* set C&T Specific Registers */
    /* set virtual screen width */
    if (pScrn->bitsPerPixel >= 8)
	tmp = (pScrn->displayWidth >> 4) * bytesPerPixel;
    else
	tmp = (pScrn->displayWidth >> 5);
    ChipsNew->XR[0x0D] = (tmp & 0x80) >> 5; 

    ChipsNew->XR[0x04] |= 4;	       /* enable addr counter bits 16-17 */
    /* XR04: Memory control 1 */
    /* bit 2: Memory Wraparound */
    /*        Enable CRTC addr counter bits 16-17 if set */

    ChipsNew->XR[0x0B] |= 0x07;       /* extended mode, dual pages enabled */
    ChipsNew->XR[0x0B] &= ~0x10;      /* linear mode off */
    /* XR0B: CPU paging */
    /* bit 0: Memory mapping mode */
    /*        VGA compatible if 0 (default) */
    /*        Extended mode (mapping for > 256 kB mem) if 1 */
    /* bit 1: CPU single/dual mapping */
    /*        0, CPU uses only a single map to access (default) */
    /*        1, CPU uses two maps to access */
    /* bit 2: CPU address divide by 4 */

    ChipsNew->XR[0x10] = 0;	       /* XR10: Single/low map */
    ChipsNew->XR[0x11] = 0;	       /* XR11: High map      */
    ChipsNew->XR[0x0C] &= ~0x50;       /* MSB for XR10 & XR11  */ 
    if (pScrn->bitsPerPixel >= 8) {
	ChipsNew->XR[0x28] |= 0x10;       /* 256-color video     */
    } else {
	ChipsNew->XR[0x28] &= 0xEF;       /* 16-color video      */
    }
    /* set up extended display timings */
    /* in CRTonly mode this is simple: only set overflow for CR00-CR06 */
    ChipsNew->XR[0x17] = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8)
	| ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7)
	| ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6)
	| ((((mode->CrtcHSyncEnd >> 3)) & 0x20) >> 2)
	| ((((mode->CrtcHBlankStart >> 3) - 1) & 0x100) >> 4)
	| ((((mode->CrtcHBlankEnd >> 3) - 1) & 0x40) >> 1);


    ChipsNew->XR[0x16]  = (((mode->CrtcVTotal -2) & 0x400) >> 10 )
	| (((mode->CrtcVDisplay -1) & 0x400) >> 9 )
	| ((mode->CrtcVSyncStart & 0x400) >> 8 )
	| (((mode->CrtcVBlankStart) & 0x400) >> 6 );

    /* set video mode */
    ChipsNew->XR[0x2B] = chipsVideoMode(pScrn->bitsPerPixel,
	pScrn->weight.green, mode->CrtcHDisplay, mode->CrtcVDisplay);
#ifdef DEBUG
    ErrorF("VESA Mode: %Xh\n", ChipsNew->XR[0x2B]);
#endif

    /* set some linear specific registers */
    if (cPtr->Flags & ChipsLinearSupport) {
	/* enable linear addressing  */
	ChipsNew->XR[0x0B] &= 0xFD;   /* dual page clear                */
	ChipsNew->XR[0x0B] |= 0x10;   /* linear mode on                 */

	ChipsNew->XR[0x08] = 
	  (unsigned char)((cPtr->FbAddress >> 16) & 0xFF);    
	ChipsNew->XR[0x09] = 
	  (unsigned char)((cPtr->FbAddress >> 24) & 0xFF);    

	/* general setup */
	ChipsNew->XR[0x40] = 0x01;    /*BitBLT Draw Mode for 8 and 24 bpp */
    }

    /* common general setup */
    ChipsNew->XR[0x52] |= 0x01;       /* Refresh count                   */
    ChipsNew->XR[0x0F] &= 0xEF;       /* not Hi-/True-Colour             */
    ChipsNew->XR[0x02] &= 0xE7;       /* Attr. Cont. default access      */
                                       /* use ext. regs. for hor. in dual */
    ChipsNew->XR[0x06] &= 0xF3;       /* bpp clear                       */

    /* bpp depend */
    /*XR06: Palette control */
    /* bit 0: Pixel Data Pin Diag, 0 for flat panel pix. data (def)  */
    /* bit 1: Internal DAC disable                                   */
    /* bit 3-2: Colour depth, 0 for 4 or 8 bpp, 1 for 16(5-5-5) bpp, */
    /*          2 for 24 bpp, 3 for 16(5-6-5)bpp                     */
    /* bit 4:   Enable PC Video Overlay on colour key                */
    /* bit 5:   Bypass Internal VGA palette                          */
    /* bit 7-6: Colour reduction select, 0 for NTSC (default),       */
    /*          1 for Equivalent weighting, 2 for green only,        */
    /*          3 for Colour w/o reduction                           */
    /* XR50 Panel Format Register 1                                  */
    /* bit 1-0: Frame Rate Control; 00, No FRC;                      */
    /*          01, 16-frame FRC for colour STN and monochrome       */
    /*          10, 2-frame FRC for colour TFT or monochrome;        */
    /*          11, reserved                                         */
    /* bit 3-2: Dither Enable                                        */
    /*          00, disable dithering; 01, enable dithering          */
    /*          for 256 mode                                         */
    /*          10, enable dithering for all modes; 11, reserved     */
    /* bit6-4: Clock Divide (CD)                                     */
    /*          000, Shift Clock Freq = Dot Clock Freq;              */
    /*          001, SClk = DClk/2; 010 SClk = DClk/4;               */
    /*          011, SClk = DClk/8; 100 SClk = DClk/16;              */
    /* bit 7: TFT data width                                         */
    /*          0, 16 bit(565RGB); 1, 24bit (888RGB)                 */
    if (pScrn->bitsPerPixel == 16) {
	ChipsNew->XR[0x06] |= 0xC4;   /*15 or 16 bpp colour         */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
	ChipsNew->XR[0x40] = 0x02;    /*BitBLT Draw Mode for 16 bpp */
	if (pScrn->weight.green != 5)
	    ChipsNew->XR[0x06] |= 0x08;	/*16bpp              */
    } else if (pScrn->bitsPerPixel == 24) {
	ChipsNew->XR[0x06] |= 0xC8;   /*24 bpp colour               */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
    }

    /*CRT only: interlaced mode */
    if (mode->Flags & V_INTERLACE) {
	ChipsNew->XR[0x28] |= 0x20;    /* set interlace         */
	/* empirical value       */
	tmp = ((((mode->CrtcHDisplay >> 3) - 1) >> 1) 
	       - 6 * (pScrn->bitsPerPixel >= 8 ? bytesPerPixel : 1 ));
	ChipsNew->XR[0x19] = tmp & 0xFF;
	ChipsNew->XR[0x17] |= ((tmp & 0x100) >> 1); /* overflow */
	ChipsNew->XR[0x0F] &= ~0x40;   /* set SW-Flag           */
    } else {
	ChipsNew->XR[0x28] &= ~0x20;   /* unset interlace       */
	ChipsNew->XR[0x0F] |=  0x40;   /* set SW-Flag           */
    }

    /* Program the registers */
    /*vgaHWProtect(pScrn, TRUE);*/
    chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
    /*vgaHWProtect(pScrn, FALSE);*/

    return (TRUE);
}

static Bool
chipsModeInit655xx(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i, bytesPerPixel;
    int lcdHTotal, lcdHDisplay;
    int lcdVTotal, lcdVDisplay;
    int lcdHRetraceStart, lcdHRetraceEnd;
    int lcdVRetraceStart, lcdVRetraceEnd;
    int HSyncStart, HDisplay;
    int CrtcHDisplay;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsNew;
    vgaRegPtr ChipsStd;
    unsigned int tmp;

    ChipsNew = &cPtr->ModeReg;
    ChipsStd = &hwp->ModeReg;

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /*
     * Possibly fix up the panel size, if the manufacture is stupid
     * enough to set it incorrectly in text modes
     */
    if (xf86IsOptionSet(cPtr->Options, OPTION_PANEL_SIZE)) {
	cPtr->PanelSize.HDisplay = mode->CrtcHDisplay;
	cPtr->PanelSize.VDisplay = mode->CrtcVDisplay;
    }
    
    /* 
     * This chipset seems to have problems if 
     * HBlankEnd is choosen equals HTotal
     */
    if (!mode->CrtcHAdjusted)
      mode->CrtcHBlankEnd = min(mode->CrtcHSyncEnd, mode->CrtcHTotal - 2);

    /* correct the timings for 16/24 bpp */
    if (pScrn->bitsPerPixel == 16) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay <<= 1;
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart <<= 1;
	    mode->CrtcHSyncEnd <<= 1;
	    mode->CrtcHBlankStart <<= 1;
	    mode->CrtcHBlankEnd <<= 1;
	    mode->CrtcHTotal <<= 1;
	    mode->CrtcHAdjusted = TRUE;
	}
    } else if (pScrn->bitsPerPixel == 24) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay += ((mode->CrtcHDisplay) << 1);
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart += ((mode->CrtcHSyncStart) << 1);
	    mode->CrtcHSyncEnd += ((mode->CrtcHSyncEnd) << 1);
	    mode->CrtcHBlankStart += ((mode->CrtcHBlankStart) << 1);
	    mode->CrtcHBlankEnd += ((mode->CrtcHBlankEnd) << 1);
	    mode->CrtcHTotal += ((mode->CrtcHTotal) << 1);
	    mode->CrtcHAdjusted = TRUE;
	}
    }
	   
    /* store orig. HSyncStart needed for flat panel mode */
    HSyncStart = mode->CrtcHSyncStart / (pScrn->bitsPerPixel >= 8 ? 
					 bytesPerPixel : 1 ) - 16;
    HDisplay = (mode->CrtcHDisplay + 1) /  (pScrn->bitsPerPixel >= 8 ? 
					 bytesPerPixel : 1 );
    
    /* generic init */
    if (!vgaHWInit(pScrn, mode)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }
    pScrn->vtSema = TRUE;
    
    /* init clock */
    if (!chipsClockFind(pScrn, mode->ClockIndex, &ChipsNew->Clock)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }

    /* get  C&T Specific Registers */
    for (i = 0; i < 0x80; i++) {
	outb(0x3D6, i);
	ChipsNew->XR[i] = inb(0x3D7);
    }

    /* some generic settings */
    if (pScrn->bitsPerPixel == 1) {
	ChipsStd->Attribute[0x10] = 0x03;   /* mode */
    } else {
	ChipsStd->Attribute[0x10] = 0x01;   /* mode */
    }
    ChipsStd->Attribute[0x11] = 0x00;   /* overscan (border) color */
    ChipsStd->Attribute[0x12] = 0x0F;   /* enable all color planes */
    ChipsStd->Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

    ChipsStd->Graphics[0x05] = 0x00;    /* normal read/write mode */

    /* set virtual screen width */
    if (pScrn->bitsPerPixel >= 8)
	ChipsStd->CRTC[0x13] = (pScrn->displayWidth * bytesPerPixel) >> 3;
    else
	ChipsStd->CRTC[0x13] = pScrn->displayWidth >> 4;

    
    /* set C&T Specific Registers */
    /* set virtual screen width */
    ChipsNew->XR[0x1E] = ChipsStd->CRTC[0x13];	/* alternate offset */
    /*databook is not clear about 0x1E might be needed for 65520/30 */
    if (pScrn->bitsPerPixel >= 8)
	tmp = (pScrn->displayWidth * bytesPerPixel) >> 2;
    else
	tmp = pScrn->displayWidth >> 3;
    ChipsNew->XR[0x0D] = (tmp & 0x01) | ((tmp << 1) & 0x02)  ; 

    ChipsNew->XR[0x04] |= 4;	       /* enable addr counter bits 16-17 */
    /* XR04: Memory control 1 */
    /* bit 2: Memory Wraparound */
    /*        Enable CRTC addr counter bits 16-17 if set */

    ChipsNew->XR[0x0B] |= 0x07;       /* extended mode, dual pages enabled */
    ChipsNew->XR[0x0B] &= ~0x10;      /* linear mode off */
    /* XR0B: CPU paging */
    /* bit 0: Memory mapping mode */
    /*        VGA compatible if 0 (default) */
    /*        Extended mode (mapping for > 256 kB mem) if 1 */
    /* bit 1: CPU single/dual mapping */
    /*        0, CPU uses only a single map to access (default) */
    /*        1, CPU uses two maps to access */
    /* bit 2: CPU address divide by 4 */

    ChipsNew->XR[0x10] = 0;	       /* XR10: Single/low map */
    ChipsNew->XR[0x11] = 0;	       /* XR11: High map      */
    if (pScrn->bitsPerPixel >= 8) {
	ChipsNew->XR[0x28] |= 0x10;       /* 256-color video     */
    } else {
	ChipsNew->XR[0x28] &= 0xEF;       /* 16-color video      */
    }
    /* set up extended display timings */
    if (cPtr->PanelType & ChipsCRT) {
	/* in CRTonly mode this is simple: only set overflow for CR00-CR06 */
	ChipsNew->XR[0x17] = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8)
	    | ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7)
	    | ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6)
	    | ((((mode->CrtcHSyncEnd >> 3)) & 0x20) >> 2)
	    | ((((mode->CrtcHBlankStart >> 3) - 1) & 0x100) >> 4)
	    | ((((mode->CrtcHBlankEnd >> 3) - 1) & 0x40) >> 1);

	ChipsNew->XR[0x16]  = (((mode->CrtcVTotal -2) & 0x400) >> 10 )
	    | (((mode->CrtcVDisplay -1) & 0x400) >> 9 )
	    | ((mode->CrtcVSyncStart & 0x400) >> 8 )
	    | (((mode->CrtcVBlankStart) & 0x400) >> 6 );
    } else {
	/* horizontal timing registers */
	/* in LCD/dual mode use saved bios values to derive timing values if
	 * not told otherwise */
	if (!xf86IsOptionSet(cPtr->Options, OPTION_USE_MODELINE)) {
	    lcdHTotal = cPtr->PanelSize.HTotal;
	    lcdHRetraceStart = cPtr->PanelSize.HRetraceStart;
	    lcdHRetraceEnd = cPtr->PanelSize.HRetraceEnd;
	    if (pScrn->bitsPerPixel == 16) {
		lcdHRetraceStart <<= 1;
		lcdHRetraceEnd <<= 1;
		lcdHTotal <<= 1;
	    } else if (pScrn->bitsPerPixel == 24) {
		lcdHRetraceStart += (lcdHRetraceStart << 1);
		lcdHRetraceEnd += (lcdHRetraceEnd << 1);
		lcdHTotal += (lcdHTotal << 1);
	    }
 	    lcdHRetraceStart -=8;       /* HBlank =  HRetrace - 1: for */
 	    lcdHRetraceEnd   -=8;       /* compatibility with vgaHW.c  */
	} else {
	    /* use modeline values if bios values don't work */
	    lcdHTotal = mode->CrtcHTotal;
	    lcdHRetraceStart = mode->CrtcHSyncStart;
	    lcdHRetraceEnd = mode->CrtcHSyncEnd;
	}
	/* The chip takes the size of the visible display area from the
	 * CRTC values. We use bios screensize for LCD in LCD/dual mode
	 * wether or not we use modeline for LCD. This way we can specify
	 * always specify a smaller than default display size on LCD
	 * by writing it to the CRTC registers. */
	lcdHDisplay = cPtr->PanelSize.HDisplay;
	if (pScrn->bitsPerPixel == 16) {
	    lcdHDisplay++;
	    lcdHDisplay <<= 1;
	    lcdHDisplay--;
	} else if (pScrn->bitsPerPixel == 24) {
	    lcdHDisplay++;
	    lcdHDisplay += (lcdHDisplay << 1);
	    lcdHDisplay--;
	}
	lcdHTotal = (lcdHTotal >> 3) - 5;
	lcdHDisplay = (lcdHDisplay >> 3) - 1;
	lcdHRetraceStart = (lcdHRetraceStart >> 3);
	lcdHRetraceEnd = (lcdHRetraceEnd >> 3);
	/* This ugly hack is needed because CR01 and XR1C share the 8th bit!*/
	CrtcHDisplay = ((mode->CrtcHDisplay >> 3) - 1);
	if ((lcdHDisplay & 0x100) != (CrtcHDisplay & 0x100)) {
	    ErrorF("This display configuration might cause problems !\n");
	    lcdHDisplay = 255;
	}

	/* now init register values */
	ChipsNew->XR[0x17] = (((lcdHTotal) & 0x100) >> 8)
	    | ((lcdHDisplay & 0x100) >> 7)
	    | ((lcdHRetraceStart & 0x100) >> 6)
	    | (((lcdHRetraceEnd) & 0x20) >> 2);

	ChipsNew->XR[0x19] = lcdHRetraceStart & 0xFF;
	ChipsNew->XR[0x1A] = lcdHRetraceEnd & 0x1F;

	/* XR1B: Alternate horizontal total */
	/* used in all flat panel mode with horiz. compression disabled, */
	/* CRT CGA text and graphic modes and Hercules graphics mode */
	/* similar to CR00, actual value - 5 */
	ChipsNew->XR[0x1B] = lcdHTotal & 0xFF;

	/*XR1C: Alternate horizontal blank start (CRT mode) */
	/*      /horizontal panel size (FP mode) */
	/* FP horizontal panel size (FP mode), */
	/* actual value - 1 (in characters unit) */
	/* CRT horizontal blank start (CRT mode) */
	/* similar to CR02, actual value - 1 */
	ChipsNew->XR[0x1C] = lcdHDisplay & 0xFF;

	if (xf86IsOptionSet(cPtr->Options, OPTION_USE_MODELINE)) {
	    /* for ext. packed pixel mode on 64520/64530 */
	    /* no need to rescale: used only in 65530    */
	    ChipsNew->XR[0x21] = lcdHRetraceStart & 0xFF;
	    ChipsNew->XR[0x22] = lcdHRetraceEnd & 0x1F;
	    ChipsNew->XR[0x23] = lcdHTotal & 0xFF;

	    /* vertical timing registers */
	    lcdVTotal = mode->CrtcVTotal - 2;
	    lcdVDisplay = cPtr->PanelSize.VDisplay - 1;
	    lcdVRetraceStart = mode->CrtcVSyncStart;
	    lcdVRetraceEnd = mode->CrtcVSyncEnd;

	    ChipsNew->XR[0x64] = lcdVTotal & 0xFF;
	    ChipsNew->XR[0x66] = lcdVRetraceStart & 0xFF;
	    ChipsNew->XR[0x67] = lcdVRetraceEnd & 0x0F;
	    ChipsNew->XR[0x68] = lcdVDisplay & 0xFF;
	    ChipsNew->XR[0x65] = ((lcdVTotal & 0x100) >> 8)
		| ((lcdVDisplay & 0x100) >> 7)
		| ((lcdVRetraceStart & 0x100) >> 6)
		| ((lcdVRetraceStart & 0x400) >> 7)
		| ((lcdVTotal & 0x400) >> 6)
		| ((lcdVTotal & 0x200) >> 4)
		| ((lcdVDisplay & 0x200) >> 3)
		| ((lcdVRetraceStart & 0x200) >> 2);

	    /* 
	     * These are important: 0x2C specifies the numbers of lines 
	     * (hsync pulses) between vertical blank start and vertical 
	     * line total, 0x2D specifies the number of clock ticks? to
	     * horiz. blank start ( caution ! 16bpp/24bpp modes: that's
	     * why we need HSyncStart - can't use mode->CrtcHSyncStart) 
	     */
	    tmp = ((cPtr->PanelType & ChipsDD) && !(ChipsNew->XR[0x6F] & 0x02))
	      ? 1 : 0; /* double LP delay, FLM: 2 lines iff DD+no acc*/
	    /* Currently we support 2 FLM schemes: #1: FLM coincides with
	     * VTotal ie. the delay is programmed to the difference bet-
	     * ween lctVTotal and lcdVRetraceStart.    #2: FLM coincides
	     * lcdVRetraceStart - in this case FLM delay will be turned
	     * off. To decide which scheme to use we compare the value of
	     * XR2C set by the bios to the two schemes. The one that fits
	     * better will be used.
	     */

	    if (ChipsNew->XR[0x2C]  < abs((cPtr->PanelSize.VTotal -
		    cPtr->PanelSize.VRetraceStart - tmp - 1) -
		    ChipsNew->XR[0x2C]))
	        ChipsNew->XR[0x2F] |= 0x80;   /* turn FLM delay off */
	    ChipsNew->XR[0x2C] = lcdVTotal - lcdVRetraceStart - tmp;
	    /*ChipsNew->XR[0x2D] = (HSyncStart >> (3 - tmp)) & 0xFF;*/
	    ChipsNew->XR[0x2D] = (HDisplay >> (3 - tmp)) & 0xFF;
	    ChipsNew->XR[0x2F] = (ChipsNew->XR[0x2F] & 0xDF)
		| (((HSyncStart >> (3 - tmp)) & 0x100) >> 3);
	}

	/* set stretching/centering */	
	if (!xf86IsOptionSet(cPtr->Options, OPTION_SUSPEND_HACK)) {
	    ChipsNew->XR[0x51] |= 0x40;   /* enable FP compensation          */
	    ChipsNew->XR[0x55] |= 0x01;   /* enable horiz. compensation      */
	    ChipsNew->XR[0x57] |= 0x01;   /* enable horiz. compensation      */
	    if (xf86IsOptionSet(cPtr->Options, OPTION_LCD_STRETCH)) {
		if (mode->CrtcHDisplay < 1489)      /* HWBug                 */
		    ChipsNew->XR[0x55] |= 0x02;	/* enable h-centering     */
		else if (pScrn->bitsPerPixel == 24)
		    ChipsNew->XR[0x56] = (lcdHDisplay - CrtcHDisplay) >> 1;
	    } else {
	      ChipsNew->XR[0x55] &= 0xFD;	/* disable h-centering    */
	      ChipsNew->XR[0x56] = 0;
	    }
	    ChipsNew->XR[0x57] = 0x03;    /* enable v-comp disable v-stretch */
	    if (!xf86IsOptionSet(cPtr->Options, OPTION_LCD_STRETCH)) {
		ChipsNew->XR[0x55] |= 0x20; /* enable h-comp disable h-double*/
		ChipsNew->XR[0x57] |= 0x60; /* Enable vertical stretching    */
		tmp = (mode->CrtcVDisplay / (cPtr->PanelSize.VDisplay -
		    mode->CrtcVDisplay + 1));
		if (tmp == 0)
		    if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CURSOR))
			cPtr->Accel.UseHWCursor = TRUE; /* H/W cursor forced */
		    else
		        cPtr->Accel.UseHWCursor = FALSE;
		else
		    cPtr->Accel.UseHWCursor = TRUE;
		if (xf86IsOptionSet(cPtr->Options, OPTION_HW_CURSOR) &&
		    !xf86IsOptionSet(cPtr->Options, OPTION_SW_CURSOR))
		    tmp = (tmp == 0 ? 1 : tmp);  /* Bug when doubling */
		ChipsNew->XR[0x5A] = tmp > 0x0F ? 0 : (unsigned char)tmp;
	    } else {
		ChipsNew->XR[0x55] &= 0xDF; /* disable h-comp, h-double */
		ChipsNew->XR[0x57] &= 0x9F; /* disable vertical stretching  */
	    }
	}
    }

    /* set video mode */
    ChipsNew->XR[0x2B] = chipsVideoMode(pScrn->bitsPerPixel,
	pScrn->weight.green, (cPtr->PanelType & ChipsLCD) ?
	min(HDisplay, cPtr->PanelSize.HDisplay) : HDisplay,
	cPtr->PanelSize.VDisplay);
#ifdef DEBUG
    ErrorF("VESA Mode: %Xh\n", ChipsNew->XR[0x2B]);
#endif

    /* set some linear specific registers */
    if (cPtr->Flags & ChipsLinearSupport) {
	/* enable linear addressing  */
	ChipsNew->XR[0x0B] &= 0xFD;   /* dual page clear                */
	ChipsNew->XR[0x0B] |= 0x10;   /* linear mode on                 */
 	if (cPtr->Chipset == CHIPS_CT65535)
 	    ChipsNew->XR[0x08] = (unsigned char)(cPtr->FbAddress >> 17);
 	else if (cPtr->Chipset > CHIPS_CT65535)
 	    ChipsNew->XR[0x08] = (unsigned char)(cPtr->FbAddress >> 20);
	else {
            /* Its probably set correctly by BIOS anyway. Leave it alone    */
	    /* 65525 - 65530 require XR04[6] set for greater than 512k of   */
            /* ram. We only correct obvious bugs; VL probably uses MEMR/MEMW*/
	    if (cPtr->Bus == ChipsISA)
		ChipsNew->XR[0x04] &= ~0x40;  /* A19 sceme       */
	    if (pScrn->videoRam > 512)
		ChipsNew->XR[0x04] |= 0x40;   /* MEMR/MEMW sceme */
	}

	/* general setup */
	ChipsNew->XR[0x03] |= 0x08;   /* High bandwidth on 65548         */
	ChipsNew->XR[0x40] = 0x01;    /*BitBLT Draw Mode for 8 and 24 bpp */
    }

    /* common general setup */
    ChipsNew->XR[0x52] |= 0x01;       /* Refresh count                   */
    ChipsNew->XR[0x0F] &= 0xEF;       /* not Hi-/True-Colour             */
    ChipsNew->XR[0x02] |= 0x01;       /* 16bit CPU Memory Access         */
    ChipsNew->XR[0x02] &= 0xE3;       /* Attr. Cont. default access      */
                                      /* use ext. regs. for hor. in dual */
    ChipsNew->XR[0x06] &= 0xF3;       /* bpp clear                       */

    /* PCI */
    if (cPtr->Bus == ChipsPCI)
	ChipsNew->XR[0x03] |= 0x40;   /*PCI burst */
    
    /* sync. polarities */
    if ((mode->Flags & (V_PHSYNC | V_NHSYNC))
	&& (mode->Flags & (V_PVSYNC | V_NVSYNC))) {
	if (mode->Flags & (V_PHSYNC | V_NHSYNC)) {
	    if (mode->Flags & V_PHSYNC) {
		ChipsNew->XR[0x55] &= 0xBF;	/* CRT Hsync positive */
	    } else {
		ChipsNew->XR[0x55] |= 0x40;	/* CRT Hsync negative */
	    }
	}
	if (mode->Flags & (V_PVSYNC | V_NVSYNC)) {
	    if (mode->Flags & V_PVSYNC) {
		ChipsNew->XR[0x55] &= 0x7F;	/* CRT Vsync positive */
	    } else {
		ChipsNew->XR[0x55] |= 0x80;	/* CRT Vsync negative */
	    }
	}
    }

    /* bpp depend */
    /*XR06: Palette control */
    /* bit 0: Pixel Data Pin Diag, 0 for flat panel pix. data (def)  */
    /* bit 1: Internal DAC disable                                   */
    /* bit 3-2: Colour depth, 0 for 4 or 8 bpp, 1 for 16(5-5-5) bpp, */
    /*          2 for 24 bpp, 3 for 16(5-6-5)bpp                     */
    /* bit 4:   Enable PC Video Overlay on colour key                */
    /* bit 5:   Bypass Internal VGA palette                          */
    /* bit 7-6: Colour reduction select, 0 for NTSC (default),       */
    /*          1 for Equivalent weighting, 2 for green only,        */
    /*          3 for Colour w/o reduction                           */
    /* XR50 Panel Format Register 1                                  */
    /* bit 1-0: Frame Rate Control; 00, No FRC;                      */
    /*          01, 16-frame FRC for colour STN and monochrome       */
    /*          10, 2-frame FRC for colour TFT or monochrome;        */
    /*          11, reserved                                         */
    /* bit 3-2: Dither Enable                                        */
    /*          00, disable dithering; 01, enable dithering          */
    /*          for 256 mode                                         */
    /*          10, enable dithering for all modes; 11, reserved     */
    /* bit6-4: Clock Divide (CD)                                     */
    /*          000, Shift Clock Freq = Dot Clock Freq;              */
    /*          001, SClk = DClk/2; 010 SClk = DClk/4;               */
    /*          011, SClk = DClk/8; 100 SClk = DClk/16;              */
    /* bit 7: TFT data width                                         */
    /*          0, 16 bit(565RGB); 1, 24bit (888RGB)                 */
    if (pScrn->bitsPerPixel == 16) {
	ChipsNew->XR[0x06] |= 0xC4;   /*15 or 16 bpp colour         */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
	ChipsNew->XR[0x40] = 0x02;    /*BitBLT Draw Mode for 16 bpp */
	if (pScrn->weight.green != 5)
	    ChipsNew->XR[0x06] |= 0x08;	/*16bpp              */
    } else if (pScrn->bitsPerPixel == 24) {
	ChipsNew->XR[0x06] |= 0xC8;   /*24 bpp colour               */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
	if (xf86IsOptionSet(cPtr->Options, OPTION_18_BIT_BUS)) {
	    ChipsNew->XR[0x50] &= 0x7F;   /*18 bit TFT data width   */
	} else {
	    ChipsNew->XR[0x50] |= 0x80;   /*24 bit TFT data width   */
	}
    }

    /*CRT only: interlaced mode */
    if (!(cPtr->PanelType & ChipsLCD)) {
	if (mode->Flags & V_INTERLACE){
	    ChipsNew->XR[0x28] |= 0x20;    /* set interlace         */
	    /* empirical value       */
	    tmp = ((((mode->CrtcHDisplay >> 3) - 1) >> 1) 
		- 6 * (pScrn->bitsPerPixel >= 8 ? bytesPerPixel : 1 ));
	    if(cPtr->Chipset < CHIPS_CT65535)
		ChipsNew->XR[0x19] = tmp & 0xFF;
	    else
		ChipsNew->XR[0x29] = tmp & 0xFF;
 	    ChipsNew->XR[0x0F] &= ~0x40;   /* set SW-Flag           */
	} else {
	    ChipsNew->XR[0x28] &= ~0x20;   /* unset interlace       */
 	    ChipsNew->XR[0x0F] |=  0x40;   /* set SW-Flag           */
	}
    }

    /* STN specific */
    if (IS_STN(cPtr->PanelType)) {
	ChipsNew->XR[0x50] &= ~0x03;  /* FRC clear                  */
	ChipsNew->XR[0x50] |= 0x01;   /* 16 frame FRC               */
	ChipsNew->XR[0x50] &= ~0x0C;  /* Dither clear               */
	ChipsNew->XR[0x50] |= 0x08;   /* Dither all modes           */
 	if (cPtr->Chipset == CHIPS_CT65548) {
	    ChipsNew->XR[0x03] |= 0x20; /* CRT I/F priority           */
	    ChipsNew->XR[0x04] |= 0x10; /* RAS precharge 65548        */
	}
    }

    /* This stuff was emprically derived several years ago. Not sure its 
     * still needed, and I'd love to get rid of it as its ugly
     */
    switch (cPtr->Chipset) {
    case CHIPS_CT65545:		  /*jet mini *//*DEC HighNote Ultra DSTN */
	ChipsNew->XR[0x03] |= 0x10;   /* do not hold off CPU for palette acc*/
	break;
    case CHIPS_CT65546:			       /*CT 65546, only for Toshiba */
	ChipsNew->XR[0x05] |= 0x80;   /* EDO RAM enable */
	break;
    }

    /* Program the registers */
    /*vgaHWProtect(pScrn, TRUE);*/
    chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
    /*vgaHWProtect(pScrn, FALSE);*/

    return (TRUE);
}

static void 
chipsRestore(ScrnInfoPtr pScrn, vgaRegPtr VgaReg, CHIPSRegPtr ChipsReg,
	     Bool restoreFonts)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp = 0;

    /*vgaHWProtect(pScrn, TRUE);*/

    /* set registers so that we can program the controller */
    if (IS_HiQV(cPtr)) {
	outw(0x3D6, 0x0E);
    } else {
	outw(0x3D6, 0x10);
	outw(0x3D6, 0x11);
	outb(0x3D6, 0x0C);      /* WINgine stores MSB here */
	tmp = inb(0x3D7) & ~0x50;
	outb(0x3D7, tmp);
	outw(0x3D6, 0x15);	       /* unprotect all registers */
	outb(0x3D6, 0x14);
	tmp = inb(0x3D7);
	outb(0x3D7, (tmp & ~0x20)); /* enable vsync on ST01 */
    }

    chipsFixResume(cPtr);

    /* wait for vsync if sequencer is running - stop sequencer */
    if (cPtr->SyncResetIgn) {        /* only do if sync reset is ignored */
	while (((inb(cPtr->IOBase + 0xA)) & 0x08) == 0x08 );/* VSync off */
	while (((inb(cPtr->IOBase + 0xA)) & 0x08) == 0 );   /* VSync on  */
	outw(0x3C4,0x07);          /* reset hsync - just in case...  */
    }

    /* set the clock */
    chipsClockLoad(pScrn, &ChipsReg->Clock);

    /* set extended regs */
    chipsRestoreExtendedRegs(cPtr, ChipsReg);
#if 0
    /* if people complain about lock ups or blank screens -- reenable */
    /* set CRTC registers - do it before sequencer restarts */
    for (i=0; i<25; i++) 
	outw(cPtr->IOBase + 4, (VgaReg->CRTC[i] << 8) | i);
#endif
    /* set generic registers */
    vgaHWRestore(pScrn, VgaReg,
		 VGA_SR_MODE | VGA_SR_CMAP | (restoreFonts ? VGA_SR_FONTS : 0));

    /* set stretching registers */
    if (IS_HiQV(cPtr)) {
	chipsRestoreStretching(pScrn, (unsigned char)ChipsReg->FR[0x40],
			       (unsigned char)ChipsReg->FR[0x48]);
#if 0 
	/* if people report about stretching not working -- reenable */
	/* why twice ? :
	 * sometimes the console is not well restored even if these registers 
	 * are good, re-write the registers works around it
	 */
	chipsRestoreStretching(pScrn, (unsigned char)ChipsReg->FR[0x40],
			       (unsigned char)ChipsReg->FR[0x48]);
#endif
    } else if (!IS_Wingine(cPtr))
	chipsRestoreStretching(pScrn, (unsigned char)ChipsReg->XR[0x55],
			       (unsigned char)ChipsReg->XR[0x57]);


    /* perform a synchronous reset */
    if (!cPtr->SyncResetIgn) {
	if (!IS_HiQV(cPtr)) {
	    outb(0x3D6,0x0E);
	    tmp = inb(0x3D7);
	    outb(0x3D7,tmp & 0x7F);    /* enable syncronous reset on 655xx */
	}
	outw(0x3C4,0x0100);
	usleep(10000);
	outw(0x3C4,0x0300);
	if (!IS_HiQV(cPtr))
	    write_xr(0x0E,tmp);
    }
    /* Flag valid start address, if using CRT extensions */
    if (IS_HiQV(cPtr) && (ChipsReg->XR[0x09] & 0x1) == 0x1) {
	outb(cPtr->IOBase + 4, 0x40);
	tmp = inb(cPtr->IOBase + 5);
	outb(cPtr->IOBase + 5, tmp | 0x80);
    }

    /* Do it again here, as Nozomi seems to need it          */
     chipsFixResume(cPtr);
    /*vgaHWProtect(pScrn, FALSE);*/
}

static void
chipsRestoreExtendedRegs(CHIPSPtr cPtr, CHIPSRegPtr Regs)
{
    int i;
    unsigned char tmp;

    if (IS_HiQV(cPtr)) {
	/* set extended regs */
	for (i = 0; i < 0x43; i++) {
	    outb(0x3D6, i);
	    if (inb(0x3D7) != Regs->XR[i])
		outb(0x3D7, Regs->XR[i]);
	}
	/* Don't touch reserved memory control registers */
	for (i = 0x50; i < 0xBF; i++) {
	    outb(0x3D6, i);
	    if (inb(0x3D7) != Regs->XR[i])
		outb(0x3D7, Regs->XR[i]);
	}
	/* Don't touch VCLK regs, but fix up MClk */
	
	/* set mem clock */
	outb(0x3D6, 0xCE);	       /* Select Fixed MClk before */
	tmp = inb(0x3D7);
	outb(0x3D7, tmp & 0x7F);
	outb(0x3D6, 0xCC);
	if (inb(0x3D7) != Regs->XR[0xCC])
	    outb(0x3D7, Regs->XR[0xCC]);
	outb(0x3D6, 0xCD);
	if (inb(0x3D7) != Regs->XR[0xCD])
	    outb(0x3D7, Regs->XR[0xCD]);
	outb(0x3D6, 0xCE);
	if (inb(0x3D7) != Regs->XR[0xCE])
	    outb(0x3D7, Regs->XR[0xCE]);

	/* set flat panel regs. */
	for (i = 0xD0; i < 0xFF; i++) {
	    outb(0x3D6, i);
	    if (inb(0x3D7) != Regs->XR[i])
		outb(0x3D7, Regs->XR[i]);
	}
	for (i = 0; i < 0x2; i++) {
	    outb(0x3D0, i);
	    if (inb(0x3D1) != Regs->FR[i])
		outb(0x3D1, Regs->FR[i]);
	}
	outb(0x3D0, 0x03);
	tmp = inb(0x3D1);	       /* restore the non clock bits */
	outb(0x3D0, 0x03);
	outb(0x3D1, ((Regs->FR[0x03] & 0xC3) | (tmp & ~0xC3)));
	i++;
	/* Don't touch alternate clock select reg. */
	for (; i < 0x80; i++) {
	    if ( (i == 0x40) || (i==0x48)) {
	      outb(0x3D0, i);  /* !! */      /* set stretching but disable  */
	      outb(0x3D1, Regs->FR[i] & 0xFE);            /* compensation   */
	      continue ;     /* some registers must be set before FR40/FR48 */
	    }
	    outb(0x3D0, i);
	    if (inb(0x3D1) != Regs->FR[i]) /* only modify if changed */
		outb(0x3D1, Regs->FR[i]);
	}

	/* set extended crtc regs. */
	for (i = 0x30; i < 0x80; i++) {
	    outb(cPtr->IOBase + 4, i);
	    if (inb(cPtr->IOBase + 5) != Regs->CR[i]) /* modify if changed */
		outb(cPtr->IOBase + 5, Regs->CR[i]);
	}
    } else {
	/* set extended regs. */
	for (i = 0; i < 0x30; i++) {
	    outb(0x3D6, i);
	    if (inb(0x3D7) != Regs->XR[i])
		outb(0x3D7, Regs->XR[i]);
	}
	outw(0x3D6,0x15); /* unprotect just in case ... */
	/* Don't touch MCLK/VCLK regs. */
	for (i = 0x34; i < 0x54; i++) {
	    outb(0x3D6, i);
	    if (inb(0x3D7) != Regs->XR[i])/* only modify if changed */
		outb(0x3D7, Regs->XR[i]);
	}
	outb(0x3D6, 0x54);
	tmp = inb(0x3D7);	/*  restore the non clock bits             */
	outb(0x3D6, 0x54); 	/* Don't touch alternate clock select reg. */
	outb(0x3D7, ((Regs->XR[0x54] & 0xF3) | (tmp & ~0xF3)));
	outb(0x3D6, 0x55);      /* output with h-comp off                  */
	outb(0x3D7, (Regs->XR[0x55] & 0xFE));
	outb(0x3D6, 0x56);
	outb(0x3D7, Regs->XR[0x56]);
	outb(0x3D6, 0x57);      /* output with v-comp off                  */
	outb(0x3D7, (Regs->XR[0x57] & 0xFE));
	for (i=0x58; i < 0x7D; i++) {/* don't touch XR7D and XR7F on WINGINE */
	    outb(0x3D6, i);
	    if (inb(0x3D7) != Regs->XR[i]) /* only modify if changed */
		outb(0x3D7, Regs->XR[i]);
	}
    }
#ifdef DEBUG
    /* debug - dump out all the extended registers... */
    if (IS_HiQV(cPtr)) {
	for (i = 0; i < 0xFF; i++) {
	    outb(0x3D6, i);
	    ErrorF("XR%X - %X : %X\n", i, ChipsReg->XR[i], inb(0x3D7));
	}
	for (i = 0; i < 0x80; i++) {
	    outb(0x3D0, i);
	    ErrorF("FR%X - %X : %X\n", i, ChipsReg->FR[i], inb(0x3D1));
	}
    } else {
	for (i = 0; i < 0x80; i++) {
	    outb(0x3D6, i);
	    ErrorF("XR%X - %X : %X\n", i, ChipsReg->XR[i], inb(0x3D7));
	}
    }
#endif
}

static void
chipsRestoreStretching(ScrnInfoPtr pScrn, unsigned char ctHorizontalStretch,
		       unsigned char ctVerticalStretch)
{
    unsigned char tmp;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    
    /* write to regs. */
    if (IS_HiQV(cPtr)) {
	read_fr(0x48, tmp);
	write_fr(0x48, (tmp & 0xFE) | (ctVerticalStretch & 0x01));
	read_fr(0x40, tmp);
	write_fr(0x40, (tmp & 0xFE) | (ctHorizontalStretch & 0x01));
    } else {
	read_xr(0x55, tmp);
	write_xr(0x55, (tmp & 0xFE) | (ctHorizontalStretch & 0x01));
	read_xr(0x57, tmp);
	write_xr(0x57, (tmp & 0xFE) | (ctVerticalStretch & 0x01));
    }

    usleep(20000);			/* to be active */
}

static int
chipsVideoMode(int vgaBitsPerPixel, int weightGreen, int displayHSize,
	       int displayVSize)
{
    /*     4 bpp  8 bpp  16 bpp  18 bpp  24 bpp  32 bpp */
    /* 640  0x20   0x30    0x40    -      0x50     -    */
    /* 800  0x22   0x32    0x42    -      0x52     -    */
    /*1024  0x24   0x34    0x44    -      0x54     -    for 1024x768 */
    /*1024   -     0x36    0x47    -      0x56     -    for 1024x600 */
    /*1152  0x27   0x37    0x47    -      0x57     -    */
    /*1280  0x28   0x38    0x49    -        -      -    */
    /*1600  0x2C   0x3C    0x4C   0x5D      -      -    */
    /*This value is only for BIOS.... */

    int videoMode = 0;

    switch (vgaBitsPerPixel) {
    case 1:
    case 4:
	videoMode = 0x20;
	break;
    case 8:
	videoMode = 0x30;
	break;
    case 16:
	videoMode = 0x40;
	if (weightGreen != 5)
	    videoMode |= 0x01;
	break;
    default:
	videoMode = 0x50;
	break;
    }

    switch (displayHSize) {
    case 800:
	videoMode |= 0x02;
	break;
    case 1024:
	videoMode |= 0x04;
	if(displayVSize < 768)
	    videoMode |= 0x02;
	break;
    case 1152:
	videoMode |= 0x07;
	break;
    case 1280:
	videoMode |= 0x08;
	if (vgaBitsPerPixel == 16)
	    videoMode |= 0x01;
	break;
    case 1600:
	videoMode |= 0x0C;
	if (vgaBitsPerPixel == 16)
	    videoMode |= 0x01;
	break;
    }

    return videoMode;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
chipsMapMem(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    if (cPtr->Flags & ChipsLinearSupport) {
	if (cPtr->UseMMIO) {
	    if (IS_HiQV(cPtr)) {
		if (cPtr->Bus == ChipsPCI)
		    cPtr->MMIOBase = xf86MapPciMem(pScrn->scrnIndex,
			  VIDMEM_MMIO,cPtr->PciTag, (pointer)cPtr->IOAddress,
			  0x20000L);
		else
		    cPtr->MMIOBase = xf86MapVidMem(pScrn->scrnIndex,
			  VIDMEM_MMIO, (pointer)cPtr->IOAddress, 0x20000L);
	    } else {
		if (cPtr->Bus == ChipsPCI)
		    cPtr->MMIOBase = xf86MapPciMem(pScrn->scrnIndex,
			  VIDMEM_MMIO, cPtr->PciTag, (pointer)cPtr->IOAddress,
			  0x10000L);
		else
		    cPtr->MMIOBase = xf86MapVidMem(pScrn->scrnIndex,
			  VIDMEM_MMIO, (pointer)cPtr->IOAddress, 0x10000L);
	    }

	    if (cPtr->MMIOBase == NULL)
		return FALSE;
	}
    
	if (cPtr->Bus == ChipsPCI)
	    cPtr->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
		  cPtr->PciTag, (pointer)((unsigned long)cPtr->FbAddress),
		  cPtr->FbMapSize);
	else
	    cPtr->FbBase = xf86MapVidMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
		  (pointer)((unsigned long)cPtr->FbAddress), cPtr->FbMapSize);

	if (cPtr->FbBase == NULL)
	  return FALSE;
    } else {
	/* In paged mode Base is the VGA window at 0xA0000 */
	cPtr->FbBase = hwp->Base;
    }
    
    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
chipsUnmapMem(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)cPtr->MMIOBase, 0x4000);
    cPtr->MMIOBase = NULL;
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)cPtr->FbBase, pScrn->videoRam);
    cPtr->FbBase = NULL;
    
    return TRUE;
}

static void
chipsProtect(ScrnInfoPtr pScrn, Bool on)
{
    vgaHWProtect(pScrn, on);
}

static void
chipsBlankScreen(ScrnInfoPtr pScrn, Bool unblank)
{
    CHIPSPtr cPtr;
    unsigned char scrn;
    cPtr = CHIPSPTR(pScrn);
    
    /* fix things that could be messed up by suspend/resume */
    if (!IS_HiQV(cPtr))
	outw(0x3D6,0x15);

    outb(0x3C4,1);
    scrn = inb(0x3C5);

    if (unblank) {
	scrn &= 0xDF;                       /* enable screen */
    } else {
	scrn |= 0x20;                       /* blank screen */
    }

    /* synchronous reset - stop counters */
    if (!cPtr->SyncResetIgn) {
	outw(0x3C4, 0x0100);        
    }

    outw(0x3C4, (scrn << 8) | 0x01); /* change mode */

    /* end reset - start counters */
    if (!cPtr->SyncResetIgn) {
	outw(0x3C4, 0x0300); 
    }
}

static void
chipsLock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp;
    
    vgaHWLock(hwp);

    if (!IS_HiQV(cPtr)) {
	/* group protection attribute controller access */
	outb(0x3D6, 0x15);
	outb(0x3D7, cPtr->SuspendHack.xr15);
	outb(0x3D6, 0x02);
	tmp = inb(0x3D7);
	outb(0x3D7, (tmp & ~0x18) | cPtr->SuspendHack.xr02);
	outb(0x3D6, 0x14);
	tmp = inb(0x3D7);
	outb(0x3D7, (tmp & ~0x20) | cPtr->SuspendHack.xr14);

	/* reset 32 bit register access */
	if (cPtr->Chipset > CHIPS_CT65540) {
	    outb(0x3D6, 0x03);
	    tmp = inb(0x3D7);
	    outb(0x3D7, (tmp & ~0x0A) | cPtr->SuspendHack.xr03);
	}
    }
}

static void
chipsUnlock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp;
    
    if (!IS_HiQV(cPtr)) {
	/* group protection attribute controller access */
	outb(0x3D6, 0x15);
	outb(0x3D7, 0x00);
	outb(0x3D6, 0x02);
	tmp = inb(0x3D7);
	outb(0x3D7, (tmp & ~0x18));
	outb(0x3D6, 0x14);
	tmp = inb(0x3D7);
	outb(0x3D7, (tmp & ~0x20));

	/* enable 32 bit register access */
	if (cPtr->Chipset > CHIPS_CT65540) {
	    outb(0x3D6, 0x03);
	    outb(0x3D7, (cPtr->SuspendHack.xr02 | 0x0A));
	}
    }
    vgaHWUnlock(hwp);
}

static void
chipsHWCursorOn(CHIPSPtr cPtr)
{
    /* enable HW cursor */
    if (cPtr->HWCursorShown) {
	if (IS_HiQV(cPtr)) {
	    outb(0x3D6, 0xA0);
	    outb(0x3D7, cPtr->HWCursorContents & 0xFF);
	} else {
	    HW_DEBUG(0x8);	
	    if (cPtr->UseMMIO) {
		MMIOmeml(DR(0x8)) = cPtr->HWCursorContents;
	    } else {
		outl(DR(0x8), cPtr->HWCursorContents);
	    }
	}
    }
}

static void
chipsHWCursorOff(CHIPSPtr cPtr)
{
    /* disable HW cursor */
    if (cPtr->HWCursorShown) {
	if (IS_HiQV(cPtr)) {
	    outb(0x3D6, 0xA0);
	    cPtr->HWCursorContents = inb(0x3D7);
	    outb(0x3D7, cPtr->HWCursorContents & 0xF8);
	} else {
	    HW_DEBUG(0x8);
	    if (cPtr->UseMMIO) {
		cPtr->HWCursorContents = MMIOmeml(DR(0x8));
		MMIOmemw(DR(0x8)) = cPtr->HWCursorContents & 0xFFFE;
	    } else {
		cPtr->HWCursorContents = inl(DR(0x8));
		outw(DR(0x8), cPtr->HWCursorContents & 0xFFFE);
	    }
	}
    }
}

void
chipsFixResume(CHIPSPtr cPtr)
{
    unsigned char tmp;
    /* fix things that could be messed up by suspend/resume */
    if (!IS_HiQV(cPtr))
	outw(0x3D6,0x15);
    tmp = inb(0x3CC);
    outb(0x3C2, (tmp & 0xFE) | cPtr->SuspendHack.vgaIOBaseFlag); 
    outb(cPtr->IOBase + 4, 0x11);
    tmp = inb(cPtr->IOBase + 5);
    outb(cPtr->IOBase + 5, (tmp & 0x7F));
}

static char
chipsTestDACComp(ScrnInfoPtr pScrn, unsigned char a, unsigned char b,
		 unsigned char c)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char type;

    outb(0x3C8,00);
    while (inb(cPtr->IOBase + 0xA) & 0x08){};   /* wait for vsync to end */
    while (!(inb(cPtr->IOBase + 0xA) & 0x08)){};  /* wait for new vsync  */
    outb(0x3C9,a);              /* set pattern */
    outb(0x3C9,b);
    outb(0x3C9,c);

    while (!(inb(cPtr->IOBase +0xA) & 0x01)){};  /* wait for hsync to end  */
    while (inb(cPtr->IOBase +0xA) & 0x01){};  /* wait for hsync to end  */
    type = inb(0x3C2);          /* read comparator        */

    return (type & 0x10);
}

static int
chipsProbeMonitor(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char dacmask;
    unsigned char dacdata[3];
    unsigned char xr1, xr2;
    int type = 2;  /* no monitor */

    dacmask = inb(0x3C6);    /* save registers */ 
    outb(0x3C6,00);
    outb(0x3C7,00);
    dacdata[0]=inb(0x3C9);
    dacdata[1]=inb(0x3C9);
    dacdata[2]=inb(0x3C9);
    if (!IS_HiQV(cPtr)) {
	read_xr(0x06, xr1);
	read_xr(0x1F, xr2);
	write_xr(0x06, xr1 & 0xF1);  /* turn on dac */
	write_xr(0x1F, xr2 & 0x7F);  /* enable comp */
    } else {
	read_xr(0x81,xr1);
	read_xr(0xD0,xr2);
	write_xr(0x81,(xr1 & 0xF0));
	write_xr(0xD0,(xr2 | 0x03));
    }
    if (chipsTestDACComp(pScrn, 0x12,0x12,0x12)) {         /* test patterns */
	if (chipsTestDACComp(pScrn,0x14,0x14,0x14))        /* taken  from   */
	    if (!chipsTestDACComp(pScrn,0x2D,0x14,0x14))   /* BIOS          */
		if (!chipsTestDACComp(pScrn,0x14,0x2D,0x14))
		    if (!chipsTestDACComp(pScrn,0x14,0x14,0x2D))
			if (!chipsTestDACComp(pScrn,0x2D,0x2D,0x2D))
			    type = 0;    /* color monitor */
    } else {     
	if (chipsTestDACComp(pScrn,0x04,0x12,0x04))
	    if (!chipsTestDACComp(pScrn,0x1E,0x12,0x04))
		if (!chipsTestDACComp(pScrn,0x04,0x2D,0x04))
		    if (!chipsTestDACComp(pScrn,0x1E,0x16,0x15))
			if (chipsTestDACComp(pScrn,0x00,0x00,0x00))
			    type = 1;    /* monochrome */
    }

    outb(0x3C8,00);         /* restore registers */
    outb(0x3C9,dacdata[0]);
    outb(0x3C9,dacdata[1]);
    outb(0x3C9,dacdata[2]);
    outb(0x3C6,dacmask);
    if (!IS_HiQV(cPtr)) {
	write_xr(0x06,xr1);
	write_xr(0x1F,xr2);
    } else {
	write_xr(0x81,xr1);
	write_xr(0xD0,xr2);
    }
    return type;
}

static int
chipsSetMonitor(ScrnInfoPtr pScrn)
{
    int tmp= chipsProbeMonitor(pScrn);

    switch (tmp) {
    case 0:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Color monitor detected\n");
	break;
    case 1:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Monochrome monitor detected\n");
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "No monitor detected\n");
    }
    return (tmp);
}

