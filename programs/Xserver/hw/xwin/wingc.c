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
/* $XFree86: xc/programs/Xserver/hw/xwin/wingc.c,v 1.4 2001/07/02 09:37:17 alanh Exp $ */

#include "win.h"

/* GC Handling Routines */
const GCFuncs winGCFuncs = {
  winValidateGCNativeGDI,
  winChangeGCNativeGDI,
  winCopyGCNativeGDI,
  winDestroyGCNativeGDI,
  winChangeClipNativeGDI,
  winDestroyClipNativeGDI,
  winCopyClipNativeGDI,
};

/* Drawing Primitives */
const GCOps winGCOps = {
  winFillSpansNativeGDI,
  winSetSpansNativeGDI,
  miPutImage,
  miCopyArea,
  miCopyPlane,
  miPolyPoint,
  winPolyLineNativeGDI,
  miPolySegment,
  miPolyRectangle,
  miPolyArc,
  miFillPolygon,
  miPolyFillRect,
  miPolyFillArc,
  miPolyText8,
  miPolyText16,
  miImageText8,
  miImageText16,
  miImageGlyphBlt,
  miPolyGlyphBlt,
  miPushPixels
#ifdef NEED_LINEHELPER
  ,NULL
#endif
};

/* See Porting Layer Definition - p. 45 */
/* See mfb/mfbgc.c - mfbCreateGC() */
/* See Strategies for Porting - pp. 15, 16 */
Bool
winCreateGCNativeGDI (GCPtr pGC)
{
  winPrivGCPtr		pGCPriv = NULL;
  winPrivScreenPtr	pScreenPriv = NULL;

  ErrorF ("winCreateGCNativeGDI () depth: %d\n",
	  pGC->depth);

  pGC->clientClip = NULL;
  pGC->clientClipType = CT_NONE;

  pGC->ops = (GCOps *) &winGCOps;
  pGC->funcs = (GCFuncs *) &winGCFuncs;

  pGC->miTranslate = 0;

  /* Allocate privates for this GC */
  pGCPriv = winGetGCPriv (pGC);
  if (pGCPriv == NULL)
    {
      ErrorF ("winCreateGCNativeGDI () - Privates pointer was NULL\n");
      return FALSE;
    }

  /* Copy the screen DC to the local privates */
  pScreenPriv = winGetScreenPriv (pGC->pScreen);
  pGCPriv->hdc = pScreenPriv->hdcScreen;

  /* Allocate a memory DC for the GC */
  pGCPriv->hdcMem = CreateCompatibleDC (pGCPriv->hdc);

#if 0
    winGetRotatedPixmapNativeGDI(pGC) = 0;
    winGetExposeNativeGDI(pGC) = 1;
    winGetFreeCompClipNativeGDI(pGC) = 0;
    winGetCompositeClipNativeGDI(pGC) = 0;
    winGetGCPrivateNativeGDI(pGC)->bpp = BitsPerPixel (pGC->depth);
#endif

  return TRUE;
}

/* See Porting Layer Definition - p. 45 */
void
winChangeGCNativeGDI (GCPtr pGC, unsigned long ulChanges)
{
#if CYGDEBUG
  ErrorF ("winChangeGCNativeGDI () - Doing nothing\n");
#endif
}


