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
/* $XFree86: xc/programs/Xserver/hw/xwin/winsetsp.c,v 1.3 2001/07/02 09:37:17 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 55 */
void
winSetSpansNativeGDI (DrawablePtr	pDrawable,
		      GCPtr		pGC,
		      char		*pSrcs,
		      DDXPointPtr	pPoints,
		      int		*pWidths,
		      int		nSpans,
		      int		fSorted)
{
#if WIN_NATIVE_GDI_SUPPORT
  winGCPriv(pGC);
  winPrivPixmapPtr	pPixmapPriv = NULL;
  PixmapPtr		pPixmap = NULL;
  int			iIdx = 0;
  static int		iCount = 0;
  HBITMAP		hBitmap = NULL;
  char			*pSrc = pSrcs;
  int			*pWidth = NULL;
  DDXPointPtr		pPoint = NULL;
  HDC			hdcMem;
  BITMAPINFOHEADER	bmih, bmihNew;

  /* Setup the bitmap header info */
  bmih.biSize = sizeof (bmih);
  bmih.biWidth = pDrawable->width;
  bmih.biHeight = pDrawable->height;
  bmih.biPlanes = 1;
  bmih.biBitCount = pDrawable->depth;
  bmih.biCompression = BI_RGB;
  bmih.biSizeImage = 0;
  bmih.biXPelsPerMeter = 0;
  bmih.biYPelsPerMeter = 0;
  bmih.biClrUsed = 0;
  bmih.biClrImportant = 0;

  ErrorF ("\nwinSetSpans () - pDrawable: %08x\n",
	  pDrawable);

  /* What kind of raster op have we got here? */
  switch (pGC->alu)
    {
    case GXclear:
      ErrorF ("winSetSpans () - GXclear\n");
      break;
    case GXand:
      ErrorF ("winSetSpans () - GXand:\n");
      break;
    case GXandReverse:
      ErrorF ("winSetSpans () - GXandReverse\n");
      break;
    case GXcopy:
      ErrorF ("winSetSpans () - GXcopy\n");
      
      /*
       * FIXME: Assuming that the drawable is a pixmap.
       */
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);

      /* Loop through spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  pWidth = pWidths + iIdx;
	  pPoint = pPoints + iIdx;
	  
	  /* Blast the bits to the drawable */
	  SetDIBits (pGCPriv->hdcMem,
		     pPixmapPriv->hBitmap,
		     pPoint->y, 1, 
		     pSrc,
		     pPixmapPriv->pbmih, 0);
	  
	  /* Display some useful information */
	  ErrorF ("(%dx%dx%d) (%d,%d) w: %d ps: %08x\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, *pWidth, pSrc);

	  /* Calculate offset of next bit source */
	  pSrc += 4 * ((*pWidth  + 31) / 32);
	}
      break;
    case GXandInverted:
      ErrorF ("winSetSpans () - GXandInverted\n");
      break;
    case GXnoop:
      ErrorF ("winSetSpans () - GXnoop\n");
      break;
    case GXxor:
      ErrorF ("winSetSpans () - GXxor\n");
      break;
    case GXor:
      ErrorF ("winSetSpans () - GXor\n");
      break;
    case GXnor:
      ErrorF ("winSetSpans () - GXnor\n");
      break;
    case GXequiv:
      ErrorF ("winSetSpans () - GXequiv\n");
      break;
    case GXinvert:
      ErrorF ("winSetSpans () - GXinvert\n");

      hdcMem = CreateCompatibleDC (pGCPriv->hdc);
 
      /* Loop through spans */
      for (iIdx = 0; iIdx < nSpans; ++iIdx)
	{
	  pWidth = pWidths + iIdx;
	  pPoint = pPoints + iIdx;
	  
	  /* Setup the bitmap header info */
	  bmihNew.biSize = sizeof (bmihNew);
	  bmihNew.biWidth = *pWidth;
	  bmihNew.biHeight = 1;
	  bmihNew.biPlanes = 1;
	  bmihNew.biBitCount = pDrawable->depth;
	  bmihNew.biCompression = BI_RGB;
	  bmihNew.biSizeImage = 0;
	  bmihNew.biXPelsPerMeter = 0;
	  bmihNew.biYPelsPerMeter = 0;
	  bmihNew.biClrUsed = 0;
	  bmihNew.biClrImportant = 0;

	  /* Create a DIB from span line */
	  if (pDrawable->depth == 1)
	    {
	      hBitmap = CreateDIBitmap (NULL, &bmih, 0, pSrc, NULL, 0);
	    }
	  else
	    {
	      hBitmap = CreateDIBitmap (pGCPriv->hdcMem,
					&bmih, 0, pSrc, NULL, 0);
	    }
	  hBitmap = SelectObject (hdcMem, hBitmap);

	  /* Blit the span line to the drawable */
	  BitBlt (pGCPriv->hdcMem, pPoint->x, pPoint->y,
		  *pWidth / pDrawable->depth, 1,
		  hdcMem, 0, 0, NOTSRCCOPY);

	  /* Display some useful information */
	  ErrorF ("(%dx%dx%d) (%d,%d) w: %d ps: %08x\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, *pWidth, pSrc);

	  /* Calculate offset of next bit source */
	  pSrc += 4 * ((*pWidth + 31) / 32);

	  /* Pop the bitmap out of the memory DC */
	  SelectObject (hdcMem, hBitmap);

	  /* Free the temporary bitmap */
	  DeleteObject (hBitmap);
	  hBitmap = NULL;
	}

      /* Release the scratch DC */
      DeleteDC (hdcMem);

      break;
    case GXorReverse:
      ErrorF ("winSetSpans () - GXorReverse\n");
      break;
    case GXcopyInverted:
      ErrorF ("winSetSpans () - GXcopyInverted\n");
      break;
    case GXorInverted:
      ErrorF ("winSetSpans () - GXorInverted\n");
      break;
    case GXnand:
      ErrorF ("winSetSpans () - GXnand\n");
      break;
    case GXset:
      ErrorF ("winSetSpans () - GXset\n");
    default:
      ErrorF ("winSetSpans () - Unknown ROP\n");
      break;
    }
#endif
}
