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
/* $XFree86: xc/programs/Xserver/hw/xwin/winfillsp.c,v 1.2 2001/06/04 13:04:41 alanh Exp $ */

#include "win.h"

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
  int		iIdx = 0, i, iX;
  DDXPointPtr	pPoint = NULL;
  int		*pnWidth = 0;
  PixmapPtr	pPixmap = NULL;
  HDC		hdcStipple = NULL;
  HDC		hdc = pGCPriv->hdc;
  HDC		hdcMem = pGCPriv->hdcMem;
  HBITMAP	hbmpFilledStipple = NULL;
  HBITMAP	hbmpMaskedForeground = NULL;
  HBITMAP	hbmpDrawable = NULL;
  HBRUSH	hbrushStipple = NULL;
  HBRUSH	hBrush = NULL;
  DEBUG_FN_NAME("winFillSpans");
  DEBUGVARS;
  
  ErrorF ("winFillSpans () - pDrawable: %08x\n",
	  pDrawable);

  /* Branch on the fill type */
  switch (pGC->fillStyle)
    {
    case FillSolid:
      /*
       * REMOVE: This is just a visual verification
       */
      BitBlt (hdc,
	      pDrawable->width, pDrawable->height,
	      pDrawable->width, pDrawable->height,
	      hdcMem,
	      0, 0,
	      SRCCOPY);
      DEBUG_MSG("Solid fill - original drawable");

      /* Enumerate spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  /* Get pointers to the current span location and width */
	  pPoint = pPoints + iIdx;
	  pnWidth = pWidths + iIdx;
	  
	  /* Draw the requested line */
	  MoveToEx (hdcMem, pPoint->x, pPoint->y, NULL);
	  LineTo (hdcMem, pPoint->x + *pnWidth, pPoint->y);

	  ErrorF ("(%dx%dx%d) from: (%d,%d) to: (%d,%d), color: %08x\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, pPoint->x + *pnWidth, pPoint->y,
		  pGC->fgPixel);
	}

      /*
       * REMOVE: This is just a visual verification
       */
      BitBlt (hdc,
	      pDrawable->width * 2, pDrawable->height,
	      pDrawable->width, pDrawable->height,
	      hdcMem,
	      0, 0, SRCCOPY);
      DEBUG_MSG("Solid Fill - Filled");
      break;
    case FillStippled:
      /* TODO: Construct the correct stipple, store it in hdcStipple */
      ErrorF ("winFillSpans () - Stipple bitmap: %08x (%dx%d)\n",
	      winGetPixmapPriv(pGC->stipple)->hBitmap,
	      pGC->stipple->drawable.width,
	      pGC->stipple->drawable.height);

      /* Create a memory DC to hold the stipple */
      hdcStipple = CreateCompatibleDC (hdc);

      /* Create a destination sized compatible bitmap */
      hbmpFilledStipple = CreateCompatibleBitmap (hdc,
						  pDrawable->width,
						  pDrawable->height);
      
      /* Select the stipple bitmap into the stipple DC */
      SelectObject (hdcStipple, hbmpFilledStipple);
      
      /* Set foreground and background to white and black */
      SetTextColor (hdcStipple, RGB(0xFF, 0xFF, 0xFF));
      SetBkColor (hdcStipple, RGB (0x00, 0x00, 0x00));
      
      /* Create a pattern brush from the original stipple */
      hbrushStipple = CreatePatternBrush (winGetPixmapPriv(pGC->stipple)->hBitmap);
      
      /* Select the original stipple brush into the stipple DC */
      SelectObject (hdcStipple, hbrushStipple);

      /* PatBlt the original stipple to the filled stipple */
      PatBlt (hdcStipple,
	      0, 0,
	      pDrawable->width, pDrawable->height,
	      PATCOPY);
      BitBlt (hdc,
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
      BitBlt (hdcStipple,
	      0, 0,
	      pDrawable->width, pDrawable->height,
	      hdcMem,
	      0, 0,
	      SRCERASE);
      BitBlt (hdc,
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
      hbmpMaskedForeground = CreateCompatibleBitmap (hdc,
						     pDrawable->width,
						     pDrawable->height);

      /* TODO: This code chunk can move to winValidateGC () */
      /* Set the foreground color for the stipple fill */
      if (pGC->fgPixel == 0x1)
	{
	  SetTextColor (hdcMem, RGB(0xFF, 0xFF, 0xFF));
	  DEBUG_MSG("Grey fill");
	}
      else if (pGC->fgPixel == 0xFFFF)
	{
	  SetTextColor (hdcMem, RGB(0xFF, 0xFF, 0xFF));
	  DEBUG_MSG("White fill");
	}
      else
	{
	  SetTextColor (hdcMem, RGB(0x00, 0x00, 0x00));
	  DEBUG_MSG("Black fill");
	}
      SetBkColor (hdcMem, RGB(0x00, 0x00, 0x00));

      /*
       * Select the masked foreground bitmap into the default memory DC;
       * this should pop the drawable bitmap out of the default DC.
       */
      hbmpDrawable = SelectObject (hdcMem, hbmpMaskedForeground);

      /* Free the original drawable */
      DeleteObject (hbmpDrawable);
      hbmpDrawable = NULL;
  
      /* Select the stipple brush into the default memory DC */
      SelectObject (hdcMem, hbrushStipple);

      /* Create the masked foreground bitmap using the original stipple */
      PatBlt (hdcMem, 0, 0, pDrawable->width, pDrawable->height, PATCOPY);
      BitBlt (hdc, pDrawable->width * 3, 0,
	      pDrawable->width, pDrawable->height,
	      hdcMem, 0, 0, SRCCOPY);
      DEBUG_MSG("Masked foreground bitmap");
      
      /*
       * Combine the masked foreground with the masked drawable;
       * hbmpFilledStipple will contain the drawable stipple filled
       * with the current foreground color.
       */
      BitBlt (hdcStipple, 0, 0, pDrawable->width, pDrawable->height,
	      hdcMem, 0, 0, SRCPAINT);
      BitBlt (hdc, pDrawable->width * 4, 0,
	      pDrawable->width, pDrawable->height,
	      hdcStipple, 0, 0, SRCCOPY);
      DEBUG_MSG("Completed stipple");

      /* Release the stipple DC */
      DeleteDC (hdcStipple);
      hdcStipple = NULL;
      
      /* Pop the stipple pattern brush out of the default mem DC */
      SelectObject (hdcMem, GetStockObject (WHITE_BRUSH));

      /* Destroy the original stipple pattern brush */
      DeleteObject (hbrushStipple);

      /* Select the result into the default memory DC */
      SelectObject (hdcMem, hbmpFilledStipple);

      /* Free the masked foreground bitmap */
      DeleteObject (hbmpMaskedForeground);
      
      /* Point the drawable to the new bitmap */
      winGetPixmapPriv((PixmapPtr)pDrawable)->hBitmap = hbmpFilledStipple;
      break;
    case FillOpaqueStippled:
      ErrorF ("\n\nwinFillSpans () - OpaqueStippled\n\n");
      break;
    case FillTiled:
      /*
       * Assumes that the drawable is the screen and the tile has been
       * selected into the default memory DC.
       */
#if 0
      hBrush = CreatePatternBrush (pGC->tile.pixmap->devPrivate.ptr);
      hBrush = SelectObject (hdc, hBrush);

      PatBlt (hdc, 0, 0, pDrawable->width, pDrawable->height, PATCOPY);

      hBrush = SelectObject (hdc, hBrush);
      
      DeleteObject (hBrush);
#endif

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
	      BitBlt (hdc,
		      pPoint->x + iX,
		      pPoint->y,
		      pGC->tile.pixmap->drawable.width,
		      1,
		      hdcMem,
		      0,
		      pPoint->y % pGC->tile.pixmap->drawable.height,
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
