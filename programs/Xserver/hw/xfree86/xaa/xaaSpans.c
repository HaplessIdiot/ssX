/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaSpans.c,v 1.2 1998/07/25 16:58:51 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "xf86str.h"
#include "mi.h"
#include "mispans.h"
#include "xaa.h"
#include "xaalocal.h"


	/*********************\
	|     Solid Spans     |
	\*********************/

void
XAAFillSpansSolid(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,		/* number of spans to fill */
    DDXPointPtr pptInit,	/* pointer to list of start points */
    int *pwidthInit,		/* pointer to list of n widths */
    int fSorted )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(pGC->pCompositeClip);
    if(!n) return;

    if(n > infoRec->NumPreAllocDDXPointRecs) {
	ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!ppt) return;	
    } else ppt = infoRec->PreAllocDDXPointRecs;


    if(n > infoRec->NumPreAllocInts) {
    	pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	if(!pwidth) {
	   if(ppt != infoRec->PreAllocDDXPointRecs)
		DEALLOCATE_LOCAL(ppt);
	   return;
	}	
    } else pwidth = infoRec->PreAllocInts;

    n = miClipSpans( pGC->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (n) {
	(*infoRec->FillSolidSpans) (infoRec->pScrn, pGC->fgPixel, 
		pGC->alu, pGC->planemask, n, ppt, pwidth, fSorted);    
    }

    if(ppt != infoRec->PreAllocDDXPointRecs)
	DEALLOCATE_LOCAL(ppt);
    if(pwidth != infoRec->PreAllocInts)
    	DEALLOCATE_LOCAL(pwidth);
}


void 
XAAFillSolidSpans(
   ScrnInfoPtr pScrn,
   int fg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted 
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);
     while(n--) {
        (*infoRec->SubsequentSolidFillRect)(pScrn, ppt->x, ppt->y, *pwidth, 1);
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}

	/********************\
	|   Mono 8x8 Spans   |
	\********************/


void
XAAFillSpansMono8x8Pattern(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,		/* number of spans to fill */
    DDXPointPtr pptInit,	/* pointer to list of start points */
    int *pwidthInit,		/* pointer to list of n widths */
    int fSorted )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(pGC->pCompositeClip);
    if(!n) return;

    if(n > infoRec->NumPreAllocDDXPointRecs) {
	ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!ppt) return;	
    } else ppt = infoRec->PreAllocDDXPointRecs;


    if(n > infoRec->NumPreAllocInts) {
    	pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	if(!pwidth) {
	   if(ppt != infoRec->PreAllocDDXPointRecs)
		DEALLOCATE_LOCAL(ppt);
	   return;
	}	
    } else pwidth = infoRec->PreAllocInts;

    n = miClipSpans( pGC->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (n) {
	int fg = pGC->fgPixel, bg = pGC->bgPixel;
	XAAPixmapPtr pPriv = XAA_GET_PIXMAP_PRIVATE(pGC->stipple);

        switch(pGC->fillStyle) {
	case FillStippled:
		bg = -1;
		break;	
	case FillOpaqueStippled:
		break;	
	default:	/* FillTiled */	
		pPriv = XAA_GET_PIXMAP_PRIVATE(pGC->tile.pixmap);
		fg = pPriv->fg;		bg = pPriv->bg;
		break;
	}
 
	(*infoRec->FillMono8x8PatternSpans) (infoRec->pScrn, 
		fg, bg, pGC->alu, pGC->planemask, 
		n, ppt, pwidth, fSorted, pPriv->pattern0, pPriv->pattern1, 
		(pDrawable->x + pGC->patOrg.x), (pDrawable->y + pGC->patOrg.y));
    }

    if(ppt != infoRec->PreAllocDDXPointRecs)
	DEALLOCATE_LOCAL(ppt);
    if(pwidth != infoRec->PreAllocInts)
    	DEALLOCATE_LOCAL(pwidth);
}


