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
/* $XFree86: xc/programs/Xserver/hw/xwin/winfillsp.c,v 1.3 2001/07/02 09:37:17 alanh Exp $ */

#include "win.h"

#if 0
void
winFillSpansNativeGDI (DrawablePtr	pDrawable,
		       GCPtr		pGC,
		       int		nSpans,
		       DDXPointPtr	pPoints,
		       int		*pWidths,
		       int		fSorted)
{
  winGCPriv(pGC);
  winPrivPixmapPtr	pStipplePriv = NULL;
  int			iIdx, iX;
  DDXPointPtr		pPoint = NULL;
  int			*pnWidth = 0;
  PixmapPtr		ppixmapStipple = NULL;

  /* Branch on the fill type */
  switch (pGC->fillStyle)
    {
    case FillSolid:
      /* Enumerate spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  /* Get pointers to the current span location and width */
	  pPoint = pPoints + iIdx;
	  pnWidth = pWidths + iIdx;
	  
	  /* Draw the requested line */
	  MoveToEx (pGCPriv->hdcMem, pPoint->x, pPoint->y, NULL);
	  LineTo (pGCPriv->hdcMem, pPoint->x + *pnWidth, pPoint->y);

	  ErrorF ("(%dx%dx%d) from: (%d,%d) to: (%d,%d), color: %08x\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, pPoint->x + *pnWidth, pPoint->y,
		  pGC->fgPixel);
	}
      break;

    case FillStippled:
      ppixmapStipple = pGC->stipple;
      pStipplePriv = winGetPixmapPriv (ppixmapStipple);

      /* Create a memory DC to hold the stipple */
      hdcStipple = CreateCompatibleDC (hdc);
      if (hdcStipple == NULL)
	FatalError ("winFillSpans - CreateCompatibleDC failed - L127\n");

#if 1
      /* Allocate bitmap info header */
      pbmih = (BITMAPINFOHEADER*) xalloc (sizeof (BITMAPINFOHEADER)
					  + 256 * sizeof (RGBQUAD));
      if (pbmih == NULL)
	{
	  FatalError ("winFillSpans - xalloc of BITMAPINFOHEADER failed\n");
	}
      ZeroMemory (pbmih, sizeof(BITMAPINFOHEADER) + 256 * sizeof (RGBQUAD));
      
      /* Describe bitmap to be created */
      pbmih->biSize = sizeof (BITMAPINFOHEADER);
      pbmih->biWidth = pDrawable->width;
      pbmih->biHeight = -pDrawable->height;
      pbmih->biPlanes = 1;
      pbmih->biBitCount = pDrawable->depth;
      pbmih->biCompression = BI_RGB;
      pbmih->biSizeImage = 0;
      pbmih->biXPelsPerMeter = 0;
      pbmih->biYPelsPerMeter = 0;
      pbmih->biClrUsed = 0;
      pbmih->biClrImportant = 0;

      /* Create a DI Bitmap */
      hbmpFilledStipple = CreateDIBitmap (hdc,
					  pbmih,
					  0,
					  NULL,
					  NULL,
					  DIB_RGB_COLORS);
#else
      /* Create a destination sized compatible bitmap */
      hbmpFilledStipple = CreateCompatibleBitmap (hdc,
						  pDrawable->width,
						  pDrawable->height);
#endif
      if (hbmpFilledStipple == NULL)
	FatalError ("winFillSpans - CreateCompatibleBitmap failed - L166\n");
      
      /* Select the stipple bitmap into the stipple DC */
      if (SelectObject (hdcStipple, hbmpFilledStipple) == NULL)
	FatalError ("winFillSpans - SelectOjbect failed - L161\n");
      
      /* Set foreground and background to white and black */
      SetTextColor (hdcStipple, RGB(0xFF, 0xFF, 0xFF));
      SetBkColor (hdcStipple, RGB (0x00, 0x00, 0x00));
      
      /* Create a pattern brush from the original stipple */
      hbrushStipple = CreatePatternBrush (winGetPixmapPriv(pGC->stipple)->hBitmap);
      if (hbrushStipple == NULL)
	FatalError ("winFillSpans - CreatePatternBrush failed - L170\n");
      
      /* Select the original stipple brush into the stipple DC */
      if (SelectObject (hdcStipple, hbrushStipple) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L174\n");

      /* PatBlt the original stipple to the filled stipple */
      if (PatBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  PATCOPY) == 0)
	FatalError ("winFillSpans - PatBlt failed - L181\n");
	  
#if 1
      /*
       * REMOVE: This is just a visual verification
       */
      BitBlt (hdc,
	      pDrawable->width, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple,
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Filled a drawable-sized stipple");
#endif

      /*
       * Mask out the bits from the drawable that are being preserved;
       * hbmpFilledStipple now contains the preserved original bits.
       */
      if (BitBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  hdcMem,
		  0, 0,
		  SRCERASE) == 0)
	FatalError ("winFillSpans - BitBlt failed - L206\n");
      
#if 1
      /*
       * REMOVE: This is just a visual verification
       */
      BitBlt (hdc,
	      pDrawable->width * 2, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple,
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Preserved original bits");
#endif

      /*
       * Create a destination sized compatible bitmap to hold
       * the masked foreground color.
       */
      hbmpMaskedForeground = CreateCompatibleBitmap (hdc,
						     pDrawable->width,
						     pDrawable->height);
      if (hbmpMaskedForeground == NULL)
	FatalError ("winFillSpans - CreateCompatibleBitmap failed - L229\n");

      /*
       * Select the masked foreground bitmap into the default memory DC;
       * this should pop the drawable bitmap out of the default DC.
       */
      if (SelectObject (hdcMem, hbmpMaskedForeground) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L245\n");

      /* Free the original bitmap associated with the drawable pixmap */
      if (DeleteObject (pPixmapPriv->hBitmap) == 0)
	FatalError ("winFillSpans - DeleteObject failed - L249\n");

      /* Save a pointer to the new bitmap assocated with the pixmap */
      pPixmapPriv->hBitmap = NULL;
  
      /* Select the stipple brush into the default memory DC */
      if (SelectObject (hdcMem, hbrushStipple) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L256\n");

      /* Create the masked foreground bitmap using the original stipple */
      if (PatBlt (hdcMem, 
		  0, 0, 
		  pDrawable->width, pDrawable->height, 
		  PATCOPY) == 0)
	FatalError ("winFillSpans - PatBlt failed - L263\n");

#if 1
      /*
       * REMOVE: This is just a visual verification
       */
      BitBlt (hdc,
	      pDrawable->width * 3, 0,
	      pDrawable->width, pDrawable->height,
	      hdcMem, 
	      0, 0, 
	      SRCCOPY);
      DEBUG_MSG("Masked foreground bitmap");
#endif
      
      /*
       * Combine the masked foreground with the masked drawable;
       * hbmpFilledStipple will contain the drawable stipple filled
       * with the current foreground color.
       */
      if (BitBlt (hdcStipple, 
		  0, 0, 
		  pDrawable->width, pDrawable->height,
		  hdcMem, 
		  0, 0, 
		  SRCPAINT) == 0)
	FatalError ("winFillSpans - BitBlt failed - L279\n");

#if 1
      /*
       * REMOVE: This is just a visual verification
       */
      BitBlt (hdc, 
	      pDrawable->width * 4, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple, 
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Completed stipple");
#endif

      /* Release the stipple DC */
      DeleteDC (hdcStipple);
      hdcStipple = NULL;
      
      /* Pop the stipple pattern brush out of the default mem DC */
      if (SelectObject (hdcMem, GetStockObject (WHITE_BRUSH)) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L300\n");

      /* Destroy the original stipple pattern brush */
      if (DeleteObject (hbrushStipple) == 0)
	FatalError ("winFillSpans - DeleteObject failed - L303\n");

      /* Select the result into the GC memory DC */
      if (SelectObject (hdcMem, hbmpFilledStipple) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L308\n");
      pPixmapPriv->hBitmap = hbmpFilledStipple;

      /* Free the masked foreground bitmap */
      if (DeleteObject (hbmpMaskedForeground) == 0)
	FatalError ("winFillSpans - DeleteObject failed - L313\n");
      break;

    case FillOpaqueStippled:
      FatalError ("winFillSpans () - FillOpaqueStippled\n\n");
      break;

    case FillTiled:
      /* Enumerate spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  /* Get pointers to the current span location and width */
	  pPoint = pPoints + iIdx;
	  pnWidth = pWidths + iIdx;
	  
	  for (iX = 0; iX < *pnWidth; iX += pGC->tile.pixmap->drawable.width)
	    {
	      /*
	       * Blit the tile to the screen, one line at a time.
	       */
	      BitBlt (pGCPriv->hdc,
		      pPoint->x + iX, pPoint->y,
		      pGC->tile.pixmap->drawable.width, 1,
		      pGCPriv->hdcMem,
		      0, pPoint->y % pGC->tile.pixmap->drawable.height,
		      SRCCOPY);
	    }
	}
      break;

    default:
      break;
    }
}

#else

/* See Porting Layer Definition - p. 54 */
void
winFillSpansNativeGDI (DrawablePtr	pDrawable,
		       GCPtr		pGC,
		       int		nSpans,
		       DDXPointPtr	pPoints,
		       int		*pWidths,
		       int		fSorted)
{
#if WIN_NATIVE_GDI_SUPPORT
  winGCPriv(pGC);
  winPrivPixmapPtr	pPixmapPriv = NULL;
  int			iIdx = 0, iX;
  DDXPointPtr		pPoint = NULL;
  int			*pnWidth = 0;
  PixmapPtr		pPixmap = NULL;
  HDC			hdcStipple = NULL;
  HBITMAP		hbmpFilledStipple = NULL;
  HBITMAP		hbmpMaskedForeground = NULL;
  HBRUSH		hbrushStipple = NULL;
  BITMAPINFOHEADER	*pbmih = NULL;
  DEBUG_FN_NAME("winFillSpans");
  DEBUGVARS;
  DEBUGPROC_MSG;
  
  ErrorF ("winFillSpans () - pDrawable: %08x\n",
	  pDrawable);

  /* Branch on the fill type */
  switch (pGC->fillStyle)
    {
    case FillSolid:
#if 1
      ErrorF ("winFillSpans - FillSolid w %d h %d hdc %08x hdcMem %08x\n",
	      pDrawable->width, pDrawable->height,
	      pGCPriv->hdc, pGCPriv->hdcMem);
      if (BitBlt (pGCPriv->hdc, 
		  pDrawable->width, pDrawable->height,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0, 
		  SRCCOPY) == 0)
	FatalError ("winFillSpans - FillSolid original blt failed\n");
      DEBUG_MSG("Solid fill - original drawable");
#endif

      /* Enumerate spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  /* Get pointers to the current span location and width */
	  pPoint = pPoints + iIdx;
	  pnWidth = pWidths + iIdx;
	  
	  /* Draw the requested line */
	  if (MoveToEx (pGCPriv->hdcMem, pPoint->x, pPoint->y, NULL) == 0)
	    FatalError ("winValidateGC - FillSolid MoveToEx failed\n");
	  if (LineTo (pGCPriv->hdcMem, pPoint->x + *pnWidth, pPoint->y) == 0)
	    FatalError ("winValidateGC - FillSolid LineTo failed\n");

	  ErrorF ("(%dx%dx%d) from: (%d,%d) to: (%d,%d), color: %08x\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, pPoint->x + *pnWidth, pPoint->y,
		  pGC->fgPixel);
	}

#if 0
      if (BitBlt (pGCPriv->hdc,
		  pDrawable->width, pDrawable->height,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0,
		  SRCCOPY) == 0)
	DEBUG_MSG("Solid Fill - Blt of fill result failed");
      DEBUG_MSG("Solid Fill - Filled");
#endif
      break;

    case FillStippled:
      /* I can handle drawable pixmaps, no problem */
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);

      /* Create a memory DC to hold the stipple */
      hdcStipple = CreateCompatibleDC (pGCPriv->hdc);
      if (hdcStipple == NULL)
	FatalError ("winFillSpans - CreateCompatibleDC failed - L127\n");

      /*
       * REMOVE: Visual verification
       */
      SelectObject (hdcStipple, winGetPixmapPriv(pGC->stipple)->hBitmap);
      BitBlt (pGCPriv->hdc, 
	      0, 0,
	      pGC->stipple->drawable.width, pGC->stipple->drawable.height,
	      hdcStipple,
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Blitted original stipple to screen");

      /* Create a destination sized compatible bitmap */
      hbmpFilledStipple = CreateCompatibleBitmap (pGCPriv->hdc,
						  pDrawable->width,
						  pDrawable->height);
      if (hbmpFilledStipple == NULL)
	FatalError ("winFillSpans - CreateCompatibleBitmap failed - L166\n");
      
      /* Select the stipple bitmap into the stipple DC */
      if (SelectObject (hdcStipple, hbmpFilledStipple) == NULL)
	FatalError ("winFillSpans - SelectOjbect failed - L161\n");
      
      /* Set foreground and background of stipple DC to white and black */
      SetTextColor (hdcStipple, RGB(0xFF, 0xFF, 0xFF));
      SetBkColor (hdcStipple, RGB (0x00, 0x00, 0x00));
      
      /* Create a pattern brush from the original stipple */
      hbrushStipple = CreatePatternBrush (winGetPixmapPriv(pGC->stipple)->hBitmap);
      if (hbrushStipple == NULL)
	FatalError ("winFillSpans - CreatePatternBrush failed - L170\n");
      
      /* Select the original stipple brush into the stipple DC */
      if (SelectObject (hdcStipple, hbrushStipple) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L174\n");

      /* PatBlt the original stipple to the filled stipple */
      if (PatBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  PATCOPY) == 0)
	FatalError ("winFillSpans - PatBlt failed - L181\n");

      /*
       * REMOVE: Visual verification
       */
      BitBlt (pGCPriv->hdc, 
	      pDrawable->width, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple, 
	      0, 0, 
	      SRCCOPY);
      DEBUG_MSG("Filled a drawable-sized stipple");

      /*
       * Mask out the bits from the drawable that are being preserved;
       * hbmpFilledStipple now contains the preserved original bits.
       */
      if (BitBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0,
		  SRCERASE) == 0)
	FatalError ("winFillSpans - BitBlt failed - L206\n");

      /*
       * REMOVE: Visual verification
       */
      BitBlt (pGCPriv->hdc, 
	      pDrawable->width * 2, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple, 
	      0, 0, 
	      SRCCOPY);
      DEBUG_MSG("Preserved original bits");

      /*
       * Create a destination sized compatible bitmap to hold
       * the masked foreground color.
       */
      hbmpMaskedForeground = CreateCompatibleBitmap (pGCPriv->hdc,
						     pDrawable->width,
						     pDrawable->height);
      if (hbmpMaskedForeground == NULL)
	FatalError ("winFillSpans - CreateCompatibleBitmap failed - L229\n");
      
      /*
       * Select the masked foreground bitmap into the default memory DC;
       * this should pop the drawable bitmap out of the default DC.
       */
      if (SelectObject (pGCPriv->hdcMem, hbmpMaskedForeground) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L245\n");

      /* Free the original bitmap associated with the drawable pixmap */
      if (DeleteObject (pPixmapPriv->hBitmap) == 0)
	FatalError ("winFillSpans - DeleteObject failed - L249\n");
      pPixmapPriv->hBitmap = NULL;
  
      /* Select the stipple brush into the default memory DC */
      if (SelectObject (pGCPriv->hdcMem, hbrushStipple) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L256\n");

      /* Create the masked foreground bitmap using the original stipple */
      if (PatBlt (pGCPriv->hdcMem, 
		  0, 0, 
		  pDrawable->width, pDrawable->height, 
		  PATCOPY) == 0)
	FatalError ("winFillSpans - PatBlt failed - L263\n");
      
      /*
       * REMOVE: Visual verification
       */
      BitBlt (pGCPriv->hdc,
	      pDrawable->width * 3, 0,
	      pDrawable->width, pDrawable->height,
	      pGCPriv->hdcMem,
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Masked foreground bitmap");

      /*
       * Combine the masked foreground with the masked drawable;
       * hbmpFilledStipple will contain the drawable stipple filled
       * with the current foreground color.
       */
      if (BitBlt (hdcStipple, 
		  0, 0, 
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem, 
		  0, 0, 
		  SRCPAINT) == 0)
	FatalError ("winFillSpans - BitBlt failed - L279\n");
      
      /*
       * REMOVE: Visual verification
       */
      BitBlt (pGCPriv->hdc,
	      pDrawable->width * 4, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple,
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Completed stipple");

      /* Release the stipple DC */
      DeleteDC (hdcStipple);
      hdcStipple = NULL;
      
      /* Pop the stipple pattern brush out of the default mem DC */
      if (SelectObject (pGCPriv->hdcMem, GetStockObject (WHITE_BRUSH)) 
	  == NULL)
	FatalError ("winFillSpans - SelectObject failed - L300\n");

      /* Destroy the original stipple pattern brush */
      if (DeleteObject (hbrushStipple) == 0)
	FatalError ("winFillSpans - DeleteObject failed - L303\n");

      /* Select the result into the GC memory DC */
      if (SelectObject (pGCPriv->hdcMem, hbmpFilledStipple) == NULL)
	FatalError ("winFillSpans - SelectObject failed - L308\n");
      pPixmapPriv->hBitmap = hbmpFilledStipple;

      /* Free the masked foreground bitmap */
      if (DeleteObject (hbmpMaskedForeground) == 0)
	FatalError ("winFillSpans - DeleteObject failed - L313\n");
      break;

    case FillOpaqueStippled:
      FatalError ("\n\nwinFillSpans () - OpaqueStippled\n\n");
      break;

    case FillTiled:
      /* Enumerate spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  /* Get pointers to the current span location and width */
	  pPoint = pPoints + iIdx;
	  pnWidth = pWidths + iIdx;
	  
	  for (iX = 0; iX < *pnWidth; iX += pGC->tile.pixmap->drawable.width)
	    {
	      /*
	       * Blit the tile to the screen, one line at a time.
	       */
	      BitBlt (pGCPriv->hdc,
		      pPoint->x + iX, pPoint->y,
		      pGC->tile.pixmap->drawable.width, 1,
		      pGCPriv->hdcMem,
		      0, pPoint->y % pGC->tile.pixmap->drawable.height,
		      SRCCOPY);
	    }
	}
      break;

    default:
      FatalError ("winFillSpans () - Unknown fillStyle\n");
      break;
    }
#endif
}
#endif
