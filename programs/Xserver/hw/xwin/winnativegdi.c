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
 * Authors:	Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winnativegdi.c,v 1.1 2001/07/02 09:37:17 alanh Exp $ */

#include "win.h"

/*
 * We wrap whatever CloseScreen procedure was specified by fb;
 * a pointer to said procedure is stored in our privates.
 */
Bool
winCloseScreenNativeGDI (int nIndex, ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
#if 0
  Bool			fReturn;
#endif

  ErrorF ("winCloseScreenNativeGDI () - Freeing screen resources\n");

  /* Flag that the screen is closed */
  pScreenPriv->fClosed = TRUE;
  pScreenPriv->fActive = FALSE;

#if 0
  /* Call the wrapped CloseScreen procedure */
  pScreen->CloseScreen = pScreenPriv->CloseScreen;
  
  ErrorF ("winCloseScreenNativeGDI () - Calling miCloseScreen ()\n");

  fReturn = (*pScreen->CloseScreen) (nIndex, pScreen);

  ErrorF ("winCloseScreenNativeGDI () - miCloseScreen () returned\n");
#endif

  /* Delete the window property */
  RemoveProp (pScreenPriv->hwndScreen, WIN_SCR_PROP);

  /* Redisplay the Windows cursor */
  if (!pScreenPriv->fCursor)
    ShowCursor (TRUE);

  /* Kill our window */
  if (pScreenPriv->hwndScreen)
    {
      DestroyWindow (pScreenPriv->hwndScreen);
      pScreenPriv->hwndScreen = NULL;
    }

  /* Delete our garbage bitmap */
  if (g_hbmpGarbage != NULL)
    {
      DeleteObject (g_hbmpGarbage);
      g_hbmpGarbage = NULL;
    }

  /* Invalidate our screeninfo's pointer to the screen */
  pScreenInfo->pScreen = NULL;

  /* Free the screen privates for this screen */
  xfree (pScreenPriv);

  ErrorF ("winCloseScreenNativeGDI () - Returning\n");

  return TRUE;
}

Bool
winInitVisualsNativeGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

  /* Set the bitsPerRGB and bit masks */
  switch (pScreenInfo->dwDepth)
    {
    case 32:
    case 24:
      pScreenPriv->dwBitsPerRGB = 8;
      pScreenPriv->dwRedMask = 0x00FF0000;
      pScreenPriv->dwGreenMask = 0x0000FF00;
      pScreenPriv->dwBlueMask = 0x000000FF;
      break;
      
    case 16:
      pScreenPriv->dwBitsPerRGB = 6;
      pScreenPriv->dwRedMask = 0xF800;
      pScreenPriv->dwGreenMask = 0x07E0;
      pScreenPriv->dwBlueMask = 0x001F;
      break;
      
    case 15:
      pScreenPriv->dwBitsPerRGB = 5;
      pScreenPriv->dwRedMask = 0x7C00;
      pScreenPriv->dwGreenMask = 0x07E0;
      pScreenPriv->dwBlueMask = 0x001F;
      break;
      
    case 8:
      pScreenPriv->dwBitsPerRGB = 8;
      pScreenPriv->dwRedMask = 0;
      pScreenPriv->dwGreenMask = 0;
      pScreenPriv->dwBlueMask = 0;
      break;

    default:
      ErrorF ("winInitVisualsNativeGDI () - Unknown screen depth\n");
      return FALSE;
      break;
    }

  /* Tell the user how many bits per RGB we are using */
  ErrorF ("winInitVisualsNativeGDI () - Using dwBitsPerRGB: %d\n",
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
      break;

    default:
      ErrorF ("winInitVisualsNativeGDI () - Unknown screen depth\n");
      return FALSE;
    }

#if 1
  ErrorF ("winInitVisualsNativeGDI () - Returning\n");
#endif

  return TRUE;
}

/* Adjust the video mode */
Bool
winAdjustVideoModeNativeGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  HDC			hdc = NULL;
  HDC			hdcMem = NULL;
  HBITMAP		hbmp = NULL;
  DWORD			dwDepth;
  
  hdc = GetDC (NULL);

  /* We're in serious trouble if we can't get a DC */
  if (hdc == NULL)
    {
      ErrorF ("winAdjustVideoModeNativeGDI () - GetDC () failed\n");
      return FALSE;
    }

  /* Set the garbage bitmap handle */
  if (g_hbmpGarbage == NULL)
    {
      g_hbmpGarbage = CreateCompatibleBitmap (hdc, 0, 0);
    }

  /* Query GDI for current display depth */
  dwDepth = GetDeviceCaps (hdc, BITSPIXEL);

  /* GDI cannot change the screen depth */
  if (pScreenInfo->dwDepth == WIN_DEFAULT_DEPTH)
    {
      /* No -depth parameter passed, let the user know the depth being used */
      ErrorF ("winAdjustVideoModeNativeGDI () - Using Windows display "
	      "depth of %d bits per pixel\n", dwDepth);

      /* Use GDI's depth */
      pScreenInfo->dwDepth = dwDepth;
    }
  else if (dwDepth != pScreenInfo->dwDepth)
    {
      /* Warn user if GDI depth is different than -depth parameter */
      ErrorF ("winAdjustVideoModeNativeGDI () - Command line depth: %d, "\
	      "using depth: %d\n", pScreenInfo->dwDepth, dwDepth);

      /* We'll use GDI's depth */
      pScreenInfo->dwDepth = dwDepth;
    }
  
  /* Release our DC */
  ReleaseDC (NULL, hdc);

  return TRUE;
}

