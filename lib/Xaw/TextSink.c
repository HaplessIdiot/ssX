/* $TOG: TextSink.c /main/21 1998/02/11 14:55:39 kaleb $ */
/*

Copyright 1989, 1994, 1998  The Open Group

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

*/
/* $XFree86: xc/lib/Xaw/TextSink.c,v 1.12 1999/06/13 13:47:23 dawes Exp $ */

/*
 * Author:  Chris Peterson, MIT X Consortium.
 *
 * Much code taken from X11R3 AsciiSink.
 */

/*
 * TextSink.c - TextSink object. (For use with the text widget).
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/TextP.h>
#include <X11/Xaw/TextSinkP.h>
#include <X11/Xaw/XawInit.h>
#include "Private.h"

/*
 * Prototypes
 */
static void XawTextSinkClassPartInitialize(WidgetClass);
static void XawTextSinkInitialize(Widget, Widget, ArgList, Cardinal*);
static void XawTextSinkDestroy(Widget);
static Boolean XawTextSinkSetValues(Widget, Widget, Widget,
				    ArgList, Cardinal*);

static int  MaxLines(Widget, unsigned int);
static int  MaxHeight(Widget, int);
static void DisplayText(Widget, int, int, XawTextPosition, XawTextPosition,
			Bool);
static void InsertCursor(Widget, int, int, XawTextInsertState);
static void ClearToBackground(Widget, int, int, unsigned int, unsigned int);
static void FindPosition(Widget, XawTextPosition, int, int, Bool,
			 XawTextPosition*, int*, int*);
static void FindDistance(Widget, XawTextPosition, int, XawTextPosition, int*,
			 XawTextPosition*, int*);
static void Resolve(Widget, XawTextPosition, int, int, XawTextPosition*);
static void SetTabs(Widget, int, short*);
static void GetCursorBounds(Widget, XRectangle*);

#ifndef OLDXAW
static Boolean CvtStringToPropertyList(Display*, XrmValue*, Cardinal*,
				       XrmValue*, XrmValue*, XtPointer*);
static Boolean CvtPropertyListToString(Display*, XrmValue*, Cardinal*,
				       XrmValue*, XrmValue*, XtPointer*);
static Bool BeginPaint(Widget);
static Bool EndPaint(Widget);
#endif

/*
 * External
 */
void _XawTextSinkClearToBackground(Widget, int, int, unsigned, unsigned);
void _XawTextSinkDisplayText(Widget, int, int, XawTextPosition, XawTextPosition,
			     Bool);

/*
 * Initialization
 */
#define offset(field) XtOffsetOf(TextSinkRec, text_sink.field)
static XtResource resources[] = {
  {
    XtNforeground,
    XtCForeground,
    XtRPixel,
    sizeof(Pixel),
    offset(foreground),
    XtRString,
    XtDefaultForeground
  },
  {
    XtNbackground,
    XtCBackground,
    XtRPixel,
    sizeof(Pixel),
    offset(background),
    XtRString,
    XtDefaultBackground
  },
#ifndef OLDXAW
  {
    XtNcursorColor,
    XtCColor,
    XtRPixel,
    sizeof(Pixel),
    offset(cursor_color),
    XtRString,
    XtDefaultForeground
  },
  {
    XawNtextProperties,
    XawCTextProperties,
    XawRTextProperties,
    sizeof(XawTextPropertyList*),
    offset(properties),
    XtRImmediate,
    NULL
  },
#endif
};
#undef offset

#ifndef OLDXAW
static TextSinkExtRec extension_rec = {
    NULL,				/* next_extension */
    NULLQUARK,				/* record_type */
    1,					/* version */
    sizeof(TextSinkExtRec),		/* record_size */
    BeginPaint,
    NULL,
    NULL,
    EndPaint
};
#endif

