/* $XConsortium: s3.c,v 1.1 94/03/28 21:13:36 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3.c,v 3.0 1994/04/29 14:07:44 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Thomas Roell makes no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * THOMAS ROELL AND KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THOMAS ROELL OR KEVIN E. MARTIN BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 * 
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * 
 * Header: /home/src/xfree86/mit/server/ddx/xf86/accel/s3/RCS/s3.c,v 2.0
 * 1993/02/22 05:58:13 jon Exp
 * 
 * Modified by Amancio Hasty and Jon Tombs
 * 
 * Id: s3.c,v 2.6 1993/08/09 06:17:57 jon Exp jon
 */


#include "misc.h"
#include "cfb.h"
#include "pixmapstr.h"
#include "fontstruct.h"
#include "s3.h"
#include "regs3.h"
#include "s3pcach.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "s3linear.h"
#include "s3Bt485.h"
#include "s3Ti3020.h"

extern int s3MaxClock;
extern int s3MaxBt485Clock, s3MaxBt485MuxClock;
extern int s3MaxTi3020Clock, s3MaxTi3020ClockFast;
char s3Mbanks;

extern s3VideoChipPtr s3Drivers[];

int vgaInterlaceType = VGA_DIVIDE_VERT;
void (*vgaSaveScreenFunc)() = (void (*)())NoopDDA;

ScrnInfoRec s3InfoRec =
{
   FALSE,			/* Bool configured */
   -1,				/* int tmpIndex */
   -1,				/* int scrnIndex */
   s3Probe,			/* Bool (* Probe)() */
   (Bool (*)())NoopDDA,		/* Bool (* Init)() */
   (void (*)())NoopDDA,		/* void (* EnterLeaveVT)() */
   (void (*)())NoopDDA,		/* void (* EnterLeaveMonitor)() */
   (void (*)())NoopDDA,		/* void (* EnterLeaveCursor)() */
   (void (*)())NoopDDA,		/* void (* AdjustFrame)() */
   (Bool (*)())NoopDDA,		/* Bool (* SwitchMode)() */
   s3PrintIdent,		/* void (* PrintIdent)() */
   8,				/* int depth */
   8,				/* int bitsPerPixel */
   PseudoColor,			/* int defaultVisual */
   -1, -1,			/* int virtualX,virtualY */
   -1, -1, -1, -1,		/* int frameX0, frameY0, frameX1, frameY1 */
   {0,},			/* OFlagSet options */
   {0,},			/* OFlagSet clockOptions */   
   {0, },              		/* OFlagSet xconfigFlag */
   NULL,			/* char *chipset */
   0,				/* int clocks */
   {0,},			/* int clock[MAXCLOCKS] */
   0,				/* int maxClock */
   0,				/* int videoRam */
   0xC0000,                     /* int BIOSbase */  
   0,				/* unsigned long MemBase */
   240, 180,			/* int width, height */
   0,				/* unsigned long  speedup */
   NULL,			/* DisplayModePtr modes */   
   NULL,			/* char           *clockprog */
   -1,			        /* int textclock */
   FALSE,			/* Bool           bankedMono */
   "S3",			/* char           *name */
   {0, },			/* RgbRec blackColour */
   {0, },			/* RgbRec whiteColour */
   s3ValidTokens,		/* int *validTokens */
   S3_PATCHLEVEL,		/* char *patchlevel */
};

short s3alu[16] =
{
   MIX_0,
   MIX_AND,
   MIX_SRC_AND_NOT_DST,
   MIX_SRC,
   MIX_NOT_SRC_AND_DST,
   MIX_DST,
   MIX_XOR,
   MIX_OR,
   MIX_NOR,
   MIX_XNOR,
   MIX_NOT_DST,
   MIX_SRC_OR_NOT_DST,
   MIX_NOT_SRC,
   MIX_NOT_SRC_OR_DST,
   MIX_NAND,
   MIX_1
};

static unsigned S3_IOPorts[] = {
	0x42E8, 0x4AE8, 0x82E8, 0x86E8, 0x8AE8, 0x8EE8, 0x92E8, 0x96E8,
	0x9AE8, 0x9EE8, 0xA2E8, 0xA6E8, 0xAAE8, 0xAEE8, 0xB2E8, 0xB6E8,
	0xBAE8, 0xBEE8, 0xE2E8, 0xE2EA,
};
static int Num_S3_IOPorts = (sizeof(S3_IOPorts)/sizeof(S3_IOPorts[0]));

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
Bool  (*s3ClockSelectFunc) ();
static Bool LegendClockSelect();
static Bool s3ClockSelect();
static Bool icd2061ClockSelect();
ScreenPtr s3savepScreen;
Bool  s3Localbus = FALSE;
Bool  s3LinearAperture = FALSE;
Bool  s3Mmio928 = FALSE;
Bool  s3DAC8Bit = FALSE;
Bool  s3DACSyncOnGreen = FALSE;
unsigned char s3LinApOpt;
int s3BankSize;
int s3DisplayWidth;
pointer vgaBase = NULL;
pointer s3VideoMem = NULL;

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed, xf86Verbose;

int s3ScissB;
unsigned char s3SwapBits[256];
unsigned char s3Port40;
unsigned char s3Port51;
unsigned char s3Port54;
unsigned char s3Port59 = 0x00;
unsigned char s3Port5A = 0x0A;
unsigned char s3Port31 = 0x8d;
void (*s3ImageReadFunc)();
void (*s3ImageWriteFunc)();
void (*s3ImageFillFunc)();
int s3MAX_SLOTS;
int s3FC_MAX_HEIGHT = 32;
CacheFont8Ptr s3HeadFont, s3FontCache;
int s3hotX, s3hotY;
Bool s3BlockCursor, s3ReloadCursor;
int s3CursorStartX, s3CursorStartY, s3CursorLines;
int s3RamdacType = UNKNOWN_DAC;
Bool s3UsingPixMux = FALSE;
Bool s3Bt485PixMux = FALSE;
Bool s3ClockDouble = FALSE;


/*
 * s3PrintIdent -- print identification message
 */
