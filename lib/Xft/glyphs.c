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
_XftLoadGlyphs (Display *dpy, XftFont *font, unsigned long *glyphs, int nglyph)
{
    FT_Error	    error;
    FT_ULong	    charcode;
    FT_GlyphSlot    glyph;
    XGlyphInfo	    *gi;
    Glyph	    g;
    char	    bufLocal[4096];
    char	    *bufBitmap = bufLocal;
    unsigned char   *b, v;
    int		    bufSize = sizeof (bufLocal);
    int		    size, pitch;
    int		    width;
    int		    height;
    int		    i;
    int		    left, right, top, bottom;
    FT_Bitmap	    ftbit;

    while (nglyph--)
    {
	charcode = (FT_ULong) *glyphs++;
	error = FT_Load_Char (font->face, charcode, 0/*|FT_LOAD_NO_HINTING */);
	if (error)
	    continue;

#define FLOOR(x)  ((x) & -64)
#define CEIL(x)   (((x)+63) & -64)
#define TRUNC(x)  ((x) >> 6)
		
	glyph = font->face->glyph;
	
	left  = FLOOR( glyph->metrics.horiBearingX );
	right = CEIL( glyph->metrics.horiBearingX + glyph->metrics.width );
	width = TRUNC(right - left);

	top    = CEIL( glyph->metrics.horiBearingY );
	bottom = FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height );
	height = TRUNC( top - bottom );

	if ( glyph->format == ft_glyph_format_outline )
	{
	    pitch = (width + 3) & ~3;
	    size = pitch * height;
	    
	    if (size > bufSize)
	    {
		if (bufBitmap != bufLocal)
		    free (bufBitmap);
		bufBitmap = malloc (size);
		if (!bufBitmap)
		    continue;
		bufSize = size;
	    }
	    memset (bufBitmap, 0, size);

	    ftbit.width      = width;
	    ftbit.rows       = height;
	    ftbit.pitch      = pitch;
	    ftbit.pixel_mode = ft_pixel_mode_grays;
	    ftbit.buffer     = bufBitmap;
	    
	    FT_Translate_Outline( &glyph->outline, -left, -bottom );

	    FT_Get_Outline_Bitmap( _XftFTlibrary, &glyph->outline, &ftbit );
	    i = size;
	    b = (unsigned char *) bufBitmap;
	    while (i--)
	    {
		v = *b;
		*b = v << 1 | v >> 7;
		b++;
	    }
#if 0
	    {
		int x, y;

		printf ("Charcode %d(%c):\n", charcode, charcode);
		for (y = 0; y < height; y++)
		{
		    for (x = 0; x < width; x++)
		    {
			v = bufBitmap[y * pitch + x];
			if (v)
			    printf ("*");
			else
			    printf (" ");
		    }
		    printf ("\n");
		}
		printf ("\n");
	    }
#endif	    
	}
	else
	{
	    printf ("glyph (%c) %d missing\n", (int) charcode, (int) charcode);
	    continue;
	}
	
	gi = font->realized[charcode];
	gi->width = width;
	gi->height = height;
	gi->x = -TRUNC(left);
	gi->y = TRUNC(top);
	if (font->monospace)
	    gi->xOff = XftFontMaxAdvanceWidth (dpy, font);
	else
	    gi->xOff = (glyph->metrics.horiAdvance + 0x20) >> 6;
	gi->yOff = 0;
	g = charcode;

	XRenderAddGlyphs (dpy, font->glyphset, &g, gi, 1, bufBitmap, size);
    }
    if (bufBitmap != bufLocal)
	free (bufBitmap);
}
