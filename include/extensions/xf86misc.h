/* $XFree86: xc/include/extensions/xf86misc.h,v 3.1 1996/01/17 12:44:38 dawes Exp $ */

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
#define X_XF86MiscGetMouseSettings	3
#define X_XF86MiscGetKbdSettings	4
#define X_XF86MiscSetMouseSettings	5
#define X_XF86MiscSetKbdSettings	6

#define XF86MiscNumberEvents		0

#define XF86MiscNumberErrors		0

#ifndef _XF86MISC_SERVER_

_XFUNCPROTOBEGIN

typedef struct {
    char*	device;
    int		type;
    int		baudrate;
    int		samplerate;
    Bool	emulate3buttons;
    int		emulate3timeout;
    Bool	chordmiddle;
    int		flags;
} XF86MiscMouseSettings;

typedef struct {
    int		type;
    int		rate;
    int		delay;
    Bool	servnumlock;
} XF86MiscKbdSettings;

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

Status XF86MiscGetMouseSettings(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XF86MiscMouseSettings*	/* mouse info */
#endif
);

Status XF86MiscGetKbdSettings(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XF86MiscKbdSettings*	/* keyboard info */
#endif
);

Status XF86MiscSetMouseSettings(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XF86MiscMouseSettings*	/* mouse info */
#endif
);

Status XF86MiscSetKbdSettings(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XF86MiscKbdSettings*	/* keyboard info */
#endif
);

_XFUNCPROTOEND

#endif

#endif
