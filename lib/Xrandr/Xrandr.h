/*
 * $XFree86$
 *
 * Copyright © 2000 Compaq Computer Corporation, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Compaq makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * COMPAQ DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL COMPAQ
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Compaq Computer Corporation
 */

#ifndef _XRANDR_H_
#define _XRANDR_H_

#include "randr.h"

Bool XRRQueryExtension (Display *dpy, int *event_basep, int *error_basep);

Status XRRQueryVersion (Display *dpy,
			    int     *major_versionp,
			    int     *minor_versionp);

Time XRRSetScreenConfig (Display *dpy,
			 Drawable draw,
			 int size_set_index,
			 int visual_set_index,
			 int rotation,
			 Time timestamp); /* returns new timestamp */

#endif /* _XRANDR_H_ */
