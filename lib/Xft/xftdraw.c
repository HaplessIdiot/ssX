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

#include <stdlib.h>
#include "xftint.h"

XftDraw *
XftDrawCreate (Display   *dpy,
	       Drawable  drawable,
	       Visual    *visual,
	       Colormap  colormap)
{
    XftDraw	*draw;

    draw = (XftDraw *) malloc (sizeof (XftDraw));
    if (!draw)
	return 0;
    draw->dpy = dpy;
    draw->drawable = drawable;
    draw->visual = visual;
    draw->colormap = colormap;
    draw->core_set = False;
    draw->render_set = False;
    draw->render_able = False;
    return draw;
}

void
XftDrawDestroy (XftDraw	*draw)
{
    if (draw->render_able)
    {
	XRenderFreePicture (draw->dpy, draw->render.pict);
	XFreePixmap (draw->dpy, draw->render.fg_pix);
	XRenderFreePicture (draw->dpy, draw->render.fg_pict);
    }
    if (draw->core_set)
    {
	int	i;
	
	XFreeGC (draw->dpy, draw->core.draw_gc);
	for (i = 0; i < CACHE_SIZE; i++)
	    if (draw->core.u.cache[i].use >= 0)
		XFreeColors (draw->dpy, draw->colormap,
			     &draw->core.u.cache[i].pixel, 1, 0);
    }
    free (draw);
}

Bool
XftDrawRenderPrepare (XftDraw	    *draw,
		      XRenderColor  *color,
		      XftFont	    *font)
{
    if (!draw->render_set)
    {
	XRenderPictFormat	    *format;
	XRenderPictFormat	    pf, *pix_format;
	XRenderPictureAttributes    pa;

	draw->render_set = True;
	draw->render_able = False;
	format = XRenderFindVisualFormat (draw->dpy, draw->visual);
	pf.depth = 32;
	pf.type = PictTypeDirect;
	pix_format = XRenderFindFormat (draw->dpy, 
					PictFormatType|PictFormatDepth, 
					&pf, 
					0);
	if (format && pix_format)
	{
	    draw->render_able = True;

	    draw->render.pict = XRenderCreatePicture (draw->dpy, draw->drawable,
						      format, 0, 0);
	    draw->render.fg_pix = XCreatePixmap (draw->dpy, draw->drawable,
						 1, 1, pix_format->depth);
	    pa.repeat = True;
	    draw->render.fg_pict = XRenderCreatePicture (draw->dpy, 
							 draw->render.fg_pix,
							 pix_format,
							 CPRepeat, &pa);
	    draw->render.fg_color.red = ~color->red;
	}
    }
    if (!draw->render_able)
	return False;
    if (memcmp (color, &draw->render.fg_color, sizeof (XRenderColor)))
    {
	XRenderFillRectangle (draw->dpy, PictOpSrc, draw->render.fg_pict,
			      color, 0, 0, 1, 1);
	draw->render.fg_color = *color;
    }
    return True;
}

static short
maskbase (unsigned long m)
{
    short        i;

    if (!m)
	return 0;
    i = 0;
    while (!(m&1))
    {
	m>>=1;
	i++;
    }
    return i;
}

static short
masklen (unsigned long m)
{
    unsigned long y;

    y = (m >> 1) &033333333333;
    y = m - y - ((y >>1) & 033333333333);
    return (short) (((y + (y >> 3)) & 030707070707) % 077);
}

