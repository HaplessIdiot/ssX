/* $XConsortium: Text.c /main/198 1995/08/17 09:37:40 kaleb $ */

/***********************************************************

Copyright (c) 1987, 1988, 1994  X Consortium

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
 * Copyright (c) 1998 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/* $XFree86: xc/lib/Xaw/Text.c,v 3.12 1998/08/16 10:24:37 dawes Exp $ */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/Xfuncs.h>
#include <X11/Xutil.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xmu/SysUtil.h>
#include <X11/Xmu/Xmu.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/MultiSinkP.h>
#include <X11/Xaw/TextP.h>
#include <X11/Xaw/TextSrcP.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/XawImP.h>
#include <X11/Xaw/XawInit.h>
#include "Private.h"
#include "XawI18n.h"

#ifndef MAX_LEN_CT
#define MAX_LEN_CT	6	/* for sequence: ESC $ ( A \xx \xx */
#endif

unsigned long FMT8BIT = 0L;
unsigned long XawFmt8Bit = 0L;
unsigned long XawFmtWide = 0L;

#define SinkClearToBG		XawTextSinkClearToBackground

#define SrcScan			XawTextSourceScan
#define SrcRead			XawTextSourceRead
#define SrcReplace		XawTextSourceReplace
#define SrcSearch		XawTextSourceSearch
#define SrcCvtSel		XawTextSourceConvertSelection
#define SrcSetSelection		XawTextSourceSetSelection

#define MULTI_CLICK_TIME	500L

#define Superclass (&simpleClassRec)

/*
 * Compute a the maximum length of a cut buffer that we can pass at any
 * time.  The 64 allows for the overhead of the Change Property request.
 */
#define MAX_CUT_LEN(dpy)  (XMaxRequestSize(dpy) - 64)

#define	ClearWindow(ctx)						     \
     _XawTextNeedsUpdating((ctx),					     \
			   (ctx)->text.lt.top,				     \
			   (ctx)->text.lt.info[ctx->text.lt.lines].position)

/*
 * Class Methods
 */
static void XawTextClassInitialize(void);
static void XawTextInitialize(Widget, Widget, ArgList, Cardinal*);
static void XawTextRealize(Widget, XtValueMask*, XSetWindowAttributes*);
static void XawTextDestroy(Widget);
static void XawTextResize(Widget);
static void XawTextExpose(Widget, XEvent*, Region);
static Boolean XawTextSetValues(Widget, Widget, Widget, ArgList, Cardinal*);
static void XawTextGetValuesHook(Widget, ArgList, Cardinal*);
static Bool XawTextChangeSensitive(Widget);

/*
 * Prototypes
 */
static XawTextPosition _BuildLineTable(TextWidget, XawTextPosition, int);
static void _CreateCutBuffers(Display*);
static Boolean TextConvertSelection(Widget, Atom*, Atom*, Atom*, XtPointer*,
				unsigned long*, int*);
static int CountLines(TextWidget, XawTextPosition, XawTextPosition);
static void CreateHScrollBar(TextWidget);
static void CreateVScrollBar(TextWidget);
#ifndef notdef
static void CvtStringToScrollMode(XrmValuePtr, Cardinal*,
				  XrmValuePtr, XrmValuePtr);
static Boolean CvtScrollModeToString(Display*, XrmValue*, Cardinal*,
				     XrmValue*, XrmValue*, XtPointer*);
#endif
static void CvtStringToWrapMode(XrmValuePtr, Cardinal*,
				XrmValuePtr, XrmValuePtr);
static Boolean CvtWrapModeToString(Display*, XrmValue*, Cardinal*,
				   XrmValue*, XrmValue*, XtPointer*);
static void DestroyHScrollBar(TextWidget);
static void DestroyVScrollBar(TextWidget);
static void DisplayText(Widget, XawTextPosition, XawTextPosition);
static void DisplayTextWindow(Widget);
static void DoCopyArea(TextWidget, int, int, unsigned int, unsigned int,
		       int, int);
static void DoSelection(TextWidget, XawTextPosition, Time, Bool);
static void ExtendSelection(TextWidget, XawTextPosition, Bool);
static XawTextPosition FindGoodPosition(TextWidget, XawTextPosition);
static void FlushUpdate(TextWidget ctx);
static int GetCutBufferNumber(Atom);
static int GetMaxTextWidth(TextWidget);
static unsigned int GetWidestLine(TextWidget);
static void HScroll(Widget, XtPointer, XtPointer);
static void HJump(Widget, XtPointer, XtPointer);
static void InsertCursor(Widget, XawTextInsertState);
static Bool LineAndXYForPosition(TextWidget, XawTextPosition, int*,
				 int*, int*);
static int LineForPosition(TextWidget, XawTextPosition);
static void TextLoseSelection(Widget, Atom*);
static Bool MatchSelection(Atom, XawTextSelection*);
static void ModifySelection(TextWidget, XawTextPosition, XawTextPosition);
static XawTextPosition PositionForXY(TextWidget, int, int);
static void PositionHScrollBar(TextWidget);
static void PositionVScrollBar(TextWidget);
static void _SetSelection(TextWidget, XawTextPosition, XawTextPosition,
			  Atom*, Cardinal);
static void TextSinkResize(Widget);
static void UpdateTextInRectangle(TextWidget, XRectangle*);
static void UpdateTextInLine(TextWidget, int, int, int);
static void VScroll(Widget, XtPointer, XtPointer);
static void VJump(Widget, XtPointer, XtPointer);

/*
 * External
 */
void _XawTextAlterSelection(TextWidget,
			    XawTextSelectionMode, XawTextSelectionAction,
			    String*, Cardinal*);
#ifndef notdef
void _XawTextCheckResize(TextWidget);
#endif
void _XawTextClearAndCenterDisplay(TextWidget);
void _XawTextExecuteUpdate(TextWidget);
char *_XawTextGetText(TextWidget, XawTextPosition, XawTextPosition);
void _XawTextPrepareToUpdate(TextWidget);
int _XawTextReplace(TextWidget, XawTextPosition, XawTextPosition,
		    XawTextBlock*);
Atom *_XawTextSelectionList(TextWidget, String*, Cardinal);
void _XawTextSetScrollBars(TextWidget);
void _XawTextSetSelection(TextWidget, XawTextPosition, XawTextPosition,
			  String*, Cardinal);
void _XawTextVScroll(TextWidget, int);
void XawTextScroll(TextWidget, int, int);

/* Not used by other modules, but were extern on previous versions
 * of the library
 */
void _XawTextNeedsUpdating(TextWidget, XawTextPosition, XawTextPosition);
void _XawTextShowPosition(TextWidget);

/*
 * From TextAction.c
 */
extern void _XawTextZapSelection(TextWidget, XEvent*, Bool);

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/
/*
 * From TextTr.c
 */
extern char _XawDefaultTextTranslations[];

static XawTextSelectType defaultSelectTypes[] = {
  XawselectPosition, XawselectWord, XawselectLine, XawselectParagraph,
  XawselectAll,      XawselectNull,
};

static XPointer defaultSelectTypesPtr = (XPointer)defaultSelectTypes;
static Dimension defWidth = 100;
static Dimension defHeight = DEFAULT_TEXT_HEIGHT;

#define offset(field) XtOffsetOf(TextRec, field)
static XtResource resources[] = {
  {
    XtNwidth,
    XtCWidth,
    XtRDimension,
    sizeof(Dimension),
    offset(core.width),
    XtRDimension,
    (XtPointer)&defWidth
  },
  {
    XtNcursor,
    XtCCursor,
    XtRCursor,
    sizeof(Cursor),
    offset(simple.cursor),
    XtRString,
    "xterm"
  },
  {
    XtNheight,
    XtCHeight,
    XtRDimension,
    sizeof(Dimension),
    offset(core.height),
    XtRDimension,
    (XtPointer)&defHeight
  },
  {
    XtNdisplayPosition,
    XtCTextPosition,
    XtRInt,
    sizeof(XawTextPosition), 
    offset(text.lt.top),
    XtRImmediate,
    (XtPointer)0
  },
  {
    XtNinsertPosition,
    XtCTextPosition,
    XtRInt,
    sizeof(XawTextPosition),
    offset(text.insertPos),
    XtRImmediate,
    (XtPointer)0
  },
  {
    XtNleftMargin,
    XtCMargin,
    XtRPosition,
    sizeof(Position),
    offset(text.r_margin.left),
    XtRImmediate,
    (XtPointer)2
  },
  {
    XtNrightMargin,
    XtCMargin,
    XtRPosition,
    sizeof(Position),
    offset(text.r_margin.right),
    XtRImmediate,
    (XtPointer)4
  },
  {
    XtNtopMargin,
    XtCMargin,
    XtRPosition,
    sizeof(Position),
    offset(text.r_margin.top),
    XtRImmediate,
    (XtPointer)2
  },
  {
    XtNbottomMargin,
    XtCMargin,
    XtRPosition,
    sizeof(Position),
    offset(text.r_margin.bottom),
    XtRImmediate,
    (XtPointer)2
  },
  {
    XtNselectTypes,
    XtCSelectTypes,
    XtRPointer,
    sizeof(XawTextSelectType*),
    offset(text.sarray),
    XtRPointer,
    (XtPointer)&defaultSelectTypesPtr
  },
  {
    XtNtextSource,
    XtCTextSource,
    XtRWidget,
    sizeof(Widget),
    offset(text.source),
    XtRImmediate,
    NULL
  },
  {
    XtNtextSink,
    XtCTextSink,
    XtRWidget,
    sizeof(Widget),
    offset(text.sink),
    XtRImmediate,
    NULL
  },
  {
    XtNdisplayCaret,
    XtCOutput,
    XtRBoolean,
    sizeof(Boolean),
    offset(text.display_caret),
    XtRImmediate,
    (XtPointer)True
  },
  {
    XtNscrollVertical,
    XtCScroll,
#ifndef notdef
    XtRScrollMode,
    sizeof(XawTextScrollMode),
#else
    XtRBoolean,
    sizeof(Boolean),
#endif
    offset(text.scroll_vert),
    XtRImmediate,
    (XtPointer)False
  },
  {
    XtNscrollHorizontal,
    XtCScroll,
#ifndef notdef
    XtRScrollMode,
    sizeof(XawTextScrollMode),
#else
    XtRBoolean,
    sizeof(Boolean),
#endif
    offset(text.scroll_horiz),
    XtRImmediate,
    (XtPointer)False
  },
  {
    XtNwrap,
    XtCWrap,
    XtRWrapMode,
    sizeof(XawTextWrapMode),
    offset(text.wrap),
    XtRImmediate,
    (XtPointer)XawtextWrapNever
  },
  {
    XtNautoFill,
    XtCAutoFill,
    XtRBoolean,
    sizeof(Boolean),
    offset(text.auto_fill),
    XtRImmediate,
    (XtPointer)False
  },
  {
    XtNadjustScrollbars,
    XtCAdjust,
    XtRBoolean,
    sizeof(Boolean),
    offset(text.adjust_scrollbars),
    XtRImmediate,
    (XtPointer)False
  },
};
#undef offset

#define done(address, type) \
	{ toVal->size = sizeof(type); toVal->addr = (XPointer)address; }

static XrmQuark QWrapNever, QWrapLine, QWrapWord;
#ifndef notdef
static XrmQuark QScrollNever, QScrollWhenNeeded, QScrollAlways;
static XtConvertArgRec ScrollArgs[] = {
  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.self),
   sizeof(Widget)},
};
#endif

#ifndef notdef
/*ARGSUSED*/
static void
CvtStringToScrollMode(XrmValuePtr args, Cardinal *num_args,
		      XrmValuePtr fromVal, XrmValuePtr toVal)
{
  static XawTextScrollMode scrollMode = XawtextScrollNever;
  XrmQuark q;
  char name[32];

  XmuNCopyISOLatin1Lowered(name, (char *)fromVal->addr, sizeof(name));
  q = XrmStringToQuark(name);

  if (q == QScrollNever || q == QScrollWhenNeeded)
    scrollMode = XawtextScrollNever;
  else if (q == QScrollAlways)
    scrollMode = XawtextScrollAlways;
  else
    {
      if (*num_args == 1)
	(void)XtConvertAndStore(*(Widget *)args[0].addr, XtRString, fromVal,
				XtRBoolean, toVal);
      else
	XtStringConversionWarning((char *)fromVal->addr, XtRBoolean);
      return;
    }

  done(&scrollMode, XawTextScrollMode);
}

/*ARGSUSED*/
static Boolean
CvtScrollModeToString(Display *dpy, XrmValue *args, Cardinal *num_args,
		      XrmValue *fromVal, XrmValue *toVal, XtPointer *data)
{
  static char *buffer;
  Cardinal size;

  switch (*(XawTextScrollMode *)fromVal->addr)
	    {
    case XawtextScrollNever:
      buffer = XtEtextScrollNever;
	  break;
    case XawtextScrollWhenNeeded:
    case XawtextScrollAlways:
      buffer = XtEtextScrollAlways;
	      break;
    default:
      XawTypeToStringWarning(dpy, XtRScrollMode);
      toVal->addr = NULL;
      toVal->size = 0;
      return (False);
            }
  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
	    {
      if (toVal->size < size)
		{
	  toVal->size = size;
	  return (False);
        }
      strcpy((char *)toVal->addr, buffer);
    }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = sizeof(String);

  return (True);
}
#endif

