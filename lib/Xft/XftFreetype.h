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

#ifndef _XFTFREETYPE_H_
#define _XFTFREETYPE_H_

#include "Xft.h"
#ifdef FREETYPE2
#include <freetype/freetype.h>
#endif

struct _XftFontStruct {
#ifdef FREETYPE2
    FT_Face		face;      /* handle to face object */
#endif
    GlyphSet		glyphset;
    int			min_char;
    int			max_char;
    int			size;
    int			ascent;
    int			descent;
    int			height;
    int			max_advance_width;
    Bool		monospace;
    int			rgba;
    Bool		antialias;
    XRenderPictFormat	*format;
    XGlyphInfo		**realized;
    int			nrealized;
};

/* xftglyphs.c */
void
XftGlyphLoad (Display		*dpy,
	      XftFontStruct	*font,
	      unsigned long	*glyphs,
	      int		nglyph);

void
XftGlyphCheck (Display		*dpy,
	       XftFontStruct	*font,
	       unsigned long	glyph,
	       unsigned long	*missing,
	       int		*nmissing);

/* xftrender.c */

void
XftRenderString8 (Display *dpy, Picture src, 
		  XftFontStruct *font, Picture dst,
		  int srcx, int srcy,
		  int x, int y,
		  unsigned char *string, int len);

void
XftRenderString16 (Display *dpy, Picture src, 
		   XftFontStruct *font, Picture dst,
		   int srcx, int srcy,
		   int x, int y,
		   unsigned short *string, int len);

void
XftRenderString32 (Display *dpy, Picture src, 
		   XftFontStruct *font, Picture dst,
		   int srcx, int srcy,
		   int x, int y,
		   unsigned long *string, int len);

void
XftRenderExtents8 (Display	    *dpy,
		   XftFontStruct    *font,
		   unsigned char    *string, 
		   int		    len,
		   XGlyphInfo	    *extents);

void
XftRenderExtents16 (Display	    *dpy,
		    XftFontStruct   *font,
		    unsigned short  *string, 
		    int		    len,
		    XGlyphInfo	    *extents);

void
XftRenderExtents32 (Display	    *dpy,
		    XftFontStruct   *font,
		    unsigned long   *string, 
		    int		    len,
		    XGlyphInfo	    *extents);

XftFontStruct *
XftFreeTypeGet (XftFont *font);

#endif /* _XFTFREETYPE_H_ */
