/* $XConsortium: XF86_Mono.c,v 1.1 94/03/28 21:21:58 dpw Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec vga2InfoRec;

#define SCREEN0 &vga2InfoRec

ScrnInfoPtr xf86Screens[] = 
{
  SCREEN0,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  VGA2,
  -1
};

int vga2ValidTokens[] =
{
  STATICGRAY,
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
  BLACK,
  WHITE,
  -1
};

/* Dummy function for PEX in LinkKit and mono server */

PexExtensionInit() {}
