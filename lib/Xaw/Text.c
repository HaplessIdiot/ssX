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
/* $XFree86: xc/lib/Xaw/Text.c,v 3.9 1998/06/28 13:04:21 dawes Exp $ */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "XawI18n.h"
#include <stdio.h>

#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/StdSel.h>
#include <X11/Xmu/Misc.h>
#include <X11/Xmu/SysUtil.h>

#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/TextP.h>
#include <X11/Xaw/MultiSinkP.h>
#include <X11/Xaw/XawImP.h>
#include <X11/Xaw/TextSrcP.h>

#include <X11/Xfuncs.h>
#include <ctype.h>		/* for isprint() */

#include "Private.h"

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

#define BIGNUM			((Dimension)32023)
#define MULTI_CLICK_TIME	500L

#define RVMargins(w) ((w)->text.r_margin.top  + (w)->text.r_margin.bottom)
#define RHMargins(w) ((w)->text.r_margin.left + (w)->text.r_margin.right)

#define Superclass (&simpleClassRec)

/*
 * Compute a the maximum length of a cut buffer that we can pass at any
 * time.  The 64 allows for the overhead of the Change Property request.
 */
#define MAX_CUT_LEN(dpy)  (XMaxRequestSize(dpy) - 64)

#define IsValidLine(ctx, num) (((num) == 0) || \
			       ((ctx)->text.lt.info[(num)].position != 0))

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
static Boolean XawTextChangeSensitive(Widget);

/*
 * Prototypes
 */
static XawTextPosition _BuildLineTable(TextWidget, XawTextPosition,
				       XawTextPosition, int);
static void _CreateCutBuffers(Display*);
static void CheckVBarScrolling(TextWidget);
static void ClearWindow(Widget);
static Boolean ConvertSelection(Widget, Atom*, Atom*, Atom*, XtPointer*,
				unsigned long*, int*);
static int CountLines(TextWidget, XawTextPosition, XawTextPosition);
static void CreateHScrollBar(TextWidget);
static void CreateVScrollBar(TextWidget);
static void CvtStringToScrollMode(XrmValuePtr, Cardinal*,
				  XrmValuePtr, XrmValuePtr);
static void CvtStringToWrapMode(XrmValuePtr, Cardinal*,
				XrmValuePtr, XrmValuePtr);
static void CvtStringToResizeMode(XrmValuePtr, Cardinal*,
				  XrmValuePtr, XrmValuePtr);
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
static unsigned int GetWidestLine(TextWidget);
static void HScroll(Widget, XtPointer, XtPointer);
static void HJump(Widget, XtPointer, XtPointer);
static void InsertCursor(Widget, XawTextInsertState);
static Bool LineAndXYForPosition(TextWidget, XawTextPosition, int*,
				 int*, int*);
static int LineForPosition(TextWidget, XawTextPosition);
static void LoseSelection(Widget, Atom*);
static Bool MatchSelection(Atom, XawTextSelection*);
static void ModifySelection(TextWidget, XawTextPosition, XawTextPosition);
static XawTextPosition PositionForXY(TextWidget, int, int);
static void PositionHScrollBar(TextWidget);
static void PositionVScrollBar(TextWidget);
static void _SetSelection(TextWidget, XawTextPosition, XawTextPosition,
			  Atom*, Cardinal);
static void TextSinkResize(Widget);
static void UnrealizeScrollbars(Widget, XtPointer, XtPointer);
static void UpdateTextInRectangle(TextWidget, XRectangle*);
static void UpdateTextInLine(TextWidget, int, int, int);
static void VScroll(Widget, XtPointer, XtPointer);
static void VJump(Widget, XtPointer, XtPointer);
static void XawTextScroll(TextWidget, int, int);

/*
 * External
 */
void _XawTextAlterSelection(TextWidget,
			    XawTextSelectionMode, XawTextSelectionAction,
			    String*, Cardinal*);
void _XawTextBuildLineTable(TextWidget, XawTextPosition, _XtBoolean);
void _XawTextCheckResize(TextWidget);
void _XawTextClearAndCenterDisplay(TextWidget);
void _XawTextExecuteUpdate(TextWidget);
char *_XawTextGetSTRING(TextWidget, XawTextPosition, XawTextPosition);
char *_XawTextGetText(TextWidget, XawTextPosition, XawTextPosition);
void _XawTextPrepareToUpdate(TextWidget);
void _XawTextPosToXY(Widget, XawTextPosition, Position*, Position*);
int _XawTextReplace(TextWidget, XawTextPosition, XawTextPosition,
		    XawTextBlock*);
void _XawTextSaltAwaySelection(TextWidget, Atom*, int);
Atom *_XawTextSelectionList(TextWidget, String*, Cardinal);
void _XawTextSetScrollBars(TextWidget);
void _XawTextSetSelection(TextWidget, XawTextPosition, XawTextPosition,
			  String*, Cardinal);
