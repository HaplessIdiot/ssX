/*
   Copyright (c) 1999  XFree86 Inc
*/
/* $XFree86$ */

#ifndef _XF86DGA_H_
#define _XF86DGA_H_

#include <X11/Xfuncproto.h>
#include "xf86dga1.h"

#define X_XDGAQueryVersion		0


typedef struct {
   int num;		/* A unique identifier for the mode (num > 0) */
   char *name;		/* name of mode given in the XF86Config */
   float verticalRefresh;
   int flags;		/* DGA_CONCURRENT_ACCESS, etc... */
   int imageWidth;	/* linear accessible portion (pixels) */
   int imageHeight;
   int pixmapWidth;	/* Xlib accessible portion (pixels) */
   int pixmapHeight;	/* both fields ignored if no concurrent access */
   int bytesPerScanline; 
   int byteOrder;	/* MSBFirst, LSBFirst */
   int depth;		
   int bitsPerPixel;
   unsigned long red_mask;
   unsigned long green_mask;
   unsigned long blue_mask;
   int viewportWidth;
   int viewportHeight;
   int xViewportStep;	/* viewport position granularity */
   int yViewportStep;
   int maxViewportX;	/* max viewport origin */
   int maxViewportY;
   int viewportFlags;	/* types of page flipping possible */
   int reserved1;
   int reserved2;
} XDGAMode;


typedef struct {
   XDGAMode mode;
   unsigned char *data;
   Pixmap pixmap;
} XDGADevice;


#ifndef _XF86DGA_SERVER_
_XFUNCPROTOBEGIN


Bool XDGAQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
#endif
);

Bool XDGAQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);


_XFUNCPROTOEND
#endif /* _XF86DGA_SERVER_ */
#endif /* _XF86DGA_H_ */
