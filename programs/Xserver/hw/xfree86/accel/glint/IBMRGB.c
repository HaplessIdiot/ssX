/* $XFree86: $ */
/*
 * Copyright 1995 The XFree86 Project, Inc
 *
 * programming the on-chip clock on the IBM RGB52x
 * Harald Koenig <koenig@tat.physik.uni-tuebingen.de>
 */
/* $XConsortium: IBMRGB.c /main/5 1996/05/07 17:13:25 kaleb $ */

#include "Xfuncproto.h"
#include "compiler.h"
#define NO_OSLIB_PROTOTYPES
#include "xf86_OSlib.h"
#include "xf86.h"
#include "xf86_Config.h"

#include "glint.h"
#include "glint_regs.h"
#define GLINT_SERVER
#include "IBMRGB.h" 

extern Bool glintDoubleBufferMode;
extern int coprotype;
/*
 * glintOutIBMRGBIndReg() and glintInIBMRGBIndReg() are used to access 
 * the indirect RGB52x registers only.
 */

void glintOutIBMRGBIndReg(unsigned char reg, unsigned char mask, unsigned char data)
{
   unsigned char tmp, tmp2 = 0x00;

   GLINT_SLOW_WRITE_REG(reg, IBMRGB_INDEX_LOW);

   if (mask != 0x00)
      tmp2 = GLINT_READ_REG(IBMRGB_INDEX_DATA) & mask;
   GLINT_SLOW_WRITE_REG(tmp2 | data, IBMRGB_INDEX_DATA);
}

unsigned char glintInIBMRGBIndReg(unsigned char reg)
{
   volatile   unsigned char tmp, ret;

   GLINT_SLOW_WRITE_REG(reg, IBMRGB_INDEX_LOW);
   ret = GLINT_READ_REG(IBMRGB_INDEX_DATA);

   return(ret);
}

static void
glintProgramIBMRGBClock(int clk, unsigned char m, unsigned char n, 
		     unsigned char df)
{
   int tmp;
   tmp = 1;

   glintOutIBMRGBIndReg(IBMRGB_misc_clock, ~1, 1);

   glintOutIBMRGBIndReg(IBMRGB_m0+2*clk, 0, (df<<6) | (m&0x3f));
   glintOutIBMRGBIndReg(IBMRGB_n0+2*clk, 0, n);

   glintOutIBMRGBIndReg(IBMRGB_pll_ctrl2, 0xf0, clk);
   glintOutIBMRGBIndReg(IBMRGB_pll_ctrl1, 0xf8, 3);
}

void IBMRGBGlintSetClock(long freq, int clk, long dacspeed, long fref)
{
   volatile   double ffreq,fdacspeed,ffref;
   volatile   int df, n, m, max_n, min_df;
   volatile   int best_m=69, best_n=17, best_df=0;
   volatile   double  diff, mindiff;
   
#define FREQ_MIN   16250  /* 1000 * (0+65) / 4 */
#define FREQ_MAX  dacspeed

   if (freq < FREQ_MIN)
      ffreq = FREQ_MIN / 1000.0;
   else if (freq > FREQ_MAX)
      ffreq = FREQ_MAX / 1000.0;
   else
      ffreq = freq / 1000.0;
   
   fdacspeed = dacspeed / 1e3;
   ffref = fref / 1e3;

   ffreq /= ffref;
   ffreq *= 16;
   mindiff = ffreq;

   if (freq <= dacspeed/4) 
      min_df = 0;
   else if (freq <= dacspeed/2) 
      min_df = 1;
   else 
      min_df = 2;

   for(df=0; df<4; df++) {
      ffreq /= 2;
      mindiff /= 2;
      if (df < min_df) continue;

      /* the remaining formula is  ffreq = (m+65) / n */

      if (df < 3) max_n = fref/1000/2;
      else        max_n = fref/1000;
      if (max_n > 31)  max_n = 31;
      
      for (n=2; n <= max_n; n++) {
	 m = (int)(ffreq * n + 0.5) - 65;
	 if (m < 0)
	    m = 0;
	 else if (m > 63) 
	    m = 63;
	 
	 diff = (m+65.0) / n - ffreq;
	 if (diff<0)
	    diff = -diff;
	 
	 if (diff < mindiff) {
	    mindiff = diff;
	    best_n = n;
	    best_m = m;
	    best_df = df;
	 }
      }
   }

#ifdef DEBUG
   ErrorF("clk %d, setting to %f, m 0x%02x %d, n 0x%02x %d, df %d\n", clk,
	  ((best_m+65.0)/best_n) / (8>>best_df) * ffref,
	  best_m, best_m, best_n, best_n, best_df);
#endif

   glintProgramIBMRGBClock(clk, best_m, best_n, best_df);
}

