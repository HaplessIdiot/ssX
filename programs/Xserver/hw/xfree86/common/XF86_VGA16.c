/* $XConsortium: XF86_VGA16.c,v 1.1 94/03/28 21:22:17 dpw Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec vga16InfoRec;

#define SCREEN0 &vga16InfoRec

ScrnInfoPtr xf86Screens[] = 
{
  SCREEN0,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  VGA16,
  -1
};

int vga16ValidTokens[] =
{
  PSEUDOCOLOR,
  STATICGRAY,
  GRAYSCALE,
  CHIPSET,
  CLOCKS,
  DISPLAYSIZE,
  MODES,
  SCREENNO,
  OPTION,
  VIDEORAM,
  VIEWPORT,
  VIRTUAL,
  CLOCKPROG,
  BIOSBASE,
  -1
};

/* Dummy function for PEX in LinkKit and non-8-bit server */

PexExtensionInit() {}
