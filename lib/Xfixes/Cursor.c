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

#include "Xfixesint.h"

void
XFixesSelectCursorInput (Display	*dpy,
			 Window		win,
			 unsigned long	eventMask)
{
    XExtDisplayInfo		    *info = XFixesFindDisplay (dpy);
    xXFixesSelectCursorInputReq	    *req;

    XFixesSimpleCheckExtension (dpy, info);

    LockDisplay (dpy);
    GetReq (XFixesSelectCursorInput, req);
    req->reqType = info->codes->major_opcode;
    req->xfixesReqType = X_XFixesSelectCursorInput;
    req->window = win;
    req->eventMask = eventMask;
    UnlockDisplay (dpy);
    SyncHandle ();
}

XFixesCursorImage *
XFixesGetCursorImage (Display *dpy)
{
    XExtDisplayInfo		    *info = XFixesFindDisplay (dpy);
    xXFixesGetCursorImageReq	    *req;
    xXFixesGetCursorImageReply	    rep;
    int				    npixels;
    int				    nbytes, nread, rlength;
    XFixesCursorImage		    *image;

    XFixesCheckExtension (dpy, info, 0);
    LockDisplay (dpy);
    GetReq (XFixesGetCursorImage, req);
    req->reqType = info->codes->major_opcode;
    req->xfixesReqType = X_XFixesGetCursorImage;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse))
    {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    npixels = rep.width * rep.height;
    /* reply data length */
    nbytes = (long) rep.length << 2;
    /* bytes of actual data in the reply */
    nread = (npixels) << 2;
    /* size of data returned to application */
    rlength = sizeof (XFixesCursorImage) + npixels * sizeof (unsigned long);

    image = (XFixesCursorImage *) Xmalloc (rlength);
    if (!image)
    {
	_XEatData (dpy, nbytes);
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    image->x = rep.x;
    image->y = rep.y;
    image->width = rep.width;
    image->height = rep.height;
    image->xhot = rep.xhot;
    image->yhot = rep.yhot;
    image->cursor_serial = rep.cursorSerial;
    image->pixels = (unsigned long *) (image + 1);
    _XRead32 (dpy, image->pixels, nread);
    /* skip any padding */
    if(nbytes > nread)
    {
	_XEatData (dpy, (unsigned long) (nbytes - nread));
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    return image;
}