void
winValidateGCNativeGDI (GCPtr pGC,
			unsigned long dwChanges,
			DrawablePtr pDrawable)
{
  winGCPriv(pGC);
  int			nIndex;
  DWORD			dwMask = dwChanges;
  winPrivPixmapPtr	pPixmapPriv = NULL;
  PixmapPtr		pPixmap = NULL;
  DEBUG_FN_NAME("winValidateGC");
  DEBUGVARS;
  DEBUGPROC_MSG;

  assert (pGCPriv->hdcMem != NULL);
  
  /* Branch on drawable type */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      /* I can handle drawable pixmaps, no problem */
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);
      assert (pPixmapPriv->hBitmap != NULL);
      
      /* Is this bitmap selected into a DC? */
      if (pPixmapPriv->hdcSelected != NULL
	  && pPixmapPriv->hdcSelected != pGCPriv->hdcMem)
	{
	  ErrorF ("winValidateGC - Pixmap was in another DC\n");

	  /* Pop the bitmap out of the last DC it was selected into */
	  if (SelectObject (pPixmapPriv->hdcSelected, g_hbmpGarbage) == NULL)
	    FatalError ("winValidateGC - SelectObject failed - L167\n");

	  /* Select the bitmap into the GC memory device context */
	  if (SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap) == NULL)
	    FatalError ("winValidateGC - SelectObject failed - L174\n");
	}
      else if (pPixmapPriv->hdcSelected == NULL)
	{
	  ErrorF ("winValidateGC - Pixmap was not in a DC\n");

	  /* Select the bitmap into the memory device context */
	  if (SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap) == NULL)
	    FatalError ("winValidateGC - SelectObject failed - L182\n");
	}
      else if (pPixmapPriv->hdcSelected == pGCPriv->hdcMem)
	{
	  ErrorF ("winValidateGC - Pixmap already selected into current DC\n");
	}
      else
	{
	  FatalError ("winValidateGC - Unhandled case - L186\n");
	}
	
      /* Save a pointer to the last DC the bitmap was selected into */
      pPixmapPriv->hdcSelected = pGCPriv->hdcMem;
      
      /* Sync the DC settings with the GC settings */
      switch (pGC->fillStyle)
	{
	case FillSolid:
	  ErrorF ("winValidateGC - DRAWABLE_PIXMAP - FillSolid\n");

	  /* Select a stock pen */
	  if (pDrawable->depth == 1 && pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting WHITE_PEN\n");
	      SelectObject (pGCPriv->hdcMem, GetStockObject (WHITE_PEN));
	    }
	  else if (pDrawable->depth == 1 && !pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting BLACK_PEN\n");
	      SelectObject (pGCPriv->hdcMem, GetStockObject (BLACK_PEN));
	    }
	  else if (pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting custom pen\n");
	      SelectObject (pGCPriv->hdcMem, 
			    CreatePen (PS_SOLID, 0, pGC->fgPixel));
	    }
	  else
	    {
	      SelectObject (pGCPriv->hdcMem, GetStockObject (BLACK_PEN));
	      ErrorF ("winValidateGC - Selecting BLACK_PEN\n");
	    }
	  break;

	case FillStippled:
	  ErrorF ("winValidateGC - DRAWABLE_PIXMAP - FillStippled\n");
	  
	  /* Set the foreground color for the stipple fill */
	  if (pGC->fgPixel == 0x1)
	    {
	      SetTextColor (pGCPriv->hdcMem, RGB(0x00, 0x00, 0x00));
	    }
	  else if (pGC->fgPixel == 0xFFFF)
	    {
	      SetTextColor (pGCPriv->hdcMem, RGB(0xFF, 0xFF, 0xFF));
	    }
	  else
	    {
	      SetTextColor (pGCPriv->hdcMem, RGB(0x00, 0x00, 0x00));
	    }
	  SetBkColor (pGCPriv->hdcMem, RGB(0x00, 0x00, 0x00));
	  break;

	default:
	  break;
	}
      break;

    case DRAWABLE_WINDOW:
      switch (pGC->fillStyle)
	{
	case FillTiled:
	  ErrorF ("winValidateGC - DRAWABLE_WINDOW - FillTiled\n");

	  /* I can handle drawable pixmaps, no problem */
	  pPixmap = pGC->tile.pixmap;
	  pPixmapPriv = winGetPixmapPriv (pPixmap);
      
	  /* Is this bitmap selected into a DC? */
	  if (pPixmapPriv->hdcSelected != NULL
	      && pPixmapPriv->hdcSelected != pGCPriv->hdcMem)
	    {
	      /* Pop the bitmap out of the last DC it was selected into */
	      if (SelectObject (pPixmapPriv->hdcSelected, g_hbmpGarbage) 
		  == NULL)
		FatalError ("winValidateGC - SelectObject failed - L167\n");

	      /* Select the bitmap into the GC memory device context */
	      if (SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap) == NULL)
		FatalError ("winValidateGC - SelectObject failed - L174\n");
	    }
	  else if (pPixmapPriv->hdcSelected == NULL)
	    {
	      /* Select the bitmap into the memory device context */
	      if (SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap) == NULL)
		FatalError ("winValidateGC - SelectObject failed - L182\n");
	    }
	  else if (pPixmapPriv->hdcSelected == pGCPriv->hdcMem)
	    {
	      ErrorF ("winValidateGC - Pixmap already selected into "
		      "current DC\n");
	    }
	  else
	    {
	      FatalError ("winValidateGC - Unhandled case - L186\n");
	    }
	  
	  /* Save a pointer to the last DC the bitmap was selected into */
	  pPixmapPriv->hdcSelected = pGCPriv->hdcMem;

	  	  /* Blit the tile to a remote area of the screen */
	  BitBlt (pGCPriv->hdc, 
		  64, 64,
		  pGC->tile.pixmap->drawable.width,
		  pGC->tile.pixmap->drawable.height,
		  pGCPriv->hdcMem,
		  0, 0, 
		  SRCCOPY);
	  DEBUG_MSG("Blitted the tile to a remote area of the screen");

	  break;

	case FillStippled:
	  FatalError ("winValidateGC - DRAWABLE_WINDOW - FillStippled\n");
	  break;

	case FillSolid:
	  FatalError ("winValidateGC - DRAWABLE_WINDOW - FillSolid\n");
	  break;

	default:
	  break;
	}

    case UNDRAWABLE_WINDOW:
      break;

    case DRAWABLE_BUFFER:
      FatalError ("winValidateGC - DRAWABLE_BUFFER\n");
      break;

    default:
      break;
    }

