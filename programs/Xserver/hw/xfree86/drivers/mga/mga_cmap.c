/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_cmap.c,v 1.4 1998/09/20 06:01:23 dawes Exp $ */

#include "X.h"
#include "misc.h"
#include "Xproto.h"
#include "colormapst.h"

#include "resource.h"

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "micmap.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#include "dgaproc.h"
#endif

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "mga_bios.h"
#include "mga.h"
#include "mga_reg.h"


static int
MGAListInstalledColormaps(
     ScreenPtr	pScreen,
     Colormap	*pmaps
){
  *pmaps = InstalledMaps[pScreen->myNum]->mid;
  return(1);
}


static void
MGAStoreColors(
     ColormapPtr	pmap,
     int		ndef,
     xColorItem	        *pdefs
){
    int 	index = pmap->pScreen->myNum;
    ScrnInfoPtr pScrn = xf86Screens[index];
    Bool	writeColormap;
    xColorItem	directDefs[256];
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr pReg = &hwp->ModeReg;
    unsigned char *pal;
    int i;
    int shift;

    if (pmap != InstalledMaps[pmap->pScreen->myNum])
      return;


    writeColormap = pScrn->vtSema;
#ifdef XFreeXDGA
    if (DGAAvailable(index)) {
      writeColormap = writeColormap ||
	(DGAGetDirectMode(index) &&
	 !(DGAGetFlags(index) & XF86DGADirectColormap)) ||
	(DGAGetFlags(index) & XF86DGAHasColormap);
    }
#endif


    if ( pmap->pVisual->nplanes > MAX_PSEUDO_DEPTH ) {
      /* green because it is deepest color in 565 mode */
      shift = 8 - pScrn->weight.green;
    } else {
      if ((pmap->pVisual->class | DynamicClass) == DirectColor) {
        ndef = miExpandDirectColors (pmap, ndef, pdefs, directDefs);
        pdefs = directDefs;
      }
      shift = 0;
    }

    for(i = 0; i < ndef; i++) {
        /* miExpandColors uses bottom bits of LUT address, 
         * but ramdac uses top bits of LUT address.
         */
        pdefs[i].pixel = pdefs[i].pixel << shift;
        pal = pReg->DAC + (pdefs[i].pixel * 3);
        pal[0] = pdefs[i].red >> 8;
        pal[1] = pdefs[i].green >> 8;
        pal[2] = pdefs[i].blue >> 8;
    }

    if(writeColormap)
      (*MGAdac->StoreColors)(pScrn, pdefs, ndef);

}



static void
MGAInstallColormap(ColormapPtr pmap)
{
    int index = pmap->pScreen->myNum;
    ScrnInfoPtr pScrn = xf86Screens[index];
    ColormapPtr oldpmap = InstalledMaps[index];
    VisualPtr pVisual = pmap->pVisual;
    Pixel 	*ppix;
    xrgb 	*prgb;
    xColorItem 	*defs;
    int         entries, i;


    if (pmap == oldpmap)
      return;

    if(oldpmap != (ColormapPtr)None)
	WalkTree(pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);

    InstalledMaps[index] = pmap;

    WalkTree(pmap->pScreen, TellGainedMap, (char *)&pmap->mid);

    /* This is where cfb left off */

    /* Is this correct ? */
    if(pScrn->bitsPerPixel == 8) {
	if ((pmap->pVisual->class | DynamicClass) == DirectColor)
	     entries = (pmap->pVisual->redMask |
			pmap->pVisual->greenMask |
			pmap->pVisual->blueMask) + 1;
	else
	     entries = pmap->pVisual->ColormapEntries;
    } else if ((pmap->pVisual->class | DynamicClass) == DirectColor) {
      entries = pmap->pVisual->ColormapEntries;
    } else if ((pScrn->bitsPerPixel == 32) && 
		(pmap->pVisual->class == PseudoColor)) {
	     entries = pmap->pVisual->ColormapEntries;
    } else return;


    ppix = (Pixel *)ALLOCATE_LOCAL( entries * sizeof(Pixel));
    prgb = (xrgb *)ALLOCATE_LOCAL( entries * sizeof(xrgb));
    defs = (xColorItem *)ALLOCATE_LOCAL(entries * sizeof(xColorItem));

    for (i = 0; i < entries; i++ ) {
      if ( pVisual->nplanes <= MAX_PSEUDO_DEPTH ) {
	ppix[i] = i; 
      } else {
	/*
	  int r = (i << pVisual->offsetRed) & pVisual->redMask;
	  int g = (i << pVisual->offsetGreen) & pVisual->greenMask;
	  int b = (i << pVisual->offsetBlue) & pVisual->blueMask;
	  */
	int maxEnt = entries-1;
	int r = ((i *  pVisual->redMask)  / maxEnt) & pVisual->redMask;
	int g = ((i *  pVisual->greenMask)/ maxEnt) & pVisual->greenMask;
	int b = ((i *  pVisual->blueMask) / maxEnt) & pVisual->blueMask;
	ppix[i] = r | g | b ;
      }
    }

    QueryColors( pmap, entries, ppix, prgb);

    for ( i=0; i<entries; i++) { /* convert xrgbs to xColorItems */
	defs[i].pixel = i;
	defs[i].red = prgb[i].red;
	defs[i].green = prgb[i].green;
	defs[i].blue = prgb[i].blue;
	defs[i].flags =  DoRed|DoGreen|DoBlue;
    }

    pmap->pScreen->StoreColors( pmap, entries, defs);
  
    DEALLOCATE_LOCAL(ppix);
    DEALLOCATE_LOCAL(prgb);
    DEALLOCATE_LOCAL(defs);
}



static void
MGAUninstallColormap(ColormapPtr pmap)
{
  ColormapPtr defColormap;
  
  if(pmap != InstalledMaps[pmap->pScreen->myNum]) return;

  defColormap = (ColormapPtr) LookupIDByType( pmap->pScreen->defColormap,
					      RT_COLORMAP);

  if(defColormap == InstalledMaps[pmap->pScreen->myNum]) return;

  (*pmap->pScreen->InstallColormap) (defColormap);
}




void
MGAHandleColormaps(ScreenPtr pScreen, ScrnInfoPtr pScrn)
{
   if(pScrn->bitsPerPixel == 1) return;

   pScreen->InstallColormap = MGAInstallColormap;
   pScreen->UninstallColormap = MGAUninstallColormap;
   pScreen->ListInstalledColormaps = MGAListInstalledColormaps;
   pScreen->StoreColors = MGAStoreColors;
}
