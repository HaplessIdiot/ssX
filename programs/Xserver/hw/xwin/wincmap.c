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
/* $XFree86: xc/programs/Xserver/hw/xwin/wincmap.c,v 1.5 2001/06/05 10:10:28 alanh Exp $ */

#include "win.h"

#define NUM_PALETTE_ENTRIES		256

#if 0
gcc -o XWin.exe -O2 -fno-strength-reduce -ansi -pedantic -Wall -Wpointer-arith -L../../exports/lib hw/xwin/stubs.o dix/libdix.a os/libos.a ../../lib/Xau/libXau.a ../../lib/Xdmcp/libXdmcp.a hw/xwin/libXwin.a fb/libfb.a dix/libxpstubs.a mi/libmi.a Xext/libext.a xkb/libxkb.a Xi/libxinput.a lbx/liblbx.a ../../lib/lbxutil/liblbxutil.a dbe/libdbe.a record/librecord.a GL/glx/libglx.a GL/mesa/src/X/libGLcoreX.a GL/mesa/src/libGLcore.a render/librender.a  miext/shadow/libshadow.a -L/usr/X11R6/lib ../../lib/font/libXfont.a dix/libxpstubs.a -L../../exports/lib -lXext -lX11 -lz.dll -lgdi32 -lddraw
#endif

