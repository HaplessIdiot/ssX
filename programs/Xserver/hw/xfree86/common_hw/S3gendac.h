/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/S3gendac.h,v 3.1 1994/09/26 15:32:17 dawes Exp $ */
/* Jon Tombs <jon@esix2.us.es>  */


#define GENDAC_INDEX	     0x3C8
#define GENDAC_DATA	     0x3C9
#define BASE_GENDAC_FREQ     14318.18   /* KHz */
#define FREQ_GENDAC_MAX     250000.0   
#define FREQ_GENDAC_MIN     100000.0

int S3gendacSetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

int ICS5342SetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     
