/* $XConsortium: vgabpp.h,v 1.2 95/06/19 19:33:39 kaleb Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86scrin.h,v 3.1 1997/01/24 01:04:50 dawes Exp $ */

extern Bool xf86XAAScreenInit(
#if NeedFunctionPrototypes
    ScreenPtr /* pScreen */,
    pointer /* pbits */,
    int /* xsize */,
    int /* ysize */,
    int /* dpix */,
    int /* dpiy */,
    int /* width */
#endif
);

