/*
 * $XFree86: $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
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

#include "xcursorint.h"
#include <X11/Xlibint.h>
#include <X11/Xutil.h>

XcursorCursors *
XcursorCursorsCreate (Display *dpy, int size)
{
    XcursorCursors  *cursors;

    cursors = malloc (sizeof (XcursorCursors) +
		      size * sizeof (Cursor));
    if (!cursors)
	return 0;
    cursors->ref = 1;
    cursors->dpy = dpy;
    cursors->ncursor = 0;
    cursors->cursors = (Cursor *) (cursors + 1);
    return cursors;
}

void
XcursorCursorsDestroy (XcursorCursors *cursors)
{
    int	    n;

    --cursors->ref;
    if (cursors->ref > 0)
	return;
    
    for (n = 0; n < cursors->ncursor; n++)
	XFreeCursor (cursors->dpy, cursors->cursors[n]);
    free (cursors);
}

XcursorAnimate *
XcursorAnimateCreate (XcursorCursors *cursors)
{
    XcursorAnimate  *animate;

    animate = malloc (sizeof (XcursorAnimate));
    if (!animate)
	return 0;
    animate->cursors = cursors;
    cursors->ref++;
    animate->sequence = 0;
    return animate;
}

void
XcursorAnimateDestroy (XcursorAnimate *animate)
{
    XcursorCursorsDestroy (animate->cursors);
    free (animate);
}

Cursor
XcursorAnimateNext (XcursorAnimate *animate)
{
    Cursor  cursor = animate->cursors->cursors[animate->sequence++];

    if (animate->sequence > animate->cursors->ncursor)
	animate->sequence = 0;
    return cursor;
}

static int
nativeByteOrder (void)
{
    int	x = 1;

    return (*((char *) &x) == 1) ? LSBFirst : MSBFirst;
}

static XcursorUInt
_XcursorPixelBrightness (XcursorPixel p)
{
    XcursorPixel    alpha = p >> 24;
    XcursorPixel    r, g, b;

    if (alpha < 0x80)
	return ~0;
    r = ((p >> 8) & 0xff00) / alpha;
    g = ((p >> 0) & 0xff00) / alpha;
    b = ((p << 8) & 0xff00) / alpha;
    return r * 153 + g * 301 + b * 58;
}

static unsigned short
_XcursorDivideAlpha (XcursorUInt value, XcursorUInt alpha)
{
    if (!alpha)
	return 0;
    value = value * 255 / alpha;
    if (value > 255)
	value = 255;
    return value | (value << 8);
}

static void
_XcursorPixelToColor (XcursorPixel p, XColor *color)
{
    XcursorPixel    alpha = p >> 24;

    color->pixel = 0;
    color->red =   _XcursorDivideAlpha ((p >> 16) & 0xff, alpha);
    color->green = _XcursorDivideAlpha ((p >>  8) & 0xff, alpha);
    color->blue =  _XcursorDivideAlpha ((p >>  0) & 0xff, alpha);
    color->flags = DoRed|DoGreen|DoBlue;
}

#if 0
void
_XcursorDumpImage (XImage *image)
{
    FILE    *f = fopen ("/tmp/images", "a");
    int	    x, y;
    if (!f)
	return;
    fprintf (f, "%d x %x\n", image->width, image->height);
    for (y = 0; y < image->height; y++)
    {
	for (x = 0; x < image->width; x++)
	    fprintf (f, "%c", XGetPixel (image, x, y) ? '*' : ' ');
	fprintf (f, "\n");
    }
    fflush (f);
    fclose (f);
}

void
_XcursorDumpColor (XColor *color, char *name)
{
    FILE    *f = fopen ("/tmp/images", "a");
    fprintf (f, "%s: %x %x %x\n", name,
	     color->red, color->green, color->blue);
    fflush (f);
    fclose (f);
}
#endif

Cursor
XcursorImageLoadCursor (Display *dpy, const XcursorImage *image)
{
    Cursor  cursor;
    
#if RENDER_MAJOR > 0 || RENDER_MINOR >= 5
    if (XcursorSupportsARGB (dpy))
    {
	XImage		    ximage;
	int		    screen = DefaultScreen (dpy);
	Pixmap		    pixmap;
	Picture		    picture;
	GC		    gc;
	XRenderPictFormat   *format;

	ximage.width = image->width;
	ximage.height = image->height;
	ximage.xoffset = 0;
	ximage.format = ZPixmap;
	ximage.data = (char *) image->pixels;
	ximage.byte_order = nativeByteOrder ();
	ximage.bitmap_unit = 32;
	ximage.bitmap_bit_order = ximage.byte_order;
	ximage.bitmap_pad = 32;
	ximage.depth = 32;
	ximage.bits_per_pixel = 32;
	ximage.bytes_per_line = image->width * 4;
	ximage.red_mask = 0xff0000;
	ximage.green_mask = 0x00ff00;
	ximage.blue_mask = 0x0000ff;
	ximage.obdata = 0;
	if (!XInitImage (&ximage))
	    return None;
	pixmap = XCreatePixmap (dpy, RootWindow (dpy, screen),
				image->width, image->height, 32);
	gc = XCreateGC (dpy, pixmap, 0, 0);
	XPutImage (dpy, pixmap, gc, &ximage, 
		   0, 0, 0, 0, image->width, image->height);
	XFreeGC (dpy, gc);
	format = XRenderFindStandardFormat (dpy, PictStandardARGB32);
	picture = XRenderCreatePicture (dpy, pixmap, format, 0, 0);
	XFreePixmap (dpy, pixmap);
	cursor = XRenderCreateCursor (dpy, picture, 
				      image->xhot, image->yhot);
	XRenderFreePicture (dpy, picture);
    }
    else
#endif
    {
	int		screen = DefaultScreen (dpy);
	XImage		*src_image, *msk_image;
	Pixmap		src_pixmap, msk_pixmap;
	GC		gc;
	XGCValues	gcv;
	XcursorPixel	on_pixel, off_pixel, pixel;
	XcursorUInt	on_inten, off_inten, inten;
	XColor		on_color, off_color;
	int		num = image->width * image->height, n;
	int		x, y;
	XcursorPixel	*p;
	
	src_image = XCreateImage (dpy, 0, 1, ZPixmap,
				  0, 0, image->width, image->height,
				  32, 0);
	src_image->data = Xmalloc (image->height * src_image->bytes_per_line);
	msk_image = XCreateImage (dpy, 0, 1, ZPixmap,
				  0, 0, image->width, image->height,
				  32, 0);
	msk_image->data = Xmalloc (image->height * msk_image->bytes_per_line);

	/*
	 * Find the brightest and dimmest colors
	 */
	on_pixel = off_pixel = image->pixels[0];
	on_inten = off_inten = _XcursorPixelBrightness (on_pixel);
	p = image->pixels + 1;
	n = num - 1;
	while (n--)
	{
	    pixel = *p++;
	    inten = _XcursorPixelBrightness (pixel);
	    if (inten != ~0)
	    {
		if (on_inten == ~0 || inten > on_inten)
		{
		    on_inten = inten;
		    on_pixel = pixel;
		}
		if (off_inten == ~0 || inten < off_inten)
		{
		    off_inten = inten;
		    off_pixel = pixel;
		}
	    }
	}
	/*
	 * Map every pixel to either the brighest or dimmest pixel
	 */
	p = image->pixels;
	for (y = 0; y < image->height; y++)
	{
	    for (x = 0; x < image->width; x++)
	    {
		pixel = *p++;
		inten = _XcursorPixelBrightness (pixel);
		if (inten == ~0)
		{
		    XPutPixel (msk_image, x, y, 0);
		    XPutPixel (src_image, x, y, 0);
		}
		else
		{
		    XPutPixel (msk_image, x, y, 1);
		    if (on_inten - inten < inten - off_inten)
			XPutPixel (src_image, x, y, 0);
		    else
			XPutPixel (src_image, x, y, 1);
		}		    
	    }
	}
	/*
	 * Create the cursor
	 */
	src_pixmap = XCreatePixmap (dpy, RootWindow (dpy, screen),
				    image->width, image->height, 1);
	msk_pixmap = XCreatePixmap (dpy, RootWindow (dpy, screen),
				    image->width, image->height, 1);
	gcv.foreground = 1;
	gcv.background = 0;
	gc = XCreateGC (dpy, src_pixmap, 
			GCForeground|GCBackground,
			&gcv);
	XPutImage (dpy, src_pixmap, gc, src_image,
		   0, 0, 0, 0, image->width, image->height);
	
	XPutImage (dpy, msk_pixmap, gc, msk_image,
		   0, 0, 0, 0, image->width, image->height);
	XFreeGC (dpy, gc);
	XDestroyImage (src_image);
	XDestroyImage (msk_image);

	_XcursorPixelToColor (on_pixel, &on_color);
	_XcursorPixelToColor (off_pixel, &off_color);
	cursor = XCreatePixmapCursor (dpy, src_pixmap, msk_pixmap,
				      &off_color, &on_color,
				      image->xhot, image->yhot);
	XFreePixmap (dpy, src_pixmap);
	XFreePixmap (dpy, msk_pixmap);
    }
    return cursor;
}