void 
XAAFillMono8x8PatternSpansScreenOrigin(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   int pattern0, int pattern1,
   int xorigin, int yorigin 
){
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

     while(n--) {
        (*infoRec->SubsequentMono8x8PatternFillRect)(pScrn, 
			xorg, yorg, ppt->x, ppt->y, *pwidth, 1);
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}


void 
XAAFillMono8x8PatternSpans(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   int pattern0, int pattern1,
   int xorigin, int yorigin 
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int patx = pattern0, paty = pattern1;
    int xorg, yorg, slot;
    XAACacheInfoPtr pCache;


    if(!(infoRec->Mono8x8PatternFillFlags & HARDWARE_PATTERN_PROGRAMMED_BITS)){
	pCache = (*infoRec->CacheMono8x8Pattern)(pScrn, pattern0, pattern1);
	patx = pCache->x;  paty = pCache->y;
    }


    (*infoRec->SetupForMono8x8PatternFill)(pScrn, patx, paty,
					fg, bg, rop, planemask);

     while(n--) {
	xorg = (ppt->x - xorigin) & 0x07;
	yorg = (ppt->y - yorigin) & 0x07;

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
		slot = (yorg << 3) + xorg;
	    	xorg = patx + pCache->offsets[slot].x;
	    	yorg = paty + pCache->offsets[slot].y;
	    }
        }

        (*infoRec->SubsequentMono8x8PatternFillRect)(pScrn, 
			xorg, yorg, ppt->x, ppt->y, *pwidth, 1);
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}



	/*********************\
	|   Color 8x8 Spans   |
	\*********************/


void
XAAFillSpansColor8x8Pattern(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,		/* number of spans to fill */
    DDXPointPtr pptInit,	/* pointer to list of start points */
    int *pwidthInit,		/* pointer to list of n widths */
    int fSorted )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(pGC->pCompositeClip);
    if(!n) return;

    if(n > infoRec->NumPreAllocDDXPointRecs) {
	ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!ppt) return;	
    } else ppt = infoRec->PreAllocDDXPointRecs;


    if(n > infoRec->NumPreAllocInts) {
    	pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	if(!pwidth) {
	   if(ppt != infoRec->PreAllocDDXPointRecs)
		DEALLOCATE_LOCAL(ppt);
	   return;
	}	
    } else pwidth = infoRec->PreAllocInts;

    n = miClipSpans( pGC->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (n) {
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
	     ErrorF("Bad fillStyle %i in XAAFillSpansColor8x8Pattern\n",
			pGC->fillStyle);
	     return;
	}
 
	(*infoRec->FillColor8x8PatternSpans) (infoRec->pScrn, 
		pGC->alu, pGC->planemask, n, ppt, pwidth, fSorted, pCache,
		(pDrawable->x + pGC->patOrg.x), (pDrawable->y + pGC->patOrg.y));
    }

    if(ppt != infoRec->PreAllocDDXPointRecs)
	DEALLOCATE_LOCAL(ppt);
    if(pwidth != infoRec->PreAllocInts)
    	DEALLOCATE_LOCAL(pwidth);
}

