/* $XFree86$ */
/* Jon Tombs <jon@esix2.us.es>  */


#define GENDAC_INDEX	     0x3C8
#define GENDAC_DATA	     0x3C9
#define BASE_GENDAC_FREQ     14318.18   /* KHz */
#define FREQ_GENDAC_MAX     250000.0   
#define FREQ_GENDAC_MIN      50000.0

int S3gendacSetClock( 
#if NeedFunctionPrototypes
   long freq, int clock
#endif
);     

