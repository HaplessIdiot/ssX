/*
 * $XFree86$
 *
 * Copyright © 2000 Compaq Computer Corporation, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Compaq makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * COMPAQ DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL COMPAQ
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Compaq Computer Corporation, Inc.
 */
#include <stdio.h>
#include "Xrandrint.h"

XExtensionInfo XRRExtensionInfo;
char XRRExtensionName[] = RANDR_NAME;

static int XRRCloseDisplay();

static /* const */ XExtensionHooks rr_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    XRRCloseDisplay,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

XExtDisplayInfo *
XRRFindDisplay (Display *dpy)
{
    XExtDisplayInfo *dpyinfo;

    dpyinfo = XextFindDisplay (&XRRExtensionInfo, dpy);
    if (!dpyinfo)
	dpyinfo = XextAddDisplay (&XRRExtensionInfo, dpy, 
				  XRRExtensionName,
				  &rr_extension_hooks,
				  0, 0);
    return dpyinfo;
}

static int
XRRCloseDisplay (Display *dpy, XExtCodes *codes)
{
    XExtDisplayInfo *info = XRRFindDisplay (dpy);
    if (info->data) XFree (info->data);
    
    return XextRemoveDisplay (&XRRExtensionInfo, dpy);
}
    
/****************************************************************************
 *                                                                          *
 *			    RandR public interfaces                         *
 *                                                                          *
 ****************************************************************************/

Bool XRRQueryExtension (Display *dpy, int *event_basep, int *error_basep)
{
    XExtDisplayInfo *info = XRRFindDisplay (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}


Status XRRQueryVersion (Display *dpy,
			    int	    *major_versionp,
			    int	    *minor_versionp)
{
    XExtDisplayInfo *info = XRRFindDisplay (dpy);
    xRRQueryVersionReply rep;
    xRRQueryVersionReq  *req;

    RRCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (RRQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->randrReqType = X_RRQueryVersion;
    req->majorVersion = RANDR_MAJOR;
    req->minorVersion = RANDR_MINOR;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    *major_versionp = rep.majorVersion;
    *minor_versionp = rep.minorVersion;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}

Time XRRSetScreenConfig (Display *dpy,
			 Drawable draw,
			 int size_set_index,
			 int visual_set_index,
			 int rotation,
			 Time timestamp) /* returns new timestamp */
{
    XExtDisplayInfo *info = XRRFindDisplay (dpy);
    xRRSetScreenConfigReply rep;
    xRRSetScreenConfigReq  *req;

    RRCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (RRSetScreenConfig, req);
    req->reqType = info->codes->major_opcode;
    req->randrReqType = X_RRSetScreenConfig;
    req->drawable = draw;
    req->sizeSetIndex = size_set_index;
    req->visualSetIndex = visual_set_index;
    req->rotation = rotation;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue) == 0) 
      rep.newtimestamp = CurrentTime;
    UnlockDisplay (dpy);
    SyncHandle ();
    return(rep.newtimestamp);
}
