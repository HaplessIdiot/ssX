/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86cmap.h,v 1.2 1998/11/22 10:37:16 dawes Exp $ */

#ifndef _XF86CMAP_H
#define _XF86CMAP_H

#include "colormapst.h"

#define CMAP_PALETTED_TRUECOLOR		0x0000001
#define CMAP_RELOAD_ON_MODE_SWITCH	0x0000002

typedef void (*LoadPaletteFuncPtr)(
    ScrnInfoPtr pScrn, 
    int numColors, 
    int *indicies,
    LOCO *colors,
    short visualClass
);

typedef void (*SetOverscanFuncPtr)(
    ScrnInfoPtr pScrn,
    int index
);

Bool xf86HandleColormaps(
    ScreenPtr pScreen,
    int maxCol,
    int sigRGBbits,
    LoadPaletteFuncPtr loadPalette,
    SetOverscanFuncPtr setOverscan,
    unsigned int flags
);

int
xf86ChangeGamma(
   ScreenPtr pScreen,
   Gamma gamma
);

#endif /* _XF86CMAP_H */

