/*
 * $XFree86: $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "Xfixesint.h"

XExtensionInfo XFixesExtensionInfo;
char XFixesExtensionName[] = XFIXES_NAME;

static int
XFixesCloseDisplay (Display *dpy, XExtCodes *codes);
    
static Bool
XFixesWireToEvent(Display *dpy, XEvent *event, xEvent *wire);

static Status
XFixesEventToWire(Display *dpy, XEvent *event, xEvent *wire);

static /* const */ XExtensionHooks xfixes_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    XFixesCloseDisplay,			/* close_display */
    XFixesWireToEvent,			/* wire_to_event */
    XFixesEventToWire,			/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

XExtDisplayInfo *
XFixesFindDisplay (Display *dpy)
{
    XExtDisplayInfo *dpyinfo;
    XFixesInfo	    *xfi;

    dpyinfo = XextFindDisplay (&XFixesExtensionInfo, dpy);
    if (!dpyinfo)
    {
	dpyinfo = XextAddDisplay (&XFixesExtensionInfo, dpy, 
				  XFixesExtensionName,
				  &xfixes_extension_hooks,
				  XFixesNumberEvents, 0);
	xfi = Xmalloc (sizeof (XFixesInfo));
	if (!xfi)
	    return 0;
	xfi->major_version = -1;
	dpyinfo->data = (char *) xfi;
    }
    return dpyinfo;
}
    
static int
XFixesCloseDisplay (Display *dpy, XExtCodes *codes)
{
    return XextRemoveDisplay (&XFixesExtensionInfo, dpy);
}

static Bool
XFixesWireToEvent(Display *dpy, XEvent *event, xEvent *wire)
{
    XExtDisplayInfo *info = XFixesFindDisplay(dpy);

    XFixesCheckExtension(dpy, info, False);

    switch ((wire->u.u.type & 0x7F) - info->codes->first_event)
    {
    case XFixesSelectionNotify: {
	XFixesSelectionNotifyEvent *aevent;
	xXFixesSelectionNotifyEvent *awire;
	awire = (xXFixesSelectionNotifyEvent *) wire;
	aevent = (XFixesSelectionNotifyEvent *) event;
	aevent->type = awire->type & 0x7F;
	aevent->subtype = awire->subtype;
	aevent->serial = _XSetLastRequestRead(dpy,
					      (xGenericReply *) wire);
	aevent->send_event = (awire->type & 0x80) != 0;
	aevent->display = dpy;
	aevent->window = awire->window;
	aevent->owner = awire->owner;
	aevent->selection = awire->selection;
	aevent->timestamp = awire->timestamp;
	aevent->selection_timestamp = awire->selectionTimestamp;
	return True;
    }
    case XFixesCursorNotify: {
	XFixesCursorNotifyEvent *aevent;
	xXFixesCursorNotifyEvent *awire;
	awire = (xXFixesCursorNotifyEvent *) wire;
	aevent = (XFixesCursorNotifyEvent *) event;
	aevent->type = awire->type & 0x7F;
	aevent->subtype = awire->subtype;
	aevent->serial = _XSetLastRequestRead(dpy,
					      (xGenericReply *) wire);
	aevent->send_event = (awire->type & 0x80) != 0;
	aevent->display = dpy;
	aevent->window = awire->window;
	aevent->cursor_serial = awire->cursorSerial;
	aevent->timestamp = awire->timestamp;
	return True;
    }
    }
    return False;
}

static Status
XFixesEventToWire(Display *dpy, XEvent *event, xEvent *wire)
{
    XExtDisplayInfo *info = XFixesFindDisplay(dpy);

    XFixesCheckExtension(dpy, info, False);

    switch ((event->type & 0x7F) - info->codes->first_event)
    {
    case XFixesSelectionNotify: {
	XFixesSelectionNotifyEvent *aevent;
	xXFixesSelectionNotifyEvent *awire;
	awire = (xXFixesSelectionNotifyEvent *) wire;
	aevent = (XFixesSelectionNotifyEvent *) event;
	awire->type = aevent->type | (aevent->send_event ? 0x80 : 0);
	awire->subtype = aevent->subtype;
	awire->window = aevent->window;
	awire->owner = aevent->owner;
	awire->selection = aevent->selection;
	awire->timestamp = aevent->timestamp;
	awire->selectionTimestamp = aevent->selection_timestamp;
	return True;
    }
    case XFixesCursorNotify: {
	XFixesCursorNotifyEvent *aevent;
	xXFixesCursorNotifyEvent *awire;
	awire = (xXFixesCursorNotifyEvent *) wire;
	aevent = (XFixesCursorNotifyEvent *) event;
	awire->type = aevent->type | (aevent->send_event ? 0x80 : 0);
	awire->subtype = aevent->subtype;
	awire->window = aevent->window;
	awire->timestamp = aevent->timestamp;
	awire->cursorSerial = aevent->cursor_serial;
    }
    }
    return False;
}

Bool 
XFixesQueryExtension (Display *dpy, int *event_basep, int *error_basep)
{
    XExtDisplayInfo *info = XFixesFindDisplay (dpy);

    if (XextHasExtension(info)) 
    {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } 
    else
	return False;
}

Status 
XFixesQueryVersion (Display *dpy,
		    int	    *major_versionp,
		    int	    *minor_versionp)
{
    XExtDisplayInfo		*info = XFixesFindDisplay (dpy);
    xXFixesQueryVersionReply	rep;
    xXFixesQueryVersionReq	*req;
    XFixesInfo			*xfi;

    XFixesCheckExtension (dpy, info, 0);

    xfi = (XFixesInfo *) info->data;

    /*
     * only get the version information from the server if we don't have it
     * already
     */
    if (xfi->major_version == -1) 
    {
	LockDisplay (dpy);
	GetReq (XFixesQueryVersion, req);
	req->reqType = info->codes->major_opcode;
	req->xfixesReqType = X_XFixesQueryVersion;
	req->majorVersion = XFIXES_MAJOR;
	req->minorVersion = XFIXES_MINOR;
	if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) 
	{
	    UnlockDisplay (dpy);
	    SyncHandle ();
	    return 0;
	}
	xfi->major_version = rep.majorVersion;
	xfi->minor_version = rep.minorVersion;
    }
    *major_versionp = xfi->major_version;
    *minor_versionp = xfi->minor_version;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}