void _XawTextVScroll(TextWidget, int);

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
extern char *_XawDefaultTextTranslations1, *_XawDefaultTextTranslations2,
  *_XawDefaultTextTranslations3, *_XawDefaultTextTranslations4;

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
    XtCTextSource, XtRWidget,
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
    XtRScrollMode,
    sizeof(XawTextScrollMode),
    offset(text.scroll_vert),
    XtRImmediate,
    (XtPointer)XawtextScrollNever
  },
  {
    XtNscrollHorizontal,
    XtCScroll,
    XtRScrollMode,
    sizeof(XawTextScrollMode),
    offset(text.scroll_horiz),
    XtRImmediate,
    (XtPointer)XawtextScrollNever
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
    XtNresize,
    XtCResize,
    XtRResizeMode,
    sizeof(XawTextResizeMode),
    offset(text.resize),
    XtRImmediate,
    (XtPointer)XawtextResizeNever
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
    XtNunrealizeCallback,
    XtCCallback,
    XtRCallback,
    sizeof(XtPointer),
    offset(text.unrealize_callbacks),
    XtRCallback,
    (XtPointer)NULL
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

static XrmQuark QScrollNever, QScrollAlways, QScrollWhenNeeded,
	  QWrapNever, QWrapLine, QWrapWord,
	  QResizeNever, QResizeWidth, QResizeHeight, QResizeBoth;

/* ARGSUSED */
static void
CvtStringToScrollMode(XrmValuePtr args, Cardinal *num_args,
		      XrmValuePtr fromVal, XrmValuePtr toVal)
{
  static XawTextScrollMode scrollMode = XawtextScrollNever;
  XrmQuark q;
  char lowerName[32];

  XmuNCopyISOLatin1Lowered (lowerName, (char *)fromVal->addr,
			    sizeof(lowerName));
  q = XrmStringToQuark(lowerName);

  if (q == QScrollNever)
    scrollMode = XawtextScrollNever;
  else if (q == QScrollWhenNeeded)
    scrollMode = XawtextScrollWhenNeeded;
  else if (q == QScrollAlways)
    scrollMode = XawtextScrollAlways;
  else
    XtStringConversionWarning((char *)fromVal->addr, XtRScrollMode);

  done(&scrollMode, XawTextScrollMode);
}

/* ARGSUSED */
static void
CvtStringToWrapMode(XrmValuePtr args, Cardinal *num_args,
		    XrmValuePtr fromVal, XrmValuePtr toVal)
{
  static XawTextWrapMode wrapMode = XawtextWrapNever;
  XrmQuark q;
  char lowerName[32];

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

/* ARGSUSED */
static void
CvtStringToResizeMode(XrmValuePtr args, Cardinal *num_args,
		      XrmValuePtr fromVal, XrmValuePtr toVal)
{
  static XawTextResizeMode resizeMode = XawtextResizeNever;
  XrmQuark q;
  char lowerName[32];

  XmuNCopyISOLatin1Lowered(lowerName, (char *)fromVal->addr,
			   sizeof(lowerName));
  q = XrmStringToQuark(lowerName);

  if (q == QResizeNever)
    resizeMode = XawtextResizeNever;
  else if (q == QResizeWidth)
    resizeMode = XawtextResizeWidth;
  else if (q == QResizeHeight)
    resizeMode = XawtextResizeHeight;
  else if (q == QResizeBoth)
    resizeMode = XawtextResizeBoth;
  else
    XtStringConversionWarning((char *)fromVal->addr, XtRResizeMode);

  done(&resizeMode, XawTextResizeMode);
}

#undef done

static void
XawTextClassInitialize(void)
{
  int len1 = strlen (_XawDefaultTextTranslations1);
  int len2 = strlen (_XawDefaultTextTranslations2);
  int len3 = strlen (_XawDefaultTextTranslations3);
  int len4 = strlen (_XawDefaultTextTranslations4);
  char *buf = XtMalloc ((unsigned)(len1 + len2 + len3 + len4 + 1));
  char *cp = buf;

  if (!XawFmt8Bit)
    FMT8BIT = XawFmt8Bit = XrmPermStringToQuark("FMT8BIT");
  if (!XawFmtWide)
    XawFmtWide = XrmPermStringToQuark("FMTWIDE");

  XawInitializeWidgetSet();

  /*
   * Set the number of actions.
   */
  textClassRec.core_class.num_actions = _XawTextActionsTableCount;
  
  (void)strcpy(cp, _XawDefaultTextTranslations1); cp += len1;
  (void)strcpy(cp, _XawDefaultTextTranslations2); cp += len2;
  (void)strcpy(cp, _XawDefaultTextTranslations3); cp += len3;
  (void)strcpy(cp, _XawDefaultTextTranslations4);
  textWidgetClass->core_class.tm_table = buf;

  QScrollNever      = XrmPermStringToQuark(XtEtextScrollNever);
  QScrollWhenNeeded = XrmPermStringToQuark(XtEtextScrollWhenNeeded);
  QScrollAlways     = XrmPermStringToQuark(XtEtextScrollAlways);
  QWrapNever        = XrmPermStringToQuark(XtEtextWrapNever);
  QWrapLine         = XrmPermStringToQuark(XtEtextWrapLine);
  QWrapWord         = XrmPermStringToQuark(XtEtextWrapWord);
  QResizeNever      = XrmPermStringToQuark(XtEtextResizeNever);
  QResizeWidth      = XrmPermStringToQuark(XtEtextResizeWidth);
  QResizeHeight     = XrmPermStringToQuark(XtEtextResizeHeight);
  QResizeBoth       = XrmPermStringToQuark(XtEtextResizeBoth);
  XtAddConverter(XtRString, XtRScrollMode, CvtStringToScrollMode,
		 (XtConvertArgList)NULL, (Cardinal)0);
  XtAddConverter(XtRString, XtRWrapMode, CvtStringToWrapMode,
		 (XtConvertArgList)NULL, (Cardinal)0);
  XtAddConverter(XtRString, XtRResizeMode, CvtStringToResizeMode,
		 (XtConvertArgList)NULL, (Cardinal)0);
}

/*	Function Name: PositionHScrollBar.
 *	Description: Positions the Horizontal scrollbar.
 *	Arguments: ctx - the text widget.
 *	Returns: none
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

/*	Function Name: PositionVScrollBar.
 *	Description: Positions the Vertical scrollbar.
 *	Arguments: ctx - the text widget.
 *	Returns: none.
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
    XtCreateWidget("vScrollbar", scrollbarWidgetClass, (Widget)ctx,
		   (ArgList)NULL, ZERO);
  XtAddCallback(vbar, XtNscrollProc, VScroll, (XtPointer)ctx);
  XtAddCallback(vbar, XtNjumpProc, VJump, (XtPointer)ctx);

  if (ctx->text.hbar == NULL)
    XtAddCallback((Widget)ctx, XtNunrealizeCallback, UnrealizeScrollbars,
		  (XtPointer)NULL);

  if (XtIsRealized((Widget)ctx))
    SinkClearToBG(ctx->text.sink, 0, 0,
		  (ctx->text.r_margin.left + XtWidth(vbar)
		   + XtBorderWidth(vbar) + (ctx->text.adjust_scrollbars != 0)),
		  XtHeight(ctx));

  ctx->text.r_margin.left += XtWidth(vbar) + XtBorderWidth(vbar);
  if (ctx->text.adjust_scrollbars)
    ctx->text.r_margin.left += XtBorderWidth(vbar);
  ctx->text.margin.left = ctx->text.r_margin.left;

  PositionVScrollBar(ctx);
  PositionHScrollBar(ctx);	/* May modify location of Horiz. Bar. */
  TextSinkResize(ctx->text.sink);

  if (XtIsRealized((Widget)ctx))
    {
      XtRealizeWidget(vbar);
      XtMapWidget(vbar);
    }
}

/*	Function Name: DestroyVScrollBar
 *	Description: Removes a vertical ScrollBar.
 *	Arguments: ctx - the parent text widget.
 *	Returns: none.
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
  if (ctx->text.hbar == NULL)
    XtRemoveCallback((Widget)ctx, XtNunrealizeCallback, UnrealizeScrollbars,
		     (XtPointer)NULL);
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

  if (ctx->text.vbar == NULL)
    XtAddCallback((Widget)ctx, XtNunrealizeCallback, UnrealizeScrollbars,
		  (XtPointer)NULL);

  bottom = ctx->text.r_margin.bottom + XtHeight(hbar) + XtBorderWidth(hbar);
  if (ctx->text.adjust_scrollbars)
    bottom += XtBorderWidth(hbar);

  if (XtIsRealized((Widget)ctx) && ctx->text.lastPos > 0)
    {
      XawTextPosition pos;
      Position y;
      int line;

      y = XtHeight(ctx) - bottom;
      pos = PositionForXY(ctx, ctx->text.r_margin.left, y);
      line = LineForPosition(ctx, pos);
      if (y > ctx->text.lt.info[line].y)
	y = ctx->text.lt.info[line].y;
      SinkClearToBG(ctx->text.sink,
		    ctx->text.r_margin.left, y,
		    XtWidth(ctx) - ctx->text.r_margin.right
		    - ctx->text.r_margin.left, bottom);
    }
  ctx->text.margin.bottom = ctx->text.r_margin.bottom = bottom;

  PositionHScrollBar(ctx);
  TextSinkResize(ctx->text.sink);
  if (XtIsRealized((Widget)ctx))
    {
      XtRealizeWidget(hbar);
      XtMapWidget(hbar);
    }
}

/*	Function Name: DestroyHScrollBar
 *	Description: Removes a horizontal ScrollBar.
 *	Arguments: ctx - the parent text widget.
 *	Returns: none.
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

  if (ctx->text.vbar == NULL)
    XtRemoveCallback((Widget)ctx, XtNunrealizeCallback, UnrealizeScrollbars,
		       (XtPointer)NULL);
  XtDestroyWidget(hbar);
  ctx->text.hbar = NULL;
  TextSinkResize(ctx->text.sink);
}

/* ARGSUSED */
static void
XawTextInitialize(Widget request, Widget c_new,
		  ArgList args, Cardinal *num_args)
{
  TextWidget ctx = (TextWidget)c_new;

  ctx->text.lt.lines = 0;
  ctx->text.lt.info = NULL;
  (void) bzero((char *) &(ctx->text.origSel), sizeof(XawTextSelection));
  (void) bzero((char *) &(ctx->text.s), sizeof(XawTextSelection)); 
  ctx->text.s.type = XawselectPosition;
  ctx->text.salt = NULL;
  ctx->text.hbar = ctx->text.vbar = (Widget)NULL;
  ctx->text.lasttime = 0; /* ||| correct? */
  ctx->text.time = 0; /* ||| correct? */
  ctx->text.showposition = TRUE;
  ctx->text.lastPos = (ctx->text.source != NULL) ? GETLASTPOS : 0;
  ctx->text.file_insert = NULL;
  ctx->text.search = NULL;
  ctx->text.updateFrom = (XawTextPosition *) XtMalloc((unsigned)ONE);
  ctx->text.updateTo = (XawTextPosition *) XtMalloc((unsigned)ONE);
  ctx->text.numranges = ctx->text.maxranges = 0;
  ctx->text.gc = DefaultGCOfScreen(XtScreen(ctx));
  ctx->text.hasfocus = FALSE;
  ctx->text.margin = ctx->text.r_margin; /* Strucure copy. */
  ctx->text.update_disabled = FALSE;
  ctx->text.old_insert = -1;
  ctx->text.mult = 1;
  ctx->text.single_char = FALSE;
  ctx->text.copy_area_offsets = NULL;
  ctx->text.salt2 = NULL;
  ctx->text.from_left = -1;

  if (XtHeight(ctx) == DEFAULT_TEXT_HEIGHT)
    {
      XtHeight(ctx) = VMargins(ctx);
      if (ctx->text.sink != NULL)
	XtHeight(ctx) += XawTextSinkMaxHeight(ctx->text.sink, 1);
    }

  if (ctx->text.scroll_vert != XawtextScrollNever)
    {
      if ((ctx->text.resize == XawtextResizeHeight)
	  || (ctx->text.resize == XawtextResizeBoth))
	ctx->text.scroll_vert = XawtextScrollNever;
      else if (ctx->text.scroll_vert == XawtextScrollAlways)
	CreateVScrollBar(ctx);
    }

  if (ctx->text.scroll_horiz != XawtextScrollNever)
    {
      if (ctx->text.wrap != XawtextWrapNever)
	ctx->text.scroll_horiz = XawtextScrollNever;
      else if ((ctx->text.resize == XawtextResizeWidth)
	       ||(ctx->text.resize == XawtextResizeBoth))
	ctx->text.scroll_horiz = XawtextScrollNever;
      else if (ctx->text.scroll_horiz == XawtextScrollAlways)
	CreateHScrollBar(ctx);
    }
}

static void
XawTextRealize(Widget w, XtValueMask *mask, XSetWindowAttributes *attr)
{
  TextWidget ctx = (TextWidget)w;

  (*textClassRec.core_class.superclass->core_class.realize)(w, mask, attr);
  
  if (ctx->text.hbar != NULL)	        /* Put up Hbar -- Must be first. */
    {
      XtRealizeWidget(ctx->text.hbar);
      XtMapWidget(ctx->text.hbar);
    }

  if (ctx->text.vbar != NULL)	        /* Put up Vbar. */
    {
      XtRealizeWidget(ctx->text.vbar);
      XtMapWidget(ctx->text.vbar);
    }

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
  _XawTextSetScrollBars(ctx);
  _XawTextCheckResize(ctx);
}

/*ARGSUSED*/
static void
UnrealizeScrollbars(Widget widget, XtPointer client, XtPointer call)
{
  TextWidget ctx = (TextWidget)widget;
    
  if (ctx->text.hbar)
    XtUnrealizeWidget(ctx->text.hbar);
  if (ctx->text.vbar)
    XtUnrealizeWidget(ctx->text.vbar);
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

  if (LineAndXYForPosition(ctx, ctx->text.insertPos, &line, &x, &y))
    {
      if (line < ctx->text.lt.lines)
	y += (ctx->text.lt.info[line + 1].y - ctx->text.lt.info[line].y) + 1;
      else
	y += (ctx->text.lt.info[line].y - ctx->text.lt.info[line - 1].y) + 1;

      if (ctx->text.display_caret)
	XawTextSinkInsertCursor(ctx->text.sink, x, y, state);
    }
  ctx->text.ev_x = x;
  ctx->text.ev_y = y;

  /* Keep Input Method up to speed  */
  if (ctx->simple.international)
    {
      Arg list[1];

      XtSetArg (list[0], XtNinsertPosition, ctx->text.insertPos);
      _XawImSetValues (w, list, 1);
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
  int i;

  if (left < right)
    {
      for (i = 0; i < ctx->text.numranges; i++)
	{
	  if (left <= ctx->text.updateTo[i]
	      && right >= ctx->text.updateFrom[i])
	    {
	      ctx->text.updateFrom[i] = Min(left, ctx->text.updateFrom[i]);
	      ctx->text.updateTo[i] = Max(right, ctx->text.updateTo[i]);
	      return;
	    }
	}

      ctx->text.numranges++;

      if (ctx->text.numranges > ctx->text.maxranges)
	{
	  ctx->text.maxranges = ctx->text.numranges;
	  i = ctx->text.maxranges * sizeof(XawTextPosition);
	  ctx->text.updateFrom = (XawTextPosition *)
	    XtRealloc((char *)ctx->text.updateFrom, (unsigned)i);
	  ctx->text.updateTo = (XawTextPosition *)
	    XtRealloc((char *)ctx->text.updateTo, (unsigned)i);
	}

      ctx->text.updateFrom[ctx->text.numranges - 1] = left;
      ctx->text.updateTo[ctx->text.numranges - 1] = right;
    }
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
      memmove(tempResult, text.ptr, text.length * bytes);
      tempResult += text.length * bytes;
    }

  if (bytes == sizeof(wchar_t))
    *((wchar_t*)tempResult) = (wchar_t)0;
  else
    *tempResult = '\0';
  return (result);
}

/* Like _XawTextGetText, but enforces ICCCM STRING type encoding.  This
routine is currently used to put just the ASCII chars in the selection into a
cut buffer. */
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
      MultiSinkObject sink = (MultiSinkObject) ctx->text.sink;
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
 *
 * NOTE: it is illegal to call this routine unless there is a valid line table!
 */
/*** figure out what line it is on ***/
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
 *
 * NOTE: It is illegal to call this routine unless there is a valid line table!
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
 *
 * NOTE: It is illegal to call this routine unless there is a valid line table!
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
      *x = ctx->text.margin.left;
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
      force_rebuild = TRUE;
    }

  if (force_rebuild || position != ctx->text.lt.top)
    {
      (void)bzero((char *)ctx->text.lt.info, size);
      (void)_BuildLineTable(ctx, ctx->text.lt.top = position, zeroPosition, 0);
    }
}