/*ARGSUSED*/
static void
CvtStringToWrapMode(XrmValuePtr args, Cardinal *num_args,
		    XrmValuePtr fromVal, XrmValuePtr toVal)
{
  static XawTextWrapMode wrapMode = XawtextWrapNever;
  XrmQuark q;
  char lowerName[6];

  XmuNCopyISOLatin1Lowered(lowerName, (char *)fromVal->addr,
			   sizeof(lowerName));
  q = XrmStringToQuark(lowerName);

  if (q == QWrapNever)
    wrapMode = XawtextWrapNever;
  else if (q == QWrapLine)
    wrapMode = XawtextWrapLine;
  else if (q == QWrapWord)
    wrapMode = XawtextWrapWord;
  else
    XtStringConversionWarning((char *)fromVal->addr, XtRWrapMode);

  done(&wrapMode, XawTextWrapMode);
}

/*ARGSUSED*/
static Boolean
CvtWrapModeToString(Display *dpy, XrmValue *args, Cardinal *num_args,
		    XrmValue *fromVal, XrmValue *toVal, XtPointer *data)
{
  static char *buffer;
  Cardinal size;

  switch (*(XawTextWrapMode *)fromVal->addr)
    {
    case XawtextWrapNever:
      buffer = XtEtextWrapNever;
      break;
    case XawtextWrapLine:
      buffer = XtEtextWrapLine;
      break;
    case XawtextWrapWord:
      buffer = XtEtextWrapWord;
      break;
    default:
      XawTypeToStringWarning(dpy, XtRWrapMode);
      toVal->addr = NULL;
      toVal->size = 0;
      return (False);
    }
  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
    {
      if (toVal->size < size)
	{
	  toVal->size = size;
	  return (False);
	}
      strcpy((char *)toVal->addr, buffer);
    }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = sizeof(String);

  return (True);
}

#undef done

static void
XawTextClassInitialize(void)
{
  if (!XawFmt8Bit)
    FMT8BIT = XawFmt8Bit = XrmPermStringToQuark("FMT8BIT");
  if (!XawFmtWide)
    XawFmtWide = XrmPermStringToQuark("FMTWIDE");

  XawInitializeWidgetSet();

  textClassRec.core_class.num_actions = _XawTextActionsTableCount;
  
  QWrapNever        = XrmPermStringToQuark(XtEtextWrapNever);
  QWrapLine         = XrmPermStringToQuark(XtEtextWrapLine);
  QWrapWord         = XrmPermStringToQuark(XtEtextWrapWord);
  XtAddConverter(XtRString, XtRWrapMode, CvtStringToWrapMode, NULL, 0);
  XtSetTypeConverter(XtRWrapMode, XtRString, CvtWrapModeToString,
		     NULL, 0, XtCacheNone, NULL);
  QScrollNever = XrmPermStringToQuark(XtEtextScrollNever);
  QScrollWhenNeeded = XrmPermStringToQuark(XtEtextScrollWhenNeeded);
  QScrollAlways = XrmPermStringToQuark(XtEtextScrollAlways);
  XtAddConverter(XtRString, XtRScrollMode, CvtStringToScrollMode,
		 &ScrollArgs[0], XtNumber(ScrollArgs));
  XtSetTypeConverter(XtRScrollMode, XtRString, CvtScrollModeToString,
		     NULL, 0, XtCacheNone, NULL);
}

/*
 * Function:
 *	PositionHScrollBar
 *
 * Parameters:
 *	ctx - text widget
 *
 * Description:
 *	Positions the Horizontal scrollbar.
 */
static void
PositionHScrollBar(TextWidget ctx)
{
  Widget hbar = ctx->text.hbar, vbar = ctx->text.vbar;
  Position x, y;
  Dimension width, height;

  if (ctx->text.hbar == NULL)
    return;

  if (ctx->text.adjust_scrollbars)
    {
      x = ctx->text.r_margin.left;
      y = XtHeight(ctx) - ctx->text.r_margin.bottom;
      width = XtWidth(ctx) - (XtBorderWidth(hbar)<<1)
	- ctx->text.r_margin.left - ctx->text.r_margin.right;
      if (vbar != NULL)
	{
	  x -= XtBorderWidth(hbar);
	  width += XtBorderWidth(hbar);
	}

      if (width > XtWidth(ctx))
	width = XtWidth(ctx);
    }
  else
    {
      if (vbar != NULL)
	x = XtWidth(vbar);
      else
	x = -XtBorderWidth(hbar);
      y = XtHeight(ctx) - XtHeight(hbar) - XtBorderWidth(hbar);
      if (vbar != NULL)
	{
	  width = XtWidth(ctx) - XtWidth(vbar) - XtBorderWidth(vbar);
	  if (width > XtWidth(ctx))
	    width = XtWidth(ctx);
	}
      else
	width = XtWidth(ctx);
    }
  height = XtHeight(hbar);

  XtConfigureWidget(hbar, x, y, width, height, XtBorderWidth(hbar));
}

/*
 * Function:
 *	PositionVScrollBar
 *
 * Parameters:
 *	ctx - text widget
 *
 * Description:
 *	Positions the Vertical scrollbar.
 */
static void
PositionVScrollBar(TextWidget ctx)
{
  Widget vbar = ctx->text.vbar, hbar = ctx->text.hbar;
  Position x, y;
  Dimension width, height;

  if (vbar == NULL)
    return;

  if (ctx->text.adjust_scrollbars)
    {
      x = ctx->text.r_margin.left - XtWidth(vbar) - (XtBorderWidth(vbar)<<1);
      y = ctx->text.r_margin.top;
      height = XtHeight(ctx) - (XtBorderWidth(vbar)<<1)
	- ctx->text.r_margin.top - ctx->text.r_margin.bottom;
      if (hbar != NULL)
	height += XtHeight(hbar) + (XtBorderWidth(hbar)<<1);

      if (height > XtHeight(ctx))
	height = XtHeight(ctx);
    }
  else
    {
      x = y = -XtBorderWidth(vbar);
      height = XtHeight(ctx);
    }
  width = XtWidth(vbar);

  XtConfigureWidget(vbar, x, y, width, height, XtBorderWidth(vbar));
}

static void
CreateVScrollBar(TextWidget ctx)
{
  Widget vbar;

  if (ctx->text.vbar != NULL)
    return;

  ctx->text.vbar = vbar =
    XtCreateWidget("vScrollbar", scrollbarWidgetClass, (Widget)ctx, NULL, 0);
  XtAddCallback(vbar, XtNscrollProc, VScroll, (XtPointer)ctx);
  XtAddCallback(vbar, XtNjumpProc, VJump, (XtPointer)ctx);

  ctx->text.r_margin.left += XtWidth(vbar) + XtBorderWidth(vbar);
  if (ctx->text.adjust_scrollbars)
    ctx->text.r_margin.left += XtBorderWidth(vbar);
  ctx->text.margin.left = ctx->text.r_margin.left;

  PositionVScrollBar(ctx);
  PositionHScrollBar(ctx);
  TextSinkResize(ctx->text.sink);

  if (XtIsRealized((Widget)ctx))
    {
      XtRealizeWidget(vbar);
      XtMapWidget(vbar);
    }
}

/*
 * Function:
 *	DestroyVScrollBar
 *
 * Parameters:
 *	ctx - parent text widget
 *
 * Description:
 *	Removes vertical ScrollBar.
 */
static void
DestroyVScrollBar(TextWidget ctx)
{
  Widget vbar = ctx->text.vbar;

  if (vbar == NULL)
    return;

  ctx->text.r_margin.left -= XtWidth(vbar) + XtBorderWidth(vbar);
  if (ctx->text.adjust_scrollbars)
    ctx->text.r_margin.left -= XtBorderWidth(vbar);
  ctx->text.margin.left = ctx->text.r_margin.left;

  XtDestroyWidget(vbar);
  ctx->text.vbar = NULL;
  PositionHScrollBar(ctx);
  TextSinkResize(ctx->text.sink);
}

static void
CreateHScrollBar(TextWidget ctx)
{
  Arg args[1];
  Widget hbar;
  int bottom;

  if (ctx->text.hbar != NULL)
    return;

  XtSetArg(args[0], XtNorientation, XtorientHorizontal);
  ctx->text.hbar = hbar =
    XtCreateWidget("hScrollbar", scrollbarWidgetClass, (Widget)ctx, args, ONE);
  XtAddCallback(hbar, XtNscrollProc, HScroll, (XtPointer)ctx);
  XtAddCallback(hbar, XtNjumpProc, HJump, (XtPointer)ctx);

  bottom = ctx->text.r_margin.bottom + XtHeight(hbar) + XtBorderWidth(hbar);
  if (ctx->text.adjust_scrollbars)
    bottom += XtBorderWidth(hbar);

  ctx->text.margin.bottom = ctx->text.r_margin.bottom = bottom;

  PositionHScrollBar(ctx);
  TextSinkResize(ctx->text.sink);
  if (XtIsRealized((Widget)ctx))
    {
      XtRealizeWidget(hbar);
      XtMapWidget(hbar);
    }
}

/*
 * Function:
 *	DestroyHScrollBar
 *
 * Parameters:
 *	ctx - parent text widget
 *
 * Description:
 *	Removes horizontal ScrollBar.
 */
static void
DestroyHScrollBar(TextWidget ctx)
{
  Widget hbar = ctx->text.hbar;

  if (hbar == NULL)
    return;

  ctx->text.r_margin.bottom -= XtHeight(hbar) + XtBorderWidth(hbar);
  if (ctx->text.adjust_scrollbars)
    ctx->text.r_margin.bottom -= XtBorderWidth(hbar);
  ctx->text.margin.bottom = ctx->text.r_margin.bottom;

  XtDestroyWidget(hbar);
  ctx->text.hbar = NULL;
  TextSinkResize(ctx->text.sink);
}

/*ARGSUSED*/
static void
XawTextInitialize(Widget request, Widget cnew,
		  ArgList args, Cardinal *num_args)
{
  TextWidget ctx = (TextWidget)cnew;

  ctx->text.lt.lines = 0;
  ctx->text.lt.info = (XawTextLineTableEntry *)
    XtCalloc(1, sizeof(XawTextLineTableEntry));
  (void)bzero(&ctx->text.origSel, sizeof(XawTextSelection));
  (void)bzero(&ctx->text.s, sizeof(XawTextSelection));
  ctx->text.s.type = XawselectPosition;
  ctx->text.salt = NULL;
  ctx->text.hbar = ctx->text.vbar = NULL;
  ctx->text.lasttime = 0;
  ctx->text.time = 0;
  ctx->text.showposition = True;
  ctx->text.lastPos = (ctx->text.source != NULL
		       ? XawTextGetLastPosition(ctx) : 0);
  ctx->text.file_insert = NULL;
  ctx->text.search = NULL;
  ctx->text.update = XmuNewScanline(0, 0, 0);
  ctx->text.gc = DefaultGCOfScreen(XtScreen(ctx));
  ctx->text.hasfocus = False;
  ctx->text.margin = ctx->text.r_margin; /* Strucure copy */
  ctx->text.update_disabled = False;
  ctx->text.clear_to_eol = True;
  ctx->text.old_insert = -1;
  ctx->text.mult = 1;
  ctx->text.salt2 = NULL;
  ctx->text.from_left = -1;

  if (XtHeight(ctx) == DEFAULT_TEXT_HEIGHT)
    {
      XtHeight(ctx) = VMargins(ctx);
      if (ctx->text.sink != NULL)
	XtHeight(ctx) += XawTextSinkMaxHeight(ctx->text.sink, 1);
    }

  if (ctx->text.scroll_vert)
	CreateVScrollBar(ctx);
  if (ctx->text.scroll_horiz)
    {
      if (ctx->text.wrap == XawtextWrapNever)
	CreateHScrollBar(ctx);
      else
	{
	  char msg[128];

	  XmuSnprintf(msg, sizeof(msg), "Xaw Text Widget %s:\n %s.",
		      ctx->core.name,
		      "Horizontal scrolling not allowed with wrapping active");
	  XtAppWarning(XtWidgetToApplicationContext(cnew), msg);
	  ctx->text.scroll_horiz = (XawTextScrollMode)False;
	}
    }
}

static void
XawTextRealize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr)
{
  TextWidget ctx = (TextWidget)w;

  (*textClassRec.core_class.superclass->core_class.realize)(w, mask, attr);
  
  if (ctx->text.hbar != NULL)
    {
      XtRealizeWidget(ctx->text.hbar);
      XtMapWidget(ctx->text.hbar);
    }

  if (ctx->text.vbar != NULL)
    {
      XtRealizeWidget(ctx->text.vbar);
      XtMapWidget(ctx->text.vbar);
    }

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
  _XawTextSetScrollBars(ctx);
}

/* Utility routines for support of Text */
static void
_CreateCutBuffers(Display *d)
{
  static struct _DisplayRec {
    struct _DisplayRec *next;
    Display *dpy;
  } *dpy_list = NULL;
  struct _DisplayRec *dpy_ptr;

  for (dpy_ptr = dpy_list; dpy_ptr != NULL; dpy_ptr = dpy_ptr->next)
    if (dpy_ptr->dpy == d)
      return;

  dpy_ptr = XtNew(struct _DisplayRec);
  dpy_ptr->next = dpy_list;
  dpy_ptr->dpy = d;
  dpy_list = dpy_ptr;

#define Create(buffer) \
  XChangeProperty(d, RootWindow(d, 0), buffer, XA_STRING, 8, \
		  PropModeAppend, NULL, 0);

  Create(XA_CUT_BUFFER0);
  Create(XA_CUT_BUFFER1);
  Create(XA_CUT_BUFFER2);
  Create(XA_CUT_BUFFER3);
  Create(XA_CUT_BUFFER4);
  Create(XA_CUT_BUFFER5);
  Create(XA_CUT_BUFFER6);
  Create(XA_CUT_BUFFER7);

#undef Create
}