XcursorCursors *
XcursorImagesLoadCursors (Display *dpy, const XcursorImages *images)
{
    XcursorCursors  *cursors = XcursorCursorsCreate (dpy, images->nimage);
    int		    n;

    if (!cursors)
	return 0;
    for (n = 0; n < images->nimage; n++)
    {
	cursors->cursors[n] = XcursorImageLoadCursor (dpy, images->images[n]);
	if (!cursors->cursors[n])
	{
	    XcursorCursorsDestroy (cursors);
	    return 0;
	}
	cursors->ncursor++;
    }
    return cursors;
}

Cursor
XcursorFilenameLoadCursor (Display *dpy, const char *file)
{
    int		    size = XcursorGetDefaultSize (dpy);
    XcursorImage    *image = XcursorFilenameLoadImage (file, size);
    Cursor	    cursor;
    
    if (!image)
	return None;
    cursor = XcursorImageLoadCursor (dpy, image);
    XcursorImageDestroy (image);
    return cursor;
}

XcursorCursors *
XcursorFilenameLoadCursors (Display *dpy, const char *file)
{
    int		    size = XcursorGetDefaultSize (dpy);
    XcursorImages   *images = XcursorFilenameLoadImages (file, size);
    XcursorCursors  *cursors;
    
    if (!images)
	return 0;
    cursors = XcursorImagesLoadCursors (dpy, images);
    XcursorImagesDestroy (images);
    return cursors;
}