#if 0
  /* Inspect changes to the GC */
  while (dwMask)
    {
      /* This peels off one change at a time */
      nIndex = lowbit (dwMask);
      dwMask &= ~nIndex;
      
      switch (nIndex)
	{
	case GCFunction:
	  /* mfb falls through to GCForeground */
	  ErrorF ("winValidateGC - GCFunction\n");
	  break;
	case GCForeground:
	  ErrorF ("winValidateGC - GCForeground\n");
	  break;
	case GCPlaneMask:
	  ErrorF ("winValidateGC - GCPlaneMask\n");
	  break;
	case GCBackground:
	  ErrorF ("winValidateGC - GCBackground\n");
	  break;
	case GCLineStyle:
	case GCLineWidth:
	case GCJoinStyle:
	  ErrorF ("winValidateGC - GCLineStyle, etc.\n");
	  break;
	case GCCapStyle:
	  ErrorF ("winValidateGC - GCCapStyle\n");
	  break;
	case GCFillStyle:
	  ErrorF ("winValidateGC - GCFillStyle\n");
	  break;
	case GCFillRule:
	  ErrorF ("winValidateGC - GCFillRule\n");
	  break;
	case GCTile:
	  ErrorF ("winValidateGC - GCTile\n");
	  break;
	case GCStipple:
	  ErrorF ("winValidateGC - GCStipple\n");
	  break;
	case GCTileStipXOrigin:
	  ErrorF ("winValidateGC - GCTileStipXOrigin\n");
	  break;
	case GCTileStipYOrigin:
	  ErrorF ("winValidateGC - GCTileStipYOrigin\n");
	  break;
	case GCFont:
	  ErrorF ("winValidateGC - GCFont\n");
	  break;
	case GCSubwindowMode:
	  ErrorF ("winValidateGC - GCSubwindowMode\n");
	  break;
	case GCGraphicsExposures:
	  ErrorF ("winValidateGC - GCGraphicsExposures\n");
	  break;
	case GCClipXOrigin:
	  ErrorF ("winValidateGC - GCClipXOrigin\n");
	  break;
	case GCClipYOrigin:
	  ErrorF ("winValidateGC - GCClipYOrigin\n");
	  break;
	case GCClipMask:
	  ErrorF ("winValidateGC - GCClipMask\n");
	  break;
	case GCDashOffset:
	  ErrorF ("winValidateGC - GCDashOffset\n");
	  break;
	case GCDashList:
	  ErrorF ("winValidateGC - GCDashList\n");
	  break;
	case GCArcMode:
	  ErrorF ("winValidateGC - GCArcMode\n");
	  break;
	default:
	  ErrorF ("winValidateGC - default\n");
	  break;
	}
    }
#endif
}

