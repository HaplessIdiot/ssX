/* $XFree86: xc/lib/Xxf86misc/XF86Misc.c,v 3.0 1996/01/16 15:01:47 dawes Exp $ */

/*
 * Copyright (c) 1995, 1996  The XFree86 Project, Inc
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"
#include "xf86mscstr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xf86misc_info_data;
static XExtensionInfo *xf86misc_info = &_xf86misc_info_data;
static char *xf86misc_extension_name = XF86MISCNAME;

#define XF86MiscCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xf86misc_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xf86misc_extension_hooks = {
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

static XEXT_GENERATE_FIND_DISPLAY (find_display, xf86misc_info, 
				   xf86misc_extension_name, 
				   &xf86misc_extension_hooks, 
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xf86misc_info)


/*****************************************************************************
 *                                                                           *
 *		    public XFree86-Misc Extension routines                *
 *                                                                           *
 *****************************************************************************/

Bool XF86MiscQueryExtension (dpy, event_basep, error_basep)
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

Bool XF86MiscQueryVersion(dpy, majorVersion, minorVersion)
    Display* dpy;
    int* majorVersion; 
    int* minorVersion;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86MiscQueryVersionReply rep;
    xXF86MiscQueryVersionReq *req;

    XF86MiscCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86MiscQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->xf86miscReqType = X_XF86MiscQueryVersion;
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

Bool XF86MiscSetSaver(dpy, screen, suspendTime, offTime)
    Display* dpy;
    int screen;
    int suspendTime, offTime;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86MiscSetSaverReq *req;

    XF86MiscCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86MiscSetSaver, req);
    req->reqType = info->codes->major_opcode;
    req->xf86miscReqType = X_XF86MiscSetSaver;
    req->screen = screen;
    req->suspendTime = suspendTime;
    req->offTime = offTime;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
    
Bool XF86MiscGetSaver(dpy, screen, suspendTime, offTime)
    Display* dpy;
    int screen;
    int *suspendTime, *offTime;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86MiscGetSaverReply rep;
    xXF86MiscGetSaverReq *req;
    int i;

    XF86MiscCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86MiscGetSaver, req);
    req->reqType = info->codes->major_opcode;
    req->xf86miscReqType = X_XF86MiscGetSaver;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *suspendTime = rep.suspendTime;
    *offTime = rep.offTime;
	
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

