/*
 * $XFree86: $
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

#include <stdlib.h>
#include "xftint.h"
#ifdef FREETYPE2
#include <freetype/ftoutln.h>
#endif

static const int    filters[3][3] = {
    /* red */
#if 0
{    65538*4/7,65538*2/7,65538*1/7 },
    /* green */
{    65536*1/4, 65536*2/4, 65537*1/4 },
    /* blue */
{    65538*1/7,65538*2/7,65538*4/7 },
#endif
{    65538*9/13,65538*3/13,65538*1/13 },
    /* green */
{    65538*1/6, 65538*4/6, 65538*1/6 },
    /* blue */
{    65538*1/13,65538*3/13,65538*9/13 },
};

void
XftGlyphLoad (Display		*dpy,
	      XftFontStruct	*font,
	      unsigned long	*glyphs,
	      int		nglyph)
{
#ifdef FREETYPE2
    FT_Error	    error;
    FT_ULong	    charcode;
    FT_UInt	    glyphindex;
    FT_GlyphSlot    glyph;
    XGlyphInfo	    *gi;
    Glyph	    g;
    unsigned char   bufLocal[4096];
    unsigned char   *bufBitmap = bufLocal;
    unsigned char   *b;
    int		    bufSize = sizeof (bufLocal);
    int		    size, pitch;
    unsigned char   bufLocalRgba[4096];
    unsigned char   *bufBitmapRgba = bufLocalRgba;
    int		    bufSizeRgba = sizeof (bufLocalRgba);
    int		    sizergba, pitchrgba, widthrgba;
    int		    width;
    int		    height;
    int		    i;
    int		    left, right, top, bottom;
    int		    mul = 1;
    FT_Bitmap	    ftbit;
    FT_Matrix	    matrix;

    if (font->antialias && font->rgba)
    {
	matrix.xx = 0x30000L;
	matrix.yy = 0x10000L;
	matrix.xy = matrix.yx = 0;
	mul = 3;
    }
    while (nglyph--)
    {
	charcode = (FT_ULong) *glyphs++;
	if (font->encoded)
	{
	    glyphindex = FT_Get_Char_Index (font->face, charcode);
	    if (!glyphindex)
		continue;
	}
	else
	    glyphindex = (FT_UInt) charcode;
	error = FT_Load_Glyph (font->face, glyphindex, 0/*|FT_LOAD_NO_HINTING */);
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
	    if (font->antialias)
		pitch = (width * mul + 3) & ~3;
	    else
		pitch = ((width + 31) & ~31) >> 3;
	    
	    size = pitch * height;
	    
	    if (size > bufSize)
	    {
		if (bufBitmap != bufLocal)
		    free (bufBitmap);
		bufBitmap = (unsigned char *) malloc (size);
		if (!bufBitmap)
		    continue;
		bufSize = size;
	    }
	    memset (bufBitmap, 0, size);

	    ftbit.width      = width * mul;
	    ftbit.rows       = height;
	    ftbit.pitch      = pitch;
	    if (font->antialias)
		ftbit.pixel_mode = ft_pixel_mode_grays;
	    else
		ftbit.pixel_mode = ft_pixel_mode_mono;
	    
	    ftbit.buffer     = bufBitmap;
	    
	    if (font->antialias && font->rgba)
		FT_Outline_Transform (&glyph->outline, &matrix);

	    FT_Outline_Translate ( &glyph->outline, -left*mul, -bottom );

	    FT_Outline_Get_Bitmap( _XftFTlibrary, &glyph->outline, &ftbit );
	    i = size;
	    b = (unsigned char *) bufBitmap;
	    /*
	     * swap bit order around
	     */
	    if (!font->antialias)
	    {
		if (BitmapBitOrder (dpy) != MSBFirst)
		{
		    unsigned char   *line;
		    unsigned char   c;
		    int		    i;

		    line = (unsigned char *) bufBitmap;
		    i = size;
		    while (i--)
		    {
			c = *line;
			c = ((c << 1) & 0xaa) | ((c >> 1) & 0x55);
			c = ((c << 2) & 0xcc) | ((c >> 2) & 0x33);
			c = ((c << 4) & 0xf0) | ((c >> 4) & 0x0f);
			*line++ = c;
		    }
		}
	    }
#if 0
	    {
		int		x, y;
		unsigned char	*line;

		line = bufBitmap;
		printf ("\nchar 0x%x (%c):\n", (int) charcode, (char) charcode);
		for (y = 0; y < height; y++)
		{
		    if (font->antialias) 
		    {
			static char    den[] = { " .:;=+*#" };
			for (x = 0; x < pitch; x++)
			    printf ("%c", den[line[x] >> 5]);
		    }
		    else
		    {
			for (x = 0; x < pitch * 8; x++)
			{
			    printf ("%c", line[x>>3] & (1 << (x & 7)) ? '#' : ' ');
			}
		    }
		    printf ("\n");
		    line += pitch;
		}
	    }
#endif
	}
	else
	{
#if 0
	    printf ("glyph (%c) %d missing\n", (int) charcode, (int) charcode);
#endif
	    continue;
	}
	
	gi = font->realized[charcode];
	gi->width = width;
	gi->height = height;
	gi->x = -TRUNC(left);
	gi->y = TRUNC(top);
	if (font->monospace)
	    gi->xOff = font->face->max_advance_width * font->size / (64 * font->face->units_per_EM);
	else
	    gi->xOff = ((glyph->metrics.horiAdvance + 0x20) >> 6);
	gi->yOff = 0;
	g = charcode;

	if (font->antialias && font->rgba != XFT_RGBA_NONE)
	{
	    int		    x, y;
	    unsigned char   *in_line, *out_line, *in;
	    unsigned int    *out;
	    unsigned int    red, green, blue;
	    int		    rf, gf, bf;
	    int		    s;
	    
	    widthrgba = width;
	    pitchrgba = (widthrgba * 4 + 3) & ~3;
	    sizergba = pitchrgba * height;

	    switch (font->rgba) {
	    case XFT_RGBA_RGB:
	    default:
		rf = 0;
		gf = 1;
		bf = 2;
		break;
	    case XFT_RGBA_BGR:
		bf = 0;
		gf = 1;
		rf = 2;
		break;
	    }
	    if (sizergba > bufSizeRgba)
	    {
		if (bufBitmapRgba != bufLocalRgba)
		    free (bufBitmapRgba);
		bufBitmapRgba = (unsigned char *) malloc (sizergba);
		if (!bufBitmapRgba)
		    continue;
		bufSizeRgba = sizergba;
	    }
	    memset (bufBitmapRgba, 0, sizergba);
	    in_line = bufBitmap;
	    out_line = bufBitmapRgba;
	    for (y = 0; y < height; y++)
	    {
		in = in_line;
		out = (unsigned int *) out_line;
		in_line += pitch;
		out_line += pitchrgba;
		for (x = 0; x < width * mul; x += 3)
		{
		    red = green = blue = 0;
		    for (s = 0; s < 3; s++)
		    {
			red += filters[rf][s]*in[x+s];
			green += filters[gf][s]*in[x+s];
			blue += filters[bf][s]*in[x+s];
		    }
		    red = red / 65536;
		    green = green / 65536;
		    blue = blue / 65536;
		    out[x/3] = (green << 24) | (red << 16) | (green << 8) | blue;
		}
	    }
	    
	    XRenderAddGlyphs (dpy, font->glyphset, &g, gi, 1, 
			      (char *) bufBitmapRgba, sizergba);
	}
	else
	{
	    XRenderAddGlyphs (dpy, font->glyphset, &g, gi, 1, 
			      (char *) bufBitmap, size);
	}
    }
    if (bufBitmap != bufLocal)
	free (bufBitmap);
    if (bufBitmapRgba != bufLocalRgba)
	free (bufBitmapRgba);
