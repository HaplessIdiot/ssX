/*
 * $XFree86: xc/lib/Xft/xftdraw.c,v 1.4 2000/12/01 21:32:01 keithp Exp $
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
#include <X11/Xutil.h>

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
    draw->clip = 0;
    return draw;
}

void
XftDrawChange (XftDraw	*draw,
	       Drawable	drawable)
{
    draw->drawable = drawable;
    if (draw->render_able)
    {
	XRenderPictFormat	    *format;
	
	XRenderFreePicture (draw->dpy, draw->render.pict);
	format = XRenderFindVisualFormat (draw->dpy, draw->visual);
	draw->render.pict = XRenderCreatePicture (draw->dpy, draw->drawable,
						  format, 0, 0);
    }
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
	XFreeGC (draw->dpy, draw->core.draw_gc);
    if (draw->clip)
	XDestroyRegion (draw->clip);
    free (draw);
}

Bool
XftDrawRenderPrepare (XftDraw	*draw,
		      XftColor	*color,
		      XftFont	*font)
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
	    draw->render.fg_color.red = ~color->color.red;
	    if (draw->clip)
	    XRenderSetPictureClipRegion (draw->dpy, draw->render.pict,
					 draw->clip);
	}
    }
    if (!draw->render_able)
	return False;
    if (memcmp (&color->color, &draw->render.fg_color, sizeof (XRenderColor)))
    {
	XRenderFillRectangle (draw->dpy, PictOpSrc, draw->render.fg_pict,
			      &color->color, 0, 0, 1, 1);
	draw->render.fg_color = color->color;
    }
    return True;
}

Bool
XftDrawCorePrepare (XftDraw	*draw,
		    XftColor	*color,
		    XftFont	*font)
{
    XGCValues	    gcv;

    if (!draw->core_set)
    {
	draw->core_set = True;

	draw->core.fg = color->pixel;
	gcv.foreground = draw->core.fg;
	draw->core.draw_gc = XCreateGC (draw->dpy, draw->drawable, 
					GCForeground, &gcv);
	if (draw->clip)
	    XSetRegion (draw->dpy, draw->core.draw_gc, draw->clip);
    }
    if (draw->core.fg != color->pixel)
	XSetForeground (draw->dpy, draw->core.draw_gc, color->pixel);
    if (font && gcv.font != font->u.core.font->fid)
	XSetFont (draw->dpy, draw->core.draw_gc, font->u.core.font->fid);
    return True;
}

void
XftDrawString8 (XftDraw		*draw,
		XftColor	*color,
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
		 XftColor	*color,
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
		 XftColor	*color,
		 XftFont	*font,
		 int		x,
		 int		y,
		 unsigned int	*string,
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
	     XftColor	    *color,
	     int	    x, 
	     int	    y,
	     unsigned int   width,
	     unsigned int   height)
{
    if (XftDrawRenderPrepare (draw, color, 0))
    {
	XRenderFillRectangle (draw->dpy, PictOpSrc, draw->render.pict,
			      &color->color, x, y, width, height);
    }
    else
    {
	XftDrawCorePrepare (draw, color, 0);
	XFillRectangle (draw->dpy, draw->drawable, draw->core.draw_gc,
			x, y, width, height);
    }
}

Bool
XftDrawSetClip (XftDraw	*draw,
		Region	r)
{
    Region			n = 0;

    if (!XEmptyRegion (r))
    {
	n = XCreateRegion ();
	if (n)
	{
	    if (!XUnionRegion (n, r, n))
	    {
		XDestroyRegion (n);
		return False;
	    }
	}
    }
    if (draw->clip)
    {
	XDestroyRegion (draw->clip);
    }
    draw->clip = n;
    if (draw->render_able)
    {
	XRenderPictureAttributes	pa;
        if (n)
	{
	    XRenderSetPictureClipRegion (draw->dpy, draw->render.pict, n);
	}
	else
	{
	    pa.clip_mask = None;
	    XRenderChangePicture (draw->dpy, draw->render.pict,
				  CPClipMask, &pa);
	}
    }
    if (draw->core_set)
    {
	XGCValues   gv;
	
	if (n)
	    XSetRegion (draw->dpy, draw->core.draw_gc, n);
	else
	{
	    gv.clip_mask = None;
	    XChangeGC (draw->dpy, draw->core.draw_gc,
		       GCClipMask, &gv);
	}
    }
    return True;
}