void
s3PrintIdent()
{
  int i, j, n = 0, c = 0;
  char *id;

  ErrorF("  %s: accelerated server for S3 graphics adaptors (Patchlevel %s)\n",
	 s3InfoRec.name, s3InfoRec.patchLevel);

  ErrorF("      ");
  for (i = 0; s3Drivers[i]; i++)
    for (j = 0; id = (s3Drivers[i]->ChipIdent)(j); j++, n++)
    {
      if (n)
      {
        ErrorF(",");
        c++;
        if (c + 1 + strlen(id) < 70)
        {
          ErrorF(" ");
          c++;
        }
        else
        {
          ErrorF("\n      ");
          c = 0;
        }
      }
      ErrorF("%s", id);
      c += strlen(id);
    }
  ErrorF("\n");
}

/*
 * s3Probe -- probe and initialize the hardware driver
 */
Bool
s3Probe()
{
   DisplayModePtr pMode, pEnd;
   unsigned char config, tmp, tmp1;
   int i, j, numClocks;
   int tx, ty;
   int maxDisplayWidth, maxDisplayHeight;
   OFlagSet validOptions;

   /*
    * These characterise a RAMDACs pixel multiplexing capabilities and
    * requirements:
    *
    *   pixMuxPossible         - pixmux is supported for the current RAMDAC
    *   allowPixMuxInterlace   - pixmux supports interlaced modes
    *   allowPixMuxSwitching   - possible to use pixmux for some modes
    *                            and non-pixmux for others
    *   pixMuxMinWidth         - smallest physical width supported in
    *                            pixmux mode
    *   nonMuxMaxClock         - highest dot clock supported without pixmux
    *   nonMuxMaxMemory        - max video memory accessible without pixmux
    *   pixMuxLimitedWidths    - pixmux only works for logical display
    *                            widths 1024 and 2048
    *   pixMuxInterlaceOK      - FALSE if pixmux isn't possible because
    *                            there is an interlaced mode present
    *   pixMuxWidthOK          - FALSE if pixmux isn't possible because
    *                            there is mode has too small a width
    */
   Bool pixMuxPossible = FALSE;
   Bool allowPixMuxInterlace = FALSE;
   Bool allowPixMuxSwitching = FALSE;
   Bool pixMuxNeeded = FALSE;
   int pixMuxMinWidth = 1024;
   int nonMuxMaxClock = 0;
   int nonMuxMaxMemory = 4096;
   Bool pixMuxLimitedWidths = TRUE;
   Bool pixMuxInterlaceOK = TRUE;
   Bool pixMuxWidthOK = TRUE;

   xf86ClearIOPortList(s3InfoRec.scrnIndex);
   xf86AddIOPorts(s3InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
   xf86AddIOPorts(s3InfoRec.scrnIndex, Num_S3_IOPorts, S3_IOPorts);

   /* Enable I/O access */
   xf86EnableIOPorts(s3InfoRec.scrnIndex);

   vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
   vgaCRIndex = vgaIOBase + 4;
   vgaCRReg = vgaIOBase + 5;

   outb(vgaCRIndex, 0x11);	/* for register CR11, (Vertical Retrace End) */
   outb(vgaCRReg, 0x00);		/* set to 0 */

   outb(vgaCRIndex, 0x38);		/* check if we have an S3 */
   outb(vgaCRReg, 0x00);

   /* Make sure we can't write when locked */

   if (testinx2(vgaCRIndex, 0x35, 0x0f)) {
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
   }
 
   outb(vgaCRIndex, 0x38);	/* for register CR38, (REG_LOCK1) */
   outb(vgaCRReg, 0x48);	/* unlock S3 register set for read/write */

   /* Make sure we can write when unlocked */

   if (!testinx2(vgaCRIndex, 0x35, 0x0f)) {
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
   }

   outb(vgaCRIndex, 0x36);		/* for register CR36 (CONFG_REG1), */
   config = inb(vgaCRReg);		/* get amount of vram installed */

   outb(vgaCRIndex, 0x30);
   s3ChipId = inb(vgaCRReg);         /* get chip id */

   if (!S3_ANY_SERIES(s3ChipId)) {
      ErrorF("%s %s: Unknown S3 chipset: chip_id = 0x%02x\n", 
	     XCONFIG_PROBED,s3InfoRec.name,s3ChipId);
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
   }

   for (i = 0; s3Drivers[i]; i++) {
      if ((s3Drivers[i]->ChipProbe)()) {
	 xf86ProbeFailed = FALSE;
	 s3InfoRec.Init = s3Drivers[i]->ChipInitialize;
	 s3InfoRec.EnterLeaveVT = s3Drivers[i]->ChipEnterLeaveVT;
	 s3InfoRec.AdjustFrame = s3Drivers[i]->ChipAdjustFrame;
	 s3InfoRec.SwitchMode = s3Drivers[i]->ChipSwitchMode;
	 break;
      }
   }
   if (xf86ProbeFailed) {
      if (s3InfoRec.chipset) {
	 ErrorF("%s: '%s' is an invalid chipset", s3InfoRec.name,
		s3InfoRec.chipset);
      }
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
   }

   OFLG_ZERO(&validOptions);
   OFLG_SET(OPTION_LEGEND, &validOptions);
   OFLG_SET(OPTION_NOLINEAR_MODE, &validOptions);
   OFLG_SET(OPTION_NO_MEM_ACCESS, &validOptions);
   OFLG_SET(OPTION_MEM_ACCESS, &validOptions);
   OFLG_SET(OPTION_NORMAL_DAC, &validOptions);
   OFLG_SET(OPTION_BT485, &validOptions);
   OFLG_SET(OPTION_BT485_CURS, &validOptions);
   OFLG_SET(OPTION_SHOWCACHE, &validOptions);
   OFLG_SET(OPTION_FB_DEBUG, &validOptions);
   OFLG_SET(OPTION_TI3020, &validOptions);
   OFLG_SET(OPTION_TI3020_CURS, &validOptions);
   OFLG_SET(OPTION_NO_TI3020_CURS, &validOptions);
   OFLG_SET(OPTION_TI3020_FAST, &validOptions);
   OFLG_SET(OPTION_DAC_8_BIT, &validOptions);
   OFLG_SET(OPTION_20C505, &validOptions);
   OFLG_SET(OPTION_FAST_DRAM, &validOptions);
   OFLG_SET(OPTION_MED_DRAM, &validOptions);
   OFLG_SET(OPTION_SLOW_DRAM, &validOptions);
   OFLG_SET(OPTION_ATT490_1, &validOptions);
   OFLG_SET(OPTION_SC15025, &validOptions);
   OFLG_SET(OPTION_SYNC_ON_GREEN, &validOptions);
   OFLG_SET(OPTION_SPEA_MERCURY, &validOptions);
   OFLG_SET(OPTION_NUMBER_NINE, &validOptions);
   OFLG_SET(OPTION_STB_PEGASUS, &validOptions);
   xf86VerifyOptions(&validOptions, &s3InfoRec);
   if (OFLG_ISSET(OPTION_MEM_ACCESS, &s3InfoRec.options)) {
      ErrorF("%s: Warning: the \"memaccess\" option is now redundant\n",
	     s3InfoRec.name);
      ErrorF("\tIt will be removed in the next release\n");
   }

   s3Localbus = ((config & 0x03) <= 1);		/* LocalBus or EISA or PCI */

   if (xf86Verbose) {
      switch (config & 0x03) {
      case 0:
	 if (S3_928_P(config)) 
	    ErrorF("%s %s: card type: PCI\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 else
	    ErrorF("%s %s: card type: EISA\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 break;
      case 1:
         ErrorF("%s %s: card type: 386/486 localbus\n",
        	XCONFIG_PROBED, s3InfoRec.name);
	 break;
      case 3:
         ErrorF("%s %s: card type: ISA\n",
		XCONFIG_PROBED, s3InfoRec.name);
	 break;
      default:
	 ErrorF("%s %s: card type: unknown (%d)\n",
		XCONFIG_PROBED, s3InfoRec.name, config & 0x03);
      }
   }

   if (xf86Verbose) {
      if (S3_x64_SERIES(s3ChipId)) {
	if (S3_864_SERIES(s3ChipId)) {
	    ErrorF("%s %s: chipset:   864 rev. %d\n",
                   XCONFIG_PROBED, s3InfoRec.name, s3ChipId & 0x0f);
	 } else if (S3_964_SERIES(s3ChipId)) {
	    ErrorF("%s %s: chipset:   964 rev. %d\n",
                   XCONFIG_PROBED, s3InfoRec.name, s3ChipId & 0x0f);
	 }
      } else if (S3_801_928_SERIES(s3ChipId)) {
	 if (S3_801_SERIES(s3ChipId)) {
	    if (!((config & 0x03) == 3))
	       ErrorF("%s %s: chipset:   805",
                      XCONFIG_PROBED, s3InfoRec.name);
	    else
	       ErrorF("%s %s: chipset:   801",
                       XCONFIG_PROBED, s3InfoRec.name);
	    if (S3_801_I_SERIES(s3ChipId))
	       ErrorF("i");
	    ErrorF(", ");
	    if (S3_801_REV_C(s3ChipId))
	       ErrorF("rev C or above\n");
	    else
	       ErrorF("rev A or B\n");
	 } else if (S3_928_SERIES(s3ChipId)) {
	    if (S3_928_P(s3ChipId))
		ErrorF("%s %s: chipset:   928-P\n",
                   XCONFIG_PROBED, s3InfoRec.name);
	    else if (S3_928_REV_E(s3ChipId))
		ErrorF("%s %s: chipset:   928, rev E or above\n",
                   XCONFIG_PROBED, s3InfoRec.name);
	    else
	        ErrorF("%s %s: chipset:   928, rev D or below\n",
                   XCONFIG_PROBED, s3InfoRec.name);
	 }
      } else if (S3_911_SERIES(s3ChipId)) {
	 if (S3_911_ONLY(s3ChipId)) {
	    ErrorF("%s %s: chipset:   911 \n",
                   XCONFIG_PROBED, s3InfoRec.name);
	 } else if (S3_924_ONLY(s3ChipId)) {
	    ErrorF("%s %s: chipset:   924\n",
                   XCONFIG_PROBED, s3InfoRec.name);
	 } else {
	    ErrorF("%s %s: S3 chipset type unknown, chip_id = 0x%02x\n",
		   XCONFIG_PROBED, s3InfoRec.name, s3ChipId);
	 }
      }
   }

   if (xf86Verbose) {
      ErrorF("%s %s: chipset driver: %s\n",
	     OFLG_ISSET(XCONFIG_CHIPSET, &s3InfoRec.xconfigFlag) ?
		XCONFIG_GIVEN : XCONFIG_PROBED,
	     s3InfoRec.name, s3InfoRec.chipset);
   }

   if (!s3InfoRec.videoRam) {
      if ((config & 0x20) != 0) {	/* if bit 5 is a 1, then 512k RAM */
	 s3InfoRec.videoRam = 512;
      } else {			/* must have more than 512k */
	 if (S3_911_SERIES(s3ChipId)) {
	    s3InfoRec.videoRam = 1024;
	 } else {
	    switch ((config & 0xC0) >> 6) {	/* look at bits 6 and 7 */
	       case 0:
	         s3InfoRec.videoRam = 4096;
		 break;
	       case 1:
	         s3InfoRec.videoRam = 3072;
		 break;
	       case 2:
		 s3InfoRec.videoRam = 2048;
	         break;
	       case 3:
	         s3InfoRec.videoRam = 1024;
		 break;
	    }
	 }
      }
      if (xf86Verbose) {
         ErrorF("%s %s: videoram:  %dk\n",
              XCONFIG_PROBED, s3InfoRec.name, s3InfoRec.videoRam);
      }
   } else {
      if (xf86Verbose) {
	 ErrorF("%s %s: videoram:  %dk\n", 
              XCONFIG_GIVEN, s3InfoRec.name, s3InfoRec.videoRam);
      }
   }
   if (s3InfoRec.videoRam > 1024)
      s3Mbanks = -1;
   else
      s3Mbanks = 0;

   /*
    * Handle RAMDAC Option flags first.
    */
   if (OFLG_ISSET(OPTION_NORMAL_DAC, &s3InfoRec.options)) {
      s3RamdacType = NORMAL_DAC;
   }
   if (OFLG_ISSET(OPTION_BT485, &s3InfoRec.options)) {
      if (s3RamdacType != UNKNOWN_DAC) {
	 ErrorF("%s %s: Only one RAMDAC Option may be specified.  %s\n",
		s3InfoRec.name, XCONFIG_PROBED, "Ignoring \"bt485\"");
      } else {
	 if (!S3_928_SERIES(s3ChipId)) {
            ErrorF("%s %s: Bt485 is only supported on 928\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    OFLG_CLR(OPTION_BT485, &s3InfoRec.options);
	 } else {
            ErrorF("%s %s: Programming for BrookTree Bt485 RAMDAC\n",
	           XCONFIG_GIVEN, s3InfoRec.name);
            s3RamdacType = BT485_DAC;
	 }
      }
   }
   if (OFLG_ISSET(OPTION_20C505, &s3InfoRec.options)) {
      if (s3RamdacType != UNKNOWN_DAC) {
	 ErrorF("%s %s: Only one RAMDAC Option may be specified.  %s\n",
		s3InfoRec.name, XCONFIG_PROBED, "Ignoring \"20c505\"");
      } else {
	 if (!S3_928_SERIES(s3ChipId)) {
            ErrorF("%s %s: 20C505 is only supported on 928\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    OFLG_CLR(OPTION_20C505, &s3InfoRec.options);
	 } else {
            ErrorF("%s %s: Programming for AT&T 20C505 RAMDAC\n",
	           XCONFIG_GIVEN, s3InfoRec.name);
            s3RamdacType = ATT20C505_DAC;
	 }
      }
   }
   if (OFLG_ISSET(OPTION_TI3020_FAST, &s3InfoRec.options)) {
      OFLG_SET(OPTION_TI3020, &s3InfoRec.options);
   }
   if (OFLG_ISSET(OPTION_TI3020, &s3InfoRec.options)) {
      if (s3RamdacType != UNKNOWN_DAC) {
	 ErrorF("%s %s: Only one RAMDAC Option may be specified.  %s\n",
		s3InfoRec.name, XCONFIG_PROBED, "Ignoring \"ti3020\"");
      } else {
	 if (!S3_928_SERIES(s3ChipId)) {
            ErrorF("%s %s: Ti3020 is only supported on 928\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    OFLG_CLR(OPTION_TI3020, &s3InfoRec.options);
	 } else {
            ErrorF("%s %s: Programming for Ti ViewPoint 3020 RAMDAC\n",
	           XCONFIG_GIVEN, s3InfoRec.name);
	    s3RamdacType = TI3020_DAC;
	 }
      }
   }

   /* Make sure CR55 is unlocked for Bt485 probe */
   outb(vgaCRIndex, 0x39);
   outb(vgaCRReg, 0xA5);

   /* For chipsets other than 928, there is only one RAMDAC type possible */
   if (!S3_928_SERIES(s3ChipId)) {
      s3RamdacType = NORMAL_DAC;
   } else if (s3RamdacType == UNKNOWN_DAC) {
      /* Otherwise, probe for the RAMDAC type */
      /*
       * Bt485/AT&T20C505 first
       *
       * Probe for the bloody thing.  Set 0x3C6 to a bogus value, then
       * try to get the Bt485 status register.  If it's there, then we will
       * get something else back from this port.
       */
      unsigned char tmp2;
      tmp = inb(0x3C6);
      outb(0x3C6, 0x0F);
      if (((tmp2 = s3InBtStatReg()) & 0x80) == 0x80) {
	 /*
	  * Found either a BrookTree Bt485 or AT&T 20C505.
	  */
	 if ((tmp2 & 0xF0) == 0xD0) {
	    s3RamdacType = ATT20C505_DAC;
	    ErrorF("%s %s: Detected an AT&T 20C505 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	 } else {
	    s3RamdacType = BT485_DAC;
	    ErrorF("%s %s: Detected a BrookTree Bt485 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	 }
      }
      outb(0x3C6, tmp);

      /* If it wasn't a Bt485 or AT&T 20C505, probe for the Ti3020 */
      if (s3RamdacType == UNKNOWN_DAC) {
	 outb(vgaCRIndex, 0x55);
	 tmp = inb(vgaCRReg);
	 /* toggle to upper 4 direct registers */
	 outb(vgaCRReg, (tmp & 0xFC) | 0x01);
	 tmp1 = inb(TI_INDEX_REG);
	 outb(TI_INDEX_REG, TI_ID);
	 if (inb(TI_DATA_REG) == TI_VIEWPOINT_ID) {
	    /*
	     * Found TI ViewPoint DAC
	     */
	    ErrorF("%s %s: Detected a TI ViewPoint 3020 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    s3RamdacType = TI3020_DAC;
	 }
	 outb(TI_INDEX_REG, tmp1);
	 outb(vgaCRReg, tmp);
      }

      /* If it wasn't a Ti3020 either, it must be a "normal" ramdac */
      if (s3RamdacType == UNKNOWN_DAC) {
	 s3RamdacType = NORMAL_DAC;
      }
   }

   /* Now handle the various ramdac cursor options */

   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options)) {
      if (DAC_IS_BT485_SERIES) {
	 ErrorF("%s %s: Using hardware cursor from Bt485/20C505 RAMDAC\n",
		XCONFIG_GIVEN, s3InfoRec.name);
      } else {
	 ErrorF("%s %s: Bt485 cursor requires a Bt485 or 20C505 RAMDAC\n",
		XCONFIG_PROBED, s3InfoRec.name);
      }
   }

   if (DAC_IS_TI3020) {
      if (OFLG_ISSET(OPTION_NO_TI3020_CURS, &s3InfoRec.options)) {
         ErrorF("%s %s: Use of Ti3020 cursor disabled in Xconfig\n",
	        XCONFIG_GIVEN, s3InfoRec.name);
	 OFLG_CLR(OPTION_TI3020_CURS, &s3InfoRec.options);
      } else {
	 /* use the ramdac cursor by default */
	 ErrorF("%s %s: Using hardware cursor from Ti3020 RAMDAC\n",
	        OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name);
	 OFLG_SET(OPTION_TI3020_CURS, &s3InfoRec.options);
      }
   } else {
      if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options)) {
	 ErrorF("%s %s: Ti30205 cursor requires a Ti3020 RAMDAC\n",
		XCONFIG_PROBED, s3InfoRec.name);
      }
   }

   if (DAC_IS_BT485_SERIES &&
       (
        OFLG_ISSET(OPTION_STB_PEGASUS, &s3InfoRec.options) ||
        OFLG_ISSET(OPTION_NUMBER_NINE, &s3InfoRec.options) ||
        OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)))
      s3Bt485PixMux = TRUE;

   /* pixmux on Bt485 requires use of Bt's cursor */
   if (s3Bt485PixMux && !OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options)) {
      OFLG_SET(OPTION_BT485_CURS, &s3InfoRec.options);
      ErrorF("%s %s: Using hardware cursor from Bt485/20C505 RAMDAC\n",
	     XCONFIG_PROBED, s3InfoRec.name);
   }

   /* Set clock limit */
   switch(s3RamdacType) {
   case BT485_DAC:
   case ATT20C505_DAC:
      if (s3Bt485PixMux)
	 s3InfoRec.maxClock = s3MaxBt485MuxClock;
      else
	 s3InfoRec.maxClock = s3MaxBt485Clock;
      break;
   case TI3020_DAC:
      if (OFLG_ISSET(OPTION_TI3020_FAST, &s3InfoRec.options))
         s3InfoRec.maxClock = s3MaxTi3020ClockFast;
      else 
         s3InfoRec.maxClock = s3MaxTi3020Clock;
      break;
   default:
      s3InfoRec.maxClock = s3MaxClock;
   }

   /* Set the pix-mux description based on the ramdac type */
   if (DAC_IS_TI3020) {
      pixMuxPossible = TRUE;
      allowPixMuxInterlace = FALSE;
      allowPixMuxSwitching = FALSE;
      nonMuxMaxClock = 70000;
   } else if (s3Bt485PixMux) {
      /* XXXX Are the defaults for the other parameters correct? */
      pixMuxPossible = TRUE;
      allowPixMuxInterlace = FALSE;	/* It doesn't work right (yet) */
      allowPixMuxSwitching = FALSE;	/* XXXX Is this right? */
      if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
	 nonMuxMaxClock = 67500;	/* Doubling only works in mux mode */
	 nonMuxMaxMemory = 1024;	/* Can't access more without mux */
	 allowPixMuxSwitching = TRUE;
	 pixMuxLimitedWidths = FALSE;
	 pixMuxMinWidth = 1024;
      } else if (OFLG_ISSET(OPTION_NUMBER_NINE, &s3InfoRec.options)) {
	 nonMuxMaxClock = 67500;
	 allowPixMuxSwitching = TRUE;
	 pixMuxLimitedWidths = TRUE;
	 pixMuxMinWidth = 800;
      } else if (OFLG_ISSET(OPTION_STB_PEGASUS, &s3InfoRec.options)) {
	 nonMuxMaxClock = 67500;
	 allowPixMuxSwitching = TRUE;
	 pixMuxLimitedWidths = FALSE;
	 pixMuxMinWidth = 1024;
      } else {
	 nonMuxMaxClock = 85000;
      }
   }

   /*
    * clock options are now done after the ramdacs because the next
    * generation ramdacs will have a built in clock (i.e. TI 3025)
    */

   if (OFLG_ISSET(OPTION_LEGEND, &s3InfoRec.options)) {
      s3ClockSelectFunc = LegendClockSelect;
      numClocks = 32;
   } else if (OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      if (xf86Verbose)
         ErrorF("%s %s: Using ICD2061A programmable clock\n",
            XCONFIG_GIVEN, s3InfoRec.name);
      numClocks = 3;
   } else if (OFLG_ISSET(CLOCK_OPTION_ICD2061ASL, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      if (xf86Verbose)
         ErrorF("%s %s: Using ICD2061A programmable clock\n",
            XCONFIG_GIVEN, s3InfoRec.name);
	 ErrorF("\tNote: \"icd2061a_slow\" is deprecated; use \"icd2061a\"\n");
      numClocks = 3;
   } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      if (xf86Verbose)
	 ErrorF("%s %s: Using Sierra SC11412 programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
	 numClocks = 3;
   } else {
      s3ClockSelectFunc = s3ClockSelect;
      numClocks = 16;
      if (!s3InfoRec.clocks) 
         vgaGetClocks(numClocks, s3ClockSelectFunc);
   }

   if (xf86Verbose) {
      if (! OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
	 for (j = 0; j < s3InfoRec.clocks; j++) {
	    if ((j % 8) == 0) {
	       if (j != 0)
		  ErrorF("\n");
               ErrorF("%s %s: clocks:",
                OFLG_ISSET(XCONFIG_CLOCKS,&s3InfoRec.xconfigFlag) ?
                    XCONFIG_GIVEN : XCONFIG_PROBED , 
                s3InfoRec.name);
	    }
	    ErrorF(" %6.2f", (double)s3InfoRec.clock[j] / 1000.0);
         }
         ErrorF("\n");
      } 
   }

   if (pixMuxPossible && s3InfoRec.videoRam > nonMuxMaxMemory)
      pixMuxNeeded = TRUE;

   tx = s3InfoRec.virtualX;
   ty = s3InfoRec.virtualY;
   pMode = pEnd = s3InfoRec.modes;
   do {
      if (!xf86LookupMode(pMode, &s3InfoRec)) {
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return (FALSE);
      }
      if ((pMode->HDisplay * (1 + pMode->VDisplay)) >
	  s3InfoRec.videoRam * 1024) {
	 ErrorF("%s: Too little memory for mode %s\n", s3InfoRec.name,
		pMode->name);
	 ErrorF("%s: NB. 1 scan line is required for the hardware cursor\n",
	        s3InfoRec.name);
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return (FALSE);
      }
      s3InfoRec.virtualX = max(s3InfoRec.virtualX, pMode->HDisplay);
      s3InfoRec.virtualY = max(s3InfoRec.virtualY, pMode->VDisplay);

      /*
       * Check what impact each mode has on pixel multiplexing, and mark those
       * modes for which pixmux must be used.
       */
      if (pixMuxPossible) {
	 if ((s3InfoRec.clock[pMode->Clock] / 1000) >
	     (nonMuxMaxClock / 1000)) {
	    pMode->Flags |= V_PIXMUX;
	    pixMuxNeeded = TRUE;
	 }
	 if (s3InfoRec.videoRam > nonMuxMaxMemory)
	    pMode->Flags |= V_PIXMUX;

	 /*
	  * Check if pixmux can't be used.  There are two cases:
	  *
	  *   1. No switching between mux and non-mux modes.  In this case
	  *      the presence of any mode which can't be used in pixmux
	  *      mode is flagged.
	  *   2. Switching allowed.  In this cases the presence of modes
	  *      which require mux for one feature, but can't use it because
	  *      of another is flagged.
	  */
         if (!allowPixMuxSwitching || (pMode->Flags & V_PIXMUX)) {
	    if (pMode->HDisplay < pixMuxMinWidth)
	       pixMuxWidthOK = FALSE;
	    if ((pMode->Flags & V_INTERLACE) && !allowPixMuxInterlace)
	       pixMuxInterlaceOK = FALSE;
	 }
      }
      pMode = pMode->next;
   } while (pMode != pEnd);

   if ((tx != s3InfoRec.virtualX) || (ty != s3InfoRec.virtualY))
      OFLG_CLR(XCONFIG_VIRTUAL,&s3InfoRec.xconfigFlag);

   /*
    * Are we using pixel multiplexing, or does the mode combination mean
    * we can't continue
    */
   if (pixMuxPossible && pixMuxNeeded) {
      if (!pixMuxWidthOK) {
	 if (s3InfoRec.videoRam > nonMuxMaxMemory) {
	    ErrorF("To access more than %dkB video memory the RAMDAC must\n",
		   nonMuxMaxMemory);
	    ErrorF("operate in pixel multiplex mode, but pixel multiplexing\n");
	    ErrorF("cannot be used for modes of width less than %d.\n",
		   pixMuxMinWidth);
	    ErrorF("Adjust the Modes and/or VideoRam and Virtual lines in\n");
	    ErrorF("your Xconfig to meet these requirements\n");
	 } else {
	    ErrorF("Modes with a dot-clock above %dMHz require the RAMDAC to\n",
		   nonMuxMaxClock / 1000);
	    ErrorF("operate in pixel multiplex mode, but pixel multiplexing\n");
	    ErrorF("cannot be used for modes with width less than %d.\n",
		   pixMuxMinWidth);
	    ErrorF("Adjust the Modes line in your Xconfig to meet these ");
	    ErrorF("requirements.\n");
	 }
      }
      if (!pixMuxInterlaceOK) {
	 if (s3InfoRec.videoRam > nonMuxMaxMemory) {
	    ErrorF("To access more than %dkB video memory the RAMDAC must\n",
		   nonMuxMaxMemory);
	    ErrorF("operate in pixel multiplex mode, but pixel multiplexing\n");
	    ErrorF("cannot be used for interlaced modes.\n",
		   pixMuxMinWidth);
	    ErrorF("Adjust the Modes and/or VideoRam and Virtual lines in\n");
	    ErrorF("your Xconfig to meet these requirements\n");
	 } else {
	    ErrorF("Modes with a dot-clock above %dMHz require the RAMDAC to\n",
		   nonMuxMaxClock / 1000);
	    ErrorF("operate in pixel multiplex mode, but pixel multiplexing\n");
	    ErrorF("cannot be used for interlaced modes.\n");
	    ErrorF("Adjust the Modes line in your Xconfig to meet these ");
	    ErrorF("requirements.\n");
	 }
      }
      if (!pixMuxWidthOK || !pixMuxInterlaceOK) {
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return(FALSE);
      } else {
	 if (xf86Verbose)
	    ErrorF("%s %s: Operating RAMDAC in pixel multiplex mode\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 s3UsingPixMux = TRUE;
      }
   }

   if (s3UsingPixMux && !allowPixMuxSwitching) {
      /* Mark all modes as V_PIXMUX */
      pEnd = pMode = s3InfoRec.modes;
      do {
	 pMode->Flags |= V_PIXMUX;
         pMode = pMode->next;
      } while (pMode != pEnd);
   }

   if (DAC_IS_BT485_SERIES || DAC_IS_TI3020) {
      if (OFLG_ISSET(OPTION_DAC_8_BIT, &s3InfoRec.options))
	 s3DAC8Bit = TRUE;
      if (OFLG_ISSET(OPTION_SYNC_ON_GREEN, &s3InfoRec.options)) {
	 s3DACSyncOnGreen = TRUE;
	 if (xf86Verbose)
	    ErrorF("%s %s: Putting RAMDAC into sync-on-green mode\n",
		   XCONFIG_GIVEN, s3InfoRec.name);
      }
   }

   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      if (xf86Verbose)
         ErrorF("%s %s: Using AT&T 20C490/1 RAMDAC\n",
            XCONFIG_GIVEN, s3InfoRec.name);
      if (OFLG_ISSET(OPTION_DAC_8_BIT, &s3InfoRec.options))
         s3DAC8Bit = TRUE;
   }
     
   if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      if (xf86Verbose)
         ErrorF("%s %s: Using Sierra SC 15025/6 RAMDAC\n",
            XCONFIG_GIVEN, s3InfoRec.name);
      if (OFLG_ISSET(OPTION_DAC_8_BIT, &s3InfoRec.options))
         s3DAC8Bit = TRUE;
   }

   if (s3DAC8Bit && xf86Verbose)
      ErrorF("%s %s: Putting RAMDAC into 8-bit mode\n",
         XCONFIG_GIVEN, s3InfoRec.name);

   if (S3_911_SERIES(s3ChipId)) {
      maxDisplayWidth = 1024;
      maxDisplayHeight = 1024 - 1; /* Cursor takes exactly 1 line for 911 */
   } else {
      maxDisplayWidth = 2048;
      maxDisplayHeight = 4096 - 3; /* Cursor can take up to 3 lines */
   }
   if (s3InfoRec.virtualX > maxDisplayWidth) {
      ErrorF("%s: Illegal screen size: (%dx%d)\n", s3InfoRec.name,
	     s3InfoRec.virtualX, s3InfoRec.virtualY);
      ErrorF("\tVirtual width must be no greater than %d\n", maxDisplayWidth);
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return (FALSE);
   }
   if (s3InfoRec.virtualY > maxDisplayHeight) {
      ErrorF("%s: Illegal screen size: (%dx%d)\n", s3InfoRec.name,
	     s3InfoRec.virtualX, s3InfoRec.virtualY);
      ErrorF("\tVirtual height must be no greater than %d\n", maxDisplayHeight);
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return (FALSE);
   }
 
   /* Select the appropriate logical line width */
   if (s3UsingPixMux && pixMuxLimitedWidths) {
      if (s3InfoRec.virtualX <= 1024) {
	 s3DisplayWidth = 1024;
      } else if (s3InfoRec.virtualX <= 2048) {
	 s3DisplayWidth = 2048;
      } else { /* should never get here */
	 ErrorF("Internal error in DisplayWidth check, virtual width = %d\n",
	        s3InfoRec.virtualX);
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return (FALSE);
      }
   } else if (S3_911_SERIES(s3ChipId)) {
      s3DisplayWidth = 1024;
   } else {
      if (s3InfoRec.virtualX <= 640) {
	 s3DisplayWidth = 640;
      } else if (s3InfoRec.virtualX <= 800) {
	 s3DisplayWidth = 800;
      } else if (s3InfoRec.virtualX <= 1024) {
	 s3DisplayWidth = 1024;
      } else if ((s3InfoRec.virtualX <= 1152) &&
		 (S3_801_REV_C(s3ChipId) || S3_928_REV_E(s3ChipId))) {
	 s3DisplayWidth = 1152;
      } else if (s3InfoRec.virtualX <= 1280) {
	 s3DisplayWidth = 1280;
      } else if ((s3InfoRec.virtualX <= 1600) && S3_928_REV_E(s3ChipId)) {
	 s3DisplayWidth = 1600;
      } else if (s3InfoRec.virtualX <= 2048) {
	 s3DisplayWidth = 2048;
      } else { /* should never get here */
	 ErrorF("Internal error in DisplayWidth check, virtual width = %d\n",
	        s3InfoRec.virtualX);
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return (FALSE);
      }
   }
      
   /*
    * Work out where to locate S3's HW cursor storage.  It must be on a
    * 1k boundary.  When using a RAMDAC cursor, set s3CursorStartY
    * and s3CursorLines appropriately for the memory usage calculation below
    */

   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options) ||
       OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options)) {
      s3CursorStartY = s3InfoRec.virtualY;
      s3CursorLines = 0;
   } else {
      int st_addr = (s3InfoRec.virtualY * s3DisplayWidth + 1023) / 1024 * 1024;
      s3CursorStartX = st_addr % s3DisplayWidth;
      s3CursorStartY = st_addr / s3DisplayWidth;
      s3CursorLines = ((s3CursorStartX + 1023) / s3DisplayWidth) + 1;
   }

   /*
    * Reduce the videoRam value if necessary to prevent Y coords exceeding
    * the 12-bit (4096) limit when small display widths are used on cards
    * with a lot of memory
    */
   if (s3InfoRec.videoRam * 1024 / s3DisplayWidth > 4096) {
      s3InfoRec.videoRam = s3DisplayWidth * 4096 / 1024;
      ErrorF("%s %s: videoram usage reduced to %dk to avoid co-ord overflow\n",
	     XCONFIG_PROBED, s3InfoRec.name, s3InfoRec.videoRam);
   }
  
   if ((s3DisplayWidth * (s3CursorStartY + s3CursorLines)) >
       s3InfoRec.videoRam * 1024) { /* XXXX improve this message */
      ErrorF("%s %s: Display size %dx%d is too large: ", 
             OFLG_ISSET(XCONFIG_VIRTUAL,&s3InfoRec.xconfigFlag) ?
                 XCONFIG_GIVEN : XCONFIG_PROBED,
             s3InfoRec.name,
	     s3DisplayWidth, s3InfoRec.virtualY);
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return (FALSE);
   }
   if (xf86Verbose) {
      ErrorF("%s %s: Virtual resolution set to %dx%d\n", 
             OFLG_ISSET(XCONFIG_VIRTUAL,&s3InfoRec.xconfigFlag) ?
                 XCONFIG_GIVEN : XCONFIG_PROBED,
             s3InfoRec.name,
	     s3InfoRec.virtualX, s3InfoRec.virtualY);
   }

   return TRUE;
}

