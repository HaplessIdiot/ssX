/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/wincmap.c,v 1.6 2001/06/25 08:12:33 alanh Exp $ */

#include "win.h"

#if 0
gcc -o XWin.exe -O2 -fno-strength-reduce -ansi -pedantic -Wall -Wpointer-arith -L../../exports/lib hw/xwin/stubs.o dix/libdix.a os/libos.a ../../lib/Xau/libXau.a ../../lib/Xdmcp/libXdmcp.a hw/xwin/libXwin.a fb/libfb.a dix/libxpstubs.a mi/libmi.a Xext/libext.a xkb/libxkb.a Xi/libxinput.a lbx/liblbx.a ../../lib/lbxutil/liblbxutil.a dbe/libdbe.a record/librecord.a GL/glx/libglx.a GL/mesa/src/X/libGLcoreX.a GL/mesa/src/libGLcore.a render/librender.a miext/layer/liblayer.a miext/shadow/libshadow.a  -L/usr/X11R6/lib ../../lib/font/libXfont.a dix/libxpstubs.a -L../../exports/lib -lXext -lX11 -lz.dll -lgdi32 -lddraw

gcc -o XWin.exe -g -ansi -pedantic -Wall -Wpointer-arith -L../../exports/lib hw/xwin/stubs.o dix/libdix.a os/libos.a ../../lib/Xau/libXau.a ../../lib/Xdmcp/libXdmcp.a  hw/xwin/libXwin.a fb/libfb.a dix/libxpstubs.a mi/libmi.a Xext/libext.a xkb/libxkb.a Xi/libxinput.a lbx/liblbx.a ../../lib/lbxutil/liblbxutil.a dbe/libdbe.a record/librecord.a   GL/glx/libglx.a GL/mesa/src/X/libGLcoreX.a  GL/mesa/src/libGLcore.a render/librender.a randr/librandr.a miext/layer/liblayer.a miext/shadow/libshadow.a -L/usr/X11R6/lib ../../lib/font/libXfont.a dix/libxpstubs.a -L../../exports/lib -lXext -lX11 -lz.dll -lgdi32 -lddraw

gcc -o XWin.exe -O2 -fno-strength-reduce -ansi -pedantic -Wall -Wpointer-arith -L../../exports/lib hw/xwin/stubs.o dix/libdix.a os/libos.a ../../lib/Xau/libXau.a ../../lib/Xdmcp/libXdmcp.a  hw/xwin/libXwin.a fb/libfb.a dix/libxpstubs.a mi/libmi.a Xext/libext.a xkb/libxkb.a Xi/libxinput.a lbx/liblbx.a ../../lib/lbxutil/liblbxutil.a dbe/libdbe.a record/librecord.a GL/glx/libglx.a GL/mesa/src/X/libGLcoreX.a GL/mesa/src/libGLcore.a render/librender.a randr/librandr.a miext/shadow/libshadow.a miext/layer/liblayer.a -L/usr/X11R6/lib ../../lib/font/libXfont.a dix/libxpstubs.a -L../../exports/lib -lXext -lX11 -lz.dll -lgdi32 -lddraw
#endif

/* See Porting Layer Definition - p. 30 */
/*
 * Walk the list of installed colormaps, filling the pmaps list
 * with the resource ids of the installed maps, and return
 * a count of the total number of installed maps.
 */
int
winListInstalledColormaps (ScreenPtr pScreen, Colormap *pmaps)
{
#if WIN_PSEUDO_SUPPORT
  winScreenPriv(pScreen);

#if CYGDEBUG
  ErrorF ("winListInstalledColormaps ()\n");
#endif

  /*
   * There will only be one installed colormap, so we only need
   * to return one id, and the count of installed maps will always
   * be one.
   */
  *pmaps = pScreenPriv->pcmapInstalled->mid;
  return 1;

#else /* WIN_PSEUDO_SUPPORT */
  return miListInstalledColormaps (pScreen, pmaps);
#endif
}

