/*
  xfIOKit.h

  IOKit specific functions and definitions
*/

#ifndef _XFIOKIT_H
#define _XFIOKIT_H

#include "X11/Xproto.h"
#include "screenint.h"

void XFIOKitStoreColors(ColormapPtr pmap, int numEntries, xColorItem *pdefs);
Bool XFIOKitInitCursor(ScreenPtr pScreen);
void XFIOKitOsVendorInit(void);
void XFIOKitGiveUp(void);

#endif