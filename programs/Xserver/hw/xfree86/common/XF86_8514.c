/* $XConsortium: XF86_8514.c,v 1.1 94/03/28 21:21:28 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/XF86_8514.c,v 3.0 1994/05/30 08:24:25 dawes Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec ibm8514InfoRec;

ScrnInfoPtr xf86Screens[] = 
{
  &ibm8514InfoRec,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  ACCEL,
  -1
};

int ibm8514ValidTokens[] =
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
  -1
};

#include "xf86ExtInit.h"

