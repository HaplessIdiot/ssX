/* $XFree86$ */

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

Bool xf86HandleColormaps(
    ScreenPtr pScreen,
    int maxCol,
    int sigRGBbits,
    LoadPaletteFuncPtr loadPalette,
    unsigned int flags
);

#endif /* _XF86CMAP_H */

