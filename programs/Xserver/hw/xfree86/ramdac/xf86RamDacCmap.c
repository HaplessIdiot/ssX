/*
 * Copyright 1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Generic RAMDAC access to colormaps.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDacCmap.c,v 1.1.2.4 1998/07/19 13:22:08 dawes Exp $ */

#include "X.h"
#include "Xproto.h"
#include "windowstr.h"
#include "compiler.h"
#include "mipointer.h"
#include "micmap.h"

#include "xf86.h"
#include "xf86_ansic.h"
#include "colormapst.h"
#include "xf86RamDacPriv.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#ifdef XFreeXDGA
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#include "dgaproc.h"
#endif

#define NOMAPYET        (ColormapPtr) 0

int
RamDacListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps)
{
  /* By the time we are processing requests, we can guarantee that there
   * is always a colormap installed */
  
  *pmaps = InstalledMaps[pScreen->myNum]->mid;
  return(1);
}

int
RamDacGetInstalledColormaps(ScreenPtr pScreen, ColormapPtr *pmaps)
{
  /* By the time we are processing requests, we can guarantee that there
   * is always a colormap installed */
  
  *pmaps = InstalledMaps[pScreen->myNum];
  return(1);
}

static int
RamDacCheckColorMap(ColormapPtr pmap)
{
  return (pmap != InstalledMaps[pmap->pScreen->myNum]);
}


void
RamDacStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs)
{
    int		i;
    unsigned char *cmap;
    xColorItem	directDefs[256];
    Bool writeColormap;

    /* This can get called before the ScrnInfoRec is installed so we
       can't rely on getting it with XF86SCRNINFO() */
    int scrnIndex = pmap->pScreen->myNum;
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RamDacHWRecPtr pRamDac = RAMDACHWPTR(pScrn);
#if PERSCREEN
    RamDacRecPtr hwp = RAMDACSCRPTR(pScrn);
#else
    RamDacRecPtr hwp = RAMDACCOLPTR(pmap);
#endif
    
    if (RamDacCheckColorMap(pmap))
        return;

    if ((pmap->pVisual->class | DynamicClass) == DirectColor)
    {
        ndef = miExpandDirectColors (pmap, ndef, pdefs, directDefs);
        pdefs = directDefs;
    }
    
    writeColormap = pScrn->vtSema;
#ifdef XFreeXDGA
    if (DGAAvailable(scrnIndex))
    {
	writeColormap = writeColormap ||
			(DGAGetDirectMode(scrnIndex) &&
			 !(DGAGetFlags(scrnIndex) & XF86DGADirectColormap)) ||
			(DGAGetFlags(scrnIndex) & XF86DGAHasColormap);
    }
#endif
    for(i = 0; i < ndef; i++)
    {
        cmap = &(pRamDac->ModeReg.DAC[pdefs[i].pixel*3]);

	if (pScrn->rgbBits == 8) {
            cmap[0] = pdefs[i].red   >> 8;
            cmap[1] = pdefs[i].green >> 8;
            cmap[2] = pdefs[i].blue  >> 8;
        } else {
            cmap[0] = pdefs[i].red   >> 10;
            cmap[1] = pdefs[i].green >> 10;
            cmap[2] = pdefs[i].blue  >> 10;
        }

        if (writeColormap)
	{
	    (*hwp->WriteAddress)(pScrn, pdefs[i].pixel);
	    (*hwp->WriteData)(pScrn, cmap[0]);
	    (*hwp->WriteData)(pScrn, cmap[1]);
	    (*hwp->WriteData)(pScrn, cmap[2]);
	}
    }	
    /* MAY NEED TO DEAL WITH OVERSCAN ? LIKE VGAHW */
}

void
RamDacInstallColormap(ColormapPtr pmap)
{
  ColormapPtr oldmap = InstalledMaps[pmap->pScreen->myNum];
  int         entries;
  Pixel *     ppix;
  xrgb *      prgb;
  xColorItem *defs;
  int         i;


  if (pmap == oldmap)
    return;

  if ((pmap->pVisual->class | DynamicClass) == DirectColor)
    entries = (pmap->pVisual->redMask |
	       pmap->pVisual->greenMask |
	       pmap->pVisual->blueMask) + 1;
  else
    entries = pmap->pVisual->ColormapEntries;

  ppix = (Pixel *)ALLOCATE_LOCAL( entries * sizeof(Pixel));
  prgb = (xrgb *)ALLOCATE_LOCAL( entries * sizeof(xrgb));
  defs = (xColorItem *)ALLOCATE_LOCAL(entries * sizeof(xColorItem));

  if ( oldmap != NOMAPYET)
    WalkTree( pmap->pScreen, TellLostMap, &oldmap->mid);

  InstalledMaps[pmap->pScreen->myNum] = pmap;

  for ( i=0; i<entries; i++) ppix[i] = i;

  QueryColors( pmap, entries, ppix, prgb);

  for ( i=0; i<entries; i++) /* convert xrgbs to xColorItems */
    {
      defs[i].pixel = ppix[i];
      defs[i].red = prgb[i].red;
      defs[i].green = prgb[i].green;
      defs[i].blue = prgb[i].blue;
      defs[i].flags =  DoRed|DoGreen|DoBlue;
    }

  pmap->pScreen->StoreColors( pmap, entries, defs);

  WalkTree(pmap->pScreen, TellGainedMap, &pmap->mid);
  
  DEALLOCATE_LOCAL(ppix);
  DEALLOCATE_LOCAL(prgb);
  DEALLOCATE_LOCAL(defs);
}

