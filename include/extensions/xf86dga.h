/*
   Copyright (c) 1999  XFree86 Inc
*/
/* $XFree86: xc/include/extensions/xf86dga.h,v 3.7 1999/03/28 15:31:33 dawes Exp $ */

#ifndef _XF86DGA_H_
#define _XF86DGA_H_

#include <X11/Xfuncproto.h>
#include "xf86dga1.h"

#define X_XDGAQueryVersion		0

/* 1 through 9 are in xf86dga1.h */

#define X_XDGAQueryModes		10
#define X_XDGAOpenFramebuffer		11
#define	X_XDGACloseFramebuffer		12
#define X_XDGASetMode			13
#define X_XDGASetViewport		14
#define X_XDGAInstallColormap		15


#define XDGAConcurrentAcces	0x00000001
#define XDGASolidFillRect	0x00000002
#define XDGABlitRect		0x00000004
#define XDGABlitTransRect	0x00000008
#define XDGAPixmap    		0x00000010

#define XDGAInterlaced          0x00010000
#define XDGADoublescan          0x00020000

#define XDGAFlipImmediate	0x00000001
#define XDGAFlipRetrace		0x00000002

#define XDGACompleted           0x00000000
#define XDGAPending             0x00000001

#define XDGANeedRoot		0x00000001



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
    Display 	*dpy,
    int 	*eventBase,
    int 	*erroBase
);

Bool XDGAQueryVersion(
    Display 	*dpy,
    int 	*majorVersion,
    int 	*minorVersion
);

XDGAMode* XDGAQueryModes(
    Display	*dpy,
    int 	screen,
    int		*num
);

XDGADevice* XDGASetMode(
    Display	*dpy,
    int		screen,
    int		mode
);

Bool XDGAOpenFramebuffer(
    Display	*dpy,
    int 	screen
);

void XDGACloseFramebuffer(
    Display	*dpy,
    int		screen
);

void XDGASetViewport(
    Display	*dpy,
    int		screen,
    int		x,
    int		y,
    int		flags
);

void XDGAInstallColormap(
    Display	*dpy,
    int		screen,
    Colormap	cmap
);

void XDGAFlush(
    Display	*dpy,
    int		screen
);   

_XFUNCPROTOEND
#endif /* _XF86DGA_SERVER_ */
#endif /* _XF86DGA_H_ */