#define Superclass	(&objectClassRec)
TextSinkClassRec textSinkClassRec = {
  /* object */
  {
    (WidgetClass)Superclass,		/* superclass */
    "TextSink",				/* class_name */
    sizeof(TextSinkRec),		/* widget_size */
    XawInitializeWidgetSet,		/* class_initialize */
    XawTextSinkClassPartInitialize,	/* class_part_initialize */
    False,				/* class_inited */
    XawTextSinkInitialize,		/* initialize */
    NULL,				/* initialize_hook */
    NULL,				/* obj1 */
    NULL,				/* obj2 */
    0,					/* obj3 */
    resources,				/* resources */
    XtNumber(resources),		/* num_resources */
    NULLQUARK,				/* xrm_class */
    False,				/* obj4 */
    False,				/* obj5 */
    False,				/* obj6 */
    False,				/* obj7 */
    XawTextSinkDestroy,			/* destroy */
    NULL,				/* obj8 */
    NULL,				/* obj9 */
    XawTextSinkSetValues,		/* set_values */
    NULL,				/* set_values_hook */
    NULL,				/* obj10 */
    NULL,				/* get_values_hook */
    NULL,				/* obj11 */
    XtVersion,				/* version */
    NULL,				/* callback_private */
    NULL,				/* obj12 */
    NULL,				/* obj13 */
    NULL,				/* obj14 */
    NULL,				/* extension */
  },
  /* text_sink */
  {
    DisplayText,			/* DisplayText */
    InsertCursor,			/* InsertCursor */
    ClearToBackground,			/* ClearToBackground */
    FindPosition,			/* FindPosition */
    FindDistance,			/* FindDistance */
    Resolve,				/* Resolve */
    MaxLines,				/* MaxLines */
    MaxHeight,				/* MaxHeight */
    SetTabs,				/* SetTabs */
    GetCursorBounds,			/* GetCursorBounds */
  },
};

WidgetClass textSinkObjectClass = (WidgetClass)&textSinkClassRec;

/*
 * Implementation
 */
static void
XawTextSinkClassPartInitialize(WidgetClass wc)
{
    TextSinkObjectClass t_src, superC;
#ifndef OLDXAW
    static XtConvertArgRec CvtArgs[] = {
      {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.self),
       sizeof(Widget)},
    };
#endif

    t_src = (TextSinkObjectClass) wc;
    superC = (TextSinkObjectClass) t_src->object_class.superclass;

#ifndef OLDXAW
    extension_rec.record_type = XrmPermStringToQuark("TextSink");
    extension_rec.next_extension = (XtPointer)t_src->text_sink_class.extension;
    t_src->text_sink_class.extension = &extension_rec;
#endif

    /* 
     * We don't need to check for null super since we'll get to TextSink
     * eventually.
     */
    if (t_src->text_sink_class.DisplayText == XtInheritDisplayText)
	t_src->text_sink_class.DisplayText =
	    superC->text_sink_class.DisplayText;

    if (t_src->text_sink_class.InsertCursor == XtInheritInsertCursor)
	t_src->text_sink_class.InsertCursor =
	    superC->text_sink_class.InsertCursor;

    if (t_src->text_sink_class.ClearToBackground== XtInheritClearToBackground)
	t_src->text_sink_class.ClearToBackground =
	    superC->text_sink_class.ClearToBackground;

    if (t_src->text_sink_class.FindPosition == XtInheritFindPosition)
	t_src->text_sink_class.FindPosition =
	    superC->text_sink_class.FindPosition;

    if (t_src->text_sink_class.FindDistance == XtInheritFindDistance)
	t_src->text_sink_class.FindDistance =
	    superC->text_sink_class.FindDistance;

    if (t_src->text_sink_class.Resolve == XtInheritResolve)
	t_src->text_sink_class.Resolve =
	    superC->text_sink_class.Resolve;

    if (t_src->text_sink_class.MaxLines == XtInheritMaxLines)
	t_src->text_sink_class.MaxLines =
	    superC->text_sink_class.MaxLines;

    if (t_src->text_sink_class.MaxHeight == XtInheritMaxHeight)
	t_src->text_sink_class.MaxHeight =
	    superC->text_sink_class.MaxHeight;

    if (t_src->text_sink_class.SetTabs == XtInheritSetTabs)
	t_src->text_sink_class.SetTabs =
	    superC->text_sink_class.SetTabs;

    if (t_src->text_sink_class.GetCursorBounds == XtInheritGetCursorBounds)
	t_src->text_sink_class.GetCursorBounds =
	    superC->text_sink_class.GetCursorBounds;

#ifndef OLDXAW
    XtSetTypeConverter(XtRString, XawRTextProperties, CvtStringToPropertyList,
		       &CvtArgs[0], XtNumber(CvtArgs), XtCacheNone, NULL);
    XtSetTypeConverter(XawRTextProperties, XtRString, CvtPropertyListToString,
		       NULL, 0, XtCacheNone, NULL);
#endif
}