/*
 * This assumes that the line table does not change size.
 */
static XawTextPosition
_BuildLineTable(TextWidget ctx, XawTextPosition position,
		XawTextPosition min_pos, int line)
{
  XawTextLineTableEntry *lt = ctx->text.lt.info + line;
  XawTextPosition endPos;
  Position y;
  int width, realW, realH;
  Widget src = ctx->text.source;

  if (((ctx->text.resize == XawtextResizeWidth)
       || (ctx->text.resize == XawtextResizeBoth))
      || (ctx->text.wrap == XawtextWrapNever))
    width = BIGNUM;
  else
    width = Max(0, ((int)XtWidth(ctx) - (int)HMargins(ctx)));

  y = line == 0 ? ctx->text.margin.top : lt->y;

  /* CONSTCOND */
  while (TRUE)
    {
      lt->y = y;
      lt->position = position;

      XawTextSinkFindPosition(ctx->text.sink, position, ctx->text.margin.left,
			      width, ctx->text.wrap == XawtextWrapWord,
			      &endPos, &realW, &realH);
      lt->textWidth = realW;
      y += realH;

      if (ctx->text.wrap == XawtextWrapNever)
	endPos = SrcScan(src, position, XawstEOL, XawsdRight, 1, True);

      /* We have reached the end. */
      if (endPos == ctx->text.lastPos
	  && SrcScan(src, position, XawstEOL, XawsdRight, 1, False) == endPos)
	{
	  while (line++ < ctx->text.lt.lines)
	    {
	      ++lt;
	      lt->y = y;
	      lt->position = endPos + 1;
	      lt->textWidth = 0;
	      y += realH;
	    }
	  return (endPos);
	}

      ++lt;
      ++line;
      if (line > ctx->text.lt.lines
	  || ((lt->position == (position = endPos)) && position > min_pos))
	return (position);
    }
  /*NOTREACHED*/
}

