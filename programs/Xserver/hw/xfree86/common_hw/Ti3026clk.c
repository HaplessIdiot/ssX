/* $XFree86$ */
/*
 * Copyright 1995 The XFree86 Project, Inc
 *
 * programming the on-chip clock on the Ti3026, derived from the
 * S3 gendac code by Jon Tombs
 * Harald Koenig <koenig@tat.physik.uni-tuebingen.de>
 */

#include "Ti302X.h" 
#include "compiler.h"
#define NO_OSLIB_PROTOTYPES
#include "xf86_OSlib.h"
#include <math.h>

#ifdef __STDC__
static void
s3ProgramTi3026Clock(int clk, unsigned char n, unsigned char m,
                          unsigned char p)
#else
static void
s3ProgramTi3026Clock(clk, n, m, p)
int clk;
unsigned char n;
unsigned char m;
unsigned char p;
#endif
{
   int tmp;
   /*
    * Reset the clock data index
    */
   s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);

   if (clk != TI_MCLK_PLL_DATA) {
      /*
       * Now output the clock frequency
       */
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (n & 0x3f) | 0x80);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (m & 0x3f) );
      tmp = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA) & 0x40;
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (p & 3) | tmp | 0xb0);

      /* wait until PLL is locked */
      for (tmp=0; tmp<10000; tmp++) 
	 if (s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA) & 0x40) 
	    break;

#if 1
      /*
       * And now set up the loop clock for RCLK
       */
      s3OutTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, 63 | 0x80);
      s3OutTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, 63);
      s3OutTi3026IndReg(TI_LOOP_CLOCK_PLL_DATA, 0x00, (p>0 ? p : 0) | 0xb0);
#if 0
      s3OutTi3026IndReg(TI_MISC_CONTROL, 0x00,
		    TI_MC_LOOP_PLL_RCLK | TI_MC_LCLK_LATCH | 
		    TI_MC_INT_6_8_CONTROL);
#endif
#endif

      /* select pixel clock PLL as dot clock soure */
      s3OutTi3026IndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_PLL);
   }
   else {
      int pn, pm, pp, csr;

      /* select pixel clock PLL as dot clock soure */
      csr = s3InTi3026IndReg(TI_INPUT_CLOCK_SELECT);
      s3OutTi3026IndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_PLL);

      /* save the old dot clock PLL settings */
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
      pn = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA);
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x01);
      pm = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA);
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x02);
      pp = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA);

      /* programm dot clock PLL to new MCLK frequency */
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (n & 0x3f) | 0x80);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (m & 0x3f) );
      tmp = s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA) & 0x40;
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (p & 3) | tmp | 0xb0);

      for (tmp=0; tmp<10000; tmp++) /* wait until PLL is locked */
	 if (s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA) & 0x40) 
	    break;

      /* switch to output dot clock on the MCLK terminal */
      s3OutTi3026IndReg(0x39, 0xe7, 0x00);
      s3OutTi3026IndReg(0x39, 0xe7, 0x08);
      
      /* Set MCLK */
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
      s3OutTi3026IndReg(TI_MCLK_PLL_DATA, 0x00, (n & 0x3f) | 0x80);
      s3OutTi3026IndReg(TI_MCLK_PLL_DATA, 0x00, (m & 0x3f));
      s3OutTi3026IndReg(TI_MCLK_PLL_DATA, 0x00, (p & 3) | 0xb0);

      /* wait until PLL is locked */
      for (tmp=0; tmp<10000; tmp++) 
	 if (s3InTi3026IndReg(TI_MCLK_PLL_DATA) & 0x40) 
	    break;

      /* switch to output MCLK on the MCLK terminal */
      s3OutTi3026IndReg(0x39, 0xe7, 0x10);
      s3OutTi3026IndReg(0x39, 0xe7, 0x18);

      /* restore dot clock PLL */
      s3OutTi3026IndReg(TI_PLL_CONTROL, 0x00, 0x00);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, pn);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, pm);
      s3OutTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA, 0x00, pp);

      /* wait until PLL is locked */
      for (tmp=0; tmp<10000; tmp++) 
	 if (s3InTi3026IndReg(TI_PIXEL_CLOCK_PLL_DATA) & 0x40) 
	    break;
      s3OutTi3026IndReg(TI_INPUT_CLOCK_SELECT, 0x00, csr);
   }
}

#ifdef __STDC__
void Ti3026SetClock(long freq, int clk)
#else
void
Ti3026SetClock(freq, clk)
long freq;
int clk;
#endif
{
volatile   double ffreq;
volatile   int n, p, m;
volatile   int best_n=32, best_m=32;
volatile   double  diff, mindiff;

#define FREQ_MIN   13767  /* ~110000 / 8 */
#define FREQ_MAX  220000

   if (freq < FREQ_MIN)
      ffreq = FREQ_MIN / 1000.0;
   else if (freq > FREQ_MAX)
      ffreq = FREQ_MAX / 1000.0;
   else
      ffreq = freq / 1000.0;
   


   /* work out suitable timings */

   /* pick the right p value */

   for(p=0; p<4 && ffreq < 110.0; p++)
      ffreq *= 2;
   
   /* now 110.0 <= ffreq <= 220.0 */   
   
   ffreq /= TI_REF_FREQ;
   
   /* now 7.6825 <= ffreq <= 15.3650 */
   /* the remaining formula is  ffreq = (65-m)*8 / (65-n) */
   
   
   mindiff = ffreq;
   
   for (n=63; n >= 65 - (int)(TI_REF_FREQ/0.5); n--) {
      m = 65 - (int)(ffreq * (65-n) / 8.0 + 0.5);
      if (m < 1)
	 m = 1;
      else if (m > 63) 
	 m = 63;
      
      diff = ((65-m) * 8) / (65.0-n) - ffreq;
      if (diff<0)
	 diff = -diff;
      
      if (diff < mindiff) {
	 mindiff = diff;
	 best_n = n;
	 best_m = m;
      }
   }

#ifdef DEBUG
   ErrorF("clk %d, setting to %f, n %02x %d, m %02x %d, p %d\n", clk,
	  8.0/(1 << p)*((65.0-best_m)/(65-best_n)) * TI_REF_FREQ,
	  best_n, best_n, best_m, best_m, p);
#endif

   s3ProgramTi3026Clock(clk, best_n, best_m, p);
}
