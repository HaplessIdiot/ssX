/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3ramdacs.c,v 3.2 1996/12/12 09:15:43 dawes Exp $ */
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
 * Rather severely reorganized by MArk Vojkovich (mvojkovi@ucsd.edu)
 * 
 */

#include "misc.h"
#include "cfb.h"
#include "pixmapstr.h"
#include "fontstruct.h"
#include "s3.h"
#include "regs3.h"
#include "xf86_HWlib.h"
#include "xf86_PCI.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "s3linear.h"
#include "s3Bt485.h"
#include "Ti302X.h"
#include "IBMRGB.h"
#include "s3ELSA.h"
#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

extern char *clockchip_probed;		/* in s3.c */
extern Bool pixMuxPossible;
extern Bool allowPixMuxInterlace;
extern Bool allowPixMuxSwitching;
extern int nonMuxMaxClock;
extern int nonMuxMaxMemory;
extern int pixMuxMinWidth;
extern int pixMuxMinClock;
extern Bool pixMuxLimitedWidths;
extern Bool clockDoublingPossible;
extern Bool pixMuxNeeded;
extern int s3BiosVendor;
extern int maxRawClock;
extern int numClocks;
extern int s3MaxClock;
extern unsigned char *find_bios_string(int, char *, char *);


static Bool NORMAL_Probe();
static Bool S3_TRIO32_Probe();
static Bool S3_TRIO64_Probe();
static Bool TI3026_Probe();
static Bool TI3030_Probe();
static Bool TI3020_Probe();
static Bool TI3025_Probe();
static Bool BT485_Probe();
static Bool ATT20C505_Probe();
static Bool ATT22C498_Probe();
static Bool ATT498_Probe();
static Bool ATT20C409_Probe();
static Bool SC15025_Probe();
static Bool STG1700_Probe();
static Bool STG1703_Probe();
static Bool IBMRGB524_Probe();
static Bool IBMRGB525_Probe();
static Bool IBMRGB528_Probe();
static Bool S3_SDAC_Probe();
static Bool S3_GENDAC_Probe();
static Bool ATT20C490_Probe();
static Bool SS2410_Probe();
static Bool SC1148x_M2_Probe();
static Bool Null_Probe() {return FALSE;}

static int BT485_SERIES_PreInit();
static int TI3020_3025_PreInit();
static int ATT409_498_PreInit();
static int SC15025_PreInit();
static int STG17xx_PreInit();
static int S3_SDAC_GENDAC_PreInit();
static int S3_TRIO_PreInit();
static int TI3030_3026_PreInit();
static int IBMRGB52x_PreInit();
static int MISC_HI_COLOR_PreInit();
static int NORMAL_PreInit();
static int Null_PreInit() {return 0;}


static void Null_Restore(){}
static void S3_TRIO_Restore();
static void TI3030_3026_Restore();
static void TI3020_3025_Restore();
static void BT485_Restore();
static void ATT409_498_Restore();
static void SC15025_Restore();
static void STG17xx_Restore();
static void IBMRGB52x_Restore();
static void S3_SDAC_GENDAC_Restore();
static void SC1148x_M2_Restore();
static void SS2410_Restore();
static void ATT20C490_Restore();

static void Null_Save(){}
static void S3_TRIO_Save();
static void TI3030_3026_Save();
static void TI3020_3025_Save();
static void BT485_Save();
static void ATT409_498_Save();
static void SC15025_Save();
static void STG17xx_Save();
static void IBMRGB52x_Save();
static void S3_SDAC_GENDAC_Save();
static void SC1148x_M2_Save();
static void SS2410_Save();
static void ATT20C490_Save();


Bool  (*s3ClockSelectFunc) ();
static Bool LegendClockSelect();
static Bool s3ClockSelect();
static Bool icd2061ClockSelect();
static Bool s3GendacClockSelect();
static Bool ti3025ClockSelect();
static Bool ti3026ClockSelect();
static Bool IBMRGBClockSelect();
static void s3ProgramTi3025Clock(
#if NeedFunctionPrototypes
	int clk,
	unsigned char n,
	unsigned char m,
	unsigned char p
#endif
);
static Bool ch8391ClockSelect();
static Bool att409ClockSelect();
static Bool STG1703ClockSelect();
static Bool Gloria8ClockSelect();

static unsigned char s3DacRegs[0x101];

/* NOTE:  This order must be the same as the #define order in
	s3.h !!!!!!!! */
s3RamdacInfo s3Ramdacs[] = {
	   /*   DacName,  DacSpeed, DacProbe(), PreInit(), DacRestore(),
			DacSave() */
/* 0 */		{"normal", 110000, NORMAL_Probe, NORMAL_PreInit,Null_Restore,
			Null_Save},
/* 1 */		{"s3_trio32", 135000, S3_TRIO32_Probe, S3_TRIO_PreInit, 	
			S3_TRIO_Restore,S3_TRIO_Save},
/* 2 */		{"s3_trio64", 135000, S3_TRIO64_Probe, S3_TRIO_PreInit, 	
			S3_TRIO_Restore,S3_TRIO_Save},
/* 3 */		{"ti3026", 135000, TI3026_Probe, TI3030_3026_PreInit,
			TI3030_3026_Restore,TI3030_3026_Save},
/* 4 */		{"ti3030", 175000, TI3030_Probe, TI3030_3026_PreInit,
			TI3030_3026_Restore,TI3030_3026_Save},
/* 5 */		{"ti3020", 135000, TI3020_Probe, TI3020_3025_PreInit, 
			TI3020_3025_Restore,TI3020_3025_Save},
/* 6 */		{"ti3025", 135000, TI3025_Probe, TI3020_3025_PreInit,
			TI3020_3025_Restore,TI3020_3025_Save},
/* 7 */		{"Bt485", 135000, BT485_Probe, BT485_SERIES_PreInit,
			BT485_Restore,BT485_Save},
/* 8 */		{"att20c505", 135000, ATT20C505_Probe, BT485_SERIES_PreInit,
			BT485_Restore,BT485_Save},
/* 9 */		{"att22c498", 135000, ATT22C498_Probe, ATT409_498_PreInit,
			ATT409_498_Restore,ATT409_498_Save},
/* 10 */	{"att20c498", 135000, ATT498_Probe, ATT409_498_PreInit,
			ATT409_498_Restore,ATT409_498_Save},
/* 11 */	{"att20c409", 135000, ATT20C409_Probe, ATT409_498_PreInit,
			ATT409_498_Restore,ATT409_498_Save},
/* 12 */	{"sc15025", 110000, SC15025_Probe, SC15025_PreInit,
			SC15025_Restore,SC15025_Save},
/* 13 */	{"stg1700", 135000, STG1700_Probe, STG17xx_PreInit,
			STG17xx_Restore,STG17xx_Save},
/* 14 */	{"stg1703", 135000, STG1703_Probe, STG17xx_PreInit,
			STG17xx_Restore,STG17xx_Save},
/* 15 */	{"ibm_rgb524", 170000, IBMRGB524_Probe, IBMRGB52x_PreInit,
			IBMRGB52x_Restore,IBMRGB52x_Save},
/* 16 */	{"ibm_rgb525", 170000, IBMRGB525_Probe, IBMRGB52x_PreInit,
			IBMRGB52x_Restore,IBMRGB52x_Save},
/* 17 */	{"ibm_rgb528", 170000, IBMRGB528_Probe, IBMRGB52x_PreInit,
			IBMRGB52x_Restore,IBMRGB52x_Save},
/* 18 */	{"s3_sdac", 135000, S3_SDAC_Probe, S3_SDAC_GENDAC_PreInit,
			S3_SDAC_GENDAC_Restore,S3_SDAC_GENDAC_Save},
/* 19 */	{"s3_gendac", 110000, S3_GENDAC_Probe, S3_SDAC_GENDAC_PreInit,
			S3_SDAC_GENDAC_Restore,S3_SDAC_GENDAC_Save},
/* 20 */	{"att20c490", 110000, ATT20C490_Probe, MISC_HI_COLOR_PreInit,
			ATT20C490_Restore,ATT20C490_Save},
/* 21 */	{"ss2410", 110000, SS2410_Probe, MISC_HI_COLOR_PreInit,
			SS2410_Restore,SS2410_Save},
/* 22 */	{"sc1148x", 110000, SC1148x_M2_Probe, MISC_HI_COLOR_PreInit,
			SC1148x_M2_Restore,SC1148x_M2_Save},
/* 23 */	{NULL, 0, Null_Probe, Null_PreInit,Null_Restore,Null_Save},
}; 

#if 0

static Bool TEMPLATE_PreInit()
{
    /* Verify that depth is supported by ramdac */

    /* Set cursor options */
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */

    /* Make any necessary clock alterations due to multiplexing,
	clock doubling, etc...  s3Probe will do some last minute
	clock sanity checks when we return */

	return 1;
}

#endif

static void OtherClocksSetup();


/*************************************************************\
			
			BT485_DAC

\*************************************************************/

static Bool BT485_Probe()
{	
   /*
    * Probe for the bloody thing.  Set 0x3C6 to a bogus value, then
    * try to get the Bt485 status register.  If it's there, then we will
    * get something else back from this port.
    */

    Bool found = FALSE;
    unsigned char tmp,tmp2;

    /*quick check*/
    if (!S3_928_ONLY(s3ChipId) && !S3_964_SERIES(s3ChipId) && 
			!S3_968_SERIES(s3ChipId))
	return FALSE;

    tmp = inb(0x3C6);
    outb(0x3C6, 0x0F);
    if (((tmp2 = s3InBtStatReg()) & 0x80) == 0x80) {
          /*
           * Found either a BrookTree Bt485 or AT&T 20C505.
           */
          if ((tmp2 & 0xF0) != 0xD0) {
             found = TRUE;
             ErrorF("%s %s: Detected a BrookTree Bt485 RAMDAC\n",
                    XCONFIG_PROBED, s3InfoRec.name);

             /* If it is a Bt485 and no clockchip is specified in the 
                XF86Config, set clockchips for SPEA Mercury / Mercury P64 */

             if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions))
              if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
               if (S3_964_SERIES(s3ChipId)) {
                   OFLG_SET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions);
                   OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
                   clockchip_probed = XCONFIG_PROBED;
               } else if (S3_928_ONLY(s3ChipId)) {
                   OFLG_SET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions);
                   OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
                   clockchip_probed = XCONFIG_PROBED;
               }
              }    
          }
     }
     outb(0x3C6, tmp);
     return found;
}


