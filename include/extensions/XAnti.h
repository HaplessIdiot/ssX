/*
   Copyright (C) 1998 by The XFree86 Project Inc.

*/
/* $XFree86$ */


#ifndef _XANTI_H
#define _XANTI_H

#include <X11/X.h>
#include <X11/Xproto.h>

Bool
XAntiQueryExtension(
    Display *dpy,
    unsigned int *p_version, 
    unsigned int *p_revision
);

int
XAntiSetInterpolationPixels(
    Display *dpy,
    GC gc,
    int num,
    unsigned long *pixels
);

int
XAntiInterpolateColors(
    Display *dpy,
    GC gc,
    Colormap cmap,
    int num,
    XColor *colors
);


int
XAntiDrawString(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length
);

int
XAntiDrawString16(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length
);

int
XAntiDrawText(
    Display *dpy,
    Drawable d,
    GC gc,
    int x, 
    int y,
    XTextItem *items,
    int nitems
);

int
XAntiDrawText16(
    Display *dpy,
    Drawable d,
    GC gc,
    int x, 
    int y,
    XTextItem16 *items,
    int nitems
);

int
XAntiDrawImageString(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length
);

int
XAntiDrawImageString16(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length
);

#endif /* _XANTI_H */
