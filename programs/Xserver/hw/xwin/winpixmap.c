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
 * Authors:	drewry, september 1986
 *		Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winpixmap.c,v 1.4 2001/06/04 13:04:41 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 34 */
/* See mfb/mfbpixmap.c - mfbCreatePixmap() */
PixmapPtr
winCreatePixmapNativeGDI (ScreenPtr pScreen,
			  int nWidth, int nHeight,
			  int nDepth)
{
#if WIN_NATIVE_GDI_SUPPORT
  winPrivPixmapPtr	pPixmapPriv = NULL;
  PixmapPtr		pPixmap = NULL;
  BITMAPINFOHEADER	*pbmih = NULL;
  HDC			hdcMem = NULL;
  
  /* Allocate pixmap memory */
  pPixmap = AllocatePixmap (pScreen, 0);
  if (!pPixmap)
    {
      ErrorF ("winCreatePixmapNativeGDI () - Couldn't allocate a pixmap\n");
      return NullPixmap;
    }

  ErrorF ("winCreatePixmap () - w %d h %d d %d bw %d\n",
	  nWidth, nHeight, nDepth,
	  PixmapBytePad (nWidth, nDepth));

  /* Setup pixmap values */
  pPixmap->drawable.type = DRAWABLE_PIXMAP;
  pPixmap->drawable.class = 0;
  pPixmap->drawable.pScreen = pScreen;
  pPixmap->drawable.depth = nDepth;
  pPixmap->drawable.bitsPerPixel = BitsPerPixel (nDepth);
  pPixmap->drawable.id = 0;
  pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
  pPixmap->drawable.x = 0;
  pPixmap->drawable.y = 0;
  pPixmap->drawable.width = nWidth;
  pPixmap->drawable.height = nHeight;
  pPixmap->devKind = 0;
  pPixmap->refcnt = 1;
  pPixmap->devPrivate.ptr = NULL;
  
  /* Check for zero width or height pixmaps */
  if (nWidth == 0 || nHeight == 0)
    {
      /* Don't allocate a real pixmap, just set fields and return */
      return pPixmap;
    }

  /* Pixmap privates are allocated by AllocatePixmap */
  pPixmapPriv = winGetPixmapPriv (pPixmap);

#if CYGDEBUG
  ErrorF ("winCreatePixmapNativeGDI () - Allocated privates\n");
#endif

  /* Create a scratch DC */
  hdcMem = CreateCompatibleDC (NULL);
  if (hdcMem == NULL)
    {
      ErrorF ("winCreatePixmapNativeGDI () - CreateCompatibleDC () failed\n");
      return NULL;
    }

  /* Allocate bitmap info header */
  pbmih = (BITMAPINFOHEADER*) xalloc (sizeof (BITMAPINFOHEADER)
				      + 256 * sizeof (RGBQUAD));
  if (pbmih == NULL)
    {
      ErrorF ("winCreatePixmapNativeGDI () - xalloc () failed\n");
      return FALSE;
    }
  ZeroMemory (pbmih, sizeof(BITMAPINFOHEADER) + 256 * sizeof (RGBQUAD));

  /* Describe bitmap to be created */
  pbmih->biSize = sizeof (BITMAPINFOHEADER);
  pbmih->biWidth = nWidth;
  pbmih->biHeight = -nHeight;
  pbmih->biPlanes = 1;
  pbmih->biBitCount = nDepth;
  pbmih->biCompression = BI_RGB;
  pbmih->biSizeImage = 0;
  pbmih->biXPelsPerMeter = 0;
  pbmih->biYPelsPerMeter = 0;
  pbmih->biClrUsed = 0;
  pbmih->biClrImportant = 0;

#if CYGDEBUG 
  ErrorF ("winCreatePixmapNativeGDI () - Calling CreateDIBSection ()\n");
#endif

  /* Create a DIB with a bit pointer */
  pPixmapPriv->hBitmap = CreateDIBSection (hdcMem,
					   (BITMAPINFO *) pbmih,
					   DIB_RGB_COLORS,
					   &pPixmapPriv->pvBits,
					   NULL,
					   0);
  if (pPixmapPriv->hBitmap == NULL)
    {
      ErrorF ("winCreatePixmapNativeGDI () - CreateDIBSection () failed\n");
      return NullPixmap;
    }

  /* Free the bitmap info header memory */
  xfree (pbmih);
  pbmih = NULL;

#if CYGDEBUG
  ErrorF ("winCreatePixmapNativeGDI () - CreateDIBSection () returned\n");
#endif

  /* Save the pixmap padded scanlie width */
  pPixmapPriv->dwScanlineBytes = PixmapBytePad (nWidth, nDepth);
  pPixmapPriv->hdcSelected = NULL;

  /* Free the scratch DC */
  DeleteDC (hdcMem);
  hdcMem = NULL;

#if CYGDEBUG
  ErrorF ("winCreatePixmap () - Created a pixmap %08x, %dx%dx%d, for " \
	  "screen: %08x\n",
	  pPixmapPriv->hBitmap, nWidth, nHeight, nDepth, pScreen);
#endif

  return pPixmap;
#else 
  return NullPixmap;
#endif
}

/* See Porting Layer Definition - p. 35 */
/* See mfb/mfbpixmap.c - mfbDestroyPixmap() */
Bool
winDestroyPixmapNativeGDI (PixmapPtr pPixmap)
{
  winPrivPixmapPtr		pPixmapPriv = NULL;
  
  ErrorF ("winDestroyPixmapNativeGDI ()\n");

  if (pPixmap == NULL)
    {
      ErrorF ("winDestroyPixmapNativeGDI () - No pixmap to destroy\n");
      return TRUE;
    }

  pPixmapPriv = winGetPixmapPriv (pPixmap);

  ErrorF ("winDestroyPixmapNativeGDI - pPixmapPriv->hBitmap: %08x\n",
	  pPixmapPriv->hBitmap);

  /* Decrement reference count, return if nonzero */
  if (--pPixmap->refcnt)
    return TRUE;

  /* Free GDI bitmap */
  if (pPixmapPriv->hBitmap) DeleteObject (pPixmapPriv->hBitmap);
  
  /* Free the pixmap memory */
  xfree (pPixmap);
  pPixmap = NULL;

  return TRUE;
}

Bool
winModifyPixmapHeaderNativeGDI (PixmapPtr pPixmap,
				int iWidth, int iHeight,
				int iDepth,
				int iBitsPerPixel,
				int devKind,
				pointer pPixData)
{
  ErrorF ("winModifyPixmapHeaderNativeGDI ()\n");
  return TRUE;
}

/* See cfb/cfbpixmap.c */
void
winXRotatePixmapNativeGDI (PixmapPtr pPix, int rw)
{
  ErrorF ("winXRotatePixmap()\n");
  /* fill in this function, look at CFB */
}

/* See cfb/cfbpixmap.c */
void
winYRotatePixmapNativeGDI (PixmapPtr pPix, int rh)
{
  ErrorF ("winYRotatePixmap()\n");
  /* fill in this function, look at CFB */
}

/* See cfb/cfbpixmap.c */
void
winCopyRotatePixmapNativeGDI (PixmapPtr psrcPix, PixmapPtr *ppdstPix,
			      int xrot, int yrot)
{
  ErrorF ("winCopyRotatePixmap()\n");
  /* fill in this function, look at CFB */
}
