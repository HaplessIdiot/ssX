/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/Ti3025clk.c,v 3.2 1994/09/18 08:49:02 dawes Exp $ */

/*
 * Copyright 1994 The XFree86 Project, Inc
 *
 * programming the on-chip clock on the Ti3025, derived from the
 * S3 gendac code by Jon Tombs
 * Dirk Hohndel <hohndel@aib.com>
 * Robin Cutshaw <robin@paros.com>
 */

#include "s3Ti3020.h" 
#include "compiler.h"
#define NO_OSLIB_PROTOTYPES
#include "xf86_OSlib.h"
#include <math.h>

extern int vgaIOBase;

#ifdef __STDC__
static void
s3ProgramTi3025Clock(int doubleit, unsigned char n, unsigned char m,
                          unsigned char p)
#else
static void
s3ProgramTi3025Clock(doubleit, n, m, p)
unsigned char doubleit;
unsigned char n;
unsigned char m;
unsigned char p;
#endif
{
   /*
    * Reset the clock data index
    */
   s3OutTiIndReg(TI_PLL_CONTROL, 0x00, 0x00);

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
   s3OutTiIndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, p);
   s3OutTiIndReg(TI_MISC_CONTROL, 0x00,
                TI_MC_LOOP_PLL_RCLK | TI_MC_LCLK_LATCH | TI_MC_INT_6_8_CONTROL);

   /*
    * Set a standard MCLK (109.7MHz / 2)
    */
   s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, 0x16);
   s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, 0x15);
   s3OutTiIndReg(TI_MCLK_PLL_DATA, 0x00, 0x80);

   /*
    * And finally enable the clock
    */
   s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00,
      doubleit ? TI_ICLK_CLK1_DOUBLE : TI_ICLK_CLK1);
}

#ifdef __STDC__
void Ti3025SetClock(long freq, int clk)
#else
void
Ti3025SetClock(freq, clk)
long freq;
int clk;
#endif
{
   float ffreq;
   unsigned char n, p, m;
   int doubleit;
   float nf, pf, mf;
   float  max_error = 0.05;  /* ~ within 1% at 100MHz */

   ffreq = (float) freq;

   if (ffreq > 100000.0) {
      ffreq /= 2.0;
      doubleit = 1;
   } else
      doubleit = 0;

   /* work out suitable timings */

   /* pick the right p value */

   if (ffreq > 110000.0) {
      p = 0;
   } else if (ffreq > 55000.0) {
      p = 1;
      ffreq *= 2.0;
   } else if (ffreq > 27500.0) {
      p = 2;
      ffreq *= 4.0;
   } else {
      p = 3;
      ffreq *= 8.0;
   }
   /* now 110000 <= ffreq <= 220000 */   

   ffreq /= TI_REF_FREQ * 1000;

   /* the remaining formula is  ffreq = (m+2)*8 / (n+2) */
   /* mf and nf are the `+2' values */

   while (1) {
      for (nf = 3.0; nf < 28.0; nf++) {  /* to have Fref/(n+2)>0.5MHz */
         for (mf = 3.0; mf < 129.0; mf++) {      
           float div = (8.0 * mf)/nf;
           
           if (div > (ffreq + max_error))   /* next n */
              break;
           if (div < (ffreq - max_error))
              continue; 
           n = nf - 2.0;
           m = mf - 2.0;
#ifdef DEBUG
           ErrorF("clk %d, setting to %f, n %02x, m %02x, p %d\n", clk,
                     8.0/(float)(1 << (int)p)*(mf/nf) * TI_REF_FREQ,
                     (int )n, (int )m, (int )p);
#endif
           s3ProgramTi3025Clock(doubleit, n, m, p);
           return;
         }
      }
      /* try again with a bigger error */
      max_error += 0.05;
   }
}
