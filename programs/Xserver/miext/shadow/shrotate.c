/*
 * $XFree86: $
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include    "X.h"
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    "font.h"
#include    "dixfontstr.h"
#include    "fontstruct.h"
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"
#include    "fb.h"

static FbBits
fetch (FbBits *bits, FbStride stride, int bpp, int x, int y)
{
    FbBits  b, mask;
    
    bits += y * stride;
    x *= bpp;
    bits += x >> FB_SHIFT;
    b = *bits;
    x &= FB_MASK;

    b = FbScrRight (b, FB_UNIT - x - bpp);
    mask = FbScrRight (FB_ALLONES, FB_UNIT - bpp);
    b &= mask;
    return b;
}

void
shadowUpdateRotatePacked (ScreenPtr	pScreen,
			  shadowBufPtr	pBuf)
{
    RegionPtr	damage = &pBuf->damage;
    PixmapPtr	pShadow = pBuf->pPixmap;
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBits;
    FbStride	shaStride;
    int		shaBpp;
    int		shaXoff, shaYoff;
    int		box_x1, box_x2, box_y1, box_y2;
    int		sha_x1, sha_x2, sha_y1, sha_y2, sha_w, sha_h;
    int		scr_x1, scr_x2, scr_y1, scr_y2, scr_w, scr_h;
    int		scr_x, scr_y, sha_x, sha_y;
    int		w;
    FbBits	startmask, endmask;
    int		nmiddle;
    int		pixelsPerBits;
    int		pixelsMask;
    FbStride	shaStepOverY, shaStepDownY, shaStepOverX, shaStepDownX;
    FbBits	*shaUpperLeft;
    FbBits	*shaLine, *sha;
    int		shaHeight = pShadow->drawable.height;
    int		shaWidth = pShadow->drawable.width;

    fbGetDrawable (&pShadow->drawable, shaBits, shaStride, shaBpp, shaXoff, shaYoff);
    pixelsPerBits = (sizeof (FbBits) * 8) / shaBpp;
    pixelsMask = ~(pixelsPerBits - 1);
    /*
     * Compute rotation related constants to walk the shadow
     */
    switch (pBuf->rotate) {
    case 0:
	shaStepOverX = 1;
	shaStepDownX = 0;
	shaStepOverY = 0;
	shaStepDownY = 1;
	break;
    case 90:
	shaStepOverX = 0;
	shaStepDownX = -1;
	shaStepOverY = 1;
	shaStepDownY = 0;
	break;
    case 180:
	shaStepOverX = -1;
	shaStepDownX = 0;
	shaStepOverY = 0;
	shaStepDownY = -1;
	break;
    case 270:
	shaStepOverX = 0;
	shaStepDownX = 1;
	shaStepOverY = -1;
	shaStepDownY = 0;
	break;
    }
    while (nbox--)
    {
        box_x1 = pbox->x1;
        box_y1 = pbox->y1;
        box_x2 = pbox->x2;
        box_y2 = pbox->y2;
        pbox++;

	/*
	 * Compute screen and shadow locations for this box
	 */
	switch (pBuf->rotate) {
	case 0:
	    scr_x1 = box_x1 & pixelsMask;
	    scr_x2 = (box_x2 + pixelsPerBits - 1) & pixelsMask;
	    scr_y1 = box_y1;
	    scr_y2 = box_y2;
	    
	    sha_x1 = box_x1;
	    sha_y1 = box_y1;
	    break;
	case 90:
	    scr_x1 = box_y1 & pixelsMask;
	    scr_x2 = (box_y2 + pixelsPerBits - 1) & pixelsMask;
	    scr_y1 = (shaWidth - box_x2);
	    scr_y2 = (shaWidth - box_x1);

	    sha_x1 = box_x2 - 1;
	    sha_y1 = scr_x1;
	    break;
	case 180:
	    scr_x1 = (shaWidth - box_x2) & pixelsMask;
	    scr_x2 = (shaWidth - box_x1 + pixelsPerBits - 1) & pixelsMask;
	    scr_y1 = shaHeight - box_y2;
	    scr_y2 = shaHeight - box_y1;

	    sha_x1 = (shaWidth - scr_x1 - (pixelsPerBits - 1));
	    sha_y1 = box_y2 - 1;
	    break;
	case 270:
	    scr_x1 = (shaHeight - box_y2) & pixelsMask;
	    scr_x2 = (shaHeight - box_y1 + pixelsPerBits - 1) & pixelsMask;
	    scr_y1 = box_x1;
	    scr_y2 = box_x2;

	    sha_x1 = box_x1;
	    sha_y1 = (shaHeight - scr_x1 - 1);
	    break;
	}
	scr_w = ((scr_x2 - scr_x1) * shaBpp) >> FB_SHIFT;
	scr_x = (scr_x1 * shaBpp) >> FB_SHIFT;
	scr_h = scr_y2 - scr_y1;

	scr_y = scr_y1;

	/*
	 * Copy the bits, always write across the physical frame buffer
	 * to take advantage of write combining.
	 */
	while (scr_h--)
	{
	    int	    p;
	    FbBits  bits;
	    FbBits  *winBase, *winLine, *win;
	    int	    i;
	    CARD32  winSize;
	    
	    w = scr_w;
	    scr_x = scr_x1;

	    sha_x = sha_x1;
	    sha_y = sha_y1;
	    while (w)
	    {
		/*
		 * Map some of this line
		 */
		win = (FbBits *) (*pBuf->window) (pScreen,
						  scr_y,
						  (scr_x * shaBpp) >> 3,
						  SHADOW_WINDOW_WRITE,
						  &winSize,
						  pBuf->closure);
		i = (winSize >> 2);
		if (i > w)
		    i = w;
		w -= i;
		/*
		 * Copy the portion of the line mapped
		 */
		while (i--)
		{
		    bits = 0;
		    p = pixelsPerBits;
		    while (p--)
		    {
			bits = FbScrLeft(bits, shaBpp);
			bits |= fetch (shaBits, shaStride, shaBpp, sha_x, sha_y);
			sha_x += shaStepOverX;
			sha_y += shaStepOverY;
		    }
		    *win++ = bits;
		    scr_x += pixelsPerBits;
		}
	    }
	    sha_x1 += shaStepDownX;
	    sha_y1 += shaStepDownY;
	    scr_y++;
	}
    }
}
