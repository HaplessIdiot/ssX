/* $XConsortium: XF86_S3.c,v 1.1 94/03/28 21:22:05 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/XF86_S3.c,v 3.5 1994/07/21 13:56:40 dawes Exp $ */
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
/* Clock limit for non-Bt485, non-Ti3020, non-ATT498 cards */
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
#define MAX_TI3020_CLOCK_175	175000
#define MAX_TI3020_CLOCK_FAST	200000

/* Clock limits for cards with a ATT 20C498 */
#define MAX_ATT498_11_CLOCK	 80000  /* ATT 20C498-11 */
#define MAX_ATT498_11_MUX_CLOCK	110000
#define MAX_ATT498_13_CLOCK	110000  /* ATT 20C498-13 */
#define MAX_ATT498_13_MUX_CLOCK	135000
#define MAX_ATT498_15_CLOCK	110000  /* ATT 20C498-15 */
#define MAX_ATT498_15_MUX_CLOCK	150000
#define MAX_ATT498_17_CLOCK	110000  /* ATT 20C498-17 */
#define MAX_ATT498_17_MUX_CLOCK	170000

int s3MaxClock = MAX_S3_CLOCK;
int s3MaxBt485Clock = MAX_BT485_CLOCK;
int s3MaxBt485MuxClock = MAX_BT485_MUX_CLOCK;
int s3MaxTi3020Clock = MAX_TI3020_CLOCK;
int s3MaxTi3020Clock175 = MAX_TI3020_CLOCK_175;
int s3MaxTi3020ClockFast = MAX_TI3020_CLOCK_FAST;

/* ELSA Winner 1000PRO uses ATT 20C498-13 RAMDAC */
int s3MaxATT498Clock = MAX_ATT498_13_CLOCK;
int s3MaxATT498MuxClock = MAX_ATT498_13_MUX_CLOCK;

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
  MODES,
  OPTION,
  VIDEORAM,
  VIEWPORT,
  VIRTUAL,
  CLOCKPROG,
  BIOSBASE,
  MEMBASE,
  RAMDAC,
  DACSPEED,
  -1
};

#include "xf86ExtInit.h"

