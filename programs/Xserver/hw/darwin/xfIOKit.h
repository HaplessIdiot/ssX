/*
  xfIOKit.h

  IOKit specific functions and definitions
*/
/* $XFree86: xc/programs/Xserver/hw/darwin/xfIOKit.h,v 1.4 2001/08/01 05:34:05 torrey Exp $ */

#ifndef _XFIOKIT_H
#define _XFIOKIT_H

#include "X11/Xproto.h"
#include "screenint.h"
#include "darwin.h"

Bool XFIOKitAddScreen(int index, ScreenPtr pScreen);
Bool XFIOKitSetupScreen(int index, ScreenPtr pScreen);
Bool XFIOKitInitCursor(ScreenPtr pScreen);
void XFIOKitInitOutput(void);
void XFIOKitGiveUp(void);
void XFIOKitBell(int volume, DeviceIntPtr pDevice, pointer ctrl, int class);

#endif
