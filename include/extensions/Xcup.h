/*
Copyright © 1986-1997   The Open Group    All Rights Reserved

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the Software), to use the Software 
without restriction, including, without limitation, the rights to copy, modify, merge, 
publish, distribute and sublicense the Software, to make, have made, license and 
distribute derivative works thereof, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and the following permission notice shall be included in all 
copies of the Software:

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
INFRINGEMENT.  IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER SIABILITIY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN 
CONNNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN 
THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be used in 
advertising or otherwise to promote the use or other dealings in this Software without 
prior written authorization from The Open Group.

X Window System is a trademark of The Open Group.
*/
/* $TOG: Xcup.h /main/2 1997/10/21 15:48:41 kaleb $ */

#ifndef _XCUP_H_
#define _XCUP_H_

#include <X11/Xfuncproto.h>

#define X_XcupQueryVersion			0
#define X_XcupGetReservedColormapEntries	1
#define X_XcupStoreColors			2

#define XcupNumberErrors			0

#ifndef _XCUP_SERVER_

_XFUNCPROTOBEGIN

Bool XcupQueryVersion(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int*			/* major_version */,
    int*			/* minor_version */
#endif
);

Status XcupGetReservedColormapEntries(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* screen */,
    XColor**			/* colors_out */,
    int*			/* ncolors */
#endif
);

Status XcupStoreColors(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    Colormap			/* colormap */,
    XColor*			/* colors */,
    int				/* ncolors */
#endif
);

_XFUNCPROTOEND

#endif /* _XCUP_SERVER_ */

#endif /* _XCUP_H_ */

