/* $XConsortium: s3.c,v 1.1 94/03/28 21:13:36 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3.c,v 3.39 1994/09/23 13:38:07 dawes Exp $ */
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

#define ICS2595

#include "misc.h"
#include "cfb.h"
#include "pixmapstr.h"
#include "fontstruct.h"
#include "s3.h"
#include "regs3.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "s3linear.h"
#include "s3Bt485.h"
#include "s3Ti3020.h"
#include "s3ELSA.h"

extern int s3MaxClock;
extern int s3MaxBt485Clock, s3MaxBt485MuxClock;
extern int s3MaxTi3020Clock, s3MaxTi3020Clock175, s3MaxTi3020ClockFast;
extern int s3MaxATT498Clock, s3MaxATT498MuxClock;
char s3Mbanks;
int s3Weight = RGB8_PSEUDO;
extern char *xf86VisualNames[];

extern s3VideoChipPtr s3Drivers[];

int vgaInterlaceType = VGA_DIVIDE_VERT;
void (*vgaSaveScreenFunc)() = (void (*)())NoopDDA;

extern int defaultColorVisualClass;

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
   {5, 6, 5},			/* xrgb weight */
   8,				/* int bitsPerPixel */
   PseudoColor,			/* int defaultVisual */
   -1, -1,			/* int virtualX,virtualY */
   -1,				/* int displayWidth */
   -1, -1, -1, -1,		/* int frameX0, frameY0, frameX1, frameY1 */
   {0,},			/* OFlagSet options */
   {0,},			/* OFlagSet clockOptions */   
   {0, },              		/* OFlagSet xconfigFlag */
   NULL,			/* char *chipset */
   NULL,			/* char *ramdac */
   0,				/* int dacSpeed */
   0,				/* int clocks */
   {0,},			/* int clock[MAXCLOCKS] */
   0,				/* int maxClock */
   0,				/* int videoRam */
   0xC0000,                     /* int BIOSbase */  
   0,				/* unsigned long MemBase */
   240, 180,			/* int width, height */
   0,				/* unsigned long  speedup */
   NULL,			/* DisplayModePtr modes */   
   NULL,			/* DisplayModePtr pModes */   
   NULL,			/* char           *clockprog */
   -1,			        /* int textclock */
   FALSE,			/* Bool           bankedMono */
   "S3",			/* char           *name */
   {0, },			/* xrgb blackColour */
   {0, },			/* xrgb whiteColour */
   s3ValidTokens,		/* int *validTokens */
   S3_PATCHLEVEL,		/* char *patchlevel */
   0,				/* int IObase */
   0,				/* int PALbase */
   0,				/* int COPbase */
   0,				/* int POSbase */
   0,				/* int instance */
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

static unsigned S3_IOPorts[] = { DISP_STAT, H_TOTAL, H_DISP, H_SYNC_STRT,
  H_SYNC_WID, V_TOTAL, V_DISP, V_SYNC_STRT, V_SYNC_WID, DISP_CNTL,
  ADVFUNC_CNTL, SUBSYS_STAT, SUBSYS_CNTL, ROM_PAGE_SEL, CUR_Y, CUR_X,
  DESTY_AXSTP, DESTX_DIASTP, ERR_TERM, MAJ_AXIS_PCNT, GP_STAT, CMD,
  SHORT_STROKE, BKGD_COLOR, FRGD_COLOR, WRT_MASK, RD_MASK, COLOR_CMP,
  BKGD_MIX, FRGD_MIX, MULTIFUNC_CNTL, PIX_TRANS, PIX_TRANS_EXT,
};


static int Num_S3_IOPorts = (sizeof(S3_IOPorts)/sizeof(S3_IOPorts[0]));