/*	Function Name: GetWidestLine
 *	Description: Returns the width (in pixels) of the widest line that
 *		     is currently visable.
 *	Arguments: ctx - the text widget.
 *	Returns: the width of the widest line.
 *
 * NOTE: This function requires a valid line table.
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

static void
CheckVBarScrolling(TextWidget ctx)
{
  float first, last;
  Boolean temp = (ctx->text.vbar == NULL);

  if (ctx->text.scroll_vert == XawtextScrollNever)
    return;

  if (ctx->text.lastPos > 0 && ctx->text.lt.lines > 0)
    {
      first = ctx->text.lt.top / (float)ctx->text.lastPos;

      if (ctx->text.lt.info[ctx->text.lt.lines].position < ctx->text.lastPos)
	last = (ctx->text.lt.info[ctx->text.lt.lines].position
		/ (float)ctx->text.lastPos);
      else
	last = 1.0;

      if (ctx->text.scroll_vert == XawtextScrollWhenNeeded)
	{
	  if (IsPositionVisible(ctx, 0)
	      && IsPositionVisible(ctx, ctx->text.lastPos))
	    DestroyVScrollBar(ctx);
	  else
	    CreateVScrollBar(ctx);
	}

      if (ctx->text.vbar != NULL)
	XawScrollbarSetThumb(ctx->text.vbar, first, last - first);

      if ((ctx->text.vbar == NULL) != temp)
	{
	  _XawTextNeedsUpdating(ctx, zeroPosition, ctx->text.lastPos);
	  if (ctx->text.vbar == NULL)
	    _XawTextBuildLineTable(ctx, zeroPosition, False);
	}
    }
  else if (ctx->text.vbar != NULL)
    {
      if (ctx->text.scroll_vert == XawtextScrollWhenNeeded)
	DestroyVScrollBar(ctx);
      else if (ctx->text.scroll_vert == XawtextScrollAlways)
	XawScrollbarSetThumb(ctx->text.vbar, 0.0, 1.0);
    }
}

/*
 * This routine is used by Text to notify an associated scrollbar of the
 * correct metrics (position and shown fraction) for the text being currently
 * displayed in the window.
 */
void
_XawTextSetScrollBars(TextWidget ctx)
{
  float first, denom, widest;
  Bool htemp, vtemp;

  CheckVBarScrolling(ctx);

  if (ctx->text.scroll_horiz == XawtextScrollNever)
    return;

  htemp = (ctx->text.hbar == NULL);
  vtemp = (ctx->text.vbar == NULL);
  denom = GetWidestLine(ctx);
  widest = ((int)XtWidth(ctx) - RHMargins(ctx)) / denom;

  if (ctx->text.scroll_horiz == XawtextScrollWhenNeeded)
    {
      if (widest < 1.0 || ctx->text.margin.left != ctx->text.r_margin.left)
	CreateHScrollBar(ctx);
      else
	DestroyHScrollBar(ctx);
    }

  if ((ctx->text.hbar == NULL) != htemp)
    {
      _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
      CheckVBarScrolling(ctx);	/* Recheck need for vbar, now that we added
				   or removed the hbar.*/
      FlushUpdate(ctx);
      _XawTextNeedsUpdating(ctx, zeroPosition, ctx->text.lastPos);
    }

  if (ctx->text.hbar != NULL)
    {
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
  width = XawMax(0, XawMin(x2 - dst_x, width));
  height = XawMax(0, XawMin(y2 - dst_y, height));

  XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx), ctx->text.gc,
	    src_x, src_y, width, height, dst_x, dst_y);
}

/*
 * Funcao generica para scroll de texto.
 * Permite scroll horizontal e vertical simultaneamente.
 * Parametros:
 *	ctx	- Widget de texto
 *	vlines	- Numero de linhas a rolar na vertical
 *	hpixels	- Numero de pixels a rolar na horizontal
 */
