
#define NEED_REPLIES

#include <stdio.h>
#include "XvMClibint.h"
#include "Xext.h"
#include "extutil.h"

static XExtensionInfo _xvmc_info_data;
static XExtensionInfo *xvmc_info = &_xvmc_info_data;
static char *xvmc_extension_name = XvMCName;

static char *xvmc_error_list[] =
{
   "BadContext",
   "BadSurface",
   "BadSubpicture"
};

static XEXT_GENERATE_CLOSE_DISPLAY (xvmc_close_display, xvmc_info)


static XEXT_GENERATE_ERROR_STRING (xvmc_error_string, xvmc_extension_name,
                                   XvMCNumErrors, xvmc_error_list)


static XExtensionHooks xvmc_extension_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    xvmc_close_display,                 /* close_display */
    NULL,                               /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    xvmc_error_string                   /* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (xvmc_find_display, xvmc_info,
                                   xvmc_extension_name,
                                   &xvmc_extension_hooks,
                                   XvMCNumEvents, NULL)

Bool XvMCQueryExtension (Display *dpy, int *event_basep, int *error_basep)
{
    XExtDisplayInfo *info = xvmc_find_display(dpy);

    if (XextHasExtension(info)) {
        *event_basep = info->codes->first_event;
        *error_basep = info->codes->first_error;
        return True;
    } else {
        return False;
    }
}

Status XvMCQueryVersion (Display *dpy, int *major, int *minor)
{
    XExtDisplayInfo *info = xvmc_find_display(dpy);
    xvmcQueryVersionReply rep;
    xvmcQueryVersionReq  *req;

    XvMCCheckExtension (dpy, info, BadImplementation);

    LockDisplay (dpy);
    XvMCGetReq (QueryVersion, req);
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
        UnlockDisplay (dpy);
        SyncHandle ();
        return 0;
    }
    *major = rep.major;
    *minor = rep.minor;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}

