/*
 * Acceleration for the Creator and Creator3D framebuffer - Unaccelerated stuff.
 *
 * Copyright (C) 1999 David S. Miller (davem@redhat.com)
 * Copyright (C) 1999 Jakub Jelinek   (jakub@redhat.com)
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
 * DAVID MILLER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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

/* Sorry, have to expose some CFB internals to get the stubs right. -DaveM */
struct cfb_gcstate {
	unsigned long pmask;
	int alu, rop, and, xor;
};

#define CFB_STATE_SAVE(__statep, __gcp, __privp) \
do {	(__statep)->pmask = (__gcp)->planemask; \
	(__statep)->alu = (__gcp)->alu; \
	(__statep)->rop = (__privp)->rop; \
	(__statep)->and = (__privp)->and; \
	(__statep)->xor = (__privp)->xor; \
} while(0)

#define CFB_STATE_RESTORE(__statep, __gcp, __privp) \
do {	(__gcp)->planemask = (__statep)->pmask; \
	(__gcp)->alu = (__statep)->alu; \
	(__privp)->rop = (__statep)->rop; \
	(__privp)->and = (__statep)->and; \
	(__privp)->xor = (__statep)->xor; \
} while(0)

#define CFB_STATE_SET_SFB(__gcp, __privp) \
do {	(__gcp)->alu = GXcopy; \
	(__gcp)->planemask = PMSK; \
	(__privp)->rop = GXcopy; \
	(__privp)->and = 0; \
	(__privp)->xor = (__gcp)->fgPixel; \
} while(0)

/* Stubs so we can wait for the raster processor to
 * unbusy itself before we let the cfb code write
 * directly to the framebuffer.
 */
void
CreatorSolidSpansGeneralStub (DrawablePtr pDrawable, GCPtr pGC,
			      int nInit, DDXPointPtr pptInit,
			      int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, 0xffffffff, pGC->alu);
	FFBWait(pFfb, ffb);
	cfbSolidSpansCopy(pDrawable, pGC, nInit, pptInit,
			  pwidthInit, fSorted);
}

void
CreatorSegmentSSStub (DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment *pSeg)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbSegmentSS(pDrawable, pGC, nseg, pSeg);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorLineSSStub (DrawablePtr pDrawable, GCPtr pGC,
		   int mode, int npt, DDXPointPtr ppt)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbLineSS(pDrawable, pGC, mode, npt, ppt);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorSegmentSDStub (DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment *pSeg)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbSegmentSD(pDrawable, pGC, nseg, pSeg);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorLineSDStub (DrawablePtr pDrawable, GCPtr pGC,
		   int mode, int npt, DDXPointPtr ppt)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbLineSD(pDrawable, pGC, mode, npt, ppt);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorPolyGlyphBlt8Stub (DrawablePtr pDrawable, GCPtr pGC,
			  int x, int y, unsigned int nglyph, CharInfoPtr *ppci,
			  pointer pglyphBase)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbPolyGlyphBlt8(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorImageGlyphBlt8Stub (DrawablePtr pDrawable, GCPtr pGC,
			   int x, int y, unsigned int nglyph,
			   CharInfoPtr *ppci, pointer pglyphBase)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbImageGlyphBlt8(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorTile32FSCopyStub(DrawablePtr pDrawable, GCPtr pGC,
			int nInit, DDXPointPtr pptInit,
			int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbTile32FSCopy(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorTile32FSGeneralStub(DrawablePtr pDrawable, GCPtr pGC,
			   int nInit, DDXPointPtr pptInit,
			   int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbTile32FSGeneral(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorUnnaturalTileFSStub(DrawablePtr pDrawable, GCPtr pGC,
			   int nInit, DDXPointPtr pptInit,
			   int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
Creator8Stipple32FSStub(DrawablePtr pDrawable, GCPtr pGC,
			int nInit, DDXPointPtr pptInit,
			int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfb8Stipple32FS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorUnnaturalStippleFSStub(DrawablePtr pDrawable, GCPtr pGC,
			      int nInit, DDXPointPtr pptInit,
			      int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
Creator8OpaqueStipple32FSStub(DrawablePtr pDrawable, GCPtr pGC,
			      int nInit, DDXPointPtr pptInit,
			      int *pwidthInit, int fSorted)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfb8OpaqueStipple32FS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}

void
CreatorPolyFillRectStub(DrawablePtr pDrawable, GCPtr pGC,
			int nrectFill, xRectangle *prectInit)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pDrawable->pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	struct cfb_gcstate cfb_state;
	cfbPrivGCPtr devPriv = cfbGetGCPrivate(pGC);

	FFBLOG(("STUB(%s:%d)\n", __FILE__, __LINE__));
	CFB_STATE_SAVE(&cfb_state, pGC, devPriv);
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, cfb_state.pmask, cfb_state.alu);
	FFBWait(pFfb, ffb);
	CFB_STATE_SET_SFB(pGC, devPriv);
	cfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit);
	CFB_STATE_RESTORE(&cfb_state, pGC, devPriv);
}
