/*
  quartz.h

  Quartz-specific functions and definitions
*/
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartz.h,v 1.1 2001/03/24 23:08:53 torrey Exp $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "X11/Xproto.h"
#include "screenint.h"
#include "quartzShared.h"
#include "quartzPasteboard.h"

void QuartzOsVendorInit(void);
Bool QuartzAddScreen(ScreenPtr screen);
Bool QuartzInitCursor(ScreenPtr pScreen);
void QuartzGiveUp(void);
void QuartzHide(void);
void QuartzShow(void);

#endif
