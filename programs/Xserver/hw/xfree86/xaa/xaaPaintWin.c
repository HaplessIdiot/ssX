/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaPaintWin.c,v 1.2 1998/07/25 16:58:50 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "xaawrap.h"


void
XAAPaintWindow(
  WindowPtr pWin,
  RegionPtr prgn,
  int what 
)
{
    ScreenPtr  pScreen = pWin->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((&pWin->drawable));
    int nBox = REGION_NUM_RECTS(prgn);
    BoxPtr pBox = REGION_RECTS(prgn);
    int fg;
    PixmapPtr pPix = NULL;

    if(!infoRec->pScrn->vtSema) goto BAILOUT;	

    switch (what) {
    case PW_BACKGROUND:
	switch(pWin->backgroundState) {
	case None: return;
	case ParentRelative:
	    do { pWin = pWin->parent; }
	    while(pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, prgn, what);
	    return;
	case BackgroundPixel:
	    fg = pWin->background.pixel;
	    break;
	case BackgroundPixmap:
	    pPix = pWin->background.pixmap;
	    break;
	}
	break;
    case PW_BORDER:
	if (pWin->borderIsPixel) 
	    fg = pWin->border.pixel;
	else 	/* pixmap */ 
	    pPix = pWin->border.pixmap;
	break;
    default: return;
    }


    if(!pPix) {
        if(infoRec->FillSolidRects &&
           (!(infoRec->FillSolidRectsFlags & RGB_EQUAL) || 
                (CHECK_RGB_EQUAL(fg))) )  {
	    (*infoRec->FillSolidRects)(infoRec->pScrn, fg, GXcopy, ~0,
					nBox, pBox);
	    return;
	}
    } else {	/* pixmap */
        XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	WindowPtr pBgWin = pWin;

	if (what == PW_BORDER) {
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);
	}

	if(pPriv->flags & DIRTY) {
	    pPriv->flags = 0;
	    pPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        }

    	if(!(pPriv->flags & REDUCIBILITY_CHECKED) &&
	    (infoRec->CanDoMono8x8 || infoRec->CanDoColor8x8)) {
	    XAACheckTileReducibility(pPix, infoRec->CanDoMono8x8);
	}

	if(pPriv->flags & REDUCIBLE_TO_8x8) {
	    if((pPriv->flags & REDUCIBLE_TO_2_COLOR) &&
		infoRec->CanDoMono8x8 && infoRec->FillMono8x8PatternRects &&
		!(infoRec->FillMono8x8PatternRectsFlags & TRANSPARENCY_ONLY) && 
		(!(infoRec->FillMono8x8PatternRectsFlags & RGB_EQUAL) || 
		(CHECK_RGB_EQUAL(pPriv->fg) && CHECK_RGB_EQUAL(pPriv->bg)))) {

	    	(*infoRec->FillMono8x8PatternRects)(infoRec->pScrn,
			pPriv->fg, pPriv->bg, GXcopy, ~0, nBox, pBox,
			pPriv->pattern0, pPriv->pattern1,
			pBgWin->drawable.x, pBgWin->drawable.y);
		return;
	    }
	    if(infoRec->CanDoColor8x8 && infoRec->FillColor8x8PatternRects) {
		XAACacheInfoPtr pCache = (*infoRec->CacheColor8x8Pattern)(
					infoRec->pScrn, pPix, -1, -1);

		(*infoRec->FillColor8x8PatternRects) (
			infoRec->pScrn, GXcopy, ~0, nBox, pBox, 
			pBgWin->drawable.x, pBgWin->drawable.y, pCache);
		return;
	    }        
	}
	if(infoRec->UsingPixmapCache && infoRec->FillCacheBltRects && 
	    (pPix->drawable.height <= infoRec->MaxCacheableTileHeight) &&
	    (pPix->drawable.width <= infoRec->MaxCacheableTileWidth)) {

	     XAACacheInfoPtr pCache = 
			(*infoRec->CacheTile)(infoRec->pScrn, pPix);
	     (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, ~0,
		nBox, pBox, pBgWin->drawable.x, pBgWin->drawable.y, pCache);
	     return;
	}
    }


    if(infoRec->NeedToSync) {
	(*infoRec->Sync)(infoRec->pScrn);
	infoRec->NeedToSync = FALSE;
    }

BAILOUT:

    if(what == PW_BACKGROUND) {
	XAA_SCREEN_PROLOGUE (pScreen, PaintWindowBackground);
	(*pScreen->PaintWindowBackground) (pWin, prgn, what);
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBackground, XAAPaintWindow);
    } else {
	XAA_SCREEN_PROLOGUE (pScreen, PaintWindowBorder);
	(*pScreen->PaintWindowBorder) (pWin, prgn, what);
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBorder, XAAPaintWindow);
    }

}