/* See Porting Layer Definition - p. 30 */
int
winListInstalledColormaps (ScreenPtr pScreen, Colormap *pmaps)
{
#if WIN_PSEUDO_SUPPORT
  winScreenPriv(pScreen);

  /* 
   * By the time we are processing requests, we can guarantee that there
   * is always a colormap installed
   */
  
  ErrorF ("winListInstalledColormaps ()\n");
  *pmaps = pScreenPriv->pcmapInstalledColormap->mid;
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
  winScreenPriv(pmap->pScreen);
  ColormapPtr		oldpmap = pScreenPriv->pcmapInstalledColormap;
  
  ErrorF ("winInstallColormap ()\n");
 
  /* Did the colormap actually change? */
  if (pmap != oldpmap)
    {
      /* Was there a previous colormap? */
      if (oldpmap != (ColormapPtr)None)
	{
	  /* Previous colormap, tells clients it is gone */
	  WalkTree (pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);
	}
      
      /* Install new colormap */
      pScreenPriv->pcmapInstalledColormap = pmap;
      WalkTree (pmap->pScreen, TellGainedMap, (char *)&pmap->mid);
      
      /*
       * Tell Windows to install the new colormap
       */
      if (SelectPalette (pScreenPriv->hdcScreen,
			 (HPALETTE)pmap->devPriv,
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
  ColormapPtr curpmap = pScreenPriv->pcmapInstalledColormap;

  ErrorF ("winUninstallColormap ()\n");

  /* Is the palette the current palette? */
  if (pmap == curpmap)
    {
      /* Install the default colormap in place of the pmap to be uninstalled */
      if (pmap->mid != pmap->pScreen->defColormap)
	{
	  curpmap = (ColormapPtr) LookupIDByType(pmap->pScreen->defColormap,
						 RT_COLORMAP);
	  (*pmap->pScreen->InstallColormap) (curpmap);

	  /* Pop the palette out of the screen DC */
	  SelectPalette (pScreenPriv->hdcScreen, 
			 (HPALETTE) curpmap->devPriv,
			 FALSE);
	}
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
  LPPALETTEENTRY	lpPalEntries = NULL;
  int			i;

  if (ndef != 1)
    ErrorF ("winStoreColors () - ndef: %d\n",
	    ndef);

  /* Allocate palette entries for X colors */
  lpPalEntries = xalloc (ndef * sizeof (PALETTEENTRY));
  if (lpPalEntries == NULL)
    {
      ErrorF ("winStoreColors () - Failed to allocated %d palette entries\n",
	      ndef);
      return;
    }

  /* Transform the X colormap entries into Windows palette entries */
  for (i = 0; i < ndef; ++i)
    {
#if 0
      lpPalEntries[i].peRed = pdefs[i].red >> 8;
      lpPalEntries[i].peGreen = pdefs[i].green >> 8;
      lpPalEntries[i].peBlue = pdefs[i].blue >> 8;
#endif

      lpPalEntries[i].peRed = pdefs[i].red;
      lpPalEntries[i].peGreen = pdefs[i].green;
      lpPalEntries[i].peBlue = pdefs[i].blue;

      ErrorF ("winStoreColors () - i %d red %d green %d blue %d pixel %d\n",
	      i,
	      lpPalEntries[i].peRed,
	      lpPalEntries[i].peGreen,
	      lpPalEntries[i].peBlue,
	      pdefs[i].pixel);
    }
  
  /* Put the X colormap entries into the Windows logical palette */
  if (SetPaletteEntries ((HPALETTE)pmap->devPriv,
			 pdefs[0].pixel,
			 ndef,
			 lpPalEntries) == 0)
    {
      ErrorF ("winStoreColors () - SetPaletteEntries () failed\n");
      free (lpPalEntries);
      return;
    }

  /* Tell Windows that the palette has changed */
  RealizePalette (pScreenPriv->hdcScreen);

  /* Free the palette entries */
  if (lpPalEntries != NULL)
    free (lpPalEntries);

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

#if WIN_PSEUDO_SUPPORT
  /*
   * FIXME: Evil, we are not supposed to truncate the color value.
   */
  *pred >>= 8;
  *pgreen >>= 8;
  *pblue >>= 8;
#else /* WIN_PSEUDO_SUPPORT */
  miResolveColor (pred, pgreen, pblue, pVisual);
#endif
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

  ErrorF ("\nwinCreateColormap ()\n");

  /* Make sure that the X colormap doesn't have an invalid devPriv */
  pmap->devPriv = NULL;
  
  /* Get a pointer to the visual that the colormap belongs to */
  pVisual = pmap->pVisual;
  
  /* Set the top-end color limit */
  dwColorLimit = (1 << pVisual->bitsPerRGBValue) - 1;
  
  /* How many bits do we need to shift color values by? */
  dwShift = 16 - pVisual->bitsPerRGBValue;

  /* Get the maximum number of palette entries for this visual */
  dwEntriesMax = pVisual->ColormapEntries;
  
  ErrorF ("winCreateColormap () - dwEntriesMax: %d\n",
	  dwEntriesMax);

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
  
  /* Set the palette version */
  lpPaletteNew->palVersion = 0x0300;

  /* Set the number of entries in the logical palette */
  lpPaletteNew->palNumEntries = dwEntriesMax;
  
  /* Tell Windows to create the palette */
  hpalNew = CreatePalette (lpPaletteNew);
  if (hpalNew == NULL)
    {
      ErrorF ("winCreateColormap () - CreatePalette () failed\n");
      free (lpPaletteNew);
      return FALSE;
    }

  /* Save the Windows logical palette handle in the X colormaps' devPriv */
  pmap->devPriv = hpalNew;

  /* Free the palette memory */
  free (lpPaletteNew);

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

  ErrorF ("winDestroyColormap ()\n");

  /* Do we have a palette to destroy? */
  if (pmap->devPriv != NULL)
    {
      /* Is colormap to be destroyed the default? */
      if (pmap->flags & IsDefault)
	{
	  ErrorF ("winDestroyColormap () - Destroying default colormap\n");
	  
	  /*
	   * FIXME: Walk the list of all screens, popping the default
	   * palette out of each screen device context.
	   */

	  /* Pop the palette out of the device context */
	  SelectPalette (pScreenPriv->hdcScreen,
			 GetStockObject (DEFAULT_PALETTE),
			 FALSE);
	}

      /* Try to delete the logical palette */
      if (DeleteObject ((HPALETTE)pmap->devPriv) == 0)
	{
	  ErrorF ("winDestroyColormap () - DeleteObject () failed\n");
	}
    }
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
  RGBQUAD		rgbColors[NUM_PALETTE_ENTRIES];
      
  /* Get the color table for the DIB */
#if !WIN_PSEUDO_SUPPORT
  /* Gets all 256 colors */
  uiColorsRetrieved = GetDIBColorTable (pScreenPriv->hdcScreen,
					0, NUM_PALETTE_ENTRIES,
					rgbColors);

  if (SetDIBColorTable (pScreenPriv->hdcShadow,
			0, uiColorsRetrieved,
			rgbColors) == 0)
    {
      ErrorF ("winGetPaletteDIB () - SetDIBColorTable () failed\n");
    }
#else
  /* Only gets 16 colors */
  uiColorsRetrieved = GetDIBColorTable (pScreenPriv->hdcShadow,
					0, NUM_PALETTE_ENTRIES,
					rgbColors);
#endif
  if (uiColorsRetrieved == 0)
    {
      ErrorF ("winGetPaletteDIB () - Could not retrieve DIB color table\n");
      return FALSE;
    }
#if CYGDEBUG
  ErrorF ("winGetPaletteDIB () - Retrieved %d colors from DIB\n",
	  uiColorsRetrieved);
#endif

  /* Alloc each color in the DIB color table */
  for (i = 0; i < uiColorsRetrieved; ++i)
    {
      pixel = i;

      /* Extract the color values for current palette entry */
#if !WIN_PSEUDO_SUPPORT
      nRed = rgbColors[i].rgbRed << 8;
      nGreen = rgbColors[i].rgbGreen << 8;
      nBlue = rgbColors[i].rgbBlue << 8;
#else
      nRed = rgbColors[i].rgbRed;
      nGreen = rgbColors[i].rgbGreen;
      nBlue = rgbColors[i].rgbBlue;
#endif

#if CYGDEBUG
      ErrorF ("winGetPaletteDIB () - Allocating a color: %d; "\
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
	  ErrorF ("winGetPaletteDIB () - AllocColor () failed, pixel %d\n",
		  i);
	  return FALSE;
	}

#if !WIN_PSEUDO_SUPPORT
      pcmap->red[i].co.local.red = nRed;
      pcmap->red[i].co.local.green = nGreen;
      pcmap->red[i].co.local.blue = nBlue;
#endif
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
  VisualPtr		pVisual;
  ColormapPtr		pcmap = NULL;


  ErrorF ("winCreateDefColormap ()\n");


  /* Use standard fb colormaps for non palettized color modes */
  if (pScreenInfo->dwDepth > 8)
    {
      ErrorF ("winCreateDefColormap () - Deferring to " \
	      "fbCreateDefColormap ()\n");
      return fbCreateDefColormap (pScreen);
    }

  /* Find the root visual */
#if CYGDEBUG
  ErrorF ("winCreateDefColormap () - Finding the root visual\n");
#endif
  for (pVisual = pScreen->visuals;
       pVisual->vid != pScreen->rootVisual;
       pVisual++);
  ErrorF ("winCreateDefColormap () - Found the root visual %08x, %d\n",
	  pVisual, pVisual->vid);

  /*
   *  AllocNone for non-Dynamic visual classes,
   *  AllocAll for Dynamic visual classes.
   */

  /*
   * Dynamic visual classes allow the colors of the color map
   * to be changed by clients.
   */

  ErrorF ("winCreateDefColormap () - defColormap: %d\n",
	  pScreen->defColormap);

  /* Allocate an X colormap, owned by client 0 */
  if (CreateColormap (pScreen->defColormap, 
		      pScreen, pVisual, &pcmap,
		      AllocNone, 0) != Success)
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
	  ErrorF ("winCreateDefColormap () - Couldn't get colors for DD\n");
	  return FALSE;
	}
    }

  /* Install the created colormap */
  (*pScreen->InstallColormap)(pcmap);

  ErrorF ("winCreateDefColormap () - Returning\n");

  return TRUE;
}

void
winClearVisualTypes (void)
{
#if CYGDEBUG
  ErrorF ("winClearVisualTypes ()\n");
#endif
  miClearVisualTypes ();
}

Bool
winSetVisualTypesNativeGDI (int nDepth, int nVisuals, int nBitsPerRGB)
{
#if CYGDEBUG
  ErrorF ("winSetVisualTypes ()\n");
#endif
  return miSetVisualTypes (nDepth, nVisuals, nBitsPerRGB, -1);
}

Bool
winInitVisualsNativeGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  HBITMAP		hbmp;
  BITMAPINFO		*pbmi = xalloc (sizeof (BITMAPINFOHEADER)
					+ 256 * sizeof (RGBQUAD));
  BITMAPV4HEADER	*pbmih = (BITMAPV4HEADER*) pbmi;
  HDC			hdc = GetDC (NULL);

  /* Exit if we could not allocate a bitmapinfo structure */
  if (pbmi == NULL)
    {
      ErrorF ("winInitVisuals () - Could not allocate a "\
	      "bitmapinfo structure\n");
      return FALSE;
    }

  /* Create a bitmap compatible with the primary display */
  hbmp = CreateCompatibleBitmap (hdc, 1, 1);

  ZeroMemory (pbmi, sizeof (BITMAPINFOHEADER) + 256 * sizeof (RGBQUAD));
  pbmi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
  
  /* Call GetDIBits for the first time; doesn't do much */
  /*
   * NOTE: This doesn't actually return the bits, because our
   * data pointer is NULL; therefore, we don't have to free
   * memory later.
   */
  GetDIBits (hdc, hbmp,
	     0, 0,
	     NULL,
	     pbmi,
	     0);

#if CYGDEBUG
  ErrorF ("winInitVisuals () - 1st comp %08x rm %08x gm %08x bm %08x\n",
	  pbmi->bmiHeader.biCompression,
	  pbmih->bV4RedMask,
	  pbmih->bV4GreenMask,
	  pbmih->bV4BlueMask);
#endif

  /* Call GetDIBits again if the masks were zero and the color depth > 8 bpp */
  if ((pScreenInfo->dwDepth > 8)
      && (pbmih->bV4RedMask == 0x0
	  || pbmih->bV4GreenMask == 0x0
	  || pbmih->bV4BlueMask == 0x0))
    {
      GetDIBits (hdc, hbmp,
		 0, 0,
		 NULL,
		 pbmi,
		 0);
    }

#if CYGDEBUG
  ErrorF ("winInitVisuals () - 2nd comp %08x rm %08x gm %08x bm %08x\n",
	  pbmi->bmiHeader.biCompression,
	  pbmih->bV4RedMask,
	  pbmih->bV4GreenMask,
	  pbmih->bV4BlueMask);
#endif

  /* Set default masks if masks could not be detected */
  switch (pScreenInfo->dwDepth)
    {
    case 32:
    case 24:
      if (pbmih->bV4RedMask != 0x00FF0000)
	{
	  pbmih->bV4RedMask = 0x00FF0000;
	}
      if (pbmih->bV4GreenMask != 0x0000FF00)
	{
	  pbmih->bV4GreenMask = 0x0000FF00;
	}
      if (pbmih->bV4BlueMask != 0x000000FF)
	{
	  pbmih->bV4BlueMask = 0x000000FF;
	}
      break;

    case 16:
      if (pbmih->bV4RedMask != 0x0000F800 && pbmih->bV4RedMask != 0x00007C00)
	{
	  pbmih->bV4RedMask = 0x0000F800;
	}
      if (pbmih->bV4GreenMask != 0x00007E0
	  && pbmih->bV4GreenMask != 0x000003E0)
	{
	  pbmih->bV4GreenMask = 0x000007E0;
	}
      if (pbmih->bV4BlueMask != 0x0000001F)
	{	
	  pbmih->bV4BlueMask = 0x0000001F;
	}
      break;
    }

#if CYGDEBUG
  ErrorF ("winInitVisuals () - 3rd comp %08x rm %08x gm %08x bm %08x\n",
	  pbmi->bmiHeader.biCompression,
	  pbmih->bV4RedMask,
	  pbmih->bV4GreenMask,
	  pbmih->bV4BlueMask);
#endif

  /* Copy the bitmasks into the screen privates, for later use */
  pScreenPriv->dwRedMask = pbmih->bV4RedMask;
  pScreenPriv->dwGreenMask = pbmih->bV4GreenMask;
  pScreenPriv->dwBlueMask = pbmih->bV4BlueMask;

  /* Release the DC and the bitmap that were used for querying */
  ReleaseDC (NULL, hdc);
  DeleteObject (hbmp);

  /* Set the significant bits per red, green, and blue */
  switch (pScreenInfo->dwDepth)
    {
    case 32:
    case 24:
      pScreenPriv->dwBitsPerRGB = 8;
      break;
      
    case 16:
      if (pScreenPriv->dwRedMask == 0xF800)
	{
	  pScreenPriv->dwBitsPerRGB = 6;
	}
      else
	{
	  pScreenPriv->dwBitsPerRGB = 5;
	}
      break;
      
    case 15:
      pScreenPriv->dwBitsPerRGB = 5;
      break;
      
    case 8:
      pScreenPriv->dwBitsPerRGB = 8;
      pScreenPriv->dwRedMask = 0;
      pScreenPriv->dwGreenMask = 0;
      pScreenPriv->dwBlueMask = 0;
      break;

    default:
      pScreenPriv->dwBitsPerRGB = 0;
      break;
    }

  /* Tell the user how many bits per RGB we are using */
  ErrorF ("winInitVisuals () - Using dwBitsPerRGB: %d\n",
	  pScreenPriv->dwBitsPerRGB);

  /* Create a single visual according to the Windows screen depth */
  switch (pScreenInfo->dwDepth)
    {
    case 32:
    case 24:
    case 16:
    case 15:
      if (!miSetVisualTypesAndMasks (pScreenInfo->dwDepth,
				     TrueColorMask,
				     pScreenPriv->dwBitsPerRGB,
				     TrueColor,
				     pScreenPriv->dwRedMask,
				     pScreenPriv->dwGreenMask,
				     pScreenPriv->dwBlueMask))
	{
	  ErrorF ("winInitVisuals () - miSetVisualTypesAndMasks failed\n");
	  return FALSE;
	}
      break;

    case 8:
      ErrorF ("winInitVisuals () - Calling miSetVisualTypesAndMasks\n");
      if (!miSetVisualTypesAndMasks (pScreenInfo->dwDepth,
				     StaticColorMask,
				     pScreenPriv->dwBitsPerRGB,
				     StaticColor,
				     pScreenPriv->dwRedMask,
				     pScreenPriv->dwGreenMask,
				     pScreenPriv->dwBlueMask))
	{
	  ErrorF ("winInitVisuals () - miSetVisualTypesAndMasks failed\n");
	  return FALSE;
	}
#if CYGDEBUG
      ErrorF ("winInitVisuals () - Returned from miSetVisualTypesAndMasks\n");
#endif
      break;

    default:
      break;
    }

  /* Free memory */
  free (pbmi);

#if CYGDEBUG
  ErrorF ("winInitVisuals () - Returning\n");
#endif

  return TRUE;
}