static Bool
s3ClockSelect(no)
     int   no;

{
   unsigned char temp;
   static unsigned char save1, save2;
 
   UNLOCK_SYS_REGS;
   
   switch(no)
   {
   case CLK_REG_SAVE:
      save1 = inb(0x3CC);
      outb(vgaCRIndex, 0x42);
      save2 = inb(vgaCRReg);
      break;
   case CLK_REG_RESTORE:
      outb(0x3C2, save1);
      outb(vgaCRIndex, 0x42);
      outb(vgaCRReg, save2);
      break;
   default:
      if (no == 0x03)
      {
	 /*
	  * Clock index 3 is a 0Hz clock on all the S3-recommended 
	  * synthesizers (except the Chrontel).  A 0Hz clock will lock up 
	  * the chip but good (requiring power to be cycled).  Nuke that.
	  */
         LOCK_SYS_REGS;
	 return(FALSE);
      }
      temp = inb(0x3CC);
      outb(0x3C2, temp | 0x0C);
      outb(vgaCRIndex, 0x42);
      outb(vgaCRReg, no);
      usleep(150000);
   }
   LOCK_SYS_REGS;
   return(TRUE);
}

static Bool
LegendClockSelect(no)
     int   no;
{

 /*
  * Sigma Legend special handling
  * 
  * The Legend uses an ICS 1394-046 clock generator.  This can generate 32
  * different frequencies.  The Legend can use all 32.  Here's how:
  * 
  * There are two flip/flops used to latch two inputs into the ICS clock
  * generator.  The five inputs to the ICS are then
  * 
  * ICS     ET-4000 ---     --- FS0     CS0 FS1     CS1 FS2     ff0 flip/flop 0
  * outbut FS3     CS2 FS4     ff1     flip/flop 1 outbut
  * 
  * The flip/flops are loaded from CS0 and CS1.  The flip/flops are latched by
  * CS2, on the rising edge. After CS2 is set low, and then high, it is then
  * set to its final value.
  * 
  */
   static unsigned char save1, save2;
   unsigned char temp = inb(0x3CC);

   switch(no)
   {
   case CLK_REG_SAVE:
      save1 = inb(0x3CC);
      outb(vgaCRIndex, 0x34);
      save2 = inb(vgaCRReg);
      break;
   case CLK_REG_RESTORE:
      outb(0x3C2, save1);
      outw(vgaCRIndex, 0x34 | (save2 << 8));
      break;
   default:
      outb(0x3C2, (temp & 0xF3) | ((no & 0x10) >> 1) | (no & 0x04));
      outw(vgaCRIndex, 0x0034);
      outw(vgaCRIndex, 0x0234);
      outw(vgaCRIndex, ((no & 0x08) << 6) | 0x34);
      outb(0x3C2, (temp & 0xF3) | ((no << 2) & 0x0C));
   }
   return(TRUE);
}