void 
XAAFillColor8x8PatternSpansScreenOrigin(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   XAACacheInfoPtr pCache,
   int xorigin, int yorigin 
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

     while(n--) {
        (*infoRec->SubsequentColor8x8PatternFillRect)(pScrn, 
			xorg, yorg, ppt->x, ppt->y, *pwidth, 1);
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}


void 
XAAFillColor8x8PatternSpans(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted,
   XAACacheInfoPtr pCache,
   int xorigin, int yorigin 
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int xorg, yorg, slot;

    (*infoRec->SetupForColor8x8PatternFill)(pScrn, pCache->x, pCache->y,
			 rop, planemask, pCache->trans_color);

     while(n--) {
	xorg = (ppt->x - xorigin) & 0x07;
	yorg = (ppt->y - yorigin) & 0x07;

   	if(!(infoRec->Color8x8PatternFillFlags & 		
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN)){
	    slot = (yorg << 3) + xorg;
	    yorg = pCache->y + pCache->offsets[slot].y;
	    xorg = pCache->x + pCache->offsets[slot].x;
        }

        (*infoRec->SubsequentColor8x8PatternFillRect)(pScrn, 
			xorg, yorg, ppt->x, ppt->y, *pwidth, 1);
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}

	/**********************\
	|   Cache Blit Spans   |
	\**********************/


void
XAAFillSpansCacheBlt(
    DrawablePtr pDrawable,
    GC		*pGC,
    int		nInit,		/* number of spans to fill */
    DDXPointPtr pptInit,	/* pointer to list of start points */
    int *pwidthInit,		/* pointer to list of n widths */
    int fSorted )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(pGC->pCompositeClip);
    if(!n) return;

    if(n > infoRec->NumPreAllocDDXPointRecs) {
	ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!ppt) return;	
    } else ppt = infoRec->PreAllocDDXPointRecs;


    if(n > infoRec->NumPreAllocInts) {
    	pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	if(!pwidth) {
	   if(ppt != infoRec->PreAllocDDXPointRecs)
		DEALLOCATE_LOCAL(ppt);
	   return;
	}	
    } else pwidth = infoRec->PreAllocInts;

    n = miClipSpans( pGC->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (n) {
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
	default:	/* FillTiled */	
	     pPix = pGC->tile.pixmap;
	     pCache = (*infoRec->CacheTile)(infoRec->pScrn, pPix);
	     break;
	}
 
	(*infoRec->FillCacheBltSpans) (infoRec->pScrn, 
		pGC->alu, pGC->planemask, n, ppt, pwidth, fSorted, pCache, 
		(pDrawable->x + pGC->patOrg.x), (pDrawable->y + pGC->patOrg.y));
    }

    if(ppt != infoRec->PreAllocDDXPointRecs)
	DEALLOCATE_LOCAL(ppt);
    if(pwidth != infoRec->PreAllocInts)
    	DEALLOCATE_LOCAL(pwidth);
}

