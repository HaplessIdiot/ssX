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

FT_Library  _XftFTlibrary;

XftFont *
XftLoadFont (Display *dpy, XftFontName *name)
{
    XftFontName		resolved;
    XftFont		*font;
    XRenderPictFormat	pf, *format;
    int			height, ascent, descent;
    int			extra;
    int			div;

    pf.depth = 8;
    pf.type = PictTypeDirect;
    pf.direct.alpha = 0;
    pf.direct.alphaMask = 0xff;
    format = XRenderFindFormat(dpy, 
			       PictFormatType|
			       PictFormatDepth|
			       PictFormatAlpha|
			       PictFormatAlphaMask,
			       &pf, 0);
    if (!format)
	goto bail0;
    if (!XftMatch (name, &resolved))
	goto bail1;
    if ((resolved.mask & XftFontNameFile) == 0)
	goto bail1;
    if ((resolved.mask & XftFontNameSize) == 0)
	resolved.size = 12 * 64;
    if ((resolved.mask & XftFontNameRotation) == 0)
	resolved.rotation = 0;
    if ((resolved.mask & XftFontNameSpacing) == 0)
	resolved.spacing = XftFontSpacingAny;
    font = malloc (sizeof (XftFont));
    if (!font)
	goto bail1;
    if (!_XftFTlibrary)
    {
	if (FT_Init_FreeType( &_XftFTlibrary ))
	    goto bail2;
    }

    if (FT_New_Face( _XftFTlibrary, resolved.file, 0, &font->face))
	goto bail1;
    
    font->size = resolved.size;
    font->min_char = 0;
    font->max_char = 255;
    font->realized = 0;
    font->nrealized = 0;
    font->monospace = resolved.spacing != XftFontSpacingAny;
    
    height = font->face->height;
    ascent = font->face->ascender;
    descent = font->face->descender;
    if (descent < 0) descent = - descent;
    extra = (height - (ascent + descent));
    if (extra > 0)
    {
	ascent = ascent + extra / 2;
	descent = height - ascent;
    }
    else if (extra < 0)
	height = ascent + descent;
    div = font->face->units_per_EM;
    if (height > div * 5)
	div *= 10;
    
    font->descent = descent * font->size / (64 * div);
    font->ascent = ascent * font->size / (64 * div);
    font->height = height * font->size / (64 * div);
    font->max_advance_width = font->face->max_advance_width * font->size / (64 * div);
    
#if 0
    if ( FT_Set_Char_Size (font->face, 0, font->size, 72, 72))
	goto bail3;
#endif
    if ( FT_Set_Pixel_Sizes (font->face, font->size >> 6, font->size >> 6))
	goto bail3;
#if 0
    for (charmap = 0; charmap < font->face->num_charmaps; charmap++)
	if (font->face->charmaps[charmap]->encoding == ft_encoding_unicode)
	    break;
    
    if (charmap == font->face->num_charmaps)
	goto bail3;

    error = FT_Set_Charmap(font->face,
			   font->face->charmaps[charmap]);

    if (error)
	goto bail3;
#endif
    font->glyphset = XRenderCreateGlyphSet (dpy, format);

    return font;
bail3:
    FT_Done_Face (font->face);
bail2:
    free (font);
bail1:
    if (resolved.face != name->face)
	free (resolved.face);
    if (resolved.file != name->file)
	free (resolved.file);
    if (resolved.encoding != name->encoding)
	free (resolved.encoding);
bail0:
    return 0;
}

void
XftFreeFont (Display *dpy, XftFont *font)
{
    FT_Done_Face (font->face);
    free (font);
}