Bool
XftDrawCorePrepare (XftDraw	    *draw,
		    XRenderColor    *color,
		    XftFont	    *font)
{
    int		    i;
    unsigned long   pixel;
    XGCValues	    gcv;

    if (!draw->core_set)
    {
	draw->core_set = True;

	if (draw->visual->class == TrueColor)
	{
	    draw->core.u.truecolor.red_shift = maskbase (draw->visual->red_mask);
	    draw->core.u.truecolor.red_len = masklen (draw->visual->red_mask);
	    draw->core.u.truecolor.green_shift = maskbase (draw->visual->green_mask);
	    draw->core.u.truecolor.green_len = masklen (draw->visual->green_mask);
	    draw->core.u.truecolor.blue_shift = maskbase (draw->visual->blue_mask);
	    draw->core.u.truecolor.blue_len = masklen (draw->visual->blue_mask);
	}
	else
	{
	    for (i = 0; i < CACHE_SIZE; i++)
		draw->core.u.cache[i].use = -1;
	}
	draw->core.draw_gc = XCreateGC (draw->dpy, draw->drawable, 0, 0);
    }
    if (draw->visual->class == TrueColor)
    {
	pixel = (((color->red >> (16 - draw->core.u.truecolor.red_len)) <<
		  draw->core.u.truecolor.red_shift) |
		 ((color->green >> (16 - draw->core.u.truecolor.green_len)) <<
		  draw->core.u.truecolor.green_shift) |
		 ((color->blue >> (16 - draw->core.u.truecolor.blue_len)) <<
		  draw->core.u.truecolor.blue_shift));
    }
    else
    {
	int	oldest, newest;
	int	match;
	XColor	xcolor;

	oldest = newest = 0;
	match = -1;
	for (i = 1; i < CACHE_SIZE; i++)
	{
	    if (draw->core.u.cache[i].use < draw->core.u.cache[oldest].use)
		oldest = i;
	    if (draw->core.u.cache[i].use > draw->core.u.cache[newest].use)
		newest = i;
	    if (draw->core.u.cache[i].use >= 0 &&
		!memcmp (&draw->core.u.cache[i].color, color, 
			 sizeof (XRenderColor)))
		match = i;
	}
	if (match < 0)
	{
	    match = oldest;
	    if (draw->core.u.cache[match].use >= 0)
	    {
		XFreeColors (draw->dpy, draw->colormap, 
			     &draw->core.u.cache[match].pixel, 1, 0);
		draw->core.u.cache[match].use = -1;
	    }
	    xcolor.red = color->red;
	    xcolor.green = color->green;
	    xcolor.blue = color->blue;
	    xcolor.flags = DoRed|DoGreen|DoBlue;
	    if (!XAllocColor (draw->dpy, draw->colormap, &xcolor))
		return False;
	    draw->core.u.cache[match].pixel = xcolor.pixel;
	    draw->core.u.cache[match].color = *color;
	    draw->core.u.cache[match].use = draw->core.u.cache[newest].use + 1;
	}
	else if (match != newest)
	    draw->core.u.cache[match].use = draw->core.u.cache[newest].use + 1;
	pixel = draw->core.u.cache[match].pixel;
    }
    XGetGCValues (draw->dpy, draw->core.draw_gc, GCForeground|GCFont, &gcv);
    if (gcv.foreground != pixel)
	XSetForeground (draw->dpy, draw->core.draw_gc, pixel);
    if (font && gcv.font != font->u.core.font->fid)
	XSetFont (draw->dpy, draw->core.draw_gc, font->u.core.font->fid);
    return True;
}

void
XftDrawString8 (XftDraw		*draw,
		XRenderColor	*color,
		XftFont		*font,
		int		x,
		int		y,
		unsigned char	*string,
		int		len)
{
    if (font->core)
    {
	XftDrawCorePrepare (draw, color, font);
	XDrawString (draw->dpy, draw->drawable, draw->core.draw_gc, x, y, 
		     (char *) string, len);
    }
    else if (XftDrawRenderPrepare (draw, color, font))
    {
	XftRenderString8 (draw->dpy, draw->render.fg_pict, font->u.ft.font,
			  draw->render.pict, 0, 0, x, y, string, len);
    }
}

#define N16LOCAL    256

void
XftDrawString16 (XftDraw	*draw,
		 XRenderColor	*color,
		 XftFont	*font,
		 int		x,
		 int		y,
		 unsigned short	*string,
		 int		len)
{
    if (font->core)
    {
	XChar2b	    *xc;
	XChar2b	    xcloc[XFT_CORE_N16LOCAL];
	
	XftDrawCorePrepare (draw, color, font);
	xc = XftCoreConvert16 (string, len, xcloc);
	XDrawString16 (draw->dpy, draw->drawable, draw->core.draw_gc, x, y, 
		       xc, len);
	if (xc != xcloc)
	    free (xc);
    }
    else if (XftDrawRenderPrepare (draw, color, font))
    {
	XftRenderString16 (draw->dpy, draw->render.fg_pict, font->u.ft.font,
			   draw->render.pict, 0, 0, x, y, string, len);
    }
}

void
XftDrawString32 (XftDraw	*draw,
		 XRenderColor	*color,
		 XftFont	*font,
		 int		x,
		 int		y,
		 unsigned long	*string,
		 int		len)
{
    if (font->core)
    {
	XChar2b	    *xc;
	XChar2b	    xcloc[XFT_CORE_N16LOCAL];
	
	XftDrawCorePrepare (draw, color, font);
	xc = XftCoreConvert32 (string, len, xcloc);
	XDrawString16 (draw->dpy, draw->drawable, draw->core.draw_gc, x, y, 
		       xc, len);
	if (xc != xcloc)
	    free (xc);
    }
    else if (XftDrawRenderPrepare (draw, color, font))
    {
	XftRenderString32 (draw->dpy, draw->render.fg_pict, font->u.ft.font,
			   draw->render.pict, 0, 0, x, y, string, len);
    }
}

void
XftDrawRect (XftDraw	    *draw,
	     XRenderColor   *color,
	     int	    x, 
	     int	    y,
	     unsigned int   width,
	     unsigned int   height)
{
    if (XftDrawRenderPrepare (draw, color, 0))
    {
	XRenderFillRectangle (draw->dpy, PictOpSrc, draw->render.pict,
			      color, x, y, width, height);
    }
    else
    {
	XftDrawCorePrepare (draw, color, 0);
	XFillRectangle (draw->dpy, draw->drawable, draw->core.draw_gc,
			x, y, width, height);
    }
}
