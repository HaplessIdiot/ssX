/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaFillRect.c,v 1.7 1998/11/15 04:30:38 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"

/*
   Much of this file based on code by 
	Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
*/


	/*********************\
	|     Solid Rects     |
	\*********************/

void
XAAPolyFillRectSolid(
    DrawablePtr pDraw,
    GCPtr pGC,
    int		nrectFill, 	/* number of rectangles to fill */
    xRectangle	*prectInit   	/* Pointer to first rectangle to fill */
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int 	MaxBoxes = nrectFill * REGION_NUM_RECTS(pGC->pCompositeClip);
    BoxPtr	pClipBoxes;
    int		nboxes;
    int		xorg = pDraw->x;
    int		yorg = pDraw->y;

    if(nrectFill <= 0)
        return;

    if(xorg | yorg) {
	int n = nrectFill;
	xRectangle *prect = prectInit;

	while(n--) {
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }

    if(MaxBoxes > infoRec->NumPreAllocBoxes) {
	pClipBoxes = (BoxPtr)ALLOCATE_LOCAL(MaxBoxes * sizeof(BoxRec));
	if(!pClipBoxes) return;	
    } else pClipBoxes = infoRec->PreAllocBoxes;
    
    nboxes = XAAGetRectClipBoxes(pGC->pCompositeClip, pClipBoxes, 
					nrectFill, prectInit);

    if(nboxes) {
	(*infoRec->FillSolidRects) (infoRec->pScrn, 
		pGC->fgPixel, pGC->alu, pGC->planemask, nboxes, pClipBoxes);
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}



	/******************\
	|  Stippled Rects  |
	\******************/

void
XAAPolyFillRectStippled(
    DrawablePtr pDraw,
    GCPtr pGC,
    int		nrectFill, 	/* number of rectangles to fill */
    xRectangle	*prectInit   	/* Pointer to first rectangle to fill */
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int 	MaxBoxes = nrectFill * REGION_NUM_RECTS(pGC->pCompositeClip);
    BoxPtr	pClipBoxes;
    int		nboxes, type, fg, bg;
    int		xorg = pDraw->x;
    int		yorg = pDraw->y;

    if(nrectFill <= 0)
        return;

    if(pGC->fillStyle == FillStippled) {
	type = (*infoRec->StippledFillChooser)(pGC);
	fg = pGC->fgPixel;  bg = -1;
    } else {
	type = (*infoRec->OpaqueStippledFillChooser)(pGC);
	fg = pGC->fgPixel;  bg = pGC->bgPixel;
    }

    if(!type) {
	(*XAAFallbackOps.PolyFillRect)(pDraw, pGC, nrectFill, prectInit);
	return;
    }

    if(xorg | yorg) {
	int n = nrectFill;
	xRectangle *prect = prectInit;

	while(n--) {
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }

    if(MaxBoxes > infoRec->NumPreAllocBoxes) {
	pClipBoxes = (BoxPtr)ALLOCATE_LOCAL(MaxBoxes * sizeof(BoxRec));
	if(!pClipBoxes) return;	
    } else pClipBoxes = infoRec->PreAllocBoxes;
    
    nboxes = XAAGetRectClipBoxes(pGC->pCompositeClip, pClipBoxes, 
					nrectFill, prectInit);

    if(nboxes) {
        XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pGC->stipple);
	XAACacheInfoPtr pCache;

	if((fg == bg) && (bg != -1) && infoRec->FillSolidRects){
	   (*infoRec->FillSolidRects) (infoRec->pScrn, fg,
			pGC->alu, pGC->planemask, nboxes, pClipBoxes);
	} else
	switch(type) {
	case DO_MONO_8x8:
           (*infoRec->FillMono8x8PatternRects) (infoRec->pScrn, 
                fg, bg, pGC->alu, pGC->planemask, 
                nboxes, pClipBoxes, pPriv->pattern0, pPriv->pattern1, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y));
	    break;
	case DO_COLOR_8x8:
            pCache = (*infoRec->CacheColor8x8Pattern)(
                        infoRec->pScrn, pGC->stipple, fg, bg);
            (*infoRec->FillColor8x8PatternRects) (infoRec->pScrn,
                pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y),
                pCache);
	    break;
	case DO_CACHE_EXPAND:
            (*infoRec->FillCacheExpandRects) (infoRec->pScrn, fg, bg,
                pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
                pGC->stipple);
	    break;
	case DO_CACHE_BLT:
             pCache = (*infoRec->CacheStipple)(infoRec->pScrn, pGC->stipple, 
                                                        fg, bg);
             (*infoRec->FillCacheBltRects) (infoRec->pScrn, pGC->alu, 
                pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
                pCache);
	     break;
	case DO_COLOR_EXPAND:
           (*infoRec->FillColorExpandRects) (infoRec->pScrn, fg, bg, 
                pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
                pGC->stipple);
	    break;
	}
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}



	/***************\
	|  Tiled Rects  |
	\***************/

void
XAAPolyFillRectTiled(
    DrawablePtr pDraw,
    GCPtr pGC,
    int		nrectFill, 	/* number of rectangles to fill */
    xRectangle	*prectInit   	/* Pointer to first rectangle to fill */
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int 	MaxBoxes = nrectFill * REGION_NUM_RECTS(pGC->pCompositeClip);
    BoxPtr	pClipBoxes;
    int		nboxes, type;
    int		xorg = pDraw->x;
    int		yorg = pDraw->y;

    if(nrectFill <= 0)
        return;

    if(!(type = (*infoRec->TiledFillChooser)(pGC))) {
	(*XAAFallbackOps.PolyFillRect)(pDraw, pGC, nrectFill, prectInit);
	return;
    }

    if(xorg | yorg) {
	int n = nrectFill;
	xRectangle *prect = prectInit;

	while(n--) {
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }

    if(MaxBoxes > infoRec->NumPreAllocBoxes) {
	pClipBoxes = (BoxPtr)ALLOCATE_LOCAL(MaxBoxes * sizeof(BoxRec));
	if(!pClipBoxes) return;	
    } else pClipBoxes = infoRec->PreAllocBoxes;
    
    nboxes = XAAGetRectClipBoxes(pGC->pCompositeClip, pClipBoxes, 
					nrectFill, prectInit);

    if(nboxes) {
	XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pGC->tile.pixmap);
	XAACacheInfoPtr pCache;

	switch(type) {
	case DO_MONO_8x8:
           (*infoRec->FillMono8x8PatternRects) (infoRec->pScrn, 
                pPriv->fg, pPriv->bg, pGC->alu, pGC->planemask, 
                nboxes, pClipBoxes, pPriv->pattern0, pPriv->pattern1, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y));
	    break;
	case DO_COLOR_8x8:
            pCache = (*infoRec->CacheColor8x8Pattern)(
                        infoRec->pScrn, pGC->tile.pixmap, -1, -1);
            (*infoRec->FillColor8x8PatternRects) (infoRec->pScrn,
                pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y),
                pCache);
	    break;
	case DO_CACHE_BLT:
            pCache = (*infoRec->CacheTile)(infoRec->pScrn, pGC->tile.pixmap);
            (*infoRec->FillCacheBltRects) (infoRec->pScrn, pGC->alu, 
                pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
                pCache);
	    break;
	case DO_IMAGE_WRITE:
            (*infoRec->FillImageWriteRects) (infoRec->pScrn, pGC->alu, 
                pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y),
                pGC->tile.pixmap);
	    break;
	case DO_PIXMAP_COPY:
	    pCache = &(infoRec->ScratchCacheInfoRec);
	    pCache->x = pPriv->offscreenArea->box.x1;
	    pCache->y = pPriv->offscreenArea->box.y1;
	    pCache->w = pCache->orig_w = 
		pPriv->offscreenArea->box.x2 - pCache->x;
	    pCache->h = pCache->orig_h = 
		pPriv->offscreenArea->box.y2 - pCache->y;

            (*infoRec->FillCacheBltRects) (infoRec->pScrn, pGC->alu, 
                pGC->planemask, nboxes, pClipBoxes, 
                (xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
                pCache);
	    break;
	}
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}





	/************\
	|   Solid    |
	\************/