static int BT485_SERIES_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options)) {
	 ErrorF("%s %s: Using hardware cursor from Bt485/20C505 RAMDAC\n",
		XCONFIG_GIVEN, s3InfoRec.name);
   }
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
   if ( OFLG_ISSET(OPTION_STB_PEGASUS, &s3InfoRec.options) ||
	OFLG_ISSET(OPTION_NUMBER_NINE, &s3InfoRec.options) ||
	OFLG_ISSET(OPTION_MIRO_MAGIC_S4, &s3InfoRec.options) ||
	OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) ||
	S3_964_SERIES(s3ChipId) || S3_968_SERIES(s3ChipId)) {
      s3Bt485PixMux = TRUE;
      /* XXXX Are the defaults for the other parameters correct? */
      pixMuxPossible = TRUE;
      allowPixMuxInterlace = FALSE;	/* It doesn't work right (yet) */
      allowPixMuxSwitching = FALSE;	/* XXXX Is this right? */
      if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) &&
          S3_928_ONLY(s3ChipId)) {
	 nonMuxMaxClock = 67500;	/* Doubling only works in mux mode */
	 nonMuxMaxMemory = 1024;	/* Can't access more without mux */
	 allowPixMuxSwitching = FALSE;
	 pixMuxLimitedWidths = FALSE;
	 pixMuxMinWidth = 1024;
	 if (s3Bpp == 2) {
	    nonMuxMaxMemory = 0;	/* Only 2:1MUX works (yet)!     */
	    pixMuxMinWidth = 800;
	 } else if (s3Bpp==4) {
	    nonMuxMaxMemory = 0;
	    pixMuxMinWidth = 640;
	 }
      } else if (OFLG_ISSET(OPTION_NUMBER_NINE, &s3InfoRec.options)) {
	 nonMuxMaxClock = 67500;
	 allowPixMuxSwitching = TRUE;
	 pixMuxLimitedWidths = TRUE;
	 pixMuxMinWidth = 800;
      } else if (OFLG_ISSET(OPTION_STB_PEGASUS, &s3InfoRec.options)) {
	allowPixMuxSwitching = TRUE;
	pixMuxLimitedWidths = TRUE;
	/* For 8bpp mode, allow PIXMUX selection based on Clock and Width. */
	if (s3Bpp == 1) {
	  nonMuxMaxClock = 85000;
	  pixMuxMinWidth = 1024;
	} else {
	  /* For 16bpp and 32bpp modes, require PIXMUX. */
	  nonMuxMaxClock = 0;
	  pixMuxMinWidth = 0;
	}
      } else if (OFLG_ISSET(OPTION_MIRO_MAGIC_S4, &s3InfoRec.options)) {
	allowPixMuxSwitching = FALSE;
	pixMuxLimitedWidths = TRUE;
 	/* For 8bpp mode, allow PIXMUX selection based on Clock and Width. */
 	if (s3Bpp == 1) {
 	  nonMuxMaxClock = 85000;
 	  pixMuxMinWidth = 1024;
 	} else {
	  /* For 16bpp and 32bpp modes, require PIXMUX. */
	  nonMuxMaxClock = 0;
	  pixMuxMinWidth = 0;
 	}
      } else if (S3_964_SERIES(s3ChipId) || S3_968_SERIES(s3ChipId)) {
         nonMuxMaxClock = 0;  /* 964 can only be in pixmux mode when */
         pixMuxMinWidth = 0;  /* working in enhanced mode */  
	 pixMuxLimitedWidths = FALSE;
	 pixMuxNeeded = TRUE;
      } else {
	 nonMuxMaxClock = 85000;
      }

   }


    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */

   /* Diamond Stealth 64 VRAM uses an ICD2061A */
   if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions) &&
       (s3BiosVendor == DIAMOND_BIOS) && S3_964_ONLY(s3ChipId)) {
      OFLG_SET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
      clockchip_probed = XCONFIG_PROBED;
   }

   OtherClocksSetup();	/* setup clock */

    /* Make any necessary clock alterations due to multiplexing,
	clock doubling, etc...  s3Probe will do some last minute
	clock sanity checks when we return */

   if(s3RamdacType == BT485_DAC) {
      if (maxRawClock > 67500)
	 clockDoublingPossible = TRUE;
      /* These limits are based on the LCLK rating, and may be too high */
      if (s3Bt485PixMux && s3Bpp < 4)
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;
      else {
	 if (s3InfoRec.dacSpeed < 150000)    /* 110 and 135 */
	    s3InfoRec.maxClock = 90000;
	 else				      /* 150 and 170 (if they exist) */
	    s3InfoRec.maxClock = 110000;
      }
   } else { /* ATT20C505_DAC */
      if (maxRawClock > 90000)
	 clockDoublingPossible = TRUE;
      /* These limits are based on the LCLK rating, and may be too high */
      if (s3Bt485PixMux && s3Bpp < 4)
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;
      else {
	 if (s3InfoRec.dacSpeed < 110000)	  /* 85 */
	    s3InfoRec.maxClock = 85000;
	 else if (s3InfoRec.dacSpeed < 135000)	  /* 110 */
	    s3InfoRec.maxClock = 90000;
	 else					  /* 135, 150, 170 */
	    s3InfoRec.maxClock = 110000;
      }
   }

   return 1;
}

static void BT485_Restore()
{
      unsigned char tmp;

      /* Turn off parallel mode explicitly here */
      if (s3Bt485PixMux) {
         if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) &&
             S3_928_ONLY(s3ChipId))
	 {
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x20);
	    outb(0x3C7, 0x00);
	    /* set s3 reg53 to non-parallel addressing by and'ing 0xDF     */
            outb(vgaCRIndex, 0x53);
            tmp = inb(vgaCRReg);
            outb(vgaCRReg, tmp & 0xDF);
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x00);
         }
         if (OFLG_ISSET(OPTION_MIRO_MAGIC_S4, &s3InfoRec.options) &&
             S3_928_ONLY(s3ChipId))
	 {
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x40); /* XXXXXXXXXXXXXXXXXXXXX */
	    outb(0x3C7, 0x00);
	    /* set s3 reg53 to non-parallel addressing by and'ing 0xDF     */
            outb(vgaCRIndex, 0x53);
            tmp = inb(vgaCRReg);
            outb(vgaCRReg, tmp & 0xDF);
         }
      }
	 
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01);
      s3OutBtRegCom3(0x00, s3DacRegs[3]);
      if (s3Bt485PixMux) {
	 s3OutBtReg(BT_COMMAND_REG_2, 0x00, s3DacRegs[2]);
	 s3OutBtReg(BT_COMMAND_REG_1, 0x00, s3DacRegs[1]);
      }
      s3OutBtReg(BT_COMMAND_REG_0, 0x00, s3DacRegs[0]);
 }

static void BT485_Save()
{         
	 s3DacRegs[0] = s3InBtReg(BT_COMMAND_REG_0);
	 if (s3Bt485PixMux) {
	    s3DacRegs[1] = s3InBtReg(BT_COMMAND_REG_1);
	    s3DacRegs[2] = s3InBtReg(BT_COMMAND_REG_2);
	 }
	 s3DacRegs[3] = s3InBtRegCom3();
}

/************************************************************\

			ATT20C505_DAC

\************************************************************/

static Bool ATT20C505_Probe()
{
    Bool found = FALSE;
    unsigned char tmp,tmp2;

    /*quick check*/
    if (!S3_928_ONLY(s3ChipId) && !S3_964_SERIES(s3ChipId))
	return FALSE;

    tmp = inb(0x3C6);
    outb(0x3C6, 0x0F);
    if (((tmp2 = s3InBtStatReg()) & 0x80) == 0x80) {
          /*
           * Found either a BrookTree Bt485 or AT&T 20C505.
           */
          if ((tmp2 & 0xF0) == 0xD0) {
             found = TRUE;
             ErrorF("%s %s: Detected an AT&T 20C505 RAMDAC\n",
                    XCONFIG_PROBED, s3InfoRec.name);
          }
    }
    outb(0x3C6, tmp);
    return found;
}


/*******************************************************\
 
		TI3020_DAC  
		TI3025_DAC 

\*******************************************************/

static Bool TI3020_3025_Probe(int type)
{
    int found = 0;
    unsigned char saveCR55, saveCR45, saveCR43, saveCR5C;
    unsigned char saveTIndx,saveTIndx2,saveTIdata;

    outb(vgaCRIndex, 0x43);
    saveCR43 = inb(vgaCRReg);
    outb(vgaCRReg, saveCR43 & ~0x02);

    outb(vgaCRIndex, 0x45);
    saveCR45 = inb(vgaCRReg);
    outb(vgaCRReg, saveCR45 & ~0x20);

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
	    found = TI3020_DAC;
	    saveCR43 &= ~0x02;
	    saveCR45 &= ~0x20;
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
	       found = TI3025_DAC;
	       saveCR43 &= ~0x02;
	       saveCR45 &= ~0x20;
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

    outb(vgaCRIndex, 0x45);
    outb(vgaCRReg, saveCR45);

    outb(vgaCRIndex, 0x43);
    outb(vgaCRReg, saveCR43);

    return (found == type);
}

static Bool TI3020_Probe()
{
  if (!S3_928_ONLY(s3ChipId) && !S3_964_SERIES(s3ChipId))
	return FALSE;
  else if (TI3020_3025_Probe(TI3020_DAC)) {
	ErrorF("%s %s: Detected a TI ViewPoint 3020 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
  } else return FALSE;	
}

static Bool TI3025_Probe()
{
  if (!S3_964_SERIES(s3ChipId))
	return FALSE;
  else if(TI3020_3025_Probe(TI3025_DAC)) {
	ErrorF("%s %s: Detected a TI ViewPoint 3025 RAMDAC\n",
	            XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
  } else return FALSE;
}

static int TI3020_3025_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
    if (OFLG_ISSET(OPTION_NO_TI3020_CURS, &s3InfoRec.options)) {
         ErrorF("%s %s: Use of Ti3020/25 cursor disabled in XF86Config\n",
	        XCONFIG_GIVEN, s3InfoRec.name);
	 OFLG_CLR(OPTION_TI3020_CURS, &s3InfoRec.options);
    } else {
	 /* use the ramdac cursor by default */
	 ErrorF("%s %s: Using hardware cursor from Ti3020/25 RAMDAC\n",
	        OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name);
	 OFLG_SET(OPTION_TI3020_CURS, &s3InfoRec.options);
    }


    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    pixMuxPossible = TRUE;
    allowPixMuxInterlace = FALSE;
    allowPixMuxSwitching = FALSE;
    nonMuxMaxClock = 70000;
    pixMuxLimitedWidths = FALSE;
    if (S3_964_SERIES(s3ChipId)) {
         nonMuxMaxClock = 0;  /* 964 can only be in pixmux mode when */
         pixMuxMinWidth = 0;  /* working in enhanced mode */  
    }

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */

  if (DAC_IS_TI3025 && 
       !OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
      OFLG_SET(CLOCK_OPTION_TI3025, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
      clockchip_probed = XCONFIG_PROBED;
   }
   if (OFLG_ISSET(CLOCK_OPTION_TI3025, &s3InfoRec.clockOptions)) {
        int mclk, m, n, p, mcc, cr5c;

        s3ClockSelectFunc = ti3025ClockSelect;
      	numClocks = 3;
        maxRawClock = s3InfoRec.dacSpeed; 

      	outb(vgaCRIndex, 0x5c);
      	cr5c = inb(vgaCRReg);
      	outb(vgaCRReg, cr5c & 0xdf);     /* clear RS4 - use 3020 mode */

      	s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x00);
      	n = s3InTiIndReg(TI_MCLK_PLL_DATA) & 0x7f;
      	s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x01);
      	m = s3InTiIndReg(TI_MCLK_PLL_DATA) & 0x7f;
      	s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x02);
      	p = s3InTiIndReg(TI_MCLK_PLL_DATA) & 0x03;
      	mcc = s3InTiIndReg(TI_MCLK_DCLK_CONTROL);
      	if (mcc & 0x08) 
	 	mcc = (mcc & 0x07) * 2 + 2;
      	else 
	 	mcc = 1;
      	mclk = ((1431818 * ((m+2) * 8)) / (n+2) / (1 << p) / mcc + 50) / 100;
      	if (xf86Verbose)
	 ErrorF("%s %s: Using TI 3025 programmable clock (MCLK %1.3f MHz)\n",
		clockchip_probed, s3InfoRec.name, mclk / 1000.0);
      	if (OFLG_ISSET(OPTION_NUMBER_NINE, &s3InfoRec.options)) {
	   mclk = 55000;
	   if (xf86Verbose)
	    ErrorF("%s %s: Setting MCLK to %1.3f MHz for #9GXE64 Pro\n",
		   XCONFIG_PROBED, s3InfoRec.name, mclk / 1000.0);
	   Ti3025SetClock(2 * mclk, TI_MCLK_PLL_DATA, s3ProgramTi3025Clock);
	   mcc = s3InTiIndReg(TI_MCLK_DCLK_CONTROL);
	   s3OutTiIndReg(TI_MCLK_DCLK_CONTROL,0x00, (mcc & 0xf0) | 0x08);
      	}
        if (!s3InfoRec.s3MClk)
	 	s3InfoRec.s3MClk = mclk;
      	outb(vgaCRIndex, 0x5c);
      	outb(vgaCRReg, cr5c);

   }	/* Ti3020 will have an external clock */
   else 
      OtherClocksSetup();	/* external clocks */

    /* Make any necessary clock alterations due to multiplexing,
	clock doubling, etc...  s3Probe will do some last minute
	clock sanity checks when we return */
   clockDoublingPossible = TRUE;
   s3InfoRec.maxClock = s3InfoRec.dacSpeed;

   return 1;
}