static Bool
icd2061ClockSelect(no)
     int   no;
{
   long  clockNo;
   long  freq;
   Bool result = TRUE;

#define MAX_SC11412_FREQ  (DAC_IS_BT485_SERIES ? \
			   ((s3RamdacType == ATT20C505_DAC) ? 90000 : 67500) : \
			   100000)

   UNLOCK_SYS_REGS;
   switch(no)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = s3ClockSelect(no);
      break;
   default:
      if (no < 2) {
         result = s3ClockSelect(no);
      } else {
	 if (no >= s3InfoRec.clocks) {
	    ErrorF("%s: Clock number too high (%d)\n", s3InfoRec.name, no);
	    result = FALSE;
	    break;
	 }
	 /* Start with freq in kHz */
	 freq = s3InfoRec.clock[no];
	 /* Check if clock frequency is within range */
	 if (
	     !OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) &&
	     OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions) &&
	     freq > MAX_SC11412_FREQ) {
	    /* SC11412 limit as there is no clockdoubling yet */
	    ErrorF("%s %s: Specified dot clock (%.3f) too high for SC11412",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    ErrorF("\tUsing %.3fMHz\n", MAX_SC11412_FREQ / 1000.0);
	    freq = MAX_SC11412_FREQ;
	 }
	 if (freq > s3InfoRec.maxClock) {
	    ErrorF("%s %s: Specified dot clock (%.3f) too high for RAMDAC.",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    ErrorF("\tUsing %.3fMHz\n", s3InfoRec.maxClock / 1000.0);
	    freq = s3InfoRec.maxClock;
	 }
	 if (DAC_IS_BT485_SERIES &&
	     (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) ||
	      !OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions))) {
	    if (freq > ((s3RamdacType == ATT20C505_DAC) ? 90000 : 67500)) {
	       s3ClockDouble = TRUE;
	       freq /= 2;
	    } else {
	       s3ClockDouble = FALSE;
	    }
#if 0
	    /* Set the clock doubler here when not using pixmux */
	    if (!s3Bt485PixMux) {
	       ErrorF("Setting clock doubler in ClockSelect(), freq = %.3f\n",
		      freq / 500.0);
	       s3OutBtRegCom3(0xF7, s3ClockDouble ? 0x08 : 0x00);
	    }
#endif
	 } else if (DAC_IS_TI3020) {
	    if (freq > 100000) {
	       s3ClockDouble = TRUE;
	       /* Use Ti3020 clock doubler */
	       freq /= 2;
#if 0
	       s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_CLK1_DOUBLE);
#endif
	    } else {
	       s3ClockDouble = FALSE;
	       /* No doubler */
#if 0
	       s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_CLK1);
