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


#ifndef _XFIXESWIRE_H_
#define _XFIXESWIRE_H_

#define XFIXES_NAME	"XFIXES"
#define XFIXES_MAJOR	1
#define XFIXES_MINOR	0

#define X_XFixesQueryVersion		    0
#define X_XFixesChangeSaveSet		    1
#define X_XFixesSelectSelectionInput	    2
#define X_XFixesSelectCursorInput	    3
#define X_XFixesGetCursorImage		    4

#define XFixesNumberRequests		    (X_XFixesGetCursorImage+1)

/* Selection events share one event number */
#define XFixesSelectionNotify		    0

/* Within the selection, the 'subtype' field distinguishes */
#define XFixesSetSelectionOwnerNotify	    0
#define XFixesSelectionWindowDestroyNotify  1
#define XFixesSelectionClientCloseNotify    2

#define XFixesSetSelectionOwnerNotifyMask	(1L << 0)
#define XFixesSelectionWindowDestroyNotifyMask	(1L << 1)
#define XFixesSelectionClientCloseNotifyMask	(1L << 2)

/* There's only one cursor event so far */
#define XFixesCursorNotify		    1

#define XFixesDisplayCursorNotify	    0

#define XFixesDisplayCursorNotifyMask	    (1L << 0)

#define XFixesNumberEvents		    (2)

#define XFixesNumberErrors		    (0)

#define SaveSetNearest			    0
#define SaveSetRoot			    1

#define SaveSetMap			    0
#define SaveSetUnmap			    1

#endif	/* _XFIXESWIRE_H_ */
