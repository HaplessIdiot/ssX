/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaOverlay.c,v 1.3 1998/12/06 06:08:42 dawes Exp $ */

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
#include "xaawrap.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "cfb8_32.h"

extern WindowPtr *WindowTable;

void
XAACopyWindow8_32(
    WindowPtr pWin,
    DDXPointRec ptOldOrg,
    RegionPtr prgnSrc
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DDXPointPtr ppt, pptSrc;
    RegionRec rgnDst8, rgnDst32;
    BoxPtr pbox;
    int i, nbox, dx, dy;
    WindowPtr pRoot = WindowTable[pScreen->myNum];
    XAAInfoRecPtr infoRec = 
	GET_XAAINFORECPTR_FROM_DRAWABLE((&pWin->drawable));

    if (!infoRec->pScrn->vtSema || !infoRec->ScreenToScreenBitBlt) { 
	XAA_SCREEN_PROLOGUE (pScreen, CopyWindow);
	if(infoRec->pScrn->vtSema && infoRec->NeedToSync) {
	    (*infoRec->Sync)(infoRec->pScrn);
	    infoRec->NeedToSync = FALSE;
	}
        (*pScreen->CopyWindow) (pWin, ptOldOrg, prgnSrc);
	XAA_SCREEN_EPILOGUE (pScreen, CopyWindow, XAACopyWindow8_32);
    	return;
    }

    REGION_INIT(pScreen, &rgnDst8, NullBox, 0);
    REGION_INIT(pScreen, &rgnDst32, NullBox, 0);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pScreen, prgnSrc, -dx, -dy);
    REGION_INTERSECT(pScreen, &rgnDst8, &pWin->borderClip, prgnSrc);

    infoRec->ScratchGC.alu = GXcopy;

    nbox = REGION_NUM_RECTS(&rgnDst8);
    if(nbox &&
	(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))) {

	pbox = REGION_RECTS(&rgnDst8);
	for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
	    ppt->x = pbox->x1 + dx;
	    ppt->y = pbox->y1 + dy;
	}

	infoRec->ScratchGC.planemask = 0xff000000;
	XAADoBitBlt((DrawablePtr)pRoot, (DrawablePtr)pRoot,
        		&(infoRec->ScratchGC), &rgnDst8, pptSrc);
	DEALLOCATE_LOCAL(pptSrc);
    }

    cfb8_32SegregateChildren(pWin, &rgnDst32);
    if(REGION_NOTEMPTY(pScreen, &rgnDst32)) {
	REGION_INTERSECT(pScreen, &rgnDst32, &rgnDst32, prgnSrc);
	nbox = REGION_NUM_RECTS(&rgnDst32);
	if(nbox &&
	  (pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec)))){

	    pbox = REGION_RECTS(&rgnDst32);
	    for (i = nbox, ppt = pptSrc; i--; ppt++, pbox++) {
		ppt->x = pbox->x1 + dx;
		ppt->y = pbox->y1 + dy;
	    }

	    infoRec->ScratchGC.planemask = 0x00ffffff;
	    XAADoBitBlt((DrawablePtr)pRoot, (DrawablePtr)pRoot,
        		&(infoRec->ScratchGC), &rgnDst32, pptSrc);

	    DEALLOCATE_LOCAL(pptSrc);
	}
    }

    REGION_UNINIT(pScreen, &rgnDst8);
    REGION_UNINIT(pScreen, &rgnDst32);
}