/*
 * Function:
 *	XawTextSinkInitialize
 *
 * Parameters:
 *	request - requested and new values for the object instance
 *	cnew	- ""
 *
 * Description:
 *	Initializes the TextSink Object.
 */
/*ARGSUSED*/
static void
XawTextSinkInitialize(Widget request, Widget cnew,
		      ArgList args, Cardinal *num_args)
{
    TextSinkObject sink = (TextSinkObject)cnew;

    sink->text_sink.tab_count = 0; /* Initialize the tab stops. */
    sink->text_sink.tabs = NULL;
    sink->text_sink.char_tabs = NULL;
#ifndef OLDXAW
    sink->text_sink.paint = NULL;
#endif
}

/*
 * Function:
 *	XawTextSinkDestroy
 *
 * Parameters:
 *	w - TextSink Object
 *
 * Description:
 *	This function cleans up when the object is destroyed.
 */
static void
XawTextSinkDestroy(Widget w)
{
    TextSinkObject sink = (TextSinkObject) w;

    XtFree((char *)sink->text_sink.tabs);
    XtFree((char *)sink->text_sink.char_tabs);
}

/*
 * Function:
 *	XawTextSinkSetValues
 *
 * Parameters:
 *	current - current state of the object
 *	request - what was requested
 *	cnew	- what the object will become
 *
 * Description:
 *	Sets the values for the TextSink.
 *
 * Returns:
 *	True if redisplay is needed
 */
/*ARGSUSED*/
static Boolean
XawTextSinkSetValues(Widget current, Widget request, Widget cnew,
		     ArgList args, Cardinal *num_args)
{
    TextSinkObject w = (TextSinkObject)cnew;
    TextSinkObject old_w = (TextSinkObject)current;

    if (w->text_sink.foreground != old_w->text_sink.foreground)
	((TextWidget)XtParent(cnew))->text.redisplay_needed = True;

    return (False);
}

/*
 * Function:
 *	DisplayText
 *
 * Parameters:
 *	w	  - TextSink Object
 *	x	  - location to start drawing text
 *	y	  - ""
 *	pos1	  - location of starting and ending points in the text buffer
 *	pos2	  - ""
 *		 highlight - hightlight this text?
 *
 * Description:
 *	Stub function that in subclasses will display text.
 */
/*ARGSUSED*/
static void
DisplayText(Widget w, int x, int y,
	    XawTextPosition pos1, XawTextPosition pos2, Bool highlight)
{
    return;
}

/*
 * Function:
 *	InsertCursor
 *
 * Parameters:
 *	w     - TextSink Object
 *	x     - location for the cursor
 *	y     - ""
 *	state - whether to turn the cursor on, or off
 *
 * Description:
 *	Places the InsertCursor.
 */
/*ARGSUSED*/
static void
InsertCursor(Widget w, int x, int y, XawTextInsertState state)
{
    return;
}

/*
 * Function:
 *	ClearToBackground
 *
 * Parameters:
 *	w      - TextSink Object
 *	x      - location of area to clear
 *	y      - ""
 *	width  - size of area to clear
 *	height - ""
 *
 * Description:
 *	Clears a region of the sink to the background color.
 */
/*ARGSUSED*/
static void
ClearToBackground(Widget w, int x, int y,
		  unsigned int width, unsigned int height)
{
    /* 
     * Don't clear in height or width are zero
     * XClearArea() has special semantic for these values
     */
    TextWidget xaw = (TextWidget)XtParent(w);
    Position x1, y1, x2, y2;

    x1 = XawMax(x, xaw->text.r_margin.left);
    y1 = XawMax(y, xaw->text.r_margin.top);
    x2 = XawMin(x + (int)width, (int)XtWidth(xaw) - xaw->text.r_margin.right);
    y2 = XawMin(y + (int)height, (int)XtHeight(xaw) - xaw->text.r_margin.bottom);

    x = x1;
    y = y1;
    width = XawMax(0, x2 - x1);
    height = XawMax(0, y2 - y1);

    if (height != 0 && width != 0)
	XClearArea(XtDisplayOfObject(w), XtWindowOfObject(w),
		   x, y, width, height, False);
}

/*
 * Function:
 *	FindPosition
 *
 * Parameters:
 *	w		- TextSink Object
 *	fromPos		- reference position
 *	fromX		- reference location
 *	width		- width of section to paint text
 *	stopAtWordBreak - returned position is a word break?
 *	resPos		- position found (return)
 *	resWidth	- Width actually used (return)
 *	resHeight	- Height actually used (return)
 *
 * Description:
 *	Finds a position in the text.
 */
