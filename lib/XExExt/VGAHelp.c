
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
from the Kaleb S. KEITHLEY.

*/
/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"
#include "VGAHelpstr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _vgahelp_info_data;
static XExtensionInfo *vgahelp_info = &_vgahelp_info_data;
static char *vgahelp_extension_name = VGAHELPNAME;

#define VGAHelpCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, vgahelp_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks vgahelp_extension_hooks = {
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

static XEXT_GENERATE_FIND_DISPLAY (find_display, vgahelp_info, 
				   vgahelp_extension_name, 
				   &vgahelp_extension_hooks, 
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, vgahelp_info)


/*****************************************************************************
 *                                                                           *
 *		    public Shared Memory Extension routines                  *
 *                                                                           *
 *****************************************************************************/

Bool XVGAHelpQueryExtension (dpy /* event_basep, error_basep */)
    Display *dpy;
/*  int *event_basep, *error_basep; */
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
/*	*event_basep = info->codes->first_event;
	*error_basep = info->codes->error_event; */
	return True;
    } else {
	return False;
    }
}

Bool XVGAHelpQueryVersion(dpy, majorVersion, minorVersion)
    Display* dpy;
    int* majorVersion; 
    int* minorVersion;
{
    XExtDisplayInfo *info = find_display (dpy);
    xVGAHelpQueryVersionReply rep;
    xVGAHelpQueryVersionReq *req;

    VGAHelpCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(VGAHelpQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->vgahelpReqType = X_VGAHelpQueryVersion;
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

Bool XVGAHelpGetModeLine(dpy, screen, dotclock, modeline)
    Display* dpy;
    int screen;
    int* dotclock; 
    XVGAHelpModeLine* modeline;
{
    XExtDisplayInfo *info = find_display (dpy);
    xVGAHelpGetModeLineReply rep;
    xVGAHelpGetModeLineReq *req;

    VGAHelpCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(VGAHelpGetModeLine, req);
    req->reqType = info->codes->major_opcode;
    req->vgahelpReqType = X_VGAHelpGetModeLine;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
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
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool XVGAHelpModModeLine (dpy, screen, modeline)
    Display *dpy;
    int screen;
    XVGAHelpModeLine* modeline;
{
    XExtDisplayInfo *info = find_display (dpy);
    xVGAHelpModModeLineReq *req;

    VGAHelpCheckExtension (dpy, info, 0);

    LockDisplay(dpy);
    GetReq(VGAHelpModModeLine, req);
    req->reqType = info->codes->major_opcode;
    req->vgahelpReqType = X_VGAHelpModModeLine;
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
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