void
RamDacUninstallColormap(ColormapPtr pmap)
{

  ColormapPtr defColormap;
  
  if ( pmap != InstalledMaps[pmap->pScreen->myNum] )
    return;

  defColormap = (ColormapPtr) LookupIDByType( pmap->pScreen->defColormap,
					      RT_COLORMAP);

  if (defColormap == InstalledMaps[pmap->pScreen->myNum])
    return;

  (*pmap->pScreen->InstallColormap) (defColormap);
}

void
RamDacHandleColormaps(ScreenPtr pScreen, ScrnInfoPtr scrnp)
{
  if (scrnp->bitsPerPixel > 1) {
     if (scrnp->bitsPerPixel <= 8) { /* For 8bpp SVGA and VGA16 */
        pScreen->InstallColormap = RamDacInstallColormap;
        pScreen->UninstallColormap = RamDacUninstallColormap;
        pScreen->ListInstalledColormaps = RamDacListInstalledColormaps;
        pScreen->StoreColors = RamDacStoreColors;
    }
  }
}

void
RamDacSetGamma(ScrnInfoPtr pScrn, Bool Real8BitDac)
{
#ifdef PERSCREEN
    RamDacRecPtr hwp = RAMDACSCRPTR(pScrn);
#endif
    int i;
    unsigned char value;

    (*hwp->WriteAddress)(pScrn, 0);
      
    if (Real8BitDac || 
	((pScrn->weight.red == 8) && 
	 (pScrn->weight.green == 8) && 
	 (pScrn->weight.blue == 8)) ) {
    		for (i=0; i<256; i++) {
	    	    value = pow(i/255.0,pScrn->gamma.red)*255.0+0.5;
	    	    (*hwp->WriteData)(pScrn, value);
	    	    value = pow(i/255.0,pScrn->gamma.green)*255.0+0.5;
	    	    (*hwp->WriteData)(pScrn, value);
	    	    value = pow(i/255.0,pScrn->gamma.blue)*255.0+0.5;
	    	    (*hwp->WriteData)(pScrn, value);
    		}
    } else {
	int r,g,b;

	r = (1 << pScrn->weight.red) - 1;
	g = (1 << pScrn->weight.green) - 1;
	b = (1 << pScrn->weight.blue) - 1;

	for (i=0; i<256; i++) {
	    value = (i >> (6 - pScrn->weight.red)) & r;
	    (*hwp->WriteData)(pScrn, (value*255+r/2)/r);
	    value = (i >> (6 - pScrn->weight.green)) & g;
	    (*hwp->WriteData)(pScrn, (value*255+g/2)/g);
	    value = (i >> (6 - pScrn->weight.blue)) & b;
	    (*hwp->WriteData)(pScrn, (value*255+b/2)/b);
	}
    }
}

void
#ifdef PERSCREEN
RamDacRestoreDACValues(ScrnInfoPtr pScrn)
#else
RamDacRestoreDACValues(ScrnInfoPtr pScrn, RamDacRecPtr hwp)
#endif
{
    int i;
    RamDacHWRecPtr pRamDac = RAMDACHWPTR(pScrn);
#ifdef PERSCREEN
    RamDacRecPtr hwp = RAMDACSCRPTR(pScrn);
#endif
    unsigned char *cmap;

    (*hwp->WriteAddress)(pScrn, 0);
      
    for (i=0; i<256; i++) {
   	cmap = &(pRamDac->ModeReg.DAC[i*3]);
	(*hwp->WriteData)(pScrn, cmap[0]);
	(*hwp->WriteData)(pScrn, cmap[1]);
	(*hwp->WriteData)(pScrn, cmap[2]);
    }
}

#ifndef PERSCREEN
static int
RamDacCmapPrivInit (pmap)
{
    pmap->devPrivates[RamDacColormapPrivateIndex].ptr = NULL;
    return 1;
}
#endif
