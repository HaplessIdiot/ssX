/*
 * Acceleration for the Creator and Creator3D framebuffer.
 *
 * Copyright (C) 1998,1999,2000 Jakub Jelinek (jakub@redhat.com)
 * Copyright (C) 1998 Michal Rehacek (majkl@iname.com)
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
 * JAKUB JELINEK, MICHAL REHACEK, OR DAVID MILLER BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/* $XFree86$ */

#define	PSZ	32
#include	<asm/types.h>
#include	<math.h>


#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mistruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#include	"mibstore.h"
#include	"mifillarc.h"
#include	"miwideline.h"
#include	"miline.h"
#include	"fastblt.h"
#include	"mergerop.h"
#include	"migc.h"
#include	"mi.h"

#include	"ffb.h"
#include	"ffb_fifo.h"
#include	"ffb_rcache.h"
#include	"ffb_loops.h"
#include	"ffb_regs.h"
#include	"ffb_stip.h"
#include 	"ffb_gc.h"

int	CreatorScreenPrivateIndex;
int	CreatorGCPrivateIndex;
int	CreatorWindowPrivateIndex;
int	CreatorGeneration;

/* Indexed by ffb resolution enum. */
struct fastfill_parms ffb_fastfill_parms[] = {
	/* fsmall, psmall,  ffh,  ffw,  pfh,  pfw */
	{  0x00c0, 0x1400, 0x04, 0x08, 0x10, 0x50 },	/* Standard: 1280 x 1024 */
	{  0x0140, 0x2800, 0x04, 0x10, 0x10, 0xa0 },	/* High:     1920 x 1360 */
	{  0x0080, 0x0a00, 0x02, 0x08, 0x08, 0x50 },	/* Stereo:   960  x 580  */
/*XXX*/	{  0x00c0, 0x0a00, 0x04, 0x08, 0x08, 0x50 },	/* Portrait: 1280 x 2048 XXX */
};

static Bool
CreatorCreateWindow (WindowPtr pWin)
{
	if (!cfbCreateWindow (pWin))
		return FALSE;
	pWin->devPrivates[CreatorWindowPrivateIndex].ptr = 0;
	return TRUE;
}

static Bool
CreatorDestroyWindow (WindowPtr pWin)
{
	CreatorStipplePtr stipple;
	if ((stipple = CreatorGetWindowPrivate(pWin)))
		xfree (stipple);
	return cfbDestroyWindow (pWin);
}

extern CreatorStipplePtr FFB_tmpStipple;