static void
XawTextScroll(TextWidget ctx, int vlines, int hpixels)
{
  XawTextPosition top;
  XRectangle vrect, hrect, rect;
  XawTextLineTable *lt;
  Arg arglist[1];
  int count;
  int vwidth, vheight;			/* largura e altura visivel */
  int dst_x, dst_y;

  vwidth = (int)XtWidth(ctx) - RHMargins(ctx);
  vheight = (int)XtHeight(ctx) - RVMargins(ctx);
  lt = &ctx->text.lt;

  /*
   * Paranoia e NoOps comuns
   */
  if (hpixels < 0 && ctx->text.margin.left - hpixels > ctx->text.r_margin.left)
    hpixels = ctx->text.margin.left - ctx->text.r_margin.left;
  if (vlines < 0)
    {
      if (IsPositionVisible(ctx, 0))
	vlines = 0;
      else
	{
	  /* Verifica se nao esta' pedindo muitas linhas */
	  count = 0;
	  top = SrcScan(ctx->text.source, lt->top, XawstEOL,
			XawsdLeft, -vlines + 1, False);
	  if (ctx->text.wrap == XawtextWrapNever)
	    {
	      while (top < lt->top)
		{
		  top = SrcScan(ctx->text.source, top, XawstEOL,
				XawsdRight, 1, True);
		  ++count;
		}
	    }
	  else
	    {
	      int dim;

	      while (top < lt->top)
		{
		  XawTextSinkFindPosition(ctx->text.sink, top,
				    ctx->text.margin.left,
				    vwidth, ctx->text.wrap == XawtextWrapWord,
				    &top, &dim, &dim);
		  ++count;
		}
	    }
	  if (-vlines > count)
	    vlines = -count;
	}
    }

  if (!lt || vwidth <= 0 || vheight <= 0 || (vlines == 0 && hpixels == 0))
    return;

  hrect.y = ctx->text.r_margin.top;
  hrect.height = vheight;
  vrect.x = ctx->text.r_margin.left;
  vrect.width = vwidth;

  /*
   * Calcula scroll vertical e topo da tabela de linhas
   */
  if (vlines > 0)		/* VScroll Up */
    {
      if (vlines < lt->lines)
	{
	  top = XawMin(lt->info[vlines].position, ctx->text.lastPos);
	  vrect.y = lt->info[lt->lines - vlines].y;
	  vrect.height = XtHeight(ctx) - vrect.y - ctx->text.r_margin.bottom;
	  hrect.y = ctx->text.r_margin.top;
	  hrect.height = vrect.y - hrect.y;
	}
      else if (ctx->text.wrap == XawtextWrapNever)
	top = SrcScan(ctx->text.source, lt->top, XawstEOL,
		      XawsdRight, vlines, True);
      else	/* O caso mais dificil */
	{
	  int dim;

	  top = lt->top;
	  count = 0;
	  while (count++ < vlines)
	    XawTextSinkFindPosition(ctx->text.sink, top,
				    ctx->text.margin.left,
				    vwidth, ctx->text.wrap == XawtextWrapWord,
				    &top, &dim, &dim);
	}
    }
  else if (vlines < 0)		/* VScroll Down */
    {
      if (-vlines < lt->lines)
	{
	  vrect.y = ctx->text.r_margin.top;
	  if (-vlines < lt->lines - 1)
	    vrect.height = lt->info[-vlines + 1].y - vrect.y;
	  else
	    vrect.height = vheight;
	  hrect.y = vrect.y + vrect.height;
	  hrect.height = XtHeight(ctx) - hrect.y - ctx->text.r_margin.bottom;
	}

      top = SrcScan(ctx->text.source, lt->top, XawstEOL,
		    XawsdLeft, -vlines + 1, False);
      if (ctx->text.wrap != XawtextWrapNever)
	/* Uma linha terminada em LF pode ocupar mais de uma linha
	 * no widget. Encontra a posicao do topo.
	 */
	{
	  int num_lines, dim;

	  count = 0;
	  num_lines = CountLines(ctx, top, lt->top);
	  while (count++ < num_lines + vlines)
	    XawTextSinkFindPosition(ctx->text.sink, top,
				    ctx->text.margin.left,
				    vwidth, ctx->text.wrap == XawtextWrapWord,
				    &top, &dim, &dim);
	}
    }
  else
    top = lt->top;

  /*
   * Calcula scroll horizontal
   */
  if (hpixels > 0)		/* HScroll Left */
    {
      if (hpixels < vwidth)
	{
	  hrect.x = XtWidth(ctx) - ctx->text.r_margin.right - hpixels;
	  hrect.width = XtWidth(ctx) - ctx->text.r_margin.right - hrect.x;
	}
    }
  else if (hpixels < 0)		/* HScroll Right */
    {
      if (-hpixels < vwidth)
	{
	  hrect.x = ctx->text.r_margin.left;
	  hrect.width = -hpixels;
	}
    }

  /*
   * Executa scroll vertical, reconstruindo tabela de linhas
   */
  (void)_XawTextBuildLineTable(ctx, top, False);
  lt = &ctx->text.lt;	/* Caso ponteiro tenha sido modificado */

  XtSetArg(arglist[0], XtNinsertPosition, lt->top + lt->lines);

  /*
   * Executa scroll horizontal, setando ctx->text.margin.left adequadamente
   */
  ctx->text.margin.left -= hpixels;

  if (XawAbs(vlines) >= lt->lines || XawAbs(hpixels) >= vwidth)
    {
      DisplayTextWindow((Widget)ctx);
      _XawImSetValues((Widget)ctx, arglist, 1);
      return;
    }

  /*
   * Copia area visivel, se background nao e' um pixmap
   */
  if (vlines > 0)
    {
      dst_y = ctx->text.r_margin.top;
      rect.y = lt->info[vlines].y;
      rect.height = vheight + dst_y - rect.y;
    }
  else if (vlines < 0)
    {
      dst_y = lt->info[-vlines].y;
      rect.y = ctx->text.r_margin.top;
      rect.height = vheight + rect.y - dst_y;
    }
  else
    {
      dst_y = rect.y = ctx->text.r_margin.top;
      rect.height = vheight;
    }
  if (hpixels > 0)
    {
      dst_x = ctx->text.r_margin.left;
      rect.x = hpixels + dst_x;
      rect.width = XtWidth(ctx) - rect.x - ctx->text.r_margin.right;
    }
  else if (hpixels < 0)
    {
      dst_x = -hpixels + ctx->text.r_margin.left;
      rect.x = ctx->text.r_margin.left;
      rect.width = XtWidth(ctx) - dst_x;
    }
  else
    {
      dst_x = rect.x = ctx->text.r_margin.left;
      rect.width = vwidth;
    }
  if (ctx->core.background_pixmap == XtUnspecifiedPixmap)
    DoCopyArea(ctx, rect.x, rect.y, rect.width, rect.height, dst_x, dst_y);
  else if (vlines > 0
	   && IsPositionVisible(ctx, ctx->text.lastPos)
	   && LineForPosition(ctx, ctx->text.lastPos) == 0)
    ClearWindow((Widget)ctx);

  /*
   * Atualiza area que se tornou visivel
   */
  if (vlines)
    SinkClearToBG(ctx->text.sink,
		  vrect.x, vrect.y, vrect.width, vrect.height);
  if (hpixels)
    SinkClearToBG(ctx->text.sink,
		  hrect.x, hrect.y, hrect.width, hrect.height);

  if (ctx->core.background_pixmap != XtUnspecifiedPixmap
      || (vlines && hpixels))
    _XawTextNeedsUpdating(ctx, zeroPosition, ctx->text.lastPos);
  else
    {
      if (vlines)
	UpdateTextInRectangle(ctx, &vrect);
      else
	UpdateTextInRectangle(ctx, &hrect);
    }

  _XawTextSetScrollBars(ctx);
  _XawImSetValues((Widget)ctx, arglist, 1);
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

/*	Function Name: UpdateTextInLine
 *	Description: Updates some text in a given line.
 *	Arguments: ctx - the text widget.
 *		   line - the line number (in the line table) of this line.
 *		   left, right - left and right pixel offsets of the
 *				 area to update.
 *	Returns: none.
 */
static void
UpdateTextInLine(TextWidget ctx, int line, int left, int right)
{
  XawTextPosition pos1, pos2;
  int width, height, local_left, local_width;
  XawTextLineTableEntry *lt = ctx->text.lt.info + line;

  if ((int)lt->textWidth + ctx->text.margin.left < left
      || ctx->text.margin.left > right)
    return;			/* no need to update. */

  local_width = left - ctx->text.margin.left;
  XawTextSinkFindPosition(ctx->text.sink, lt->position,
			  ctx->text.margin.left,
			  local_width, False, &pos1, &width, &height);

  if (right >= (Position)lt->textWidth - ctx->text.margin.left)
    {
      if (IsValidLine(ctx, line + 1)
	  && ctx->text.lt.info[line + 1].position <= ctx->text.lastPos)
	pos2 = SrcScan(ctx->text.source, (lt + 1)->position, XawstPositions,
		       XawsdLeft, 1, True);
      else
	pos2 = GETLASTPOS;
    }
  else
    {
      XawTextPosition t_pos;

      local_left = ctx->text.margin.left + width;
      local_width = right  - local_left;
      XawTextSinkFindPosition(ctx->text.sink, pos1, local_left,
			      local_width, False, &pos2, &width, &height);

      t_pos = SrcScan(ctx->text.source, pos2,
		      XawstPositions, XawsdRight, 1, True);
      if (t_pos < (lt + 1)->position)
	pos2 = t_pos;
    }

  _XawTextNeedsUpdating(ctx, pos1,
			pos2 + (ctx->text.wrap != XawtextWrapNever));
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
  XawTextPosition top, first, last;
  XawTextLineTable *lt = &(ctx->text.lt);
  int dim, vlines = 0, wwidth = XtWidth(ctx) - RHMargins(ctx);

  top = percent * ctx->text.lastPos;

  if (top == lt->top || !lt->lines)
    return;

  _XawTextPrepareToUpdate(ctx);

  last = lt->top;

  if (top > lt->top)		/* VScroll Up */
    {
      if (top > lt->top && top < lt->info[lt->lines].position)
	vlines = LineForPosition(ctx, top);
      else if (ctx->text.wrap != XawtextWrapNever)
	{
	  while (last < top)
	    {
	      XawTextSinkFindPosition(ctx->text.sink, last,
				      ctx->text.margin.left,
				      wwidth,
				      ctx->text.wrap == XawtextWrapWord,
				      &last, &dim, &dim);
	      ++vlines;
	    }
	}
      else
	{
	  while (last < top)
	    {
	      last = SrcScan(ctx->text.source, last, XawstEOL,
			     XawsdRight, 1, True);
	      ++vlines;
	    }
	}
    }
  else			/* VScroll Down */
    {
      /*
       * Conta numero de linhas
       */
      while (last > top)
	{
	  first = last;
	  last = SrcScan(ctx->text.source, last, XawstEOL,
			 XawsdLeft, 1, True);
	  vlines -= CountLines(ctx, last, first);
	}
      /*
       * Normaliza
       */
      if (ctx->text.wrap != XawtextWrapNever)
	{
	  while (last < top)
	    {
	      XawTextSinkFindPosition(ctx->text.sink, last,
				      ctx->text.margin.left,
				      wwidth,
				      ctx->text.wrap == XawtextWrapWord,
				      &last, &dim, &dim);
	      ++vlines;
	    }
	}
    }

  XawTextScroll(ctx, vlines, 0);
  _XawTextExecuteUpdate(ctx);
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
ConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type,
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
				  target, type, (XPointer*)&std_targets,
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

      XtSetArg(args[0], XtNeditType,&edit_mode);
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
		  XtFree(*value);
		  return (False);
		}
	      XtFree(*value);
	      *value = (XtPointer)textprop.value;
	      *length = textprop.nitems;
	    }
	  else
	    *length = strlen(*value);
	}
      else
	{
	  *value = XtMalloc((salt->length + 1) * sizeof(unsigned char));
	  strcpy (*value, salt->contents);
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
	      XtFree(*value);
	      return (False);
	    }
	  XtFree(*value);
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
	_XawTextZapSelection(ctx, (XEvent *)NULL, True);
      *value = NULL;
      *type = XA_NULL(d);
      *length = 0;
      *format = 32;
      return (True);
    }

  if (XmuConvertStandardSelection(w, ctx->text.time, selection, target, type,
				  (XPointer *)value, length, format))
    return (True);

  /* else */
  return (False);
}