/* See Porting Layer Definition - p. 30 */
/* See Programming Windows - p. 663 */
void
winInstallColormap (ColormapPtr pmap)
{
#if WIN_PSEUDO_SUPPORT
  ScreenPtr		pScreen = pmap->pScreen;
  winScreenPriv(pScreen);
  winCmapPriv(pmap);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  ColormapPtr		oldpmap = pScreenPriv->pcmapInstalled;

#if CYGDEBUG
  ErrorF ("winInstallColormap ()\n");
#endif
 
  /* Did the colormap actually change? */
  if (pmap != oldpmap)
    {
#if CYGDEBUG
      ErrorF ("winInstallColormap () - Colormap has changed, attempt "
	      "to install.\n");
#endif
      
      /* Was there a previous colormap? */
      if (oldpmap != (ColormapPtr) None)
	{
	  /* There was a previous colormap; tell clients it is gone */
	  WalkTree (pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);
	}
      
      /* Install new colormap */
      pScreenPriv->pcmapInstalled = pmap;
      WalkTree (pmap->pScreen, TellGainedMap, (char *)&pmap->mid);
      
      /*
       * Tell Windows to install the new colormap
       */
      if (SelectPalette (pScreenPriv->hdcScreen,
			 pCmapPriv->hPalette,
			 FALSE) == NULL)
	{
	  ErrorF ("winInstallColormap () - SelectPalette () failed\n");
	  return;
	}
      
      /* Realize the palette */
      if (GDI_ERROR == RealizePalette (pScreenPriv->hdcScreen))
	{
	  ErrorF ("winInstallColormap () - RealizePalette () failed\n");
	  return;
	}

      /* Set the DIB color table */
      if (SetDIBColorTable (pScreenPriv->hdcShadow,
			    0,
			    WIN_NUM_PALETTE_ENTRIES,
			    pCmapPriv->rgbColors) == 0)
	{
	  ErrorF ("winInstallColormap () - SetDIBColorTable () failed\n");
	  return;
	}

      /* Redraw the whole window, to take account for the new colors */
      BitBlt (pScreenPriv->hdcScreen,
	      0, 0,
	      pScreenInfo->dwWidth, pScreenInfo->dwHeight,
	      pScreenPriv->hdcShadow,
	      0, 0,
	      SRCCOPY);
    }

#else /* WIN_PSEUDO_SUPPORT */
  miInstallColormap (pmap);
#endif
}

/* See Porting Layer Definition - p. 30 */
void
winUninstallColormap (ColormapPtr pmap)
{
#if WIN_PSEUDO_SUPPORT
  winScreenPriv(pmap->pScreen);
  ColormapPtr curpmap = pScreenPriv->pcmapInstalled;

#if CYGDEBUG
  ErrorF ("winUninstallColormap ()\n");
#endif

  /* Is the colormap installed? */
  if (pmap != curpmap)
    {
      /* Colormap not installed, nothing to do */
      return;
    }

  /*
   * NOTE: The default colormap does not get "uninstalled" before
   * it is destroyed.
   */

  /* Install the default cmap in place of the cmap to be uninstalled */
  if (pmap->mid != pmap->pScreen->defColormap)
    {
      curpmap = (ColormapPtr) LookupIDByType(pmap->pScreen->defColormap,
					     RT_COLORMAP);
      (*pmap->pScreen->InstallColormap) (curpmap);
    }

#else /* WIN_PSEUDO_SUPPORT */
  miUninstallColormap (pmap);
#endif
}


/* See Porting Layer Definition - p. 30 */
void
winStoreColors (ColormapPtr pmap,
		int ndef,
		xColorItem *pdefs)
{
#if WIN_PSEUDO_SUPPORT
  winScreenPriv(pmap->pScreen);
  winCmapPriv(pmap);
  int			i;
  unsigned short	nRed, nGreen, nBlue;
  ColormapPtr curpmap = pScreenPriv->pcmapInstalled;

#if CYGDEBUG
  if (ndef != 1)
    ErrorF ("winStoreColors () - ndef: %d\n",
	    ndef);
#endif

  /* Save the new colors in the colormap privates */
  for (i = 0; i < ndef; ++i)
    {
      /* Adjust the colors from the X color spec to the Windows color spec */
      nRed = pdefs[i].red >> 8;
      nGreen = pdefs[i].green >> 8;
      nBlue = pdefs[i].blue >> 8;

      /* Copy the colors to a palette entry table */
      pCmapPriv->peColors[pdefs[0].pixel + i].peRed = nRed;
      pCmapPriv->peColors[pdefs[0].pixel + i].peGreen = nGreen;
      pCmapPriv->peColors[pdefs[0].pixel + i].peBlue = nBlue;
      
      /* Copy the colors to a RGBQUAD table */
      pCmapPriv->rgbColors[pdefs[0].pixel + i].rgbRed = nRed;
      pCmapPriv->rgbColors[pdefs[0].pixel + i].rgbGreen = nGreen;
      pCmapPriv->rgbColors[pdefs[0].pixel + i].rgbBlue = nBlue;

#if CYGDEBUG
      ErrorF ("winStoreColors () - nRed %d nGreen %d nBlue %d\n",
	      nRed, nGreen, nBlue);
#endif
    }

  /* Put the X colormap entries into the Windows logical palette */
  if (SetPaletteEntries (pCmapPriv->hPalette,
			 pdefs[0].pixel,
			 ndef,
			 pCmapPriv->peColors + pdefs[0].pixel) == 0)
    {
      ErrorF ("winStoreColors () - SetPaletteEntries () failed\n");
      return;
    }

  /* Don't install the Windows palette if the colormap is not isntalled */
  if (pmap != curpmap)
    {
      return;
    }

  /* Tell Windows that the palette has changed */
  RealizePalette (pScreenPriv->hdcScreen);
  
  /* Set the DIB color table */
  if (SetDIBColorTable (pScreenPriv->hdcShadow,
			pdefs[0].pixel,
			ndef,
			pCmapPriv->rgbColors + pdefs[0].pixel) == 0)
    {
      ErrorF ("winInstallColormap () - SetDIBColorTable () failed\n");
      return;
    }

#endif /* WIN_PSEUDO_SUPPORT */
}

