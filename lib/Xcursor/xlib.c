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
#include <X11/Xatom.h>
#include <stdlib.h>

static XcursorBool
_XcursorFontIsCursor (Display *dpy, Font font)
{
    XcursorFontInfo	*fi;
    XcursorDisplayInfo	*info;
    XcursorBool		ret;
    XFontStruct		*fs;
    int			n;
    Atom		cursor;

    if (font == dpy->cursor_font)
	return XcursorTrue;

    info = _XcursorGetDisplayInfo (dpy);
    if (!info)
	return XcursorFalse;
    LockDisplay (dpy);
    for (fi = info->fonts; fi; fi = fi->next)
	if (fi->font == font)
	{
	    ret = fi->is_cursor_font;
	    UnlockDisplay (dpy);
	    return ret;
	}
    UnlockDisplay (dpy);
    ret = XcursorFalse;
    fs = XQueryFont (dpy, font);
    if (fs)
    {
	cursor = XInternAtom (dpy, "cursor", False);
	for (n = 0; n < fs->n_properties; n++)
	    if (fs->properties[n].name == XA_FONT)
	    {
		ret = (fs->properties[n].card32 == cursor);
		break;
	    }
    }
    fi = malloc (sizeof (XcursorFontInfo));
    if (fi)
    {
	fi->font = font;
	fi->is_cursor_font = ret;
	LockDisplay (dpy);
	fi->next = info->fonts;
	info->fonts = fi;
	UnlockDisplay (dpy);
    }
    return ret;
}

Cursor
XcursorTryShapeCursor (Display	    *dpy,
		       Font	    source_font,
		       Font	    mask_font,
		       unsigned int source_char,
		       unsigned int mask_char,
		       XColor _Xconst *foreground,
		       XColor _Xconst *background)
{
    Cursor  cursor = None;
    
    if (source_font == mask_font && 
	_XcursorFontIsCursor (dpy, source_font) &&
	source_char + 1 == mask_char)
    {
	int		size = XcursorGetDefaultSize (dpy);
	char		*theme = XcursorGetTheme (dpy);
	XcursorImage    *image = XcursorShapeLoadImage (source_char, theme, size);

	if (image)
	{
	    cursor = XcursorImageLoadCursor (dpy, image);
	    XcursorImageDestroy (image);
	}
    }
    return cursor;
}
