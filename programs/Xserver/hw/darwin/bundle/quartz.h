/*
 * quartz.h
 *
 * External interface of the Quartz modes seen by the generic, mode
 * independent parts of the Darwin X server.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartz.h,v 1.6 2001/09/23 04:04:49 torrey Exp $ */

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "screenint.h"
#include "quartzPasteboard.h"

int QuartzProcessArgument(int argc, char *argv[], int i);
void QuartzInitOutput(void);
Bool QuartzAddScreen(int index, ScreenPtr pScreen);
Bool QuartzSetupScreen(int index, ScreenPtr pScreen);
void QuartzGiveUp(void);
void QuartzHide(void);
void QuartzShow(int x, int y);

#endif