static int
CreatorChangeWindowAttributes (WindowPtr pWin, Mask mask)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN(pWin->drawable.pScreen);
	CreatorStipplePtr stipple;
	Mask index;
	WindowPtr pBgWin;
	register cfbPrivWin *pPrivWin;
	int width;

	FFBLOG(("CreatorChangeWindowAttributes: WIN(%p) mask(%08x)\n", pWin, mask));
	pPrivWin = (cfbPrivWin *)(pWin->devPrivates[cfbWindowPrivateIndex].ptr);
	/*
	 * When background state changes from ParentRelative and
	 * we had previously rotated the fast border pixmap to match
	 * the parent relative origin, rerotate to match window
	 */
	if (mask & (CWBackPixmap | CWBackPixel) &&
	    pWin->backgroundState != ParentRelative &&
	    pPrivWin->fastBorder &&
	    (pPrivWin->oldRotate.x != pWin->drawable.x ||
	     pPrivWin->oldRotate.y != pWin->drawable.y)) {
		cfbXRotatePixmap(pPrivWin->pRotatedBorder,
				 pWin->drawable.x - pPrivWin->oldRotate.x);
		cfbYRotatePixmap(pPrivWin->pRotatedBorder,
				 pWin->drawable.y - pPrivWin->oldRotate.y);
		pPrivWin->oldRotate.x = pWin->drawable.x;
		pPrivWin->oldRotate.y = pWin->drawable.y;
	}
	while (mask) {
		index = lowbit(mask);
		mask &= ~index;
		switch (index) {
		case CWBackPixmap:
			stipple = CreatorGetWindowPrivate(pWin);
			if (pWin->backgroundState == None ||
			    pWin->backgroundState == ParentRelative) {
				pPrivWin->fastBackground = FALSE;
				if (stipple) {
					xfree (stipple);
					CreatorSetWindowPrivate(pWin,0);
				}
				/* Rotate border to match parent origin */
				if (pWin->backgroundState == ParentRelative &&
				    pPrivWin->pRotatedBorder)  {
					for (pBgWin = pWin->parent;
					     pBgWin->backgroundState == ParentRelative;
					     pBgWin = pBgWin->parent);
					cfbXRotatePixmap(pPrivWin->pRotatedBorder,
							 pBgWin->drawable.x - pPrivWin->oldRotate.x);
					cfbYRotatePixmap(pPrivWin->pRotatedBorder,
							 pBgWin->drawable.y - pPrivWin->oldRotate.y);
					pPrivWin->oldRotate.x = pBgWin->drawable.x;
					pPrivWin->oldRotate.y = pBgWin->drawable.y;
				}
				break;
			}
			if (!stipple) {
				if (!FFB_tmpStipple)
					FFB_tmpStipple = (CreatorStipplePtr)
						xalloc (sizeof *FFB_tmpStipple);
				stipple = FFB_tmpStipple;
			}
			if (stipple) {
				int ph = FFB_FFPARMS(pFfb).pagefill_height;

				if(CreatorCheckTile (pWin->background.pixmap, stipple,
						     ((DrawablePtr)pWin)->x & 31,
						     ((DrawablePtr)pWin)->y & 31, ph)) {
					stipple->alu = GXcopy;
					pPrivWin->fastBackground = FALSE;
					if (stipple == FFB_tmpStipple) {
						CreatorSetWindowPrivate(pWin, stipple);
						FFB_tmpStipple = 0;
					}
					break;
				}
			}
			if ((stipple = CreatorGetWindowPrivate(pWin))) {
				xfree (stipple);
				CreatorSetWindowPrivate(pWin,0);
			}
			if (((width = (pWin->background.pixmap->drawable.width * PSZ)) <= 32) &&
			    !(width & (width - 1))) {
				cfbCopyRotatePixmap(pWin->background.pixmap,
						    &pPrivWin->pRotatedBackground,
						    pWin->drawable.x,
						    pWin->drawable.y);
				if (pPrivWin->pRotatedBackground) {
					pPrivWin->fastBackground = TRUE;
					pPrivWin->oldRotate.x = pWin->drawable.x;
					pPrivWin->oldRotate.y = pWin->drawable.y;
				} else
					pPrivWin->fastBackground = FALSE;
				break;
			}
			pPrivWin->fastBackground = FALSE;
			break;

		case CWBackPixel:
			pPrivWin->fastBackground = FALSE;
			break;

		case CWBorderPixmap:
			/* don't bother with accelerator for border tiles (just lazy) */
			if (((width = (pWin->border.pixmap->drawable.width * PSZ)) <= 32) &&
			    !(width & (width - 1))) {
				for (pBgWin = pWin;
				     pBgWin->backgroundState == ParentRelative;
				     pBgWin = pBgWin->parent)
					;
				cfbCopyRotatePixmap(pWin->border.pixmap,
						    &pPrivWin->pRotatedBorder,
						    pBgWin->drawable.x,
						    pBgWin->drawable.y);
				if (pPrivWin->pRotatedBorder) {
					pPrivWin->fastBorder = TRUE;
					pPrivWin->oldRotate.x = pBgWin->drawable.x;
					pPrivWin->oldRotate.y = pBgWin->drawable.y;
				} else
					pPrivWin->fastBorder = FALSE;
			} else
				pPrivWin->fastBorder = FALSE;
			break;
			
		case CWBorderPixel:
			pPrivWin->fastBorder = FALSE;
			break;
		}
	}
	return (TRUE);
}

