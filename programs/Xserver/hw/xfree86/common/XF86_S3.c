/* $XConsortium: XF86_S3.c,v 1.1 94/03/28 21:22:05 dpw Exp $ */
/* $XFree86$ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec s3InfoRec;

/*
 * This limit is set to 110MHz because this is the limit for
 * the ramdacs used on many S3 cards Increasing this limit
 * could result in damage to your hardware.
 */
/* Clock limit for non-Bt485, non-Ti3020 cards */
#define MAX_S3_CLOCK    110000

/*
 * This limit is currently set to 85MHz because this is the limit for
 * the Bt485 ramdac when running in 1:1 mode.  It will be increased when
 * support for using the ramdac in 4:1 mode.  Increasing this limit
 * could result in damage to your hardware.
 */

/* Clock limit for cards with a Bt485 */
#define MAX_BT485_CLOCK		 85000
/* Clock limit for Bt485 cards where we support pixel multiplexing */
#define MAX_BT485_MUX_CLOCK	135000

/* Clock limits for cards with a Ti3020 */
#define MAX_TI3020_CLOCK	135000
#define MAX_TI3020_CLOCK_FAST	200000

int s3MaxClock = MAX_S3_CLOCK;
int s3MaxBt485Clock = MAX_BT485_CLOCK;
int s3MaxBt485MuxClock = MAX_BT485_MUX_CLOCK;
int s3MaxTi3020Clock = MAX_TI3020_CLOCK;
int s3MaxTi3020ClockFast = MAX_TI3020_CLOCK_FAST;

ScrnInfoPtr xf86Screens[] = 
{
  &s3InfoRec,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  ACCEL,
  -1
};

int s3ValidTokens[] =
{
  STATICGRAY,
  GRAYSCALE,
  STATICCOLOR,
  PSEUDOCOLOR,
  TRUECOLOR,
  DIRECTCOLOR,
  CHIPSET,
  CLOCKS,
  DISPLAYSIZE,
  MODES,
  OPTION,
  VIDEORAM,
  VIEWPORT,
  VIRTUAL,
  CLOCKPROG,
  BIOSBASE,
  MEMBASE,
  -1
};

/* Dummy function for PEX in LinkKit */

#if defined(LINKKIT) && !defined(PEXEXT)
PexExtensionInit() {}
#endif

/* Dummy function for XIE in LinkKit */

#if defined(LINKKIT) && !defined(XIE)
XieInit() {}
#endif