/*
 * Procedure to manage insert cursor visibility for editable text.  It uses
 * the value of ctx->insertPos and an implicit argument. In the event that
 * position is immediately preceded by an eol graphic, then the insert cursor
 * is displayed at the beginning of the next line.
*/
static void
InsertCursor(Widget w, XawTextInsertState state)
{
  TextWidget ctx = (TextWidget)w;
  int x, y;
  int line;

  if (ctx->text.lt.lines < 1)
    return;

  if (ctx->text.display_caret
      && LineAndXYForPosition(ctx, ctx->text.insertPos, &line, &x, &y))
    {
      if (line < ctx->text.lt.lines)
	y += (ctx->text.lt.info[line + 1].y - ctx->text.lt.info[line].y) + 1;
      else
	y += (ctx->text.lt.info[line].y - ctx->text.lt.info[line - 1].y) + 1;

	XawTextSinkInsertCursor(ctx->text.sink, x, y, state);
    }

  /* Keep Input Method up to speed  */
  if (ctx->simple.international)
    {
      Arg list[1];

      XtSetArg(list[0], XtNinsertPosition, ctx->text.insertPos);
      _XawImSetValues(w, list, 1);
    }
}

/*
 * Procedure to register a span of text that is no longer valid on the display
 * It is used to avoid a number of small, and potentially overlapping, screen
 * updates.
*/
void
_XawTextNeedsUpdating(TextWidget ctx,
		      XawTextPosition left, XawTextPosition right)
{
  XmuSegment segment;

  if (left >= right)
    return;

  segment.x1 = (int)left;
  segment.x2 = (int)right;
  (void)XmuScanlineOrSegment(ctx->text.update, &segment);
}

/*
 * Procedure to read a span of text in Ascii form. This is purely a hack and
 * we probably need to add a function to sources to provide this functionality.
 * [note: this is really a private procedure but is used in multiple modules].
 */
char *
_XawTextGetText(TextWidget ctx, XawTextPosition left, XawTextPosition right)
{
  char *result, *tempResult;
  XawTextBlock text;
  int bytes;

  if (_XawTextFormat(ctx) == XawFmt8Bit)
    bytes = sizeof(unsigned char);
  else if (_XawTextFormat(ctx) == XawFmtWide) 
    bytes = sizeof(wchar_t);
  else /* if there is another fomat, add here */
    bytes = 1;

  /* leave space for ZERO */
  tempResult = result = XtMalloc((unsigned)(right - left + ONE) * bytes);

  while (left < right)
    {
      left = SrcRead(ctx->text.source, left, &text, (int)(right - left));
      if (!text.length)
	break;
      memmove(tempResult, text.ptr, (unsigned)(text.length * bytes));
      tempResult += text.length * bytes;
    }

  if (bytes == sizeof(wchar_t))
    *((wchar_t*)tempResult) = (wchar_t)0;
  else
    *tempResult = '\0';
  return (result);
}

/* Like _XawTextGetText, but enforces ICCCM STRING type encoding.  This
 * routine is currently used to put just the ASCII chars in the selection
 * into a cut buffer.
*/
char *
_XawTextGetSTRING(TextWidget ctx, XawTextPosition left, XawTextPosition right)
{
  unsigned char *s;
  unsigned char c;
  long i, j, n;
  wchar_t *ws, wc;

  /* allow ESC in accordance with ICCCM */
  if (_XawTextFormat(ctx) == XawFmtWide)
    {
      MultiSinkObject sink = (MultiSinkObject)ctx->text.sink;
      ws = (wchar_t *)_XawTextGetText(ctx, left, right);
      n = wcslen(ws);
      for (j = 0, i = 0; j < n; j++)
	{
	  wc = ws[j];
	  if (XwcTextEscapement (sink->multi_sink.fontset, &wc, 1)
	      || (wc == _Xaw_atowc(XawTAB)) || (wc == _Xaw_atowc(XawLF))
	      || (wc == _Xaw_atowc(XawESC)))
            ws[i++] = wc;
	}
      ws[i] = (wchar_t)0;
      return ((char *)ws);
  }
  else
    {
      s = (unsigned char *)_XawTextGetText(ctx, left, right);
      /* only HT and NL control chars are allowed, strip out others */
      n = strlen((char *)s);
      i = 0;
      for (j = 0; j < n; j++)
	{
	  c = s[j];
	  if (((c >= 0x20) && c <= 0x7f)
	      ||(c >= 0xa0) || (c == XawTAB) || (c == XawLF)
	      || (c == XawESC))
	    {
	      s[i] = c;
	      i++;
	    }
	}
      s[i] = 0;
      return ((char *)s);
  }
#undef ESC
}

/* 
 * This routine maps an x and y position in a window that is displaying text
 * into the corresponding position in the source.
 */
static XawTextPosition
PositionForXY(TextWidget ctx, int x, int y)
{
  int fromx, line, width, height;
  XawTextPosition position;

  if (ctx->text.lt.lines == 0)
    return (0);

  for (line = 0; line < ctx->text.lt.lines - 1; line++)
    {
      if (y <= ctx->text.lt.info[line + 1].y)
	break;
    }
  position = ctx->text.lt.info[line].position;
  if (position >= ctx->text.lastPos)
    return (ctx->text.lastPos);
  fromx = (int)ctx->text.margin.left;
  XawTextSinkFindPosition(ctx->text.sink, position, fromx, x - fromx,
			  False, &position, &width, &height);

  if (position > ctx->text.lastPos)
    return (ctx->text.lastPos);

  if (position >= ctx->text.lt.info[line + 1].position)
    position = SrcScan(ctx->text.source, ctx->text.lt.info[line + 1].position,
		       XawstPositions, XawsdLeft, 1, True);

  return (position);
}

/*
 * This routine maps a source position in to the corresponding line number
 * of the text that is displayed in the window.
 */
static int
LineForPosition(TextWidget ctx, XawTextPosition position)
{
  int line;

  for (line = 0; line < ctx->text.lt.lines; line++)
    if (position < ctx->text.lt.info[line + 1].position)
      break;

  return (line);
}

/*
 * This routine maps a source position into the corresponding line number
 * and the x, y coordinates of the text that is displayed in the window.
 */
static Bool
LineAndXYForPosition(TextWidget ctx, XawTextPosition pos,
		     int *line, int *x, int *y)
{
  XawTextPosition linePos, endPos;
  Boolean visible;
  int realW, realH;

  *line = 0;
  *x = ctx->text.margin.left;
  *y = ctx->text.margin.top + 1;
  if ((visible = IsPositionVisible(ctx, pos)) != False)
    {
      *line = LineForPosition(ctx, pos);
      *y = ctx->text.lt.info[*line].y;
      linePos = ctx->text.lt.info[*line].position;
      XawTextSinkFindDistance(ctx->text.sink, linePos,
			      *x, pos, &realW, &endPos, &realH);
      *x += realW;
    }

  return (visible);
}

/*
 * This routine builds a line table. It does this by starting at the
 * specified position and measuring text to determine the staring position
 * of each line to be displayed. It also determines and saves in the
 * linetable all the required metrics for displaying a given line (e.g.
 * x offset, y offset, line length, etc.).
 */
void
_XawTextBuildLineTable(TextWidget ctx, XawTextPosition position,
		       _XtBoolean force_rebuild)
{
  Dimension height = 0;
  int lines = 0;
  Cardinal size;

  if ((int)XtHeight(ctx) > VMargins(ctx))
    {
      height = XtHeight(ctx) - VMargins(ctx);
      lines = XawTextSinkMaxLines(ctx->text.sink, height);
    }
  size = sizeof(XawTextLineTableEntry) * (lines + 1);

  if (lines != ctx->text.lt.lines || ctx->text.lt.info == NULL)
    {
      ctx->text.lt.info = (XawTextLineTableEntry *)
	XtRealloc((char *)ctx->text.lt.info, size);
      ctx->text.lt.lines = lines;
      force_rebuild = True;
    }

  if (force_rebuild || position != ctx->text.lt.top)
    {
      (void)bzero((char *)ctx->text.lt.info, size);
      (void)_BuildLineTable(ctx, ctx->text.lt.top = position, 0);
      ClearWindow(ctx);
    }
}

/*
 * This assumes that the line table does not change size.
 */
static XawTextPosition
_BuildLineTable(TextWidget ctx, XawTextPosition position, int line)
{
  XawTextLineTableEntry *lt = ctx->text.lt.info + line;
  XawTextPosition end;
  Position y;
  int wwidth, width, height;
  Widget src = ctx->text.source;

  if (ctx->text.wrap == XawtextWrapNever)
    wwidth = 32767;
  else
    wwidth = GetMaxTextWidth(ctx);

  y = line == 0 ? ctx->text.margin.top : lt->y;

  /* CONSTCOND */
  while (True)
    {
      XawTextSinkFindPosition(ctx->text.sink, position, ctx->text.margin.left,
			      wwidth, ctx->text.wrap == XawtextWrapWord,
			      &end, &width, &height);

      if (lt->position != position)
	_XawTextNeedsUpdating(ctx, position,
			      end <= position ? position + 1 : end);

      lt->y = y;
      lt->position = position;
      lt->textWidth = width;
      y += height;

      if (ctx->text.wrap == XawtextWrapNever)
	end = SrcScan(src, position, XawstEOL, XawsdRight, 1, True);

      if (end == ctx->text.lastPos
	  && SrcScan(src, position, XawstEOL, XawsdRight, 1, False) == end)
	{
	  Boolean first = True;

	  position = end;
	  while (line++ < ctx->text.lt.lines)
	    {
	      end = lt->position;
	      ++lt;
	      lt->y = y;
	      ++position;
	      if (!first && lt->position - end != 1)
		_XawTextNeedsUpdating(ctx, position - 1, position);
	      lt->position = position;
	      lt->textWidth = 0;
	      y += height;
	      first = False;
	    }
	  return (end);
	}

      ++lt;
      ++line;
      position = end;

      if (line > ctx->text.lt.lines)
	return (position);
    }
  /*NOTREACHED*/
}

/*
 * Function:
 *	GetWidestLine
 *
 * Parameters:
 *	ctx - text widget
 *
 * Description:
 *	  Returns the width (in pixels) of the widest line that
 *		     is currently visable.
 *
 * Returns:
 *	The width of the widest line
 */
static unsigned int
GetWidestLine(TextWidget ctx)
{
  int i;
  unsigned int widest;
  XawTextLineTablePtr lt = &(ctx->text.lt);

  for (i = 0, widest = 1; i < lt->lines; i++)
    if (widest < lt->info[i].textWidth)
      widest = lt->info[i].textWidth;

  return (widest);
}

/*
 * This routine is used by Text to notify an associated scrollbar of the
 * correct metrics (position and shown fraction) for the text being currently
 * displayed in the window.
 */
void
_XawTextSetScrollBars(TextWidget ctx)
{
  float first, last, denom, widest;

  if (ctx->text.scroll_vert)
    {
      if (ctx->text.lastPos == 0)
	first = 0.0;
      else
      first = ctx->text.lt.top / (float)ctx->text.lastPos;

      if (ctx->text.lt.info[ctx->text.lt.lines].position < ctx->text.lastPos)
	last = (ctx->text.lt.info[ctx->text.lt.lines].position
		/ (float)ctx->text.lastPos);
      else
	last = 1.0;

	XawScrollbarSetThumb(ctx->text.vbar, first, last - first);
    }

  if (ctx->text.scroll_horiz)
    {
  denom = GetWidestLine(ctx);
      if (denom == 0)
	denom = 1;
  widest = ((int)XtWidth(ctx) - RHMargins(ctx)) / denom;
      first = ctx->text.r_margin.left - ctx->text.margin.left;
      first /= denom;

      XawScrollbarSetThumb(ctx->text.hbar, first, widest);
    }
}

static void
DoCopyArea(TextWidget ctx, int src_x, int src_y,
	   unsigned int width, unsigned int height, int dst_x, int dst_y)
{
  int x1, y1, x2, y2;

  x1 = ctx->text.r_margin.left;
  y1 = ctx->text.r_margin.top;
  x2 = XtWidth(ctx) - ctx->text.r_margin.right;
  y2 = XtHeight(ctx) - ctx->text.r_margin.bottom;

  if (x1 >= x2 || y1 >= y2)
    return;

  src_x = XawMax(x1, XawMin(src_x, x2));
  src_y = XawMax(y1, XawMin(src_y, y2));
  dst_x = XawMax(x1, XawMin(dst_x, x2));
  dst_y = XawMax(y1, XawMin(dst_y, y2));
  width = XawMax(0, XawMin(x2 - dst_x, (int)width));
  height = XawMax(0, XawMin(y2 - dst_y, (int)height));

  XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx), ctx->text.gc,
	    src_x, src_y, width, height, dst_x, dst_y);
}

/*
 * Function:
 *	XawTextScroll
 *
 * Parameters:
 *	ctx	- text widget
 *	vlines	- number of lines to scroll vertically
 *	hpixels	- number of pixels to scroll horizontally
 *
 * Description:
 *	Generic function for scrolling the text window.
 *	Allows vertical and horizontal scroll at the same time.
 */