static SymTabRec s3DacTable[] = {
   { NORMAL_DAC,	"normal" },
   { BT485_DAC,		"bt485" },
   { ATT20C505_DAC,	"att20c505" },
   { TI3020_DAC,	"ti3020" },
   { ATT20C498_DAC,	"att20c498" },
   { TI3025_DAC,	"ti3025" },
   { ATT20C490_DAC,	"att20c490" },
   { SC15025_DAC,	"sc15025" },
   { STG1700_DAC,	"stg1700" },
   { S3_SDAC_DAC,	"s3_sdac" },
   { -1,		"" },
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
Bool  (*s3ClockSelectFunc) ();
static Bool LegendClockSelect();
static Bool s3ClockSelect();
static Bool icd2061ClockSelect();
static Bool s3GendacClockSelect();
static Bool ti3025ClockSelect();
ScreenPtr s3savepScreen;
Bool  s3Localbus = FALSE;
Bool  s3LinearAperture = FALSE;
Bool  s3Mmio928 = FALSE;
Bool  s3DAC8Bit = FALSE;
Bool  s3DACSyncOnGreen = FALSE;
Bool  s3PCIHack = FALSE;
Bool  s3PowerSaver = FALSE;
unsigned char s3LinApOpt;
unsigned char s3SAM256 = 0x00;
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
int s3hotX, s3hotY;
Bool s3BlockCursor, s3ReloadCursor;
int s3CursorStartX, s3CursorStartY, s3CursorLines;
int s3RamdacType = UNKNOWN_DAC;
Bool s3UsingPixMux = FALSE;
Bool s3Bt485PixMux = FALSE;
Bool s3ATT498PixMux = FALSE;
static int maxRawClock = 0;
static Bool clockDoublingPossible = FALSE;

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
   char *card, *serno;
   int card_id, max_pix_clock, max_mem_clock;

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
   int nonMuxMaxMemory = 8192;
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

   if (s3InfoRec.ramdac) {
      s3RamdacType = xf86StringToToken(s3DacTable, s3InfoRec.ramdac);
      if (s3RamdacType < 0) {
	 ErrorF("%s %s: Unknown RAMDAC type \"%s\"\n", XCONFIG_GIVEN,
		s3InfoRec.name, s3InfoRec.ramdac);
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return(FALSE);
      }
   }

   OFLG_ZERO(&validOptions);
   OFLG_SET(OPTION_LEGEND, &validOptions);
   OFLG_SET(OPTION_NOLINEAR_MODE, &validOptions);
   if (!S3_x64_SERIES(s3ChipId))
      OFLG_SET(OPTION_NO_MEM_ACCESS, &validOptions);
   OFLG_SET(OPTION_BT485_CURS, &validOptions);
   OFLG_SET(OPTION_SHOWCACHE, &validOptions);
   OFLG_SET(OPTION_FB_DEBUG, &validOptions);
   OFLG_SET(OPTION_TI3020_CURS, &validOptions);
   OFLG_SET(OPTION_NO_TI3020_CURS, &validOptions);
   OFLG_SET(OPTION_DAC_8_BIT, &validOptions);
   OFLG_SET(OPTION_FAST_DRAM, &validOptions);
   OFLG_SET(OPTION_MED_DRAM, &validOptions);
   OFLG_SET(OPTION_SLOW_DRAM, &validOptions);
   OFLG_SET(OPTION_SYNC_ON_GREEN, &validOptions);
   OFLG_SET(OPTION_SPEA_MERCURY, &validOptions);
   OFLG_SET(OPTION_NUMBER_NINE, &validOptions);
   OFLG_SET(OPTION_STB_PEGASUS, &validOptions);
   /* ELSA_W1000PRO isn't really required any more */
   OFLG_SET(OPTION_ELSA_W1000PRO, &validOptions);
   OFLG_SET(OPTION_ELSA_W2000PRO, &validOptions);
#if 0
   /* These aren't needed any more */
   OFLG_SET(OPTION_STEALTH64, &validOptions);
   OFLG_SET(OPTION_MIRO_CRYSTAL20SV, &validOptions);
#endif
   if (S3_928_P(s3ChipId))
      OFLG_SET(OPTION_PCI_HACK, &validOptions);
   OFLG_SET(OPTION_POWER_SAVER, &validOptions);
   xf86VerifyOptions(&validOptions, &s3InfoRec);

   if (S3_x64_SERIES(s3ChipId))
      if (OFLG_ISSET(OPTION_NO_MEM_ACCESS, &s3InfoRec.options)) {
	 ErrorF("%s %s: Option \"nomemaccess\" is ignored for 864/964\n",
		XCONFIG_PROBED, s3InfoRec.name);
	 OFLG_CLR(OPTION_NO_MEM_ACCESS, &s3InfoRec.options);
      }

   /* LocalBus or EISA or PCI */
   s3Localbus = ((config & 0x03) <= 2) || S3_928_P(s3ChipId);

   if (xf86Verbose) {
      if (S3_928_P(s3ChipId)) {
	 ErrorF("%s %s: card type: PCI\n", XCONFIG_PROBED, s3InfoRec.name);
      } else {
	 switch (config & 0x03) {
	 case 0:
	    ErrorF("%s %s: card type: EISA\n", XCONFIG_PROBED, s3InfoRec.name);
	    break;
	 case 1:
            ErrorF("%s %s: card type: 386/486 localbus\n",
        	   XCONFIG_PROBED, s3InfoRec.name);
	    break;
	 case 3:
            ErrorF("%s %s: card type: ISA\n", XCONFIG_PROBED, s3InfoRec.name);
	    break;
	 case 2:
	    ErrorF("%s %s: card type: PCI\n", XCONFIG_PROBED, s3InfoRec.name);
	 }
      }
   }

   card_id = s3DetectELSA(s3InfoRec.BIOSbase, &card, &serno, &max_pix_clock, &max_mem_clock);
   if (card_id > 0) {
      ErrorF("%s %s: card: %s, Ser.No. %s\n",
	     XCONFIG_PROBED, s3InfoRec.name, card, serno);
      free(card);
      free(serno);

      do {
	 switch (card_id) {
	 case ELSA_WINNER_1000PRO:
	    /* This option isn't required at the moment */
	    OFLG_SET(OPTION_ELSA_W1000PRO,  &s3InfoRec.options);
	    break;
	 case ELSA_WINNER_2000PRO:
	    OFLG_SET(OPTION_ELSA_W2000PRO,  &s3InfoRec.options);
	    break;
	 case ELSA_WINNER_1000:
	 case ELSA_WINNER_1000VL:
	 case ELSA_WINNER_1000PCI:
	 case ELSA_WINNER_1000ISA:
	 case ELSA_WINNER_2000:
	 case ELSA_WINNER_2000VL:
	 case ELSA_WINNER_2000PCI:
	    break;
	 default: continue; /* unknown card_id, don't set ICD2061A flags */
	 }

	 /* a known ELSA card_id was returned, set ICD 2061A clock support */

	 if (!OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions)) {
	    OFLG_SET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions);
	    OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
	    s3ClockSelectFunc = icd2061ClockSelect;
	    if (xf86Verbose)
	       ErrorF("%s %s: Using ICD2061A programmable clock\n",
		      XCONFIG_PROBED, s3InfoRec.name);
	    numClocks = 3;
	 }
      } while (0);
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
            if (S3_805_I_SERIES(s3ChipId)) {
               ErrorF("%s %s: chipset:   805i",
                      XCONFIG_PROBED, s3InfoRec.name);
               if ((config & 0x03) == 3)
                  ErrorF(" (ISA)");
               else
                  ErrorF(" (VL)");
            }
	    else if (!((config & 0x03) == 3))
	       ErrorF("%s %s: chipset:   805",
                      XCONFIG_PROBED, s3InfoRec.name);
	    else
	       ErrorF("%s %s: chipset:   801",
                       XCONFIG_PROBED, s3InfoRec.name);
	    ErrorF(", ");
	    if (S3_801_REV_C(s3ChipId))
	       ErrorF("rev C or above\n");
	    else
	       ErrorF("rev A or B\n");
	 } else if (S3_928_SERIES(s3ChipId)) {
	    char *pci = S3_928_P(s3ChipId) ? "-P" : "";
	    if (S3_928_REV_E(s3ChipId))
		ErrorF("%s %s: chipset:   928%s, rev E or above\n",
                   XCONFIG_PROBED, s3InfoRec.name, pci);
	    else
	        ErrorF("%s %s: chipset:   928%s, rev D or below\n",
                   XCONFIG_PROBED, s3InfoRec.name, pci);
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
	    switch ((config & 0xE0) >> 5) {	/* look at bits 6 and 7 */
	       case 0:
	         s3InfoRec.videoRam = 4096;
		 break;
	       case 2:
	         s3InfoRec.videoRam = 3072;
		 break;
	       case 3:
	         s3InfoRec.videoRam = 8192;
		 break;
	       case 4:
		 s3InfoRec.videoRam = 2048;
	         break;
	       case 5:
		 s3InfoRec.videoRam = 6144;
	         break;
	       case 6:
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

   if (xf86bpp < 0) {
      xf86bpp = s3InfoRec.depth;
   }
   if (xf86weight.red == 0 || xf86weight.green == 0 || xf86weight.blue == 0) {
      xf86weight = s3InfoRec.weight;
   }
   switch (xf86bpp) {
   case 8:
      break;
   case 15:
      s3InfoRec.depth = 15;
      xf86bpp = 16;
      s3Weight = RGB16_555;
      xf86weight.red = xf86weight.green = xf86weight.blue = 5;
      s3InfoRec.bitsPerPixel = 16;
      if (s3InfoRec.defaultVisual < 0)
	 s3InfoRec.defaultVisual = TrueColor;
      if (defaultColorVisualClass < 0)
	 defaultColorVisualClass = s3InfoRec.defaultVisual;
      break;
   case 16:
      if (xf86weight.red==5 && xf86weight.green==5 && xf86weight.blue==5) {
	 s3Weight = RGB16_555;
	 s3InfoRec.depth = 15;
      }
      else if (xf86weight.red==5 && xf86weight.green==6 && xf86weight.blue==5) {
	 s3Weight = RGB16_565;
	 s3InfoRec.depth = 16;
      }
      else {
	 ErrorF(
	   "Invalid color weighting %1d%1d%1d (only 555 and 565 are valid)\n",
	   xf86weight.red,xf86weight.green,xf86weight.blue);
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return(FALSE);
      }
      s3InfoRec.bitsPerPixel = 16;
      if (s3InfoRec.defaultVisual < 0)
	 s3InfoRec.defaultVisual = TrueColor;
      if (defaultColorVisualClass < 0)
	 defaultColorVisualClass = s3InfoRec.defaultVisual;
      break;
   case 24:
   case 32:
      xf86bpp = 32;
      s3InfoRec.depth = 24;
      s3InfoRec.bitsPerPixel = 32; /* Use sparse 24 bpp (RGBX) */
      s3Weight = RGB32_888;
      /* s3MaxClock = S3_MAX_32BPP_CLOCK; */
      xf86weight.red =  xf86weight.green = xf86weight.blue = 8;
      if (s3InfoRec.defaultVisual < 0)
	 s3InfoRec.defaultVisual = TrueColor;
      if (defaultColorVisualClass < 0)
	 defaultColorVisualClass = s3InfoRec.defaultVisual;
      break;
   default:
      ErrorF(
	"Invalid value for bpp.  Valid values are 8, 15, 16, 24 and 32.\n");
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
   }

   if (s3InfoRec.bitsPerPixel > 8 &&
       defaultColorVisualClass >= 0 && defaultColorVisualClass != TrueColor) {
      ErrorF("Invalid default visual type: %d (%s)\n", defaultColorVisualClass,
	     xf86VisualNames[defaultColorVisualClass]);
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
   }

   s3Bpp = s3InfoRec.bitsPerPixel / 8;

   /* Make sure CR55 is unlocked for Bt485 probe */
   outb(vgaCRIndex, 0x39);
   outb(vgaCRReg, 0xA5);

   /*
    * For chipsets other than 928, 805i or 864/964, there is only one RAMDAC
    * type possible.  Only probe for 928, 805i and 864/964.
    */

   /* XXXX The "Detected an ....." messages should probably go */

   if (S3_928_SERIES(s3ChipId) || S3_x64_SERIES(s3ChipId)
       || S3_805_I_SERIES(s3ChipId)) {
      /* First probe for Ti3020 and Ti3025 */
      if (s3RamdacType == UNKNOWN_DAC) {
	 unsigned char saveCR55, saveCR5C, saveTIndx, saveTIndx2, saveTIdata;

	 outb(vgaCRIndex, 0x55);
	 saveCR55 = inb(vgaCRReg);
	 /* toggle to upper 4 direct registers */
	 outb(vgaCRReg, (saveCR55 & 0xFC) | 0x01);

	 saveTIndx = inb(TI_INDEX_REG);
	 outb(TI_INDEX_REG, TI_ID);
	 if (inb(TI_DATA_REG) == TI_VIEWPOINT20_ID) {
	    /*
	     * Found TI ViewPoint 3020 DAC
	     */
	    ErrorF("%s %s: Detected a TI ViewPoint 3020 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    s3RamdacType = TI3020_DAC;
	 } else {
	    outb(vgaCRIndex, 0x5C);
	    saveCR5C = inb(vgaCRReg);
	    /* clear 0x20 (RS4) for 3020 mode */
	    outb(vgaCRReg, saveCR5C & 0xDF);
	    saveTIndx2 = inb(TI_INDEX_REG);
	    /* already twiddled CR55 above */
	    outb(TI_INDEX_REG, TI_CURS_CONTROL);
	    saveTIdata = inb(TI_DATA_REG);
	    /* clear TI_PLANAR_ACCESS bit */
	    outb(TI_DATA_REG, saveTIdata & 0x7F);

	    outb(TI_INDEX_REG, TI_ID);
	    if (inb(TI_DATA_REG) == TI_VIEWPOINT25_ID) {
	       /*
	        * Found TI ViewPoint 3025 DAC
	        */
	       ErrorF("%s %s: Detected a TI ViewPoint 3025 RAMDAC\n",
	              XCONFIG_PROBED, s3InfoRec.name);
	       s3RamdacType = TI3025_DAC;
	    }

	    /* restore this mess */
	    outb(TI_INDEX_REG, TI_CURS_CONTROL);
	    outb(TI_DATA_REG, saveTIdata);
	    outb(TI_INDEX_REG, saveTIndx2);
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, saveCR5C);
	 }
	 outb(TI_INDEX_REG, saveTIndx);
	 outb(vgaCRIndex, 0x55);
	 outb(vgaCRReg, saveCR55);
      }

      /*
       * Bt485/AT&T20C505 next
       *
       * Probe for the bloody thing.  Set 0x3C6 to a bogus value, then
       * try to get the Bt485 status register.  If it's there, then we will
       * get something else back from this port.
       */

      if (s3RamdacType == UNKNOWN_DAC) {
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
      }

      /* If it wasn't a Bt485, probe for the ATT 20C498 */
      if (s3RamdacType == UNKNOWN_DAC) {
	 int dir, mir;
	 xf86dactopel();
	 xf86dactocomm();
	 (void)inb(0x3C6);
	 mir = inb(0x3C6);
	 dir = inb(0x3C6);
	 xf86dactopel();

	 if ((mir == 0x84) && (dir == 0x98)) {
	    ErrorF("%s %s: Detected an ATT 20C498 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    s3RamdacType = ATT498_DAC;
	 }
      }

      /* now, probe for the SC 15025/26 */
      if (s3RamdacType == UNKNOWN_DAC) {
	 int i;
	 unsigned char c,id[4];
	 c = xf86getdaccomm();
	 xf86setdaccomm(c | 0x10);
	 for (i=0; i<4; i++) {
	    outb(0x3C7, 0x9+i); 
	    id[i] = inb(0x3C8);
	 }
	 xf86setdaccomm(c);
	 xf86dactopel();
	 if (id[0] == 'S' &&                  /* Sierra */
	     ((id[1]<<8)|id[2]) == 15025) {   /* unique for the SC 15025/26 */
	    if (id[3] != 'A') {                     /* version number */
	       ErrorF(
		"%s %s: ==> New Sierra SC 15025/26 version (0x%x) found,\n",
		XCONFIG_PROBED, s3InfoRec.name, id[3]);
	       ErrorF("\tplease report!\n");
	    }
	    ErrorF("%s %s: Detected a Sierra SC 15025/26 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    s3RamdacType = SC15025_DAC;
	 }
      }

      /* If it wasn't a Sierra, probe for the STG1700 */
      if (s3RamdacType == UNKNOWN_DAC) 
      {
         int cid, did;
	 xf86setdaccommbit(0x10);
	 xf86dactocomm();
         inb(0x3c6);
         outb(0x3c6, 0x00);
         outb(0x3c6, 0x00);
         cid = inb(0x3c6);     /* company ID */
         did = inb(0x3c6);     /* device ID */
	 xf86dactopel();

         if ((cid == 0x44) && (did == 0x00))
         {
	    ErrorF("%s %s: Detected an STG1700 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	    s3RamdacType = STG1700_DAC;
         }
      }

      /* If it wan't an SGS/inmos, probe for S3 SDAC */
      if (s3RamdacType == UNKNOWN_DAC)
      {
         xf86dactopel();
         inb(0x3c6);
         inb(0x3c6);
         inb(0x3c6);

         /* the forth read will show the chip ID and revision */
         if ((inb(0x3c6) & 0xf0) == 0x70)
         {
            ErrorF("%s %s: Detected an S3 SDAC 86C716 RAMDAC\n",
                   XCONFIG_PROBED, s3InfoRec.name);
            s3RamdacType = S3_SDAC_DAC;
         }
      }
   }

   /* If we still don't know the ramdac type, set it to NORMAL_DAC */
   if (s3RamdacType == UNKNOWN_DAC) {
      s3RamdacType = NORMAL_DAC;
   }
   
   /* make sure s3InfoRec.ramdac is set correctly */
   s3InfoRec.ramdac = xf86TokenToString(s3DacTable, s3RamdacType);

   /* Check Ramdac type is supported on the current S3 chipset */
   {
      char *chips = NULL;

      switch (s3RamdacType) {
      case BT485_DAC:
      case ATT20C505_DAC:
      case TI3020_DAC:
	 if (!S3_928_ONLY(s3ChipId) && !S3_964_SERIES(s3ChipId))
	    chips = "928 and 964 chips";
	 break;
      case TI3025_DAC:
	 if (!S3_964_SERIES(s3ChipId))
	    chips = "the 964 chip";
	 break;
      case ATT20C498_DAC:
      case STG1700_DAC:
      case S3_SDAC_DAC:
	 if (!S3_864_SERIES(s3ChipId) && !S3_805_I_SERIES(s3ChipId))
	    chips = "864 and 805i chips";
	 break;
      }
      if (chips) {
	 ErrorF("%s %s: Ramdac \"%s\" is only supported with %s\n",
		XCONFIG_PROBED, s3InfoRec.name, s3InfoRec.ramdac, chips);
	 OFLG_CLR(XCONFIG_RAMDAC, &s3InfoRec.xconfigFlag);
	 /* Treat the ramdac as a "normal" dac */
	 s3RamdacType = NORMAL_DAC;
	 s3InfoRec.ramdac = xf86TokenToString(s3DacTable, s3RamdacType);
      }
   }

   if (xf86Verbose) {
      ErrorF("%s %s: Ramdac type: %s\n",
	     OFLG_ISSET(XCONFIG_RAMDAC, &s3InfoRec.xconfigFlag) ?
	     XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name, s3InfoRec.ramdac);
   }

   /* Check that the depth requested is supported by the ramdac/chipset */
   {
      char *reason = NULL;

      if (S3_801_SERIES(s3ChipId)) {
	 if (s3Bpp > 2)
	    reason = "801 and 805 chips";
      }
      else if (S3_911_SERIES(s3ChipId)) {
	 if (s3Bpp > 2)
	    reason = "911 and 924 chips";
      }
      {
	 switch (s3RamdacType) {
	 case NORMAL_DAC:
	    if (s3Bpp > 1)
	       reason = "a \"normal\" RAMDAC";
	    break;
	 case ATT20C490_DAC:
	    /* XXXX Is this right (??) */
	    if (s3Bpp > 2)
	       reason = "an ATT20C490 RAMDAC";
	    break;
	 case BT485_DAC:
	 case ATT20C505_DAC:
	    if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
	       if (s3Bpp > 2)
		  reason = "Bt485 and ATT20C505 RAMDACs";
	    } else {
	       if (s3Bpp > 1)
		  reason = "Bt485 and ATT20C505 RAMDACs";
	    }
	    break;
	 case ATT20C498_DAC:
	 case STG1700_DAC:
	 case S3_SDAC_DAC:
	    break;
	 case SC15025_DAC:
	    break;
	 case TI3020_DAC:
	    if (s3Bpp > 1)
	       reason = "a TI3020 RAMDAC";
	    break;
	 case TI3025_DAC:
	    break;
	 default:
	    /* Should never get here */
	    if (s3Bpp > 1)
	       reason = "an unknown RAMDAC";
	    break;
	 }
      }
      if (reason) {
	 ErrorF("Depth %d is not supported with %s\n", s3InfoRec.depth, reason);
	 xf86DisableIOPorts(s3InfoRec.scrnIndex);
	 return(FALSE);
      }
   }

   /* Now check/set the DAC speed */
   /* XXXX Are these reasonable defaults? */
   if (s3InfoRec.dacSpeed <= 0) {
      switch (s3RamdacType) {
      case NORMAL_DAC:
      case ATT20C490_DAC:
	 s3InfoRec.dacSpeed = 110000;
	 break;
      case SC15025_DAC:
	 s3InfoRec.dacSpeed = 110000;
	 break;
      case BT485_DAC:
      case ATT20C505_DAC:
      case ATT20C498_DAC:
      case STG1700_DAC:
      case S3_SDAC_DAC:
	 s3InfoRec.dacSpeed = 135000;
	 break;
      case TI3020_DAC:
         if (OFLG_ISSET(OPTION_ELSA_W2000PRO, &s3InfoRec.options))
	    s3InfoRec.dacSpeed = 170000;
	 else
	    s3InfoRec.dacSpeed = 135000;
	 break;
      case TI3025_DAC:
	 s3InfoRec.dacSpeed = 135000;  /* push 135MHz part to 170+ ? */
	 break;
      }
   }
   
   if (xf86Verbose) {
      ErrorF("%s %s: Ramdac speed: %d\n",
	     OFLG_ISSET(XCONFIG_DACSPEED, &s3InfoRec.xconfigFlag) ?
	     XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name,
	     s3InfoRec.dacSpeed / 1000);
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

   if (DAC_IS_TI3020_SERIES) {
      if (OFLG_ISSET(OPTION_NO_TI3020_CURS, &s3InfoRec.options)) {
         ErrorF("%s %s: Use of Ti3020 cursor disabled in XF86Config\n",
	        XCONFIG_GIVEN, s3InfoRec.name);
	 OFLG_CLR(OPTION_TI3020_CURS, &s3InfoRec.options);
      } else {
	 /* use the ramdac cursor by default */
	 ErrorF("%s %s: Using hardware cursor from Ti3020/25 RAMDAC\n",
	        OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name);
	 OFLG_SET(OPTION_TI3020_CURS, &s3InfoRec.options);
      }
   } else {
      if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options)) {
	 ErrorF("%s %s: Ti3020 cursor requires a Ti3020/25 RAMDAC\n",
		XCONFIG_PROBED, s3InfoRec.name);
      }
   }

   /* Check when pixmux is supported */

   if (DAC_IS_BT485_SERIES &&
       (
	OFLG_ISSET(OPTION_STB_PEGASUS, &s3InfoRec.options) ||
	OFLG_ISSET(OPTION_NUMBER_NINE, &s3InfoRec.options) ||
	OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) ||
	S3_964_SERIES(s3ChipId)))
      s3Bt485PixMux = TRUE;

   if ((DAC_IS_ATT498 || DAC_IS_STG1700 || DAC_IS_SDAC) && 
       (S3_864_SERIES(s3ChipId) || S3_805_I_SERIES(s3ChipId)))
      if (xf86bpp <= 8) s3ATT498PixMux = TRUE;

   /* Set the pix-mux description based on the ramdac type */
   if (DAC_IS_TI3020_SERIES) {
      pixMuxPossible = TRUE;
      allowPixMuxInterlace = FALSE;
      allowPixMuxSwitching = FALSE;
      nonMuxMaxClock = 70000;
      if (S3_964_SERIES(s3ChipId)) {
         nonMuxMaxClock = 0;  /* 964 can only be in pixmux mode when */
         pixMuxMinWidth = 0;  /* working in enhanced mode */  
	 if (!OFLG_ISSET(OPTION_ELSA_W2000PRO, &s3InfoRec.options)) {
	    pixMuxLimitedWidths = FALSE;
	 }
      }
   } else if (s3ATT498PixMux) {
      pixMuxPossible = TRUE;
      nonMuxMaxClock = 67500;
      allowPixMuxInterlace = FALSE;
      allowPixMuxSwitching = TRUE;
      pixMuxLimitedWidths = FALSE;
      pixMuxMinWidth = 0;
   } else if (s3Bt485PixMux) {
      /* XXXX Are the defaults for the other parameters correct? */
      pixMuxPossible = TRUE;
      allowPixMuxInterlace = FALSE;	/* It doesn't work right (yet) */
      allowPixMuxSwitching = FALSE;	/* XXXX Is this right? */
      if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
	 nonMuxMaxClock = 67500;	/* Doubling only works in mux mode */
	 nonMuxMaxMemory = 1024;	/* Can't access more without mux */
	 allowPixMuxSwitching = FALSE;
	 pixMuxLimitedWidths = FALSE;
	 pixMuxMinWidth = 1024;
	 if (s3Bpp == 2) {
	    nonMuxMaxMemory = 0;	/* Only 2:1MUX works (yet)!     */
	    pixMuxMinWidth = 800;
	 } else if (s3Bpp==4) {
	    nonMuxMaxMemory = 2048;
	    pixMuxMinWidth = 640;
	 }
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
      } else if (S3_964_SERIES(s3ChipId)) {
         nonMuxMaxClock = 0;  /* 964 can only be in pixmux mode when */
         pixMuxMinWidth = 0;  /* working in enhanced mode */  
	 pixMuxLimitedWidths = FALSE;
      } else {
	 nonMuxMaxClock = 85000;
      }
   }

   /*
    * clock options are now done after the ramdacs because the next
    * generation ramdacs will have a built in clock (i.e. TI 3025)
    */

   if (DAC_IS_TI3025) {
      s3ClockSelectFunc = ti3025ClockSelect;
      OFLG_SET(CLOCK_OPTION_TI3025, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
      if (xf86Verbose)
	 ErrorF("%s %s: Using TI 3025 programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
	 numClocks = 3;
   } else if (OFLG_ISSET(OPTION_LEGEND, &s3InfoRec.options)) {
      s3ClockSelectFunc = LegendClockSelect;
      numClocks = 32;
   } else if (OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      if (xf86Verbose)
         ErrorF("%s %s: Using ICD2061A programmable clock\n",
            XCONFIG_GIVEN, s3InfoRec.name);
      numClocks = 3;
   } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      if (xf86Verbose)
	 ErrorF("%s %s: Using Sierra SC11412 programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
      numClocks = 3;
   } else if (OFLG_ISSET(CLOCK_OPTION_S3GENDAC, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = s3GendacClockSelect;
      if (xf86Verbose)
	 ErrorF("%s %s: Using S3 Gendac/SDAC programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
      numClocks = 3;
   } else if (OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
#ifdef ICS2595
      s3ClockSelectFunc = icd2061ClockSelect;
      if (xf86Verbose)
	 ErrorF("%s %s: Using ICS2595 programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
      numClocks = 3;
#else
      ErrorF("ICS2595 clock chip support is not yet included\n");
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return(FALSE);
#endif
   } else {
      s3ClockSelectFunc = s3ClockSelect;
      numClocks = 16;
      if (!s3InfoRec.clocks) 
         vgaGetClocks(numClocks, s3ClockSelectFunc);
   }

   /*
    * Set the maximum raw clock for programmable clock chips.
    * Setting maxRawClock to 0 means no clock-chip limit imposed.
    */
   if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
      if (OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions)) {
	 maxRawClock = 120000;
      } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
	 if (!OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
	    switch (s3RamdacType) {
	    case BT485_DAC:
	       maxRawClock = 67500;
	       break;
	    case ATT20C505_DAC:
	       maxRawClock = 90000;
	       break;
	    default:
	       maxRawClock = 100000;
	       break;
	    }
	 } else {
	    maxRawClock = 100000;
	 }
      } else if (OFLG_ISSET(CLOCK_OPTION_S3GENDAC, &s3InfoRec.clockOptions)) {
	 maxRawClock = 110000;
      } else if (OFLG_ISSET(CLOCK_OPTION_TI3025, &s3InfoRec.clockOptions)) {
	 maxRawClock = s3InfoRec.dacSpeed; /* Is this right?? */
#ifdef ICS2595
      } else if (OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
	 maxRawClock = 145000; /* This is what is in common_hw/ICS2595.h */
#endif
      } else {
	 /* Shouldn't get here */
	 maxRawClock = 0;
      }
   } else {
      maxRawClock = 0;
   }

   /*
    * Set pixel clock limit based on RAMDAC type/speed/bpp and pixmux usage.
    * Also scale maxRawClock so that it can be compared with a pixel clock,
    * and re-adjust the pixel clock limit if required.
    */

   switch (s3RamdacType) {
   case BT485_DAC:
      if (maxRawClock > 67500 && s3Bpp == 1)  /* XXXX is the s3Bpp bit right? */
	 clockDoublingPossible = TRUE;
      if (s3Bt485PixMux)
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;
      else
	 s3InfoRec.maxClock = 85000;
      break;
   case ATT20C505_DAC:
      if (maxRawClock > 90000 && s3Bpp == 1)  /* XXXX is the s3Bpp bit right? */
	 clockDoublingPossible = TRUE;
      if (s3Bt485PixMux)
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;
      else
	 s3InfoRec.maxClock = 85000;
      break;
      /* XXXX What happens for 16bpp and 32bpp?? */
      /* XXXX Include scaling of maxRawClock for 16bpp and 32bpp */
   case ATT20C498_DAC:
   case STG1700_DAC:
   case S3_SDAC_DAC:
      if (s3ATT498PixMux) {
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;
	 if (s3Bpp == 1)	/* XXXX is this right?? */
	    clockDoublingPossible = TRUE;
      }
      else {
	 if (s3InfoRec.dacSpeed >= 135000) /* 20C498 -13, -15, -17 */
	    s3InfoRec.maxClock = 110000;
	 else				   /* 20C498 -11 */
	    s3InfoRec.maxClock = 80000;
	 /* Halve it for 32bpp */
	 if (s3Bpp == 4) {
	    s3InfoRec.maxClock /= 2;
	    maxRawClock /= 2;
	 }
      }
      break;
   case TI3020_DAC:
   case TI3025_DAC:
      if (s3Bpp == 1)	/* XXXX is this right?? */
	 clockDoublingPossible = TRUE;
      s3InfoRec.maxClock = s3InfoRec.dacSpeed;
      break;
      /* XXXX What happens for 16bpp and 32bpp?? */
      /* XXXX Include scaling of maxRawClock for 16bpp and 32bpp */
   case ATT20C490_DAC:
      s3InfoRec.maxClock = s3InfoRec.dacSpeed;
      /* Halve it for 16bpp (32bpp not supported) */
      if (s3Bpp > 1)
	 s3InfoRec.maxClock /= 2;
	 maxRawClock /= 2;
      break;
   case SC15025_DAC:
      {
	 int doubleEdgeLimit;
	 if (s3InfoRec.dacSpeed >= 125000)	/* -125 */
	    doubleEdgeLimit = 85000;
	 else if (s3InfoRec.dacSpeed >= 110000)	/* -110 */
	    doubleEdgeLimit = 65000;
	 else					/* -80, -66 */
	    doubleEdgeLimit = 50000;
	 switch (s3Bpp) {
	 case 1:
	    s3InfoRec.maxClock = s3InfoRec.dacSpeed;
	    break;
	 case 2:
	    s3InfoRec.maxClock = doubleEdgeLimit;
	    maxRawClock /= 2;
	    break;
	 case 4:
	    s3InfoRec.maxClock = doubleEdgeLimit / 2;
	    maxRawClock /= 4;
	    break;
	 }
      }
      break;
   default:
      /* For DACs we don't have special code for, keep this as a limit */
      s3InfoRec.maxClock = s3MaxClock;
   }
   /* Check that maxClock is not higher than dacSpeed */
   if (s3InfoRec.maxClock > s3InfoRec.dacSpeed)
      s3InfoRec.maxClock = s3InfoRec.dacSpeed;

   /* Check if this exceeds the clock chip's limit */
   if (clockDoublingPossible)
      maxRawClock *= 2;
   if (maxRawClock > 0 && s3InfoRec.maxClock > maxRawClock)
      s3InfoRec.maxClock = maxRawClock;

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

   /* Adjust s3InfoRec.clock[] when not using a programable clock chip */

   if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
      Bool clocksChanged = FALSE;

      for (j = 0; j < s3InfoRec.clocks; j++) {
	 switch(s3RamdacType) {
	 case NORMAL_DAC:
	    /* only suports 8bpp -- nothing to do */
	    break;
	 case BT485_DAC:
	 case ATT20C505_DAC:
	    /* XXXX What happens here for 16bpp/32bpp ? */
	    break;
	 case TI3020_DAC:
	 case TI3025_DAC:
	    /* XXXX What happens here for 16bpp/32bpp ? */
	    break;
	 case ATT20C498_DAC:
	 case STG1700_DAC:	/* XXXX should this be here? */
	 case S3_SDAC_DAC:	/* XXXX should this be here? */
	    switch (s3Bpp) {
	    case 1:
	       /*
	        * This one depend on pixel multiplexing for 8bpp.
	        * Although existing code implies it depends on ramdac
	        * clock doubling instead (are the two tied together?)
	        * Hopefully no 498s are used with non-programable clocks
	        */
	       break;
	    case 2:
	       /* No change for 16bpp */
	       break;
	    case 4:
	       s3InfoRec.clock[j] /= 2;
	       clocksChanged = TRUE;
	       break;
	    }
	    break;
	 case ATT20C490_DAC:
	    if (s3Bpp > 1) {
	       s3InfoRec.clock[j] /= s3Bpp;
	       clocksChanged = TRUE;
	    }
	    break;
	 case SC15025_DAC:
	    if (s3Bpp > 1) {
	       s3InfoRec.clock[j] /= s3Bpp;
	       clocksChanged = TRUE;
	    }
	    break;
	 default:
	    /* Do nothing */
	    break;
	 }
      }
      if (xf86Verbose && clocksChanged) {
	 ErrorF("%s %s: Effective pixel clocks available for depth %d:\n",
		XCONFIG_PROBED, s3InfoRec.name, s3InfoRec.depth);
	 for (j = 0; j < s3InfoRec.clocks; j++) {
	    if ((j % 8) == 0) {
	       if (j != 0)
		  ErrorF("\n");
               ErrorF("%s %s: pixel clocks:", XCONFIG_PROBED, s3InfoRec.name);
	    }
	    ErrorF(" %6.2f", (double)s3InfoRec.clock[j] / 1000.0);
         }
         ErrorF("\n");
      }
   }

   /* At this point, the s3InfoRec.clock[] values are pixel clocks */

   tx = s3InfoRec.virtualX;
   ty = s3InfoRec.virtualY;
   pMode = s3InfoRec.modes;
   if (pMode == NULL) {
      ErrorF("No modes supplied in XF86Config\n");
      xf86DisableIOPorts(s3InfoRec.scrnIndex);
      return (FALSE);
   }
   pEnd = NULL;
   do {
      DisplayModePtr pModeSv;

      pModeSv = pMode->next;
      /*
       * xf86LookupMode returns FALSE if it ran into an invalid
       * parameter 
       */
      if (!xf86LookupMode(pMode, &s3InfoRec)) {
	 xf86DeleteMode(&s3InfoRec, pMode);
      } else if ((pMode->HDisplay * (1 + pMode->VDisplay) * s3Bpp) >
		 s3InfoRec.videoRam * 1024) {
	 ErrorF("%s %s: Too little memory for mode \"%s\"\n", XCONFIG_PROBED,
		s3InfoRec.name, pMode->name);
	 ErrorF("%s %s: NB. 1 scan line is required for the hardware cursor\n",
	        XCONFIG_PROBED, s3InfoRec.name);
	 xf86DeleteMode(&s3InfoRec, pMode);
      } else if (((tx > 0) && (pMode->HDisplay > tx)) ||
		 ((ty > 0) && (pMode->VDisplay > ty))) {
	 ErrorF("%s %s: Resolution %dx%d too large for virtual %dx%d\n",
		XCONFIG_PROBED, s3InfoRec.name,
		pMode->HDisplay, pMode->VDisplay, tx, ty);
	 xf86DeleteMode(&s3InfoRec, pMode);
      } else {
	 /*
	  * Successfully looked up this mode.  If pEnd isn't
	  * initialized, set it to this mode.
	  */
	 if (pEnd == (DisplayModePtr) NULL)
	    pEnd = pMode;

	 s3InfoRec.virtualX = max(s3InfoRec.virtualX, pMode->HDisplay);
	 s3InfoRec.virtualY = max(s3InfoRec.virtualY, pMode->VDisplay);

	 /*
	  * Check what impact each mode has on pixel multiplexing,
	  * and mark those modes for which pixmux must be used.
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
	     *      which require mux for one feature, but can't use it
	     *      because of another is flagged.
	     */
	    if (!allowPixMuxSwitching || (pMode->Flags & V_PIXMUX)) {
	       if (pMode->HDisplay < pixMuxMinWidth)
		  pixMuxWidthOK = FALSE;
	       if ((pMode->Flags & V_INTERLACE) && !allowPixMuxInterlace)
		  pixMuxInterlaceOK = FALSE;
	    }
	 }
      }
      pMode = pModeSv;
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
	    ErrorF("your XF86Config to meet these requirements\n");
	 } else {
	    ErrorF("Modes with a dot-clock above %dMHz require the RAMDAC to\n",
		   nonMuxMaxClock / 1000);
	    ErrorF("operate in pixel multiplex mode, but pixel multiplexing\n");
	    ErrorF("cannot be used for modes with width less than %d.\n",
		   pixMuxMinWidth);
	    ErrorF("Adjust the Modes line in your XF86Config to meet these ");
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
	    ErrorF("your XF86Config to meet these requirements\n");
	 } else {
	    ErrorF("Modes with a dot-clock above %dMHz require the RAMDAC to\n",
		   nonMuxMaxClock / 1000);
	    ErrorF("operate in pixel multiplex mode, but pixel multiplexing\n");
	    ErrorF("cannot be used for interlaced modes.\n");
	    ErrorF("Adjust the Modes line in your XF86Config to meet these ");
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

   /* pixmux on Bt485 requires use of Bt's cursor */
   if (s3Bt485PixMux && s3UsingPixMux &&
       !OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options)) {
      OFLG_SET(OPTION_BT485_CURS, &s3InfoRec.options);
      ErrorF("%s %s: Using hardware cursor from Bt485/20C505 RAMDAC\n",
	     XCONFIG_PROBED, s3InfoRec.name);
   }

   if (s3UsingPixMux && !allowPixMuxSwitching) {
      /* Mark all modes as V_PIXMUX */
      pEnd = pMode = s3InfoRec.modes;
      do {
	 pMode->Flags |= V_PIXMUX;
         pMode = pMode->next;
      } while (pMode != pEnd);
   }

   /*
    * For programmable clocks, fill in the SynthClock value
    * and set V_DBLCLK as required for each mode
    */

   if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
      /* First just copy the pixel values */
      pEnd = pMode = s3InfoRec.modes;
      do {
	 pMode->SynthClock = s3InfoRec.clock[pMode->Clock];
	 pMode = pMode->next;
      } while (pMode != pEnd);
      /* Now make adjustments */
      pEnd = pMode = s3InfoRec.modes;
      do {
	 switch(s3RamdacType) {
	 case NORMAL_DAC:
	    /* only suports 8bpp -- nothing to do */
	    break;
	 case BT485_DAC:
	    if (pMode->SynthClock > 67500) {
	       pMode->SynthClock /= 2;
	       pMode->Flags |= V_DBLCLK;
	    }
	    /* XXXX What happens here for 16bpp/32bpp ? */
	    break;
	 case ATT20C505_DAC:
	    if (pMode->SynthClock > 90000) {
	       pMode->SynthClock /= 2;
	       pMode->Flags |= V_DBLCLK;
	    }
	    /* XXXX What happens here for 16bpp/32bpp ? */
	    break;
	 case TI3020_DAC:
	    if (pMode->SynthClock > 100000) {
	       pMode->SynthClock /= 2;
	       pMode->Flags |= V_DBLCLK;
	    }
	    /* XXXX What happens here for 16bpp/32bpp ? */
	    break;
	 case TI3025_DAC:
	    if (pMode->SynthClock > 80000) {
               /* the SynthClock will be divided and clock doubled by the PLL */
	       pMode->Flags |= V_DBLCLK;
	    }
	    /* XXXX What happens here for 16bpp/32bpp ? */
	    break;
	 case ATT20C498_DAC:
	 case STG1700_DAC:	/* XXXX should this be here? */
	 case S3_SDAC_DAC:	/* XXXX should this be here? */
	    switch (s3Bpp) {
	    case 1:
	       /*
	        * This one depend on pixel multiplexing for 8bpp.
	        * Although existing code implies it depends on ramdac
	        * clock doubling instead (are the two tied together?)
	        * We'll act based on clock doubling changeover at 67500
	        */
	       if (pMode->SynthClock > 67500) {
		  pMode->SynthClock /= 2;
		  pMode->Flags |= V_DBLCLK;
	       }
	       break;
	    case 2:
	       /* No change for 16bpp */
	       break;
	    case 4:
	       pMode->SynthClock *= 2;
	       break;
	    }
	    break;
	 case ATT20C490_DAC:
	    if (s3Bpp > 1) {
	       pMode->SynthClock *= s3Bpp;
	    }
	    break;
	 case SC15025_DAC:
	    if (s3Bpp > 1) {
	       pMode->SynthClock *= s3Bpp;
	    }
	    break;
	 default:
	    /* Do nothing */
	    break;
	 }
	 pMode = pMode->next;
      } while (pMode != pEnd);
   }
   if (DAC_IS_BT485_SERIES || DAC_IS_TI3020_SERIES) {
      if (OFLG_ISSET(OPTION_DAC_8_BIT, &s3InfoRec.options))
	 s3DAC8Bit = TRUE;
      if (OFLG_ISSET(OPTION_SYNC_ON_GREEN, &s3InfoRec.options)) {
	 s3DACSyncOnGreen = TRUE;
	 if (xf86Verbose)
	    ErrorF("%s %s: Putting RAMDAC into sync-on-green mode\n",
		   XCONFIG_GIVEN, s3InfoRec.name);
      }
   }

   if (DAC_IS_ATT490 || DAC_IS_SC15025 || DAC_IS_ATT498 || DAC_IS_STG1700) {
      if (OFLG_ISSET(OPTION_DAC_8_BIT, &s3InfoRec.options))
         s3DAC8Bit = TRUE;
   }

   if (s3InfoRec.bitsPerPixel == 8 && s3DAC8Bit && xf86Verbose)
      ErrorF("%s %s: Putting RAMDAC into 8-bit mode\n",
         XCONFIG_GIVEN, s3InfoRec.name);

   if (xf86Verbose) {
      if (s3InfoRec.bitsPerPixel == 8)
	 ErrorF("%s %s: Using %d bits per RGB value\n",
		XCONFIG_PROBED, s3InfoRec.name,
		s3DAC8Bit ?  8 : 6);
      else if (s3InfoRec.bitsPerPixel == 16)
	 ErrorF("%s %s: Using 16 bpp.  Color weight: %1d%1d%1d\n",
		XCONFIG_GIVEN, s3InfoRec.name, xf86weight.red,
		xf86weight.green, xf86weight.blue);
      else if (s3InfoRec.bitsPerPixel == 32)
	 ErrorF("%s %s: Using sparse 32 bpp.  Color weight: %1d%1d%1d\n",
		XCONFIG_GIVEN, s3InfoRec.name, xf86weight.red,
		xf86weight.green, xf86weight.blue);
   }

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
		 (   S3_801_REV_C(s3ChipId) 
                  || S3_805_I_SERIES(s3ChipId)
		  || S3_928_REV_E(s3ChipId)
		  || S3_x64_SERIES(s3ChipId))) {
	 s3DisplayWidth = 1152;
      } else if (s3InfoRec.virtualX <= 1280) {
	 s3DisplayWidth = 1280;
      } else if ((s3InfoRec.virtualX <= 1600) && 
		 (   S3_928_REV_E(s3ChipId)
		  || S3_x64_SERIES(s3ChipId))) {
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
   s3BppDisplayWidth = s3Bpp * s3DisplayWidth;
      
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
      int st_addr = (s3InfoRec.virtualY * s3BppDisplayWidth + 1023) & ~1023;
      s3CursorStartX = st_addr % s3BppDisplayWidth;
      s3CursorStartY = st_addr / s3BppDisplayWidth;
      s3CursorLines = ((s3CursorStartX + 1023) / s3BppDisplayWidth) + 1;
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
  
   if ((s3BppDisplayWidth * (s3CursorStartY + s3CursorLines)) >
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

   if (OFLG_ISSET(OPTION_PCI_HACK, &s3InfoRec.options))
      s3PCIHack = TRUE;
   if (OFLG_ISSET(OPTION_POWER_SAVER, &s3InfoRec.options))
      s3PowerSaver = TRUE;

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
icd2061ClockSelect(freq)
     int   freq;
{
   Bool result = TRUE;

   UNLOCK_SYS_REGS;
   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = s3ClockSelect(freq);
      break;
   default:
      {
	 /* Convert freq to Hz */
	 freq *= 1000;
	 /* Use the "Alt" version always since it is more reliable */
	 if (OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions)) {
	    /* setting exactly 120 MHz doesn't work all the time */
	    if (freq > 119900000) freq = 119900000;
	    AltICD2061SetClock(freq, 2);
	    AltICD2061SetClock(freq, 2);
	    AltICD2061SetClock(freq, 2);
	 } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
	    result = SC11412SetClock((long)freq/1000);
#ifdef ICS2595
	 } else if (OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
	    result = ICS2595SetClock((long)freq/1000);
	    result = ICS2595SetClock((long)freq/1000);
	    result = ICS2595SetClock((long)freq/1000);
#endif
	 } else { /* Should never get here */
	    result = FALSE;
	    break;
	 }
	 if (!OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
	    outb(vgaCRIndex, 0x42);/* select the clock */
	    outb(vgaCRReg, 0x02);
	 }
	 usleep(150000);
	 /* Do the clock doubler selection in s3Init() */
      }
   }
   LOCK_SYS_REGS;
   return(result);
}


/* The GENDAC code also works for the SDAC */

static Bool
s3GendacClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
 
   UNLOCK_SYS_REGS;
   
   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = s3ClockSelect(freq);
      break;
   default:
      {
	 (void) S3gendacSetClock(freq, 2); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 outb(vgaCRReg, 0x02);
	 usleep(150000);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}


static Bool
ti3025ClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
 
   UNLOCK_SYS_REGS;
   
   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = s3ClockSelect(freq);
      break;
   default:
      {
	 /* Check if clock frequency is within range */
	 /* XXXX Check this elsewhere */
	 if (freq < 20000) {
	    ErrorF("%s %s: Specified dot clock (%.3f) too low for TI 3025",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    result = FALSE;
	    break;
	 }
	 (void) Ti3025SetClock(freq, 2); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 outb(vgaCRReg, 0x02);
	 usleep(150000);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}
