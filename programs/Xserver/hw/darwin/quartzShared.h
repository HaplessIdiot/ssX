/*
 * Shared definitions between the Darwin X Server
 * and the Cocoa front end. 
 */

#ifndef _QUARTZSHARED_H
#define _QUARTZSHARED_H

extern int                  gDarwinEventWriteFD;

// NX_APPDEFINED event subtypes for special commands to the X server
// update modifiers: update all modifier keys
// show: vt switch to X server; recapture screen and restore X drawing
// hide: vt switch away from X server; release screen and clip X drawing
// quit: kill the X server and release the display
enum {
  kXDarwinUpdateModifiers,
  kXDarwinShow,
  kXDarwinHide,
  kXDarwinQuit
};

#endif	/* _QUARTZSHARED_H */

