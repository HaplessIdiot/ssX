/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaFillRect.c,v 1.2 1998/07/25 16:58:45 dawes Exp $ */

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


void
XAAFillSolidRects(
    ScrnInfoPtr pScrn,
    int	fg, int rop,
    unsigned int planemask,
    int		nBox, 		/* number of rectangles to fill */
    BoxPtr	pBox  		/* Pointer to first rectangle to fill */
)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);
     while(nBox--) {
        (*infoRec->SubsequentSolidFillRect)(pScrn, pBox->x1, pBox->y1,
 			pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);
	pBox++;
     }
     SET_SYNC_FLAG(infoRec);
}

	/*****************************\
	|    8x8 Mono Pattern Fills   |
	\*****************************/

void
XAAPolyFillRectMono8x8Pattern(
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
	int fg = pGC->fgPixel, bg = pGC->bgPixel;
	XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pGC->stipple);

        switch(pGC->fillStyle) {
	case FillStippled:
		bg = -1;
		break;	
	case FillOpaqueStippled:
		break;	
	case FillTiled:
		pPriv = XAA_GET_PIXMAP_PRIVATE(pGC->tile.pixmap);
		fg = pPriv->fg;		bg = pPriv->bg;
		break;
	default:
		ErrorF("Bad fillStyle %i in XAAPolyFillRectMono8x8Pattern\n",
			pGC->fillStyle);
		return;
	}
 
	(*infoRec->FillMono8x8PatternRects) (infoRec->pScrn, 
		fg, bg, pGC->alu, pGC->planemask, 
		nboxes, pClipBoxes, pPriv->pattern0, pPriv->pattern1, 
		(xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y));
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}


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

	/******************************\
	|    8x8 Color Pattern Fills   |
	\******************************/

void
XAAPolyFillRectColor8x8Pattern(
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
	XAACacheInfoPtr pCache;
        PixmapPtr pPix = pGC->stipple;

        switch(pGC->fillStyle) {
	case FillStippled:
	     pCache = (*infoRec->CacheColor8x8Pattern)(
			infoRec->pScrn, pPix, pGC->fgPixel, -1);
	     break;
	case FillOpaqueStippled:
	     pCache = (*infoRec->CacheColor8x8Pattern)(
			infoRec->pScrn, pPix, pGC->fgPixel, pGC->bgPixel);
	     break;
	case FillTiled:
	     pPix = pGC->tile.pixmap;
	     pCache = (*infoRec->CacheColor8x8Pattern)(
			infoRec->pScrn, pPix, -1, -1);
	     break;
	default:
	     ErrorF("Bad fillStyle %i in XAAPolyFillRectColor8x8Pattern\n",
			pGC->fillStyle);
	     return;
	}

	(*infoRec->FillColor8x8PatternRects) (infoRec->pScrn,
		pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
		(xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y),
		pCache);
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}

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


	/**********************\
	|   Cache Blit Fills   |
	\**********************/


void
XAAPolyFillRectCacheBlt(
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
	XAACacheInfoPtr pCache;
        PixmapPtr pPix = pGC->stipple;

        switch(pGC->fillStyle) {
	case FillStippled:
	     pCache = (*infoRec->CacheStipple)(infoRec->pScrn, pPix, 
							pGC->fgPixel, -1);
	     break;
	case FillOpaqueStippled:
	     pCache = (*infoRec->CacheStipple)(infoRec->pScrn, pPix, 
			pGC->fgPixel, pGC->bgPixel);
	     break;
	case FillTiled:
	     pPix = pGC->tile.pixmap;
	     pCache = (*infoRec->CacheTile)(infoRec->pScrn, pPix);
	     break;
	default:
	     ErrorF("Bad fillStyle %i in XAAPolyFillRectCacheBlt\n",
			pGC->fillStyle);
	     return;
	}

	(*infoRec->FillCacheBltRects) (infoRec->pScrn, pGC->alu, 
		pGC->planemask, nboxes, pClipBoxes, 
		(xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
		pCache);
   }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}

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
	
#if 0
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
		    blit_w = x - pBox->x1  + skipleft;
		    while(w) {
			if (blit_w > w) blit_w = w;
			(*infoRec->SubsequentScreenToScreenCopy)(pScrn,
			    pBox->x1 + skipleft, pBox->y1, x, y,
			    blit_w, blit_h);
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
		blit_h = y - pBox->y1  + phaseY;
		while(height) {
		    if (blit_h > height) blit_h = height;
		    (*infoRec->SubsequentScreenToScreenCopy)(pScrn, pBox->x1,
			pBox->y1 + phaseY, pBox->x1, y, blit_w, blit_h);
		    height -= blit_h;
		    y += blit_h;
		    blit_h <<= 1;
		}
	    }
	} else 
#endif
	{
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
	|  Color Expansion  |
	\*******************/

void
XAAPolyFillRectColorExpand(
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
	(*infoRec->FillColorExpandRects) (infoRec->pScrn, pGC->fgPixel,
		(pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel, 
		pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
		(xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
		pGC->stipple);
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}

	/*******************\
	|  Cache Expansion  |
	\*******************/

void
XAAPolyFillRectCacheExpand(
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
	(*infoRec->FillCacheExpandRects) (infoRec->pScrn, pGC->fgPixel,
		(pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel, 
		pGC->alu, pGC->planemask, nboxes, pClipBoxes, 
		(xorg + pGC->patOrg.x), (yorg + pGC->patOrg.y), 
		pGC->stipple);
    }
   
    if(pClipBoxes != infoRec->PreAllocBoxes)
	DEALLOCATE_LOCAL(pClipBoxes);
}


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

    cacheWidth = pCache->w * pScrn->bitsPerPixel;

    (*infoRec->SetupForScreenToScreenColorExpandCopy)(pScrn, fg, bg, rop, 
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
		(*infoRec->SubsequentScreenToScreenColorExpandCopy)(
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




int
XAAGetRectClipBoxes(
    RegionPtr	prgnClip,
    BoxPtr pboxClippedBase,
    int nrectFill,
    xRectangle *prectInit
)
{
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

