/*
 * Acceleration for the Creator and Creator3D framebuffer - Segment rops.
 *
 * Copyright (C) 1998,1999,2000 Jakub Jelinek (jakub@redhat.com)
 * Copyright (C) 1999 David S. Miller (davem@redhat.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JAKUB JELINEK OR DAVID MILLER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
/* $XFree86$ */

#define PSZ 32

#include "ffb.h"
#include "ffb_regs.h"
#include "ffb_rcache.h"
#include "ffb_fifo.h"
#include "ffb_loops.h"
#include "ffb_stip.h"

#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "miline.h"

#undef DEBUG_LINES

/* The general strategy is to do the drawing completely in hardware
 * iff:
 * 1) Single region rect
 * 2) Both ends of the segment lie within it
 */
#define SEG_DRAWOP(__pgc) ((__pgc)->capStyle == CapNotLast ? \
			   FFB_DRAWOP_BRLINEOPEN : \
			   FFB_DRAWOP_BRLINECAP)
static void
ReloadSegmentAttrs(FFBPtr pFfb, CreatorPrivGCPtr gcPriv, GCPtr pGC)
{
	if(gcPriv->stipple == NULL) {
		FFB_WRITE_ATTRIBUTES(pFfb,
				     (FFB_PPC_FW_DISABLE|FFB_PPC_VCE_DISABLE|
				      FFB_PPC_APE_DISABLE|FFB_PPC_CS_CONST),
				     (FFB_PPC_FW_MASK|FFB_PPC_VCE_MASK|
				      FFB_PPC_APE_MASK|FFB_PPC_CS_MASK),
				     pGC->planemask,
				     FFB_ROP_EDIT_BIT | pGC->alu,
				     SEG_DRAWOP(pGC),
				     pGC->fgPixel,
				     FFB_FBC_DEFAULT);
	} else {
		ffb_fbcPtr ffb = pFfb->regs;

		FFBSetStipple(pFfb, ffb, gcPriv->stipple,
			      FFB_PPC_FW_DISABLE|FFB_PPC_VCE_DISABLE|FFB_PPC_CS_CONST,
			      FFB_PPC_FW_MASK|FFB_PPC_VCE_MASK|FFB_PPC_CS_MASK);
		FFB_WRITE_PMASK(pFfb, ffb, pGC->planemask);
		FFB_WRITE_FBC(pFfb, ffb, FFB_FBC_DEFAULT);
		FFB_WRITE_DRAWOP(pFfb, ffb, SEG_DRAWOP(pGC));
	}
}

void
CreatorPolySegment (DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment *pSeg)
{
	CreatorPrivGCPtr gcPriv = CreatorGetGCPrivate (pGC);
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pGC->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	BoxPtr extent;
	int xorg, yorg, lpat;

	if(nseg == 0)
		return;
	FFBLOG(("CreatorPolySegment: ALU(%x) PMSK(%08x) nseg(%d) lpat(%08x)\n",
		pGC->alu, pGC->planemask, nseg, gcPriv->linepat));
	if(gcPriv->stipple == NULL) {
		FFB_WRITE_ATTRIBUTES(pFfb,
				     FFB_PPC_FW_DISABLE|FFB_PPC_VCE_DISABLE|FFB_PPC_APE_DISABLE|FFB_PPC_CS_CONST,
				     FFB_PPC_FW_MASK|FFB_PPC_VCE_MASK|FFB_PPC_APE_MASK|FFB_PPC_CS_MASK,
				     pGC->planemask,
				     FFB_ROP_EDIT_BIT | pGC->alu,
				     SEG_DRAWOP(pGC),
				     pGC->fgPixel,
				     FFB_FBC_DEFAULT);
	} else {
		FFBSetStipple(pFfb, ffb, gcPriv->stipple,
			      FFB_PPC_FW_DISABLE|FFB_PPC_VCE_DISABLE|FFB_PPC_CS_CONST,
			      FFB_PPC_FW_MASK|FFB_PPC_VCE_MASK|FFB_PPC_CS_MASK);
		FFB_WRITE_PMASK(pFfb, ffb, pGC->planemask);
		FFB_WRITE_FBC(pFfb, ffb, FFB_FBC_DEFAULT);
		FFB_WRITE_DRAWOP(pFfb, ffb, SEG_DRAWOP(pGC));
	}

	pFfb->rp_active = 1;
	xorg = pDrawable->x;
	yorg = pDrawable->y;
	extent = REGION_RECTS(cfbGetCompositeClip(pGC));
	lpat = gcPriv->linepat;
	if (lpat == 0) {
		FFBFifo(pFfb, 1);
		ffb->lpat = 0;
		while(nseg--) {
			register int x1 = pSeg->x1 + xorg;
			register int y1 = pSeg->y1 + yorg;
			register int x2 = pSeg->x2 + xorg;
			register int y2 = pSeg->y2 + yorg;

			if (x1 >= extent->x1	&&	x2 >= extent->x1	&&
			    x1 < extent->x2	&&	x2 < extent->x2		&&
			    y1 >= extent->y1	&&	y2 >= extent->y1	&&
			    y1 < extent->y2	&&	y2 < extent->y2) {
				if (pFfb->has_brline_bug) {
					FFBFifo(pFfb, 5);
					ffb->ppc = 0;
					FFB_WRITE64(&ffb->by, y1, x1);
					FFB_WRITE64_2(&ffb->bh, y2, x2);
				} else {
					FFBFifo(pFfb, 4);
					FFB_WRITE64(&ffb->by, y1, x1);
					FFB_WRITE64_2(&ffb->bh, y2, x2);
				}
			} else {
				gcPriv->PolySegment(pDrawable, pGC, 1, pSeg);
				ReloadSegmentAttrs(pFfb, gcPriv, pGC);
				pFfb->rp_active = 1;
			}
			pSeg++;
		}
	} else {
		while(nseg--) {
			register int x1 = pSeg->x1 + xorg;
			register int y1 = pSeg->y1 + yorg;
			register int x2 = pSeg->x2 + xorg;
			register int y2 = pSeg->y2 + yorg;

			if (x1 >= extent->x1	&&	x2 >= extent->x1	&&
			    x1 < extent->x2	&&	x2 < extent->x2		&&
			    y1 >= extent->y1	&&	y2 >= extent->y1	&&
			    y1 < extent->y2	&&	y2 < extent->y2) {
				FFBFifo(pFfb, 5);
				ffb->lpat = lpat;
				FFB_WRITE64(&ffb->by, y1, x1);
				FFB_WRITE64_2(&ffb->bh, y2, x2);
			} else {
				gcPriv->PolySegment(pDrawable, pGC, 1, pSeg);
				ReloadSegmentAttrs(pFfb, gcPriv, pGC);
				pFfb->rp_active = 1;
			}
			pSeg++;
		}
	}

	pFfb->rp_active = 1;
	FFBSync(pFfb, ffb);
}