static void
CreatorPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pWin->drawable.pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	register cfbPrivWin *pPrivWin;
	CreatorStipplePtr stipple;
	WindowPtr pBgWin;

	FFBLOG(("CreatorPaintWindow: WIN(%p) what(%d)\n", pWin, what));
	pPrivWin = cfbGetWindowPrivate(pWin);
	switch (what) {
	case PW_BACKGROUND:
		stipple = CreatorGetWindowPrivate(pWin);
		switch (pWin->backgroundState) {
		case None:
			return;
		case ParentRelative:
			do {
				pWin = pWin->parent;
			} while (pWin->backgroundState == ParentRelative);
			(*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion, what);
			return;
		case BackgroundPixmap:
			if (stipple) {
				CreatorFillBoxStipple ((DrawablePtr)pWin, 
						       (int)REGION_NUM_RECTS(pRegion),
						       REGION_RECTS(pRegion),
						       stipple);
				return;
			}
			FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, 0xffffffff, GXcopy);
			FFBWait(pFfb, ffb);
			if (pPrivWin->fastBackground) {
				cfbFillBoxTile32 ((DrawablePtr)pWin,
						  (int)REGION_NUM_RECTS(pRegion),
						  REGION_RECTS(pRegion),
						  pPrivWin->pRotatedBackground);
			} else {
				cfbFillBoxTileOdd ((DrawablePtr)pWin,
						   (int)REGION_NUM_RECTS(pRegion),
						   REGION_RECTS(pRegion),
						   pWin->background.pixmap,
						   (int) pWin->drawable.x, (int) pWin->drawable.y);
			}
			return;
		case BackgroundPixel:
			CreatorFillBoxSolid ((DrawablePtr)pWin,
					     (int)REGION_NUM_RECTS(pRegion),
					     REGION_RECTS(pRegion),
					     pWin->background.pixel);
			return;
		}
		break;
	case PW_BORDER:
		if (pWin->borderIsPixel) {
			CreatorFillBoxSolid ((DrawablePtr)pWin,
					     (int)REGION_NUM_RECTS(pRegion),
					     REGION_RECTS(pRegion),
					     pWin->border.pixel);
			return;
		}
		FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, 0xffffffff, GXcopy);
		FFBWait(pFfb, ffb);
		if (pPrivWin->fastBorder) {
			cfbFillBoxTile32 ((DrawablePtr)pWin,
					  (int)REGION_NUM_RECTS(pRegion),
					  REGION_RECTS(pRegion),
					  pPrivWin->pRotatedBorder);
		} else {
			for (pBgWin = pWin;
			     pBgWin->backgroundState == ParentRelative;
			     pBgWin = pBgWin->parent)
				;

			cfbFillBoxTileOdd ((DrawablePtr)pWin,
					   (int)REGION_NUM_RECTS(pRegion),
					   REGION_RECTS(pRegion),
					   pWin->border.pixmap,
					   (int) pBgWin->drawable.x,
					   (int) pBgWin->drawable.y);
		}
		return;
	}
}

#ifdef USE_VIS
static void 
CreatorCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pWin->drawable.pScreen);
	DDXPointPtr pptSrc;
	DDXPointPtr ppt;
	RegionPtr prgnDst;
	BoxPtr pbox;
	int dx, dy;
	int i, nbox;
	WindowPtr pwinRoot;
	extern WindowPtr *WindowTable;

	FFBLOG(("CreatorCopyWindow: WIN(%p)\n", pWin));
	dx = ptOldOrg.x - pWin->drawable.x;
	dy = ptOldOrg.y - pWin->drawable.y;

	pwinRoot = WindowTable[pWin->drawable.pScreen->myNum];

	prgnDst = REGION_CREATE(pWin->drawable.pScreen, NULL, 1);

	REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);
	REGION_INTERSECT(pWin->drawable.pScreen, prgnDst, &pWin->borderClip, prgnSrc);

	pbox = REGION_RECTS(prgnDst);
	nbox = REGION_NUM_RECTS(prgnDst);
	if(!(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec))))
		return;
	ppt = pptSrc;

	for (i = nbox; --i >= 0; ppt++, pbox++) {
		ppt->x = pbox->x1 + dx;
		ppt->y = pbox->y1 + dy;
	}

	if (!pFfb->disable_vscroll && (!dx && dy)) {
		FFBPtr pFfb = GET_FFB_FROM_SCREEN(pWin->drawable.pScreen);

		FFB_WRITE_ATTRIBUTES_VSCROLL(pFfb, PMSK);
		CreatorDoVertBitblt ((DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot,
				     GXcopy, prgnDst, pptSrc, ~0L);
	} else {
		FFBPtr pFfb = GET_FFB_FROM_SCREEN(pWin->drawable.pScreen);
		ffb_fbcPtr ffb = pFfb->regs;

		FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, PMSK, GXcopy);
		FFBWait(pFfb, ffb);
		CreatorDoBitblt ((DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot,
				 GXcopy, prgnDst, pptSrc, ~0L);
	}
	DEALLOCATE_LOCAL(pptSrc);
	REGION_DESTROY(pWin->drawable.pScreen, prgnDst);
}