Bool
winActivateAppNativeGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

  /*
   * Are we active?
   * Are we fullscreen?
   */
  if (pScreenPriv != NULL
      && pScreenPriv->fActive
      && pScreenInfo->fFullScreen)
    {
      /*
       * Activating, attempt to bring our window 
       * to the top of the display
       */
      ShowWindow (pScreenPriv->hwndScreen, SW_RESTORE);
    }

  /*
   * Are we inactive?
   * Are we fullscreen?
   */
  if (pScreenPriv != NULL
      && !pScreenPriv->fActive
      && pScreenInfo->fFullScreen)
    {
      /*
       * Deactivating, stuff our window onto the
       * task bar.
       */
      ShowWindow (pScreenPriv->hwndScreen, SW_MINIMIZE);
    }

  return TRUE;
}

HBITMAP
winCreateDIBNativeGDI (int iWidth, int iHeight, int iDepth,
		       void **ppvBits)
{
  BITMAPINFOHEADER	*pbmih = NULL;
  HDC			hdcMem = NULL;
  HBITMAP		hBitmap = NULL;

  /* Don't create an invalid bitmap */
  if (iWidth == 0
      || iHeight == 0
      || iDepth == 0)
    {
      ErrorF ("\nwinCreateDIBNativeGDI () - Invalid specs w %d h %d d %d\n\n",
	      iWidth, iHeight, iDepth);
      return NULL;
    }

  /* Create a scratch DC */
  hdcMem = CreateCompatibleDC (NULL);
  if (hdcMem == NULL)
    {
      ErrorF ("winCreateDIBNativeGDI () - CreateCompatibleDC () failed\n");
      return NULL;
    }

  /* Allocate bitmap info header */
  pbmih = (BITMAPINFOHEADER*) xalloc (sizeof (BITMAPINFOHEADER)
				      + 256 * sizeof (RGBQUAD));
  if (pbmih == NULL)
    {
      ErrorF ("winCreateDIBNativeGDI () - xalloc () failed\n");
      return FALSE;
    }
  ZeroMemory (pbmih, sizeof(BITMAPINFOHEADER) + 256 * sizeof (RGBQUAD));

  /* Describe bitmap to be created */
  pbmih->biSize = sizeof (BITMAPINFOHEADER);
  pbmih->biWidth = iWidth;
  pbmih->biHeight = -iHeight;
  pbmih->biPlanes = 1;
  pbmih->biBitCount = iDepth;
  pbmih->biCompression = BI_RGB;
  pbmih->biSizeImage = 0;
  pbmih->biXPelsPerMeter = 0;
  pbmih->biYPelsPerMeter = 0;
  pbmih->biClrUsed = 0;
  pbmih->biClrImportant = 0;

  /* Create a DIB with a bit pointer */
  hBitmap = CreateDIBSection (hdcMem,
			      (BITMAPINFO *) pbmih,
			      DIB_RGB_COLORS,
			      ppvBits,
			      NULL,
			      0);
  if (hBitmap == NULL)
    {
      ErrorF ("winCreateDIBNativeGDI () - CreateDIBSection () failed\n");
      return NULL;
    }

  /* Free the bitmap info header memory */
  xfree (pbmih);
  pbmih = NULL;

#if CYGDEBUG
  ErrorF ("winCreateDIBNativeGDI () - CreateDIBSection () returned\n");
#endif

  /* Free the scratch DC */
  DeleteDC (hdcMem);
  hdcMem = NULL;
  
  return hBitmap;
}

/* Set engine specific funtions */
Bool
winSetEngineFunctionsNativeGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  
  /* Set our pointers */
  pScreenPriv->pwinAllocateFB
    = (winAllocateFBProcPtr) (void (*)())NoopDDA;
  pScreenPriv->pwinShadowUpdate
    = (winShadowUpdateProcPtr) (void (*)())NoopDDA;
  pScreenPriv->pwinCloseScreen = winCloseScreenNativeGDI;
  pScreenPriv->pwinInitVisuals = winInitVisualsNativeGDI;
  pScreenPriv->pwinAdjustVideoMode = winAdjustVideoModeNativeGDI;
  if (pScreenInfo->fFullScreen)
    pScreenPriv->pwinCreateBoundingWindow = winCreateBoundingWindowFullScreen;
  else
    pScreenPriv->pwinCreateBoundingWindow = winCreateBoundingWindowWindowed;
  pScreenPriv->pwinFinishScreenInit = winFinishScreenInitNativeGDI;
  pScreenPriv->pwinBltExposedRegions
    = (winBltExposedRegionsProcPtr) (void (*)())NoopDDA;
  pScreenPriv->pwinActivateApp = winActivateAppNativeGDI;

  return TRUE;
}