int glintIBMRGB_Probe()
{
   unsigned char CR43, CR55, dac[3], lut[6];
   unsigned char ilow, ihigh, id, rev, id2, rev2;
   int i,j;
   int ret=0;

#if 0
   /* save DAC and first LUT entries */
   for (i=0; i<3; i++) 
      dac[i] = GLINT_READ_REG(IBMRGB_PIXEL_MASK+i);
   for (i=j=0; i<2; i++) {
      GLINT_SLOW_WRITE_REG(i, IBMRGB_READ_ADDR);
      lut[j++] = GLINT_READ_REG(IBMRGB_RAMDAC_DATA);
      lut[j++] = GLINT_READ_REG(IBMRGB_RAMDAC_DATA);
      lut[j++] = GLINT_READ_REG(IBMRGB_RAMDAC_DATA);
   }

   glintIBMRGB_Init();
#endif

   /* read ID and revision */
   ilow  = GLINT_READ_REG(IBMRGB_INDEX_LOW);
   ihigh = GLINT_READ_REG(IBMRGB_INDEX_HIGH);
   GLINT_SLOW_WRITE_REG(0, IBMRGB_INDEX_HIGH);  /* index high */
   GLINT_SLOW_WRITE_REG(IBMRGB_rev, IBMRGB_INDEX_LOW);
   rev = GLINT_READ_REG(IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_id, IBMRGB_INDEX_LOW);
   id  = GLINT_READ_REG(IBMRGB_INDEX_DATA);

   /* known IDs:  
      1 = RGB525
      2 = RGB524, RGB528 
      */
      
   if (id >= 1 && id <= 2) { 
      /* check if ID and revision are read only */
      GLINT_SLOW_WRITE_REG(IBMRGB_rev, IBMRGB_INDEX_LOW);
      GLINT_SLOW_WRITE_REG(~rev, IBMRGB_INDEX_DATA);
      GLINT_SLOW_WRITE_REG(IBMRGB_id, IBMRGB_INDEX_LOW);
      GLINT_SLOW_WRITE_REG(~id, IBMRGB_INDEX_DATA);
      GLINT_SLOW_WRITE_REG(IBMRGB_rev, IBMRGB_INDEX_LOW);
      rev2 = GLINT_READ_REG(IBMRGB_INDEX_DATA);
      GLINT_SLOW_WRITE_REG(IBMRGB_id, IBMRGB_INDEX_LOW);
      id2  = GLINT_READ_REG(IBMRGB_INDEX_DATA);
      
      if (id == id2 && rev == rev2) {  /* IBM RGB52x found */
	 ret = (id<<8) | rev;

	 /* check for 128bit VRAM -> RGB528 */
	 GLINT_SLOW_WRITE_REG(IBMRGB_misc1, IBMRGB_INDEX_LOW);
	 if ((GLINT_READ_REG(IBMRGB_INDEX_DATA) & 0x03) == 0x03)  
	    /* 128bit DAC found */
	    ret |= 1<<16;
      }
      else {
	 GLINT_SLOW_WRITE_REG(IBMRGB_rev, IBMRGB_INDEX_LOW);
	 GLINT_SLOW_WRITE_REG(rev, IBMRGB_INDEX_DATA);
	 GLINT_SLOW_WRITE_REG(IBMRGB_id, IBMRGB_INDEX_LOW);
	 GLINT_SLOW_WRITE_REG(id, IBMRGB_INDEX_DATA);
      }
   }   
   GLINT_SLOW_WRITE_REG(ilow, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(ihigh, IBMRGB_INDEX_HIGH);

#if 0
   /* restore DAC and first LUT entries */
   for (i=j=0; i<2; i++) {
      GLINT_SLOW_WRITE_REG(i, IBMRGB_WRITE_ADDR);
      GLINT_SLOW_WRITE_REG(lut[j++], IBMRGB_RAMDAC_DATA);
      GLINT_SLOW_WRITE_REG(lut[j++], IBMRGB_RAMDAC_DATA);
      GLINT_SLOW_WRITE_REG(lut[j++], IBMRGB_RAMDAC_DATA);
   }
   for (i=0; i<3; i++) 
      GLINT_SLOW_WRITE_REG(dac[i], IBMRGB_PIXEL_MASK+i);
#endif
   
   return ret;
}


glintIBMRGB_Init()
{
   unsigned char CR55, tmp;

   tmp = GLINT_READ_REG(IBMRGB_INDEX_CONTROL);
   /* turn off auto-increment */
   GLINT_SLOW_WRITE_REG(tmp & ~1, IBMRGB_INDEX_CONTROL);  
   GLINT_SLOW_WRITE_REG(0, IBMRGB_INDEX_HIGH);        /* index high byte */
}

int IBMRGB52x_Init(DisplayModePtr mode)
{
      unsigned char tmp, tmp2;

	/*
	 * the GLINT uses SCLK so there's no need to set the
	 * DDOTCLK to a special value
	 * we disable the DDOTCLK
	 */
      if (mode->Flags & V_DBLCLK)
	 glintOutIBMRGBIndReg(IBMRGB_misc_clock, 0xf0, 0x83);
      else
	 glintOutIBMRGBIndReg(IBMRGB_misc_clock, 0xf0, 0x87);

      glintOutIBMRGBIndReg(IBMRGB_sync, 0, 0);
      glintOutIBMRGBIndReg(IBMRGB_hsync_pos, 0, 0);
      glintOutIBMRGBIndReg(IBMRGB_pwr_mgmt, 0, 0x08); /* disable DDOTCLK */
      glintOutIBMRGBIndReg(IBMRGB_dac_op, 0, 0); /* Fast DAC mode */
      glintOutIBMRGBIndReg(IBMRGB_pal_ctrl, 0, 0);
      glintOutIBMRGBIndReg(IBMRGB_sysclk, 0, 0x41);
      if (coprotype == PCI_CHIP_3DLABS_500TX) {
	      /* uses 64bit wide VRAM
	       * set VRAM size to 128/64 bit and disable VRAM mask */
	      glintOutIBMRGBIndReg(IBMRGB_misc1, 0, 0x31);
	      glintOutIBMRGBIndReg(IBMRGB_misc2, ~0x08, 0x47);
      } else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
      	      /* uses 32bit SDRAM
	       * set 32bit and use LCK */
	      glintOutIBMRGBIndReg(IBMRGB_misc1, 0, 0x30);
	      glintOutIBMRGBIndReg(IBMRGB_misc2, 0, 0x07);
     }
     if (glintInfoRec.bitsPerPixel == 32) {			/* 32 bpp */
	glintOutIBMRGBIndReg(IBMRGB_pix_fmt, 0xf8, 6);
	glintOutIBMRGBIndReg(IBMRGB_32bpp, 0, 3);
     } else if (glintInfoRec.bitsPerPixel == 16) {             /* 16 bpp 555 */
	glintOutIBMRGBIndReg(IBMRGB_pix_fmt, 0xf8, 4);
	glintOutIBMRGBIndReg(IBMRGB_16bpp, 0, 0xC5);
     } else {                                        /*  8 bpp */
	glintOutIBMRGBIndReg(IBMRGB_pix_fmt, 0xf8, 3);
	glintOutIBMRGBIndReg(IBMRGB_8bpp, 0, 0);
     }

     if (glintDoubleBufferMode)
	glintOutIBMRGBIndReg(IBMRGB_misc3, 0, 1);

#if 0
      if (OFLG_ISSET(OPTION_IBMRGB_CURS, &glintInfoRec.options)) {
	 /* enable interlaced cursor;
	    not very useful without CR45 bit 5 set, but anyway */
	 if (mode->Flags & V_INTERLACE) {
	    static int already = 0;
	    if (!already) {
	       already++;
	       ErrorF("%s %s: IBMRGB hardware cursor in interlaced modes "
		      "doesn't work correctly,\n"
		      "\tplease use Option \"sw_cursor\" when using "
		      "interlaced modes!\n"
		      ,XCONFIG_PROBED, glintInfoRec.name);
	    }
	    glintOutIBMRGBIndReg(TI_CURS_CONTROL, ~0x60, 0x60);
	 }
	 else
	    glintOutIBMRGBIndReg(TI_CURS_CONTROL, ~0x60, 0x00);
      }
#endif
      return 1;
}

