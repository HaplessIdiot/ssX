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

#ifndef _XFIXESPROTO_H_
#define _XFIXESPROTO_H_

#include <X11/Xmd.h>
#include <X11/extensions/xfixeswire.h>

#define Window CARD32
#define Drawable CARD32
#define Font CARD32
#define Pixmap CARD32
#define Cursor CARD32
#define Colormap CARD32
#define GContext CARD32
#define Atom CARD32
#define VisualID CARD32
#define Time CARD32
#define KeyCode CARD8
#define KeySym CARD32

/* 
 * requests and replies
 */
typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
} xXFixesQueryVersionReq;

#define sz_xXFixesQueryVersionReq   12

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
} xXFixesQueryVersionReply;

#define sz_xXFixesQueryVersionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    BYTE    mode;	    /* SetModeInsert/SetModeDelete*/
    BYTE    target;	    /* SaveSetNearest/SaveSetRoot*/
    BYTE    map;	    /* SaveSetMap/SaveSetUnmap */
    BYTE    pad1;
    Window  window;
} xXFixesChangeSaveSetReq;

#define sz_xXFixesChangeSaveSetReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  window  B32;
    Atom    selection B32;
    CARD32  eventMask B32;
} xXFixesSelectSelectionInputReq;

#define sz_xXFixesSelectSelectionInputReq   16

typedef struct {
    CARD8   type;
    CARD8   subtype;
    CARD16  sequenceNumber B16;
    Window  window B32;
    Window  owner B32;
    Atom    selection B32;
    Time    timestamp B32;
    Time    selectionTimestamp B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
} xXFixesSelectionNotifyEvent;

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  window B32;
    CARD32  eventMask B32;
} xXFixesSelectCursorInputReq;

#define sz_xXFixesSelectCursorInputReq	12

typedef struct {
    CARD8   type;
    CARD8   subtype;
    CARD16  sequenceNumber B16;
    Window  window B32;
    CARD32  cursorSerial B32;
    Time    timestamp;
    CARD32  pad1 B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
} xXFixesCursorNotifyEvent;

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
} xXFixesGetCursorImageReq;

#define sz_xXFixesGetCursorImageReq 4

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    INT16   x B16;
    INT16   y B16;
    CARD16  width B16;
    CARD16  height B16;
    CARD16  xhot B16;
    CARD16  yhot B16;
    CARD32  cursorSerial B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
} xXFixesGetCursorImageReply;

#define sz_xXFixesGetCursorImageReply	32

#undef Window
#undef Drawable
#undef Font
#undef Pixmap
#undef Cursor
#undef Colormap
#undef GContext
#undef Atom
#undef VisualID
#undef Time
#undef KeyCode
#undef KeySym

#endif /* _XFIXESPROTO_H_ */