/*ARGSUSED*/
static void
FindPosition(Widget w, XawTextPosition fromPos, int fromx, int width,
	     Bool stopAtWordBreak, XawTextPosition *resPos,
	     int *resWidth, int *resHeight)
{
    *resPos = fromPos;
    *resHeight = *resWidth = 0;
}

/*
 * Function:
 *	FindDistance
 *
 * Parameters:
 *	w	  - TextSink Object
 *	fromPos	  - starting Position
 *	fromX	  - x location of starting Position
 *	toPos	  - end Position
 *	resWidth  - Distance between fromPos and toPos
 *	resPos	  - Acutal toPos used
 *	resHeight - Height required by this text
 *
 * Description:
 *	Find the Pixel Distance between two text Positions.
 */
/*ARGSUSED*/
static void
FindDistance(Widget w, XawTextPosition fromPos, int fromx,
	     XawTextPosition toPos, int *resWidth,
	     XawTextPosition *resPos, int *resHeight)
{
    *resWidth = *resHeight = 0;
    *resPos = fromPos;
}

/*
 * Function:
 *	Resolve
 *
 * Parameters:
 *	w      - TextSink Object
 *	pos    - reference Position
 *	fromx  - reference Location
 *	width  - width to move
 *	resPos - resulting position
 *
 * Description:
 *	Resloves a location to a position.
 */
/*ARGSUSED*/
static void
Resolve(Widget w, XawTextPosition pos, int fromx, int width,
	XawTextPosition *resPos)
{
    *resPos = pos;
}

/*
 * Function:
 *	MaxLines
 *
 * Parameters:
 *	w      - TextSink Object
 *	height - height to fit lines into
 *
 * Description:
 *	Finds the Maximum number of lines that will fit in a given height.
 *
 * Returns:
 *	Number of lines that will fit
 */
/*ARGSUSED*/
static int
MaxLines(Widget w, unsigned int height)
{
    /*
     * The fontset has gone down to descent Sink Widget, so
     * the functions such MaxLines, SetTabs... are bound to the descent.
     *
     * by Li Yuhong, Jan. 15, 1991
     */
    return (0);
}

/*
 * Function:
 *	MaxHeight
 *
 * Parameters:
 *	w     - TextSink Object
 *	lines - number of lines
 *
 * Description:
 *	Finds the Minium height that will contain a given number lines.
 *
 * Returns:
 *	the height
 */
/*ARGSUSED*/
static int
MaxHeight(Widget w, int lines)
{
    return (0);
}

/*
 * Function:
 *	SetTabs
 *
 * Parameters:
 *	w	  - TextSink Object
 *	tab_count - the number of tabs in the list
 *	tabs	  - text positions of the tabs
 * Description:
 *	Sets the Tab stops.
 */
/*ARGSUSED*/
static void
SetTabs(Widget w, int tab_count, short *tabs)
{
    return;
}

/*
 * Function:
 *	GetCursorBounds
 *
 * Parameters:
 *	w - TextSinkObject.
 *	rect - X rectangle containing the cursor bounds
 *
 * Description:
 *	Finds the bounding box for the insert cursor (caret)
 */
/*ARGSUSED*/
static void
GetCursorBounds(Widget w, XRectangle *rect)
{
    rect->x = rect->y = rect->width = rect->height = 0;
}

/*
 * Public Functions
 */
/*
 * Function:
 *	XawTextSinkDisplayText
 *
 * Parameters:
 *	w	  - TextSink Object
 *	x	  - location to start drawing text
 *	y	  - ""
 *	pos1	  - location of starting and ending points in the text buffer
 *	pos2	  - ""
 *	highlight - hightlight this text?
 */
/*ARGSUSED*/
void
XawTextSinkDisplayText(Widget w,
#if NeedWidePrototypes
		       int x, int y,
#else
		       Position x, Position y,
#endif
		       XawTextPosition pos1, XawTextPosition pos2,
#if NeedWidePrototypes
		       int highlight
#else
		       Boolean highlight
#endif
)
{
    _XawTextSinkDisplayText(w, x, y, pos1, pos2, highlight);
}

void
_XawTextSinkDisplayText(Widget w, int x, int y,
			XawTextPosition pos1, XawTextPosition pos2,
			Bool highlight)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.DisplayText)(w, x, y, pos1, pos2, highlight);
}

