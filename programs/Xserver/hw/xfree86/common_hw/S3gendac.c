/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/S3gendac.c,v 3.0 1994/06/12 16:36:58 dawes Exp $ */
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

static void setdacpll(
#if NeedFunctionPrototypes
int reg, unsigned char data1, unsigned char data2
#endif
);

int
S3gendacSetClock(freq, clk)
long freq;
int clk;
{
   float ffreq;
   unsigned char n, n1, n2, m;
   float n1f, n2f, mf;
   float  max_error = 0.05;  /* ~ within 1% at 100MHz */

   ffreq = (float) freq;

   /* changed check -- this may be used for the SDAC also (Bernhard) */
   if (ffreq < 20000.0 /* || ffreq > 110000.0 */) {
      fprintf(stderr, "invalid frequency %f. [freq>20000]\n", ffreq);
      return 3;
   }

   /* work out suitable timings */

   /* output divider */
   if (ffreq < 40000.0) {
     n2f = 1.0;
     ffreq *= 2;
   } else {
     n2f = 0.0; 
   }
   
   ffreq /= BASE_GENDAC_FREQ;

   while (1) {
      for (n1f = 4.0; n1f < 33.0; n1f++) {
         for (mf = 3.0; mf < 129.0; mf++) {	 
           float div = mf/n1f;
	   
           if (div > (ffreq + max_error)) /* next n1 */
	      break;
           if ((div > FREQ_GENDAC_MAX/BASE_GENDAC_FREQ) ||
	       (div < FREQ_GENDAC_MIN/BASE_GENDAC_FREQ))
	      continue; /* out of spec */
	   if (fabs(div - ffreq) < max_error) {          
              ErrorF("clk %d, setting to %f\n", clk,
		     (mf/n1f) * BASE_GENDAC_FREQ);
              n1 = n1f - 2.0;
              n2 = n2f;
              n = n1 | n2 <<5;
              m = mf - 2.0;
	      setdacpll(clk, m, n);
              return 0;
	   }	   
	 }
      }
      /* try again with a bigger error */
      max_error += 0.05;
   }
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
