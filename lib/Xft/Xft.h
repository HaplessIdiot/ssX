/*
 * $XFree86: xc/lib/Xft/Xft.h,v 1.1 2000/10/05 18:05:26 keithp Exp $
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

#ifndef _XFT_H_
#define _XFT_H_

typedef struct _XftFontName {
    unsigned long   mask;
    char	    *face;
    char	    *encoding;
    char	    *file;
    int		    size;
    int		    rotation;
    int		    spacing;
} XftFontName;

#define XftFontNameFace	    0x1
#define XftFontNameEncoding 0x2
#define XftFontNameFile	    0x4
#define XftFontNameSize	    0x8
#define XftFontNameRotation 0x10
#define XftFontNameSpacing  0x20

#define XftFontSpacingAny   0
#define XftFontSpacingMono  1
#define XftFontSpacingCell  2

typedef struct _XftFont	XftFont;

/* load.c */
XftFont	*
XftLoadFont (Display *dpy, XftFontName *name);

void
XftFreeFont (Display *dpy, XftFont *font);

/* metrics.c */
int
XftFontAscent(Display *dpy, XftFont *font);

int
XftFontDescent(Display *dpy, XftFont *font);

int
XftFontHeight(Display *dpy, XftFont *font);

int
XftFontMaxAdvanceWidth (Display *dpy, XftFont *font);
    
void
XftExtentsString (Display	*dpy,
		  XftFont	*font,
		  unsigned char	*string, 
		  int		len,
		  XGlyphInfo	*extents);

void
XftDrawString (Display		*dpy,
	       Picture		src,
	       XftFont		*font,
	       Picture		dst,
	       int		srcx,
	       int		srcy,
	       int		x,
	       int		y,
	       unsigned char	*string,
	       int		len);

#endif /* _XFT_H_ */
