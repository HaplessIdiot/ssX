/* $XFree86: xc/lib/Xxf86rush/XF86Rush.c,v 1.1 1999/09/04 09:14:09 dawes Exp $ */
/*

Copyright (c) 1998 Daryll Strauss

*/

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"
#include "xf86rushstr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xf86rush_info_data;
static XExtensionInfo *xf86rush_info = &_xf86rush_info_data;
static char *xf86rush_extension_name = XF86RUSHNAME;

#define XF86RushCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xf86rush_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xf86rush_extension_hooks = {
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

static XEXT_GENERATE_FIND_DISPLAY (find_display, xf86rush_info, 
				   xf86rush_extension_name, 
				   &xf86rush_extension_hooks, 
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xf86rush_info)


/*****************************************************************************
 *                                                                           *
 *		    public XFree86-DGA Extension routines                *
 *                                                                           *
 *****************************************************************************/

Bool XF86RushQueryExtension (Display *dpy, int *event_basep, int *error_basep)
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

Bool XF86RushQueryVersion(Display *dpy, int *majorVersion, int *minorVersion)
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86RushQueryVersionReply rep;
    xXF86RushQueryVersionReq *req;

    XF86RushCheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86RushQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->rushReqType = X_XF86RushQueryVersion;
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

Bool XF86RushLockPixmap(Display *dpy, int screen, Pixmap pixmap, void **addr)
{
  XExtDisplayInfo *info = find_display (dpy);
  xXF86RushLockPixmapReply rep;
  xXF86RushLockPixmapReq *req;

  XF86RushCheckExtension (dpy, info, False);
  LockDisplay(dpy);
  GetReq(XF86RushLockPixmap, req);
  req->reqType = info->codes->major_opcode;
  req->rushReqType = X_XF86RushLockPixmap;
  req->screen=screen;
  req->pixmap=pixmap;
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  *addr=(void*)rep.addr;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}

Bool XF86RushUnlockPixmap(Display *dpy, int screen, Pixmap pixmap)
{
  XExtDisplayInfo *info = find_display(dpy);
  xXF86RushUnlockPixmapReq *req;

  XF86RushCheckExtension (dpy, info, False);
  LockDisplay(dpy);
  GetReq(XF86RushUnlockPixmap, req);
  req->reqType = info->codes->major_opcode;
  req->rushReqType = X_XF86RushUnlockPixmap;
  req->screen=screen;
  req->pixmap=pixmap;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}

Bool XF86RushUnlockAllPixmaps(Display *dpy)
{
  XExtDisplayInfo *info = find_display(dpy);
  xXF86RushUnlockAllPixmapsReq *req;

  XF86RushCheckExtension (dpy, info, False);
  LockDisplay(dpy);
  GetReq(XF86RushUnlockAllPixmaps, req);
  req->reqType = info->codes->major_opcode;
  req->rushReqType = X_XF86RushUnlockAllPixmaps;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}

Bool XF86RushSetCopyMode(Display *dpy, int screen, int mode)
{
  XExtDisplayInfo *info = find_display(dpy);
  xXF86RushSetCopyModeReq *req;

  XF86RushCheckExtension (dpy, info, False);
  LockDisplay(dpy);
  GetReq(XF86RushSetCopyMode, req);
  req->reqType = info->codes->major_opcode;
  req->rushReqType = X_XF86RushSetCopyMode;
  req->screen = screen;
  req->CopyMode = mode;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}

Bool XF86RushSetPixelStride(Display *dpy, int screen, int stride)
{
  XExtDisplayInfo *info = find_display(dpy);
  xXF86RushSetPixelStrideReq *req;

  XF86RushCheckExtension (dpy, info, False);
  LockDisplay(dpy);
  GetReq(XF86RushSetPixelStride, req);
  req->reqType = info->codes->major_opcode;
  req->rushReqType = X_XF86RushSetPixelStride;
  req->screen = screen;
  req->PixelStride = stride;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}