void
XawTextScroll(TextWidget ctx, int vlines, int hpixels)
{
  XawTextPosition top;
  XawTextLineTable *lt;
  Arg arglist[1];
  int y0, y1, y2, count, dim;
  int vwidth, vheight;		/* visible width and height */
  Bool scroll;
  XRectangle rect;

  vwidth = GetMaxTextWidth(ctx);
  vheight = (int)XtHeight(ctx) - RVMargins(ctx);
  lt = &ctx->text.lt;

  if (!lt || vwidth <= 0 || vheight <= 0)
    return;

  scroll = ctx->core.background_pixmap == XtUnspecifiedPixmap;

  /*
   * Do the horizontall scrolling
   */
  if (hpixels < 0 && ctx->text.margin.left - hpixels > ctx->text.r_margin.left)
    hpixels = ctx->text.margin.left - ctx->text.r_margin.left;
  ctx->text.margin.left -= hpixels;

  /*
   *   Checks the requested number of lines and calculates the top
   * of the line table
   */
  if (vlines < 0)		/* VScroll Up */
	{
	  top = SrcScan(ctx->text.source, lt->top, XawstEOL,
			XawsdLeft, -vlines + 1, False);
      if (IsPositionVisible(ctx, 0))
	vlines = 0;
      else if (ctx->text.wrap != XawtextWrapNever)
	    {
	  int n_lines = CountLines(ctx, top, lt->top);

	  count = -vlines;
	  while (count++ < n_lines)
		  XawTextSinkFindPosition(ctx->text.sink, top,
				    ctx->text.margin.left,
				    vwidth, ctx->text.wrap == XawtextWrapWord,
				    &top, &dim, &dim);
		}
      if (-vlines >= ctx->text.lt.lines)
	scroll = False;
	    }
  else if (vlines > 0)		/* VScroll Down */
    {
      if (LineForPosition(ctx, ctx->text.lastPos) == 0)
	vlines = 0;
      if (vlines < lt->lines)
	  top = XawMin(lt->info[vlines].position, ctx->text.lastPos);
      else if (ctx->text.wrap == XawtextWrapNever)
	top = SrcScan(ctx->text.source, lt->top, XawstEOL,
		      XawsdRight, vlines, True);
      else
	{
	  top = lt->top;
	  count = 0;
	  while (count++ < vlines)
	    XawTextSinkFindPosition(ctx->text.sink, top,
				    ctx->text.margin.left,
				    vwidth, ctx->text.wrap == XawtextWrapWord,
				    &top, &dim, &dim);
	}
      if (vlines >= ctx->text.lt.lines
	  || lt->info[vlines].position >= ctx->text.lastPos)
	scroll = False;
	}

  if (hpixels)
    _XawTextNeedsUpdating(ctx, 0, vlines > 0 ? lt->info[lt->lines].position
			  : ctx->text.lastPos);

  if (!vlines)
	{
      _XawTextSetScrollBars(ctx);
      return;
    }

  /*
   * Rebuild the line table, doing the vertical scroll
   */
  (void)_BuildLineTable(ctx, ctx->text.lt.top = top, 0);

  XtSetArg(arglist[0], XtNinsertPosition, lt->top + lt->lines);
  _XawImSetValues((Widget)ctx, arglist, 1);
  _XawTextSetScrollBars(ctx);

  if (hpixels)
    return;
  else if (!scroll)
    {
      _XawTextNeedsUpdating(ctx, 0, vlines > 0 ? lt->info[lt->lines].position
			    : ctx->text.lastPos);
      return;
    }

      rect.x = ctx->text.r_margin.left;
      rect.width = vwidth;
  if (vlines < 0)
    {
      y0 = lt->info[-vlines].y;
      rect.y = y1 = ctx->text.r_margin.top;
      y2 = lt->info[lt->lines].y;
      rect.height = y1 - y0;
      DoCopyArea(ctx, rect.x, y1, rect.width, y2 - y0, rect.x, y0);
    }
  else
    {
      y0 = ctx->text.r_margin.top;
      rect.y = y1 = lt->info[vlines].y;
      y2 = lt->info[lt->lines].y;
      rect.height = y2 - y0;
      DoCopyArea(ctx, rect.x, y1, rect.width, y2 - y1, rect.x, y0);
    }
  UpdateTextInRectangle(ctx, &rect);
}

/*
 * The routine will scroll the displayed text by lines.  If the arg  is
 * positive, move up; otherwise, move down. [note: this is really a private
 * procedure but is used in multiple modules].
 */
void
_XawTextVScroll(TextWidget ctx, int n)
{
  XawTextScroll(ctx, n, 0);
}

/*ARGSUSED*/
static void
HScroll(Widget w, XtPointer closure, XtPointer callData)
{
  TextWidget ctx = (TextWidget)closure;
  int pixels = (int)callData;

  if (pixels > 0)
    {
      int max;

      max = (int)GetWidestLine(ctx) + (int)ctx->text.margin.left
	- ctx->text.r_margin.left;
      max = XawMax(0, max);
      pixels = XawMin(pixels, max);
    }

  if (pixels)
    {
      _XawTextPrepareToUpdate(ctx);
      XawTextScroll(ctx, 0, pixels);
      _XawTextSetScrollBars(ctx);
      _XawTextExecuteUpdate(ctx);
    }
}

/*ARGSUSED*/
static void
HJump(Widget w, XtPointer closure, XtPointer callData)
{
  TextWidget ctx = (TextWidget)closure;
  float percent = *(float *)callData;
  int pixels;

  pixels = ((int)ctx->text.margin.left
	    - (ctx->text.r_margin.left - (int)(percent * GetWidestLine(ctx))));

  HScroll(w, (XtPointer)ctx, (XtPointer)pixels);
}

/*
 * Function:
 *	UpdateTextInLine
 *
 * Parameters:
 *	ctx  - text widget
 *	line - line to update
 *	x1   - left pixel
 *	x2   - right pixel
 *
 * Description:
 *	Updates the text in the given line and pixel interval
 */
static void
UpdateTextInLine(TextWidget ctx, int line, int x1, int x2)
{
  XawTextLineTableEntry *lt = ctx->text.lt.info + line;
  XawTextPosition left, right;
  int from_x, width, height;

  if (lt->position >= ctx->text.lastPos
      || ctx->text.margin.left > x2
      || (int)lt->textWidth + ctx->text.margin.left < x1)
    {
      /* Mark line to be cleared */
      if (ctx->text.clear_to_eol)
	_XawTextNeedsUpdating(ctx, lt->position, lt->position + 1);
      return;
    }

  from_x = ctx->text.margin.left;
  XawTextSinkFindPosition(ctx->text.sink, lt->position,
			  from_x, x1 - from_x,
			  False,
			  &left, &width, &height);
  if (x2 >= lt->textWidth - from_x)
    right = lt[1].position - 1;
  else
    {
      from_x += width;
      XawTextSinkFindPosition(ctx->text.sink, left,
			      from_x, x2 - from_x,
			      False,
			      &right, &width, &height);
    }

  if (right + 1 <= lt[1].position)
    ++right;

  /* Mark text interval to be repainted */
  _XawTextNeedsUpdating(ctx, left, right);
}

/*
 * The routine will scroll the displayed text by pixels.  If the calldata is
 * positive, move up; otherwise, move down.
 */
/*ARGSUSED*/
static void
VScroll(Widget w, XtPointer closure, XtPointer callData)
{
  TextWidget ctx = (TextWidget)closure;
  int height, lines = (int)callData;

  height = (int)XtHeight(ctx) - (int)VMargins(ctx);
  if (height < 1)
    height = 1;
  lines = (int)(lines * (int)ctx->text.lt.lines) / height;
  _XawTextPrepareToUpdate(ctx);
  XawTextScroll(ctx, lines, 0);
  _XawTextExecuteUpdate(ctx);
}

/*ARGSUSED*/
static void
VJump(Widget w, XtPointer closure, XtPointer callData)
{
  float percent = *(float *)callData;
  TextWidget ctx = (TextWidget)closure;
  XawTextPosition top, last, position;
  XawTextLineTable *lt = &(ctx->text.lt);
  int dim, vlines = 0, wwidth = GetMaxTextWidth(ctx);
  Bool scroll = True;

  position = percent * ctx->text.lastPos;
  top = lt->top;

  if (!lt->lines || (position >= lt->top && position < lt->info[1].position))
    return;

  if (position > lt->top)	/* VScroll Up */
    {
      if (position > lt->top && position < lt->info[lt->lines].position)
	vlines = LineForPosition(ctx, position);
      else
	{
	  scroll = False;
	  top = SrcScan(ctx->text.source, position, XawstEOL,
			XawsdLeft, 1, False);
	  if (ctx->text.wrap != XawtextWrapNever)
	    {
	      last = top;
	      while (last < position)
	    {
	      XawTextSinkFindPosition(ctx->text.sink, last,
				      ctx->text.margin.left,
				      wwidth,
				      ctx->text.wrap == XawtextWrapWord,
				      &last, &dim, &dim);
		  if (last < position)
		    top = last;
	}
	    }
	}
    }
  else			/* VScroll Down */
    {
      /*
       * Calculates the number of lines
       */
      while (top > position)
	{
	  last = top;
	  top = SrcScan(ctx->text.source, top, XawstEOL,
			XawsdLeft, 2, False);
	  vlines -= CountLines(ctx, top, last);
	  if (-vlines >= ctx->text.lt.lines)
	{
	      scroll = False;
	      top = SrcScan(ctx->text.source, position, XawstEOL,
			    XawsdLeft, 1, False);
	      break;
	    }
	}
      /*
       * Normalize
       */
      if (ctx->text.wrap != XawtextWrapNever)
	{
	  last = top;
	  while (last < position)
	    {
	      XawTextSinkFindPosition(ctx->text.sink, last,
				      ctx->text.margin.left,
				      wwidth,
				      ctx->text.wrap == XawtextWrapWord,
				      &last, &dim, &dim);
	      if (last < position)
		top = last;
	      ++vlines;
	    }
	}
    }

  if (vlines || !scroll)
    {
      _XawTextPrepareToUpdate(ctx);
      if (scroll)
  XawTextScroll(ctx, vlines, 0);
      else
	_XawTextBuildLineTable(ctx, top, True);
  _XawTextExecuteUpdate(ctx);
    }
}

static Bool
MatchSelection(Atom selection, XawTextSelection *s)
{
  Atom *match;
  int count;

  for (count = 0, match = s->selections; count < s->atom_count;
       match++, count++)
    if (*match == selection)
      return (True);

  return (False);
}

