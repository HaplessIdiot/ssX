/*
 * $Id: fbline.c,v 1.1 1999/11/19 13:53:44 hohndel Exp $
 *
 * Copyright © 1998 Keith Packard
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
/* $XFree86: $ */

#include "fb.h"

void
fbZeroLine (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		mode,
	    int		npt,
	    DDXPointPtr	ppt)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate (pGC);
    int		    x1, y1, x2, y2;
    int		    x, y;
    int		    dashOffset;
    int		    totalDash;
    int		    n;
    unsigned char   *dash;

    x = pDrawable->x;
    y = pDrawable->y;
    x1 = ppt->x;
    y1 = ppt->y;
    dashOffset = 0;
    totalDash = 0;
    if (pGC->lineStyle != LineSolid)
	totalDash = pPriv->dashLength;
    while (--npt)
    {
	++ppt;
	x2 = ppt->x;
	y2 = ppt->y;
	if (mode == CoordModePrevious)
	{
	    x2 += x1;
	    y2 += y1;
	}
	fbSegment (pDrawable, pGC, x1 + x, y1 + y, 
		   x2 + x, y2 + y, 
		   npt == 1 && pGC->capStyle != CapNotLast,
		   &dashOffset);
	if (totalDash)
	    dashOffset = dashOffset % totalDash;
	x1 = x2;
	y1 = y2;
    }
}

void
fbPolyLine (DrawablePtr	pDrawable,
	    GCPtr	pGC,
	    int		mode,
	    int		npt,
	    DDXPointPtr	ppt)
{
    if (pGC->lineWidth == 0)
    {
	fbZeroLine (pDrawable, pGC, mode, npt, ppt);
    }
    else
    {
	if (pGC->lineStyle != LineSolid)
	    miWideDash (pDrawable, pGC, mode, npt, ppt);
	else
	    miWideLine (pDrawable, pGC, mode, npt, ppt);
    }
}