/* See Porting Layer Definition - p. 30 */
void
winResolveColor (unsigned short *pred,
		 unsigned short *pgreen,
		 unsigned short *pblue,
		 VisualPtr	pVisual)
{
#if CYGDEBUG
  ErrorF ("winResolveColor ()\n");
#endif

#if 0
  if ((pVisual->class | DynamicClass) == PseudoColor)
    {
#if 0
      ErrorF ("winResolveColor () - PseudoColor\n");
#endif

      /* I'll resolve your damn color for ya, have a little of this! */
      *pred >>= 8;
      *pgreen >>= 8;
      *pblue >>= 8;
    }
  else
    {
#if 0
      ErrorF ("winResolveColor () - !PseudoColor\n");
#endif
    }
#endif

  miResolveColor (pred, pgreen, pblue, pVisual);

  ErrorF ("winResolveColor () - Returning %s %d %d %d\n",
	  (pVisual->class | DynamicClass) == PseudoColor ? "Pseudo" : "Other",
	  *pred, *pgreen, *pblue);
}

/* See Porting Layer Definition - p. 29 */
Bool
winCreateColormap (ColormapPtr pmap)
{
#if WIN_PSEUDO_SUPPORT
  LPLOGPALETTE		lpPaletteNew = NULL;
  VisualPtr		pVisual;
  DWORD			dwEntriesMax;
  DWORD			dwColorLimit;
  DWORD			dwShift;
  HPALETTE		hpalNew = NULL;
  winPrivCmapPtr	pCmapPriv = NULL;

#if CYGDEBUG
  ErrorF ("winCreateColormap ()\n");
#endif

  /* Allocate colormap privates */
  if (!winAllocateCmapPrivates (pmap))
    {
      ErrorF ("winCreateColorma () - Couldn't allocate cmap privates\n");
      return FALSE;
    }

  /* Get a pointer to the newly allocated privates */
  pCmapPriv = winGetCmapPriv (pmap);

  /*
   * FIXME: This is some evil hackery to help in handling some X clients
   * that expect the top pixel to be white.  This "help" only lasts until
   * some client overwrites the top colormap entry.
   * 
   * We don't want to actually allocate the top entry, as that causes
   * problems with X clients that need 7 planes (128 colors) in the default
   * colormap, such as Magic 7.1.
   */
  pCmapPriv->rgbColors[WIN_NUM_PALETTE_ENTRIES - 1].rgbRed = 255;
  pCmapPriv->rgbColors[WIN_NUM_PALETTE_ENTRIES - 1].rgbGreen = 255;
  pCmapPriv->rgbColors[WIN_NUM_PALETTE_ENTRIES - 1].rgbBlue = 255;
  pCmapPriv->peColors[WIN_NUM_PALETTE_ENTRIES - 1].peRed = 255;
  pCmapPriv->peColors[WIN_NUM_PALETTE_ENTRIES - 1].peGreen = 255;
  pCmapPriv->peColors[WIN_NUM_PALETTE_ENTRIES - 1].peBlue = 255;

  /* Get a pointer to the visual that the colormap belongs to */
  pVisual = pmap->pVisual;
  
  /* Set the top-end color limit */
  dwColorLimit = (1 << pVisual->bitsPerRGBValue) - 1;
  
  /* How many bits do we need to shift color values by? */
  dwShift = 16 - pVisual->bitsPerRGBValue;

  /* Get the maximum number of palette entries for this visual */
  dwEntriesMax = pVisual->ColormapEntries;
  
#if CYGDEBUG
  ErrorF ("winCreateColormap () - dwEntriesMax: %d\n",
	  dwEntriesMax);
#endif

  /* Allocate a Windows logical color palette with max entries */
  lpPaletteNew = xalloc (sizeof (LOGPALETTE)
			 + (dwEntriesMax - 1) * sizeof (PALETTEENTRY));
  if (lpPaletteNew == NULL)
    {
      ErrorF ("winCreateColormap () - Couldn't allocate palette "
	      "with %d entries\n",
	      dwEntriesMax);
      return FALSE;
    }

  /* Zero out the colormap */
  ZeroMemory (lpPaletteNew, sizeof (LOGPALETTE)
	      + (dwEntriesMax - 1) * sizeof (PALETTEENTRY));
  
  /* Set the logical palette structure */
  lpPaletteNew->palVersion = 0x0300;
  lpPaletteNew->palNumEntries = dwEntriesMax;

  /* Tell Windows to create the palette */
  hpalNew = CreatePalette (lpPaletteNew);
  if (hpalNew == NULL)
    {
      ErrorF ("winCreateColormap () - CreatePalette () failed\n");
      free (lpPaletteNew);
      return FALSE;
    }

  /* Save the Windows logical palette handle in the X colormaps' privates */
  pCmapPriv->hPalette = hpalNew;

  /* Free the palette initialization memory */
  xfree (lpPaletteNew);

  return TRUE;

#else /* WIN_PSEUDO_SUPPORT */
  return miInitializeColormap (pmap);
#endif
}

