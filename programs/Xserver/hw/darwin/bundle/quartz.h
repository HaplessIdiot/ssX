/*
  quartz.h

  Quartz-specific functions and definitions
*/
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartz.h,v 1.4 2001/07/15 01:57:35 torrey Exp $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "X11/Xproto.h"
#include "screenint.h"
#include "darwin.h"
#include "quartzShared.h"
#include "quartzPasteboard.h"

int QuartzProcessArgument(int argc, char *argv[], int i);
void QuartzInitOutput(void);
Bool QuartzAddScreen(int index, ScreenPtr pScreen);
Bool QuartzSetupScreen(int index, ScreenPtr pScreen);
void QuartzGiveUp(void);
void QuartzHide(void);
void QuartzShow(int x, int y);

#endif