#if 0
/* See Porting Layer Definition - pp. 45-46 */
/* See mfb/mfbgc.c - mfbValidateGC() */
/* See Strategies for Porting - pp. 15, 16 */
void
winValidateGCNativeGDI (GCPtr pGC,
			unsigned long dwChanges,
			DrawablePtr pDrawable)
{
#if WIN_NATIVE_GDI_SUPPORT
  winGCPriv(pGC);
  winPrivPixmapPtr	pPixmapPriv = NULL;
  PixmapPtr		pPixmap = NULL;
  int			nIndex, iResult;
  unsigned long		dwMask = dwChanges;
  HPEN			hPen;
  HBRUSH		hBrush;
  HBITMAP		hBitmap;
  HDC			hdc = pGCPriv->hdc;
  HDC			hdcMem = pGCPriv->hdcMem;
  HDC			hdcStipple = NULL;
  DEBUG_FN_NAME("winValidateGC");
  DEBUGVARS;

  ErrorF ("winValidateGC - pDrawable: %08x, pGC: %08x\n",
	  pDrawable, pGC);

  /* Branch on drawable type */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      /* I can handle drawable pixmaps, no problem */
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);

      ErrorF ("winValidateGC - pPixmapPriv->hBitmap: %08x\n",
	      pPixmapPriv->hBitmap);
      
      /* Is this bitmap selected into a DC? */
      if (pPixmapPriv->hdcSelected != NULL
	  && pPixmapPriv->hdcSelected != hdcMem)
	{
	  /* Pop the bitmap out of the last DC it was selected into */
	  if (SelectObject (pPixmapPriv->hdcSelected, g_hbmpGarbage) == NULL)
	    FatalError ("winValidateGC - SelectObject failed - L167\n");
	    	  
	  /* Set the last selected DC for the pixmap */
	  pPixmapPriv->hdcSelected = hdcMem;

	  /* Select the bitmap into the memory device context */
	  if (SelectObject (hdcMem, pPixmapPriv->hBitmap) == NULL)
	    FatalError ("winValidateGC - SelectObject failed - L174\n");
	}
      else if (pPixmapPriv->hdcSelected == NULL)
	{
	  /* Set the last selected DC for the pixmap */
	  pPixmapPriv->hdcSelected = hdcMem;

	  /* Select the bitmap into the memory device context */
	  if (SelectObject (hdcMem, pPixmapPriv->hBitmap) == NULL)
	    FatalError ("winValidateGC - SelectObject failed - L182\n");
	}
      else if (pPixmapPriv->hdcSelected == hdcMem)
	{
	  ErrorF ("winValidateGC - Pixmap already selected into current DC\n");
	}
      else
	{
	  FatalError ("winValidateGC - Unhandled case - L186\n");
	}
	
      /* Save a pointer to the last HDC this pixmap was selected into */
      pPixmapPriv->hdcSelected = hdcMem;

      /* Sync the DC settings with the GC settings */
      switch (pGC->fillStyle)
	{
	case FillSolid:
	  /* Select a stock pen */
	  if (pDrawable->depth == 1 && pGC->fgPixel)
	    {
	      SelectObject (hdcMem, GetStockObject (WHITE_PEN));
	    }
	  else if (pDrawable->depth == 1 && !pGC->fgPixel)
	    {
	      SelectObject (hdcMem, GetStockObject (BLACK_PEN));
	    }
	  else if (pGC->fgPixel)
	    {
	      SelectObject (hdcMem, GetStockObject (WHITE_PEN));
	    }
	  else
	    {
	      SelectObject (hdcMem, GetStockObject (BLACK_PEN));
	    }
	  break;

	case FillStippled:

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


	  ErrorF ("winValidateGC - Pixmap with pGC->fillStyle "
		  "FillStippled\n");
	  break;

	default:
	  break;
	}
      break;

    case DRAWABLE_WINDOW:
      switch (pGC->fillStyle)
	{
	case FillTiled:
	  pPixmapPriv = winGetPixmapPriv (pGC->tile.pixmap);
	  
	  /* Is this bitmap selected into a DC? */
	  if (pPixmapPriv->hdcSelected != NULL
	      && pPixmapPriv->hdcSelected != hdcMem)
	    {
	      /* Pop the bitmap out of the last DC it was selected into */
	      if (SelectObject (pPixmapPriv->hdcSelected, g_hbmpGarbage)
		  == NULL)
		FatalError ("winValidateGC - SelectObject failed - L233\n");
	      
	      /* Set the last selected DC for the pixmap */
	      pPixmapPriv->hdcSelected = hdcMem;
	      
	      /* Select the bitmap into the memory device context */
	      if (SelectObject (hdcMem, pPixmapPriv->hBitmap) == NULL)
		FatalError ("winValidateGC - SelectObject failed - L240\n");
	    }
	  else if (pPixmapPriv->hdcSelected == NULL)
	    {
	      /* Set the last selected DC for the pixmap */
	      pPixmapPriv->hdcSelected = hdcMem;
	      
	      /* Select the bitmap into the memory device context */
	      if (SelectObject (hdcMem, pPixmapPriv->hBitmap) == NULL)
		FatalError ("winValidateGC - SelectObject failed - L249\n");
	    }

	  /* Blit the tile to a remote area of the screen */
	  BitBlt (hdc,
		  64, 64,
		  pGC->tile.pixmap->drawable.width,
		  pGC->tile.pixmap->drawable.height,
		  hdcMem,
		  0, 0,
		  SRCCOPY);
	  DEBUG_MSG("Blitted the tile to a remote area of the screen");
	  break;
	case FillStippled:
	  break;

	case FillSolid:
	  break;

	default:
	  break;
	}
      break;
    case UNDRAWABLE_WINDOW:
      break;

    case DRAWABLE_BUFFER:
      break;

    default:
      ErrorF ("winValidateGC - Unknown pDrawable->type: %d\n",
	      pDrawable->type);
      break;
    }