static void
CreatorSaveAreas(PixmapPtr pPixmap, RegionPtr prgnSave, int xorg, int yorg, WindowPtr pWin)
{
	ScreenPtr pScreen = pPixmap->drawable.pScreen;
	FFBPtr pFfb = GET_FFB_FROM_SCREEN(pScreen);
	ffb_fbcPtr ffb = pFfb->regs;
	register DDXPointPtr pPt;
	DDXPointPtr pPtsInit;
	register BoxPtr pBox;
	register int i;
	PixmapPtr pScrPix;
    
	FFBLOG(("CreatorSaveAreas: WIN(%p)\n", pWin));
	i = REGION_NUM_RECTS(prgnSave);
	pPtsInit = (DDXPointPtr)ALLOCATE_LOCAL(i * sizeof(DDXPointRec));
	if (!pPtsInit)
		return;
    
	pBox = REGION_RECTS(prgnSave);
	pPt = pPtsInit;
	while (--i >= 0) {
		pPt->x = pBox->x1 + xorg;
		pPt->y = pBox->y1 + yorg;
		pPt++;
		pBox++;
	}

#ifdef CFB_NEED_SCREEN_PRIVATE
	pScrPix = (PixmapPtr) pScreen->devPrivates[cfbScreenPrivateIndex].ptr;
#else
	pScrPix = (PixmapPtr) pScreen->devPrivate;
#endif

	/* SRC is the framebuffer, DST is a pixmap */
	FFBWait(pFfb, ffb);
	CreatorDoBitblt((DrawablePtr) pScrPix, (DrawablePtr)pPixmap,
			GXcopy, prgnSave, pPtsInit, ~0L);

	DEALLOCATE_LOCAL (pPtsInit);
}

static void
CreatorRestoreAreas(PixmapPtr pPixmap, RegionPtr prgnRestore, int xorg, int yorg, WindowPtr pWin)
{
	FFBPtr pFfb;
	ffb_fbcPtr ffb;
	register DDXPointPtr pPt;
	DDXPointPtr pPtsInit;
	register BoxPtr pBox;
	register int i;
	ScreenPtr pScreen = pPixmap->drawable.pScreen;
	PixmapPtr pScrPix;
    
	FFBLOG(("CreatorRestoreAreas: WIN(%p)\n", pWin));
	i = REGION_NUM_RECTS(prgnRestore);
	pPtsInit = (DDXPointPtr)ALLOCATE_LOCAL(i*sizeof(DDXPointRec));
	if (!pPtsInit)
		return;
    
	pBox = REGION_RECTS(prgnRestore);
	pPt = pPtsInit;
	while (--i >= 0) {
		pPt->x = pBox->x1 - xorg;
		pPt->y = pBox->y1 - yorg;
		pPt++;
		pBox++;
	}

#ifdef CFB_NEED_SCREEN_PRIVATE
	pScrPix = (PixmapPtr) pScreen->devPrivates[cfbScreenPrivateIndex].ptr;
#else
	pScrPix = (PixmapPtr) pScreen->devPrivate;
#endif
	pFfb = GET_FFB_FROM_SCREEN(pScreen);
	ffb = pFfb->regs;

	/* SRC is a pixmap, DST is the framebuffer */
	FFB_WRITE_ATTRIBUTES_SFB_VAR(pFfb, PMSK, GXcopy);
	FFBWait(pFfb, ffb);
	CreatorDoBitblt((DrawablePtr)pPixmap, (DrawablePtr) pScrPix,
			GXcopy, prgnRestore, pPtsInit, ~0L);

	DEALLOCATE_LOCAL (pPtsInit);
}

