/*
 * quartz.h
 *
 * External interface of the Quartz modes seen by the generic, mode
 * independent parts of the Darwin X server.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartz.h,v 1.5 2001/08/01 05:34:06 torrey Exp $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "screenint.h"
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
