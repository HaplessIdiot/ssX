/* $XFree86: xc/include/extensions/xf86rush.h,v 1.1 1999/09/04 09:14:08 dawes Exp $ */
/*

Copyright (c) 1998  Daryll Strauss

*/

#ifndef _XF86RUSH_H_
#define _XF86RUSH_H_

#include <X11/Xfuncproto.h>

#define X_XF86RushQueryVersion		0
#define X_XF86RushLockPixmap		1
#define X_XF86RushUnlockPixmap		2
#define X_XF86RushUnlockAllPixmaps	3
#define X_XF86RushSetCopyMode		4
#define X_XF86RushSetPixelStride	5

#define XF86RushNumberEvents		0

#define XF86RushClientNotLocal		0
#define XF86RushNumberErrors		(XF86RushClientNotLocal + 1)

#ifndef _XF86RUSH_SERVER_

_XFUNCPROTOBEGIN

Bool XF86RushQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
#endif
);

Bool XF86RushQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);

Bool XF86RushLockPixmap(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    Pixmap		/* Pixmap */,
    void **		/* Return address */
#endif
);

Bool XF86RushUnlockPixmap(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    Pixmap		/* Pixmap */
#endif
); 

Bool XF86RushUnlockAllPixmaps(
#if NeedFunctionPrototypes
    Display *		/* dpy */
#endif			    
);

Bool XF86RushSetCopyMode(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    int			/* copy mode */
#endif			    
);

Bool XF86RushSetPixelStride(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    int			/* pixel stride */
#endif			    
);

_XFUNCPROTOEND

#endif /* _XF86RUSH_SERVER_ */

#endif /* _XF86RUSH_H_ */