static void
CreatorGetImage(DrawablePtr pDrawable, int sx, int sy, int w, int h, unsigned int format, unsigned long planeMask, char* pdstLine)
{
	BoxRec box;
	DDXPointRec ptSrc;
	RegionRec rgnDst;
	ScreenPtr pScreen;
	PixmapPtr pPixmap;

	FFBLOG(("CreatorGetImage: s[%08x:%08x] wh[%08x:%08x]\n", sx, sy, w, h));
	if ((w == 0) || (h == 0))
		return;
	if (pDrawable->bitsPerPixel == 1) {
		mfbGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
		return;
	}
	pScreen = pDrawable->pScreen;
	if(format == ZPixmap) {
		FFBPtr pFfb = GET_FFB_FROM_SCREEN (pScreen);
		ffb_fbcPtr ffb = pFfb->regs;

		/* We have to have the full planemask. */
		FFBWait(pFfb, ffb);
		if((planeMask & PMSK) != PMSK) {
			cfbGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
			return;
		}

		/* SRC is the framebuffer, DST is a pixmap */
		if(w == 1 && h == 1) {
			unsigned int *dstPixel = (unsigned int *)pdstLine;
			unsigned char *sfb = (unsigned char *)pFfb->fb;

			/* Benchmarks do this make sure the acceleration hardware
			 * has completed all of it's operations, therefore I feel
			 * it is not cheating to special case this because if
			 * anything it gives the benchmarks more accurate results.
			 */
			*dstPixel = *((unsigned int *)(sfb +
						       ((sy + pDrawable->y) << 13) +
						       ((sx + pDrawable->x) << 2)));
			return;
		}
		pPixmap = GetScratchPixmapHeader(pScreen, w, h, 
						 pDrawable->depth, pDrawable->bitsPerPixel,
						 PixmapBytePad(w,pDrawable->depth), (pointer)pdstLine);
		if (!pPixmap)
			return;
		ptSrc.x = sx + pDrawable->x;
		ptSrc.y = sy + pDrawable->y;
		box.x1 = 0;
		box.y1 = 0;
		box.x2 = w;
		box.y2 = h;
		REGION_INIT(pScreen, &rgnDst, &box, 1);
		CreatorDoBitblt(pDrawable, (DrawablePtr)pPixmap, GXcopy, &rgnDst,
				&ptSrc, planeMask);
		REGION_UNINIT(pScreen, &rgnDst);
		FreeScratchPixmapHeader(pPixmap);
	} else
		miGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
}

#endif /* USE_VIS */

extern void
CreatorGetSpans(DrawablePtr pDrawable, int wMax, DDXPointPtr ppt,
		int *pwidth, int nspans, char *pchardstStart);

void
CreatorVtChange (ScreenPtr pScreen, int enter)
{
	FFBPtr pFfb = GET_FFB_FROM_SCREEN (pScreen); 
	ffb_fbcPtr ffb = pFfb->regs;

	pFfb->rp_active = 1;
	FFBWait(pFfb, ffb);	
	pFfb->fifo_cache = -1;
	pFfb->fbc_cache = FFB_FBC_WB_A|FFB_FBC_WM_COMBINED|
			     FFB_FBC_RB_A|FFB_FBC_SB_BOTH|FFB_FBC_XE_OFF|
			     FFB_FBC_ZE_OFF|FFB_FBC_YE_OFF|FFB_FBC_RGBE_MASK;
	pFfb->ppc_cache = (FFB_PPC_FW_DISABLE|
			      FFB_PPC_VCE_DISABLE|FFB_PPC_APE_DISABLE|FFB_PPC_CS_CONST|
			      FFB_PPC_XS_CONST|FFB_PPC_YS_CONST|FFB_PPC_ZS_CONST|
			      FFB_PPC_DCE_DISABLE|FFB_PPC_ABE_DISABLE|FFB_PPC_TBE_OPAQUE);
	pFfb->pmask_cache = ~0;
	pFfb->rop_cache = FFB_ROP_EDIT_BIT;
	pFfb->drawop_cache = FFB_DRAWOP_RECTANGLE;
	pFfb->fg_cache = pFfb->bg_cache = 0;
	pFfb->fontw_cache = 32;
	pFfb->fontinc_cache = (1 << 16) | 0;
	pFfb->laststipple = NULL;
	FFBFifo(pFfb, 9);
	ffb->fbc = pFfb->fbc_cache;
	ffb->ppc = pFfb->ppc_cache;
	ffb->pmask = pFfb->pmask_cache;
	ffb->rop = pFfb->rop_cache;
	ffb->drawop = pFfb->drawop_cache;
	ffb->fg = pFfb->fg_cache;
	ffb->bg = pFfb->bg_cache;
	ffb->fontw = pFfb->fontw_cache;
	ffb->fontinc = pFfb->fontinc_cache;
	pFfb->rp_active = 1;
	FFBWait(pFfb, ffb);
}

