/* $XFree86: xc/lib/X11/ImUtil.h,v 1.1 2003/04/13 19:22:16 dawes Exp $ */

#ifndef _IMUTIL_H_
#define _IMUTIL_H_

extern int
_XGetScanlinePad(
    Display *dpy,
    int depth);

int
_XGetBitsPerPixel(
 Display *dpy,
 int depth);

extern int
_XSetImage(
    XImage *srcimg,
    register XImage *dstimg,
    register int x,
    register int y);

extern int
_XReverse_Bytes(
    register unsigned char *bpt,
    register int nb);
extern void
_XInitImageFuncPtrs(
    register XImage *image);

#endif /* _IMUTIL_H_ */