/*
 * Function:
 *	XawTextSinkInsertCursor
 *
 * Parameters:
 *	w     - TextSink Object
 *	x     - location for the cursor
 *	y     - ""
 *	state - whether to turn the cursor on, or off
 *
 * Description:
 *	Places the InsertCursor.
 */
/*ARGSUSED*/
void
#if NeedWidePrototypes
XawTextSinkInsertCursor(Widget w, int x, int y, int state)
#else
XawTextSinkInsertCursor(Widget w, Position x, Position y, XawTextInsertState state)
#endif
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.InsertCursor)(w, x, y, state);
}

/*
 * Function:
 *	XawTextSinkClearToBackground
 *
 * Parameters:
 *	w      - TextSink Object
 *	x      - location of area to clear
 *	y      - ""
 *	width  - size of area to clear
 *	height - ""
 *
 * Description:
 *	Clears a region of the sink to the background color.
 */
/*ARGSUSED*/
void
XawTextSinkClearToBackground(Widget w,
#if NeedWidePrototypes
			     int x, int y,
			     unsigned int width, unsigned int height
#else
			     Position x, Position y,
			     Dimension width, Dimension height
#endif
)
{
    _XawTextSinkClearToBackground(w, x, y, width, height);
}

void
_XawTextSinkClearToBackground(Widget w,
			      int x, int y,
			      unsigned int width, unsigned int height)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.ClearToBackground)(w, x, y, width, height);
}

/*
 * Function:
 *	XawTextSinkFindPosition
 *
 *  Parameters:
 *	w		- TextSink Object
 *	fromPos		- reference position
 *	fromX		- reference location
 *	width		- width of section to paint text
 *	stopAtWordBreak - returned position is a word break?
 *	resPos		- position found (return)
 *	resWidth	- Width actually used (return)
 *	resHeight	- Height actually used (return)
 *
 * Description:
 *	Finds a position in the text.
 */
/*ARGSUSED*/
void
XawTextSinkFindPosition(Widget w, XawTextPosition fromPos, int fromx, int width,
#if NeedWidePrototypes
			int stopAtWordBreak,
#else
			Boolean stopAtWordBreak,
#endif
			XawTextPosition *resPos, int *resWidth, int *resHeight)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.FindPosition)(w, fromPos, fromx, width,
					    stopAtWordBreak, 
					    resPos, resWidth, resHeight);
}

/*
 * Function:
 *	XawTextSinkFindDistance
 *
 *  Parameters:
 *	w	  - TextSink Object
 *	fromPos	  - starting Position
 *	fromX	  - x location of starting Position
 *	toPos	  - end Position
 *	resWidth  - Distance between fromPos and toPos
 *	resPos	  - Acutal toPos used
 *	resHeight - Height required by this text
 *
 * Description:
 *	Find the Pixel Distance between two text Positions.
 */
/*ARGSUSED*/
void
XawTextSinkFindDistance(Widget w, XawTextPosition fromPos, int fromx,
			XawTextPosition toPos, int *resWidth, 
			XawTextPosition *resPos, int *resHeight)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.FindDistance)(w, fromPos, fromx, toPos,
					    resWidth, resPos, resHeight);
}

/*
 * Function:
 *	XawTextSinkResolve
 *
 *  Parameters:
 *	w      - TextSink Object
 *	pos    - reference Position
 *	fromx  - reference Location
 *	width  - width to move
 *	resPos - resulting position
 *
 * Description:
 *	Resloves a location to a position.
 */
/*ARGSUSED*/
void
XawTextSinkResolve(Widget w, XawTextPosition pos, int fromx, int width,
		   XawTextPosition *resPos)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass) w->core.widget_class;

    (*cclass->text_sink_class.Resolve)(w, pos, fromx, width, resPos);
}

/*
 * Function:
 *	XawTextSinkMaxLines
 *
 *  Parameters:
 *	w      - TextSink Object
 *	height - height to fit lines into
 *
 * Description:
 *	Finds the Maximum number of lines that will fit in a given height.
 *
 * Returns:
 *	Number of lines that will fit
 */
/*ARGSUSED*/
int
#if NeedWidePrototypes
XawTextSinkMaxLines(Widget w, unsigned int height)
#else
XawTextSinkMaxLines(Widget w, Dimension height)
#endif
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    return((*cclass->text_sink_class.MaxLines)(w, height));
}