void
XAAFillSolidRects(
    ScrnInfoPtr pScrn,
    int	fg, int rop,
    unsigned int planemask,
    int		nBox, 		/* number of rectangles to fill */
    BoxPtr	pBox  		/* Pointer to first rectangle to fill */
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);
     while(nBox--) {
        (*infoRec->SubsequentSolidFillRect)(pScrn, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	pBox++;
     }
     SET_SYNC_FLAG(infoRec);
}




	/*********************\
	|  8x8 Mono Patterns  |
	\*********************/


void
XAAFillMono8x8PatternRectsScreenOrigin(
    ScrnInfoPtr pScrn,
    int	fg, int bg, int rop,
    unsigned int planemask,
    int	nBox,
    BoxPtr pBox,
    int pattern0, int pattern1,
    int xorigin, int yorigin
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int patx = pattern0, paty = pattern1;
    int xorg = (-xorigin) & 0x07;
    int yorg = (-yorigin) & 0x07;


    if(infoRec->Mono8x8PatternFillFlags & HARDWARE_PATTERN_PROGRAMMED_BITS) {
   	if(!(infoRec->Mono8x8PatternFillFlags & 		
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN)){
	    XAARotateMonoPattern(&patx, &paty, xorg, yorg,
				(infoRec->Mono8x8PatternFillFlags & 		
				BIT_ORDER_IN_BYTE_MSBFIRST));
	    xorg = patx; yorg = paty;
        }
    } else {
	XAACacheInfoPtr pCache =
		(*infoRec->CacheMono8x8Pattern)(pScrn, pattern0, pattern1);
	patx = pCache->x;  paty = pCache->y;
   	if(!(infoRec->Mono8x8PatternFillFlags & 
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN)){
	    int slot = (yorg << 3) + xorg;
	    patx += pCache->offsets[slot].x;
	    paty += pCache->offsets[slot].y;
	    xorg = patx;  yorg = paty;
	}	
    }

    (*infoRec->SetupForMono8x8PatternFill)(pScrn, patx, paty,
					fg, bg, rop, planemask);

     while(nBox--) {
        (*infoRec->SubsequentMono8x8PatternFillRect)(pScrn, 
			xorg, yorg, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	pBox++;
     }
     SET_SYNC_FLAG(infoRec);
}

void
XAAFillMono8x8PatternRects(
    ScrnInfoPtr pScrn,
    int	fg, int bg, int rop,
    unsigned int planemask,
    int	nBox,
    BoxPtr pBox,
    int pattern0, int pattern1,
    int xorigin, int yorigin
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int patx = pattern0, paty = pattern1;
    int xorg, yorg;
    XAACacheInfoPtr pCache = NULL;


    if(!(infoRec->Mono8x8PatternFillFlags & HARDWARE_PATTERN_PROGRAMMED_BITS)){
	pCache = (*infoRec->CacheMono8x8Pattern)(pScrn, pattern0, pattern1);
	patx = pCache->x;  paty = pCache->y;
    }


    (*infoRec->SetupForMono8x8PatternFill)(pScrn, patx, paty,
					fg, bg, rop, planemask);


     while(nBox--) {
	xorg = (pBox->x1 - xorigin) & 0x07;
	yorg = (pBox->y1 - yorigin) & 0x07;

   	if(!(infoRec->Mono8x8PatternFillFlags & 		
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN)){
	    if(infoRec->Mono8x8PatternFillFlags & 
				HARDWARE_PATTERN_PROGRAMMED_BITS) {
		patx = pattern0; paty = pattern1;
		XAARotateMonoPattern(&patx, &paty, xorg, yorg,
				(infoRec->Mono8x8PatternFillFlags & 		
				BIT_ORDER_IN_BYTE_MSBFIRST));
		xorg = patx; yorg = paty;
	    } else {
		int slot = (yorg << 3) + xorg;
	    	xorg = patx + pCache->offsets[slot].x;
	    	yorg = paty + pCache->offsets[slot].y;
	    }
        }

        (*infoRec->SubsequentMono8x8PatternFillRect)(pScrn, 
			xorg, yorg, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	pBox++;
     }

     SET_SYNC_FLAG(infoRec);
}


	/**********************\
	|  8x8 Color Patterns  |
	\**********************/


void
XAAFillColor8x8PatternRectsScreenOrigin(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorigin, int yorigin,
   XAACacheInfoPtr pCache
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int patx = pCache->x, paty = pCache->y;
    int xorg = (-xorigin) & 0x07;
    int yorg = (-yorigin) & 0x07;

    if(!(infoRec->Color8x8PatternFillFlags & 
					HARDWARE_PATTERN_PROGRAMMED_ORIGIN)){
	int slot = (yorg << 3) + xorg;
	paty += pCache->offsets[slot].y;
	patx += pCache->offsets[slot].x;
	xorg = patx;  yorg = paty;
    }	

    (*infoRec->SetupForColor8x8PatternFill)(pScrn, patx, paty,
			 rop, planemask, pCache->trans_color);

    while(nBox--) {
        (*infoRec->SubsequentColor8x8PatternFillRect)(pScrn, 
			xorg, yorg, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	pBox++;
    }
    SET_SYNC_FLAG(infoRec);
}

void
XAAFillColor8x8PatternRects(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorigin, int yorigin,
   XAACacheInfoPtr pCache
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int xorg, yorg;

    (*infoRec->SetupForColor8x8PatternFill)(pScrn, pCache->x, pCache->y,
			 rop, planemask, pCache->trans_color);

     while(nBox--) {
	xorg = (pBox->x1 - xorigin) & 0x07;
	yorg = (pBox->y1 - yorigin) & 0x07;

   	if(!(infoRec->Color8x8PatternFillFlags & 		
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN)){
	    int slot = (yorg << 3) + xorg;
	    yorg = pCache->y + pCache->offsets[slot].y;
	    xorg = pCache->x + pCache->offsets[slot].x;
        }

        (*infoRec->SubsequentColor8x8PatternFillRect)(pScrn, 
			xorg, yorg, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	pBox++;
     }

     SET_SYNC_FLAG(infoRec);
}


	/***************\
	|  Cache Blits  |
	\***************/

void 
XAAFillCacheBltRects(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   XAACacheInfoPtr pCache
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x, y, phaseY, phaseX, skipleft, height, width, w, blit_w, blit_h;

    (*infoRec->SetupForScreenToScreenCopy)(pScrn, 1, 1, rop, planemask,
		pCache->trans_color);

    while(nBox--) {
	y = pBox->y1;
	phaseY = (y - yorg) % pCache->orig_h;
	if(phaseY < 0) phaseY += pCache->orig_h;
	phaseX = (pBox->x1 - xorg) % pCache->orig_w;
	if(phaseX < 0) phaseX += pCache->orig_w;
	height = pBox->y2 - y;
	width = pBox->x2 - pBox->x1;
	
	if (rop == GXcopy) {
	    while(1) {
		w = width; skipleft = phaseX; x = pBox->x1;
		blit_h = pCache->h - phaseY;
		if(blit_h > height) blit_h = height;
	
		while(1) {
		    blit_w = pCache->w - skipleft;
		    if(blit_w > w) blit_w = w;
		    (*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			pCache->x + skipleft, pCache->y + phaseY,
			x, y, blit_w, blit_h);
		    w -= blit_w;
		    if(!w) break;
		    x += blit_w;
		    skipleft = (skipleft + blit_w) % pCache->orig_w;
		    if(blit_w >= pCache->orig_w) break;
		}

		/* Expand horizontally */
		if (w) {
		    skipleft -= phaseX;
		    if (skipleft < 0) skipleft += pCache->orig_w;
		    blit_w = x - pBox->x1 - skipleft;
		    while(w) {
			if (blit_w > w) blit_w = w;
			(*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			    pBox->x1 + skipleft, y, x, y, blit_w, blit_h);
			w -= blit_w;
			x += blit_w;
			blit_w <<= 1;
		    }
		}

		height -= blit_h;
		if(!height) break;
		y += blit_h;
		phaseY = (phaseY + blit_h) % pCache->orig_h;
		if(blit_h >= pCache->orig_h) break;
	    }

	    /* Expand vertically */
	    if (height) {
		blit_w = pBox->x2 - pBox->x1;
		phaseY -= (pBox->y1 - yorg) % pCache->orig_h;
		if (phaseY < 0) phaseY += pCache->orig_h;
		blit_h = y - pBox->y1  - phaseY;
		while(height) {
		    if (blit_h > height) blit_h = height;
		    (*infoRec->SubsequentScreenToScreenCopy)(pScrn, pBox->x1,
			pBox->y1 + phaseY, pBox->x1, y, blit_w, blit_h);
		    height -= blit_h;
		    y += blit_h;
		    blit_h <<= 1;
		}
	    }
	} else {
	    while(1) {
		w = width; skipleft = phaseX; x = pBox->x1;
		blit_h = pCache->h - phaseY;
		if(blit_h > height) blit_h = height;
	
		while(1) {
		    blit_w = pCache->w - skipleft;
		    if(blit_w > w) blit_w = w;
		    (*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			pCache->x + skipleft, pCache->y + phaseY,
			x, y, blit_w, blit_h);
		    w -= blit_w;
		    if(!w) break;
		    x += blit_w;
		    skipleft = (skipleft + blit_w) % pCache->orig_w;
		}
		height -= blit_h;
		if(!height) break;
		y += blit_h;
		phaseY = (phaseY + blit_h) % pCache->orig_h;
	    }
	}
	pBox++;
    }
    
    SET_SYNC_FLAG(infoRec);
}




	/*******************\
	|  Cache Expansion  |
	\*******************/



void 
XAAFillCacheExpandRects(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x, y, phaseY, phaseX, skipleft, height, width, w, blit_w, blit_h;
    int cacheWidth;
    XAACacheInfoPtr pCache;

    pCache = (*infoRec->CacheMonoStipple)(pScrn, pPix);

    cacheWidth = (pCache->w * pScrn->bitsPerPixel) / 
	infoRec->CacheColorExpandDensity;

    (*infoRec->SetupForScreenToScreenColorExpandFill)(pScrn, fg, bg, rop, 
							planemask);

    while(nBox--) {
	y = pBox->y1;
	phaseY = (y - yorg) % pCache->orig_h;
	if(phaseY < 0) phaseY += pCache->orig_h;
	phaseX = (pBox->x1 - xorg) % pCache->orig_w;
	if(phaseX < 0) phaseX += pCache->orig_w;
	height = pBox->y2 - y;
	width = pBox->x2 - pBox->x1;
	
	while(1) {
	    w = width; skipleft = phaseX; x = pBox->x1;
	    blit_h = pCache->h - phaseY;
	    if(blit_h > height) blit_h = height;
	
	    while(1) {
		blit_w = cacheWidth - skipleft;
		if(blit_w > w) blit_w = w;
		(*infoRec->SubsequentScreenToScreenColorExpandFill)(
			pScrn, x, y, blit_w, blit_h,
			pCache->x, pCache->y + phaseY, skipleft);
		w -= blit_w;
		if(!w) break;
		x += blit_w;
		skipleft = (skipleft + blit_w) % pCache->orig_w;
	    }
	    height -= blit_h;
	    if(!height) break;
	    y += blit_h;
	    phaseY = (phaseY + blit_h) % pCache->orig_h;
	}
	pBox++;
    }
    
    SET_SYNC_FLAG(infoRec);
}


	/******************\
	|   Image Writes   |
	\******************/



/*  This requires all LEFT_EDGE clipping.  You get too many problems
    with reading past the edge of the pattern otherwise */

static void
WriteColumn(
    ScrnInfoPtr pScrn,
    unsigned char *pSrc,
    int x, int y, int w, int h,
    int xoff, int yoff,
    int pHeight,
    int srcwidth,
    int Bpp
) {
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    unsigned char *src;
    Bool PlusOne = FALSE;
    int skipleft, dwords;

    pSrc += (Bpp * xoff);
   
    if((skipleft = (long)pSrc & 0x03L)) {
        if(Bpp == 3)
           skipleft = 4 - skipleft;
        else
           skipleft /= Bpp;

        x -= skipleft;       
        w += skipleft;
        
        if(Bpp == 3)
           pSrc -= 3 * skipleft;  
        else   /* is this Alpha friendly ? */
           pSrc = (unsigned char*)((long)pSrc & ~0x03L);     
    }

    src = pSrc + (yoff * srcwidth);

    dwords = ((w * Bpp) + 3) >> 2;

    if((infoRec->ImageWriteFlags & CPU_TRANSFER_PAD_QWORD) && 
                                                ((dwords * h) & 0x01)) {
        PlusOne = TRUE;
    } 

    (*infoRec->SubsequentImageWriteRect)(pScrn, x, y, w, h, skipleft);

    if(dwords > infoRec->ImageWriteRange) {
        while(h--) {
            XAAMoveDWORDS_FixedBase((CARD32*)infoRec->ImageWriteBase,
                (CARD32*)src, dwords);
            src += srcwidth;
	    yoff++;
	    if(yoff >= pHeight) {
		yoff = 0;
		src = pSrc;
	    }
        }
    } else {
        if(srcwidth == (dwords << 2)) {
           int maxLines = infoRec->ImageWriteRange/dwords;
	   int step;

	   while(h) {
		step = pHeight - yoff;
		if(step > maxLines) step = maxLines;
		if(step > h) step = h;

                XAAMoveDWORDS((CARD32*)infoRec->ImageWriteBase,
                        (CARD32*)src, dwords * step);

                src += (srcwidth * step);
		yoff += step;
		if(yoff >= pHeight) {
		    yoff = 0;
		    src = pSrc;
		}
                h -= step;		
	   }
        } else {
            while(h--) {
                XAAMoveDWORDS((CARD32*)infoRec->ImageWriteBase,
                        (CARD32*)src, dwords);
                src += srcwidth;
		yoff++;
		if(yoff >= pHeight) {
		    yoff = 0;
		    src = pSrc;
		}
            }
        }
    }

    if(PlusOne) {
        CARD32* base = (CARD32*)infoRec->ImageWriteBase;
        *base = 0x00000000;
    }
}

void 
XAAFillImageWriteRects(
    ScrnInfoPtr pScrn,
    int rop,
    unsigned int planemask,
    int nBox,
    BoxPtr pBox,
    int xorg, int yorg,
    PixmapPtr pPix
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x, phaseY, phaseX, height, width, blit_w;
    int pHeight = pPix->drawable.height;
    int pWidth = pPix->drawable.width;
    int Bpp = pPix->drawable.bitsPerPixel >> 3;
    int srcwidth = pPix->devKind;

    (*infoRec->SetupForImageWrite)(pScrn, rop, planemask, -1,
		pPix->drawable.bitsPerPixel, pPix->drawable.depth);

    while(nBox--) {
	x = pBox->x1;
	phaseY = (pBox->y1 - yorg) % pHeight;
	if(phaseY < 0) phaseY += pHeight;
	phaseX = (x - xorg) % pWidth;
	if(phaseX < 0) phaseX += pWidth;
	height = pBox->y2 - pBox->y1;
	width = pBox->x2 - x;
	
	while(1) {
	    blit_w = pWidth - phaseX;
	    if(blit_w > width) blit_w = width;

	    WriteColumn(pScrn, pPix->devPrivate.ptr, x, pBox->y1, 
		blit_w, height, phaseX, phaseY, pHeight, srcwidth, Bpp);

	    width -= blit_w;
	    if(!width) break;
	    x += blit_w;
	    phaseX = (phaseX + blit_w) % pWidth;
	}
	pBox++;
    }

    if(infoRec->ImageWriteFlags & SYNC_AFTER_IMAGE_WRITE)
        (*infoRec->Sync)(pScrn);
    else SET_SYNC_FLAG(infoRec);
}


	/*************\
	|  Utilities  |
	\*************/

int
XAAGetRectClipBoxes(
    RegionPtr	prgnClip,
    BoxPtr pboxClippedBase,
    int nrectFill,
    xRectangle *prectInit
){
    int 	Right, Bottom;
    BoxPtr 	pextent, pboxClipped = pboxClippedBase;
    xRectangle	*prect = prectInit;


    if (REGION_NUM_RECTS(prgnClip) == 1) {
	pextent = REGION_RECTS(prgnClip);
    	while (nrectFill--) {
	    pboxClipped->x1 = max(pextent->x1, prect->x);
	    pboxClipped->y1 = max(pextent->y1, prect->y);

	    Right = (int)prect->x + (int)prect->width;
	    pboxClipped->x2 = min(pextent->x2, Right);
    
	    Bottom = (int)prect->y + (int)prect->height;
	    pboxClipped->y2 = min(pextent->y2, Bottom);

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2)) {
		pboxClipped++;
	    }
    	}
    } else {
	pextent = REGION_EXTENTS(pGC->pScreen, prgnClip);
    	while (nrectFill--) {
	    int n;
	    BoxRec box, *pbox;
   
	    box.x1 = max(pextent->x1, prect->x);
   	    box.y1 = max(pextent->y1, prect->y);
     
	    Right = (int)prect->x + (int)prect->width;
	    box.x2 = min(pextent->x2, Right);
  
	    Bottom = (int)prect->y + (int)prect->height;
	    box.y2 = min(pextent->y2, Bottom);
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--) {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2) {
		    pboxClipped++;
		}
	    }
    	}
    }

    return(pboxClipped - pboxClippedBase);  
}

