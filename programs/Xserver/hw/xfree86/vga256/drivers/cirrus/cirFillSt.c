/* $XConsortium: cir_fillst.c,v 1.1 94/03/28 21:49:18 dpw Exp $ */
/*
 *
 * Copyright 1993 by H. Hanemaayer, Utrecht, The Netherlands
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of H. Hanemaayer not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  H. Hanemaayer makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * H. HANEMAAYER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL H. HANEMAAYER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  H. Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 */

/*
 * These are the functions for stipples and tiles, which call low-level
 * functions.
 */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "cfb.h"
#include "cfbmskbits.h"
#include "cfbrrop.h"
#include "cfb8bit.h"	/* For cfb8StippleRRop. */
#include "mergerop.h"
#include "vgaBank.h"
#include "xf86.h"
#include "vga.h"	/* For vga256InfoRec */

#include "compiler.h"

#include "cir_driver.h"


/*
 * This function uses the color expand fill for opaque stipples.
 * I'm not entirely sure about the origin of the stipple; I assume
 * it should be (0, 0).
 */

void CirrusFillRectOpaqueStippled32(pDrawable, pGC, nBox, pBox)
	DrawablePtr pDrawable;
	GCPtr pGC;
	int nBox;		/* number of boxes to fill */
	register BoxPtr pBox;	/* pointer to list of boxes to fill */
{
	unsigned long *src;
	int stippleHeight;
	cfbPrivGCPtr devPriv;
	PixmapPtr stipple;
	int destPitch;

	devPriv = ((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr);
	stipple = devPriv->pRotatedPixmap;

	destPitch = (int)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind);

	cfb8CheckOpaqueStipple(pGC->alu, pGC->fgPixel, pGC->bgPixel,
		pGC->planemask);

	stippleHeight = stipple->drawable.height;
	src = (unsigned long *)stipple->devPrivate.ptr;

	for (;nBox; nBox--, pBox++) {
		int w = pBox->x2 - pBox->x1;
		/* Small stipples are slightly broken. Disabled until fixed. */
		if (cfb8StippleRRop == GXcopy && w >= 64) {
			if (w >= 64)
				CirrusColorExpand32bitFill(pBox->x1, pBox->y1,
					w, pBox->y2 - pBox->y1, src,
					stippleHeight, 0, 0, pGC->bgPixel,
					pGC->fgPixel, destPitch);
			else
				Cirrus32bitFillSmall(pBox->x1,
					pBox->y1, w, pBox->y2 - pBox->y1, src,
					stippleHeight, 0, 0, pGC->bgPixel,
					pGC->fgPixel, destPitch);
		}
		else {
			/* Special raster op. */
			/* Let cfb do this one. */
			speedupcfb8FillRectOpaqueStippled32(
				pDrawable, pGC, 1, pBox);
		}
	}
}


/*
 * This function uses the color expand fill for transparent stipples.
 */

void CirrusFillRectTransparentStippled32(pDrawable, pGC, nBox, pBox)
	DrawablePtr pDrawable;
	GCPtr pGC;
	int nBox;		/* number of boxes to fill */
	register BoxPtr pBox;	/* pointer to list of boxes to fill */
{
	unsigned long *src;
	int stippleHeight;
	cfbPrivGCPtr devPriv;
	PixmapPtr stipple;
	int destPitch;

	devPriv = ((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr);
	stipple = devPriv->pRotatedPixmap;

	destPitch = (int)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind);

	cfb8CheckStipple(pGC->alu, pGC->fgPixel, pGC->planemask);

	stippleHeight = stipple->drawable.height;
	src = (unsigned long *)stipple->devPrivate.ptr;

	for (;nBox; nBox--, pBox++) {
		int w = pBox->x2 - pBox->x1;
		if (cfb8StippleRRop == GXcopy && w >= 64)
			/* Background pixel -1 means transparent. */
			CirrusColorExpand32bitFill(pBox->x1, pBox->y1, w,
				pBox->y2 - pBox->y1, src, stippleHeight,
				0, 0, -1, pGC->fgPixel,
				destPitch);
		else {
			/* Special raster op. */
			/* Let cfb do this one. */
			speedupcfb8FillRectTransparentStippled32(
				pDrawable, pGC, 1, pBox);
		}
	}
}


