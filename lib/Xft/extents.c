/*
 * $XFree86$
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
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

#include "xftint.h"

void
XftExtentsString (Display	*dpy,
		  XftFont	*font,
		  unsigned char	*string, 
		  int		len,
		  XGlyphInfo	*extents)
{
    unsigned long   missing[XFT_NMISSING];
    int		    nmissing;
    unsigned char   *s;
    int		    l;
    XGlyphInfo	    *gi;
    int		    x, y;

    s = string;
    l = len;
    nmissing = 0;
    while (l--)
	_XftCheckGlyph (dpy, font, (unsigned long) *s++, missing, &nmissing);
    if (nmissing)
	_XftLoadGlyphs (dpy, font, missing, nmissing);
    
    if (!len)
    {
	extents->width = 0;
	extents->height = 0;
	extents->x = 0;
	extents->y = 0;
	extents->yOff = 0;
	extents->xOff = 0;
	return;
    }
    len--;
    gi = font->realized[*string++];
    *extents = *gi;
    x = gi->xOff;
    y = gi->yOff;
    while (len--)
    {
	gi = font->realized[*string++];
	if (gi->x + x < extents->x)
	    extents->x = gi->x + x;
	if (gi->y + y < extents->y)
	    extents->y = gi->y + y;
	if (gi->width + x > extents->width)
	    extents->width = gi->width + x;
	if (gi->height + y > extents->height)
	    extents->height = gi->height + y;
	x += gi->xOff;
	y += gi->yOff;
    }
    extents->xOff = x;
    extents->yOff = y;
}
