/* $XFree86: xc/include/extensions/xf86misc.h,v 3.0 1996/01/16 15:00:32 dawes Exp $ */

/*
 * Copyright (c) 1995, 1996  The XFree86 Project, Inc
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _XF86MISC_H_
#define _XF86MISC_H_

#include <X11/Xfuncproto.h>

#define X_XF86MiscQueryVersion		0
#define X_XF86MiscGetSaver		1
#define X_XF86MiscSetSaver		2

#define XF86MiscNumberEvents		0

#define XF86MiscNumberErrors		0

#ifndef _XF86MISC_SERVER_

_XFUNCPROTOBEGIN

Bool XF86MiscQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
#endif
);

Bool XF86MiscQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);

Status XF86MiscGetSaver(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    int*			/* suspendtime */,
    int*			/* offtime */
#endif
);

Status XF86MiscSetSaver(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    int				/* suspendtime */,
    int				/* offtime */
#endif
);

_XFUNCPROTOEND

#endif

#endif
