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

static XcursorDisplayInfo *_XcursorDisplayInfo;

static int
_XcursorCloseDisplay (Display *dpy, XExtCodes *codes)
{
    XcursorDisplayInfo  *info, **prev;

    /*
     * Unhook from the global list
     */
    _XLockMutex (_Xglobal_lock);
    for (prev = &_XcursorDisplayInfo; (info = *prev); prev = &(*prev)->next)
	if (info->display == dpy)
	{
            *prev = info->next;
	    break;
	}
    _XUnlockMutex (_Xglobal_lock);
    
    free (info);
    return 0;
}

XcursorDisplayInfo *
_XcursorGetDisplayInfo (Display *dpy)
{
    XcursorDisplayInfo	*info, **prev;
    int			event_base, error_base;
    int			major, minor;
    char		*v;

    _XLockMutex (_Xglobal_lock);
    for (prev = &_XcursorDisplayInfo; (info = *prev); prev = &(*prev)->next)
    {
	if (info->display == dpy)
	{
	    /*
	     * MRU the list
	     */
	    if (prev != &_XcursorDisplayInfo)
	    {
		*prev = info->next;
		info->next = _XcursorDisplayInfo;
		_XcursorDisplayInfo = info;
	    }
	    break;
	}
    }
    _XUnlockMutex (_Xglobal_lock);
    if (info)
        return info;
    info = (XcursorDisplayInfo *) malloc (sizeof (XcursorDisplayInfo));
    if (!info)
	return 0;
    info->next = 0;
    info->display = dpy;
    
    info->codes = XAddExtension (dpy);
    if (!info->codes)
    {
	free (info);
	return 0;
    }
    (void) XESetCloseDisplay (dpy, info->codes->extension, _XcursorCloseDisplay);

    /*
     * Check whether the display supports the Render CreateCursor request
     */
    if (XRenderQueryExtension (dpy, &event_base, &error_base) &&
	XRenderQueryVersion (dpy, &major, &minor) &&
	(major > 0 || minor >= 5))
    {
	if (getenv ("XCURSOR_CORE"))
	    info->has_render_cursor = XcursorFalse;
	else
	    info->has_render_cursor = XcursorTrue;
    }
    else
	info->has_render_cursor = XcursorFalse;
    
    info->size = 0;

    /*
     * Get desired cursor size
     */
    v = getenv ("XCURSOR_SIZE");
    if (!v)
	v = XGetDefault (dpy, "Xcursor", "size");
    if (v)
	info->size = atoi (v);
    
    /*
     * Use the Xft size to guess a size; make cursors 16 "points" tall
     */
    if (info->size == 0)
    {
	int dpi = 0;
	v = XGetDefault (dpy, "Xft", "dpi");
	if (v)
	    dpi = atoi (v);
	if (dpi)
	    info->size = dpi * 16 / 72;
    }
    
    /*
     * Use display size to guess a size
     */
    if (info->size == 0)
    {
	int dim;
	    
	if (DisplayHeight (dpy, DefaultScreen (dpy)) < 
	    DisplayWidth (dpy, DefaultScreen (dpy)))
	    dim = DisplayHeight (dpy, DefaultScreen (dpy));
	else
	    dim = DisplayWidth (dpy, DefaultScreen (dpy));
	/*
	 * 16 pixels on a display of dimension 768
	 */
	info->size = dim / 48;
    }
    
    info->theme = 0;

    /*
     * Get the desired theme
     */
    v = getenv ("XCURSOR_THEME");
    if (!v)
	v = XGetDefault (dpy, "Xcursor", "theme");
    if (v)
    {
	info->theme = malloc (strlen (v) + 1);
	if (info->theme)
	    strcpy (info->theme, v);
    }

    info->fonts = 0;

    /*
     * Link new info info list
     */
    _XLockMutex (_Xglobal_lock);
    info->next = _XcursorDisplayInfo;
    _XcursorDisplayInfo = info;
    _XUnlockMutex (_Xglobal_lock);
    
    return info;
}

XcursorBool
XcursorSupportsARGB (Display *dpy)
{
    XcursorDisplayInfo	*info = _XcursorGetDisplayInfo (dpy);

    return info && info->has_render_cursor;
}

XcursorBool
XcursorSetDefaultSize (Display *dpy, int size)
{
    XcursorDisplayInfo	*info = _XcursorGetDisplayInfo (dpy);

    if (!info)
	return XcursorFalse;
    info->size = size;
    return XcursorTrue;
}

int
XcursorGetDefaultSize (Display *dpy)
{
    XcursorDisplayInfo	*info = _XcursorGetDisplayInfo (dpy);

    if (!info)
	return 0;
    return info->size;
}

XcursorBool
XcursorSetTheme (Display *dpy, const char *theme)
{
    XcursorDisplayInfo	*info = _XcursorGetDisplayInfo (dpy);
    char		*copy;

    if (!info)
	return XcursorFalse;
    copy = malloc (strlen (theme) + 1);
    if (!copy)
	return XcursorFalse;
    strcpy (copy, theme);
    if (info->theme)
	free (info->theme);
    info->theme = copy;
    return XcursorTrue;
}

char *
XcursorGetTheme (Display *dpy)
{
    XcursorDisplayInfo	*info = _XcursorGetDisplayInfo (dpy);

    if (!info)
	return 0;
    return info->theme;
}