void 
XAAFillCacheBltSpans(
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   XAACacheInfoPtr pCache,
   int xorg, int yorg
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x, w, phaseX, phaseY, blit_w;  

    (*infoRec->SetupForScreenToScreenCopy)(pScrn, 1, 1, rop, planemask,
		pCache->trans_color);

     while(n--) {
	x = ppt->x;
	w = *pwidth; 
	phaseX = (x - xorg) % pCache->orig_w;
	if(phaseX < 0) phaseX += pCache->orig_w;
	phaseY = (ppt->y - yorg) % pCache->orig_h;
	if(phaseY < 0) phaseY += pCache->orig_h;

	while(1) {
	    blit_w = pCache->w - phaseX;
	    if(blit_w > w) blit_w = w;

            (*infoRec->SubsequentScreenToScreenCopy)(pScrn, 
		pCache->x + phaseX, pCache->y + phaseY,
		x, ppt->y, blit_w, 1);

	    w -= blit_w;
	    if(!w) break;
	    x += blit_w;
	    phaseX = (phaseX + blit_w) % pCache->orig_w;
	}
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}

	/*******************\
	|  Color Expansion  |
	\*******************/

void
XAAFillSpansColorExpand(
    DrawablePtr pDraw,
    GC		*pGC,
    int		nInit,		/* number of spans to fill */
    DDXPointPtr pptInit,	/* pointer to list of start points */
    int *pwidthInit,		/* pointer to list of n widths */
    int fSorted )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(pGC->pCompositeClip);
    if(!n) return;

    if(n > infoRec->NumPreAllocDDXPointRecs) {
	ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!ppt) return;	
    } else ppt = infoRec->PreAllocDDXPointRecs;


    if(n > infoRec->NumPreAllocInts) {
    	pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	if(!pwidth) {
	   if(ppt != infoRec->PreAllocDDXPointRecs)
		DEALLOCATE_LOCAL(ppt);
	   return;
	}	
    } else pwidth = infoRec->PreAllocInts;

    n = miClipSpans( pGC->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (n) {
	(*infoRec->FillColorExpandSpans) (infoRec->pScrn, pGC->fgPixel,
		(pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel, 
		pGC->alu, pGC->planemask, n, ppt, pwidth, fSorted,   
		(pDraw->x + pGC->patOrg.x), (pDraw->y + pGC->patOrg.y),
		pGC->stipple); 
    }

    if(ppt != infoRec->PreAllocDDXPointRecs)
	DEALLOCATE_LOCAL(ppt);
    if(pwidth != infoRec->PreAllocInts)
    	DEALLOCATE_LOCAL(pwidth);
}


	/****************\
	|  Cache Expand  |
	\****************/

void
XAAFillSpansCacheExpand(
    DrawablePtr pDraw,
    GC		*pGC,
    int		nInit,		/* number of spans to fill */
    DDXPointPtr pptInit,	/* pointer to list of start points */
    int *pwidthInit,		/* pointer to list of n widths */
    int fSorted )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(pGC->pCompositeClip);
    if(!n) return;

    if(n > infoRec->NumPreAllocDDXPointRecs) {
	ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!ppt) return;	
    } else ppt = infoRec->PreAllocDDXPointRecs;


    if(n > infoRec->NumPreAllocInts) {
    	pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	if(!pwidth) {
	   if(ppt != infoRec->PreAllocDDXPointRecs)
		DEALLOCATE_LOCAL(ppt);
	   return;
	}	
    } else pwidth = infoRec->PreAllocInts;

    n = miClipSpans( pGC->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (n) {
	(*infoRec->FillCacheExpandSpans) (infoRec->pScrn, pGC->fgPixel,
		(pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel, 
		pGC->alu, pGC->planemask, n, ppt, pwidth, fSorted,   
		(pDraw->x + pGC->patOrg.x), (pDraw->y + pGC->patOrg.y),
		pGC->stipple); 
    }

    if(ppt != infoRec->PreAllocDDXPointRecs)
	DEALLOCATE_LOCAL(ppt);
    if(pwidth != infoRec->PreAllocInts)
    	DEALLOCATE_LOCAL(pwidth);
}



void 
XAAFillCacheExpandSpans(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth,
   int fSorted,
   int xorg, int yorg,
   PixmapPtr pPix
){
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int x, w, phaseX, phaseY, blit_w, cacheWidth;  
    XAACacheInfoPtr pCache;

    pCache = (*infoRec->CacheMonoStipple)(pScrn, pPix);

    cacheWidth = pCache->w * pScrn->bitsPerPixel;

    (*infoRec->SetupForScreenToScreenColorExpandCopy)(pScrn, fg, bg, rop, 
							planemask);

     while(n--) {
	x = ppt->x;
	w = *pwidth; 
	phaseX = (x - xorg) % pCache->orig_w;
	if(phaseX < 0) phaseX += pCache->orig_w;
	phaseY = (ppt->y - yorg) % pCache->orig_h;
	if(phaseY < 0) phaseY += pCache->orig_h;

	while(1) {
	    blit_w = cacheWidth - phaseX;
	    if(blit_w > w) blit_w = w;

	    (*infoRec->SubsequentScreenToScreenColorExpandCopy)(
			pScrn, x, ppt->y, blit_w, 1,
			pCache->x, pCache->y + phaseY, phaseX);

	    w -= blit_w;
	    if(!w) break;
	    x += blit_w;
	    phaseX = (phaseX + blit_w) % pCache->orig_w;
	}
	ppt++; pwidth++;
     }
     SET_SYNC_FLAG(infoRec);
}
