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

#ifndef _XFIXES_H_
#define _XFIXES_H_

#include <X11/extensions/xfixeswire.h>

#include <X11/Xfuncproto.h>

typedef struct {
    int type;			/* event base */
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int subtype;
    Window owner;
    Atom selection;
    Time timestamp;
    Time selection_timestamp;
} XFixesSelectionNotifyEvent;

typedef struct {
    int type;			/* event base */
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int subtype;
    unsigned long cursor_serial;
    Time timestamp;
} XFixesCursorNotifyEvent;

typedef struct {
    short	    x, y;
    unsigned short  width, height;
    unsigned short  xhot, yhot;
    unsigned long   cursor_serial;
    unsigned long   *pixels;
} XFixesCursorImage;

_XFUNCPROTOBEGIN

Bool XFixesQueryExtension (Display *dpy, int *event_basep, int *error_basep);
Status XFixesQueryVersion (Display *dpy,
			    int     *major_versionp,
			    int     *minor_versionp);

void
XFixesChangeSaveSet (Display	*dpy,
		     Window	win,
		     int	mode,
		     int	target,
		     int	map);

void
XFixesSelectSelectionInput (Display	    *dpy,
			    Window	    win,
			    Atom	    selection, 
			    unsigned long   eventMask);

void
XFixesSelectCursorInput (Display	*dpy,
			 Window		win,
			 unsigned long	eventMask);

XFixesCursorImage *
XFixesGetCursorImage (Display *dpy);

_XFUNCPROTOEND

#endif /* _XFIXES_H_ */