static void TI3020_3025_Restore()
{
   s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, s3DacRegs[TI_MUX_CONTROL_1]);
   s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, s3DacRegs[TI_MUX_CONTROL_2]);
   s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00,
		    s3DacRegs[TI_INPUT_CLOCK_SELECT]);
   s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00,
		    s3DacRegs[TI_OUTPUT_CLOCK_SELECT]);
   s3OutTiIndReg(TI_GENERAL_CONTROL, 0x00,
		    s3DacRegs[TI_GENERAL_CONTROL]);
   s3OutTiIndReg(TI_AUXILIARY_CONTROL, 0x00,
		    s3DacRegs[TI_AUXILIARY_CONTROL]);
   s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
   s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00,
		    s3DacRegs[TI_GENERAL_IO_DATA]);
   if (DAC_IS_TI3025) {
     s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x00);
     s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[0 + 0x40]);  /* N */
     s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[1 + 0x40]);  /* M */
     s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[2 + 0x40]);  /* P */

     s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[3 + 0x40]);         /* N */
     s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[4 + 0x40]);         /* M */
     s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[5 + 0x40] | 0x80 ); /* P */

     s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[6 + 0x40]);  /* N */
     s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[7 + 0x40]);  /* M */
     s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[8 + 0x40]);  /* P */

     s3OutTiIndReg(TI_TRUE_COLOR_CONTROL, 0x00,
                    s3DacRegs[TI_TRUE_COLOR_CONTROL]);
     s3OutTiIndReg(TI_MISC_CONTROL, 0x00, s3DacRegs[TI_MISC_CONTROL]);
   }
   
   s3OutTiIndReg(TI_CURS_CONTROL, 0x00, s3DacRegs[TI_CURS_CONTROL]);
}


static void TI3020_3025_Save()
{
      if (DAC_IS_TI3025) {
          unsigned char CR5C;

          /* switch the ramdac from bt485 to ti3020 mode clearing RS4 */
	  outb(vgaCRIndex, 0x5C);
	  CR5C = inb(vgaCRReg);
	  outb(vgaCRReg, CR5C & 0xDF);

          s3DacRegs[TI_CURS_CONTROL] = s3InTiIndReg(TI_CURS_CONTROL);
          /* clear TI_PLANAR_ACCESS bit */
	  s3OutTiIndReg(TI_CURS_CONTROL, 0x7F, 0x00);
      }

      s3DacRegs[TI_MUX_CONTROL_1] = s3InTiIndReg(TI_MUX_CONTROL_1);
      s3DacRegs[TI_MUX_CONTROL_2] = s3InTiIndReg(TI_MUX_CONTROL_2);
      s3DacRegs[TI_INPUT_CLOCK_SELECT] =
                 s3InTiIndReg(TI_INPUT_CLOCK_SELECT);
      s3DacRegs[TI_OUTPUT_CLOCK_SELECT] =
                 s3InTiIndReg(TI_OUTPUT_CLOCK_SELECT);
      s3DacRegs[TI_GENERAL_CONTROL] = s3InTiIndReg(TI_GENERAL_CONTROL);
      s3DacRegs[TI_AUXILIARY_CONTROL] =
		 s3InTiIndReg(TI_AUXILIARY_CONTROL);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      s3DacRegs[TI_GENERAL_IO_DATA] = s3InTiIndReg(TI_GENERAL_IO_DATA);

      if (DAC_IS_TI3025) {
          s3DacRegs[TI_TRUE_COLOR_CONTROL] =
                 s3InTiIndReg(TI_TRUE_COLOR_CONTROL);
          s3DacRegs[TI_MISC_CONTROL] = s3InTiIndReg(TI_MISC_CONTROL);

          s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x00);
          s3DacRegs[0 + 0x40] = s3InTiIndReg(TI_PIXEL_CLOCK_PLL_DATA);  /* N */
          s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[0 + 0x40]);
          s3DacRegs[1 + 0x40] = s3InTiIndReg(TI_PIXEL_CLOCK_PLL_DATA);  /* M */
          s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[1 + 0x40]);
          s3DacRegs[2 + 0x40] = s3InTiIndReg(TI_PIXEL_CLOCK_PLL_DATA);  /* P */
          s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[2 + 0x40]);

          s3DacRegs[3 + 0x40] = s3InTiIndReg(TI_MCLK_PLL_DATA);  /* N */
          s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[3 + 0x40]);
          s3DacRegs[4 + 0x40] = s3InTiIndReg(TI_MCLK_PLL_DATA);  /* M */
          s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[4 + 0x40]);
          s3DacRegs[5 + 0x40] = s3InTiIndReg(TI_MCLK_PLL_DATA);  /* P */
          s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[5 + 0x40]);

          s3DacRegs[6 + 0x40] = s3InTiIndReg(TI_LOOP_CLOCK_PLL_DATA);  /* N */
          s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[6 + 0x40]);
          s3DacRegs[7 + 0x40] = s3InTiIndReg(TI_LOOP_CLOCK_PLL_DATA);  /* M */
          s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[7 + 0x40]);
          s3DacRegs[8 + 0x40] = s3InTiIndReg(TI_LOOP_CLOCK_PLL_DATA);  /* P */
          s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[8 + 0x40]);

      }
}

/***********************************************************\

			ATT498_DAC 
			ATT20C498_DAC			
			ATT22C498_DAC 
			ATT20C409_DAC

\***********************************************************/

static Bool ATT409_498_Probe(int type)
{
    int found = 0;
    int dir, mir, olddaccomm;

    /*quick check*/
    if (!S3_86x_SERIES(s3ChipId) && !S3_805_I_SERIES(s3ChipId))
	return FALSE;

    xf86dactopel();
    xf86dactocomm();
    (void)inb(0x3C6);
    mir = inb(0x3C6);
    dir = inb(0x3C6);
    xf86dactopel();

    if ((mir == 0x84) && (dir == 0x98)) {
	olddaccomm = xf86getdaccomm();
	xf86setdaccomm(0x0a);
	if (xf86getdaccomm() == 0) 
		found = ATT22C498_DAC;
	else
		found = ATT498_DAC;
	
	xf86setdaccomm(olddaccomm);
     } else if ((mir == 0x84) && (dir == 0x09)) {
	found = ATT20C409_DAC;
	if (!OFLG_ISSET(CLOCK_OPTION_ATT409, &s3InfoRec.clockOptions)) {
		OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
		OFLG_SET(CLOCK_OPTION_ATT409, &s3InfoRec.clockOptions);
		clockchip_probed = XCONFIG_PROBED;
	}
     } else if ((mir == 0x84) && (dir == 0x99)) {
	/*
	 * according to the 21C499 data sheet it is fully compatible
	 * with the 22C409. So we will only miss its new features
	 * this way, but in theory things might work.
	 */
	found = ATT20C409_DAC;
	if(type == ATT20C409_DAC) {
	    ErrorF("%s %s: Detected an ATT 21C499 RAMDAC\n",
		XCONFIG_PROBED, s3InfoRec.name);
	    ErrorF("%s %s:    support for this RAMDAC is untested. "
		"Please report to XFree86@XFree86.Org\n",
		XCONFIG_PROBED, s3InfoRec.name);
   	}
	if (!OFLG_ISSET(CLOCK_OPTION_ATT409, &s3InfoRec.clockOptions)) {
		OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
		OFLG_SET(CLOCK_OPTION_ATT409, &s3InfoRec.clockOptions);
		clockchip_probed = XCONFIG_PROBED;
	}
      }

     return (found == type);
}