/*
 * Cirrus Tile fill.
 * There's no clear way to do tiling efficiently. There are lots of
 * different ways.
 * Uses a specific latched-write function for 32 pixels wide tiles.
 * For 8x8 and 16x16 tiles, the BitBLT engine fill is used. For larger
 * tiles, we use repeated BitBlt.
 *
 * There's potential for using the 8x8 BitBLT engine pattern fill for
 * (1, 2, 4, 8, 16) x (1, 2, 4, 8) tiles, and more sizes by splitting
 * the area into 'bands' (the BitBLT engine should also work for any rop).
 * Also you can 'interleave' by using a doubled pitch (with a virtual screen
 * width < 2048, this works for height 16 because the pitch range goes up to
 * 4095).
 * On the 5434 (which can handle a pitch up to 8191) 4-way interleaving
 * can be used, so that we can use the BitBlt engine for 32x32 tiling
 * with 4 vertical subdivisions, and 16 pixels wide bands horizontally.
 * The 5434 may support a 32 byte wide pattern (for truecolor 8x8 pattern
 * fill), which would make it easier.
 *
 * Currently the accel routine is very restrictive.
 * I think for the 5434, the cfb routine (which does a very good
 * job) may be more efficient than the 32xH latched write fill (h != 32)
 * because of the 32-bit host interface.
 */

void rotatepattern( unsigned char *pattern, unsigned char *src, int pixWidth,
int width, int height, int hrot, int vrot ) {
	int i;
	for (i = 0; i < height; i++) {
		int j;
		unsigned char *srcline;
		srcline = src + ((i + vrot) & (height - 1)) * pixWidth;
		for (j = 0; j < width; j++) {
			*pattern = *(srcline + ((j + hrot) & (width - 1)));
			pattern++;
		}
	}
}

void CirrusFillRectTile(pDrawable, pGC, nBox, pBox)
	DrawablePtr pDrawable;
	GCPtr pGC;
	int nBox;		/* number of boxes to fill */
	register BoxPtr pBox;	/* pointer to list of boxes to fill */
{
	unsigned long *src;
	int stippleHeight;
	PixmapPtr pPix;
	int destPitch;
	int width, height;
	int pixWidth;
	int xrot, yrot;
	void *pattern;

	pPix = pGC->tile.pixmap;
	width = pPix->drawable.width;
	height = pPix->drawable.height;
	pixWidth = PixmapBytePad(width, pPix->drawable.depth);

	destPitch = (int)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind);

	src = (unsigned long *)pPix->devPrivate.ptr;
        xrot = pDrawable->x + pGC->patOrg.x;
	yrot = pDrawable->y + pGC->patOrg.y;

	if (HAVEBITBLTENGINE()) {
		if (width == 8 && height == 8)
			goto tile8x8;
		if (width == 16 && height == 16 && (vga256InfoRec.virtualX
		< 2048 || (HAVE543X() && vga256InfoRec.virtualX < 4096)))
			goto tile16x16;
#if 0
		/* Not yet. */
		if (width == 32 && height == 32 && HAVE543X() &&
		vga256InfoRec.virtualX < 2048))
			goto tile32x32;
#endif
#if 0	/* broken. */
		if (width * height >= 500 && (width != 32 || height > 32 ||
		cirrusBusType == CIRRUS_SLOWBUS || HAVE543X()))
			goto tileblit;
#endif			
	}
	/* On local bus (but not on the 543x), or if there's no blit engine,
	 * use the extended write mode function for 32x32 tiles. 
	 */
	if (width == 32 && height <= 32)
		goto tile32;

	cfbFillRectTileOdd(pDrawable, pGC, nBox, pBox);
	return;

tile8x8:
	/* 8x8 BitBLT tile fill. */
	pattern = ALLOCATE_LOCAL(8 * 8);
	/* This could be speeded up by avoiding the pattern being copied
	 * to video memory each time. */
	for (;nBox; nBox--, pBox++) {
		int w, h;
		rotatepattern(pattern, (unsigned char *)src, pixWidth, 8, 8,
			(pBox->x1 - xrot) & 7, (pBox->y1 - yrot) & 7);
		w = pBox->x2 - pBox->x1;
		h = pBox->y2 - pBox->y1;
		if (w * h < 500)
			cfbFillRectTileOdd(pDrawable, pGC, 1, pBox);
		else {
			CirrusBLT8x8PatternFill(pBox->y1 * destPitch +
				pBox->x1, w, h,	pattern, destPitch,
				CROP_SRC);
		}
	}
	DEALLOCATE_LOCAL(pattern);
	return;

tile16x16:
	/* 16x16 BitBLT tile fill. */
	pattern = ALLOCATE_LOCAL(16 * 16);
	for (;nBox; nBox--, pBox++) {
		int w, h;
		rotatepattern(pattern, (unsigned char *)src, pixWidth, 16, 16,
			(pBox->x1 - xrot) & 15, (pBox->y1 - yrot) & 15);
		w = pBox->x2 - pBox->x1;
		h = pBox->y2 - pBox->y1;
		if (w * h < 250)
			cfbFillRectTileOdd(pDrawable, pGC, 1, pBox);
		else {
			/* Low level function uses vertical interleaving. */
			CirrusBLT16x16PatternFill(pBox->y1 * destPitch +
				pBox->x1, w, h, pattern, destPitch, CROP_SRC);
		}
	}
	DEALLOCATE_LOCAL(pattern);
	return;

