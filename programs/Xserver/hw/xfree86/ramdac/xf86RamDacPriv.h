/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDacPriv.h,v 1.1.2.1 1998/07/11 13:52:38 dawes Exp $ */

#include "xf86RamDac.h"

void RamDacGetRecPrivate(void);
Bool RamDacGetRec(ScrnInfoPtr pScrn);
int  RamDacGetScreenIndex(void);
void RamDacStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs);
void RamDacInstallColormap(ColormapPtr pmap);
void RamDacUninstallColormap(ColormapPtr pmap);
int  RamDacListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);
int  RamDacGetInstalledColormaps(ScreenPtr pScreen, ColormapPtr *pmaps);
