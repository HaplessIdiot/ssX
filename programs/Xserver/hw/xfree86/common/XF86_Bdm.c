/* $XConsortium: XF86_Bdm.c,v 1.1 94/03/28 21:21:34 dpw Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec bdm2InfoRec;

#define SCREEN0 &bdm2InfoRec

ScrnInfoPtr xf86Screens[] = 
{
  SCREEN0,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  BDM2,
  -1
};

int bdm2ValidTokens[] =
{
  STATICGRAY,
  CHIPSET,
  OPTION,
  MEMBASE,
  SCREENNO,
  DISPLAYSIZE,
  VIRTUAL,
  VIEWPORT,
  -1
}; 

/* Dummy function for PEX in LinkKit and mono server */

PexExtensionInit() {}
