/*
 * Acceleration for the Creator and Creator3D framebuffer - Get spans.
 *
 * Copyright (C) 1998,1999 Jakub Jelinek (jakub@redhat.com)
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
 */
/* $XFree86$ */

#define PSZ 32

#include "ffb.h"
#include "ffb_regs.h"
#include "ffb_rcache.h"
#include "ffb_fifo.h"

#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"

void
CreatorGetSpans(DrawablePtr pDrawable, int wMax, DDXPointPtr ppt,
		int *pwidth, int nspans, char *pchardstStart)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	unsigned int *pdst = (unsigned int *)pchardstStart;
	char *addrp = (char *) pFfb->fb;

	FFBLOG(("CreatorGetSpans: wmax(%d) nspans(%d)\n", wMax, nspans));

	/* Punt early for this case. */
	if(pDrawable->bitsPerPixel == 1) {
		mfbGetSpans(pDrawable, wMax, ppt, pwidth,
			    nspans, pchardstStart);
		return;
	}

	/* This code only works when sucking bits directly from
	 * the framebuffer.
	 */
	if(pDrawable->type != DRAWABLE_WINDOW) {
		cfbGetSpans(pDrawable, wMax, ppt, pwidth,
			    nspans, pchardstStart);
		return;
	}

	/* We're just reading pixels from SFB, and we never change
	 * the read buffer (yet) so just make sure RP is not busy.
	 */
	FFBWait(pFfb, ffb);

	if ((nspans == 1) && (*pwidth == 1)) {
		*pdst = *(unsigned int *)(addrp + (ppt->y << 13) + (ppt->x << 2));
		return;
	}

	while(nspans--) {
		int w = min(ppt->x + *pwidth, 2048) - ppt->x;
		unsigned int *psrc = (unsigned int *) (addrp +
						       (ppt->y << 13) +
						       (ppt->x << 2));
		unsigned int *pdstNext = pdst + w;

		while (w--)
			*psrc++ = *pdst++;
		pdst = pdstNext;
		ppt++;
		pwidth++;
	}
}
