/*
 * quartzCursor.h
 *
 * External interface for Quartz hardware cursor
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartzCursor.h,v 1.1 2001/04/11 08:34:18 torrey Exp $ */

#ifndef QUARTZCURSOR_H
#define QUARTZCURSOR_H

#include "screenint.h"

Bool QuartzInitCursor(ScreenPtr pScreen);
void QuartzSuspendXCursor(ScreenPtr pScreen);
void QuartzResumeXCursor(ScreenPtr pScreen, int x, int y);

#endif
