/* $XConsortium: XF86VMode.c /main/2 1995/11/14 18:17:58 kaleb $ */
/* $XFree86: xc/lib/Xxf86vm/XF86VMode.c,v 3.13 1996/01/16 15:01:53 dawes Exp $ */
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

Except as contained in this notice, the name of Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from Kaleb S. KEITHLEY.

*/
/* $XConsortium: XF86VMode.c /main/4 1996/01/16 07:52:25 kaleb CHECKEDOUT $ */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_EVENTS
#define NEED_REPLIES
#ifndef XBUILD_IN_CLIENT
#include "Xlibint.h"
#include "xf86vmstr.h"
#include "Xext.h"
#include "extutil.h"
#else
#include "lib/X11/Xlibint.h"
#include "include/extensions/xf86vmstr.h"
#include "include/extensions/Xext.h"
#include "include/extensions/extutil.h"
#endif

static XExtensionInfo _xf86vidmode_info_data;
static XExtensionInfo *xf86vidmode_info = &_xf86vidmode_info_data;
static char *xf86vidmode_extension_name = XF86VIDMODENAME;

#define XF86VidModeCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xf86vidmode_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xf86vidmode_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, xf86vidmode_info, 
				   xf86vidmode_extension_name, 
				   &xf86vidmode_extension_hooks, 
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xf86vidmode_info)


/*****************************************************************************
 *                                                                           *
 *		    public XFree86-VidMode Extension routines                *
 *                                                                           *
 *****************************************************************************/

Bool XF86VidModeQueryExtension (dpy, event_basep, error_basep)
    Display *dpy;
    int *event_basep, *error_basep;
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}

Bool XF86VidModeQueryVersion(dpy, majorVersion, minorVersion)
    Display* dpy;
    int* majorVersion; 
    int* minorVersion;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86VidModeQueryVersionReply rep;
    xXF86VidModeQueryVersionReq *req;

    XF86VidModeCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86VidModeQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->xf86vidmodeReqType = X_XF86VidModeQueryVersion;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *majorVersion = rep.majorVersion;
    *minorVersion = rep.minorVersion;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86VidModeGetModeLine(dpy, screen, dotclock, modeline)
    Display* dpy;
    int screen;
    int* dotclock; 
    XF86VidModeModeLine* modeline;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86VidModeGetModeLineReply rep;
    xXF86VidModeGetModeLineReq *req;

    XF86VidModeCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86VidModeGetModeLine, req);
    req->reqType = info->codes->major_opcode;
    req->xf86vidmodeReqType = X_XF86VidModeGetModeLine;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 
        (SIZEOF(xXF86VidModeGetModeLineReply) - SIZEOF(xReply)) >> 2, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *dotclock = rep.dotclock;
    modeline->hdisplay   = rep.hdisplay;
    modeline->hsyncstart = rep.hsyncstart;
    modeline->hsyncend   = rep.hsyncend;
    modeline->htotal     = rep.htotal;
    modeline->vdisplay   = rep.vdisplay;
    modeline->vsyncstart = rep.vsyncstart;
    modeline->vsyncend   = rep.vsyncend;
    modeline->vtotal     = rep.vtotal;
    modeline->flags      = rep.flags;
    modeline->privsize   = rep.privsize;
    if (rep.privsize > 0) {
	if (!(modeline->private = Xcalloc(rep.privsize, sizeof(INT32)))) {
	    _XEatData(dpy, (rep.privsize) * sizeof(INT32));
	    Xfree(modeline->private);
	    return False;
	}
	_XRead32(dpy, modeline->private, rep.privsize * sizeof(INT32));
    } else {
	modeline->private = NULL;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86VidModeModModeLine (dpy, screen, modeline)
    Display *dpy;
    int screen;
    XF86VidModeModeLine* modeline;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86VidModeModModeLineReq *req;

    XF86VidModeCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(XF86VidModeModModeLine, req);
    req->reqType = info->codes->major_opcode;
    req->xf86vidmodeReqType = X_XF86VidModeModModeLine;
    req->screen = screen;
    req->hdisplay = modeline->hdisplay;
    req->hsyncstart = modeline->hsyncstart;
    req->hsyncend = modeline->hsyncend;
    req->htotal = modeline->htotal;
    req->vdisplay = modeline->vdisplay;
    req->vsyncstart = modeline->vsyncstart;
    req->vsyncend = modeline->vsyncend;
    req->vtotal = modeline->vtotal;
    req->flags = modeline->flags;
    req->privsize = modeline->privsize;
    if (modeline->privsize) {
	req->length += modeline->privsize;
	Data32(dpy, (long *) modeline->private,
	       modeline->privsize * sizeof(INT32));
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XF86VidModeSwitchMode(dpy, screen, zoom)
    Display* dpy;
    int screen;
    int zoom;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86VidModeSwitchModeReq *req;

    XF86VidModeCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86VidModeSwitchMode, req);
    req->reqType = info->codes->major_opcode;
    req->xf86vidmodeReqType = X_XF86VidModeSwitchMode;
    req->screen = screen;
    req->zoom = zoom;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
    
Bool XF86VidModeLockModeSwitch(dpy, screen, lock)
    Display* dpy;
    int screen;
    int lock;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86VidModeLockModeSwitchReq *req;

    XF86VidModeCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86VidModeLockModeSwitch, req);
    req->reqType = info->codes->major_opcode;
    req->xf86vidmodeReqType = X_XF86VidModeLockModeSwitch;
    req->screen = screen;
    req->lock = lock;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
    
