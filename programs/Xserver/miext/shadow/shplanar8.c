/*
 * $XFree86$
 *
 * Copyright © 2000 Keith Packard
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

/*
 * Expose 8bpp depth 4
 */

#define PL_SHIFT    8
#define PL_UNIT	    (1 << PL_SHIFT)
#define PL_MASK	    (PL_UNIT - 1)

void
shadowUpdatePlanar4x8 (ScreenPtr	pScreen,
		       PixmapPtr	pShadow,
		       RegionPtr	damage)
{
    shadowScrPriv(pScreen);
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBase, *shaLine, *sha;
    FbBits	a, b;
    FbBits	s0, s1, s2, s3;
    FbStride	shaStride;
    int		scrBase, scrLine, scr;
    int		shaBpp;
    int		x, y, w, h, width;
    int         i;
    CARD32	*winBase, *winLine, *win;
    CARD32	winSize;
    int		plane;

    fbGetDrawable (&pShadow->drawable, shaBase, shaStride, shaBpp);
    while (nbox--)
    {
	x = pbox->x1 * shaBpp;
	y = pbox->y1;
	w = (pbox->x2 - pbox->x1) * shaBpp;
	h = pbox->y2 - pbox->y1;

	w = (w + (x & PL_MASK) + PL_MASK) >> PL_SHIFT;
	x &= ~PL_MASK;
	
	scrLine = (x >> PL_SHIFT);
	shaLine = shaBase + y * shaStride + (x >> FB_SHIFT);
	
	while (h--)
	{
	    for (plane = 0; plane < 4; plane++)
	    {
		width = w;
		scr = scrLine;
		sha = shaLine;
		winSize = 0;
		scrBase = 0;
		while (width) {
		    /* how much remains in this window */
		    i = scrBase + winSize - scr;
		    if (i <= 0 || scr < scrBase)
		    {
			winBase = (CARD32 *) (*pScrPriv->window) (pScreen,
								 y,
								 (scr << 4) | plane,
								 SHADOW_WINDOW_WRITE,
								 &winSize);
			if(!winBase)
			    return;
			winSize >>= 2;
			scrBase = scr;
			i = winSize;
		    }
		    win = winBase + (scr - scrBase);
		    if (i > width)
			i = width;
		    width -= i;
		    scr += i;
#define PickBit(a,i)	(((a) >> (i)) & 1)
		    while (i--)
		    {
			a = *sha++;
			b = *sha++;
			a >>= plane;
			b >>= plane;
			s0 = ((PickBit(a,0) << 7) |
			     (PickBit(a,8) << 6) |
			     (PickBit(a,16) << 5) |
			     (PickBit(a,24) << 4) |
			     (PickBit(b,0) << 3) |
			     (PickBit(b,8) << 2) |
			     (PickBit(b,16) << 1) |
			     (PickBit(b,24) << 0));
			a = *sha++;
			b = *sha++;
			a >>= plane;
			b >>= plane;
			s1 = ((PickBit(a,0) << 7) |
			     (PickBit(a,8) << 6) |
			     (PickBit(a,16) << 5) |
			     (PickBit(a,24) << 4) |
			     (PickBit(b,0) << 3) |
			     (PickBit(b,8) << 2) |
			     (PickBit(b,16) << 1) |
			     (PickBit(b,24) << 0));
			a = *sha++;
			b = *sha++;
			a >>= plane;
			b >>= plane;
			s2 = ((PickBit(a,0) << 7) |
			     (PickBit(a,8) << 6) |
			     (PickBit(a,16) << 5) |
			     (PickBit(a,24) << 4) |
			     (PickBit(b,0) << 3) |
			     (PickBit(b,8) << 2) |
			     (PickBit(b,16) << 1) |
			     (PickBit(b,24) << 0));
			a = *sha++;
			b = *sha++;
			a >>= plane;
			b >>= plane;
			s3 = ((PickBit(a,0) << 7) |
			     (PickBit(a,8) << 6) |
			     (PickBit(a,16) << 5) |
			     (PickBit(a,24) << 4) |
			     (PickBit(b,0) << 3) |
			     (PickBit(b,8) << 2) |
			     (PickBit(b,16) << 1) |
			     (PickBit(b,24) << 0));
			*win++ = s0 | (s1 << 8) | (s2 << 16) | (s3 << 24);
		    }
		}
	    }
	    shaLine += shaStride;
	    y++;
	}
	pbox++;
    }
}
		    
