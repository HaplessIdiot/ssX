/* $XFree86$ */
/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995  The XFree86 Project, Inc

*/

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"
#include "xf86dgastr.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xf86dga_info_data;
static XExtensionInfo *xf86dga_info = &_xf86dga_info_data;
static char *xf86dga_extension_name = XF86DGANAME;

#define XF86DGACheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xf86dga_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xf86dga_extension_hooks = {
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

static XEXT_GENERATE_FIND_DISPLAY (find_display, xf86dga_info, 
				   xf86dga_extension_name, 
				   &xf86dga_extension_hooks, 
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xf86dga_info)


/*****************************************************************************
 *                                                                           *
 *		    public XFree86-DGA Extension routines                *
 *                                                                           *
 *****************************************************************************/

Bool XF86DGAQueryExtension (dpy, event_basep, error_basep)
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

Bool XF86DGAQueryVersion(dpy, majorVersion, minorVersion)
    Display* dpy;
    int* majorVersion; 
    int* minorVersion;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGAQueryVersionReply rep;
    xXF86DGAQueryVersionReq *req;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGAQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGAQueryVersion;
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

Bool XF86DGAGetVideoLL(dpy, screen, offset, width, bank_size, ram_size)
    Display* dpy;
    int screen;
    int *offset;
    int *width, *bank_size, *ram_size;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGAGetVideoLLReply rep;
    xXF86DGAGetVideoLLReq *req;
    int i;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGAGetVideoLL, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGAGetVideoLL;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *offset = /*(char *)*/rep.offset;
    *width = rep.width;
    *bank_size = rep.bank_size;
    *ram_size = rep.ram_size;
	
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

    
Bool XF86DGADirectVideoLL(dpy, screen, enable)
    Display* dpy;
    int screen;
    int enable;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGADirectVideoReq *req;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGADirectVideo, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGADirectVideo;
    req->screen = screen;
    req->enable = enable;
    UnlockDisplay(dpy);
    SyncHandle();
    XSync(dpy,False);
    return True;
}

Bool XF86DGAGetViewPort(dpy, screen, x, y)
    Display* dpy;
    int screen;
    int *x, *y;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGAGetViewPortReply rep;
    xXF86DGAGetViewPortReq *req;
    int i;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGAGetViewPort, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGAGetViewPort;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *x = rep.x;
    *y = rep.y;
	
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
    
    
Bool XF86DGASetViewPort(dpy, screen, x, y)
    Display* dpy;
    int screen;
    int x, y;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGASetViewPortReq *req;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGASetViewPort, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGASetViewPort;
    req->screen = screen;
    req->x = x;
    req->y = y;
    UnlockDisplay(dpy);
    SyncHandle();
    XSync(dpy,False);
    return True;
}

    
Bool XF86DGAGetVidPage(dpy, screen, vpage)
    Display* dpy;
    int screen;
    int *vpage;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGAGetVidPageReply rep;
    xXF86DGAGetVidPageReq *req;
    int i;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGAGetVidPage, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGAGetVidPage;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *vpage = rep.vpage;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

    
Bool XF86DGASetVidPage(dpy, screen, vpage)
    Display* dpy;
    int screen;
    int vpage;
{
    XExtDisplayInfo *info = find_display (dpy);
    xXF86DGASetVidPageReq *req;

    XF86DGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XF86DGASetVidPage, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XF86DGASetVidPage;
    req->screen = screen;
    req->vpage = vpage;
    UnlockDisplay(dpy);
    SyncHandle();
    XSync(dpy,False);
    return True;
}
