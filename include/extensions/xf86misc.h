/* $XFree86$ */
/*

Copyright (c) 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES 
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Kaleb S. KEITHLEY 
shall not be used in advertising or otherwise to promote the sale, use 
or other dealings in this Software without prior written authorization
from Kaleb S. KEITHLEY

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