/* See Porting Layer Definition - p. 29, 30 */
void
winDestroyColormap (ColormapPtr pmap)
{
#if WIN_PSEUDO_SUPPORT
  winScreenPriv(pmap->pScreen);
  winCmapPriv(pmap);

#if CYGDEBUG
  ErrorF ("winDestroyColormap () pCmapPriv %08x\n",
	  pCmapPriv);
#endif

  /* Is colormap to be destroyed the default?
   *
   * Non-default colormaps should have had winUninstallColormap
   * called on them before we get here.  The default colormap
   * will not have had winUninstallColormap called on it.  Thus,
   * we need to handle the default colormap in a special way.
   */
  if (pmap->flags & IsDefault)
    {
#if CYGDEBUG
      ErrorF ("winDestroyColormap () - Destroying default colormap\n");
#endif
      
      /*
       * FIXME: Walk the list of all screens, popping the default
       * palette out of each screen device context.
       */
      
      /* Pop the palette out of the device context */
      SelectPalette (pScreenPriv->hdcScreen,
		     GetStockObject (DEFAULT_PALETTE),
		     FALSE);

      /* Clear our private installed colormap pointer */
      pScreenPriv->pcmapInstalled = NULL;
    }
  
  /* Try to delete the logical palette */
  if (DeleteObject (pCmapPriv->hPalette) == 0)
    {
      ErrorF ("winDestroyColormap () - DeleteObject () failed\n");
    }
  
  /* Gotta free go of the colormap privates */
  pCmapPriv->hPalette = NULL;
  xfree (pCmapPriv);
  winSetCmapPriv (pmap, NULL);

#if CYGDEBUG
  ErrorF ("winDestroyColormap () - Returning\n");
#endif

#endif /* WIN_PSEUDO_SUPPORT */
}

int
winExpandDirectColors (ColormapPtr pmap, int ndef,
		       xColorItem *indefs, xColorItem *outdefs)
{
  ErrorF ("\nwinExpandDirectColors ()\n");
  return miExpandDirectColors (pmap, ndef, indefs, outdefs);
}


/*
 *
 *
 *
 *
 */

/*
 * Load the palette used by the Shadow DIB
 */