#if 0
  /* Inspect changes to the GC */
  while (dwMask)
    {
      /* This peels off one change at a time */
      nIndex = lowbit (dwMask);
      dwMask &= ~nIndex;
      
      switch (nIndex)
	{
	case GCFunction:
	  /* mfb falls through to GCForeground */
	  ErrorF ("winValidateGC - GCFunction\n");
	  break;
	case GCForeground:
	  ErrorF ("winValidateGC - GCForeground\n");
	  break;
	case GCPlaneMask:
	  ErrorF ("winValidateGC - GCPlaneMask\n");
	  break;
	case GCBackground:
	  ErrorF ("winValidateGC - GCBackground\n");
	  break;
	case GCLineStyle:
	case GCLineWidth:
	case GCJoinStyle:
	  ErrorF ("winValidateGC - GCLineStyle, etc.\n");
	  break;
	case GCCapStyle:
	  ErrorF ("winValidateGC - GCCapStyle\n");
	  break;
	case GCFillStyle:
	  ErrorF ("winValidateGC - GCFillStyle\n");
	  break;
	case GCFillRule:
	  ErrorF ("winValidateGC - GCFillRule\n");
	  break;
	case GCTile:
	  ErrorF ("winValidateGC - GCTile\n");
	  break;
	case GCStipple:
	  ErrorF ("winValidateGC - GCStipple\n");
	  break;
	case GCTileStipXOrigin:
	  ErrorF ("winValidateGC - GCTileStipXOrigin\n");
	  break;
	case GCTileStipYOrigin:
	  ErrorF ("winValidateGC - GCTileStipYOrigin\n");
	  break;
	case GCFont:
	  ErrorF ("winValidateGC - GCFont\n");
	  break;
	case GCSubwindowMode:
	  ErrorF ("winValidateGC - GCSubwindowMode\n");
	  break;
	case GCGraphicsExposures:
	  ErrorF ("winValidateGC - GCGraphicsExposures\n");
	  break;
	case GCClipXOrigin:
	  ErrorF ("winValidateGC - GCClipXOrigin\n");
	  break;
	case GCClipYOrigin:
	  ErrorF ("winValidateGC - GCClipYOrigin\n");
	  break;
	case GCClipMask:
	  ErrorF ("winValidateGC - GCClipMask\n");
	  break;
	case GCDashOffset:
	  ErrorF ("winValidateGC - GCDashOffset\n");
	  break;
	case GCDashList:
	  ErrorF ("winValidateGC - GCDashList\n");
	  break;
	case GCArcMode:
	  ErrorF ("winValidateGC - GCArcMode\n");
	  break;
	default:
	  ErrorF ("winValidateGC - default\n");
	  break;
	}
    }
#endif
#endif
}
#endif

/* See Porting Layer Definition - p. 46 */
void
winCopyGCNativeGDI (GCPtr pGCsrc, unsigned long ulMask, GCPtr pGCdst)
{

}

/* See Porting Layer Definition - p. 46 */
void
winDestroyGCNativeGDI (GCPtr pGC)
{
  winGCPriv(pGC);

  /* Free the memory DC */
  if (pGCPriv->hdcMem != NULL)
    {
      DeleteDC (pGCPriv->hdcMem);
      pGCPriv->hdcMem = NULL;
    }

  /* Invalidate the screen DC pointer */
  pGCPriv->hdc = NULL;

  /* Invalidate the GC privates pointer */
  winSetGCPriv (pGC, NULL);
}

/* See Porting Layer Definition - p. 46 */
void
winChangeClipNativeGDI (GCPtr pGC, int nType, pointer pValue, int nRects)
{

}

/* See Porting Layer Definition - p. 47 */
void
winDestroyClipNativeGDI (GCPtr pGC)
{

}

/* See Porting Layer Definition - p. 47 */
void
winCopyClipNativeGDI (GCPtr pGCdst, GCPtr pGCsrc)
{

}