#endif
}

#define STEP	    256

void
XftGlyphCheck (Display		*dpy,
	       XftFontStruct	*font,
	       unsigned long	glyph,
	       unsigned long	*missing,
	       int		*nmissing)
{
#ifdef FREETYPE2
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
	    realized = (XGlyphInfo **) malloc (nrealized * sizeof (XGlyphInfo *));
	if (!realized)
	    return;
	memset (realized + font->nrealized, 0, 
		(nrealized - font->nrealized) *  sizeof (XGlyphInfo *));
	font->realized = realized;
	font->nrealized = nrealized;
    }
    if (!font->realized[glyph])
    {
	font->realized[glyph] = (XGlyphInfo *) malloc (sizeof (XGlyphInfo));
	n = *nmissing;
	missing[n++] = glyph;
	if (n == XFT_NMISSING)
	{
	    XftGlyphLoad (dpy, font, missing, n);
	    n = 0;
	}
	*nmissing = n;
    }
#endif
}

Bool
XftFreeTypeGlyphExists (Display		*dpy,
			XftFontStruct	*font,
			unsigned int	glyph)
{
#ifdef FREETYPE2
    if (font->encoded)
	return FT_Get_Char_Index (font->face, (FT_ULong) glyph) != 0;
    else
	return glyph && glyph <= font->face->num_glyphs;
#else
    return False;
#endif
}