static Bool ATT498_Probe()
{
    if(ATT409_498_Probe(ATT498_DAC)) {
	ErrorF("%s %s: Detected an ATT 20C498/21C498 RAMDAC\n",
			XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
    } else return FALSE;
}

static Bool ATT22C498_Probe()
{
    if(ATT409_498_Probe(ATT22C498_DAC)) {
	ErrorF("%s %s: Detected an ATT 22C498 RAMDAC\n",
			XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
    } else return FALSE;
}

static Bool ATT20C409_Probe()
{
    if(ATT409_498_Probe(ATT20C409_DAC)) {
	ErrorF("%s %s: Detected an ATT 20C409 RAMDAC\n",
		XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
    } else return FALSE;
}


static int ATT409_498_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */
	
    /* Set cursor options */
   	/* none */

    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    if((xf86bpp <= 8) && 
	(S3_x64_SERIES(s3ChipId) || S3_805_I_SERIES(s3ChipId))) {
    	s3ATT498PixMux = TRUE;
       	pixMuxPossible = TRUE;

        if (DAC_IS_ATT20C498) {  
	  if (S3_866_SERIES(s3ChipId) || S3_868_SERIES(s3ChipId)) {
	    nonMuxMaxClock = 100000; /* 866/868 DCLK limit */
	    pixMuxMinClock =  67500;
	  }
	  else if (S3_864_SERIES(s3ChipId)) {
	    nonMuxMaxClock =  95000; /* 864 DCLK limit */
	    pixMuxMinClock =  67500;
	  }
	  else if (S3_805_I_SERIES(s3ChipId)) {
	    nonMuxMaxClock = 80000;  /* XXXX just a guess, who has 805i docs? */
	    pixMuxMinClock = 67500;
	  }
	  else {
	    nonMuxMaxClock = 67500;
	    pixMuxMinClock = 67500;
	  }
        }
        else {
	 nonMuxMaxClock = 67500;
	 pixMuxMinClock = 67500;
        }
        allowPixMuxInterlace = TRUE;
        allowPixMuxSwitching = TRUE;
        pixMuxLimitedWidths = FALSE;
        pixMuxMinWidth = 0;

    }		

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */
   if (DAC_IS_ATT20C409 && 
       !OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
      OFLG_SET(CLOCK_OPTION_ATT409, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
      clockchip_probed = XCONFIG_PROBED;
   }
   if (OFLG_ISSET(CLOCK_OPTION_ATT409, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = att409ClockSelect;
      numClocks = 3;
      maxRawClock = s3InfoRec.dacSpeed; /* Is this right?? */
      if (xf86Verbose)
	 ErrorF("%s %s: Using ATT20C409/ATT20C499 programmable clock\n",
		clockchip_probed, s3InfoRec.name);
   }
   else 
       OtherClocksSetup();   


   if (s3ATT498PixMux) {
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;
	 if (s3Bpp == 1)	/* XXXX is this right?? */
	    clockDoublingPossible = TRUE;
   } else {
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

   return 1;
}

void ATT409_498_Restore()
{
   xf86setdaccomm(s3DacRegs[0]);
}

void ATT409_498_Save()
{
   s3DacRegs[0] = xf86getdaccomm();
}


/********************************************************\

		 SC15025_DAC 

\********************************************************/

static Bool SC15025_Probe()
{
    /* What chipsets use this? so we can do a quick check */

    Bool found = FALSE;
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
    if (id[0] == 'S' && ((id[1]<<8)|id[2]) == 15025) {  
        /* unique for the SC 15025/26 */
	if (id[3] != 'A') {           /* version number */
	   ErrorF(
 	     "%s %s: ==> New Sierra SC 15025/26 version (0x%x) found,\n",
		XCONFIG_PROBED, s3InfoRec.name, id[3]);
	   ErrorF("\tplease report!\n");
	}
	ErrorF("%s %s: Detected a Sierra SC 15025/26 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	found = TRUE;
    }
    return found;    
}

static int SC15025_PreInit()
{
    int doubleEdgeLimit;
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
   	/* none */

    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
	/* no PixMux */

    /* let OtherClocksSetup() take care of the clock options */
    OtherClocksSetup();
     
    /* Make any necessary clock alterations due to multiplexing,
	clock doubling, etc...  s3Probe will do some last minute
	clock sanity checks when we return */

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

    return 1;
}

static void SC15025_Restore()
{
      unsigned char c;

      c=xf86getdaccomm();
      xf86setdaccomm( c | 0x10 );  /* set internal register access */
      (void)xf86dactocomm();
      outb(0x3c7, 0x8);   /* Auxiliary Control Register */
      outb(0x3c8, s3DacRegs[1]);
      outb(0x3c7, 0x10);  /* Pixel Repack Register */
      outb(0x3c8, s3DacRegs[2]);
      xf86setdaccomm( c );
      xf86setdaccomm(s3DacRegs[0]);
}

static void SC15025_Save()
{
      LOCK_SYS_REGS;
         s3DacRegs[0] = xf86getdaccomm();
	 xf86setdaccomm((s3DacRegs[0] | 0x10));
         (void)xf86dactocomm();
	 outb(0x3c7,0x8);   /* Auxiliary Control Register */
	 s3DacRegs[1] = inb(0x3c8);
	 outb(0x3c7,0x10);  /* Pixel Repack Register */
	 s3DacRegs[2] = inb(0x3c8);
	 xf86setdaccomm(s3DacRegs[0]);
      UNLOCK_SYS_REGS;
}
/*********************************************************\

 			STG1700_DAC  
			STG1703_DAC 

\*********************************************************/

static Bool STG17xx_Probe(int type)
{
    int found = 0;
    int cid, did, daccomm, readmask;

    if (!S3_86x_SERIES(s3ChipId) && !S3_805_I_SERIES(s3ChipId))
	return FALSE;

    readmask = inb(0x3c6);
    xf86dactopel();
    daccomm = xf86getdaccomm();
    xf86setdaccommbit(0x10);
    xf86dactocomm();
    inb(0x3c6);
    outb(0x3c6, 0x00);
    outb(0x3c6, 0x00);
    cid = inb(0x3c6);     /* company ID */
    did = inb(0x3c6);     /* device ID */
    xf86dactopel(); 
    outb(0x3c6,readmask);

    if ((cid == 0x44) && (did == 0x00))
    {
	found = STG1700_DAC;
    }  else if ((cid == 0x44) && (did == 0x03)) {
	found = STG1703_DAC;
	if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE,&s3InfoRec.clockOptions)) {
	       OFLG_SET(CLOCK_OPTION_STG1703,    &s3InfoRec.clockOptions);
	       OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
	       clockchip_probed = XCONFIG_PROBED;
	}
    }

    xf86setdaccomm(daccomm);

    return (found == type);
}

static Bool STG1700_Probe()
{
    if(STG17xx_Probe(STG1700_DAC)) {
	ErrorF("%s %s: Detected an STG1700 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
    } else return FALSE;
}

static Bool STG1703_Probe()
{
    if(STG17xx_Probe(STG1703_DAC)) {
	ErrorF("%s %s: Detected an STG1703 RAMDAC\n",
	           XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
    } else return FALSE;
}



static int STG17xx_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
	/* none */
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    if(DAC_IS_STG1700 && (xf86bpp <= 8) && 
	(S3_x64_SERIES(s3ChipId) || S3_805_I_SERIES(s3ChipId))) {
	s3ATT498PixMux = TRUE;
	nonMuxMaxClock = 67500;
	pixMuxMinClock = 67500;
        allowPixMuxInterlace = TRUE;
        allowPixMuxSwitching = TRUE;
        pixMuxLimitedWidths = FALSE;
        pixMuxMinWidth = 0;

    }   

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */
    if (OFLG_ISSET(CLOCK_OPTION_STG1703, &s3InfoRec.clockOptions)) {
      unsigned char mi, ml, mh, tmp;
      int mclk;
      
      outb(vgaCRIndex, 0x43);
      tmp = inb(vgaCRReg);
      outb(vgaCRReg, tmp & ~0x02);
      
      outb(vgaCRIndex, 0x55);
      tmp = inb(vgaCRReg) & ~0x03;
      outb(vgaCRReg, tmp | 1);  /* set RS2 */
      
      outb(0x3c7, 0x00);  /* index high */
      outb(0x3c8, 0x48);  /* index low  */
      mi = (inb(0x3c9) >> 4) & 0x03;
      
      outb(0x3c8, 0x40 + 2*mi);  /* index low  */
      ml = inb(0x3c9);
      mh = inb(0x3c9);
      
      outb(vgaCRReg, tmp);  /* reset RS2 */
      
      mclk = ((((1431818 * ((ml&0x7f) + 2)) / ((mh&0x1f) + 2)) 
	       >> ((mh>>5)&0x03)) + 50) / 100;
      
      s3ClockSelectFunc = STG1703ClockSelect;
      numClocks = 3;
      maxRawClock = 135000;

      if (xf86Verbose)
	 ErrorF("%s %s: Using STG1703 programmable clock(MCLK%d %02x %02x "
	   "%1.3f MHz)\n",XCONFIG_GIVEN, s3InfoRec.name, mi, ml,mh, mclk/1e3);
      if (s3InfoRec.s3MClk > 0) {
	 if (xf86Verbose)
	    ErrorF("%s %s: using specified MCLK value of %1.3f MHz for DRAM "
 	"timings\n",XCONFIG_GIVEN, s3InfoRec.name, s3InfoRec.s3MClk / 1000.0);
      }
      else
	 s3InfoRec.s3MClk = mclk;
      
   }
   else
       OtherClocksSetup();

    /* Make any necessary clock alterations due to multiplexing,
	clock doubling, etc...  s3Probe will do some last minute
	clock sanity checks when we return */
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

   return 1;
}

static void STG17xx_Restore()
{
      xf86dactopel();
      xf86setdaccommbit(0x10);   /* enable extended registers */
      xf86dactocomm();
      inb(0x3c6);                /* command reg */

      outb(0x3c6, 0x03);         /* index low */
      outb(0x3c6, 0x00);         /* index high */
      
      outb(0x3c6, s3DacRegs[1]);  /* primary pixel mode */
      outb(0x3c6, s3DacRegs[2]);  /* secondary pixel mode */
      outb(0x3c6, s3DacRegs[3]);  /* PLL control */
      usleep(500);		       /* PLL settling time */

      xf86dactopel();
      xf86setdaccomm(s3DacRegs[0]);
}


static void STG17xx_Save()
{
         xf86dactopel();
         s3DacRegs[0] = xf86getdaccomm();

         xf86setdaccommbit(0x10);   /* enable extended registers */
         xf86dactocomm();
         inb(0x3c6);                /* command reg */

         outb(0x3c6, 0x03);         /* index low */
         outb(0x3c6, 0x00);         /* index high */

         s3DacRegs[1] = inb(0x3c6);  /* primary pixel mode */
         s3DacRegs[2] = inb(0x3c6);  /* secondary pixel mode */
         s3DacRegs[3] = inb(0x3c6);  /* PLL control */

         xf86dactopel();
}

/*********************************************************\

 			S3_SDAC_DAC  
			S3_GENDAC_DAC 

\*********************************************************/

static Bool S3_SDAC_GENDAC_Probe(int type)
{
   /* probe for S3 GENDAC or SDAC */
   /*
    * S3 GENDAC and SDAC have two fixed read only PLL clocks
    *     CLK0 f0: 25.255MHz   M-byte 0x28  N-byte 0x61
    *     CLK0 f1: 28.311MHz   M-byte 0x3d  N-byte 0x62
    * which can be used to detect GENDAC and SDAC since there is no chip-id
    * for the GENDAC.
    *
    * NOTE: for the GENDAC on a MIRO 10SD (805+GENDAC) reading PLL values
    * for CLK0 f0 and f1 always returns 0x7f (but is documented "read only")
    */
      
   unsigned char saveCR55, saveCR45, saveCR43, savelut[6];
   unsigned int i;		/* don't use signed int, UW2.0 compiler bug */
   long clock01, clock23;
   int found = 0;

   outb(vgaCRIndex, 0x43);
   saveCR43 = inb(vgaCRReg);
   outb(vgaCRReg, saveCR43 & ~0x02);

   outb(vgaCRIndex, 0x45);
   saveCR45 = inb(vgaCRReg);
   outb(vgaCRReg, saveCR45 & ~0x20);

   outb(vgaCRIndex, 0x55);
   saveCR55 = inb(vgaCRReg);
   outb(vgaCRReg, saveCR55 & ~1);

   outb(0x3c7,0);
   for(i=0; i<2*3; i++)		/* save first two LUT entries */
      savelut[i] = inb(0x3c9);
   outb(0x3c8,0);
   for(i=0; i<2*3; i++)		/* set first two LUT entries to zero */
      outb(0x3c9,0);

   outb(vgaCRIndex, 0x55);
   outb(vgaCRReg, saveCR55 | 1);

   outb(0x3c7,0);
   for(i=clock01=0; i<4; i++)
      clock01 = (clock01 << 8) | (inb(0x3c9) & 0xff);
   for(i=clock23=0; i<4; i++)
      clock23 = (clock23 << 8) | (inb(0x3c9) & 0xff);

   outb(vgaCRIndex, 0x55);
   outb(vgaCRReg, saveCR55 & ~1);

   outb(0x3c8,0);
   for(i=0; i<2*3; i++)		/* restore first two LUT entries */
      outb(0x3c9,savelut[i]);

   outb(vgaCRIndex, 0x55);
   outb(vgaCRReg, saveCR55);

   if ( clock01 == 0x28613d62 ||
       (clock01 == 0x7f7f7f7f && clock23 != 0x7f7f7f7f)) {

	
	 xf86dactopel();
	 inb(0x3c6);
	 inb(0x3c6);
	 inb(0x3c6);

	 /* the fourth read will show the SDAC chip ID and revision */
	 if (((i=inb(0x3c6)) & 0xf0) == 0x70) {
	    found = S3_SDAC_DAC;
	    saveCR43 &= ~0x02;
	    saveCR45 &= ~0x20;
	 }
	 else {
	    found = S3_GENDAC_DAC;
	    saveCR43 &= ~0x02;
	    saveCR45 &= ~0x20;
	 }
         xf86dactopel();
   }

   outb(vgaCRIndex, 0x45);
   outb(vgaCRReg, saveCR45);

   outb(vgaCRIndex, 0x43);
   outb(vgaCRReg, saveCR43);

   if((found==type) && !OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, 
					&s3InfoRec.clockOptions)) {
	OFLG_SET(CLOCK_OPTION_S3GENDAC,    &s3InfoRec.clockOptions);
	OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
	clockchip_probed = XCONFIG_PROBED;
   }


   return (found == type);
}

static Bool S3_SDAC_Probe()
{
   if (!S3_86x_SERIES(s3ChipId) && !S3_805_I_SERIES(s3ChipId))
	return FALSE;
   else if(S3_SDAC_GENDAC_Probe(S3_SDAC_DAC)) {
   ErrorF("%s %s: Detected an S3 SDAC 86C716 RAMDAC and programmable clock\n", 
		XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
   } else return FALSE;
}

static Bool S3_GENDAC_Probe()
{
   if (!S3_801_SERIES(s3ChipId))
	return FALSE;
   else if (S3_SDAC_GENDAC_Probe(S3_GENDAC_DAC)) {
  ErrorF("%s %s: Detected an S3 GENDAC 86C708 RAMDAC and programmable clock\n", 
			XCONFIG_PROBED, s3InfoRec.name);
	return TRUE;
   } else return FALSE;
}

static int S3_SDAC_GENDAC_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
	/* none */
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    if(DAC_IS_SDAC && (xf86bpp <= 8) && 
	(S3_x64_SERIES(s3ChipId) || S3_805_I_SERIES(s3ChipId))) {
	s3ATT498PixMux = TRUE;
	nonMuxMaxClock = 67500;
	pixMuxMinClock = 67500;
        allowPixMuxInterlace = TRUE;
        allowPixMuxSwitching = TRUE;
        pixMuxLimitedWidths = FALSE;
        pixMuxMinWidth = 0;
    }

    OtherClocksSetup();   /* S3_GENDAC clock is in OtherClocksSetup()
			     since it is compatible with the ICS5342 */

    /* Make any necessary clock alterations due to multiplexing,
	clock doubling, etc...  s3Probe will do some last minute
	clock sanity checks when we return */
    if(DAC_IS_SDAC) {
    	if (s3ATT498PixMux) {
	  s3InfoRec.maxClock = s3InfoRec.dacSpeed;
	  if (s3Bpp == 1)	/* XXXX is this right?? */
	    clockDoublingPossible = TRUE;
        } else {
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
     } else  /* DAC_IS_GENDAC */
        s3InfoRec.maxClock = s3InfoRec.dacSpeed / s3Bpp;

      return 1;
}


static void S3_SDAC_GENDAC_Restore()
{
#if !defined(PC98_PW) && !defined(PC98_PWLB)
      unsigned char tmp;

      outb(vgaCRIndex, 0x55);
      tmp = inb(vgaCRReg);
      outb(vgaCRReg, tmp | 1);

      outb(0x3c6, s3DacRegs[0]);      /* Enhanced command register */
      outb(0x3c8, 2);                   /* index to f2 reg */
      outb(0x3c9, s3DacRegs[3]);      /* f2 PLL M divider */
      outb(0x3c9, s3DacRegs[4]);      /* f2 PLL N1/N2 divider */
      outb(0x3c8, 0x0e);                /* index to PLL control */
      outb(0x3c9, s3DacRegs[5]);      /* PLL control */
      outb(0x3c8, s3DacRegs[2]);      /* PLL write index */
      outb(0x3c7, s3DacRegs[1]);      /* PLL read index */

      outb(vgaCRReg, tmp & ~1);
#endif
}

static void S3_SDAC_GENDAC_Save()
{
#if !defined(PC98_PW) && !defined(PC98_PWLB)
         unsigned char tmp;

         outb(vgaCRIndex, 0x55);
	 tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp | 1);

         s3DacRegs[0] = inb(0x3c6);      /* Enhanced command register */
         s3DacRegs[2] = inb(0x3c8);      /* PLL write index */
         s3DacRegs[1] = inb(0x3c7);      /* PLL read index */
         outb(0x3c7, 2);                   /* index to f2 reg */
         s3DacRegs[3] = inb(0x3c9);      /* f2 PLL M divider */
         s3DacRegs[4] = inb(0x3c9);      /* f2 PLL N1/N2 divider */
         outb(0x3c7, 0x0e);                /* index to PLL control */
         s3DacRegs[5] = inb(0x3c9);      /* PLL control */

         outb(vgaCRReg, tmp & ~1);
#endif
}

/***********************************************************\

 			S3_TRIO32_DAC  
			S3_TRIO64_DAC 

\***********************************************************/


static Bool S3_TRIO_Probe(int type) 
{
   int found = 0;

   if (S3_TRIOxx_SERIES(s3ChipId)) {
	 if (S3_TRIO32_SERIES(s3ChipId))
		found = S3_TRIO32_DAC;
	 else 
		found = S3_TRIO64_DAC;
   }

   if(found == type)
   {
      if ( OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions) &&
	  !OFLG_ISSET(CLOCK_OPTION_S3TRIO, &s3InfoRec.clockOptions)) {
	ErrorF("%s %s: for Trio32/64 chips you shouldn't specify a Clockchip\n",
		XCONFIG_PROBED, s3InfoRec.name);
	 /* Clear the other clock options */
	OFLG_ZERO(&s3InfoRec.clockOptions);
      }
      if (!OFLG_ISSET(CLOCK_OPTION_S3TRIO, &s3InfoRec.clockOptions)) {
	 OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
	 OFLG_SET(CLOCK_OPTION_S3TRIO, &s3InfoRec.clockOptions);
	 clockchip_probed = XCONFIG_PROBED;
      }
   }
  
   return (found == type);
}

static Bool S3_TRIO64_Probe()
{
    return S3_TRIO_Probe(S3_TRIO64_DAC);
}

static Bool S3_TRIO32_Probe()
{
    return S3_TRIO_Probe(S3_TRIO32_DAC);
}

static int S3_TRIO_PreInit()
{
    unsigned char sr8;
    int m,n,n1,n2, mclk;

    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
	/* none */
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    if(xf86bpp <= 8) {
	s3ATT498PixMux = TRUE;
       	pixMuxPossible = TRUE;
    	if (OFLG_ISSET(OPTION_TRIO64VP_BUG3, &s3InfoRec.options)) {
	    nonMuxMaxClock = 0;   /* need CR67=10 and DCLK/2 all the time */
	    pixMuxMinClock = 0;   /* to avoid problems with blank signal */
    	} else {
	    nonMuxMaxClock = 80000;
	    pixMuxMinClock = 80000;
    	}
    	allowPixMuxInterlace = TRUE;
    	allowPixMuxSwitching = TRUE;
    	pixMuxLimitedWidths = FALSE;
    	pixMuxMinWidth = 0;
   
	s3InfoRec.maxClock = s3InfoRec.dacSpeed;
    } else if (s3Bpp < 4)
	 s3InfoRec.maxClock = 80000;
    else
	 s3InfoRec.maxClock = 50000;
 

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */
    s3ClockSelectFunc = s3GendacClockSelect;
    numClocks = 3;
    maxRawClock = 135000;
      
    outb(0x3c4, 0x08);
    sr8 = inb(0x3c5);
    outb(0x3c5, 0x06);
      
    outb(0x3c4, 0x11);
    m = inb(0x3c5);
    outb(0x3c4, 0x10);
    n = inb(0x3c5);
      
    outb(0x3c4, 0x08);
    outb(0x3c5, sr8);
      
    m &= 0x7f;
    n1 = n & 0x1f;
    n2 = (n>>5) & 0x03;
    mclk = ((1431818 * (m+2)) / (n1+2) / (1 << n2) + 50) / 100;
    if (xf86Verbose)
	 ErrorF("%s %s: Using Trio32/64 programmable clock (MCLK %1.3f MHz)\n"
		,clockchip_probed, s3InfoRec.name
		,mclk / 1000.0);
    if (s3InfoRec.s3MClk > 0) {
      if (xf86Verbose)
	ErrorF("%s %s: using specified MCLK value of %1.3f MHz for DRAM "
	 "timings\n",XCONFIG_GIVEN, s3InfoRec.name, s3InfoRec.s3MClk/1000.0);
    } else {
	 s3InfoRec.s3MClk = mclk;
    }

 
    return 1;  
}

static void S3_TRIO_Restore()
{
      unsigned char tmp;

      outb(0x3c2, s3DacRegs[0]);
      outb(0x3c4, 0x08); outb(0x3c5, 0x06);

      outb(0x3c4, 0x09); outb(0x3c5, s3DacRegs[2]);
      outb(0x3c4, 0x0a); outb(0x3c5, s3DacRegs[3]);
      outb(0x3c4, 0x0b); outb(0x3c5, s3DacRegs[4]);
      outb(0x3c4, 0x0d); outb(0x3c5, s3DacRegs[5]);

      outb(0x3c4, 0x10); outb(0x3c5, s3DacRegs[8]); 
      outb(0x3c4, 0x11); outb(0x3c5, s3DacRegs[9]); 
      outb(0x3c4, 0x12); outb(0x3c5, s3DacRegs[10]); 
      outb(0x3c4, 0x13); outb(0x3c5, s3DacRegs[11]); 
      outb(0x3c4, 0x1a); outb(0x3c5, s3DacRegs[12]); 
      outb(0x3c4, 0x1b); outb(0x3c5, s3DacRegs[13]); 
      outb(0x3c4, 0x15);
      tmp = inb(0x3c5);
      outb(0x3c4, tmp & ~0x20);
      outb(0x3c4, tmp |  0x20);
      outb(0x3c4, tmp & ~0x20);

      outb(0x3c4, 0x15); outb(0x3c5, s3DacRegs[6]); 
      outb(0x3c4, 0x18); outb(0x3c5, s3DacRegs[7]);

      outb(0x3c4, 0x08); outb(0x3c5, s3DacRegs[1]);

} 

static void S3_TRIO_Save()
{
	 s3DacRegs[0] = inb(0x3cc);

	 outb(0x3c4, 0x08); s3DacRegs[1] = inb(0x3c5);
	 outb(0x3c5, 0x06);

	 outb(0x3c4, 0x09); s3DacRegs[2]  = inb(0x3c5);
	 outb(0x3c4, 0x0a); s3DacRegs[3]  = inb(0x3c5);
	 outb(0x3c4, 0x0b); s3DacRegs[4]  = inb(0x3c5);
	 outb(0x3c4, 0x0d); s3DacRegs[5]  = inb(0x3c5);
	 outb(0x3c4, 0x15); s3DacRegs[6]  = inb(0x3c5) & 0xfe; 
	 outb(0x3c5, s3DacRegs[6]);
	 outb(0x3c4, 0x18); s3DacRegs[7]  = inb(0x3c5);

	 outb(0x3c4, 0x10); s3DacRegs[8]  = inb(0x3c5);
	 outb(0x3c4, 0x11); s3DacRegs[9]  = inb(0x3c5);
	 outb(0x3c4, 0x12); s3DacRegs[10] = inb(0x3c5);
	 outb(0x3c4, 0x13); s3DacRegs[11] = inb(0x3c5);
	 outb(0x3c4, 0x1a); s3DacRegs[12] = inb(0x3c5);
	 outb(0x3c4, 0x1b); s3DacRegs[13] = inb(0x3c5);

	 outb(0x3c4, 8);
	 outb(0x3c5, 0x00);
}

/******************************************************\

  			TI3030_DAC  
			TI3026_DAC 

\******************************************************/

static Bool TI3030_3026_Probe(int type)
{ 
    int found = 0;
    unsigned char saveCR55, saveCR45, saveCR43, saveID, saveTIndx;

    if (!S3_964_SERIES(s3ChipId) && !S3_968_SERIES(s3ChipId))
	return FALSE;

    outb(vgaCRIndex, 0x43);
    saveCR43 = inb(vgaCRReg);
    outb(vgaCRReg, saveCR43 & ~0x02);

    outb(vgaCRIndex, 0x45);
    saveCR45 = inb(vgaCRReg);
    outb(vgaCRReg, saveCR45 & ~0x20);

    outb(vgaCRIndex, 0x55);
    saveCR55 = inb(vgaCRReg);

    outb(vgaCRReg, (saveCR55 & 0xFC) | 0x00);
    saveTIndx = inb(0x3c8);
    outb(0x3c8, 0x3f);

    outb(vgaCRReg, (saveCR55 & 0xFC) | 0x02);
    saveID = inb(0x3c6);
    if (saveID == 0x26 || saveID == 0x30) {
	 outb(0x3c6, ~saveID);    /* check if ID reg. is read-only */
	 if (inb(0x3c6) != saveID) {
	       outb(0x3c6, saveID);
	 }
	 else {
	     /*
	      * Found TI ViewPoint 3026/3030 DAC
	      */
	      int rev;
	      outb(vgaCRReg, (saveCR55 & 0xFC) | 0x00);
	      saveTIndx = inb(0x3c8);
	      outb(0x3c8, 0x01);
	      outb(vgaCRReg, (saveCR55 & 0xFC) | 0x02);
	      rev = inb(0x3c6);
	       
	      if (saveID == 0x26)
		  found = TI3026_DAC;
	      else  /* saveID == 0x30 */
		  found = TI3030_DAC;
	      
 	      if(found == type)
	       ErrorF("%s %s: Detected a TI ViewPoint 30%02x RAMDAC rev. %x\n",
		      XCONFIG_PROBED, s3InfoRec.name, saveID, rev);
	      saveCR43 &= ~0x02;
	      saveCR45 &= ~0x20;
	 }
    }

    /* restore this mess */
    outb(vgaCRReg, (saveCR55 & 0xFC) | 0x00);
    outb(0x3c8, saveTIndx);

    outb(vgaCRIndex, 0x55);
    outb(vgaCRReg, saveCR55);

    outb(vgaCRIndex, 0x45);
    outb(vgaCRReg, saveCR45);

    outb(vgaCRIndex, 0x43);
    outb(vgaCRReg, saveCR43);

    return (found == type);
}

static Bool TI3030_Probe()
{
    return TI3030_3026_Probe(TI3030_DAC);
}

static Bool TI3026_Probe()
{
    return TI3030_3026_Probe(TI3026_DAC);
}

static int TI3030_3026_PreInit()
{
    int mclk, m, n, p;

    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
    if (OFLG_ISSET(OPTION_SW_CURSOR, &s3InfoRec.options)) {
         ErrorF("%s %s: Use of Ti3026/3030 cursor disabled in XF86Config\n",
	        XCONFIG_GIVEN, s3InfoRec.name);
	 OFLG_CLR(OPTION_TI3026_CURS, &s3InfoRec.options);
    } else {
	 /* use the ramdac cursor by default */
	 ErrorF("%s %s: Using hardware cursor from Ti3026/3030 RAMDAC\n",
	        OFLG_ISSET(OPTION_TI3026_CURS, &s3InfoRec.options) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name);
	 OFLG_SET(OPTION_TI3026_CURS, &s3InfoRec.options);
    }
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    pixMuxPossible = TRUE;
    allowPixMuxInterlace = TRUE;
    allowPixMuxSwitching = FALSE;
    nonMuxMaxClock = 70000;
    pixMuxLimitedWidths = FALSE;
    if (S3_964_SERIES(s3ChipId)) {
         nonMuxMaxClock = 0;  /* 964 can only be in pixmux mode when */
         pixMuxMinWidth = 0;  /* working in enhanced mode */  
    }

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */
   
   if(!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
   	OFLG_SET(CLOCK_OPTION_TI3026, &s3InfoRec.clockOptions);
   	OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
   	clockchip_probed = XCONFIG_PROBED;
   }  /* there are cards with Ti3026 and EXTERNAL clock! */

   s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
   n = s3InTi3026IndReg(TI_MCLK_PLL_DATA) & 0x3f;
   s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x04);
   m = s3InTi3026IndReg(TI_MCLK_PLL_DATA) & 0x3f;
   s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x08);
   p = s3InTi3026IndReg(TI_MCLK_PLL_DATA) & 0x03;
   mclk = ((1431818 * ((65-m) * 8)) / (65-n) / (1 << p) + 50) / 100;
   if (xf86Verbose)
	ErrorF("%s %s: Using Ti3026/3030 programmable MCLK (MCLK %1.3f MHz)\n",
		clockchip_probed, s3InfoRec.name, mclk / 1000.0);
   numClocks = 3;
   if (!s3InfoRec.s3MClk)
	s3InfoRec.s3MClk = mclk;

   if (OFLG_ISSET(CLOCK_OPTION_TI3026, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = ti3026ClockSelect;
      maxRawClock = s3InfoRec.dacSpeed; /* Is this right?? */
      OFLG_SET(CLOCK_OPTION_TI3026, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
      if (xf86Verbose)
	 ErrorF("%s %s: Using Ti3026/3030 programmable DCLK\n",
		clockchip_probed, s3InfoRec.name);
   }  /* there are cards with Ti3026 and EXTERNAL clock! */
   else
      OtherClocksSetup();

   clockDoublingPossible = TRUE;
   s3InfoRec.maxClock = s3InfoRec.dacSpeed;

   return 1;
}

static void TI3030_3026_Restore()
{
      int i;

      /* select pixel clock PLL as dot clock soure */
      s3OutTi3026IndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_PLL);

      /* programm dot clock PLL to new MCLK frequency */
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[3 + 0x40]);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[4 + 0x40]);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[5 + 0x40]);

      /* wait until PLL is locked */
      for (i=0; i<10000; i++)
	 if (s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA) & 0x40) 
	    break;

      /* switch to output dot clock on the MCLK terminal */
      s3OutTi3026IndReg(0x39, 0xe7, 0x00);
      s3OutTi3026IndReg(0x39, 0xe7, 0x08);
      
      /* Set MCLK */
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
      s3OutTi3026IndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[3 + 0x40]);
      s3OutTi3026IndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[4 + 0x40]);
      s3OutTi3026IndReg(TI_MCLK_PLL_DATA, 0x00, s3DacRegs[5 + 0x40]);

      /* wait until PLL is locked */
      for (i=0; i<10000; i++) 
	 if (s3InTi3026IndReg(TI_MCLK_PLL_DATA) & 0x40) 
	    break;

      /* switch to output MCLK on the MCLK terminal */
      s3OutTi3026IndReg(0x39, 0xe7, 0x10);
      s3OutTi3026IndReg(0x39, 0xe7, 0x18);


      /* restore dot clock PLL */

      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[0 + 0x40]);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[1 + 0x40]);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, s3DacRegs[2 + 0x40]);

      s3OutTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[6 + 0x40]);
      s3OutTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[7 + 0x40]);
      s3OutTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, s3DacRegs[8 + 0x40]);

      s3OutTi3026IndReg(TI_CURS_CONTROL, 0x00, 
			s3DacRegs[TI_CURS_CONTROL]);
      s3OutTi3026IndReg(TI_INPUT_CLOCK_SELECT, 0x00,
			s3DacRegs[TI_INPUT_CLOCK_SELECT]);
      s3OutTi3026IndReg(TI_MUX_CONTROL_1, 0x00,
			s3DacRegs[TI_MUX_CONTROL_1]);
      s3OutTi3026IndReg(TI_MUX_CONTROL_2, 0x00,
			s3DacRegs[TI_MUX_CONTROL_2]);
      s3OutTi3026IndReg(TI_COLOR_KEY_CONTROL, 0x00, 
			s3DacRegs[TI_COLOR_KEY_CONTROL]);
      s3OutTi3026IndReg(TI_GENERAL_CONTROL, 0x00, 
			s3DacRegs[TI_GENERAL_CONTROL]);
      s3OutTi3026IndReg(TI_MISC_CONTROL, 0x00, 
			s3DacRegs[TI_MISC_CONTROL]);
      s3OutTi3026IndReg(TI_MCLK_LCLK_CONTROL, 0x00, 
			s3DacRegs[TI_MCLK_LCLK_CONTROL]);
      s3OutTi3026IndReg(TI_GENERAL_IO_CONTROL, 0x00, 
			s3DacRegs[TI_GENERAL_IO_CONTROL]);
      s3OutTi3026IndReg(TI_GENERAL_IO_DATA, 0x00, 
			s3DacRegs[TI_GENERAL_IO_DATA]);
}

