/*
  quartz.h

  Quartz-specific functions and definitions
*/
/* $XFree86: $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "X11/Xproto.h"
#include "screenint.h"
#include "quartzShared.h"
#include "bundle/quartzPasteboard.h"

void QuartzOsVendorInit(void);
Bool QuartzAddScreen(ScreenPtr screen);
void QuartzGiveUp(void);
void QuartzHide(void);
void QuartzShow(void);

#endif