/*	Function Name: GetCutBuffferNumber
 *	Description: Returns the number of the cut buffer.
 *	Arguments: atom - the atom to check.
 *	Returns: the number of the cut buffer representing this atom or
 *		 NOT_A_CUT_BUFFER.
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
LoseSelection(Widget w, Atom *selection)
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
   * Must walk the selection list in opposite order from UnsetSelection.
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
       * Must walk the selection list in opposite order from UnsetSelection.
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
    LoseSelection((Widget)ctx, selections + i);
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
			 ConvertSelection, LoseSelection,
			 (XtSelectionDoneProc)NULL);
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
	   * If this is a cut buffer.
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
	    XtOwnSelection(w, selection, ctx->text.time, ConvertSelection,
			   LoseSelection, (XtSelectionDoneProc)NULL);
	}
    }
  else
    XawTextUnsetSelection((Widget)ctx);
}

/*
 * This internal routine deletes the text from pos1 to pos2 in a source and
 * then inserts, at pos1, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any).
 *
 * NOTE: It is illegal to call this routine unless there is a valid line table!
 */
int
_XawTextReplace(TextWidget ctx, XawTextPosition pos1, XawTextPosition pos2,
		XawTextBlock *text)
{
  int i, line1, delta, error;
  XawTextPosition updateFrom, updateTo;
  Widget src = ctx->text.source;
  XawTextEditType edit_mode;
  Arg args[1];
  Boolean tmp = ctx->text.update_disabled;

  ctx->text.update_disabled = True; /* No redisplay during replacement. */

  /*
   * The insertPos may not always be set to the right spot in XawtextAppend 
   */
  XtSetArg(args[0], XtNeditType, &edit_mode);
  XtGetValues(src, args, ONE);

  if (pos1 == ctx->text.insertPos && edit_mode == XawtextAppend)
    {
      ctx->text.insertPos = ctx->text.lastPos;
      pos2 = SrcScan(src, ctx->text.insertPos, XawstPositions, XawsdRight,
		     (int)(ctx->text.insertPos - pos1), True);
      pos1 = ctx->text.insertPos;
      if ((pos1 == pos2) && (text->length == 0))
	{
	  ctx->text.update_disabled = FALSE; /* rearm redisplay. */
	  return (XawEditError);
	}
    }

  updateFrom = SrcScan(src, pos1, XawstWhiteSpace, XawsdLeft, 1, False);
  updateFrom = Max(updateFrom, ctx->text.lt.top);

  line1 = LineForPosition(ctx, updateFrom);
  if ((error = SrcReplace(src, pos1, pos2, text)) != 0)
    {
      ctx->text.update_disabled = tmp; /* restore redisplay */
      return (error);
    }

  XawTextUnsetSelection((Widget)ctx);

  ctx->text.lastPos = GETLASTPOS;
  if (ctx->text.lt.top >= ctx->text.lastPos)
    {
      _XawTextBuildLineTable(ctx, ctx->text.lastPos, False);
      ClearWindow((Widget)ctx);
      ctx->text.update_disabled = tmp; /* restore redisplay */
      return (0);			/* Things are fine. */
    }

  ctx->text.single_char = (text->length <= 1 && pos2 - pos1 <= 1);

  delta = text->length - (pos2 - pos1);

  if (delta < ctx->text.lastPos)
    {
      for (pos2 += delta, i = 0; i < ctx->text.numranges; i++)
	{
	  if (ctx->text.updateFrom[i] > pos1)
	    ctx->text.updateFrom[i] += delta;
	  if (ctx->text.updateTo[i] >= pos1)
	    ctx->text.updateTo[i] += delta;
	}
    }

  /*
   * fixup all current line table entries to reflect edit.
   * %%% it is not legal to do arithmetic on positions.
   * using Scan would be more proper.
   */
  if (delta != 0)
    {
      XawTextLineTableEntry *lineP;

      i = LineForPosition(ctx, pos1) + 1;
      for (lineP = ctx->text.lt.info + i; i <= ctx->text.lt.lines;
	   i++, lineP++)
	lineP->position += delta;
    }

  /*
   * Now process the line table and fixup in case edits caused
   * changes in line breaks. If we are breaking on word boundaries,
   * this code checks for moving words to and from lines.
   */
  if (IsPositionVisible(ctx, updateFrom))
    {
      updateTo = _BuildLineTable(ctx, ctx->text.lt.info[line1].position,
				 pos1, line1);
      _XawTextNeedsUpdating(ctx, updateFrom, updateTo);
    }

  ctx->text.update_disabled = tmp; /* restore redisplay */
  return (0);			/* Things are fine. */
}

/*
 * This routine will display text between two arbitrary source positions.
 * In the event that this span contains highlighted text for the selection,
 * only that portion will be displayed highlighted.
 *
 * NOTE: it is illegal to call this routine unless there
 *	 is a valid line table!
 */
