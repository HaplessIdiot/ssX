/*
 * Acceleration for the Creator and Creator3D framebuffer - Point rops.
 *
 * Copyright (C) 1999 Jakub Jelinek (jakub@redhat.com)
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

#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"

void
CreatorPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode,
		 int npt, xPoint *pptInit)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pGC->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	RegionPtr clip;
	int numRects;
	register int off, c1, c2;
	register char *addrp;
	register int *ppt, pt, i;
	BoxPtr pbox;
	xPoint *pptPrev;

	FFBLOG(("CreatorPolyPoint: ALU(%x) PMSK(%08x) mode(%d) npt(%d)\n",
		pGC->alu, pGC->planemask, mode, npt));
	if (pGC->alu == GXnoop)
		return;

	clip = cfbGetCompositeClip(pGC);
	numRects = REGION_NUM_RECTS(clip);
	off = *(int *)&pDrawable->x;
	off -= (off & 0x8000) << 1;
	if (mode == CoordModePrevious && npt > 1) {
		for (pptPrev = pptInit + 1, i = npt - 1; --i >= 0; pptPrev++) {
			pptPrev->x += pptPrev[-1].x;
			pptPrev->y += pptPrev[-1].y;
		}
	}
	{
		unsigned int ppc = FFB_PPC_FW_DISABLE|FFB_PPC_VCE_DISABLE|FFB_PPC_APE_DISABLE|FFB_PPC_CS_CONST;
		unsigned int ppc_mask = FFB_PPC_FW_MASK|FFB_PPC_VCE_MASK|FFB_PPC_APE_MASK|FFB_PPC_CS_MASK;
		unsigned int fg = pGC->fgPixel;
		unsigned int fbc = FFB_FBC_DEFAULT;
		unsigned int rop = FFB_ROP_EDIT_BIT | pGC->alu;
		unsigned int pmask = pGC->planemask;

		if((pFfb->ppc_cache & ppc_mask) != ppc ||
		   pFfb->fg_cache != fg ||
		   pFfb->fbc_cache != fbc ||
		   pFfb->rop_cache != rop ||
		   pFfb->pmask_cache != pmask) {
			pFfb->ppc_cache &= ~ppc_mask;
			pFfb->ppc_cache |= ppc;
			pFfb->fg_cache = fg;
			pFfb->fbc_cache = fbc;
			pFfb->rop_cache = rop;
			pFfb->pmask_cache = pmask;
			pFfb->rp_active = 1;
			FFBFifo(pFfb, 5);
			ffb->ppc = ppc;
			ffb->fg = fg;
			ffb->fbc = fbc;
			ffb->rop = rop;
			ffb->pmask = pmask;
		}
	}
	FFBWait(pFfb, ffb);

	addrp = (char *)pFfb->fb + (pDrawable->y << 13) + (pDrawable->x << 2);
	pbox = REGION_RECTS(clip);
	while (numRects--) {
		c1 = *(int *)&pbox->x1 - off;
		c2 = *(int *)&pbox->x2 - off - 0x00010001;
		for (ppt = (int *)pptInit, i = npt; --i >= 0; ) {
			pt = *ppt++;
			if (!(((pt - c1) | (c2 - pt)) & 0x80008000))
				*(unsigned int *)(addrp + ((pt << 13) & 0xffe000) + ((pt >> 14) & 0x1ffc)) = 0;
		}
		pbox++;
	}
}