#endif
	    }
	 }
	 /* Convert freq to Hz */
	 freq *= 1000;
	 /* Use the "Alt" version always since it is more reliable */
	 if (OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions) ||
	     OFLG_ISSET(CLOCK_OPTION_ICD2061ASL, &s3InfoRec.clockOptions)) {
	    AltICD2061SetClock(freq, 2);
	    AltICD2061SetClock(freq, 2);
	    AltICD2061SetClock(freq, 2);
	 } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
	    result = SC11412SetClock((long)freq/1000);
	 } else { /* Should never get here */
	    result = FALSE;
	    break;
	 }
         outb(vgaCRIndex, 0x42);/* select the clock */
         outb(vgaCRReg, 0x02);
	 usleep(150000);
	 /* Do the clock doubler selection in s3Init() */
#if 0
	 if (s3Bt485PixMux) {
	    s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01); /* sleep mode */
	    s3OutBtReg(BT_COMMAND_REG_2, 0xEF, 0x10); /* pclock 1   */
	    /* Use Bt485 clock doubler - Bit 3 of Command Reg 3 */
	    s3OutBtRegCom3(0xF7, (s3ClockDouble ? 0x08 : 0x00));
	    s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x00); /* wake up    */
	 }
#endif
      }
   }
   LOCK_SYS_REGS;
   return(result);
}