static void
DisplayText(Widget w, XawTextPosition pos1, XawTextPosition pos2)
{
  TextWidget ctx = (TextWidget)w;
  int x, y;
  int height, line, i, lastPos = ctx->text.lastPos;
  XawTextPosition startPos, endPos;
  Boolean clear_eol, done_painting;

  pos1 = (pos1 < ctx->text.lt.top) ? ctx->text.lt.top : pos1;
  pos2 = FindGoodPosition(ctx, pos2);
  if (pos1 >= pos2 || !LineAndXYForPosition(ctx, pos1, &line, &x, &y))
    return;			/* line not visible, or pos1 >= pos2 */

  for (startPos = pos1, i = line;
       IsValidLine(ctx, i) && (i < ctx->text.lt.lines);
       i++)
    {
      if ((endPos = ctx->text.lt.info[i + 1].position) > pos2)
	{
	  clear_eol = (endPos = pos2) >= lastPos;
	  done_painting = !clear_eol || ctx->text.single_char;
	}
      else
	{
	  clear_eol = TRUE;
	  done_painting = FALSE;
	}

      height = ctx->text.lt.info[i + 1].y - ctx->text.lt.info[i].y;

      if (endPos > startPos)
	{
	  if (startPos >= ctx->text.s.right || endPos <= ctx->text.s.left)
	    XawTextSinkDisplayText(ctx->text.sink, x, y,
				   startPos, endPos, False);
	  else if (startPos >= ctx->text.s.left && endPos <= ctx->text.s.right)
	    XawTextSinkDisplayText(ctx->text.sink, x, y,
				   startPos, endPos, True);
	  else
	    {
	      DisplayText(w, startPos, ctx->text.s.left);
	      DisplayText(w, Max(startPos, ctx->text.s.left),
			  Min(endPos, ctx->text.s.right));
	      DisplayText(w, ctx->text.s.right, endPos);
	    }
	}
      startPos = endPos;
      if (clear_eol)
	{
	  SinkClearToBG(ctx->text.sink,
			(ctx->text.lt.info[i].textWidth
			 + ctx->text.margin.left),
			y, XtWidth(w), height);

	  /*
	   * We only get here if single character is true, and we need
	   * to clear to the end of the screen.  We know that since there
	   * was only one character deleted that this is the same
	   * as clearing an extra line, so we do this, and are done.
	   *
	   * This a performance hack, and a pretty gross one, but it works.
	   *
	   * Chris Peterson 11/13/89.
	   */
	  if (done_painting)
	    {
	      y += height;
	      SinkClearToBG(ctx->text.sink,
			    ctx->text.margin.left, y,
			    w->core.width, height);

	      break;		/* set single_char to FALSE and return. */
	    }
	}

      x = ctx->text.margin.left;
      y = ctx->text.lt.info[i + 1].y;
      if (done_painting
	  || y >= (int)(XtHeight(ctx) - ctx->text.margin.bottom))
	break;
    }
  ctx->text.single_char = FALSE;
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
    case XawselectPosition:	/*FALLTHROUGH*/
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
 * Clear the window to background color.
 */
static void
ClearWindow(Widget w)
{
  TextWidget ctx = (TextWidget)w;

  if (XtIsRealized(w))
    SinkClearToBG(ctx->text.sink, 0, 0, XtWidth(w), XtHeight(w));
}

/*	Function Name: _XawTextClearAndCenterDisplay
 *	Description: Redraws the display with the cursor in insert point
 *		     centered vertically.
 *	Arguments: ctx - the text widget.
 *	Returns: none.
 */
void
_XawTextClearAndCenterDisplay(TextWidget ctx)
{
  int insert_line = LineForPosition(ctx, ctx->text.insertPos);
  int scroll_by = insert_line - ctx->text.lt.lines / 2;

  XawTextScroll(ctx, scroll_by, 0);
  DisplayTextWindow((Widget)ctx);
}

/*
 * Internal redisplay entire window.
 * Legal to call only if widget is realized.
 */
static void
DisplayTextWindow(Widget w)
{
  TextWidget ctx = (TextWidget)w;

  ClearWindow(w);
  _XawTextBuildLineTable(ctx, ctx->text.lt.top, False);
  _XawTextNeedsUpdating(ctx, zeroPosition, ctx->text.lastPos);
  _XawTextSetScrollBars(ctx);
}

static void
TextSinkResize(Widget w)
{
  if (w && XtClass(w)->core_class.resize)
    XtClass(w)->core_class.resize(w);
}

/*
 * This routine checks to see if the window should be resized (grown or
 * shrunk) when text to be painted overflows to the right or
 * the bottom of the window. It is used by the keyboard input routine.
 */
void
_XawTextCheckResize(TextWidget ctx)
{
  Widget w = (Widget)ctx;
  int line = 0, old_height;
  XtWidgetGeometry rbox, return_geom;

  if (ctx->text.resize == XawtextResizeWidth
      || ctx->text.resize == XawtextResizeBoth)
    {
      XawTextLineTableEntry *lt;
      XRectangle rect;

      XawTextSinkGetCursorBounds(ctx->text.sink, &rect);
      rbox.width = 0;
      for (lt = ctx->text.lt.info;
	   IsValidLine(ctx, line) && (line < ctx->text.lt.lines);
	   line++, lt++)
	{
	  if ((int)(lt->textWidth + ctx->text.margin.left) > (int)rbox.width)
	    rbox.width = lt->textWidth + ctx->text.margin.left;
	}

      rbox.width += ctx->text.margin.right + rect.width;
      if (rbox.width > ctx->core.width)	/* Only get wider. */
	{
	  rbox.request_mode = CWWidth;
	  switch (XtMakeGeometryRequest(w, &rbox, &return_geom))
	    {
	    case XtGeometryAlmost:
	      (void)XtMakeGeometryRequest(w, &return_geom,
					  (XtWidgetGeometry*)NULL);
	      /* FALLTHROUGH */
	    case XtGeometryYes:
	    case XtGeometryDone:
	      TextSinkResize(ctx->text.sink);
	    default:
	      break;
	    }
	}
    }

  if (!(ctx->text.resize == XawtextResizeHeight
	|| ctx->text.resize == XawtextResizeBoth))
    return;

  if (IsPositionVisible(ctx, ctx->text.lastPos))
    line = LineForPosition(ctx, ctx->text.lastPos);
  else
    line = ctx->text.lt.lines;

  if ((line + 1) == ctx->text.lt.lines)
    return;

  old_height = ctx->core.height;
  rbox.request_mode = CWHeight;
  rbox.height = XawTextSinkMaxHeight(ctx->text.sink, line + 1) + VMargins(ctx);

  if ((int)rbox.height < old_height)	/* It will only get taller. */
    return;

  if (XtMakeGeometryRequest(w, &rbox, &return_geom) == XtGeometryAlmost)
    {
      if (XtMakeGeometryRequest(w, &return_geom, (XtWidgetGeometry *)NULL)
	  != XtGeometryYes)
	return;
      else
	TextSinkResize(ctx->text.sink);
    }

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
}

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

  if (nelems > ctx->text.s.array_size)
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

/*	Function Name: SetSelection
 *	Description: Sets the current selection.
 *	Arguments: ctx - the text widget.
 *		   defaultSel - the default selection.
 *		   l, r - the left and right ends of the selection.
 *		   list, nelems - the selection list (as strings).
 *	Returns: none.
 *
 *  NOTE: if (ctx->text.s.left >= ctx->text.s.right) then the selection
 *        is unset.
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

/*	Function Name: ModifySelection
 *	Description: Modifies the current selection.
 *	Arguments: ctx - the text widget.
 *		   left, right - the left and right ends of the selection.
 *	Returns: none.
 *
 *  NOTE: if (ctx->text.s.left >= ctx->text.s.right) then the selection
 *        is unset.
 */
static void
ModifySelection(TextWidget ctx, XawTextPosition left, XawTextPosition right)
{
  if (left == right)
    ctx->text.insertPos = left;
  _SetSelection(ctx, left, right, (Atom*)NULL, ZERO);
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
    ctx->text.search->selection_changed = TRUE;

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

/*	Function Name: UpdateTextInRectangle.
 *	Description: Updates the text in a rectangle.
 *	Arguments: ctx - the text widget.
 *		   rect - the rectangle to update.
 *	Returns: none.
 */
static void
UpdateTextInRectangle(TextWidget ctx, XRectangle *rect)
{
  XawTextLineTableEntry *info = ctx->text.lt.info;
  int line, x = rect->x, y = rect->y;
  int right = rect->width + x, bottom = rect->height + y;

  for (line = 0;
       (line < ctx->text.lt.lines
	&& IsValidLine(ctx, line) && info->y < bottom);
       line++, info++)
    if ((info + 1)->y >= y)
      UpdateTextInLine(ctx, line, x, right);
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
  XRectangle expose;

  if (event->type == Expose)
    {
      expose.x = event->xexpose.x;
      expose.y = event->xexpose.y;
      expose.width = event->xexpose.width;
      expose.height = event->xexpose.height;
      if (Superclass->core_class.expose)
	(*Superclass->core_class.expose)(w, event, region);
    }
  else if (event->type == GraphicsExpose)
    {
      expose.x = event->xgraphicsexpose.x;
      expose.y = event->xgraphicsexpose.y;
      expose.width = event->xgraphicsexpose.width;
      expose.height = event->xgraphicsexpose.height;
    }
  else	/* No Expose */
    return;			/* no more processing necessary. */

  _XawTextPrepareToUpdate(ctx);
  UpdateTextInRectangle(ctx, &expose);
  _XawTextExecuteUpdate(ctx);
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
      ctx->text.numranges = 0;
      ctx->text.showposition = FALSE;
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
  int i, w;
  XawTextPosition updateFrom, updateTo;

  if (!XtIsRealized((Widget)ctx))
    {
      ctx->text.numranges = 0;
      return;
    }
  while (ctx->text.numranges > 0)
    {
      updateFrom = ctx->text.updateFrom[0];
      w = 0;
      for (i = 1 ; i < ctx->text.numranges ; i++)
	{
	  if (ctx->text.updateFrom[i] < updateFrom)
	    {
	      updateFrom = ctx->text.updateFrom[i];
	      w = i;
	    }
	}
      updateTo = ctx->text.updateTo[w];
      ctx->text.numranges--;
      ctx->text.updateFrom[w] = ctx->text.updateFrom[ctx->text.numranges];
      ctx->text.updateTo[w] = ctx->text.updateTo[ctx->text.numranges];

      for (i = ctx->text.numranges - 1 ; i >= 0 ; i--)
	{
	  while (ctx->text.updateFrom[i] <= updateTo
		 && i < ctx->text.numranges)
	    {
	      updateTo = ctx->text.updateTo[i];
	      ctx->text.numranges--;
	      ctx->text.updateFrom[i] =
		ctx->text.updateFrom[ctx->text.numranges];
	      ctx->text.updateTo[i] =
		ctx->text.updateTo[ctx->text.numranges];
	    }
	}
      DisplayText((Widget)ctx, updateFrom, updateTo);
    }
}

static int
CountLines(TextWidget ctx, XawTextPosition left, XawTextPosition right)
{
  if (ctx->text.wrap == XawtextWrapNever || left >= right)
    return (1);
  else
    {
      int dim, lines = 0, wwidth = XtWidth(ctx) - RHMargins(ctx);

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

/*
 * This is a private utility routine used by _XawTextExecuteUpdate. This
 * routine worries about edits causing new data or the insertion point becoming
 * invisible (off the screen, or under the horiz. scrollbar). Currently
 * it always makes it visible by scrolling. It probably needs
 * generalization to allow more options.
 */
void
_XawTextShowPosition(TextWidget ctx)
{
  int hpixels, vlines;
  XawTextPosition first, last, top;
  Bool visible;

  /*
   * Verifica se e' necessario fazer scroll horizontal
   */
  if (ctx->text.wrap == XawtextWrapNever)
    {
      int x, vwidth, distance, dim;
      XRectangle rect;

      vwidth = (int)XtWidth(ctx) - (int)RHMargins(ctx);
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

  if (!XtIsRealized((Widget)ctx) || (!hpixels && visible))
    return;

  vlines = 0;
  first = ctx->text.lt.top;
  top = SrcScan(ctx->text.source, ctx->text.insertPos,
		XawstEOL, XawsdLeft, 1, False);

  if (ctx->text.wrap != XawtextWrapNever)
    {
      int dim, vwidth = XtWidth(ctx) - RHMargins(ctx);

      last = top;
      /*CONSTCOND*/
      while (1)
	{
	  XawTextSinkFindPosition(ctx->text.sink, last,
				  ctx->text.margin.left, vwidth,
				  ctx->text.wrap == XawtextWrapWord,
				  &last, &dim, &dim);
	  if (last < ctx->text.insertPos)
	    top = last;
	  else
	    break;
	}
    }

  if (ctx->text.insertPos < first)	/* Scroll Down */
    {
      while (first > top)
	{
	  last = first;
	  first = SrcScan(ctx->text.source, first,
			  XawstEOL, XawsdLeft, 1, True);
	  vlines -= CountLines(ctx, first, last);
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
	}
    }

  XawTextScroll(ctx, vlines ? vlines - (ctx->text.lt.lines >> 1) : 0, hpixels);
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
  XtFree((char *)ctx->text.updateFrom);
  XtFree((char *)ctx->text.updateTo);
}

/*
 * by the time we are managed (and get this far) we had better
 * have both a source and a sink 
 */
void
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
XawTextSetValues(Widget current, Widget request, Widget c_new,
		 ArgList args, Cardinal *num_args)
{
  TextWidget oldtw = (TextWidget)current;
  TextWidget newtw = (TextWidget)c_new;
  Boolean redisplay = FALSE;
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
      redisplay = TRUE;
    }

  if (oldtw->text.scroll_vert != newtw->text.scroll_vert)
    {
      if (newtw->text.scroll_vert == XawtextScrollNever)
	DestroyVScrollBar(newtw);
      else if (newtw->text.scroll_vert == XawtextScrollAlways)
	CreateVScrollBar(newtw);
      redisplay = TRUE;
    }

  if (oldtw->text.r_margin.bottom != newtw->text.r_margin.bottom)
    {
      newtw->text.margin.bottom = newtw->text.r_margin.bottom;
      if (newtw->text.hbar != NULL)
	newtw->text.margin.bottom += newtw->text.hbar->core.height
	  + newtw->text.hbar->core.border_width;
      redisplay = TRUE;
    }

  if (oldtw->text.scroll_horiz != newtw->text.scroll_horiz)
    {
      if (newtw->text.scroll_horiz == XawtextScrollNever) 
	DestroyHScrollBar(newtw);
      else if (newtw->text.scroll_horiz == XawtextScrollAlways)
	CreateHScrollBar(newtw);
      redisplay = TRUE;
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
      redisplay = TRUE;
    }

  if (oldtw->text.insertPos != newtw->text.insertPos)
    {
      newtw->text.showposition = TRUE;
      redisplay = TRUE;
    }

  _XawTextExecuteUpdate(newtw);
  if (redisplay)
    _XawTextSetScrollBars(newtw);

  return (redisplay);
}

/* invoked by the Simple widget's SetValues */
static Boolean
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

/*	Function Name: GetValuesHook
 *	Description: This is a get values hook routine that gets the
 *		     values in the text source and sink.
 *	Arguments: w - the Text Widget.
 *		   args - the argument list.
 *		   num_args - the number of args.
 *	Returns: none.
 */
static void
XawTextGetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
  XtGetValues(((TextWidget)w)->text.source, args, *num_args);
  XtGetValues(((TextWidget)w)->text.sink, args, *num_args);
}

/*	Function Name: FindGoodPosition
 *	Description: Returns a valid position given any postition
 *	Arguments: pos - any position.
 *	Returns: a position between (0 and lastPos);
 */
static XawTextPosition
FindGoodPosition(TextWidget ctx, XawTextPosition pos)
{
  if (pos < 0)
    return (0);
  return (((pos > ctx->text.lastPos) ? ctx->text.lastPos : pos));
}

/* Li wrote this so the IM can find a given text position's screen position. */
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

  _XawTextCheckResize(ctx);
  _XawTextExecuteUpdate(ctx);
  _XawTextSetScrollBars(ctx);

  return (result);
}

XawTextPosition 
XawTextTopPosition(Widget w)
{
  return (((TextWidget) w)->text.lt.top);
}

void
XawTextSetInsertionPoint(Widget w, XawTextPosition position)
{
  TextWidget ctx = (TextWidget)w;

  _XawTextPrepareToUpdate(ctx);
  ctx->text.insertPos = FindGoodPosition(ctx, position);
  ctx->text.showposition = TRUE;

  _XawTextExecuteUpdate(ctx);
}

XawTextPosition
XawTextGetInsertionPoint(Widget w)
{
  return (((TextWidget)w)->text.insertPos);
}

/*
 * NOTE: Must walk the selection list in opposite order from LoseSelection.
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
	   * As selections are lost the atom_count will decrement.
	   */
	  if (GetCutBufferNumber(sel) == NOT_A_CUT_BUFFER)
	    XtDisownSelection(w, sel, ctx->text.time);
	  LoseSelection(w, &sel); /* In case this is a cut buffer, or 
				     XtDisownSelection failed to call us. */
	}
    }
}