/*
 * Function:
 *	XawTextSinkMaxHeight
 *
 *  Parameters:
 *	w     - TextSink Object
 *	lines - number of lines
 *
 * Description:
 *	Finds the Minium height that will contain a given number lines.
 *
 * Returns:
 *	the height
 */
/*ARGSUSED*/
int
XawTextSinkMaxHeight(Widget w, int lines)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    return((*cclass->text_sink_class.MaxHeight)(w, lines));
}

/*
 * Function:
 *	XawTextSinkSetTabs
 *
 *  Parameters:
 *	w	  - TextSink Object
 *	tab_count - the number of tabs in the list
 *	tabs	  - text positions of the tabs
 * Description:
 *	Sets the Tab stops.
 */
void
XawTextSinkSetTabs(Widget w, int tab_count, int *tabs)
{
    if (tab_count > 0) {
	TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;
	short *char_tabs = (short*)XtMalloc((unsigned)tab_count * sizeof(short));
	short *tab, len = 0;
	int i;

	for (i = tab_count, tab = char_tabs; i; i--) {
	    if ((short)*tabs > len)
		*tab++ = (len = (short)*tabs++);
	    else {
		tabs++;
		--tab_count;
	    }
	}

	if (tab_count > 0)
	    (*cclass->text_sink_class.SetTabs)(w, tab_count, char_tabs);
	XtFree((char *)char_tabs);
    }
}

/*
 * Function:
 *	XawTextSinkGetCursorBounds
 *
 * Parameters:
 *	w    - TextSinkObject
 *	rect - X rectance containing the cursor bounds
 *
 * Description:
 *	Finds the bounding box for the insert cursor (caret).
 */
/*ARGSUSED*/
void
XawTextSinkGetCursorBounds(Widget w, XRectangle *rect)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.GetCursorBounds)(w, rect);
}

#ifndef OLDXAW
Bool
XawTextSinkBeginPaint(Widget w)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    if (cclass->text_sink_class.extension->BeginPaint == NULL ||
	cclass->text_sink_class.extension->PreparePaint == NULL ||
	cclass->text_sink_class.extension->DoPaint == NULL ||
	cclass->text_sink_class.extension->EndPaint == NULL)
	return (False);

    return ((*cclass->text_sink_class.extension->BeginPaint)(w));
}

static Bool
BeginPaint(Widget w)
{
    TextSinkObject sink = (TextSinkObject)w;

    if (sink->text_sink.paint != NULL)
	return (False);

    sink->text_sink.paint = XtNew(XawTextPaintList);
    sink->text_sink.paint->clip = XmuCreateArea();
    sink->text_sink.paint->hightabs = NULL;
    sink->text_sink.paint->paint = NULL;
    sink->text_sink.paint->bearings = NULL;

    return (True);
}

void
XawTextSinkPreparePaint(Widget w, int y, int line, XawTextPosition from,
			XawTextPosition to, Bool highlight)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.extension->PreparePaint)
	(w, y, line, from, to, highlight);
}

/*ARGSUSED*/
static void
PreparePaint(Widget w, int y, int line, XawTextPosition from, XawTextPosition to,
	     Bool highlight)
{
}

void
XawTextSinkDoPaint(Widget w)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    (*cclass->text_sink_class.extension->DoPaint)(w);
}

/*ARGSUSED*/
static void
DoPaint(Widget w)
{
}

Bool
XawTextSinkEndPaint(Widget w)
{
    TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

    return ((*cclass->text_sink_class.extension->EndPaint)(w));
}

static Bool
EndPaint(Widget w)
{
    TextSinkObject sink = (TextSinkObject)w;
    XawTextPaintStruct *paint, *next;

    if (sink->text_sink.paint == NULL)
	return (False);

    XmuDestroyArea(sink->text_sink.paint->clip);
    if (sink->text_sink.paint->hightabs)
	XmuDestroyArea(sink->text_sink.paint->hightabs);
    paint = sink->text_sink.paint->paint;
    while (paint) {
	next = paint->next;
	if (paint->text)
	    XtFree((XtPointer)paint->text);
	if (paint->backtabs)
	    XmuDestroyArea(paint->backtabs);
	XtFree((XtPointer)paint);
	paint = next;
    }

    paint = sink->text_sink.paint->bearings;
    while (paint) {
	next = paint->next;
	if (paint->text)
	    XtFree((XtPointer)paint->text);
	XtFree((XtPointer)paint);
	paint = next;
    }

    XtFree((XtPointer)sink->text_sink.paint);
    sink->text_sink.paint = NULL;
}

