/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/S3gendac.c,v 3.3 1995/01/10 10:25:14 dawes Exp $ */
/*
 * Progaming of the S3 gendac programable clocks, from the S3 Gendac
 * programing documentation by S3 Inc. 
 * Jon Tombs <jon@esix2.us.es>
 */
 
#include "S3gendac.h" 
#include "compiler.h"
#define NO_OSLIB_PROTOTYPES
#include "xf86_OSlib.h"
#include <math.h>

extern int vgaIOBase;
static int min_n2;

static void setdacpll(
#if NeedFunctionPrototypes
int reg, unsigned char data1, unsigned char data2
#endif
);

static int commonSetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int
S3gendacSetClock(freq, clk)
long freq;
int clk;
{
   min_n2 = 0;
   return commonSetClock(freq, clk);
}

int
ICS5342SetClock(freq, clk)
long freq;
int clk;
{
   min_n2 = 1;
   return commonSetClock(freq, clk);
}

static int
commonSetClock(freq, clk)
long freq;
int clk;
{
   double ffreq;
   double div, diff, best_diff;
   unsigned int m, m0;
   unsigned char n, n1, n2;
   unsigned char best_n1=16+2, best_n2=2, best_m=125+2;

   ffreq = (double) freq;

   if (ffreq < FREQ_GENDAC_MIN/8) {
      ErrorF("invalid frequency %1.3f MHz  [freq >= %1.3f MHz]\n", 
	     ffreq/1e3,FREQ_GENDAC_MIN/8/1e3);
      ffreq = FREQ_GENDAC_MIN/8;
   }
   if (ffreq > FREQ_GENDAC_MAX) {
      ErrorF("invalid frequency %1.3f MHz  [freq <= %1.3f MHz]\n", 
	     ffreq/1e3,FREQ_GENDAC_MAX/1e3);
      ffreq = FREQ_GENDAC_MAX;
   }

   /* work out suitable timings */

   ffreq /= BASE_GENDAC_FREQ;
   best_diff = ffreq;
   
   for (n2=min_n2; n2<=3; n2++) {
      for (n1 = 1+2; n1 <= 31+2; n1++) {
	 m0 = ffreq * (1<<n2) * n1;
	 for (m = m0-1; m <= m0+1; m++) {	 
	    if (m < 1+3 || m > 127+2) 
	       continue;
	    div = (double)(m) / (double)(n1);	 
	    if ((div >= FREQ_GENDAC_MIN/BASE_GENDAC_FREQ) &&
		(div <= FREQ_GENDAC_MAX/BASE_GENDAC_FREQ)) {
	       diff = ffreq - div / (1<<n2);
	       if (diff < 0.0) 
		  diff = -diff;
	       if (diff < best_diff) {
		  best_diff = diff;
		  best_m    = m;
		  best_n1   = n1;
		  best_n2   = n2;
	       }
	    }
	 }
      }
   }
   
#if 0
   ErrorF("clk %d, setting to %1.3f MHz\n", clk,
	  ((double)(best_m) / (double)(best_n1) / (1 << best_n2)) * BASE_GENDAC_FREQ / 1e3);
#endif

   n = (best_n1 - 2) | (best_n2 << 5);
   m = best_m - 2;
   setdacpll(clk, m, n);

   return 0;
}	   


static void
setdacpll(reg, data1, data2)
int reg;
unsigned char data1;
unsigned char data2;
{
   unsigned char tmp, tmp1;
   int vgaCRIndex = vgaIOBase + 4;
   int vgaCRReg = vgaIOBase + 5;
		
   /* set RS2 via CR55, yuck */
   outb(vgaCRIndex, 0x55);
   tmp = inb(vgaCRReg) & 0xFC;
   outb(vgaCRReg, tmp | 0x01);  
   tmp1 = inb(GENDAC_INDEX);

   outb(GENDAC_INDEX, reg);
   outb(GENDAC_DATA, data1);
   outb(GENDAC_DATA, data2);

   /* Now clean up our mess */
   outb(GENDAC_INDEX, tmp1);  
   outb(vgaCRReg, tmp);


}