void
XawTextSetSelection(Widget w, XawTextPosition left, XawTextPosition right)
{
  TextWidget ctx = (TextWidget)w;

  _XawTextPrepareToUpdate(ctx);
  _XawTextSetSelection(ctx, FindGoodPosition(ctx, left),
		       FindGoodPosition(ctx, right), (String*)NULL, ZERO);
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
  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
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

  _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
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

/*	Function Name: XawTextSearch(w, dir, text).
 *	Description: searches for the given text block.
 *	Arguments: w - The text widget.
 *		   dir - The direction to search.
 *		   text - The text block containing info about the string
 *		   to search for.
 *	Returns: The position of the text found, or XawTextSearchError on 
 *		 an error.
 */
XawTextPosition
XawTextSearch(Widget w, XawTextScanDirection dir, XawTextBlock *text)
{
  TextWidget ctx = (TextWidget)w;

  return (SrcSearch(ctx->text.source, ctx->text.insertPos, dir, text));
}

TextClassRec textClassRec = {
  {
    /* core fields */
    (WidgetClass)&simpleClassRec,	/* superclass */ 
    "Text",				/* class_name */
    sizeof(TextRec),			/* widget_size */
    XawTextClassInitialize,		/* class_initialize */
    NULL,				/* class_part_init */
    FALSE,				/* class_inited */
    XawTextInitialize,			/* initialize */
    NULL,				/* initialize_hook */
    XawTextRealize,			/* realize */
    _XawTextActionsTable,		/* actions */
    0,				/* num_actions - Set in ClassInitialize */
    resources,				/* resources */
    XtNumber(resources),		/* num_ resource */
    NULLQUARK,				/* xrm_class */
    TRUE,				/* compress_motion */
    XtExposeGraphicsExpose | XtExposeNoExpose,	/* compress_exposure */
    TRUE,				/* compress_enterleave */
    FALSE,				/* visible_interest */
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
    NULL,			/* tm_table - set in ClassInitialize */
    XtInheritQueryGeometry,		/* query_geometry */
    XtInheritDisplayAccelerator,	/* display_accelerator */
    NULL,				/* extension */
  },
  {
    /* Simple fields */
    XawTextChangeSensitive,		/* change_sensitive */
  },
  {
    /* text fields */
    0,					/* empty */
  }
};

WidgetClass textWidgetClass = (WidgetClass)&textClassRec;
