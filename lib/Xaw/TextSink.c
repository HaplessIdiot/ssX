/* $XConsortium: TextSink.c,v 1.19 94/04/17 20:13:11 kaleb Exp $ */
/*

Copyright (c) 1989, 1994  X Consortium

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

*/
/* $XFree86$ */

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
  {
    XtNcursorColor,
    XtCColor,
    XtRPixel,
    sizeof(Pixel),
    offset(cursor_color),
    XtRString,
    XtDefaultForeground
  },
};
#undef offset

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

  t_src = (TextSinkObjectClass) wc;
  superC = (TextSinkObjectClass) t_src->object_class.superclass;

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
 *                 highlight - hightlight this text?
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
 *                 stopAtWordBreak - returned position is a word break?
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
 *                 highlight - hightlight this text?
 */
/*ARGSUSED*/
void
XawTextSinkDisplayText(Widget w, int x, int y,
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
 *	staye - whether to turn the cursor on, or off
 *
 * Description:
 *	Places the InsertCursor.
 */
/*ARGSUSED*/
void
XawTextSinkInsertCursor(Widget w, int x, int y, XawTextInsertState state)
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
XawTextSinkClearToBackground(Widget w, int x, int y,
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
 *                 stopAtWordBreak - returned position is a word break?
 *	resPos		- position found (return)
 *	resWidth	- Width actually used (return)
 *	resHeight	- Height actually used (return)
 *
 * Description:
 *	Finds a position in the text.
 */
/*ARGSUSED*/
void
XawTextSinkFindPosition(Widget w, XawTextPosition fromPos, int fromx,
			int width, Bool stopAtWordBreak,
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
XawTextSinkMaxLines(Widget w, unsigned int height)
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
  if (tab_count > 0)
    {
      TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;
      short *char_tabs = (short*)XtMalloc((unsigned)tab_count * sizeof(short));
      short *tab;
      int i;

      for (i = tab_count, tab = char_tabs; i; i--)
	*tab++ = (short)*tabs++;

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
 *	Finds the bounding box for the insert curor (caret).
 */
/*ARGSUSED*/
void
XawTextSinkGetCursorBounds(Widget w, XRectangle *rect)
{
  TextSinkObjectClass cclass = (TextSinkObjectClass)w->core.widget_class;

  (*cclass->text_sink_class.GetCursorBounds)(w, rect);
}