#if 0
tile32x32:
	/* 32x32 BitBLT tile fill (for 5434). */
	pattern = ALLOCATE_LOCAL(32 * 32);
	for (;nBox; nBox--, pBox++) {
		int w, h;
		rotatepattern(pattern, src, pixWidth, 32, 32,
			(pBox->x1 - xrot) & 31, (pBox->y1 - yrot) & 31);
		w = pBox->x2 - pBox->x1;
		h = pBox->y2 - pBox->y1;
		if (w * h < 500)
			cfbFillRectTileOdd(pDrawable, pGC, 1, pBox);
		else {
			/* Low level function uses vertical interleaving. */
			CirrusBLT32x32PatternFill(pBox->y1 * destPitch +
				pBox->x1, w, h, pattern, destPitch, CROP_SRC);
		}
	}
	DEALLOCATE_LOCAL(pattern);
	return;
#endif	

tileblit:
	/* Tile with repeated BitBLT. For largish tiles. */
	for (;nBox; nBox--, pBox++) {
		int w, h, x, y;
		int blitx, blith, blity;
		BoxRec box;
		x = pBox->x1;
		y = pBox->y1;
		w = pBox->x2 - x;
		h = pBox->y2 - y;
		/* Don't use the blitter for small tile fills. */
		if (w * h < 250) {
			cfbFillRectTileOdd(pDrawable, pGC, 1, pBox);
			continue;
		}
		box.x1 = x;
		box.y1 = y;
		box.x2 = x + min(width, w);
		box.y2 = y + height;
		/* Draw first tile. */
		cfbFillRectTileOdd(pDrawable, pGC, 1, &box);
		/* Repeat tile horizontally. */
		blitx = x + width;	/* Will skip if width > w. */
		blith = height;
		if (h < height)
			blith = h;
		while (blitx <= x + w - width) {
			CirrusBLTBitBlt(y * destPitch + blitx, y * destPitch +
				x, destPitch, destPitch, width, blith, 1);
			blitx += width;
		}
		/* Right edge. */
		if (blitx < x + w)
			CirrusBLTBitBlt(y * destPitch + blitx, y * destPitch +
				x, destPitch, destPitch, x + w - blitx,
				blith, 1);
		/* Repeat row of tiles vertically. */
		blity = y + height;	/* Will skip if height > h. */
		while (blity < y + h - height) {
			CirrusBLTBitBlt(blity * destPitch + x, y * destPitch +
				x, destPitch, destPitch, w, height, 1);
			blity += height;
		}
		/* Bottom edge. */
		if (blity < y + h)
			CirrusBLTBitBlt(blity * destPitch + x, y * destPitch +
				x, destPitch, destPitch, w, y + h - blity, 1);
	}
	return;

tile32:
	/* The accel routine will only write on 32-aligned x-coords. */
	pattern = ALLOCATE_LOCAL(32 * 32);
	rotatepattern(pattern, (unsigned char *)src, pixWidth, 32, 32,
		      (0 - xrot) & 31, 0);
	for (;nBox; nBox--, pBox++) {
		int w, h, x, y;
		w = pBox->x2 - pBox->x1;
		h = pBox->y2 - pBox->y1;
		if (w < 32 || w * h < 80000)
			cfbFillRectTileOdd(pDrawable, pGC, 1, pBox);
		else {
			int nx, nw;
			int left, right;
			x = pBox->x1;
			y = pBox->y1;
			/* Determine 32-pixel aligned part of area. */
			if (x & 31 == 0)
				left = 0;
			else
				left = 32 - (x & 31);
			nx = x + left;
			/* Width must be multiple of 8 (or better 32). */
			nw = (w - left) & ~31;
			right = w - left - nw;
			if (nw == 0)
				cfbFillRectTileOdd(pDrawable, pGC, 1, pBox);
			else {
				BoxRec box[2];
				/* Do main part. */
				CirrusColorExpandFillTile32(nx, y, nw, h,
					pattern, 32, width, height, yrot,
					destPitch);
				/* Left edge. */
				box[0].x1 = x;
				box[0].y1 = y;
				box[0].x2 = x + left;
				box[0].y2 = y + h;
				/* Right edge. */
				if (right == 0)
					cfbFillRectTileOdd(pDrawable, pGC,
						1, (BoxPtr)&box);
				else {
					box[1].x1 = x + left + nw;
					box[1].y1 = y;
					box[1].x2 = box[1].x1 + right;
					box[1].y2 = y + h;
					cfbFillRectTileOdd(pDrawable, pGC, 2,
						(BoxPtr)&box);
				}
			}
		}
	}
	DEALLOCATE_LOCAL(pattern);
}