static Boolean
TextConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type,
		 XtPointer *value, unsigned long *length, int *format)
{
  Display *d = XtDisplay(w);
  TextWidget ctx = (TextWidget)w;
  Widget src = ctx->text.source;
  XawTextEditType edit_mode;
  Arg args[1];
  XawTextSelectionSalt *salt = NULL;
  XawTextSelection *s;

  if (*target == XA_TARGETS(d))
    {
      Atom *targetP, *std_targets;
      unsigned long std_length;

      if (SrcCvtSel(src, selection, target, type, value, length, format))
	return (True);

      XmuConvertStandardSelection(w, ctx->text.time, selection,
				  target, type, (XtPointer*)&std_targets,
				  &std_length, format);

      *value = XtMalloc((unsigned)sizeof(Atom)*(std_length + 7));
      targetP = *(Atom**)value;
      *length = std_length + 6;
      *targetP++ = XA_STRING;
      *targetP++ = XA_TEXT(d);
      *targetP++ = XA_COMPOUND_TEXT(d);
      *targetP++ = XA_LENGTH(d);
      *targetP++ = XA_LIST_LENGTH(d);
      *targetP++ = XA_CHARACTER_POSITION(d);

      XtSetArg(args[0], XtNeditType, &edit_mode);
      XtGetValues(src, args, ONE);

      if (edit_mode == XawtextEdit)
	{
	  *targetP++ = XA_DELETE(d);
	  (*length)++;
	}
      (void)memmove((char*)targetP, (char*)std_targets,
		    sizeof(Atom) * std_length);
      XtFree((char*)std_targets);
    *type = XA_ATOM;
    *format = 32;
    return (True);
  }

  if (SrcCvtSel(src, selection, target, type, value, length, format))
    return (True);

  if (MatchSelection(*selection, &ctx->text.s))
    s = &ctx->text.s;
  else
    {
      for (salt = ctx->text.salt; salt; salt = salt->next)
	if (MatchSelection(*selection, &salt->s))
	  break;
      if (!salt)
	return (False);
      s = &salt->s;
    }
  if (*target == XA_STRING || *target == XA_TEXT(d)
      || *target == XA_COMPOUND_TEXT(d))
    {
      if (*target == XA_TEXT(d))
	{
	  if (_XawTextFormat(ctx) == XawFmtWide)
	    *type = XA_COMPOUND_TEXT(d);
	  else
	    *type = XA_STRING;
	}
      else
	*type = *target;
      /* 
       * If salt is True, the salt->contents stores CT string,
       * its length is measured in bytes.
       * Refer to _XawTextSaltAwaySelection().
       *
       * by Li Yuhong, Mar. 20, 1991.
       */
      if (!salt)
	{
	  *value = _XawTextGetSTRING(ctx, s->left, s->right);
	  if (_XawTextFormat(ctx) == XawFmtWide)
	    {
	      XTextProperty textprop;
	      if (XwcTextListToTextProperty(d, (wchar_t **)value, 1,
					    XCompoundTextStyle, &textprop)
		  <  Success)
		{
		  XtFree((char *)*value);
		  return (False);
		}
	      XtFree((char *)*value);
	      *value = (XtPointer)textprop.value;
	      *length = textprop.nitems;
	    }
	  else
	    *length = strlen((char *)*value);
	}
      else
	{
	  *value = XtMalloc((salt->length + 1) * sizeof(unsigned char));
	  strcpy ((char *)*value, salt->contents);
	  *length = salt->length;
	}
      if (_XawTextFormat(ctx) == XawFmtWide && *type == XA_STRING)
	{
	  XTextProperty textprop;
	  wchar_t **wlist;
	  int count;

	  textprop.encoding = XA_COMPOUND_TEXT(d);
	  textprop.value = (unsigned char *)*value;
	  textprop.nitems = strlen(*value);
	  textprop.format = 8;
	  if (XwcTextPropertyToTextList(d, &textprop, (wchar_t ***)&wlist,
					&count) < Success)
	    {
	      XtFree((char *)*value);
	      return (False);
	    }
	  XtFree((char *)*value);
	  if (XwcTextListToTextProperty(d, (wchar_t **)wlist, 1,
					XStringStyle, &textprop) < Success)
	    {
	      XwcFreeStringList((wchar_t**) wlist);
	      return (False);
	    }
	  *value = (XtPointer)textprop.value;
	  *length = textprop.nitems;
	  XwcFreeStringList((wchar_t**)wlist);
	}
      *format = 8;
      return (True);
    }

  if ((*target == XA_LIST_LENGTH(d)) || (*target == XA_LENGTH(d)))
    {
      long * temp;

      temp = (long *)XtMalloc((unsigned)sizeof(long));
      if (*target == XA_LIST_LENGTH(d))
	*temp = 1L;
      else			/* *target == XA_LENGTH(d) */
	*temp = (long) (s->right - s->left);

      *value = (XPointer)temp;
      *type = XA_INTEGER;
      *length = 1L;
      *format = 32;
      return (True);
    }

  if (*target == XA_CHARACTER_POSITION(d))
    {
      long * temp;

      temp = (long *)XtMalloc((unsigned)(2 * sizeof(long)));
      temp[0] = (long)(s->left + 1);
      temp[1] = s->right;
      *value = (XPointer)temp;
      *type = XA_SPAN(d);
      *length = 2L;
      *format = 32;
      return (True);
    }

  if (*target == XA_DELETE(d))
    {
      if (!salt)
	_XawTextZapSelection(ctx, NULL, True);
      *value = NULL;
      *type = XA_NULL(d);
      *length = 0;
      *format = 32;
      return (True);
    }

  if (XmuConvertStandardSelection(w, ctx->text.time, selection, target, type,
				  (XtPointer *)value, length, format))
    return (True);

  /* else */
  return (False);
}

/*
 * Function:
 *	GetCutBuffferNumber
 *
 * Parameters:
 *	atom - atom to check
 *
 * Description:
 *	Returns the number of the cut buffer.
 *
 * Returns:
 *	The number of the cut buffer representing this atom or NOT_A_CUT_BUFFER
 */
#define NOT_A_CUT_BUFFER -1
static int
GetCutBufferNumber(Atom atom)
{
  if (atom == XA_CUT_BUFFER0) return (0);
  if (atom == XA_CUT_BUFFER1) return (1);
  if (atom == XA_CUT_BUFFER2) return (2);
  if (atom == XA_CUT_BUFFER3) return (3);
  if (atom == XA_CUT_BUFFER4) return (4);
  if (atom == XA_CUT_BUFFER5) return (5);
  if (atom == XA_CUT_BUFFER6) return (6);
  if (atom == XA_CUT_BUFFER7) return (7);
  return (NOT_A_CUT_BUFFER);
}

static void
TextLoseSelection(Widget w, Atom *selection)
{
  TextWidget ctx = (TextWidget)w;
  Atom *atomP;
  int i;
  XawTextSelectionSalt*salt, *prevSalt, *nextSalt;

  _XawTextPrepareToUpdate(ctx);

  atomP = ctx->text.s.selections;
  for (i = 0 ; i < ctx->text.s.atom_count; i++, atomP++)
    if ((*selection == *atomP)
	|| (GetCutBufferNumber(*atomP) != NOT_A_CUT_BUFFER))
      *atomP = (Atom)0;

  while (ctx->text.s.atom_count
	 && ctx->text.s.selections[ctx->text.s.atom_count - 1] == 0)
    ctx->text.s.atom_count--;

  /*
   * Must walk the selection list in opposite order from UnsetSelection
   */
  atomP = ctx->text.s.selections;
  for (i = 0 ; i < ctx->text.s.atom_count; i++, atomP++)
    if (*atomP == (Atom)0)
      {
	*atomP = ctx->text.s.selections[--ctx->text.s.atom_count];
	while (ctx->text.s.atom_count
	       && ctx->text.s.selections[ctx->text.s.atom_count-1] == 0)
	  ctx->text.s.atom_count--;
      }

  if (ctx->text.s.atom_count == 0)
    ModifySelection(ctx, ctx->text.insertPos, ctx->text.insertPos);

  if (ctx->text.old_insert >= 0) /* Update in progress. */
    _XawTextExecuteUpdate(ctx);

  prevSalt = 0;
  for (salt = ctx->text.salt; salt; salt = nextSalt)
    {
      atomP = salt->s.selections;
      nextSalt = salt->next;
      for (i = 0 ; i < salt->s.atom_count; i++, atomP++)
	if (*selection == *atomP)
	  *atomP = (Atom)0;

      while (salt->s.atom_count
	     && salt->s.selections[salt->s.atom_count-1] == 0)
	salt->s.atom_count--;
    	
      /*
       * Must walk the selection list in opposite order from UnsetSelection
       */
      atomP = salt->s.selections;
      for (i = 0 ; i < salt->s.atom_count; i++, atomP++)
	if (*atomP == (Atom)0)
	  {
	    *atomP = salt->s.selections[--salt->s.atom_count];
	    while (salt->s.atom_count
		   && salt->s.selections[salt->s.atom_count-1] == 0)
	      salt->s.atom_count--;
	  }
      if (salt->s.atom_count == 0)
	{
	  XtFree ((char *) salt->s.selections);
	  XtFree (salt->contents);
	  if (prevSalt)
	    prevSalt->next = nextSalt;
	  else
	    ctx->text.salt = nextSalt;
	  XtFree((char *)salt);
	}
      else
	prevSalt = salt;
    }
}

void
_XawTextSaltAwaySelection(TextWidget ctx, Atom *selections, int num_atoms)
{
  XawTextSelectionSalt *salt;
  int i, j;

  for (i = 0; i < num_atoms; i++)
    TextLoseSelection((Widget)ctx, selections + i);
  if (num_atoms == 0)
    return;
  salt = (XawTextSelectionSalt *)
    XtMalloc((unsigned)sizeof(XawTextSelectionSalt));
  if (!salt)
    return;
  salt->s.selections = (Atom *)XtMalloc((unsigned)(num_atoms * sizeof(Atom)));
  if (!salt->s.selections)
    {
      XtFree((char *)salt);
      return;
    }
  salt->s.left = ctx->text.s.left;
  salt->s.right = ctx->text.s.right;
  salt->s.type = ctx->text.s.type;
  salt->contents = _XawTextGetSTRING(ctx, ctx->text.s.left, ctx->text.s.right);
  if (_XawTextFormat(ctx) == XawFmtWide)
    {
      XTextProperty textprop;
      if (XwcTextListToTextProperty(XtDisplay((Widget)ctx),
				    (wchar_t**)(&(salt->contents)), 1,
				    XCompoundTextStyle,
				    &textprop) < Success)
	{
	  XtFree(salt->contents);
	  salt->length = 0;
	  return;
	}
      XtFree(salt->contents);
      salt->contents = (char *)textprop.value;
      salt->length = textprop.nitems;
    }
  else
    salt->length = strlen (salt->contents);
  salt->next = ctx->text.salt;
  ctx->text.salt = salt;
  j = 0;
  for (i = 0; i < num_atoms; i++)
    {
      if (GetCutBufferNumber (selections[i]) == NOT_A_CUT_BUFFER)
	{
	  salt->s.selections[j++] = selections[i];
	  XtOwnSelection((Widget)ctx, selections[i], ctx->text.time,
			 TextConvertSelection, TextLoseSelection, NULL);
	}
    }
  salt->s.atom_count = j;
}

static void
_SetSelection(TextWidget ctx, XawTextPosition left, XawTextPosition right,
	      Atom *selections, Cardinal count)
{
  XawTextPosition pos;

  if (left < ctx->text.s.left)
    {
      pos = Min(right, ctx->text.s.left);
      _XawTextNeedsUpdating(ctx, left, pos);
    }
  if (left > ctx->text.s.left)
    {
      pos = Min(left, ctx->text.s.right);
      _XawTextNeedsUpdating(ctx, ctx->text.s.left, pos);
    }
  if (right < ctx->text.s.right)
    {
      pos = Max(right, ctx->text.s.left);
      _XawTextNeedsUpdating(ctx, pos, ctx->text.s.right);
    }
  if (right > ctx->text.s.right)
    {
      pos = Max(left, ctx->text.s.right);
      _XawTextNeedsUpdating(ctx, pos, right);
    }

  ctx->text.s.left = left;
  ctx->text.s.right = right;

  SrcSetSelection(ctx->text.source, left, right,
		  (count == 0) ? None : selections[0]);

  if (left < right)
    {
      Widget w = (Widget)ctx;
      int buffer;

      while (count)
	{
	  Atom selection = selections[--count];

	  /*
	   * If this is a cut buffer
	   */
	  if ((buffer = GetCutBufferNumber(selection)) != NOT_A_CUT_BUFFER)
	    {
	      unsigned char *ptr, *tptr;
	      unsigned int amount, max_len = MAX_CUT_LEN(XtDisplay(w));
	      unsigned long len;

	      tptr= ptr= (unsigned char *)_XawTextGetSTRING(ctx,
							    ctx->text.s.left,
							    ctx->text.s.right);
	      if (_XawTextFormat(ctx) == XawFmtWide)
		{
		  /*
		   * Only XA_STRING(Latin 1) is allowed in CUT_BUFFER,
		   * so we get it from wchar string, then free the wchar string
		   */
		  XTextProperty textprop;

		  if (XwcTextListToTextProperty(XtDisplay(w), (wchar_t**)&ptr,
						1, XStringStyle, &textprop)
		      <  Success)
		    {
		      XtFree((char *)ptr);
		      return;
		    }
		  XtFree((char *)ptr);
		  tptr = ptr = textprop.value;
		} 
	      if (buffer == 0)
		{
		  _CreateCutBuffers(XtDisplay(w));
		  XRotateBuffers(XtDisplay(w), 1);
		}
	      amount = Min ((len = strlen((char *)ptr)), max_len);
	      XChangeProperty(XtDisplay(w), RootWindow(XtDisplay(w), 0),
			      selection, XA_STRING, 8, PropModeReplace,
			      ptr, amount);

	      while (len > max_len)
		{
		  len -= max_len;
		  tptr += max_len;
		  amount = Min (len, max_len);
		  XChangeProperty(XtDisplay(w), RootWindow(XtDisplay(w), 0),
				  selection, XA_STRING, 8, PropModeAppend,
				  tptr, amount);
		}
	      XtFree ((char *)ptr);
	    }
	  else			/* This is a real selection */
	    XtOwnSelection(w, selection, ctx->text.time, TextConvertSelection,
			   TextLoseSelection, NULL);
	}
    }
  else
    XawTextUnsetSelection((Widget)ctx);
}

/*
 * Function:
 *	_XawTextReplace
 *
 * Parameters:
 *	ctx   - text widget
 *	left  - left offset
 *	right - right offset
 *	block - text block
 *
 * Description:
 *	  Replaces the text between left and right by the text in block.
 *	  Does all the required calculations of offsets, and rebuild the
 *	the line table, from the insertion point (or previous line, if
 *	wrap mode is 'word').
 *
 * Returns:
 *	XawEditDone     - success
 *	any other value - error code
 */