static
Bool
winGetPaletteDIB (ScreenPtr pScreen, ColormapPtr pcmap)
{
  winScreenPriv(pScreen);
  int			i;
  Pixel			pixel; /* Pixel == CARD32 */
  CARD16		nRed, nGreen, nBlue; /* CARD16 == unsigned short */
  UINT			uiColorsRetrieved = 0;
  RGBQUAD		rgbColors[WIN_NUM_PALETTE_ENTRIES];
      
  /* Get the color table for the screen */
  uiColorsRetrieved = GetDIBColorTable (pScreenPriv->hdcScreen,
					0,
					WIN_NUM_PALETTE_ENTRIES,
					rgbColors);
  if (uiColorsRetrieved == 0)
    {
      ErrorF ("winGetPaletteDIB () - Could not retrieve screen color table\n");
      return FALSE;
    }

#if CYGDEBUG
  ErrorF ("winGetPaletteDIB () - Retrieved %d colors from DIB\n",
	  uiColorsRetrieved);
#endif

  /* Set the DIB color table to the default screen palette */
  if (SetDIBColorTable (pScreenPriv->hdcShadow,
			0,
			uiColorsRetrieved,
			rgbColors) == 0)
    {
      ErrorF ("winGetPaletteDIB () - SetDIBColorTable () failed\n");
      return FALSE;
    }

  /* Alloc each color in the DIB color table */
  for (i = 0; i < uiColorsRetrieved; ++i)
    {
      pixel = i;

      /* Extract the color values for current palette entry */
      nRed = rgbColors[i].rgbRed << 8;
      nGreen = rgbColors[i].rgbGreen << 8;
      nBlue = rgbColors[i].rgbBlue << 8;

#if CYGDEBUG
      ErrorF ("winGetPaletteDIB () - Allocating a color: %d; "\
	      "%d %d %d\n",
	      pixel, nRed, nGreen, nBlue);
#endif

      /* Allocate a entry in the X colormap */
      if (AllocColor (pcmap,
		      &nRed,
		      &nGreen,
		      &nBlue,
		      &pixel,
		      0) != Success)
	{
	  ErrorF ("winGetPaletteDIB () - AllocColor () failed, pixel %d\n",
		  i);
	  return FALSE;
	}

      if (i != pixel
	  || nRed != rgbColors[i].rgbRed 
	  || nGreen != rgbColors[i].rgbGreen
	  || nBlue != rgbColors[i].rgbBlue)
	{
	  ErrorF ("winGetPaletteDIB () - Got: %d; "\
		  "%d %d %d\n",
		  pixel, nRed, nGreen, nBlue);
	}
	  
      /* FIXME: Not sure that this bit is needed at all */
      pcmap->red[i].co.local.red = nRed;
      pcmap->red[i].co.local.green = nGreen;
      pcmap->red[i].co.local.blue = nBlue;
    }

  /* System is using a colormap */
  /* Set the black and white pixel indices */
  pScreen->whitePixel = uiColorsRetrieved - 1;
  pScreen->blackPixel = 0;

  return TRUE;
}

/*
 * Load the standard system palette being used by GDI
 */
static
Bool
winGetPaletteDD (ScreenPtr pScreen, ColormapPtr pcmap)
{
  int			i;
  Pixel			pixel; /* Pixel == CARD32 */
  CARD16		nRed, nGreen, nBlue; /* CARD16 == unsigned short */
  UINT			uiSystemPaletteEntries;
  LPPALETTEENTRY	ppeColors = NULL;
  HDC			hdc = NULL;

  /* Get a DC to obtain the default palette */
  hdc = GetDC (NULL);
  if (hdc == NULL)
    {
      ErrorF ("winGetPaletteDD () - Couldn't get a DC\n");
      return FALSE;
    }

  /* Get the number of entries in the system palette */
  uiSystemPaletteEntries = GetSystemPaletteEntries (hdc,
						    0, 0, NULL);
  if (uiSystemPaletteEntries == 0)
    {
      ErrorF ("winGetPaletteDD () - Unable to determine number of "
	      "system palette entries\n");
      return FALSE;
    }

#if CYGDEBUG
  ErrorF ("winGetPaletteDD () - uiSystemPaletteEntries %d\n",
	  uiSystemPaletteEntries);
#endif
  
  /* Allocate palette entries structure */
  ppeColors = xalloc (uiSystemPaletteEntries * sizeof (PALETTEENTRY));
  if (ppeColors == NULL)
    {
      ErrorF ("winGetPaletteDD () - xalloc () for colormap failed\n");
      return FALSE;
    }

  /* Get system palette entries */
  GetSystemPaletteEntries (hdc,
			   0, uiSystemPaletteEntries, ppeColors);

  /* Allocate an X colormap entry for every system palette entry */
  for (i = 0; i < uiSystemPaletteEntries; ++i)
    {
      pixel = i;

      /* Extract the color values for current palette entry */
      nRed = ppeColors[i].peRed << 8;
      nGreen = ppeColors[i].peGreen << 8;
      nBlue = ppeColors[i].peBlue << 8;
#if CYGDEBUG
      ErrorF ("winGetPaletteDD () - Allocating a color: %d; "\
	      "%d %d %d\n",
	      pixel, nRed, nGreen, nBlue);
#endif
      if (AllocColor (pcmap,
		      &nRed,
		      &nGreen,
		      &nBlue,
		      &pixel,
		      0) != Success)
	{
	  ErrorF ("winGetPaletteDD () - AllocColor () failed, pixel %d\n",
		  i);
	  free (ppeColors);
	  ppeColors = NULL;
	  return FALSE;
	}

      pcmap->red[i].co.local.red = nRed;
      pcmap->red[i].co.local.green = nGreen;
      pcmap->red[i].co.local.blue = nBlue;
    }

  /* System is using a colormap */
  /* Set the black and white pixel indices */
  pScreen->whitePixel = uiSystemPaletteEntries - 1;
  pScreen->blackPixel = 0;

  /* Free colormap */
  if (ppeColors != NULL)
    {
      free (ppeColors);
      ppeColors = NULL;
    }

  /* Free the DC */
  if (hdc != NULL)
    {
      ReleaseDC (NULL, hdc);
      hdc = NULL;
    }

  return TRUE;
}

