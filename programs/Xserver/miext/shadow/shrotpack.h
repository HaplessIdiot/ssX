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

void
FUNC (ScreenPtr pScreen,
      PixmapPtr pShadow,
      RegionPtr damage)
{
    shadowScrPriv(pScreen);
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBits;
    Data	*shaBase, *shaLine, *sha;
    FbStride	shaStride;
    int		scrBase, scrLine, scr;
    int		shaBpp;
    int		x, y, w, h, width;
    int         i;
    Data	*winBase, *winLine, *win;
    CARD32	winSize;
    int		plane;

    fbGetDrawable (&pShadow->drawable, shaBits, shaStride, shaBpp);
    shaBase = (Data *) shaBits;
    shaStride = shaStride * sizeof (FbBits) / sizeof (Data);
    while (nbox--)
    {
	x = pbox->x1;
	y = pbox->y1;
	w = (pbox->x2 - pbox->x1);
	h = pbox->y2 - pbox->y1;

	scrLine = y;
	shaLine = shaBase + y * shaStride + x;
				   
	while (w--)
	{
	    winSize = 0;
	    scrBase = 0;
	    width = h;
	    scr = scrLine;
	    sha = shaLine;
	    while (width) {
		/* how much remains in this window */
		i = scrBase + winSize - scr;
		if (i <= 0 || scr < scrBase)
		{
		    winBase = (Data *) (*pScrPriv->window) (pScreen,
							    pScreen->width - x - 1,
							    scr * sizeof (Data),
							    SHADOW_WINDOW_WRITE,
							    &winSize);
		    if(!winBase)
			return;
		    scrBase = scr;
		    winSize /= sizeof (Data);
		    i = winSize;
		}
		win = winBase + (scr - scrBase);
		if (i > width)
		    i = width;
		width -= i;
		scr += i;
		while (i--)
		{
		    *win++ = *sha;
		    sha += shaStride;
		}
	    }
	    shaLine++;
	    x++;
	}
	pbox++;
    }
}
