/*
  quartz.h

  Quartz-specific functions and definitions
*/
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz.h,v 1.4 2001/03/15 22:24:26 torrey Exp $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "X11/Xproto.h"
#include "screenint.h"
#include "quartzShared.h"
#include "quartzPasteboard.h"

void QuartzOsVendorInit(void);
Bool QuartzAddScreen(ScreenPtr screen);
void QuartzGiveUp(void);
void QuartzHide(void);
void QuartzShow(void);

#endif
