/*
  xfIOKit.h

  IOKit specific functions and definitions
*/

#ifndef _XFIOKIT_H
#define _XFIOKIT_H

#include "X11/Xproto.h"
#include "screenint.h"

Bool XFIOKitAddScreen(ScreenPtr pScreen);
Bool XFIOKitInitCursor(ScreenPtr pScreen);
void XFIOKitOsVendorInit(void);
void XFIOKitGiveUp(void);

#endif
