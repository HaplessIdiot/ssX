/*
 * $XFree86: xc/lib/Xft/drawstr.c,v 1.2 2000/10/05 22:57:04 keithp Exp $
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

#define STEP	    256

void
_XftCheckGlyph (Display *dpy, XftFont *font, unsigned long glyph,
		unsigned long *missing, int *nmissing)
{
    XGlyphInfo	    **realized;
    int		    nrealized;
    int		    n;
    
    if (glyph >= font->nrealized)
    {
	nrealized = glyph + STEP;
	if (font->realized)
	    realized = (XGlyphInfo **) realloc ((void *) font->realized,
						nrealized * sizeof (XGlyphInfo *));
	else
	    realized = malloc (nrealized * sizeof (XGlyphInfo *));
	if (!realized)
	    return;
	memset (realized + font->nrealized, 0, 
		(nrealized - font->nrealized) *  sizeof (XGlyphInfo *));
	font->realized = realized;
	font->nrealized = nrealized;
    }
    if (!font->realized[glyph])
    {
	font->realized[glyph] = malloc (sizeof (XGlyphInfo));
	n = *nmissing;
	missing[n++] = glyph;
	if (n == XFT_NMISSING)
	{
	    _XftLoadGlyphs (dpy, font, missing, n);
	    n = 0;
	}
	*nmissing = n;
    }
}

void
XftDrawString (Display *dpy, Picture src, XftFont *font, Picture dst,
	       int srcx, int srcy,
	       int x, int y,
	       unsigned char *string, int len)
{
    unsigned long   missing[XFT_NMISSING];
    int		    nmissing;
    unsigned char   *s;
    int		    l;

    s = string;
    l = len;
    nmissing = 0;
    while (l--)
	_XftCheckGlyph (dpy, font, (unsigned long) *s++, missing, &nmissing);
    if (nmissing)
	_XftLoadGlyphs (dpy, font, missing, nmissing);
    XRenderCompositeString8 (dpy, PictOpOver, src, dst,
			     font->format, font->glyphset,
			     srcx, srcy, x, y, (char *) string, len);
}