static void TI3030_3026_Save()
{
	int i;

	  for (i=0; i<0x40; i++) 
	     s3DacRegs[i] = s3InTi3026IndReg(i);

          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
          s3DacRegs[0 + 0x40] = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x01);
          s3DacRegs[1 + 0x40] = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x02);
          s3DacRegs[2 + 0x40] = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA);

          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
          s3DacRegs[3 + 0x40] = s3InTi3026IndReg(TI_MCLK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x04);
          s3DacRegs[4 + 0x40] = s3InTi3026IndReg(TI_MCLK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x08);
          s3DacRegs[5 + 0x40] = s3InTi3026IndReg(TI_MCLK_PLL_DATA);

          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
          s3DacRegs[6 + 0x40] = s3InTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x10);
          s3DacRegs[7 + 0x40] = s3InTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x20);
          s3DacRegs[8 + 0x40] = s3InTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA);
          s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);

 }
/*********************************************************\

 			IBMRGB524_DAC 
			IBMRGB525_DAC 
			IBMRGB528_DAC 

\*********************************************************/


static Bool IBMRGB52x_Probe(int type)
{
    int ibm_id;
	 
    if (!S3_964_SERIES(s3ChipId) && !S3_968_SERIES(s3ChipId))
	return FALSE;

    if ((ibm_id = s3IBMRGB_Probe())) {
	s3IBMRGB_Init();
	switch(ibm_id >> 8) {
	    case 1:
	       if(type == IBMRGB525_DAC) {
	         ErrorF("%s %s: Detected an IBM RGB525 ramdac rev. %x\n",
		      XCONFIG_PROBED, s3InfoRec.name, ibm_id&0xff);
	         return TRUE;
               }
	       break;
	    case 2:
	       if ( (ibm_id & 0xff) == 0xf0) {
	         if(type == IBMRGB528_DAC) {
	           ErrorF("%s %s: Detected an IBM RGB528 ramdac rev. %x\n",
		      XCONFIG_PROBED, s3InfoRec.name, ibm_id&0xff);
	           return TRUE;
		 }
	       }
	       else if ( (ibm_id & 0xff) == 0xe0) {
	          if(type == IBMRGB528_DAC) {
	            ErrorF("%s %s: Detected an IBM RGB528A ramdac rev. %x\n",
		      XCONFIG_PROBED, s3InfoRec.name, ibm_id&0xff);	
		    return TRUE;
		  }
	       }
	       else {
	          if(type == IBMRGB524_DAC) {
	            ErrorF("%s %s: Detected an IBM RGB524 ramdac rev. %x\n",
		      XCONFIG_PROBED, s3InfoRec.name, ibm_id&0xff);
		    return TRUE;
		  }
	       }
	       break;
	    case 0x102:
	       if(type == IBMRGB528_DAC) {
	          ErrorF("%s %s: Detected an IBM RGB528 ramdac rev. %x\n",
		      XCONFIG_PROBED, s3InfoRec.name, ibm_id&0xff);
		  return TRUE;
               }
	       break;
	    default:
	       break;
	}
    }

    return FALSE;
}

