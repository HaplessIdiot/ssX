/*
 * quartzCursor.h
 *
 *  Quartz hardware cursor
 */
/* $XFree86: $ */

#ifndef QUARTZCURSOR_H
#define QUARTZCURSOR_H

Bool QuartzInitCursor(ScreenPtr pScreen);
void QuartzSuspendXCursor(ScreenPtr pScreen);
void QuartzResumeXCursor(ScreenPtr pScreen, int x, int y);

#endif