int
_XawTextReplace(TextWidget ctx, XawTextPosition left, XawTextPosition right,
		XawTextBlock *block)
{
  Arg args[1];
  Widget src;
  XawTextEditType edit_mode;
  XawTextPosition update_from, update_to, top;
  Boolean update_disabled;
  int delta, error, line, line_from;

  src = ctx->text.source;
  XtSetArg(args[0], XtNeditType, &edit_mode);
  XtGetValues(src, args, 1);

  if (edit_mode == XawtextAppend)
	{
      if (left == right && block->length == 0)
	return (XawEditDone);
      else if (block->length == 0)
	  return (XawEditError);
      ctx->text.insertPos = left = right = ctx->text.lastPos;
	}

  if ((error = SrcReplace(src, left, right, block)) != 0)
      return (error);

  update_from = left;
  update_to = XawMax(right, left + block->length);
  update_to = SrcScan(src, update_to, XawstEOL, XawsdRight, 1, False);
  if (update_from == update_to)
    ++update_to;	/* Force clear to eol if deleting last char(s) */
  update_disabled = ctx->text.update_disabled;
  ctx->text.update_disabled = True;
  ctx->text.lastPos = XawTextGetLastPosition(ctx);
  top = ctx->text.lt.info[0].position;
  delta = block->length - (right - left);
  ctx->text.clear_to_eol = block->length != 1 && right - left != 0;

  XawTextUnsetSelection((Widget)ctx);

  if (delta)
    {
      int i;
      XmuSegment *seg;

      for (seg = ctx->text.update->segment; seg; seg = seg->next)
    {
	  if (seg->x1 > (int)left)
	    break;
	  else if (seg->x2 > (int)left)
	    {
	      seg->x2 += delta;
	      seg = seg->next;
	      break;
	    }
	}
      for (; seg; seg = seg->next)
	{
	  seg->x1 += delta;
	  seg->x2 += delta;
	}
      XmuOptimizeScanline(ctx->text.update);

      for (i = 0; i <= ctx->text.lt.lines; i++)
	if (ctx->text.lt.info[i].position > left)
	  break;
      for (; i <= ctx->text.lt.lines; i++)
	ctx->text.lt.info[i].position += delta;
    }

  if (top != ctx->text.lt.info[0].position)
    {
      line_from = line = 0;
      ctx->text.lt.top = top = SrcScan(src, ctx->text.lt.info[0].position,
				       XawstEOL, XawsdLeft, 1, False);
    }
  else
    {
      line_from = line = LineForPosition(ctx, update_from + delta);
      top = ctx->text.lt.info[line].position;
    }

  if (line > 0 && ctx->text.wrap == XawtextWrapWord)
    {
      --line;
      top = ctx->text.lt.info[line].position;
    }

  (void)_BuildLineTable(ctx, top, line);

  if (ctx->text.wrap == XawtextWrapWord)
    {
      if (line_from != LineForPosition(ctx, update_from)
	  || line_from != LineForPosition(ctx, update_to))
	{
	  ctx->text.clear_to_eol = True;
	  update_from = SrcScan(src, update_from,
				XawstWhiteSpace, XawsdLeft, 1, True);
	  if (update_to >= ctx->text.lastPos)
	    ++update_to;
	}
    }
  else if (!ctx->text.clear_to_eol)
    {
      if (LineForPosition(ctx, update_from)
	  != LineForPosition(ctx, update_to))
	ctx->text.clear_to_eol = True;
    }

  _XawTextNeedsUpdating(ctx, update_from, update_to);
  ctx->text.update_disabled = update_disabled;

  return (XawEditDone);
}

/*
 * This routine will display text between two arbitrary source positions.
 * In the event that this span contains highlighted text for the selection,
 * only that portion will be displayed highlighted.
 */
static void
DisplayText(Widget w, XawTextPosition left, XawTextPosition right)
{
  static XmuSegment segment;
  static XmuScanline next;
  static XmuScanline scanline = {0, &segment, &next};
  static XmuArea area = {&scanline};

  TextWidget ctx = (TextWidget)w;
  int x, y, line;
  XawTextPosition start, end, last, final;
  XmuScanline *scan;
  XmuSegment *seg;
  XmuArea *clip = NULL;
  Bool cleol = ctx->text.clear_to_eol;

  left = left < ctx->text.lt.top ? ctx->text.lt.top : left;

  if (left > right || !LineAndXYForPosition(ctx, left, &line, &x, &y))
    return;

  last = XawTextGetLastPosition(ctx);
  segment.x2 = (int)XtWidth(ctx) - ctx->text.r_margin.right;

  if (cleol)
    clip = XmuCreateArea();

  for (start = left; start < right && line < ctx->text.lt.lines; line++)
	{
      if ((end = ctx->text.lt.info[line + 1].position) > right)
	end = right;

      final = end;
      if (end > last)
	end = last;

      if (end > start)
	{
	  if (start >= ctx->text.s.right || end <= ctx->text.s.left)
	    XawTextSinkDisplayText(ctx->text.sink, x, y, start, end, False);
	  else if (start >= ctx->text.s.left && end <= ctx->text.s.right)
	    XawTextSinkDisplayText(ctx->text.sink, x, y, start, end, True);
	  else
	    {
	      DisplayText(w, start, ctx->text.s.left);
	      DisplayText(w, Max(start, ctx->text.s.left),
			  Min(end, ctx->text.s.right));
	      DisplayText(w, ctx->text.s.right, end);
	    }
	}

      if (cleol)
	    {
	  segment.x1 = (ctx->text.lt.info[line].textWidth
			+ (x = ctx->text.margin.left));
	  if (XmuValidSegment(&segment))
	    {
	      scanline.y = y;
	      next.y = ctx->text.lt.info[line + 1].y;
	      XmuAreaOr(clip, &area);
	    }
	}

      start = final;
      y = ctx->text.lt.info[line + 1].y;
    }

  if (cleol)
    {
      for (scan = clip->scanline; scan && scan->next; scan = scan->next)
	for (seg = scan->segment; seg; seg = seg->next)
	  SinkClearToBG(ctx->text.sink,
			seg->x1, scan->y,
			seg->x2 - seg->x1, scan->next->y - scan->y);
      XmuDestroyArea(clip);
    }
}

/*
 * This routine implements multi-click selection in a hardwired manner.
 * It supports multi-click entity cycling (char, word, line, file) and mouse
 * motion adjustment of the selected entitie (i.e. select a word then, with
 * button still down, adjust wich word you really meant by moving the mouse).
 * [NOTE: This routine is to be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
 */
static void
DoSelection(TextWidget ctx, XawTextPosition pos, Time time, Bool motion)
{
  XawTextPosition newLeft, newRight;
  XawTextSelectType newType, *sarray;
  Widget src = ctx->text.source;

  if (motion)
    newType = ctx->text.s.type;
  else
    {
      if ((abs((long) time - (long) ctx->text.lasttime) < MULTI_CLICK_TIME)
	  && (pos >= ctx->text.s.left && pos <= ctx->text.s.right))
	{
	  sarray = ctx->text.sarray;
	  for (; *sarray != XawselectNull && *sarray != ctx->text.s.type;
	       sarray++)
	    ;
	  if (*sarray == XawselectNull)
	    newType = *(ctx->text.sarray);
	  else
	    {
	      newType = *(sarray + 1);
	      if (newType == XawselectNull)
		newType = *(ctx->text.sarray);
	    }
	}
      else		/* single-click event */
	newType = *(ctx->text.sarray);

      ctx->text.lasttime = time;
    }
  switch (newType)
    {
    case XawselectPosition:
      newLeft = newRight = pos;
      break;
    case XawselectChar:
      newLeft = pos;
      newRight = SrcScan(src, pos, XawstPositions, XawsdRight, 1, False);
      break;
    case XawselectWord:
    case XawselectParagraph:
      {
	XawTextScanType stype;

	if (newType == XawselectWord)
	  stype = XawstWhiteSpace;
	else
	  stype = XawstParagraph;

	/*
	 * Somewhat complicated, but basically I treat the space between
	 * two objects as another object.  The object that I am currently
	 * in then becomes the end of the selection.
	 *
	 * Chris Peterson - 4/19/90.
	 */
	newRight = SrcScan(ctx->text.source, pos, stype,
			   XawsdRight, 1, False);
	newRight = SrcScan(ctx->text.source, newRight, stype,
			   XawsdLeft, 1, False);

	if (pos != newRight)
	  newLeft = SrcScan(ctx->text.source, pos, stype, XawsdLeft, 1, False);
	else
	  newLeft = pos;

	newLeft =SrcScan(ctx->text.source, newLeft, stype, XawsdRight,1,False);

	if (newLeft > newRight)
	  {
	    XawTextPosition temp = newLeft;
	    newLeft = newRight;
	    newRight = temp;
	  }
      }
      break;
    case XawselectLine:
      newLeft = SrcScan(src, pos, XawstEOL, XawsdLeft, 1, False);
      newRight = SrcScan(src, pos, XawstEOL, XawsdRight, 1, False);
      break;
    case XawselectAll:
      newLeft = SrcScan(src, pos, XawstAll, XawsdLeft, 1, False);
      newRight = SrcScan(src, pos, XawstAll, XawsdRight, 1, False);
      break;
    default:
      XtAppWarning(XtWidgetToApplicationContext((Widget) ctx),
		   "Text Widget: empty selection array.");
      return;
    }

  if (newLeft != ctx->text.s.left || newRight != ctx->text.s.right
      || newType != ctx->text.s.type)
    {
      ModifySelection(ctx, newLeft, newRight);
      if (pos - ctx->text.s.left < ctx->text.s.right - pos)
	ctx->text.insertPos = newLeft;
      else
	ctx->text.insertPos = newRight;
      ctx->text.s.type = newType;
    }
  if (!motion)	/* setup so we can freely mix select extend calls*/
    {
      ctx->text.origSel.type = ctx->text.s.type;
      ctx->text.origSel.left = ctx->text.s.left;
      ctx->text.origSel.right = ctx->text.s.right;

      if (pos >= ctx->text.s.left + (ctx->text.s.right - ctx->text.s.left) / 2)
	ctx->text.extendDir = XawsdRight;
      else
	ctx->text.extendDir = XawsdLeft;
    }
}

/*
 * This routine implements extension of the currently selected text in
 * the "current" mode (i.e. char word, line, etc.). It worries about
 * extending from either end of the selection and handles the case when you
 * cross through the "center" of the current selection (e.g. switch which
 * end you are extending!).
 */
static void
ExtendSelection(TextWidget ctx, XawTextPosition pos, Bool motion)
{
  XawTextScanDirection dir;

  if (!motion)		/* setup for extending selection */
    {
      if (ctx->text.s.left == ctx->text.s.right) /* no current selection. */
	ctx->text.s.left = ctx->text.s.right = ctx->text.insertPos;
      else
	{
	  ctx->text.origSel.left = ctx->text.s.left;
	  ctx->text.origSel.right = ctx->text.s.right;
	}

      ctx->text.origSel.type = ctx->text.s.type;

      if (pos >= ctx->text.s.left + (ctx->text.s.right - ctx->text.s.left) / 2)
	ctx->text.extendDir = XawsdRight;
      else
	ctx->text.extendDir = XawsdLeft;
    }
  else	/* check for change in extend direction */
    if ((ctx->text.extendDir == XawsdRight && pos <= ctx->text.origSel.left)
	||
	(ctx->text.extendDir == XawsdLeft && pos >= ctx->text.origSel.right))
      {
	ctx->text.extendDir = (ctx->text.extendDir == XawsdRight) ?
	  XawsdLeft : XawsdRight;
	ModifySelection(ctx, ctx->text.origSel.left, ctx->text.origSel.right);
      }

  dir = ctx->text.extendDir;
  switch (ctx->text.s.type)
    {
    case XawselectWord:
    case XawselectParagraph:
      {
	XawTextPosition left_pos, right_pos;
	XawTextScanType stype;

	if (ctx->text.s.type == XawselectWord)
	  stype = XawstWhiteSpace;
	else
	  stype = XawstParagraph;

	/*
	 * Somewhat complicated, but basically I treat the space between
	 * two objects as another object.  The object that I am currently
	 * in then becomes the end of the selection.
	 *
	 * Chris Peterson - 4/19/90.
	 */
	right_pos = SrcScan(ctx->text.source, pos, stype,
			    XawsdRight, 1, False);
	right_pos =SrcScan(ctx->text.source, right_pos, stype,
			   XawsdLeft, 1, False);

	if (pos != right_pos)
	  left_pos = SrcScan(ctx->text.source, pos, stype,
			     XawsdLeft, 1, False);
	else
	  left_pos = pos;

	left_pos =SrcScan(ctx->text.source, left_pos, stype,
			  XawsdRight, 1, False);

	if (dir == XawsdLeft)
	  pos = Min(left_pos, right_pos);
	else /* dir == XawsdRight */
	  pos = Max(left_pos, right_pos);
      }
      break;
    case XawselectLine:
      pos = SrcScan(ctx->text.source, pos, XawstEOL,
		    dir, 1, dir == XawsdRight);
      break;
    case XawselectAll:
      pos = ctx->text.insertPos;
      /*FALLTHROUGH*/
    case XawselectPosition:
    default:
      break;
    }

  if (dir == XawsdRight)
    ModifySelection(ctx, ctx->text.s.left, pos);
  else
    ModifySelection(ctx, pos, ctx->text.s.right);

  ctx->text.insertPos = pos;
}

/*
 * Function:
 *	_XawTextClearAndCenterDisplay
 *
 * Parameters:
 *	ctx - text widget
 *
 * Description:
 *	  Redraws the display with the cursor in insert point
 *		     centered vertically.
 */
void
_XawTextClearAndCenterDisplay(TextWidget ctx)
{
  int left_margin = ctx->text.margin.left;
  Bool visible = IsPositionVisible(ctx, ctx->text.insertPos);

  _XawTextShowPosition(ctx);

  if (visible && left_margin == ctx->text.margin.left)
    {
      int insert_line = LineForPosition(ctx, ctx->text.insertPos);
      int scroll_by = insert_line - (ctx->text.lt.lines >> 1);
      Boolean clear_to_eol = ctx->text.clear_to_eol;

      XawTextScroll(ctx, scroll_by, 0);
      SinkClearToBG(ctx->text.sink, 0, 0, XtWidth(ctx), XtHeight(ctx));
      ClearWindow(ctx);
      clear_to_eol = ctx->text.clear_to_eol;
      ctx->text.clear_to_eol = False;
      FlushUpdate(ctx);
      _XawTextSetScrollBars(ctx);
      ctx->text.clear_to_eol = clear_to_eol;
    }
}