Bool XF86VidModeGetMonitor(dpy, screen, monitor)
    Display* dpy;
    int screen;
    XF86VidModeMonitor* monitor;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86VidModeGetMonitorReply rep;
    xXF86VidModeGetMonitorReq *req;
    CARD32 syncrange;
    int i;

    XF86VidModeCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86VidModeGetMonitor, req);
    req->reqType = info->codes->major_opcode;
    req->xf86vidmodeReqType = X_XF86VidModeGetMonitor;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    monitor->nhsync = rep.nhsync;
    monitor->nvsync = rep.nvsync;
    monitor->bandwidth = (float)rep.bandwidth / 1e6;
    if (rep.vendorLength) {
	if (!(monitor->vendor = (char *)Xcalloc(rep.vendorLength + 1, 1))) {
	    _XEatData(dpy, (rep.nhsync + rep.nvsync) * 4 +
		      (rep.vendorLength + 3 & 3) + (rep.modelLength + 3 & 3));
	    return False;
	}
    } else {
	monitor->vendor = NULL;
    }
    if (rep.modelLength) {
	if (!(monitor->model = Xcalloc(rep.modelLength + 1, 1))) {
	    _XEatData(dpy, (rep.nhsync + rep.nvsync) * 4 +
		      (rep.vendorLength + 3 & 3) + (rep.modelLength + 3 & 3));
	    if (monitor->vendor)
		Xfree(monitor->vendor);
	    return False;
	}
    } else {
	monitor->model = NULL;
    }
    if (!(monitor->hsync = Xcalloc(rep.nhsync, sizeof(XF86VidModeSyncRange)))) {
	_XEatData(dpy, (rep.nhsync + rep.nvsync) * 4 +
		  (rep.vendorLength + 3 & 3) + (rep.modelLength + 3 & 3));
	
	if (monitor->vendor)
	    Xfree(monitor->vendor);
	if (monitor->model)
	    Xfree(monitor->model);
	return False;
    }
    if (!(monitor->vsync = Xcalloc(rep.nvsync, sizeof(XF86VidModeSyncRange)))) {
	_XEatData(dpy, (rep.nhsync + rep.nvsync) * 4 +
		  (rep.vendorLength + 3 & 3) + (rep.modelLength + 3 & 3));
	if (monitor->vendor)
	    Xfree(monitor->vendor);
	if (monitor->model)
	    Xfree(monitor->model);
	Xfree(monitor->hsync);
	return False;
    }
    for (i = 0; i < rep.nhsync; i++) {
	_XRead32(dpy, (long *)&syncrange, 4);
	monitor->hsync[i].lo = (float)(syncrange & 0xFFFF) / 100.0;
	monitor->hsync[i].hi = (float)(syncrange >> 16) / 100.0;
    }
    for (i = 0; i < rep.nvsync; i++) {
	_XRead32(dpy, (long *)&syncrange, 4);
	monitor->vsync[i].lo = (float)(syncrange & 0xFFFF) / 100.0;
	monitor->vsync[i].hi = (float)(syncrange >> 16) / 100.0;
    }
    if (rep.vendorLength)
	_XReadPad(dpy, monitor->vendor, rep.vendorLength);
    else
	monitor->vendor = "";
    if (rep.modelLength)
	_XReadPad(dpy, monitor->model, rep.modelLength);
    else
	monitor->model = "";
	
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
