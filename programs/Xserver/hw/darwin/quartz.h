/*
  quartz.h

  Quartz-specific functions and definitions
*/

#ifndef _QUARTZ_H
#define _QUARTZ_H

#include "X11/Xproto.h"
#include "screenint.h"

// NX_APPDEFINED event subtypes for special commands
enum {
  kXServerClearModifiers,
  kXServerShow,
  kXServerHide,
  kXServerQuit
};

Bool QuartzAddScreen(ScreenPtr screen);
void QuartzStoreColors(ColormapPtr pmap, int numEntries, xColorItem *pdefs);
Bool QuartzInitCursor(ScreenPtr pScreen);
void QuartzOsVendorInit(void);
void QuartzGiveUp(void);
void QuartzHide(void);
void QuartzShow(void);

#endif