/*
 * Internal redisplay entire window
 * Legal to call only if widget is realized
 */
static void
DisplayTextWindow(Widget w)
{
  TextWidget ctx = (TextWidget)w;

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, False);
  ClearWindow(ctx);
  _XawTextSetScrollBars(ctx);
}

static void
TextSinkResize(Widget w)
{
  if (w && XtClass(w)->core_class.resize)
    XtClass(w)->core_class.resize(w);
}

#ifndef notdef
/* ARGSUSED */
void
_XawTextCheckResize(TextWidget ctx)
{
	return;
}
#endif

/*
 * Converts (params, num_params) to a list of atoms & caches the
 * list in the TextWidget instance.
 */
Atom *
_XawTextSelectionList(TextWidget ctx, String *list, Cardinal nelems)
{
  Atom *sel = ctx->text.s.selections;
  Display *dpy = XtDisplay((Widget)ctx);
  int n;

  if (nelems > (Cardinal)ctx->text.s.array_size)
    {
      sel = (Atom *)XtRealloc((char *)sel, sizeof(Atom) * nelems);
      ctx->text.s.array_size = nelems;
      ctx->text.s.selections = sel;
    }
  for (n = nelems; --n >= 0; sel++, list++)
    *sel = XInternAtom(dpy, *list, False);
  ctx->text.s.atom_count = nelems;

  return (ctx->text.s.selections);
}

/*
 * Function:
 *	SetSelection
 *
 * Parameters:
 *	ctx	   - text widget
 *	defaultSel - default selection
 *	l	   - left and right ends of the selection
 *	r	   - ""
 *	list	   - the selection list (as strings).
 *	nelems	   - ""
 *
 * Description:
 *	Sets the current selection.
 *
 * Note:
 *	if (ctx->text.s.left >= ctx->text.s.right) then the selection is unset
 */
void
_XawTextSetSelection(TextWidget ctx, XawTextPosition l, XawTextPosition r,
		     String *list, Cardinal nelems)
{
  if (nelems == 1 && !strcmp (list[0], "none"))
    return;
  if (nelems == 0)
    {
      String defaultSel = "PRIMARY";
      list = &defaultSel;
      nelems = 1;
    }
  _SetSelection(ctx, l, r, _XawTextSelectionList(ctx, list, nelems), nelems);
}

/*
 * Function:
 *	ModifySelection
 *
 * Parameters:
 *	ctx   - text widget
 *	left  - left and right ends of the selection
 *	right - ""
 *
 * Description:
 *	Modifies the current selection.
 *
 * Note:
 *	if (ctx->text.s.left >= ctx->text.s.right) then the selection is unset
 */
static void
ModifySelection(TextWidget ctx, XawTextPosition left, XawTextPosition right)
{
  if (left == right)
    ctx->text.insertPos = left;
  _SetSelection(ctx, left, right, NULL, 0);
}

/*
 * This routine is used to perform various selection functions. The goal is
 * to be able to specify all the more popular forms of draw-through and
 * multi-click selection user interfaces from the outside.
 */
void
_XawTextAlterSelection(TextWidget ctx, XawTextSelectionMode mode,
		       XawTextSelectionAction action, String *params,
		       Cardinal *num_params)
{
  XawTextPosition position;
  Boolean flag;

  /*
   * This flag is used by TextPop.c:DoReplace() to determine if the selection
   * is okay to use, or if it has been modified.
   */
  if (ctx->text.search != NULL)
    ctx->text.search->selection_changed = True;

  position = PositionForXY(ctx, (int) ctx->text.ev_x, (int) ctx->text.ev_y);

  flag = (action != XawactionStart);
  if (mode == XawsmTextSelect)
    DoSelection(ctx, position, ctx->text.time, flag);
  else			/* mode == XawsmTextExtend */
    ExtendSelection (ctx, position, flag);

  if (action == XawactionEnd)
    _XawTextSetSelection(ctx, ctx->text.s.left, ctx->text.s.right,
			 params, *num_params);
}

/*
 * Function:
 *	UpdateTextInRectangle
 *
 * Parameters:
 *	ctx  - the text widget
 *	rect - rectangle
 *
 * Description:
 *	Updates the text in the given rectangle
 */
static void
UpdateTextInRectangle(TextWidget ctx, XRectangle *rect)
{
  XawTextLineTable *lt;
  int line, y1, y2, x2;

  y1 = rect->y;
  y2 = y1 + rect->height;
  x2 = rect->x + rect->width;

  for (line = 0, lt = &ctx->text.lt; line < lt->lines; line++)
    if (lt->info[line + 1].y > y1)
      break;
  for (; line <= lt->lines; line++)
    {
      if (lt->info[line].y > y2)
	break;
      UpdateTextInLine(ctx, line, rect->x, x2);
    }
}

/*
 * This routine processes all "expose region" XEvents. In general, its job
 * is to the best job at minimal re-paint of the text, displayed in the
 * window, that it can.
 */
/* ARGSUSED */
static void
XawTextExpose(Widget w, XEvent *event, Region region)
{
  TextWidget ctx = (TextWidget)w;
  Boolean clear_to_eol;
  XRectangle expose;

      expose.x = event->xexpose.x;
      expose.y = event->xexpose.y;
      expose.width = event->xexpose.width;
      expose.height = event->xexpose.height;

      if (Superclass->core_class.expose)
	(*Superclass->core_class.expose)(w, event, region);

  clear_to_eol = ctx->text.clear_to_eol;
  ctx->text.clear_to_eol = False;

  UpdateTextInRectangle(ctx, &expose);
  FlushUpdate(ctx);
  if (ctx->text.display_caret)
    InsertCursor((Widget)ctx, XawisOn);

  ctx->text.clear_to_eol = clear_to_eol;
}

/*
 * This routine does all setup required to syncronize batched screen updates
 */
void
_XawTextPrepareToUpdate(TextWidget ctx)
{
  if (ctx->text.old_insert < 0)
    {
      InsertCursor((Widget)ctx, XawisOff);
      ctx->text.showposition = False;
      ctx->text.old_insert = ctx->text.insertPos;
    }
}

/*
 * This is a private utility routine used by _XawTextExecuteUpdate. It
 * processes all the outstanding update requests and merges update
 * ranges where possible.
 */
static void
FlushUpdate(TextWidget ctx)
{
  XmuSegment *seg;

  if (XtIsRealized((Widget)ctx))
    {
      for (seg = ctx->text.update->segment; seg; seg = seg->next)
	DisplayText((Widget)ctx,
		    (XawTextPosition)seg->x1,
		    (XawTextPosition)seg->x2);
    }
  (void)XmuScanlineXor(ctx->text.update, ctx->text.update);
}

static int
CountLines(TextWidget ctx, XawTextPosition left, XawTextPosition right)
{
  if (ctx->text.wrap == XawtextWrapNever || left >= right)
    return (1);
  else
    {
      int dim, lines = 0, wwidth = GetMaxTextWidth(ctx);

      while (left < right)
	{
	  XawTextSinkFindPosition(ctx->text.sink, left,
				  ctx->text.margin.left,
				  wwidth, ctx->text.wrap == XawtextWrapWord,
				  &left, &dim, &dim);
	  ++lines;
	}

      return (lines);
    }
  /*NOTREACHED*/
}

static int
GetMaxTextWidth(TextWidget ctx)
{
  XRectangle cursor;
  int width;

  XawTextSinkGetCursorBounds(ctx->text.sink, &cursor);
  width = (int)XtWidth(ctx) - RHMargins(ctx) - cursor.width;

  return (XawMax(0, width));
}

/*
 * Function:
 *	_XawTextShowPosition
 *
 * Parameters:
 *	ctx - the text widget to show the position
 *
 * Description:
 *	  Makes sure the text cursor visible, scrolling the text window
 *	if required.
 */
void
_XawTextShowPosition(TextWidget ctx)
{
  /*
   * Variable scroll is used to avoid scanning large files to calculate
   * line offsets
   */
  int hpixels, vlines;
  XawTextPosition first, last, top;
  Bool visible, scroll;

  if (!XtIsRealized((Widget)ctx))
    return;

  /*
   * Checks if a horizontal scroll is required
   */
  if (ctx->text.wrap == XawtextWrapNever)
    {
      int x, vwidth, distance, dim;
      XRectangle rect;

      vwidth = (int)XtWidth(ctx) - RHMargins(ctx);
      last = SrcScan(ctx->text.source, ctx->text.insertPos,
		     XawstEOL, XawsdLeft, 1, False);
      XawTextSinkFindDistance(ctx->text.sink, last,
			      ctx->text.margin.left,
			      ctx->text.insertPos,
			      &distance, &first, &dim);
      XawTextSinkGetCursorBounds(ctx->text.sink, &rect);
      x = ctx->text.margin.left - ctx->text.r_margin.left;

      if (x + distance + rect.width > vwidth)
	hpixels = x + distance + rect.width - vwidth + (vwidth >> 2);
      else if (x + distance < 0)
	hpixels = x + distance - (vwidth >> 2);
      else
	hpixels = 0;
    }
  else
    hpixels = 0;

  visible = IsPositionVisible(ctx, ctx->text.insertPos);

  /*
   * If the cursor is already visible
   */
  if (!hpixels && visible)
    return;

  scroll = ctx->core.background_pixmap == XtUnspecifiedPixmap && !hpixels;
  vlines = 0;
  first = ctx->text.lt.top;

  /*
   * Needs to scroll the text window
   */
  if (visible)
    top = ctx->text.lt.top;
  else
    {
  top = SrcScan(ctx->text.source, ctx->text.insertPos,
		XawstEOL, XawsdLeft, 1, False);

      /*
       * Finds the nearest left position from ctx->text.insertPos
       */
  if (ctx->text.wrap != XawtextWrapNever)
    {
	  int dim, vwidth = GetMaxTextWidth(ctx);

      last = top;
      /*CONSTCOND*/
      while (1)
	{
	  XawTextSinkFindPosition(ctx->text.sink, last,
				  ctx->text.margin.left, vwidth,
				  ctx->text.wrap == XawtextWrapWord,
				  &last, &dim, &dim);
	      if (last <= ctx->text.insertPos)
	    top = last;
	  else
	    break;
	}
    }
    }

  if (scroll)
    {
  if (ctx->text.insertPos < first)	/* Scroll Down */
    {
      while (first > top)
	{
	  last = first;
	  first = SrcScan(ctx->text.source, first,
			      XawstEOL, XawsdLeft, 2, False);
	  vlines -= CountLines(ctx, first, last);
	      if (-vlines >= ctx->text.lt.lines)
		{
		  scroll = False;
		  break;
		}
	}
    }
  else if (!visible)			/* Scroll Up */
    {
      while (first < top)
	{
	  last = first;
	  first = SrcScan(ctx->text.source, first,
			  XawstEOL, XawsdRight, 1, True);
	  vlines += CountLines(ctx, last, first);
	      if (vlines > ctx->text.lt.lines)
		{
		  scroll = False;
		  break;
		}
	    }
	}
      else
	scroll = False;
    }

  /*
   * If a portion of the text that will be scrolled is visible
   */
  if (scroll)
    XawTextScroll(ctx, vlines ? vlines - (ctx->text.lt.lines >> 1) : 0, 0);
  /*
   * Else redraw the entire text window
   */
  else
    {
      ctx->text.margin.left -= hpixels;
      if (ctx->text.margin.left > ctx->text.r_margin.left)
	ctx->text.margin.left = ctx->text.r_margin.left;

      if (!visible)
	{
	  vlines = ctx->text.lt.lines >> 1;
	  if (vlines)
	    top = SrcScan(ctx->text.source, ctx->text.insertPos,
			  XawstEOL, XawsdLeft, vlines + 1, False);

	  if (ctx->text.wrap != XawtextWrapNever)
	    {
	      int dim;
	      int n_lines = CountLines(ctx, top, ctx->text.insertPos);
	      int vwidth = GetMaxTextWidth(ctx);

	      while (n_lines-- > vlines)
		{
		  XawTextSinkFindPosition(ctx->text.sink, top,
					  ctx->text.margin.left,
					  vwidth,
					  ctx->text.wrap == XawtextWrapWord,
					  &top, &dim, &dim);
		}
	    }
	  _XawTextBuildLineTable(ctx, top, True);
	}
      else
	ClearWindow(ctx);
      _XawTextSetScrollBars(ctx);
    }
  ctx->text.clear_to_eol = True;
}

/*
 * This routine causes all batched screen updates to be performed
 */
void
_XawTextExecuteUpdate(TextWidget ctx)
{
  if (ctx->text.update_disabled || ctx->text.old_insert < 0)
    return;

  if(ctx->text.old_insert != ctx->text.insertPos || ctx->text.showposition)
    _XawTextShowPosition(ctx);

  FlushUpdate(ctx);
  InsertCursor((Widget)ctx, XawisOn);
  ctx->text.old_insert = -1;
  ctx->text.clear_to_eol = True;
}

static void
XawTextDestroy(Widget w)
{
  TextWidget ctx = (TextWidget)w;

  DestroyHScrollBar(ctx);
  DestroyVScrollBar(ctx);

  XtFree((char *)ctx->text.s.selections);
  XtFree((char *)ctx->text.lt.info);
  XtFree((char *)ctx->text.search);
  XmuDestroyScanline(ctx->text.update);
}

/*
 * by the time we are managed (and get this far) we had better
 * have both a source and a sink 
 */