static Bool IBMRGB524_Probe()
{
    return IBMRGB52x_Probe(IBMRGB524_DAC);
}

static Bool IBMRGB525_Probe()
{
    return IBMRGB52x_Probe(IBMRGB525_DAC);
}

static Bool IBMRGB528_Probe()
{
    return IBMRGB52x_Probe(IBMRGB528_DAC);
}

static int IBMRGB52x_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
    if (OFLG_ISSET(OPTION_SW_CURSOR, &s3InfoRec.options)) {
         ErrorF("%s %s: Use of IBM RGB52x cursor disabled in XF86Config\n",
	        XCONFIG_GIVEN, s3InfoRec.name);
	 OFLG_CLR(OPTION_IBMRGB_CURS, &s3InfoRec.options);
    } else {
	 /* use the ramdac cursor by default */
	 ErrorF("%s %s: Using hardware cursor from IBM RGB52x RAMDAC\n",
	        OFLG_ISSET(OPTION_IBMRGB_CURS, &s3InfoRec.options) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, s3InfoRec.name);
	 OFLG_SET(OPTION_IBMRGB_CURS, &s3InfoRec.options);
    }
   
    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
    pixMuxPossible = TRUE;
    allowPixMuxInterlace = TRUE;
    allowPixMuxSwitching = TRUE;
    nonMuxMaxClock = 70000;
    pixMuxLimitedWidths = FALSE;
    if (S3_964_SERIES(s3ChipId)) {
         nonMuxMaxClock = 0;  /* 964 can only be in pixmux mode when */
         pixMuxMinWidth = 0;  /* working in enhanced mode */  
    }

    /* Set Clock options, s3ClockSelectFunc, & maxRawClock 
	for internal clocks. Pass the job to the OtherClocksSetup()
	function for external clocks*/
   if (DAC_IS_IBMRGB && 
       !OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions)) {
      OFLG_SET(CLOCK_OPTION_IBMRGB, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);
      clockchip_probed = XCONFIG_PROBED;
   }

   if (OFLG_ISSET(CLOCK_OPTION_IBMRGB, &s3InfoRec.clockOptions)) {
      char *refclock_probed;
      int mclk=0, m, n, df;
      int m0,m1,n0,n1;
      double f0,f1,f,fdiff;
	 
      maxRawClock = s3InfoRec.dacSpeed; /* Is this right?? */
      s3ClockSelectFunc = IBMRGBClockSelect;
      numClocks = 3;

      OFLG_SET(CLOCK_OPTION_IBMRGB, &s3InfoRec.clockOptions);
      OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &s3InfoRec.clockOptions);

      if (s3InfoRec.s3RefClk) 
	 refclock_probed = XCONFIG_GIVEN;
      else 
	 refclock_probed = XCONFIG_PROBED;

      if (s3InIBMRGBIndReg(IBMRGB_pll_ctrl1) & 1) {
	 m0 = s3InIBMRGBIndReg(IBMRGB_m0+0*2);
	 n0 = s3InIBMRGBIndReg(IBMRGB_n0+0*2) & 0x1f;
	 m1 = s3InIBMRGBIndReg(IBMRGB_m0+1*2);
	 n1 = s3InIBMRGBIndReg(IBMRGB_n0+1*2) & 0x1f;
      }
      else {
	 m0 = s3InIBMRGBIndReg(IBMRGB_f0+0);
	 m1 = s3InIBMRGBIndReg(IBMRGB_f0+1);
	 n0 = s3InIBMRGBIndReg(IBMRGB_pll_ref_div_fix) & 0x1f;
	 n1 = n0;
      }
      f0 = 25.175 / ((m0&0x3f)+65.0) * n0 * (8 >> (m0>>6));
      f1 = 28.322 / ((m1&0x3f)+65.0) * n1 * (8 >> (m1>>6));      
      if (f1>f0) fdiff = f1-f0;
      else       fdiff = f0-f1;


      /* refclock defaults should not depend on vendor options
       * but only on probed BIOS tags; it's too dangerous
       * when users play with options */

      if (find_bios_string(s3InfoRec.BIOSbase,"VideoBlitz III AV",
			   "Genoa Systems Corporation") != NULL) {
         if (s3BiosVendor == UNKNOWN_BIOS) 
	    s3BiosVendor = GENOA_BIOS;
	 if (xf86Verbose)
	    ErrorF("%s %s: Genoa VideoBlitz III AV BIOS found\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 if (!s3InfoRec.s3RefClk) 
	    s3InfoRec.s3RefClk = 50000;
      }
      else if (find_bios_string(s3InfoRec.BIOSbase,"STB Systems, Inc.", NULL) 
	       != NULL) {
	 if (s3BiosVendor == UNKNOWN_BIOS) 
	    s3BiosVendor = STB_BIOS;
	 if (xf86Verbose)
	    ErrorF("%s %s: STB Velocity 64 BIOS found\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 if (!s3InfoRec.s3RefClk)
	    s3InfoRec.s3RefClk = 24000;
      }
      else if (find_bios_string(s3InfoRec.BIOSbase,
				"Number Nine Visual Technology","Motion 771")
	       != NULL) {
	 if (s3BiosVendor == UNKNOWN_BIOS) 
	    s3BiosVendor = NUMBER_NINE_BIOS;
	 if (xf86Verbose)
	    ErrorF("%s %s: #9 Motion 771 BIOS found\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 if (!s3InfoRec.s3RefClk)
	    s3InfoRec.s3RefClk = 16000;
      }
      else if (find_bios_string(s3InfoRec.BIOSbase,
				"Hercules Graphite Terminator",NULL) != NULL) {
	 if (s3BiosVendor == UNKNOWN_BIOS) 
	    s3BiosVendor = HERCULES_BIOS;
	 if (xf86Verbose)
	    ErrorF("%s %s: Hercules Graphite Terminator BIOS found\n",
		   XCONFIG_PROBED, s3InfoRec.name);
	 if (!s3InfoRec.s3RefClk)
	    if (S3_968_SERIES(s3ChipId))
	       /* Hercules Graphite Terminator Pro(tm) BIOS. */
	       s3InfoRec.s3RefClk = 16000;
	    else		/* S3 964 */
	       /* Hercules Graphite Terminator 64(tm) BIOS. */
	       s3InfoRec.s3RefClk = 50000;
      }
      else if (!s3InfoRec.s3RefClk) {
	 if (fdiff < f0*0.02) {
	    /* f = (f0+f1)/2; */ 
	    f = f1;
            /* 28.322 MHz clock seems to be more acurate then 25.175 */
	    /* try to match some known reclock values */
	    if ((int)(f*1e3/200+0.5) == 16000/200)
	       s3InfoRec.s3RefClk = 16000;
	    else if ((int)(f*1e3/200+0.5) == 50000/200)
	       s3InfoRec.s3RefClk = 50000;
	    else if ((int)(f*1e3/200+0.5) == 24000/200)
	       s3InfoRec.s3RefClk = 24000;
	    else if ((int)(f*1e3/200+0.5) == 14318/200)
	       s3InfoRec.s3RefClk = 14318;
	    else 
	       s3InfoRec.s3RefClk = (int)(f*2+0.5)*500;
	 }
      }
      else if (!s3InfoRec.s3RefClk) {
	 s3InfoRec.s3RefClk = 16000; /* default */
      }
      
      if (!DAC_IS_IBMRGB525) {
	 m = s3InIBMRGBIndReg(IBMRGB_sysclk_vco_div);
	 n = s3InIBMRGBIndReg(IBMRGB_sysclk_ref_div) & 0x1f;
	 df = m>>6;
	 m &= 0x3f;
	 if (!n) { m=0; n=1; }
	 mclk = ((s3InfoRec.s3RefClk*100 * (m+65)) / n / (8 >> df) + 50) / 100;
      }
      if (xf86Verbose) {
	 ErrorF("%s %s: Using IBM RGB52x programmable clock",
		clockchip_probed, s3InfoRec.name);
	 if (mclk)
	    ErrorF(" (MCLK %1.3f MHz)", mclk / 1000.0);
	 ErrorF("\n");	
	 ErrorF("%s %s: with refclock %1.3f MHz (probed %1.3f & %1.3f)\n",
		refclock_probed,s3InfoRec.name,s3InfoRec.s3RefClk/1e3,f0,f1);
      }
      if (!s3InfoRec.s3MClk)
	 s3InfoRec.s3MClk = mclk;
   } 
   else 
       OtherClocksSetup();

   clockDoublingPossible = FALSE;
      /* LCLK & SCLK limit is 100 MHz */
   if ((s3InfoRec.dacSpeed * s3Bpp) / 8 > 100000)  
	 s3InfoRec.maxClock = (100000 * 8) / s3Bpp; 
   else
	 s3InfoRec.maxClock = s3InfoRec.dacSpeed;

   return 1;
}

static void IBMRGB52x_Restore()
{
      int i;

      if (DAC_IS_IBMRGB525) {
	 s3OutIBMRGBIndReg(IBMRGB_vram_mask_0, 0, 3);
	 s3OutIBMRGBIndReg(IBMRGB_misc1, ~0x40, 0x40);
	 usleep (1000);
	 s3OutIBMRGBIndReg(IBMRGB_misc2, ~1, 0);
      }
      for (i=0; i<0x100; i++)
	 s3OutIBMRGBIndReg(i, 0, s3DacRegs[i]);
      outb(vgaCRIndex, 0x22);
      outb(vgaCRReg, s3DacRegs[0x100]);   
}

static void IBMRGB52x_Save()
{
	int i;

	for (i=0; i<0x100; i++)
	    s3DacRegs[i] = s3InIBMRGBIndReg(i);
	outb(vgaCRIndex, 0x22);
	s3DacRegs[0x100] = inb(vgaCRReg);
}

/***********************************************************\

  			ATT20C490_DAC  
			SS2410_DAC  
			SC1148x_M2_DAC  

\***********************************************************/

#define Setcomm(v)        (xf86dactocomm(),outb(0x3C6,v),\
                        xf86dactocomm(),inb(0x3C6))

