/* $XFree86: xc/include/extensions/VGAHelp.h,v 3.2 1995/03/19 12:16:22 dawes Exp $ */
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

#ifndef _XVGAHELP_H_
#define _XVGAHELP_H_

#include <X11/Xfuncproto.h>

#define X_VGAHelpQueryVersion		0
#define X_VGAHelpGetModeLine		1
#define X_VGAHelpModModeLine		2
#define X_VGAHelpSwitchMode		3
#define X_VGAHelpGetMonitor		4

#define VGAHelpNumberEvents		0

#define VGAHelpBadClock			0
#define VGAHelpBadHTimings		1
#define VGAHelpBadVTimings		2
#define VGAHelpModeUnsuitable		3
#define VGAHelpNumberErrors		(VGAHelpModeUnsuitable + 1)

#ifndef _XVGAHELP_SERVER_

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
} XVGAHelpModeLine;

typedef struct {
    float		hi;
    float		lo;
} XVGAHelpSyncRange;

typedef struct {
    char*		vendor;
    char*		model;
    float		bandwidth;
    unsigned char	nhsync;
    XVGAHelpSyncRange*	hsync;
    unsigned char	nvsync;
    XVGAHelpSyncRange*	vsync;
} XVGAHelpMonitor;
    
#define XVGAHelpSelectNextMode(disp, scr) XVGAHelpSwitchMode(disp, scr, 1)
#define XVGAHelpSelectPrevMode(disp, scr) XVGAHelpSwitchMode(disp, scr, -1)

_XFUNCPROTOBEGIN

Bool XVGAHelpQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
#endif
);

Bool XVGAHelpQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);

Status XVGAHelpGetModeLine(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    int*		/* dotclock */,
    XVGAHelpModeLine*	/* modeline */
#endif
);

Status XVGAHelpModModeLine(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    XVGAHelpModeLine*	/* modeline */
#endif
);

Status XVGAHelpSwitchMode(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    int			/* zoom */
#endif
);

Status XVGAHelpGetMonitor(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    XVGAHelpMonitor*	/* monitor */
#endif
);

_XFUNCPROTOEND

#endif

#endif
