/*
* $TOG: TextSinkP.h /main/7 1998/02/06 12:51:22 kaleb $
*/


/***********************************************************

Copyright 1987, 1988, 1994, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


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
/* $XFree86: xc/lib/Xaw/TextSinkP.h,v 1.7 1999/06/20 08:41:11 dawes Exp $ */

#ifndef _XawTextSinkP_h
#define _XawTextSinkP_h

/*
 * TextSink Object Private Data
 */
#include <X11/Xaw/TextSink.h>
#include <X11/Xaw/TextP.h>	/* This sink works with the Text widget */
#include <X11/Xaw/TextSrcP.h>	/* This sink works with the Text Source */
#include <X11/Xmu/Xmu.h>

#ifndef OLDXAW
#define XAW_TPROP_FOREGROUND	(1<<0)
#define XAW_TPROP_BACKGROUND	(1<<1)
#define XAW_TPROP_FONT		(1<<2)
#define XAW_TPROP_FONTSET	(1<<3)
#define XAW_TPROP_OVERSTRIKE	(1<<4)
#define XAW_TPROP_UNDERLINE	(1<<5)
struct _XawTextProperty {
    XrmQuark identifier;
    Pixel foreground, background;
    XFontStruct *font;
    XFontSet fontset;
    unsigned long mask;
};

struct _XawTextPropertyList {
    XrmQuark identifier;
    Screen *screen;
    Colormap colormap;
    int depth;
    XawTextProperty **properties;
    Cardinal num_properties;
    XawTextPropertyList *next;
};

typedef struct _XawTextPaintStruct XawTextPaintStruct;
struct _XawTextPaintStruct {
    XawTextPaintStruct *next;
    int x, y, width;
    unsigned char *text;	/* formatted text */
    Cardinal length;		/* length of text */
    XawTextProperty *property;
    int max_ascent, max_descent;
    XmuArea *backtabs;
    Boolean highlight;
};

typedef struct {
    XmuArea *clip, *hightabs;			/* clip list */
    XawTextPaintStruct *paint, *bearings;	/* drawing information */
} XawTextPaintList;

typedef struct {
    XtPointer next_extension;
    XrmQuark record_type;
    long version;
    Cardinal record_size;
    Bool (*BeginPaint)(Widget);
    void (*PreparePaint)(Widget, int, int,
			 XawTextPosition, XawTextPosition, Bool);
    void (*DoPaint)(Widget);
    Bool (*EndPaint)(Widget);
} TextSinkExtRec, *TextSinkExt;
#endif

typedef void (*_XawSinkDisplayTextProc)
     (Widget, int, int, XawTextPosition, XawTextPosition, Bool);

typedef void (*_XawSinkInsertCursorProc)
     (Widget, int, int, XawTextInsertState);

typedef void (*_XawSinkClearToBackgroundProc)
     (Widget, int, int, unsigned int, unsigned int);

typedef void (*_XawSinkFindPositionProc)
     (Widget, XawTextPosition, int, int, Bool, XawTextPosition*, int*, int*);

typedef void (*_XawSinkFindDistanceProc)
     (Widget, XawTextPosition, int, XawTextPosition, int*,
      XawTextPosition*, int*);

typedef void (*_XawSinkResolveProc)
     (Widget, XawTextPosition, int, int, XawTextPosition*);

typedef int  (*_XawSinkMaxLinesProc)
     (Widget, unsigned int);

typedef int  (*_XawSinkMaxHeightProc)
     (Widget, int);

typedef void (*_XawSinkSetTabsProc)
     (Widget, int, short*);

typedef void (*_XawSinkGetCursorBoundsProc)
     (Widget, XRectangle*);

typedef struct _TextSinkClassPart {
    _XawSinkDisplayTextProc DisplayText;
    _XawSinkInsertCursorProc InsertCursor;
    _XawSinkClearToBackgroundProc ClearToBackground;
    _XawSinkFindPositionProc FindPosition;
    _XawSinkFindDistanceProc FindDistance;
    _XawSinkResolveProc Resolve;
    _XawSinkMaxLinesProc MaxLines;
    _XawSinkMaxHeightProc MaxHeight;
    _XawSinkSetTabsProc	SetTabs;
    _XawSinkGetCursorBoundsProc GetCursorBounds;
#ifndef OLDXAW
    TextSinkExt extension;
#endif
} TextSinkClassPart;

/* Full class record */
typedef struct _TextSinkClassRec {
    ObjectClassPart     object_class;
    TextSinkClassPart	text_sink_class;
} TextSinkClassRec;

extern TextSinkClassRec textSinkClassRec;

/* New fields for the TextSink object */
typedef struct {
    /* resources */
    Pixel foreground;		/* Foreground color */
    Pixel background;		/* Background color */

    /* private */
    Position *tabs;		/* The tab stops as pixel values */
    short *char_tabs;		/* The tabs stops as character values */
    int tab_count;		/* number of items in tabs */

#ifndef OLDXAW
    /* more resources */
    Pixel cursor_color;
    XawTextPropertyList *properties;
    XawTextPaintList *paint;
    XtPointer pad[2];	/* for future use and keep binary compatability */
#endif
} TextSinkPart;

/* Full instance record */
typedef struct _TextSinkRec {
    ObjectPart	 object;
    TextSinkPart text_sink;
} TextSinkRec;

/* Semi private routines */
#ifndef OLDXAW
XawTextPropertyList *XawTextSinkConvertPropertyList
(
 String			 name,
 String			 spec,
 Screen			*screen,
 Colormap		 Colormap,
 int			 depth
 );

XawTextProperty *XawTextSinkGetProperty
(
 Widget			 w,
 XrmQuark		 property
 );

Bool XawTextSinkBeginPaint
(
 Widget			w
 );

void XawTextSinkPreparePaint
(
 Widget			w,
 int			y,
 int			line,
 XawTextPosition	from,
 XawTextPosition	to,
 Bool			highlight
);

void XawTextSinkDoPaint
(
 Widget			w
 );

Bool XawTextSinkEndPaint
(
 Widget			w
 );
#endif

#define XtInheritDisplayText	   ((_XawSinkDisplayTextProc)_XtInherit)
#define XtInheritInsertCursor	   ((_XawSinkInsertCursorProc)_XtInherit)
#define XtInheritClearToBackground ((_XawSinkClearToBackgroundProc)_XtInherit)
#define XtInheritFindPosition	   ((_XawSinkFindPositionProc)_XtInherit)
#define XtInheritFindDistance	   ((_XawSinkFindDistanceProc)_XtInherit)
#define XtInheritResolve	   ((_XawSinkResolveProc)_XtInherit)
#define XtInheritMaxLines	   ((_XawSinkMaxLinesProc)_XtInherit)
#define XtInheritMaxHeight	   ((_XawSinkMaxHeightProc)_XtInherit)
#define XtInheritSetTabs	   ((_XawSinkSetTabsProc)_XtInherit)
#define XtInheritGetCursorBounds   ((_XawSinkGetCursorBoundsProc)_XtInherit)

#endif /* _XawTextSinkP_h */
