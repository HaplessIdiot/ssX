/* $XConsortium: XF86_SVGA.c,v 1.1 94/03/28 21:22:11 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/XF86_SVGA.c,v 3.2 1994/08/31 04:33:06 dawes Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec vga256InfoRec;

ScrnInfoPtr xf86Screens[] = 
{
  &vga256InfoRec,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  SVGA,
  -1
};

int vga256ValidTokens[] =
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
  SPEEDUP,
  NOSPEEDUP,
  CLOCKPROG,
  BIOSBASE,
  MEMBASE,
  -1
};

#include "xf86ExtInit.h"