/*
 * Install the standard fb colormap, or the GDI colormap,
 * depending on the current screen depth.
 */
Bool
winCreateDefColormap (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  unsigned short	zero = 0, ones = 0xFFFF;
  VisualPtr		pVisual = pScreenPriv->pRootVisual;
  ColormapPtr		pcmap = NULL;
  Pixel			wp, bp;

#if CYGDEBUG
  ErrorF ("winCreateDefColormap ()\n");
#endif

  /* Use standard fb colormaps for non palettized color modes */
  if (pScreenInfo->dwDepth > 8)
    {
      ErrorF ("winCreateDefColormap () - Deferring to " \
	      "fbCreateDefColormap ()\n");
      return fbCreateDefColormap (pScreen);
    }

  /*
   *  AllocAll for non-Dynamic visual classes,
   *  AllocNone for Dynamic visual classes.
   */

  /*
   * Dynamic visual classes allow the colors of the color map
   * to be changed by clients.
   */

#if CYGDEBUG
  ErrorF ("winCreateDefColormap () - defColormap: %d\n",
	  pScreen->defColormap);
#endif

  /* Allocate an X colormap, owned by client 0 */
  if (CreateColormap (pScreen->defColormap, 
		      pScreen,
		      pVisual,
		      &pcmap,
		      (pVisual->class & DynamicClass) ? AllocNone : AllocAll,
		      0) != Success)
    {
      ErrorF ("winCreateDefColormap () - CreateColormap failed\n");
      return FALSE;
    }
  if (pcmap == NULL)
    {
      ErrorF ("winCreateDefColormap () - Colormap could not be created\n");
      return FALSE;
    }

#if CYGDEBUG
  ErrorF ("winCreateDefColormap () - Created a colormap\n");
#endif

  /* Branch on the visual class */
  if (!(pVisual->class & DynamicClass))
    {
      /* Branch on engine type */
      if (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI)
	{
	  /* Load the colors being used by the Shadow DIB */
	  if (!winGetPaletteDIB (pScreen, pcmap))
	    {
	      ErrorF ("winCreateDefColormap () - Couldn't get DIB colors\n");
	      return FALSE;
	    }
	}
      else
	{
	  /* Load the colors from the default system palette */
	  if (!winGetPaletteDD (pScreen, pcmap))
	    {
	      ErrorF ("winCreateDefColormap () - Couldn't get colors "
		      "for DD\n");
	      return FALSE;
	    }
	}
    }
  else
    {
      wp = pScreen->whitePixel;
      bp = pScreen->blackPixel;
      
      /* Allocate a black and white pixel */
      if ((AllocColor(pcmap, &ones, &ones, &ones, &wp, 0) !=
	   Success)
	  ||
	  (AllocColor(pcmap, &zero, &zero, &zero, &bp, 0) !=
	   Success))
	{
	  ErrorF ("winCreateDefColormap () - Couldn't allocate bp or wp\n");
	  return FALSE;
	}
      
      pScreen->whitePixel = wp;
      pScreen->blackPixel = bp;
    }

  /* Install the created colormap */
  (*pScreen->InstallColormap)(pcmap);

#if CYGDEBUG
  ErrorF ("winCreateDefColormap () - Returning\n");
#endif

  return TRUE;
}
