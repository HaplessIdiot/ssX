/* $XConsortium: XF86_Hga.c,v 1.1 94/03/28 21:21:40 dpw Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec hga2InfoRec;

#define SCREEN0 &hga2InfoRec

ScrnInfoPtr xf86Screens[] = 
{
  SCREEN0,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  HGA2,
  -1
};

int hga2ValidTokens[] =
{
  STATICGRAY,
  SCREENNO,
  DISPLAYSIZE,
  -1
};

/* Dummy function for PEX in LinkKit and mono server */

PexExtensionInit() {}
