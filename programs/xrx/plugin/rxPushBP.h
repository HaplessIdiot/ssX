/*
* $TOG: rxPushBP.h /main/2 1997/08/29 17:54:38 kaleb $
*/


/***********************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* 
 * rxPushB.h - Private definitions for PushB widget
 * 
 */

#ifndef _rxPushBP_h
#define _rxPushBP_h

#include "rxPushB.h"
#include "rxLabelP.h"

/* New fields for the PushB widget class record */
typedef struct {
    int			not_used;  /* make compilers happy */
} RxPushBClassPart;

/* Full class record declaration */
typedef struct _RxPushBClassRec {
    CoreClassPart	core_class;
    RxLabelClassPart	label_class;
    RxPushBClassPart	pushb_class;
} RxPushBClassRec;

extern RxPushBClassRec rxPushBClassRec;

/* New fields for the PushB widget record */
typedef struct {
    /* resources */
    Dimension		highlight_thickness;
    XtCallbackList	callbacks;

    /* private state */
    GC			normal_GC;
    GC          	inverse_GC;
    Boolean     	set;
    Boolean		highlighted;
} RxPushBPart;

   /* Full widget declaration */
typedef struct _RxPushBRec {
    CorePart		core;
    RxLabelPart		label;
    RxPushBPart		pushb;
} RxPushBRec;

#endif /* _rxPushBP_h */

