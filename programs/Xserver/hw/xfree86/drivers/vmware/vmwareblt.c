/* **********************************************************
 * Copyright (C) 1998-2001 VMware, Inc.
 * All Rights Reserved
 * **********************************************************/
#ifdef VMX86_DEVEL
char rcsId_vmwareblt[] =

    "Id: vmwareblt.c,v 1.4 2001/01/27 00:28:15 bennett Exp $";
#endif
/* $XFree86$ */

#include "X.h"
#include "cfb.h"
#include "vmware.h"

void
vmwareDoBitblt(DrawablePtr pSrc,
    DrawablePtr pDst,
    int alu, RegionPtr prgnDst, DDXPointPtr pptSrc, unsigned long planemask, unsigned long bitplane)
{
    BoxPtr pbox;
    int nbox;
    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
    DDXPointPtr pptTmp, pptNew1, pptNew2;
    VMWAREPtr pVMWARE;

    pVMWARE = VMWAREPTR(infoFromScreen(pSrc->pScreen));

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);
    pboxNew1 = NULL;
    pptNew1 = NULL;
    pboxNew2 = NULL;
    pptNew2 = NULL;
    if (pptSrc->y < pbox->y1) {
	if (nbox > 1) {
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr) ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if (!pboxNew1)
		return;
	    pptNew1 =
		(DDXPointPtr) ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if (!pptNew1) {
		DEALLOCATE_LOCAL(pboxNew1);
		return;
	    }
	    pboxBase = pboxNext = pbox + nbox - 1;
	    while (pboxBase >= pbox) {
		while ((pboxNext >= pbox) && (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
		pboxTmp = pboxNext + 1;
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
    }
    if (pptSrc->x < pbox->x1) {
	if (nbox > 1) {
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr) ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 =
		(DDXPointPtr) ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if (!pboxNew2 || !pptNew2) {
		if (pptNew2)
		    DEALLOCATE_LOCAL(pptNew2);
		if (pboxNew2)
		    DEALLOCATE_LOCAL(pboxNew2);
		if (pboxNew1) {
		    DEALLOCATE_LOCAL(pptNew1);
		    DEALLOCATE_LOCAL(pboxNew1);
		}
		return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox + nbox) {
		while ((pboxNext < pbox + nbox) &&
		    (pboxNext->y1 == pboxBase->y1)) pboxNext++;
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
    }
    /* Send the commands */
    while (nbox--) {
	vmwareWriteWordToFIFO(pVMWARE, SVGA_CMD_RECT_ROP_COPY);
	vmwareWriteWordToFIFO(pVMWARE, pptSrc->x);
	vmwareWriteWordToFIFO(pVMWARE, pptSrc->y);
	vmwareWriteWordToFIFO(pVMWARE, pbox->x1);
	vmwareWriteWordToFIFO(pVMWARE, pbox->y1);
	vmwareWriteWordToFIFO(pVMWARE, pbox->x2 - pbox->x1);
	vmwareWriteWordToFIFO(pVMWARE, pbox->y2 - pbox->y1);
	vmwareWriteWordToFIFO(pVMWARE, alu);
	pptSrc++;
	pbox++;
    }
    if (pboxNew2) {
	DEALLOCATE_LOCAL(pptNew2);
	DEALLOCATE_LOCAL(pboxNew2);
    }
    if (pboxNew1) {
	DEALLOCATE_LOCAL(pptNew1);
	DEALLOCATE_LOCAL(pboxNew1);
    }
}

RegionPtr
vmwareCopyArea(DrawablePtr pSrcDrawable,
DrawablePtr pDstDrawable,
GCPtr pGC, int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    RegionPtr prgn;
    VMWAREPtr pVMWARE = VMWAREPTR(infoFromScreen(pGC->pScreen));

    TRACEPOINT
    if ((pVMWARE->vmwareCapability & SVGA_CAP_RECT_COPY) &&
	(pGC->alu == GXcopy || (pVMWARE->vmwareCapability & SVGA_CAP_RASTER_OP)) &&
	pSrcDrawable->type == DRAWABLE_WINDOW &&
	pDstDrawable->type == DRAWABLE_WINDOW &&
	(pGC->planemask & pVMWARE->Pmsk) == pVMWARE->Pmsk) {
	void (*doBitBlt) (DrawablePtr, DrawablePtr, int, RegionPtr, DDXPointPtr, unsigned long, unsigned long);
	BoxRec updateBB;
	BoxRec mouseBB;
	Bool hidden = pVMWARE->mouseHidden;

	updateBB.x1 = pDstDrawable->x + dstx;
	updateBB.y1 = pDstDrawable->y + dsty;
	updateBB.x2 = updateBB.x1 + width;
	updateBB.y2 = updateBB.y1 + height;
	mouseBB.x1 = MIN(pSrcDrawable->x + srcx, pDstDrawable->x + dstx);
	mouseBB.y1 = MIN(pSrcDrawable->y + srcy, pDstDrawable->y + dsty);
	mouseBB.x2 = MAX(pSrcDrawable->x + srcx, pDstDrawable->x + dstx) + width;
	mouseBB.y2 = MAX(pSrcDrawable->y + srcy, pDstDrawable->y + dsty) + height;
	doBitBlt = vmwareDoBitblt;
	if (!hidden) {
	    HIDE_CURSOR_ACCEL(pVMWARE, mouseBB);
	}
	prgn =
	    cfbBitBlt(pSrcDrawable, pDstDrawable, pGC, srcx, srcy, width,
	    height, dstx, dsty, doBitBlt, 0L);
	if (!hidden) {
	    SHOW_CURSOR(pVMWARE, mouseBB);
	}
	UPDATE_ACCEL_AREA(pVMWARE, updateBB);
    } else if (pDstDrawable->type == DRAWABLE_WINDOW ||
	pSrcDrawable->type == DRAWABLE_WINDOW) {
	if (pVMWARE->vmwareBBLevel == 0) {
	    BoxRec updateBB;
	    BoxRec mouseBB;

	    if (pDstDrawable->type == DRAWABLE_WINDOW &&
		pSrcDrawable->type != DRAWABLE_WINDOW) {
		updateBB.x1 = pDstDrawable->x + dstx;
		updateBB.y1 = pDstDrawable->y + dsty;
		updateBB.x2 = updateBB.x1 + width;
		updateBB.y2 = updateBB.y1 + height;
		mouseBB = updateBB;
	    } else if (pDstDrawable->type != DRAWABLE_WINDOW &&
		       pSrcDrawable->type == DRAWABLE_WINDOW) {
		updateBB.x1 = pSrcDrawable->x + srcx;
		updateBB.y1 = pSrcDrawable->y + srcy;
		updateBB.x2 = updateBB.x1 + width;
		updateBB.y2 = updateBB.y1 + height;
		mouseBB = updateBB;
	    } else {
		updateBB.x1 = pDstDrawable->x + dstx;
		updateBB.y1 = pDstDrawable->y + dsty;
		updateBB.x2 = updateBB.x1 + width;
		updateBB.y2 = updateBB.y1 + height;
		mouseBB.x1 = MIN(pSrcDrawable->x + srcx, pDstDrawable->x + dstx);
		mouseBB.y1 = MIN(pSrcDrawable->y + srcy, pDstDrawable->y + dsty);
		mouseBB.x2 = MAX(pSrcDrawable->x + srcx, pDstDrawable->x + dstx) + width;
		mouseBB.y2 = MAX(pSrcDrawable->y + srcy, pDstDrawable->y + dsty) + height;
	    }
	    HIDE_CURSOR(pVMWARE, mouseBB);
	    vmwareWaitForFB(pVMWARE);
	    pVMWARE->vmwareBBLevel++;
	    prgn =
		GC_OPS(pGC)->CopyArea(pSrcDrawable,
		pDstDrawable, pGC, srcx, srcy, width, height, dstx, dsty);
	    pVMWARE->vmwareBBLevel--;
	    if (pDstDrawable->type == DRAWABLE_WINDOW) {
		vmwareSendSVGACmdUpdate(pVMWARE, &updateBB);
	    }
	    SHOW_CURSOR(pVMWARE, mouseBB);
	} else {
	    vmwareWaitForFB(pVMWARE);
	    prgn =
		GC_OPS(pGC)->CopyArea(pSrcDrawable,
		pDstDrawable, pGC, srcx, srcy, width, height, dstx, dsty);
	}
    } else {
	prgn =
	    GC_OPS(pGC)->CopyArea(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty);
    }
    return prgn;
}

RegionPtr
vmwareCopyPlane(DrawablePtr pSrcDrawable,
DrawablePtr pDstDrawable,
GCPtr pGC,
int srcx, int srcy, int width, int height, int dstx, int dsty, unsigned long bitPlane)
{
    RegionPtr prgn;
    VMWAREPtr pVMWARE = VMWAREPTR(infoFromScreen(pGC->pScreen));

    TRACEPOINT

    if (pDstDrawable->type == DRAWABLE_WINDOW ||
	pSrcDrawable->type == DRAWABLE_WINDOW) {
	if (pVMWARE->vmwareBBLevel == 0) {
	    BoxRec updateBB;
	    BoxRec mouseBB;

	    if (pDstDrawable->type == DRAWABLE_WINDOW &&
		pSrcDrawable->type != DRAWABLE_WINDOW) {
		updateBB.x1 = pDstDrawable->x + dstx;
		updateBB.y1 = pDstDrawable->y + dsty;
		updateBB.x2 = updateBB.x1 + width;
		updateBB.y2 = updateBB.y1 + height;
		mouseBB = updateBB;
	    } else if (pDstDrawable->type != DRAWABLE_WINDOW &&
		       pSrcDrawable->type == DRAWABLE_WINDOW) {
		updateBB.x1 = pSrcDrawable->x + srcx;
		updateBB.y1 = pSrcDrawable->y + srcy;
		updateBB.x2 = updateBB.x1 + width;
		updateBB.y2 = updateBB.y1 + height;
		mouseBB = updateBB;
	    } else {
		updateBB.x1 = pDstDrawable->x + dstx;
		updateBB.y1 = pDstDrawable->y + dsty;
		updateBB.x2 = updateBB.x1 + width;
		updateBB.y2 = updateBB.y1 + height;
		mouseBB.x1 = MIN(pSrcDrawable->x + srcx, pDstDrawable->x + dstx);
		mouseBB.y1 = MIN(pSrcDrawable->y + srcy, pDstDrawable->y + dsty);
		mouseBB.x2 = MAX(pSrcDrawable->x + srcx, pDstDrawable->x + dstx) + width;
		mouseBB.y2 = MAX(pSrcDrawable->y + srcy, pDstDrawable->y + dsty) + height;
	    }
	    HIDE_CURSOR(pVMWARE, mouseBB);
	    vmwareWaitForFB(pVMWARE);
	    pVMWARE->vmwareBBLevel++;
	    prgn =
		pVMWARE->pcfbCopyPlane(pSrcDrawable,
		pDstDrawable, pGC, srcx, srcy, width, height, dstx, dsty,
		bitPlane);
	    pVMWARE->vmwareBBLevel--;
	    if (pDstDrawable->type == DRAWABLE_WINDOW) {
		vmwareSendSVGACmdUpdate(pVMWARE, &updateBB);
	    }
	    SHOW_CURSOR(pVMWARE, mouseBB);
	} else {
	    vmwareWaitForFB(pVMWARE);
	    prgn =
		pVMWARE->pcfbCopyPlane(pSrcDrawable,
		pDstDrawable, pGC, srcx, srcy, width, height, dstx, dsty,
		bitPlane);
	}
    } else {
	prgn =
	    pVMWARE->pcfbCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
    }
    return prgn;
}

