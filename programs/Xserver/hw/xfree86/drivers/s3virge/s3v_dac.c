/* $XFree86$ */

/*
 *
 * Copyright 1998 The XFree86 Project, Inc.
 *
 */


/*
 * s3v_dac.c
 * Port to 4.0 design level
 *
 * S3 ViRGE driver
 *
 *
 * commonCalcClock from S3gendac.c in pre 4.0 tree.
 *
 */

#include	"s3v.h"


#define BASE_FREQ         14.31818   /* MHz */

	/* proto */
void
commonCalcClock(long freq, int min_m, int min_n1, int max_n1, int min_n2, int max_n2, 
		long freq_min, long freq_max,
		unsigned char * mdiv, unsigned char * ndiv);



	/* function */
void
commonCalcClock(long freq, int min_m, int min_n1, int max_n1, int min_n2, int max_n2, 
		long freq_min, long freq_max,
		unsigned char * mdiv, unsigned char * ndiv)
{
   double ffreq, ffreq_min, ffreq_max;
   double div, diff, best_diff;
   unsigned int m;
   unsigned char n1, n2;
   unsigned char best_n1=16+2, best_n2=2, best_m=125+2;

   ffreq     = freq     / 1000.0 / BASE_FREQ;
   ffreq_min = freq_min / 1000.0 / BASE_FREQ;
   ffreq_max = freq_max / 1000.0 / BASE_FREQ;

   if (ffreq < ffreq_min / (1<<max_n2)) {
      ErrorF("invalid frequency %1.3f MHz  [freq >= %1.3f MHz]\n", 
	     ffreq*BASE_FREQ, ffreq_min*BASE_FREQ / (1<<max_n2));
      ffreq = ffreq_min / (1<<max_n2);
   }
   if (ffreq > ffreq_max / (1<<min_n2)) {
      ErrorF("invalid frequency %1.3f MHz  [freq <= %1.3f MHz]\n", 
	     ffreq*BASE_FREQ, ffreq_max*BASE_FREQ / (1<<min_n2));
      ffreq = ffreq_max / (1<<min_n2);
   }

   /* work out suitable timings */

   best_diff = ffreq;
   
   for (n2=min_n2; n2<=max_n2; n2++) {
      for (n1 = min_n1+2; n1 <= max_n1+2; n1++) {
	 m = (int)(ffreq * n1 * (1<<n2) + 0.5) ;
	 if (m < min_m+2 || m > 127+2) 
	    continue;
	 div = (double)(m) / (double)(n1);	 
	 if ((div >= ffreq_min) &&
	     (div <= ffreq_max)) {
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
   
#if EXTENDED_DEBUG
   ErrorF("Clock parameters for %1.6f MHz: m=%d, n1=%d, n2=%d\n",
	  ((double)(best_m) / (double)(best_n1) / (1 << best_n2)) * BASE_FREQ,
	  best_m-2, best_n1-2, best_n2);
#endif
  
   if (max_n1 == 63)
      *ndiv = (best_n1 - 2) | (best_n2 << 6);
   else
      *ndiv = (best_n1 - 2) | (best_n2 << 5);
   *mdiv = best_m - 2;
}

