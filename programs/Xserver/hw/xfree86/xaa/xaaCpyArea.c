/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaCpyArea.c,v 1.1.2.3 1998/07/19 13:22:10 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "migc.h"
#include "gcstruct.h"
#include "pixmapstr.h"

/*
  Written mostly by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
 */


RegionPtr
XAACopyArea(
    DrawablePtr pSrcDrawable,
    DrawablePtr pDstDrawable,
    GC *pGC,
    int srcx, int srcy,
    int width, int height,
    int dstx, int dsty )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

    if(pDstDrawable->type == DRAWABLE_WINDOW) {
	if(pSrcDrawable->type == DRAWABLE_WINDOW) {
	    if(infoRec->ScreenToScreenBitBlt &&
	     CHECK_ROP(pGC,infoRec->ScreenToScreenBitBltFlags) &&
	     CHECK_PLANEMASK(pGC,infoRec->ScreenToScreenBitBltFlags))
            return (XAABitBlt( pSrcDrawable, pDstDrawable,
		pGC, srcx, srcy, width, height, dstx, dsty,
		XAADoBitBlt, 0L));
	} else {
	    if(infoRec->WritePixmap &&
	     CHECK_ROP(pGC,infoRec->WritePixmapFlags) &&
	     CHECK_PLANEMASK(pGC,infoRec->WritePixmapFlags) &&
	     !((infoRec->ImageWriteFlags & NO_GXCOPY) && 
	     (pGC->alu == GXcopy))) 
            return (XAABitBlt( pSrcDrawable, pDstDrawable,
		pGC, srcx, srcy, width, height, dstx, dsty,
		XAADoImageWrite, 0L));
	}
    } else {
    }

    return (XAAFallbackOps.CopyArea(pSrcDrawable, pDstDrawable, pGC,
   	    srcx, srcy, width, height, dstx, dsty));
}


void
XAADoBitBlt(
    DrawablePtr	    pSrc, 
    DrawablePtr	    pDst,
    GC		    *pGC,
    RegionPtr	    prgnDst,
    DDXPointPtr	    pptSrc )
{
    int nbox, careful;
    BoxPtr pbox, pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
    DDXPointPtr pptTmp, pptNew1, pptNew2;
    int xdir, ydir;
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);

    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
    careful = 1;

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);

    pboxNew1 = NULL;
    pptNew1 = NULL;
    pboxNew2 = NULL;
    pptNew2 = NULL;
    if (careful && (pptSrc->y < pbox->y1)) {
        /* walk source botttom to top */
	ydir = -1;

	if (nbox > 1) {
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNew1)
		return;
	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNew1) {
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox) {
	        while ((pboxNext >= pbox) &&
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase) {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
	    pptNew1 -= nbox;
	    pptSrc = pptNew1;
        }
    } else {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (careful && (pptSrc->x < pbox->x1)) {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1) {
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew2 || !pptNew2) {
		if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
		if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
		if (pboxNew1) {
		    DEALLOCATE_LOCAL(pptNew1);
		    DEALLOCATE_LOCAL(pboxNew1);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox) {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase) {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	    pptNew2 -= nbox;
	    pptSrc = pptNew2;
	}
    } else {
	/* walk source left to right */
        xdir = 1;
    }

    (*infoRec->ScreenToScreenBitBlt)(infoRec->pScrn, nbox, pptSrc, pbox, 
	xdir, ydir, pGC->alu, pGC->planemask);
 
    if (pboxNew2) {
	DEALLOCATE_LOCAL(pptNew2);
	DEALLOCATE_LOCAL(pboxNew2);
    }
    if (pboxNew1) {
	DEALLOCATE_LOCAL(pptNew1);
	DEALLOCATE_LOCAL(pboxNew1);
    }

}

void
XAADoImageWrite(
    DrawablePtr	    pSrc, 
    DrawablePtr	    pDst,
    GC		    *pGC,
    RegionPtr	    prgnDst,
    DDXPointPtr	    pptSrc )
{
    int srcwidth;
    unsigned char* psrcBase;			/* start of image */
    unsigned char* srcPntr;			/* index into the image */
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_GC(pGC);
    int Bpp = pSrc->bitsPerPixel >> 3; 

    psrcBase = (unsigned char *)((PixmapPtr)pSrc)->devPrivate.ptr;
    srcwidth = (int)((PixmapPtr)pSrc)->devKind;

    for(; nbox; pbox++, pptSrc++, nbox--) {
        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * Bpp);

	(*infoRec->WritePixmap)(infoRec->pScrn, pbox->x1, pbox->y1, 
		pbox->x2 - pbox->x1, pbox->y2 - pbox->y1, srcPntr, srcwidth,
		pGC->alu, pGC->planemask, -1, pSrc->bitsPerPixel, pSrc->depth);
    }
}

void 
XAAScreenToScreenBitBlt(
    ScrnInfoPtr pScrn,
    int nbox,
    DDXPointPtr pptSrc,
    BoxPtr pbox,
    int xdir, int ydir,
    int alu,
    unsigned int planemask )
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);

    (*infoRec->SetupForScreenToScreenCopy)(pScrn, xdir, ydir, 
		alu, planemask, -1);

    for(; nbox; pbox++, pptSrc++, nbox--) {
	(*infoRec->SubsequentScreenToScreenCopy)(pScrn, pptSrc->x, pptSrc->y,
		pbox->x1, pbox->y1, pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
    }

    SET_SYNC_FLAG(infoRec);
}
