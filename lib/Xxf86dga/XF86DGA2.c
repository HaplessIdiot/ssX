/* $XFree86$ */
/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995,1996  The XFree86 Project, Inc

*/

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifdef __EMX__ /* needed here to override certain constants in X headers */
#define INCL_DOS
#define INCL_DOSIOCTL
#include <os2.h>
#endif

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"
#include "xf86dgastr.h"
#include "Xext.h"
#include "extutil.h"

char *xdga_extension_name = XF86DGANAME;

static XExtensionInfo _xdga_info_data;
static XExtensionInfo *xdga_info = &_xdga_info_data;

#define XDGACheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xdga_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/


static XEXT_GENERATE_CLOSE_DISPLAY (xdga_close_display, xdga_info)

static /* const */ XExtensionHooks xdga_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    xdga_close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};



XEXT_GENERATE_FIND_DISPLAY (xdga_find_display, xdga_info, 
				   xdga_extension_name, 
				   &xdga_extension_hooks, 
				   0, NULL)



Bool XDGAQueryExtension (
    Display *dpy,
    int *event_basep,
    int *error_basep
){
    XExtDisplayInfo *info = xdga_find_display (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}


Bool XDGAQueryVersion(
    Display* dpy,
    int* majorVersion, 
    int* minorVersion
){
    XExtDisplayInfo *info = xdga_find_display (dpy);
    xXDGAQueryVersionReply rep;
    xXDGAQueryVersionReq *req;

    XDGACheckExtension (dpy, info, False);

    LockDisplay(dpy);
    GetReq(XDGAQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->dgaReqType = X_XDGAQueryVersion;
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
