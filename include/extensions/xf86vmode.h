/* $XConsortium: xf86vmode.h /main/2 1995/09/01 10:41:23 kaleb $ */
/* $XFree86: xc/include/extensions/xf86vmode.h,v 3.9 1995/12/02 05:03:03 dawes Exp $ */
/*

Copyright (c) 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES 
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from the Kaleb S. KEITHLEY

*/
/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _XF86VIDMODE_H_
#define _XF86VIDMODE_H_

#include <X11/Xfuncproto.h>

#define X_VGAHelpQueryVersion		0
#define X_VGAHelpGetModeLine		1
#define X_VGAHelpModModeLine		2
#define X_VGAHelpSwitchMode		3
#define X_VGAHelpGetMonitor		4
#define X_XF86VidModeLockModeSwitch	5
#define X_XF86VidModeGetSaver		6
#define X_XF86VidModeSetSaver		7
#define X_XF86VidModeGetServerName	8

#ifdef XF86VIDMODE_EVENTS
#define XF86VidModeNotify		0
#define XF86VidModeNumberEvents		(XF86VidModeNotify + 1)

#define XF86VidModeNotifyMask		0x00000001

#define XF86VidModeNonEvent		0
#define XF86VidModeModeChange		1
#else
#define XF86VidModeNumberEvents		0
#endif

#define XF86VidModeBadClock		0
#define XF86VidModeBadHTimings		1
#define XF86VidModeBadVTimings		2
#define XF86VidModeModeUnsuitable	3
#define XF86VidModeNumberErrors		(XF86VidModeModeUnsuitable + 1)

#ifndef _XF86VIDMODE_SERVER_

typedef struct {
    unsigned short	hdisplay;
    unsigned short	hsyncstart;
    unsigned short	hsyncend;
    unsigned short	htotal;
    unsigned short	vdisplay;
    unsigned short	vsyncstart;
    unsigned short	vsyncend;
    unsigned short	vtotal;
    unsigned int	flags;
    int			privsize;
    INT32		*private;
} XF86VidModeModeLine;

typedef struct {
    float		hi;
    float		lo;
} XF86VidModeSyncRange;

typedef struct {
    char*			vendor;
    char*			model;
    float			bandwidth;
    unsigned char		nhsync;
    XF86VidModeSyncRange*	hsync;
    unsigned char		nvsync;
    XF86VidModeSyncRange*	vsync;
} XF86VidModeMonitor;
    
typedef struct {
    int type;			/* of event */
    unsigned long serial;	/* # of last request processed by server */
    Bool send_event;		/* true if this came from a SendEvent req */
    Display *display;		/* Display the event was read from */
    Window root;		/* root window of event screen */
    int state;			/* What happened */
    int kind;			/* What happened */
    Bool forced;		/* extents of new region */
    Time time;			/* event timestamp */
} XF86VidModeNotifyEvent;

#define XF86VidModeSelectNextMode(disp, scr) \
	XF86VidModeSwitchMode(disp, scr, 1)
#define XF86VidModeSelectPrevMode(disp, scr) \
	XF86VidModeSwitchMode(disp, scr, -1)

_XFUNCPROTOBEGIN

Bool XF86VidModeQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
#endif
);

Bool XF86VidModeQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);

Status XF86VidModeGetModeLine(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    int*			/* dotclock */,
    XF86VidModeModeLine*	/* modeline */
#endif
);

Status XF86VidModeModModeLine(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    XF86VidModeModeLine*	/* modeline */
#endif
);

Status XF86VidModeSwitchMode(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    int			/* zoom */
#endif
);

Status XF86VidModeLockModeSwitch(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    int			/* lock */
#endif
);

Status XF86VidModeGetMonitor(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    XF86VidModeMonitor*	/* monitor */
#endif
);

Status XF86VidModeGetSaver(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    int*			/* suspendtime */,
    int*			/* offtime */
#endif
);

Status XF86VidModeSetSaver(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    int				/* suspendtime */,
    int				/* offtime */
#endif
);

_XFUNCPROTOEND

#endif

#endif