static XawTextPropertyList **prop_lists;
static Cardinal num_prop_lists;

static int
bcmp_qident(_Xconst void *left, _Xconst void *right)
{
    return ((int)left - (*(XawTextProperty**)right)->identifier);
}

static int
qcmp_qident(_Xconst void *left, _Xconst void *right)
{
    return ((*(XawTextProperty**)left)->identifier -
	    (*(XawTextProperty**)right)->identifier);
}

static void
DestroyTextPropertyList(XawTextPropertyList *list)
{
    int i;

    for (i = 0; i < list->num_properties; i++)
	XtFree((char*)list->properties[i]);
    if (list->properties)
	XtFree((char*)list->properties);
    XtFree((char*)list);
}

XawTextProperty *
XawTextSinkGetProperty(Widget w, XrmQuark property)
{
    TextSinkObject sink = (TextSinkObject)w;
    XawTextPropertyList *list = sink->text_sink.properties;

    if (list) {
	XawTextProperty **ptr = (XawTextProperty**)
	    bsearch((void*)property, list->properties, list->num_properties,
		    sizeof(XawTextProperty*), bcmp_qident);

	if (ptr)
	    return (*ptr);
    }

    return (NULL);
}

XawTextPropertyList *
XawTextSinkConvertPropertyList(String name, String spec, Screen *screen,
			       Colormap colormap, int depth)
{
    XrmQuark qname = XrmStringToQuark(name);
    XawTextPropertyList **ptr = (XawTextPropertyList**)
	bsearch((void*)qname, prop_lists, num_prop_lists,
		sizeof(XawTextPropertyList*), bcmp_qident);
    XawTextPropertyList *propl, *prev = NULL;
    String str, tok, tmp;

    if (ptr) {
	propl = *ptr;
	while (propl) {
	    prev = propl;
	    if (propl->screen == screen &&
		propl->colormap == colormap &&
		propl->depth == depth)
		return (propl);
	    propl = propl->next;
	}
    }

    propl = XtNew(XawTextPropertyList);
    propl->identifier = qname;
    propl->screen = screen;
    propl->colormap = colormap;
    propl->depth = depth;
    propl->next = NULL;

    if (prev)
	prev->next = propl;

    propl->properties = NULL;
    propl->num_properties = 0;

    str = XtNewString(spec);
    for (tok = str; tok; tok = tmp) {
	XawTextProperty *prop;
	XawParams *params;
	XrmQuark ident;
	XawArgVal *argval;
	XColor color, exact;

	tmp = strchr(tok, ',');
	if (tmp) {
	    *tmp = '\0';
	    if (*++tmp == '\0')
		tmp = NULL;
	}
	params = XawParseParamsString(tok);
	ident = XrmStringToQuark(params->name);
	if (ident == NULLQUARK) {
	    DestroyTextPropertyList(propl);
	    if (prev)
		prev->next = NULL;
	    XawFreeParamsStruct(params);
	    return (NULL);
	}
	else if (bsearch((void*)qname, propl->properties, propl->num_properties,
			 sizeof(XawTextProperty*), bcmp_qident)) {
	    XawFreeParamsStruct(params);
	    continue;
	}

	prop = XtNew(XawTextProperty);
	prop->identifier = ident;
	prop->mask = 0;
	if ((argval = XawFindArgVal(params, "foreground")) != NULL &&
	    argval->value) {
	    if (!XAllocNamedColor(DisplayOfScreen(screen), colormap,
				  argval->value, &color, &exact)) {
		DestroyTextPropertyList(propl);
		if (prev)
		    prev->next = NULL;
		XawFreeParamsStruct(params);
		return (NULL);
	    }
	    prop->foreground = color.pixel;
	    prop->mask |= XAW_TPROP_FOREGROUND;
	}
	if ((argval = XawFindArgVal(params, "background")) != NULL &&
	    argval->value) {
	    if (!XAllocNamedColor(DisplayOfScreen(screen), colormap,
				  argval->value, &color, &exact)) {
		DestroyTextPropertyList(propl);
		if (prev)
		    prev->next = NULL;
		XawFreeParamsStruct(params);
		return (NULL);
	    }
	    prop->background = color.pixel;
	    prop->mask |= XAW_TPROP_BACKGROUND;
	}
	if ((argval = XawFindArgVal(params, "font")) != NULL &&
	    argval->value) {
	    if ((prop->font = XLoadQueryFont(DisplayOfScreen(screen),
					     argval->value)) == NULL) {
		DestroyTextPropertyList(propl);
		if (prev)
		    prev->next = NULL;
		XawFreeParamsStruct(params);
		return (NULL);
	    }
	    prop->mask |= XAW_TPROP_FONT;
	}

	/* fontset */

	if (XawFindArgVal(params, "overstrike"))
	    prop->mask |= XAW_TPROP_OVERSTRIKE;
	if (XawFindArgVal(params, "underline"))
	    prop->mask |= XAW_TPROP_UNDERLINE;

	propl->properties = (XawTextProperty**)
	XtRealloc((XtPointer)propl->properties, sizeof(XawTextProperty*) *
		  (propl->num_properties + 1));
	propl->properties[propl->num_properties++] = prop;
	qsort((void*)propl->properties, propl->num_properties,
	      sizeof(XawTextProperty*), qcmp_qident);

	XawFreeParamsStruct(params);
    }

    prop_lists = (XawTextPropertyList**)
    XtRealloc((XtPointer)prop_lists, sizeof(XawTextPropertyList*) *
	      (num_prop_lists + 1));
    prop_lists[num_prop_lists++] = propl;
    qsort((void*)prop_lists, num_prop_lists, sizeof(XawTextPropertyList*),
	  qcmp_qident);

    XtFree(str);

    return (propl);
}

