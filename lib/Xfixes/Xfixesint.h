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

#ifndef _XFIXESINT_H_
#define _XFIXESINT_H_

#define NEED_EVENTS
#define NEED_REPLIES
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xext.h"			/* in ../include */
#include "extutil.h"			/* in ../include */
#include "Xfixes.h"
#include "xfixesproto.h"

extern XExtensionInfo XFixesExtensionInfo;
extern char XFixesExtensionName[];

#define XFixesCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, XFixesExtensionName, val)
#define XFixesSimpleCheckExtension(dpy,i) \
  XextSimpleCheckExtension (dpy, i, XFixesExtensionName)

XExtDisplayInfo *XFixesFindDisplay (Display *dpy);

/* cache version info */

typedef struct _XFixesInfo {
    int major_version;	    /* -1 means we don't know */
    int	minor_version;	
} XFixesInfo;

#endif /* _XFIXESINT_H_ */