Cursor
XcursorLibraryLoadCursor (Display *dpy, const char *file)
{
    int		    size = XcursorGetDefaultSize (dpy);
    char	    *theme = XcursorGetTheme (dpy);
    XcursorImage    *image = XcursorLibraryLoadImage (file, theme, size);
    Cursor	    cursor;
    
    if (!image)
	return None;
    cursor = XcursorImageLoadCursor (dpy, image);
    XcursorImageDestroy (image);
    return cursor;
}

XcursorCursors *
XcursorLibraryLoadCursors (Display *dpy, const char *file)
{
    int		    size = XcursorGetDefaultSize (dpy);
    char	    *theme = XcursorGetTheme (dpy);
    XcursorImages   *images = XcursorLibraryLoadImages (file, theme, size);
    XcursorCursors  *cursors;
    
    if (!images)
	return 0;
    cursors = XcursorImagesLoadCursors (dpy, images);
    XcursorImagesDestroy (images);
    return cursors;
}

/*
 * Stolen from XCreateGlyphCursor (which we cruelly override)
 */

Cursor
_XcursorCreateGlyphCursor(Display	    *dpy,
			  Font		    source_font,
			  Font		    mask_font,
			  unsigned int	    source_char,
			  unsigned int	    mask_char,
			  XColor _Xconst    *foreground,
			  XColor _Xconst    *background)
{       
    Cursor cid;
    register xCreateGlyphCursorReq *req;

    LockDisplay(dpy);
    GetReq(CreateGlyphCursor, req);
    cid = req->cid = XAllocID(dpy);
    req->source = source_font;
    req->mask = mask_font;
    req->sourceChar = source_char;
    req->maskChar = mask_char;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
    return (cid);
}

/*
 * Stolen from XCreateFontCursor (which we cruelly override)
 */

static Cursor
_XcursorCreateFontCursor (Display *dpy, unsigned int shape)
{
    static XColor _Xconst foreground = { 0,    0,     0,     0  };  /* black */
    static XColor _Xconst background = { 0, 65535, 65535, 65535 };  /* white */

    /* 
     * the cursor font contains the shape glyph followed by the mask
     * glyph; so character position 0 contains a shape, 1 the mask for 0,
     * 2 a shape, etc.  <X11/cursorfont.h> contains hash define names
     * for all of these.
     */

    if (dpy->cursor_font == None) 
    {
	dpy->cursor_font = XLoadFont (dpy, CURSORFONT);
	if (dpy->cursor_font == None)
	    return None;
    }

    return _XcursorCreateGlyphCursor (dpy, dpy->cursor_font, dpy->cursor_font, 
				      shape, shape + 1, &foreground, &background);
}

Cursor
XcursorShapeLoadCursor (Display *dpy, unsigned int shape)
{
    int		    size = XcursorGetDefaultSize (dpy);
    char	    *theme = XcursorGetTheme (dpy);
    XcursorImage    *image = XcursorShapeLoadImage (shape, theme, size);
    Cursor	    cursor;
    
    if (image)
    {
	cursor = XcursorImageLoadCursor (dpy, image);
	XcursorImageDestroy (image);
    }
    else
	cursor = None;
    if (cursor == None)
	cursor = _XcursorCreateFontCursor (dpy, shape);
    return cursor;
}

XcursorCursors *
XcursorShapeLoadCursors (Display *dpy, unsigned int shape)
{
    int		    size = XcursorGetDefaultSize (dpy);
    char	    *theme = XcursorGetTheme (dpy);
    XcursorImages   *images = XcursorShapeLoadImages (shape, theme, size);
    XcursorCursors  *cursors;
    
    if (images)
    {
	cursors = XcursorImagesLoadCursors (dpy, images);
	XcursorImagesDestroy (images);
    }
    else
	cursors = 0;
    if (!cursors)
    {
	cursors = XcursorCursorsCreate (dpy, 1);
	cursors->cursors[0] = _XcursorCreateFontCursor (dpy, shape);
	if (cursors->cursors[0] == None)
	{
	    XcursorCursorsDestroy (cursors);
	    cursors = 0;
	}
	else
	    cursors->ncursor = 1;
    }
    return cursors;
}