void
XAAPaintWindow8_32(
  WindowPtr pWin,
  RegionPtr prgn,
  int what 
){
    ScreenPtr  pScreen = pWin->drawable.pScreen;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_DRAWABLE((&pWin->drawable));
    int nBox = REGION_NUM_RECTS(prgn);
    BoxPtr pBox = REGION_RECTS(prgn);
    int depth = pWin->drawable.depth;
    int fg, key;
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
	if(depth == 24)
	   key = cfb8_32GetKey(pScreen) << 24;
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
	    if(depth == 8)
		(*infoRec->FillSolidRects)(infoRec->pScrn, fg << 24, GXcopy, 
						0xff000000, nBox, pBox);
	    else if(what == PW_BORDER) 
		(*infoRec->FillSolidRects)(infoRec->pScrn, 	
					(fg & 0x00ffffff) | key, 
					GXcopy, ~0, nBox, pBox);
	    else
		(*infoRec->FillSolidRects)(infoRec->pScrn, fg, GXcopy, 
						0x00ffffff, nBox, pBox);
	    return;
	}
    } else {	/* pixmap */
        XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pPix);
	WindowPtr pBgWin = pWin;
	unsigned int pm = (depth == 8) ? 0xff000000 : 0x00ffffff;
	Bool DoExpose = FALSE;


	if (what == PW_BORDER) {
	    for (pBgWin = pWin;
		 pBgWin->backgroundState == ParentRelative;
		 pBgWin = pBgWin->parent);
	    if(depth == 24)
		DoExpose = TRUE;
	}

	if(IS_OFFSCREEN_PIXMAP(pPix) && infoRec->FillCacheBltRects) {
	    XAACacheInfoPtr pCache = &(infoRec->ScratchCacheInfoRec);

	    pCache->x = pPriv->offscreenArea->box.x1;
	    pCache->y = pPriv->offscreenArea->box.y1;
	    pCache->w = pCache->orig_w = 
		pPriv->offscreenArea->box.x2 - pCache->x;
	    pCache->h = pCache->orig_h = 
		pPriv->offscreenArea->box.y2 - pCache->y;
	     
	    (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, pm,
		nBox, pBox, pBgWin->drawable.x, pBgWin->drawable.y, pCache);

	    if(DoExpose)
		(*infoRec->FillSolidRects)(infoRec->pScrn, key, GXcopy, 
						0xff000000, nBox, pBox);
	    return;
	}

	if(pPriv->flags & DIRTY) {
	    pPriv->flags &= ~(DIRTY | REDUCIBILITY_MASK);
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
			pPriv->fg, pPriv->bg, GXcopy, pm, nBox, pBox,
			pPriv->pattern0, pPriv->pattern1,
			pBgWin->drawable.x, pBgWin->drawable.y);
		if(DoExpose)
		    (*infoRec->FillSolidRects)(infoRec->pScrn, key, GXcopy, 
						0xff000000, nBox, pBox);
		return;
	    }
	    if(infoRec->CanDoColor8x8 && infoRec->FillColor8x8PatternRects) {
		XAACacheInfoPtr pCache = (*infoRec->CacheColor8x8Pattern)(
					infoRec->pScrn, pPix, -1, -1);

		(*infoRec->FillColor8x8PatternRects) (
			infoRec->pScrn, GXcopy, pm, nBox, pBox, 
			pBgWin->drawable.x, pBgWin->drawable.y, pCache);
		if(DoExpose)
		    (*infoRec->FillSolidRects)(infoRec->pScrn, key, GXcopy, 
						0xff000000, nBox, pBox);
		return;
	    }        
	}

	if(infoRec->UsingPixmapCache && infoRec->FillCacheBltRects && 
	    (pPix->drawable.height <= infoRec->MaxCacheableTileHeight) &&
	    (pPix->drawable.width <= infoRec->MaxCacheableTileWidth)) {

	     XAACacheInfoPtr pCache = 
			(*infoRec->CacheTile)(infoRec->pScrn, pPix);
	     (*infoRec->FillCacheBltRects)(infoRec->pScrn, GXcopy, pm,
		nBox, pBox, pBgWin->drawable.x, pBgWin->drawable.y, pCache);
	     if(DoExpose)
		(*infoRec->FillSolidRects)(infoRec->pScrn, key, GXcopy, 
						0xff000000, nBox, pBox);
	     return;
	}

	if(infoRec->FillImageWriteRects && 
		!(infoRec->FillImageWriteRectsFlags & NO_GXCOPY)) {
	    (*infoRec->FillImageWriteRects) (infoRec->pScrn, GXcopy, 
                   pm, nBox, pBox, pBgWin->drawable.x, pBgWin->drawable.y,
                   pPix);
	    if(DoExpose)
		(*infoRec->FillSolidRects)(infoRec->pScrn, key, GXcopy, 
						0xff000000, nBox, pBox);
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
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBackground, XAAPaintWindow8_32);
    } else {
	XAA_SCREEN_PROLOGUE (pScreen, PaintWindowBorder);
	(*pScreen->PaintWindowBorder) (pWin, prgn, what);
	XAA_SCREEN_EPILOGUE(pScreen, PaintWindowBorder, XAAPaintWindow8_32);
    }
}


void
XAAWindowExposures8_32(
   WindowPtr pWin,
   RegionPtr pReg,
   RegionPtr pOtherReg
){
    ScreenPtr pScreen = pWin->drawable.pScreen;
    if(pWin->drawable.depth == 24) {
	XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCREEN(pScreen);
	if(REGION_NUM_RECTS(pReg) && infoRec->FillSolidRects) {
	    (*infoRec->FillSolidRects)(infoRec->pScrn, 
			(cfb8_32GetKey(pScreen) << 24), GXcopy, 0xff000000,
			REGION_NUM_RECTS(pReg), REGION_RECTS(pReg));
	    miWindowExposures(pWin, pReg, pOtherReg);
	    return;
	} else if(infoRec->NeedToSync) {
            (*infoRec->Sync)(infoRec->pScrn);
            infoRec->NeedToSync = FALSE;
	}
    } 

    XAA_SCREEN_PROLOGUE (pScreen, WindowExposures);
    (*pScreen->WindowExposures) (pWin, pReg, pOtherReg);
    XAA_SCREEN_EPILOGUE(pScreen, WindowExposures, XAAWindowExposures8_32);
}
