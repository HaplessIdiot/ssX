/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach64/mach64cmap.c,v 3.11 1998/01/24 16:56:50 hohndel Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993,1994 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 *
 * THOMAS ROELL, KEVIN E. MARTIN, AND RICKARD E. FAITH DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THOMAS
 * ROELL OR KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Modified for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach64 by Kevin E. Martin (martin@cs.unc.edu)
 *
 */
/* $TOG: mach64cmap.c /main/10 1997/10/19 15:02:58 kaleb $ */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "windowstr.h"
#include "cfb.h"
#include "compiler.h"
#include "mach64.h"

#ifdef XFreeXDGA
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#define NOMAPYET        (ColormapPtr) 0

ColormapPtr InstalledMaps[MAXSCREENS]; /* current colormap for each screen */

int
mach64ListInstalledColormaps(pScreen, pmaps)
     ScreenPtr	pScreen;
     Colormap	*pmaps;
{
  /* By the time we are processing requests, we can guarantee that there
   * is always a colormap installed */
  
  *pmaps = InstalledMaps[pScreen->myNum]->mid;
  return(1);
}

int
mach64GetInstalledColormaps(pScreen, pmap)
     ScreenPtr		pScreen;
     ColormapPtr	*pmap;
{
  *pmap = InstalledMaps[pScreen->myNum];
  return(1);
}

void
mach64StoreColors(pmap, ndef, pdefs)
     ColormapPtr	pmap;
     int		ndef;
     xColorItem	        *pdefs;
{
    int		i,nshift;
    xColorItem	directDefs[256];
    extern LUTENTRY mach64savedLUT[256];

    if (pmap != InstalledMaps[pmap->pScreen->myNum])
	return;

    if ((pmap->pVisual->class | DynamicClass) == DirectColor) {
	ndef = cfbExpandDirectColors (pmap, ndef, pdefs, directDefs);
	pdefs = directDefs;
    }

    nshift = ((mach64DAC8Bit) ? 8 : 10);

    for (i = 0; i < ndef; i++) {
	unsigned char red, green, blue;
	/* Return the n most significant bits from a 16-bit value.
	 * For VGA, n = 6.  For 8-bit DACs, n = 8.
	 */
	red   = mach64savedLUT[pdefs[i].pixel].r = pdefs[i].red >> nshift;
	green = mach64savedLUT[pdefs[i].pixel].g = pdefs[i].green >> nshift;
	blue  = mach64savedLUT[pdefs[i].pixel].b = pdefs[i].blue >> nshift;

	if (xf86VTSema
#ifdef XFreeXDGA
	    || ((mach64InfoRec.directMode & XF86DGADirectGraphics)
	        && !(mach64InfoRec.directMode & XF86DGADirectColormap))
	    || (mach64InfoRec.directMode & XF86DGAHasColormap)
#endif
	   ) {
            /* WaitQueue(4); */
            outb(ioDAC_REGS, pdefs[i].pixel);
            outb(ioDAC_REGS+1, red);
            outb(ioDAC_REGS+1, green);
            outb(ioDAC_REGS+1, blue);
	}
    }
    checkCursorColor = TRUE;
}

void
mach64InstallColormap(pmap)
     ColormapPtr	pmap;
{
  ColormapPtr oldmap = InstalledMaps[pmap->pScreen->myNum];
  int         entries;
  Pixel *     ppix;
  xrgb *      prgb;
  xColorItem *defs;
  int         i,j;


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

  if (pmap->class == GrayScale || pmap->class == PseudoColor)
    {
      for ( i=j=0; i<entries; i++) 
        {
	  if (pmap->red[i].fShared || pmap->red[i].refcnt != 0)
	    {
	      defs[j].pixel = i;
              defs[j].flags = DoRed|DoGreen|DoBlue;
	      if (pmap->red[i].fShared)
	        {
	          defs[j].red = pmap->red[i].co.shco.red->color;
	          defs[j].green = pmap->red[i].co.shco.green->color;
	          defs[j].blue = pmap->red[i].co.shco.blue->color;
	        }
	        else if (pmap->red[i].refcnt != 0)
	        {
	          defs[j].red = pmap->red[i].co.local.red;
	          defs[j].green = pmap->red[i].co.local.green;
	          defs[j].blue = pmap->red[i].co.local.blue;
	        }
	      j++;
	    }
        }
      entries = j;
    }
  else
    {
      QueryColors( pmap, entries, ppix, prgb);

      for ( i=0; i<entries; i++) /* convert xrgbs to xColorItems */
        {
          defs[i].pixel = ppix[i];
          defs[i].red = prgb[i].red;
          defs[i].green = prgb[i].green;
          defs[i].blue = prgb[i].blue;
          defs[i].flags =  DoRed|DoGreen|DoBlue;
        }
    }

  mach64StoreColors( pmap, entries, defs);

  WalkTree(pmap->pScreen, TellGainedMap, &pmap->mid);

  mach64RenewCursorColor(pmap->pScreen);

  DEALLOCATE_LOCAL(ppix);
  DEALLOCATE_LOCAL(prgb);
  DEALLOCATE_LOCAL(defs);
}

void
mach64UninstallColormap(pmap)
     ColormapPtr pmap;
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

/* This is for the screen saver */
void
mach64RestoreColor0(pScreen)
     ScreenPtr pScreen;
{
  Pixel       pix = 0;
  xrgb        rgb;

  if (InstalledMaps[pScreen->myNum] == NOMAPYET)
      return;

  QueryColors(InstalledMaps[pScreen->myNum], 1, &pix, &rgb);

    /* WaitQueue(4); */
    outb(ioDAC_REGS, 0);
    if (mach64DAC8Bit) {
        outb(ioDAC_REGS+1, rgb.red >> 8);
        outb(ioDAC_REGS+1, rgb.green >> 8);
        outb(ioDAC_REGS+1, rgb.blue >> 8);
    } else {
        outb(ioDAC_REGS+1, rgb.red >> 10);
        outb(ioDAC_REGS+1, rgb.green >> 10);
        outb(ioDAC_REGS+1, rgb.blue >> 10);
    }
}