int glintIBMRGB52x_PreInit()
{
    /* Verify that depth is supported by ramdac */
	/* all are supported */

    /* Set cursor options */
    if (OFLG_ISSET(OPTION_SW_CURSOR, &glintInfoRec.options)) {
         ErrorF("%s %s: Use of IBM RGB52x cursor disabled in XF86Config\n",
	        XCONFIG_GIVEN, glintInfoRec.name);
	 OFLG_CLR(OPTION_IBMRGB_CURS, &glintInfoRec.options);
    } else {
	 /* use the ramdac cursor by default */
	 ErrorF("%s %s: Using hardware cursor from IBM RGB52x RAMDAC\n",
	        OFLG_ISSET(OPTION_IBMRGB_CURS, &glintInfoRec.options) ?
		XCONFIG_GIVEN : XCONFIG_PROBED, glintInfoRec.name);
	 OFLG_SET(OPTION_IBMRGB_CURS, &glintInfoRec.options);
    }
   
	/*
	 * fixed refclk of 40MHz
	 */
      glintInfoRec.s3RefClk = 40000;

      if (xf86Verbose) {
	 ErrorF("%s %s: Using IBM RGB52x programmable clock",
		XCONFIG_PROBED, glintInfoRec.name);
	 ErrorF("\n");	
	 ErrorF("%s %s: with refclock %1.3f MHz \n",
		XCONFIG_PROBED,glintInfoRec.name,glintInfoRec.s3RefClk/1e3);
      }

   glintInfoRec.maxClock = glintInfoRec.dacSpeeds[0];

   return 1;
}
