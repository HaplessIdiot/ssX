/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDacPriv.h,v 1.2 1998/07/25 16:57:20 dawes Exp $ */

#include "xf86RamDac.h"
#include "xf86cmap.h"

void RamDacGetRecPrivate(void);
Bool RamDacGetRec(ScrnInfoPtr pScrn);
int  RamDacGetScreenIndex(void);
void RamDacLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
    			LOCO *colors, short visualClass);