static Bool MISC_HI_COLOR_Probe(int type)
{
    int found = 0;
    unsigned char tmp, olddacpel, olddaccomm, notdaccomm;

    /*quick check*/
    if(!S3_8XX_9XX_SERIES(s3ChipId)) 
	return FALSE;
	
    (void) xf86dactocomm();
    olddaccomm = inb(0x3C6);
    xf86dactopel();
    olddacpel = inb(0x3C6);

    notdaccomm = ~olddaccomm;
    outb(0x3C6, notdaccomm);
    (void) xf86dactocomm();

    if (inb(0x3C6) != notdaccomm) {
            /* Looks like a HiColor RAMDAC */
            if ((Setcomm(0xE0) & 0xE0) == 0xE0) {
               if ((Setcomm(0x60) & 0xE0) == 0x00) {
                  if ((Setcomm(0x02) & 0x02) != 0x00) {
                     found = ATT20C490_DAC;
                  }
               } else {
                  tmp = Setcomm(olddaccomm);
                  if (inb(0x3C6) != notdaccomm) {
                     (void) inb(0x3C6);
                     (void) inb(0x3C6);
                     (void) inb(0x3C6);
                     if (inb(0x3C6) != notdaccomm) {
                        xf86dactopel();
                        outb(0x3C6, 0xFF);
                        (void) inb(0x3C6);
                        (void) inb(0x3C6);
                        (void) inb(0x3C6);
                        switch (inb(0x3C6)) {
                        case 0x44:
                        case 0x82:
			    break;
                        case 0x8E:
			    found = SS2410_DAC;		
			    break;
                        default:
                            xf86dactopel();
                            outb(0x3C6, olddacpel & 0xFB);
                            xf86dactocomm();
                            outb(0x3C6, olddaccomm & 0x04);
                            tmp = inb(0x3C6);
                            outb(0x3C6, tmp & 0xFB);
                            if ((tmp & 0x04) == 0) {
                               found = SC1148x_M2_DAC;
                            }
                        }
                     }
                  }
               }
            }
    }
    (void) xf86dactocomm();
    outb(0x3C6, olddaccomm);
    xf86dactopel();
    outb(0x3C6, olddacpel);

    return (found == type);
}
#undef Setcomm

static Bool SC1148x_M2_Probe()
{
    return MISC_HI_COLOR_Probe(SC1148x_M2_DAC);
}

static Bool SS2410_Probe()
{
    return MISC_HI_COLOR_Probe(SS2410_DAC);
}

static Bool ATT20C490_Probe()
{
    return MISC_HI_COLOR_Probe(ATT20C490_DAC);
}

static int MISC_HI_COLOR_PreInit()
{
    /* Verify that depth is supported by ramdac */
    switch(s3RamdacType) {
	case SC1148x_M2_DAC:
    	    if ( s3InfoRec.depth > 16 ) {
		ErrorF("Depths greater than 16bpp are not supported for "
		   	"the Sierra 1148x RAMDAC.\n");
	        return 0;
    	    }
	    break;
	case SS2410_DAC:
    	    if ( s3InfoRec.depth > 24 ) {
		ErrorF("Depths greater than 24bpp are not supported for "
		    "the Diamond SS2410 RAMDAC.\n");
		return 0;
    	     }
	    break;
	case ATT20C490_DAC:
    	    if (s3Bpp > 2) {
		ErrorF("Depths greater than 16bpp are not supported for "
		    "the ATT20C490 RAMDAC.\n");
		return 0;
    	     }
	    break;
	default:
	    ErrorF("Internal error in MISC_HI_COLOR_PreInit().\n");
		return 0;
     }	


    /* Set cursor options */
   	/* none */

    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
	/* No PixMux */

    /* Set Clock options, s3ClockSelectFunc, & maxRawClock 
	for internal clocks. Pass the job to the OtherClocksSetup()
	function for external clocks*/
    OtherClocksSetup();

    s3InfoRec.maxClock = s3InfoRec.dacSpeed;
    /* Halve it for 16bpp (32bpp not supported) */
    if (s3Bpp > 1) {
	 s3InfoRec.maxClock /= 2;
	 maxRawClock /= 2;
    }

    return 1;
}


static void SC1148x_M2_Restore()
{
   xf86setdaccomm(s3DacRegs[0]);
}

static void ATT20C490_Restore()
{
    xf86setdaccomm(s3DacRegs[0]);
}

static void SS2410_Restore()
{
    unsigned char tmp;

    outb( vgaCRIndex, 0x55 );
    tmp = inb( vgaCRReg );
    outb( vgaCRReg, tmp | 1 ); 
    xf86setdaccomm(s3DacRegs[0]);
    outb( vgaCRReg, tmp );
}

static void SC1148x_M2_Save()
{
   s3DacRegs[0] = xf86getdaccomm();
}

static void ATT20C490_Save()
{
   s3DacRegs[0] = xf86getdaccomm();
}

static void SS2410_Save()
{
	unsigned char tmp;

 	outb( vgaCRIndex, 0x55 );
	tmp = inb( vgaCRReg );
	outb( vgaCRReg, tmp | 1 ); 
	s3DacRegs[0] = xf86getdaccomm( );
	outb( vgaCRReg, tmp );
}


/*****************************************************\

  			NORMAL_DAC 

\*****************************************************/

static Bool NORMAL_Probe()
{
    return TRUE;
}

static int NORMAL_PreInit()
{
    /* Verify that depth is supported by ramdac */
    if(s3Bpp > 1) {
	ErrorF("Depths greater than 8bpp are not supported for a "
	"\"normal\" RAMDAC.\n");
	return 0;	/* BAD_DEPTH */
    }

    /* Set cursor options */
   	/* none */

    /* Check if PixMux is supported and set the PixMux
	related flags and variables */
	/* of course not */

    /* If there is an internal clock, set s3ClockSelectFunc, maxRawClock
	numClocks and whatever options need to be set. For external
	clocks, pass the job to OtherClocksSetup() */
    OtherClocksSetup();
    
    /* anything else */
    s3InfoRec.maxClock = s3MaxClock;

    return 1;
}



/**********************************************************\

	THE CLOCKS

\**********************************************************/


