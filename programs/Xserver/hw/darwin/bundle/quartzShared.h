/*
 * Shared definitions between the Darwin X Server
 * and the Cocoa front end. 
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartzShared.h,v 1.6 2001/06/26 23:29:12 torrey Exp $ */

#ifndef _QUARTZSHARED_H
#define _QUARTZSHARED_H

// Data stored at startup for Cocoa front end
extern int                  quartzEventWriteFD;
extern int                  quartzStartClients;

// User preferences used by X server
extern int                  quartzRootless;
extern int                  quartzUseSysBeep;
extern int                  quartzMouseAccelChange;
extern int                  darwinFakeButtons;
extern char                 *darwinKeymapFile;

void QuartzCapture(void);
void QuartzReadPreferences(void);
void QuartzMessageMainThread(unsigned msg);

// NX_APPDEFINED event subtypes for special commands to the X server
enum {
  kXDarwinUpdateModifiers,  // update all modifier keys
  kXDarwinShow,             // vt switch to X server;
                            // recapture screen and restore X drawing
  kXDarwinHide,             // vt switch away from X server;
                            // release screen and clip X drawing
  kXDarwinQuit,             // kill the X server and release the display
  kXDarwinReadPasteboard,   // copy Mac OS X pasteboard into X cut buffer
  kXDarwinWritePasteboard   // copy X cut buffer onto Mac OS X pasteboard
};

// Messages that can be sent to the main thread.
enum {
  kQuartzServerHidden,
  kQuartzServerDied
};

#endif	/* _QUARTZSHARED_H */

