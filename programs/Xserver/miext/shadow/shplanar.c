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
 * 32 4-bit pixels per write
 */

#define PL_SHIFT    7	    
#define PL_UNIT	    (1 << PL_SHIFT)
#define PL_MASK	    (PL_UNIT - 1)

void
shadowUpdatePlanar4 (ScreenPtr	pScreen,
		     PixmapPtr	pShadow,
		     RegionPtr	damage)
{
    shadowScrPriv(pScreen);
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBase, *shaLine, *sha;
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
			s0 = *sha++;
			s0 >>= plane;
			s0 = ((PickBit(s0,0) << 7) |
			     (PickBit(s0,4) << 6) |
			     (PickBit(s0,8) << 5) |
			     (PickBit(s0,12) << 4) |
			     (PickBit(s0,16) << 3) |
			     (PickBit(s0,20) << 2) |
			     (PickBit(s0,24) << 1) |
			     (PickBit(s0,28) << 0));
			s1 = *sha++;
			s1 >>= plane;
			s1 = ((PickBit(s1,0) << 7) |
			     (PickBit(s1,4) << 6) |
			     (PickBit(s1,8) << 5) |
			     (PickBit(s1,12) << 4) |
			     (PickBit(s1,16) << 3) |
			     (PickBit(s1,20) << 2) |
			     (PickBit(s1,24) << 1) |
			     (PickBit(s1,28) << 0));
			s2 = *sha++;
			s2 >>= plane;
			s2 = ((PickBit(s2,0) << 7) |
			     (PickBit(s2,4) << 6) |
			     (PickBit(s2,8) << 5) |
			     (PickBit(s2,12) << 4) |
			     (PickBit(s2,16) << 3) |
			     (PickBit(s2,20) << 2) |
			     (PickBit(s2,24) << 1) |
			     (PickBit(s2,28) << 0));
			s3 = *sha++;
			s3 >>= plane;
			s3 = ((PickBit(s3,0) << 7) |
			     (PickBit(s3,4) << 6) |
			     (PickBit(s3,8) << 5) |
			     (PickBit(s3,12) << 4) |
			     (PickBit(s3,16) << 3) |
			     (PickBit(s3,20) << 2) |
			     (PickBit(s3,24) << 1) |
			     (PickBit(s3,28) << 0));
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
		    