/* Multiplies and divides suck... */
static void CreatorAlignTabInit(FFBPtr pFfb)
{
	struct fastfill_parms *ffp = &FFB_FFPARMS(pFfb);
	short *tab = pFfb->Pf_AlignTab;
	int i;

	for(i = 0; i < 0x800; i++) {
		int alignval;

		alignval = (i / ffp->pagefill_width) * ffp->pagefill_width;
		*tab++ = alignval;
	}
}

extern Bool CreatorCreateGC (GCPtr pGC);

#ifdef DEBUG_FFB
FILE *FDEBUG_FD = NULL;
#endif

BSFuncRec CreatorBSFuncRec = {
    CreatorSaveAreas,
    CreatorRestoreAreas,
    (BackingStoreSetClipmaskRgnProcPtr) 0,
    (BackingStoreGetImagePixmapProcPtr) 0,
    (BackingStoreGetSpansPixmapProcPtr) 0,
};

Bool FFBAccelInit (ScreenPtr pScreen, FFBPtr pFfb)
{
	ffb_fbcPtr ffb;

	if (serverGeneration != CreatorGeneration) {
		CreatorScreenPrivateIndex = AllocateScreenPrivateIndex ();
		if (CreatorScreenPrivateIndex == -1) return FALSE;
		CreatorGCPrivateIndex = AllocateGCPrivateIndex ();
		CreatorWindowPrivateIndex = AllocateWindowPrivateIndex ();
		CreatorGeneration = serverGeneration;
	}
	
	/* Allocate private structures holding pointer to both videoRAM and control registers.
	   We do not have to map these by ourselves, because the XServer did it for us; we
	   only copy the pointers to out structures. */
	if (!AllocateGCPrivate(pScreen, CreatorGCPrivateIndex, sizeof(CreatorPrivGCRec))) return FALSE;
	if (!AllocateWindowPrivate(pScreen, CreatorWindowPrivateIndex, 0)) return FALSE;
	pScreen->devPrivates[CreatorScreenPrivateIndex].ptr = pFfb;
	pFfb->fifo_cache = 0;
	ffb = pFfb->regs;
	pFfb->strapping_bits = (volatile unsigned int *)
		xf86MapSbusMem (pFfb->psdp, FFB_EXP_VOFF, 8192);
	if (!pFfb->strapping_bits)
		return FALSE;
#if 0
	ErrorF("STRAPPING BITS [%08x] [%02x]\n",
	       *(pFfb->strapping_bits),
	       *((volatile unsigned char *)pFfb->strapping_bits));
#endif
	/* Now probe for the type of FFB/AFB chip we have. */
	{
		volatile unsigned int *afb_fem;
		unsigned int val;

		afb_fem = ((volatile unsigned int *)
			   ((char *)ffb + 0x1540));
		val = *afb_fem;
		val &= 0xff;
		xf86Msg(X_INFO, "/dev/fb%d: ", pFfb->psdp->fbNum);
		if (val == 0x3f || val == 0x07) {
			if (val == 0x3f) {
				pFfb->ffb_type = afb_m6;
				ErrorF("AFB: Detected Elite3D/M6.\n");
			} else {
				pFfb->ffb_type = afb_m3;
				ErrorF("AFB: Detected Elite3D/M3.\n");
			}
		} else {
			unsigned char sbits = *((volatile unsigned char *)pFfb->strapping_bits);
			sbits = *((volatile unsigned char *)pFfb->strapping_bits);
			switch (sbits & 0x78) {
			case (0x0 << 5) | (0x0 << 3):
				pFfb->ffb_type = ffb1_prototype;
				ErrorF("FFB: Detected FFB1 pre-FCS prototype, ");
				break;
			case (0x0 << 5) | (0x1 << 3):
				pFfb->ffb_type = ffb1_standard;
				ErrorF("FFB: Detected FFB1, ");
				break;
			case (0x0 << 5) | (0x3 << 3):
				pFfb->ffb_type = ffb1_speedsort;
				ErrorF("FFB: Detected FFB1-SpeedSort, ");
				break;
			case (0x1 << 5) | (0x0 << 3):
				pFfb->ffb_type = ffb2_prototype;
				ErrorF("FFB: Detected FFB2/vertical pre-FCS prototype, ");
				break;
			case (0x1 << 5) | (0x1 << 3):
				pFfb->ffb_type = ffb2_vertical;
				ErrorF("FFB: Detected FFB2/vertical, ");
				break;
			case (0x1 << 5) | (0x2 << 3):
				pFfb->ffb_type = ffb2_vertical_plus;
				ErrorF("FFB: Detected FFB2+/vertical, ");
				break;
			case (0x2 << 5) | (0x0 << 3):
				pFfb->ffb_type = ffb2_horizontal;
				ErrorF("FFB: Detected FFB2/horizontal, ");
				break;
			case (0x2 << 5) | (0x2 << 3):
				pFfb->ffb_type = ffb2_horizontal;
				ErrorF("FFB: Detected FFB2+/horizontal, ");
				break;
			default:
				pFfb->ffb_type = ffb2_vertical;
				ErrorF("FFB: Unknown boardID[%08x], assuming FFB2, ", sbits);
				break;
			};
			if (sbits & (1 << 2))
				ErrorF("DoubleRES, ");
			if (sbits & (1 << 1))
				ErrorF("Z-buffer, ");
			if (sbits & (1 << 0))
				ErrorF("Double-buffered.\n");
			else
				ErrorF("Single-buffered.\n");
		}
	}

	/* Replace various screen functions. */
	pScreen->CreateGC = CreatorCreateGC;
	pScreen->CreateWindow = CreatorCreateWindow;
	pScreen->ChangeWindowAttributes = CreatorChangeWindowAttributes;
	pScreen->DestroyWindow = CreatorDestroyWindow;
	pScreen->PaintWindowBackground = CreatorPaintWindow;
	pScreen->PaintWindowBorder = CreatorPaintWindow;
	pScreen->GetSpans = CreatorGetSpans;
#ifdef USE_VIS
	pScreen->CopyWindow = CreatorCopyWindow;
	pScreen->GetImage = CreatorGetImage;
	pScreen->BackingStoreFuncs = CreatorBSFuncRec;
#endif

	/* Set FFB line-bias for clipping. */
	miSetZeroLineBias(pScreen, OCTANT3 | OCTANT4 | OCTANT6 | OCTANT1);

	FFB_DEBUG_init();
	FDEBUG((FDEBUG_FD,
		"FFB: cfg0(%08x) cfg1(%08x) cfg2(%08x) cfg3(%08x) ppcfg(%08x)\n",
		ffb->fbcfg0, ffb->fbcfg1, ffb->fbcfg2, ffb->fbcfg3, ffb->ppcfg));

	/* Determine the screen resolution active, needed to figure out
	 * the fastfill/pagefill parameters.
	 */
	switch(ffb->fbcfg0 & FFB_FBCFG0_RES_MASK) {
	default:
	case FFB_FBCFG0_RES_STD:
		pFfb->ffb_res = ffb_res_standard;
		break;
	case FFB_FBCFG0_RES_HIGH:
		pFfb->ffb_res = ffb_res_high;
		break;
	case FFB_FBCFG0_RES_STEREO:
		pFfb->ffb_res = ffb_res_stereo;
		break;
	case FFB_FBCFG0_RES_PRTRAIT:
		pFfb->ffb_res = ffb_res_portrait;
		break;
	};
	CreatorAlignTabInit(pFfb);

	/* Next, determine the hwbug workarounds and feature enables
	 * we should be using on this board.
	 */
	pFfb->disable_pagefill = 0;
	pFfb->disable_vscroll = 0;
	pFfb->has_brline_bug = 0;
	pFfb->use_blkread_prefetch = 0;
	if (pFfb->ffb_type == ffb1_prototype ||
	    pFfb->ffb_type == ffb1_standard ||
	    pFfb->ffb_type == ffb1_speedsort) {
		pFfb->has_brline_bug = 1;
		if (pFfb->ffb_res == ffb_res_high)
			pFfb->disable_vscroll = 1;
		if (pFfb->ffb_res == ffb_res_high ||
		    pFfb->ffb_res == ffb_res_stereo)
			pFfb->disable_pagefill = 1;

	} else {
		/* FFB2 has blkread prefetch.  AFB supposedly does too
		 * but the chip locks up on me when I try to use it. -DaveM
		 */
#define AFB_PREFETCH_IS_BUGGY	1
		if (!AFB_PREFETCH_IS_BUGGY ||
		    (pFfb->ffb_type != afb_m3 &&
		     pFfb->ffb_type != afb_m6)) {
			pFfb->use_blkread_prefetch = 1;
		}
		/* XXX I still cannot get page/block fast fills
		 * XXX to work reliably on any of my AFB boards. -DaveM
		 */
#define AFB_FASTFILL_IS_BUGGY	1
		if (AFB_FASTFILL_IS_BUGGY &&
		    (pFfb->ffb_type == afb_m3 ||
		     pFfb->ffb_type == afb_m6))
			pFfb->disable_pagefill = 1;
	}
	pFfb->disable_fastfill_ap = 0;
	if (pFfb->ffb_res == ffb_res_stereo ||
	    pFfb->ffb_res == ffb_res_high)
		pFfb->disable_fastfill_ap = 1;

	pFfb->ppc_cache = (FFB_PPC_FW_DISABLE|
			      FFB_PPC_VCE_DISABLE|FFB_PPC_APE_DISABLE|FFB_PPC_CS_CONST|
			      FFB_PPC_XS_CONST|FFB_PPC_YS_CONST|FFB_PPC_ZS_CONST|
			      FFB_PPC_DCE_DISABLE|FFB_PPC_ABE_DISABLE|FFB_PPC_TBE_OPAQUE);
	pFfb->pmask_cache = ~0;
	pFfb->rop_cache = FFB_ROP_ZERO;
	pFfb->drawop_cache = FFB_DRAWOP_RECTANGLE;
	pFfb->fg_cache = pFfb->bg_cache = 0;
	pFfb->fontw_cache = 32;
	pFfb->fontinc_cache = (1 << 16) | 0;
	pFfb->fbc_cache = FFB_FBC_WB_A|FFB_FBC_WM_COMBINED|
			     FFB_FBC_RB_A|FFB_FBC_SB_BOTH|FFB_FBC_XE_OFF|
			     FFB_FBC_ZE_OFF|FFB_FBC_YE_OFF|FFB_FBC_RGBE_MASK;
	pFfb->laststipple = NULL;

	/* We will now clear the screen: we'll draw a rectangle covering all the
	 * viewscreen, using a 'blackness' ROP.
	 */
	FFBFifo(pFfb, 13);
	ffb->fbc = pFfb->fbc_cache;
	ffb->ppc = pFfb->ppc_cache;
	ffb->pmask = pFfb->pmask_cache;
	ffb->rop = pFfb->rop_cache;
	ffb->drawop = pFfb->drawop_cache;
	ffb->fg = pFfb->fg_cache;
	ffb->bg = pFfb->bg_cache;
	ffb->fontw = pFfb->fontw_cache;
	ffb->fontinc = pFfb->fontinc_cache;
	FFB_WRITE64(&ffb->by, 0, 0);
	FFB_WRITE64_2(&ffb->bh, pFfb->psdp->height, pFfb->psdp->width);
	pFfb->rp_active = 1;
	FFBWait(pFfb, ffb);
	
	/* Success */
	return TRUE;
}

void
FFBAccelFini(FFBPtr pFfb)
{
	if (pFfb->sfb32)
		xf86UnmapSbusMem(pFfb->psdp, pFfb->sfb32, 0x1000000);
	if (pFfb->strapping_bits)
		xf86UnmapSbusMem(pFfb->psdp, (void *)pFfb->strapping_bits, 8192);
}
