/* $XFree86$ */
/* Id: XF86_P9000.c,v 4.0 1994/05/28 01:26:10 nygren Exp */

#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec p9000InfoRec;

/*
 * This limit is set to a value which is typical for many of the ramdacs
 * used on P9000 cards.  Increasing this limit could result in damage to
 * to your hardware.
 */
/* XXXX This value needs to be checked (currently using 135 MHz) */
#define MAX_P9000_CLOCK 135000

int p9000MaxClock = MAX_P9000_CLOCK;

ScrnInfoPtr xf86Screens[] = 
{
  &p9000InfoRec,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  ACCEL,
  -1
};

int p9000ValidTokens[] =
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

/* Dummy function for PEX in LinkKit and mono server */

#if defined(LINKKIT) && !defined(PEXEXT)
PexExtensionInit() {}
#endif