/*ARGSUSED*/
static Boolean
CvtStringToPropertyList(Display *dpy, XrmValue *args, Cardinal *num_args,
			XrmValue *fromVal, XrmValue *toVal,
			XtPointer *converter_data)
{
    XawTextPropertyList *propl = NULL;
    String name;
    Widget w;

    if (*num_args != 1) {
      XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		      "wrongParameters", "cvtStringToTextProperties",
		      "ToolkitError",
		      "String to textProperties conversion needs widget argument",
		      NULL, NULL);
	return (False);
    }

    w = *(Widget*)args[0].addr;
    while (w && !XtIsWidget(w))
	w = XtParent(w);

    name = (String)(fromVal[0].addr);

    if (w) {
	XawTextPropertyList **ptr = (XawTextPropertyList**)
	    bsearch((void*)XrmStringToQuark(name), prop_lists, num_prop_lists,
		    sizeof(XawTextPropertyList*), bcmp_qident);

	if (ptr) {
	    Screen *screen = w->core.screen;
	    Colormap colormap = w->core.colormap;
	    int depth = w->core.depth;

	    propl = *ptr;
	    while (propl) {
		if (propl->screen == screen &&
		    propl->colormap == colormap &&
		    propl->depth == depth)
		    break;
		propl = propl->next;
	    }
	}
    }

    if (!propl) {
	XtDisplayStringConversionWarning(dpy, (String)fromVal->addr,
					 XawRTextProperties);
	toVal->addr = NULL;
	toVal->size = sizeof(XawTextPropertyList*);
	return (False);
    }

    if (toVal->addr != NULL) {
	if (toVal->size < sizeof(XawTextPropertyList*))	{
	    toVal->size = sizeof(XawTextPropertyList*);
	    return (False);
	}
	*(XawTextPropertyList**)(toVal->addr) = propl;
    }
    else {
	static XawTextPropertyList *static_val;

	static_val = propl;
	toVal->addr = (XPointer)&static_val;
    }
    toVal->size = sizeof(XawTextProperty*);

    return (True);
}

/*ARGSUSED*/
static Boolean
CvtPropertyListToString(Display *dpy, XrmValue *args, Cardinal *num_args,
			XrmValue *fromVal, XrmValue *toVal,
			XtPointer *converter_data)
{
    static char *buffer;
    Cardinal size;
    XawTextPropertyList *propl;

    propl = *(XawTextPropertyList**)fromVal[0].addr;

    buffer = XrmQuarkToString(propl->identifier);
    size = strlen(buffer) + 1;

    if (toVal->addr != NULL) {
	if (toVal->size < size) {
	    toVal->size = size;
	    return (False);
	}
	strcpy((char *)toVal->addr, buffer);
    }
    else
	toVal->addr = buffer;
    toVal->size = size;

    return (True);
}
#endif
