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
/* $XFree86: xc/programs/Xserver/hw/xwin/wingetsp.c,v 1.3 2001/07/02 09:37:17 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 55 */
void
winGetSpansNativeGDI (DrawablePtr	pDrawable, 
		      int		nMax, 
		      DDXPointPtr	pPoints, 
		      int		*piWidths, 
		      int		iSpans, 
		      char		*pDsts)
{
  PixmapPtr		pPixmap = NULL;
  winPrivPixmapPtr	pPixmapPriv = NULL;
  int			iSpan;
  DDXPointPtr		pPoint = NULL;
  int			*piWidth = NULL;
  char			*pDst = pDsts;
  HBITMAP		hbmpOrig = NULL;
  HDC			hdcMem;

  /* Branch on the drawable type */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);

      /* Create a temporary DC */
      hdcMem = CreateCompatibleDC (NULL);

      /* Select the drawable pixmap into a DC */
      hbmpOrig = SelectObject (hdcMem, pPixmapPriv->hBitmap);

      /* Loop through spans */
      for (iSpan = 0; iSpan < iSpans; ++iSpan)
	{
	  pPoint = pPoints + iSpan;
	  piWidth = piWidths + iSpan;

	  /* Grab the bits from the DIB */
	  GetDIBits (hdcMem, 
		     pPixmapPriv->hBitmap,
		     pPoint->y, 1, 
		     pDst, 
		     pPixmapPriv->pbmih, 
		     0);

	  ErrorF ("(%dx%dx%d) (%d,%d) w: %d\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, *piWidth);

	  /* Calculate offset of next bit destination */
	  pDst += 4 * ((*piWidth + 31) / 32);
	}
      
      /* Push the drawable pixmap out of the GC HDC */
      SelectObject (hdcMem, hbmpOrig);

      /* Delete the temporary DC */
      DeleteDC (hdcMem);
      break;

    case DRAWABLE_WINDOW:
      FatalError ("winGetSpansNativeGDI - DRAWABLE_WINDOW\n");
      break;

    case UNDRAWABLE_WINDOW:
      FatalError ("winGetSpansNativeGDI - UNDRAWABLE_WINDOW\n");
      break;

    case DRAWABLE_BUFFER:
      FatalError ("winGetSpansNativeGDI - DRAWABLE_BUFFER\n");
      break;
      
    default:
      FatalError ("winGetSpansNativeGDI - Unknown drawable type\n");
      break;
    }
}