static void
XawTextResize(Widget w)
{
  TextWidget ctx = (TextWidget)w;

  PositionVScrollBar(ctx);
  PositionHScrollBar(ctx);
  TextSinkResize(ctx->text.sink);

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
  _XawTextSetScrollBars(ctx);
}

/*
 * This routine allow the application program to Set attributes.
 */
/*ARGSUSED*/
static Boolean
XawTextSetValues(Widget current, Widget request, Widget cnew,
		 ArgList args, Cardinal *num_args)
{
  TextWidget oldtw = (TextWidget)current;
  TextWidget newtw = (TextWidget)cnew;
  Boolean redisplay = False;
  Boolean display_caret = newtw->text.display_caret;

  newtw->text.display_caret = oldtw->text.display_caret;
  _XawTextPrepareToUpdate(newtw);
  newtw->text.display_caret = display_caret;

  if (oldtw->text.r_margin.left != newtw->text.r_margin.left)
    {
      newtw->text.margin.left = newtw->text.r_margin.left;
      if (newtw->text.vbar != NULL)
	{
	  newtw->text.margin.left += XtWidth(newtw->text.vbar)
	    + XtBorderWidth(newtw->text.vbar);
	  if (newtw->text.adjust_scrollbars)
	    newtw->text.margin.left -= XtBorderWidth(newtw->text.vbar);
	}
      redisplay = True;
    }

  if (oldtw->text.scroll_vert != newtw->text.scroll_vert)
    {
      if (newtw->text.scroll_vert)
	CreateVScrollBar(newtw);
      else
	DestroyVScrollBar(newtw);

      redisplay = True;
    }

  if (oldtw->text.r_margin.bottom != newtw->text.r_margin.bottom)
    {
      newtw->text.margin.bottom = newtw->text.r_margin.bottom;
      if (newtw->text.hbar != NULL)
	newtw->text.margin.bottom += newtw->text.hbar->core.height
	  + newtw->text.hbar->core.border_width;
      redisplay = True;
    }

  if (oldtw->text.scroll_horiz != newtw->text.scroll_horiz)
    {
      if (newtw->text.scroll_horiz && !newtw->text.wrap)
	CreateHScrollBar(newtw);
      else
	DestroyHScrollBar(newtw);

      redisplay = True;
    }

  if (oldtw->text.source != newtw->text.source)
    XawTextSetSource((Widget)newtw, newtw->text.source, newtw->text.lt.top);

  newtw->text.redisplay_needed = False;
  XtSetValues((Widget)newtw->text.source, args, *num_args);
  XtSetValues((Widget)newtw->text.sink, args, *num_args);

  if (oldtw->text.wrap != newtw->text.wrap
      || oldtw->text.lt.top != newtw->text.lt.top
      || oldtw->text.r_margin.right != newtw->text.r_margin.right
      || oldtw->text.r_margin.top != newtw->text.r_margin.top
      || oldtw->text.sink != newtw->text.sink
      || newtw->text.redisplay_needed)
    {
      _XawTextBuildLineTable(newtw, newtw->text.lt.top, True);
      redisplay = True;
    }

  if (oldtw->text.insertPos != newtw->text.insertPos)
    {
      newtw->text.showposition = True;
      redisplay = True;
    }

  _XawTextExecuteUpdate(newtw);
  if (redisplay)
    _XawTextSetScrollBars(newtw);

  return (redisplay);
}

/* invoked by the Simple widget's SetValues */
static Bool
XawTextChangeSensitive(Widget w)
{
  Arg args[1];
  TextWidget tw = (TextWidget)w;

  (*(&simpleClassRec)->simple_class.change_sensitive)(w);

  XtSetArg(args[0], XtNancestorSensitive,
	   (tw->core.ancestor_sensitive && tw->core.sensitive));
  if (tw->text.vbar)
    XtSetValues(tw->text.vbar, args, ONE);
  if (tw->text.hbar)
    XtSetValues(tw->text.hbar, args, ONE);
  return (False);
}

/*
 * Function:
 *	XawTextGetValuesHook
 *
 * Parameters:
 *	w	 - Text Widget
 *	args	 - argument list
 *	num_args - number of args
 *
 * Description:
 *	  This is a get values hook routine that gets the
 *		     values in the text source and sink.
 */
static void
XawTextGetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
  XtGetValues(((TextWidget)w)->text.source, args, *num_args);
  XtGetValues(((TextWidget)w)->text.sink, args, *num_args);
}

/*
 * Function:
 *	FindGoodPosition
 *
 * Parameters:
 *	pos - any position
 *
 * Description:
 *	Returns a valid position given any postition.
 *
 * Returns:
 *	A position between (0 and lastPos)
 */
static XawTextPosition
FindGoodPosition(TextWidget ctx, XawTextPosition pos)
{
  if (pos < 0)
    return (0);
  return (((pos > ctx->text.lastPos) ? ctx->text.lastPos : pos));
}

/* Li wrote this so the IM can find a given text position's screen position */
void
_XawTextPosToXY(Widget w, XawTextPosition pos, Position *x, Position *y)
{
  int line, ix, iy;

  LineAndXYForPosition((TextWidget)w, pos, &line, &ix, &iy);
  *x = ix;
  *y = iy;
}

/*******************************************************************
The following routines provide procedural interfaces to Text window state
setting and getting. They need to be redone so than the args code can use
them. I suggest we create a complete set that takes the context as an
argument and then have the public version lookup the context and call the
internal one. The major value of this set is that they have actual application
clients and therefore the functionality provided is required for any future
version of Text.
********************************************************************/
void
XawTextDisplay(Widget w)
{
  if (!XtIsRealized(w))
    return;

  _XawTextPrepareToUpdate((TextWidget)w);
  DisplayTextWindow(w);
  _XawTextExecuteUpdate((TextWidget)w);
}

void
XawTextSetSelectionArray(Widget w, XawTextSelectType *sarray)
{
  ((TextWidget)w)->text.sarray = sarray;
}

void
XawTextGetSelectionPos(Widget w, XawTextPosition *left, XawTextPosition *right)
{
  *left = ((TextWidget)w)->text.s.left;
  *right = ((TextWidget)w)->text.s.right;
}

void 
XawTextSetSource(Widget w, Widget source, XawTextPosition startPos)
{
  TextWidget ctx = (TextWidget)w;

  ctx->text.source = source;
  ctx->text.lt.top = startPos;
  ctx->text.s.left = ctx->text.s.right = 0;
  ctx->text.insertPos = startPos;
  ctx->text.lastPos = GETLASTPOS;

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
  XawTextDisplay(w);
}

/*
 * This public routine deletes the text from startPos to endPos in a source and
 * then inserts, at startPos, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any), so that things
 * will be repainted properly.
 */
int
XawTextReplace(Widget w, XawTextPosition startPos, XawTextPosition endPos,
               XawTextBlock *text)
{
  TextWidget ctx = (TextWidget)w;
  int result;

  _XawTextPrepareToUpdate(ctx);
  endPos = FindGoodPosition(ctx, endPos);
  startPos = FindGoodPosition(ctx, startPos);
  if ((result = _XawTextReplace(ctx, startPos, endPos, text)) == XawEditDone)
    {
      int delta = text->length - (endPos - startPos);
      if (ctx->text.insertPos >= endPos + delta)
	{
	  XawTextScanDirection sd = (delta < 0) ? XawsdLeft : XawsdRight;
	  ctx->text.insertPos = SrcScan(ctx->text.source, ctx->text.insertPos,
					XawstPositions, sd, abs(delta), True);
	}
    }

  _XawTextExecuteUpdate(ctx);
  _XawTextSetScrollBars(ctx);

  return (result);
}

XawTextPosition 
XawTextTopPosition(Widget w)
{
  return (((TextWidget)w)->text.lt.top);
}

void
XawTextSetInsertionPoint(Widget w, XawTextPosition position)
{
  TextWidget ctx = (TextWidget)w;

  _XawTextPrepareToUpdate(ctx);
  ctx->text.insertPos = FindGoodPosition(ctx, position);
  ctx->text.showposition = True;

  _XawTextExecuteUpdate(ctx);
}

XawTextPosition
XawTextGetInsertionPoint(Widget w)
{
  return (((TextWidget)w)->text.insertPos);
}

/*
 * Note: Must walk the selection list in opposite order from TextLoseSelection
 */
void
XawTextUnsetSelection(Widget w)
{
  TextWidget ctx = (TextWidget)w;

  while (ctx->text.s.atom_count != 0)
    {
      Atom sel = ctx->text.s.selections[ctx->text.s.atom_count - 1];

      if (sel != (Atom) 0)
	{
	  /*
	   * As selections are lost the atom_count will decrement
	   */
	  if (GetCutBufferNumber(sel) == NOT_A_CUT_BUFFER)
	    XtDisownSelection(w, sel, ctx->text.time);
	  TextLoseSelection(w, &sel); /* In case this is a cut buffer, or
					 XtDisownSelection failed to call us */
	}
    }
}

void
XawTextSetSelection(Widget w, XawTextPosition left, XawTextPosition right)
{
  TextWidget ctx = (TextWidget)w;

  _XawTextPrepareToUpdate(ctx);
  _XawTextSetSelection(ctx, FindGoodPosition(ctx, left),
		       FindGoodPosition(ctx, right), NULL, 0);
  _XawTextExecuteUpdate(ctx);
}

void
XawTextInvalidate(Widget w, XawTextPosition from, XawTextPosition to)
{
  TextWidget ctx = (TextWidget)w;

  from = FindGoodPosition(ctx, from);
  to = FindGoodPosition(ctx, to);
  ctx->text.lastPos = GETLASTPOS;
  _XawTextPrepareToUpdate(ctx);
  _XawTextNeedsUpdating(ctx, from, to);
  _XawTextExecuteUpdate(ctx);
}

/*ARGSUSED*/
void
XawTextDisableRedisplay(Widget w)
{
  ((TextWidget)w)->text.update_disabled = True;
  _XawTextPrepareToUpdate((TextWidget)w);
}

void 
XawTextEnableRedisplay(Widget w)
{
  TextWidget ctx = (TextWidget)w;
  XawTextPosition lastPos;

  if (!ctx->text.update_disabled)
    return;

  ctx->text.update_disabled = False;
  lastPos = ctx->text.lastPos = GETLASTPOS;
  ctx->text.lt.top = FindGoodPosition(ctx, ctx->text.lt.top);
  ctx->text.insertPos = FindGoodPosition(ctx, ctx->text.insertPos);

  if (ctx->text.s.left > lastPos || ctx->text.s.right > lastPos)
    ctx->text.s.left = ctx->text.s.right = 0;

  if (XtIsRealized(w))
    DisplayTextWindow(w);
  _XawTextExecuteUpdate(ctx);
}

Widget
XawTextGetSource(Widget w)
{
  return (((TextWidget)w)->text.source);
}

void
XawTextDisplayCaret(Widget w, Bool display_caret)
{
  TextWidget ctx = (TextWidget)w;

  if (XtIsRealized(w))
    {
      _XawTextPrepareToUpdate(ctx);
      ctx->text.display_caret = display_caret;
      _XawTextExecuteUpdate(ctx);
    }
  else
    ctx->text.display_caret = display_caret;
}

/*
 * Function:
 *	XawTextSearch
 *
 * Parameters:
 *	w    - text widget
 *	dir  - direction to search
 *	text - text block containing info about the string to search for
 *
 * Description:
 *	Searches for the given text block.
 *
 * Returns:
 *	The position of the text found, or XawTextSearchError on an error
 */
XawTextPosition
XawTextSearch(Widget w, XawTextScanDirection dir, XawTextBlock *text)
{
  TextWidget ctx = (TextWidget)w;

  return (SrcSearch(ctx->text.source, ctx->text.insertPos, dir, text));
}

TextClassRec textClassRec = {
  /* core */
  {
    (WidgetClass)&simpleClassRec,	/* superclass */ 
    "Text",				/* class_name */
    sizeof(TextRec),			/* widget_size */
    XawTextClassInitialize,		/* class_initialize */
    NULL,				/* class_part_init */
    False,				/* class_inited */
    XawTextInitialize,			/* initialize */
    NULL,				/* initialize_hook */
    XawTextRealize,			/* realize */
    _XawTextActionsTable,		/* actions */
    0,					/* num_actions */
    resources,				/* resources */
    XtNumber(resources),		/* num_resource */
    NULLQUARK,				/* xrm_class */
    True,				/* compress_motion */
    True,				/* compress_exposure */
    True,				/* compress_enterleave */
    False,				/* visible_interest */
    XawTextDestroy,			/* destroy */
    XawTextResize,			/* resize */
    XawTextExpose,			/* expose */
    XawTextSetValues,			/* set_values */
    NULL,				/* set_values_hook */
    XtInheritSetValuesAlmost,		/* set_values_almost */
    XawTextGetValuesHook,		/* get_values_hook */
    NULL,				/* accept_focus */
    XtVersion,				/* version */
    NULL,				/* callback_private */
    _XawDefaultTextTranslations,	/* tm_table */
    XtInheritQueryGeometry,		/* query_geometry */
    XtInheritDisplayAccelerator,	/* display_accelerator */
    NULL,				/* extension */
  },
  /* simple */
  {
    XawTextChangeSensitive,		/* change_sensitive */
  },
  /* text */
  {
    NULL,				/* extension */
  }
};

WidgetClass textWidgetClass = (WidgetClass)&textClassRec;