void OtherClocksSetup()
{
	/*******************************************\
	|  Clocks that aren't built into the ramdac |
	\*******************************************/

   if (OFLG_ISSET(OPTION_LEGEND, &s3InfoRec.options)) {
      s3ClockSelectFunc = LegendClockSelect;
      numClocks = 32;
/*      maxRawClock = ;		(MArk) what to do here? */
      if (xf86Verbose)
         ErrorF("%s %s: Using Sigma Legend ICS 1394-046 clock generator\n",
		clockchip_probed, s3InfoRec.name);
   } else if (OFLG_ISSET(CLOCK_OPTION_ICD2061A, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      numClocks = 3;
      maxRawClock = 120000;
      if (xf86Verbose)
         ErrorF("%s %s: Using ICD2061A programmable clock\n",
		clockchip_probed, s3InfoRec.name);
   } else if (OFLG_ISSET(CLOCK_OPTION_GLORIA8, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = Gloria8ClockSelect;
      numClocks = 3;
/*      maxRawClock = ;		(MArk) what to do here? */
      if (xf86Verbose)
         ErrorF("%s %s: Using ELSA Gloria-8 ICS9161/TVP3030 programmable " 	
		"clock\n",clockchip_probed, s3InfoRec.name);
   } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      numClocks = 3;
      if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
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
      }
      if (xf86Verbose)
	 ErrorF("%s %s: Using Sierra SC11412 programmable clock\n",
		clockchip_probed, s3InfoRec.name);
   } else if (OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = icd2061ClockSelect;
      numClocks = 3;
      maxRawClock = 145000; /* This is what is in common_hw/ICS2595.h */
      if (xf86Verbose)
	 ErrorF("%s %s: Using ICS2595 programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
   } else if (OFLG_ISSET(CLOCK_OPTION_CH8391, &s3InfoRec.clockOptions)) {
      s3ClockSelectFunc = ch8391ClockSelect;
      numClocks = 3;
      maxRawClock = 135000;
      if (xf86Verbose)
	 ErrorF("%s %s: Using Chrontel 8391 programmable clock\n",
		XCONFIG_GIVEN, s3InfoRec.name);
   } else if (OFLG_ISSET(CLOCK_OPTION_S3GENDAC, &s3InfoRec.clockOptions) ||
	      OFLG_ISSET(CLOCK_OPTION_ICS5342,  &s3InfoRec.clockOptions)) {
      unsigned char saveCR55;
      int m,n,n1,n2, mclk;
      
      s3ClockSelectFunc = s3GendacClockSelect;
      numClocks = 3;
      maxRawClock = 110000;
      
      outb(vgaCRIndex, 0x55);
      saveCR55 = inb(vgaCRReg);
      outb(vgaCRReg, saveCR55 | 1);
      
      outb(0x3C7, 10); /* read MCLK */
      m = inb(0x3C9);
      n = inb(0x3C9);
      
      outb(vgaCRIndex, 0x55);
      outb(vgaCRReg, saveCR55);	 
      
      m &= 0x7f;
      n1 = n & 0x1f;
      n2 = (n>>5) & 0x03;
      mclk = ((1431818 * (m+2)) / (n1+2) / (1 << n2) + 50) / 100;

      if (xf86Verbose)
	 ErrorF("%s %s: Using %s programmable clock (MCLK %1.3f MHz)\n"
		,clockchip_probed, s3InfoRec.name
		,OFLG_ISSET(CLOCK_OPTION_ICS5342, &s3InfoRec.clockOptions)
		? "ICS5342" : "S3 Gendac/SDAC"
		,mclk / 1000.0);
      if (s3InfoRec.s3MClk > 0) {
       if (xf86Verbose)
	 ErrorF("%s %s: using specified MCLK value of %1.3f MHz for DRAM "
 	"timings\n",XCONFIG_GIVEN, s3InfoRec.name, s3InfoRec.s3MClk/1000.0);
      } else {
	 s3InfoRec.s3MClk = mclk;
      }
   } else {
      s3ClockSelectFunc = s3ClockSelect;
      numClocks = 16;
      if (!s3InfoRec.clocks) 
         vgaGetClocks(numClocks, s3ClockSelectFunc);
   }

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
      no &= 0xF;
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
      temp = inb(vgaCRReg) & 0xf0;
      outb(vgaCRReg, temp | no);
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
#if defined(PC98_PW) || defined(PC98_PWLB)
	    AltICD2061SetClock(freq, 0);
#else
	    AltICD2061SetClock(freq, 2);
	    AltICD2061SetClock(freq, 2);
	    AltICD2061SetClock(freq, 2);
#endif
	    if (DAC_IS_TI3026 || DAC_IS_TI3030) {
	       /* 
	        * then we need to setup the loop clock
	        */
	       Ti3026SetClock(freq/1000, 2, s3Bpp, TI_LOOP_CLOCK);
	       s3OutTi3026IndReg(TI_MCLK_LCLK_CONTROL, ~0x20, 0x20);
            }
	 } else if (OFLG_ISSET(CLOCK_OPTION_SC11412, &s3InfoRec.clockOptions)) {
	    result = SC11412SetClock((long)freq/1000);
	 } else if (OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
	    result = ICS2595SetClock((long)freq/1000);
	    result = ICS2595SetClock((long)freq/1000);
	 } else { /* Should never get here */
	    result = FALSE;
	    break;
	 }

	 if (!OFLG_ISSET(CLOCK_OPTION_ICS2595, &s3InfoRec.clockOptions)) {
	    unsigned char tmp;
	    outb(vgaCRIndex, 0x42);/* select the clock */
	    tmp = inb(vgaCRReg) & 0xf0;
	    if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options) &&
                S3_964_SERIES(s3ChipId)) /* SPEA Mercury P64 uses bit2/3  */
                 outb(vgaCRReg, tmp | 0x06);   /* for synchronizing reasons (?) */
            else outb(vgaCRReg, tmp | 0x02); 
            usleep(150000);
	 }
	 /* Do the clock doubler selection in s3Init() */
      }
   }
   LOCK_SYS_REGS;
   return(result);
}




/* the ELSA Gloria-8 uses a TVP3030 with ICS9161 as refclock */

static Bool
Gloria8ClockSelect(freq)
     int   freq;
{
   Bool result = TRUE;
   unsigned char tmp;
   int p;

   UNLOCK_SYS_REGS;
   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = s3ClockSelect(freq);
      break;
   default:
      for(p=0; p<4; p++)
	 if ((freq << p) >= 120000) break;

      AltICD2061SetClock((freq * 1000) >> (3-p), 2);

      Ti3030SetClock(14318 << (3-p), 2, s3Bpp, TI_BOTH_CLOCKS);
      Ti3030SetClock(freq, 2, s3Bpp, TI_LOOP_CLOCK);
      s3OutTi3026IndReg(TI_MCLK_LCLK_CONTROL, ~0x38, 0x30);
      s3OutTi3026IndReg(TI_MCLK_LCLK_CONTROL, ~0x38, 0x38);

      outb(vgaCRIndex, 0x42);/* select the clock */
      tmp = inb(vgaCRReg) & 0xf0;
      outb(vgaCRReg, tmp | 0x06);
   }
   LOCK_SYS_REGS;
   return(result);
}






/* The GENDAC code also works for the SDAC, used for Trio32/Trio64 too */

static Bool
s3GendacClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
   unsigned char tmp;
 
   UNLOCK_SYS_REGS;
   
   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = s3ClockSelect(freq);
      break;
   default:
      {
#if defined(PC98_PW)
	(void) S3gendacSetClock(freq, 7);  /* PW805i can't use reg 2 */
#else

	 if (S3_TRIOxx_SERIES(s3ChipId)) {
	    (void) S3TrioSetClock(freq, 2); /* can't fail */
	 }
	 else {
	    if (OFLG_ISSET(CLOCK_OPTION_ICS5342, &s3InfoRec.clockOptions))
	       (void) ICS5342SetClock(freq, 2); /* can't fail */
	    else
	       (void) S3gendacSetClock(freq, 2); /* can't fail */
#endif
	    outb(vgaCRIndex, 0x42);/* select the clock */
#if defined(PC98_PW)
	    tmp = inb(vgaCRReg) & 0xf0;
	    outb(vgaCRReg, tmp | 0x07);
#else
	    tmp = inb(vgaCRReg) & 0xf0;
	    outb(vgaCRReg, tmp | 0x02);
#endif
	    usleep(150000);
#if !defined(PC98_PW)
	 }
#endif
      }
   }
   LOCK_SYS_REGS;
   return(result);
}



static void
#if NeedFunctionPrototypes
s3ProgramTi3025Clock(
int clk,
unsigned char n,
unsigned char m,
unsigned char p)
#else
s3ProgramTi3025Clock(clk, n, m, p)
int clk;
unsigned char n;
unsigned char m;
unsigned char p;
#endif
{
   /*
    * Reset the clock data index
    */
   s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x00);

   if (clk != TI_MCLK_PLL_DATA) {
      /*
       * Now output the clock frequency
       */
      s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, n);
      s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, m);
      s3OutTiIndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, p | TI_PLL_ENABLE);

      /*
       * And now set up the loop clock for RCLK
       */
      s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, 0x01);
      s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, 0x01);
      s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, p>0 ? p : 1);
      s3OutTiIndReg(TI_MISC_CONTROL, 0x00,
		    TI_MC_LOOP_PLL_RCLK | TI_MC_LCLK_LATCH | TI_MC_INT_6_8_CONTROL);

      /*
       * And finally enable the clock
       */
      s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_PLL);
   } else {
      /*
       * Set MCLK
       */
      s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, n);
      s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, m);
      s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, p | 0x80);
   }
}




static Bool
ti3025ClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
   unsigned char tmp;
 
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
	 (void) Ti3025SetClock(freq, 2, s3ProgramTi3025Clock); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 tmp = inb(vgaCRReg) & 0xf0;
	 outb(vgaCRReg, tmp | 0x02);
	 usleep(150000);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}




static Bool
ti3026ClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
   unsigned char tmp;
 
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
	 if (freq < 13750) {
	    ErrorF("%s %s: Specified dot clock (%.3f) too low for Ti3026/3030",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    result = FALSE;
	    break;
	 }
	 (void) Ti3026SetClock(freq, 2, s3Bpp, TI_BOTH_CLOCKS); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 tmp = inb(vgaCRReg) & 0xf0;
	 outb(vgaCRReg, tmp | 0x02);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}




static Bool
IBMRGBClockSelect(freq)
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
	 if (freq < 16250) {
	    ErrorF("%s %s: Specified dot clock (%.3f) too low for IBM RGB52x",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    result = FALSE;
	    break;
	 }
	 (void)IBMRGBSetClock(freq, 2, s3InfoRec.dacSpeed, s3InfoRec.s3RefClk);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}




static Bool
ch8391ClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
   unsigned char tmp;
 
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
	 if (freq < 8500 || freq > 135000) {
	    ErrorF("%s %s: Specified dot clock (%.3f) out of range for Chrontel 8391",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    result = FALSE;
	    break;
	 }
	 (void) Chrontel8391SetClock(freq, 2); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 tmp = inb(vgaCRReg) & 0xf0;
	 outb(vgaCRReg, tmp | 0x02);
	 usleep(150000);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}




static Bool
att409ClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
   unsigned char tmp;
 
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
	 if (freq < 15000 || freq > 240000) {
	    ErrorF("%s %s: Specified dot clock (%.3f) out of range for ATT20C409",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    result = FALSE;
	    break;
	 }
	 (void) Att409SetClock(freq, 2); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 tmp = inb(vgaCRReg) & 0xf0;
	 outb(vgaCRReg, tmp | 0x02);
	 usleep(150000);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}




static Bool
STG1703ClockSelect(freq)
     int   freq;

{
   Bool result = TRUE;
   unsigned char tmp;
 
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
	 if (freq < 8500 || freq > 135000) {
	    ErrorF("%s %s: Specified dot clock (%.3f) out of range for STG1703",
		   XCONFIG_PROBED, s3InfoRec.name, freq / 1000.0);
	    result = FALSE;
	    break;
	 }
	 (void) STG1703SetClock(freq, 2); /* can't fail */
	 outb(vgaCRIndex, 0x42);/* select the clock */
	 tmp = inb(vgaCRReg) & 0xf0;
	 outb(vgaCRReg, tmp | 0x02);
	 usleep(150000);
      }
   }
   LOCK_SYS_REGS;
   return(result);
}


