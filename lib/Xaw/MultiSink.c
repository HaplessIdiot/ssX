/* $XConsortium: MultiSink.c,v 1.6 95/01/23 18:34:46 kaleb Exp $ */

/*
 * Copyright 1991 by OMRON Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of OMRON not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  OMRON makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * OMRON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OMRON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *      Author: Li Yuhong	 OMRON Corporation
 */

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

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/MultiSinkP.h>
#include <X11/Xaw/MultiSrcP.h>
#include <X11/Xaw/TextP.h>
#include "XawI18n.h"
#include <stdio.h>
#include <ctype.h>
#include "Private.h"

#ifdef GETLASTPOS
#undef GETLASTPOS		/* We will use our own GETLASTPOS. */
#endif

#define GETLASTPOS	\
	XawTextSourceScan(source, 0, XawstAll, XawsdRight, 1, True)

/*
 * Class Methods
 */
static void XawMultiSinkClassInitialize(void);
static void XawMultiSinkInitialize(Widget, Widget, ArgList, Cardinal*);
static void XawMultiSinkDestroy(Widget);
static void XawMultiSinkResize(Widget);
static Boolean XawMultiSinkSetValues(Widget, Widget, Widget,
				     ArgList, Cardinal*);

/*
 * Prototypes
 */
static int MaxLines(Widget, unsigned int);
static int MaxHeight(Widget, int);
static void SetTabs(Widget, int, short*);
static void DisplayText(Widget, int, int,
			XawTextPosition, XawTextPosition, Bool);
static void InsertCursor(Widget, int, int, XawTextInsertState);
static void FindPosition(Widget, XawTextPosition, int, int, Bool,
			 XawTextPosition*, int*, int*);
static void FindDistance(Widget, XawTextPosition, int, XawTextPosition, int*,
			 XawTextPosition*, int*);
static void Resolve(Widget, XawTextPosition, int, int, XawTextPosition*);
static void GetCursorBounds(Widget, XRectangle*);

static void GetGC(MultiSinkObject);
static int CharWidth(Widget , int, wchar_t);
static unsigned int PaintText(Widget w, GC gc, int x, int y,
			      wchar_t *buf, int len);

static wchar_t wspace[2];

#define offset(field) XtOffsetOf(MultiSinkRec, multi_sink.field)
static XtResource resources[] = {
  {
    XtNfontSet,
    XtCFontSet,
    XtRFontSet,
    sizeof(XFontSet),
    offset(fontset),
    XtRString,
    XtDefaultFontSet
  },
  {
    XtNecho,
    XtCOutput,
    XtRBoolean,
    sizeof(Boolean),
    offset(echo),
    XtRImmediate,
    (XtPointer)True
  },
  {
    XtNdisplayNonprinting,
    XtCOutput,
    XtRBoolean,
    sizeof(Boolean),
    offset(display_nonprinting),
    XtRImmediate,
    (XtPointer)True
  },
};
#undef offset

#define SuperClass		(&textSinkClassRec)
MultiSinkClassRec multiSinkClassRec = {
  {
    /* core_class fields */	
    (WidgetClass)SuperClass,		/* superclass */
    "MultiSink",			/* class_name */
    sizeof(MultiSinkRec),		/* widget_size */
    XawMultiSinkClassInitialize,	/* class_initialize */
    NULL,				/* class_part_initialize */
    FALSE,				/* class_inited */
    XawMultiSinkInitialize,		/* initialize */
    NULL,				/* initialize_hook */
    NULL,				/* obj1 */
    NULL,				/* obj2 */
    0,					/* obj3 */
    resources,				/* resources */
    XtNumber(resources),		/* num_resources */
    NULLQUARK,				/* xrm_class */
    FALSE,				/* obj4 */
    FALSE,				/* obj5 */
    FALSE,				/* obj6 */
    FALSE,				/* obj7 */
    XawMultiSinkDestroy,		/* destroy */
    (XtProc)XawMultiSinkResize,		/* obj8 */
    NULL,				/* obj9 */
    XawMultiSinkSetValues,		/* set_values */
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
  {
    /* text_sink_class fields */
    DisplayText,			/* DisplayText */
    InsertCursor,			/* InsertCursor */
    XtInheritClearToBackground,		/* ClearToBackground */
    FindPosition,			/* FindPosition */
    FindDistance,			/* FindDistance */
    Resolve,				/* Resolve */
    MaxLines,				/* MaxLines */
    MaxHeight,				/* MaxHeight */
    SetTabs,				/* SetTabs */
    GetCursorBounds,			/* GetCursorBounds */
  },
  {
    /* multi_sink_class fields */
    0,					/* unused */
  }
};

WidgetClass multiSinkObjectClass = (WidgetClass)&multiSinkClassRec;

/* Utilities */
static int
CharWidth(Widget w, int x, wchar_t c)
{
  int i, width;
  MultiSinkObject sink = (MultiSinkObject)w;
  XFontSet fontset = sink->multi_sink.fontset;
  Position *tab;

  if (c == _Xaw_atowc(XawLF))
    return (0);

  if (c == _Xaw_atowc(XawTAB))
    {
      /* Adjust for Left Margin. */
      x -= ((TextWidget) XtParent(w))->text.margin.left;

      if (x >= (int)XtParent(w)->core.width)
	return 0;
      for (i = 0, tab = sink->text_sink.tabs ; 
	   i < sink->text_sink.tab_count ; i++, tab++)
	{
	  if (x < *tab)
	    {
	      if (*tab < (int)XtParent(w)->core.width)
		return (*tab - x);
	      else
		return (0);
	    }
	}
	return (0);
    }

  if (XwcTextEscapement (fontset, &c, 1) == 0)
    {
      if (sink->multi_sink.display_nonprinting)
	c = _Xaw_atowc('@');
      else
	c = _Xaw_atowc(XawSP);
    }

    /*
     * if more efficiency(suppose one column is one ASCII char)

    width = XwcGetColumn(fontset->font_charset, fontset->num_of_fonts, c) *
            fontset->font_struct_list[0]->min_bounds.width;
     *
     * WARNING: Very Slower!!!
     *
     * Li Yuhong.
     */

  width = XwcTextEscapement(fontset, &c, 1);

  return (width);
}

/*	Function Name: PaintText
 *	Description: Actually paints the text into the windoe.
 *	Arguments: w - the text widget.
 *		   gc - gc to paint text with.
 *		   x, y - location to paint the text.
 *		   buf, len - buffer and length of text to paint.
 *	Returns: the width of the text painted, or 0.
 *
 * NOTE:  If this string attempts to paint past the end of the window
 *        then this function will return zero.
 */
static unsigned int
PaintText(Widget w, GC gc, int x, int y, wchar_t *buf, int len)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  TextWidget ctx = (TextWidget)XtParent(w);
  XFontSet fontset = sink->multi_sink.fontset;
  int max_x;
  unsigned int width = XwcTextEscapement(fontset, buf, len);
  XFontSetExtents *ext = XExtentsOfFontSet(fontset);

  max_x = XtWidth(ctx);

  if (((int)width) <= -x)		/* Don't draw if we can't see it. */
    return (width);

  if (ctx->core.background_pixmap != XtUnspecifiedPixmap
      && gc == sink->multi_sink.normgc)
    {
      XwcTextItem item;

      item.chars = buf;
      item.nchars = len;
      item.delta = 0;
      item.font_set = fontset;
      XawTextSinkClearToBackground(w, x, y - abs(ext->max_logical_extent.y),
				   width, ext->max_logical_extent.height);
      XwcDrawText(XtDisplay(ctx), XtWindow(ctx), gc, x, y, &item, 1);
    }
  else
    XwcDrawImageString(XtDisplay(ctx), XtWindow(ctx), fontset, gc,
		       x, y, buf, len);
  if ((int)width + x > max_x && ctx->text.margin.right != 0)
    return (0);

  return (width);
}

/* Sink Object Functions */
/*
 * This function does not know about drawing more than one line of text.
 */
static void
DisplayText(Widget w, int x, int y,
	    XawTextPosition pos1, XawTextPosition pos2, Bool highlight)
{
  TextWidget ctx = (TextWidget)XtParent(w);
  MultiSinkObject sink = (MultiSinkObject)w;
  Widget source = XawTextGetSource(XtParent(w));
  wchar_t buf[BUFSIZ];
  XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);
  int j, k;
  XawTextBlock blk;
  GC gc, invgc, tabgc;

  if (!sink->multi_sink.echo || !ctx->text.lt.lines)
    return;

  gc = highlight ? sink->multi_sink.invgc : sink->multi_sink.normgc;
  invgc = highlight ? sink->multi_sink.normgc : sink->multi_sink.invgc;

  if (highlight && sink->multi_sink.xorgc)
    tabgc = sink->multi_sink.xorgc;
  else
    tabgc = invgc;

  y += abs(ext->max_logical_extent.y);
  for (j = 0; pos1 < pos2;)
    {
      pos1 = XawTextSourceRead(source, pos1, &blk, (int) pos2 - pos1);
      for (k = 0; k < blk.length; k++)
	{
	  if (j >= BUFSIZ)		/* buffer full, dump the text. */
	    {
	      x += PaintText(w, gc, x, y, buf, j);
	      j = 0;
	    }
	  buf[j] = ((wchar_t *)blk.ptr)[k];
	  if (buf[j] == _Xaw_atowc(XawLF))
	    continue;

	  else if (buf[j] == _Xaw_atowc(XawTAB))
	    {
	      Position temp = 0;
	      Dimension width;

	      if (j != 0 && (temp = PaintText(w, gc, x, y, buf, j)) == 0)
		return;

	      x += temp;
	      width = CharWidth(w, x, _Xaw_atowc(XawTAB));
	      if (!highlight
		  && ctx->core.background_pixmap != XtUnspecifiedPixmap)
		XawTextSinkClearToBackground
		  (w,
		   x, y - abs(ext->max_logical_extent.y),
		   width, ext->max_logical_extent.height);
	      else
		XFillRectangle(XtDisplayOfObject(w), XtWindowOfObject(w),
			       tabgc, x,
                               y - abs(ext->max_logical_extent.y),
                               width,
                               ext->max_logical_extent.height);
	      x += width;
	      j = -1;
            }
	  else if (XwcTextEscapement(sink->multi_sink.fontset, &buf[j], 1)
		   == 0)
	    {
	      if (sink->multi_sink.display_nonprinting)
		buf[j] = _Xaw_atowc('@');
	      else
		buf[j] = _Xaw_atowc(' ');
            }
	  j++;
	}
    }

  if (j > 0)
    (void)PaintText(w, gc, x, y, buf, j);
}

/*	Function Name: GetCursorBounds
 *	Description: Returns the size and location of the cursor.
 *	Arguments: w - the text object.
 * RETURNED	   rect - an X rectangle to return the cursor bounds in.
 *	Returns: none.
 */
static void
GetCursorBounds(Widget w, XRectangle *rect)
{
  MultiSinkObject sink = (MultiSinkObject)w;

  rect->width = CharWidth(w, 0, _Xaw_atowc(' '));
  rect->height = (XExtentsOfFontSet(sink->multi_sink.fontset)
		  ->max_logical_extent.height);
  rect->x = sink->multi_sink.cursor_x;
  rect->y = sink->multi_sink.cursor_y - (short)rect->height;
}

/*
 * The following procedure manages the "insert" cursor.
 */
static void
InsertCursor(Widget w, int x, int y, XawTextInsertState state)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  Widget ctx = XtParent(w);
  XawTextPosition position = XawTextGetInsertionPoint(ctx);

  if (XtIsRealized(ctx))
    {
      int fheight, fdiff;
      XawTextBlock block;
      wchar_t c;
      XawTextPosition selection_start, selection_end;
      Boolean has_selection;
      XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);

      XawTextGetSelectionPos((Widget)ctx, &selection_start, &selection_end);
      has_selection = selection_start != selection_end;

      fheight = ext->max_logical_extent.height;
      fdiff = fheight - abs(ext->max_logical_extent.y);

      if (sink->multi_sink.laststate != XawisOff)
	{
	  wchar_t *ochar;

	  (void)XawTextSourceRead(XawTextGetSource(ctx),
				  sink->multi_sink.cursor_position,
				  &block, 1);
	  if (!block.length)
	    ochar = NULL;
	  else
	    {
	      c = ((wchar_t *)block.ptr)[0];
	      if (c == _Xaw_atowc(XawLF))
		ochar = NULL;
	      else if (c == _Xaw_atowc(XawTAB))
		ochar = wspace;
	      else
		ochar = (wchar_t *)block.ptr;
	    }

	  if (!ochar)
	    XawTextSinkClearToBackground(w,
					 sink->multi_sink.cursor_x,
					 (sink->multi_sink.cursor_y - 1
					  - fheight),
					 CharWidth(w, 0, wspace[0]), fheight);
	  else
	    PaintText(w,
		      sink->multi_sink.normgc,
		      sink->multi_sink.cursor_x,
		      (sink->multi_sink.cursor_y - 1 - fdiff),
		      ochar, 1);
	}

      if (!has_selection && state != XawisOff)
	{
	  wchar_t *nchar;
	  Boolean focus = ((TextWidget)ctx)->text.hasfocus;

	  (void)XawTextSourceRead(XawTextGetSource(ctx),
				  position, &block, 1);
	  c = ((wchar_t *)block.ptr)[0];
	  if (!block.length || c == _Xaw_atowc(XawLF)
	      || c == _Xaw_atowc(XawTAB))
	      nchar = wspace;
	    else
	      nchar = (wchar_t *)block.ptr;

	  if (focus)
	    XwcDrawImageString(XtDisplay(ctx), XtWindow(ctx),
			       sink->multi_sink.fontset,
			       sink->multi_sink.invgc,
			       x, (y - 1 - fdiff),
			       nchar, 1);
	  else
	    XDrawRectangle(XtDisplay(ctx), XtWindow(ctx),
			   sink->multi_sink.xorgc ?
			   sink->multi_sink.xorgc : sink->multi_sink.normgc,
			   x, y - 1 - fheight,
			   CharWidth(w, 0, *nchar) - 1, fheight - 1);
	}
    }

  sink->multi_sink.cursor_x = x;
  sink->multi_sink.cursor_y = y;
  sink->multi_sink.laststate = state;
  sink->multi_sink.cursor_position = position;
}

/*
 * Given two positions, find the distance between them.
 */
static void
FindDistance(Widget w, XawTextPosition fromPos, int fromx,
	     XawTextPosition toPos, int *resWidth,
	     XawTextPosition *resPos, int *resHeight)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  Widget source = XawTextGetSource(XtParent(w));
  XawTextPosition index, lastPos;
  wchar_t c;
  XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);
  XawTextBlock blk;

  /* we may not need this */
  lastPos = GETLASTPOS;
  XawTextSourceRead(source, fromPos, &blk, (int)toPos - fromPos);
  *resWidth = 0;
  for (index = fromPos; index != toPos && index < lastPos; index++)
    {
      if (index - blk.firstPos >= blk.length)
	XawTextSourceRead(source, index, &blk, (int) toPos - fromPos);
      c = ((wchar_t *)blk.ptr)[index - blk.firstPos];
      *resWidth += CharWidth(w, fromx + *resWidth, c);
      if (c == _Xaw_atowc(XawLF))
	{
	  index++;
	  break;
	}
    }
  *resPos = index;
  *resHeight = ext->max_logical_extent.height;
}

static void
FindPosition(Widget w, XawTextPosition fromPos, int fromx, int width,
	     Bool stopAtWordBreak, XawTextPosition *resPos, int *resWidth,
	     int *resHeight)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  Widget source = XawTextGetSource(XtParent(w));

  XawTextPosition lastPos, index, whiteSpacePosition = 0;
  int lastWidth = 0, whiteSpaceWidth = 0;
  Boolean whiteSpaceSeen;
  wchar_t c;
  XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);
  XawTextBlock blk;

  lastPos = GETLASTPOS;

  XawTextSourceRead(source, fromPos, &blk, BUFSIZ);
  *resWidth = 0;
  whiteSpaceSeen = FALSE;
  c = 0;
  for (index = fromPos; *resWidth <= width && index < lastPos; index++)
    {
      lastWidth = *resWidth;
      if (index - blk.firstPos >= blk.length)
	XawTextSourceRead(source, index, &blk, BUFSIZ);
      c = ((wchar_t *)blk.ptr)[index - blk.firstPos];
      *resWidth += CharWidth(w, fromx + *resWidth, c);

      if ((c == _Xaw_atowc(XawSP) || c == _Xaw_atowc(XawTAB))
	  && *resWidth <= width)
	{
	  whiteSpaceSeen = TRUE;
	  whiteSpacePosition = index;
	  whiteSpaceWidth = *resWidth;
	}
      if (c == _Xaw_atowc(XawLF))
	{
	  index++;
	  break;
	}
    }
  if (*resWidth > width && index > fromPos)
    {
      *resWidth = lastWidth;
      index--;
      if (stopAtWordBreak && whiteSpaceSeen)
	{
	  index = whiteSpacePosition + 1;
	  *resWidth = whiteSpaceWidth;
	}
    }
  if (index == lastPos && c != _Xaw_atowc(XawLF)) index = lastPos + 1;
  *resPos = index;
  *resHeight = ext->max_logical_extent.height;
}

static void
Resolve(Widget w, XawTextPosition pos, int fromx, int width,
	XawTextPosition *pos_return)
{
  int resWidth, resHeight;
  Widget source = XawTextGetSource(XtParent(w));

  FindPosition(w, pos, fromx, width, FALSE, pos_return, &resWidth, &resHeight);
  if (*pos_return > GETLASTPOS)
    *pos_return = GETLASTPOS;
}

static void
GetGC(MultiSinkObject sink)
{
  XtGCMask valuemask = (GCClipXOrigin |
			GCGraphicsExposures | GCForeground | GCBackground );
  XGCValues values;

  /* XXX We dont want do share a gc that will change the clip-mask */
  values.clip_x_origin = (int)sink;
  values.clip_mask = None;
  values.graphics_exposures = False;
    
  values.foreground = sink->text_sink.foreground;
  values.background = sink->text_sink.background;

  sink->multi_sink.normgc = XtAllocateGC((Widget)sink, 0, valuemask, &values,
					 GCFont | GCClipMask, 0);

  values.foreground = sink->text_sink.background;
  values.background = sink->text_sink.cursor_color;
  sink->multi_sink.invgc = XtAllocateGC((Widget)sink, 0, valuemask, &values,
					GCFont | GCClipMask, 0);
  if (sink->text_sink.cursor_color != sink->text_sink.foreground)
    {
      values.foreground = sink->text_sink.cursor_color;
      values.background = sink->text_sink.foreground;
      sink->multi_sink.xorgc = XtAllocateGC((Widget)sink, 0, valuemask,
					    &values, GCFont | GCClipMask, 0);
    }
  else
    sink->multi_sink.xorgc = NULL;

  XawMultiSinkResize((Widget)sink);
}

/***** Public routines *****/
static void
XawMultiSinkClassInitialize(void)
{
  wspace[0] = _Xaw_atowc(' ');
  XawInitializeWidgetSet();
}

/*	Function Name: XawMultiSinkInitialize
 *	Description: Initializes the TextSink Object.
 *	Arguments: request, c_new - the requested and new values for the object
 *				    instance.
 *	Returns: none.
 *
 */
/* ARGSUSED */
static void
XawMultiSinkInitialize(Widget request, Widget c_new,
		       ArgList args, Cardinal *num_args)
{
  MultiSinkObject sink = (MultiSinkObject)c_new;

  GetGC(sink);

  sink->multi_sink.cursor_position = 0;
  sink->multi_sink.laststate = XawisOff;
  sink->multi_sink.cursor_x = sink->multi_sink.cursor_y = 0;
}

/*	Function Name: XawMultiSinkDestroy
 *	Description: This function cleans up when the object is 
 *		     destroyed.
 *	Arguments: w - the MultiSink Object.
 *	Returns: none.
 */
static void
XawMultiSinkDestroy(Widget w)
{
  MultiSinkObject sink = (MultiSinkObject)w;

  XtReleaseGC(w, sink->multi_sink.normgc);
  XtReleaseGC(w, sink->multi_sink.invgc);
  if (sink->multi_sink.xorgc)
    XtReleaseGC(w, sink->multi_sink.xorgc);
  sink->multi_sink.normgc =
    sink->multi_sink.invgc =
    sink->multi_sink.xorgc = NULL;
}

static void
XawMultiSinkResize(Widget w)
{
  TextWidget ctx = (TextWidget)XtParent(w);
  MultiSinkObject sink = (MultiSinkObject)w;
  XRectangle rect;
  int width, height;

  rect.x = ctx->text.r_margin.left;
  rect.y = ctx->text.r_margin.top;
  width = (int)XtWidth(ctx) -
    (int)ctx->text.r_margin.right - (int)ctx->text.r_margin.left;
  height = (int)XtHeight(ctx) -
    (int)ctx->text.r_margin.top - (int)ctx->text.r_margin.bottom;
  rect.width = width;
  rect.height = height;

  if (sink->multi_sink.normgc)
    {
      if (width >= 0 && height >= 0)
	XSetClipRectangles(XtDisplay((Widget)ctx), sink->multi_sink.normgc,
			   0, 0, &rect, 1, Unsorted);
      else
	XSetClipMask(XtDisplay((Widget)ctx), sink->multi_sink.normgc, None);
    }
  if (sink->multi_sink.invgc)
    {
      if (width >= 0 && height >= 0)
	XSetClipRectangles(XtDisplay((Widget)ctx), sink->multi_sink.invgc,
			   0, 0, &rect, 1, Unsorted);
      else
	XSetClipMask(XtDisplay((Widget)ctx), sink->multi_sink.invgc, None);
    }
  if (sink->multi_sink.xorgc)
    {
      if (width >= 0 && height >= 0)
	XSetClipRectangles(XtDisplay((Widget)ctx), sink->multi_sink.xorgc,
			   0, 0, &rect, 1, Unsorted);
      else
	XSetClipMask(XtDisplay((Widget)ctx), sink->multi_sink.xorgc, None);
    }
}

/*	Function Name: XawMultiSinkSetValues
 *	Description: Sets the values for the MultiSink
 *	Arguments: current - current state of the object.
 *		   request - what was requested.
 *		   c_new - what the object will become.
 *	Returns: True if redisplay is needed.
 */
/* ARGSUSED */
static Boolean
XawMultiSinkSetValues(Widget current, Widget request, Widget c_new,
		      ArgList args, Cardinal *num_args)
{
  MultiSinkObject w = (MultiSinkObject)c_new;
  MultiSinkObject old_w = (MultiSinkObject)current;

  /* Font set is not in the GC! Do not make a new GC when font set changes! */

  if (w->multi_sink.fontset != old_w->multi_sink.fontset)
    {
      ((TextWidget)XtParent(c_new))->text.redisplay_needed = True;
#ifndef NO_TAB_FIX
      SetTabs((Widget)w, w->text_sink.tab_count, w->text_sink.char_tabs);
#endif
    }

  if (w->text_sink.background != old_w->text_sink.background
      || w->text_sink.foreground != old_w->text_sink.foreground
      || w->text_sink.cursor_color != old_w->text_sink.cursor_color)
    {
      XtReleaseGC(c_new, w->multi_sink.normgc);
      XtReleaseGC(c_new, w->multi_sink.invgc);
      if (w->multi_sink.xorgc)
	XtReleaseGC(c_new, w->multi_sink.xorgc);
      GetGC(w);
      ((TextWidget)XtParent(c_new))->text.redisplay_needed = True;
    }
  else if ((w->multi_sink.echo != old_w->multi_sink.echo)
	   || (w->multi_sink.display_nonprinting
	       != old_w->multi_sink.display_nonprinting) )
    ((TextWidget)XtParent(c_new))->text.redisplay_needed = True;

  return (False);
}

/*	Function Name: MaxLines
 *	Description: Finds the Maximum number of lines that will fit in
 *		     a given height.
 *	Arguments: w - the MultiSink Object.
 *		   height - height to fit lines into.
 *	Returns: the number of lines that will fit.
 */
/* ARGSUSED */
static int
MaxLines(Widget w, unsigned int height)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  int font_height;
  XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);

  font_height = ext->max_logical_extent.height;
  return (((int)height) / font_height);
}

/*	Function Name: MaxHeight
 *	Description: Finds the Minium height that will contain a given number 
 *		     lines.
 *	Arguments: w - the MultiSink Object.
 *		   lines - the number of lines.
 *	Returns: the height.
 */
/* ARGSUSED */
static int
MaxHeight(Widget w, int lines)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);

  return (lines * ext->max_logical_extent.height);
}

/*	Function Name: SetTabs
 *	Description: Sets the Tab stops.
 *	Arguments: w - the MultiSink Object.
 *		   tab_count - the number of tabs in the list.
 *		   tabs - the text positions of the tabs.
 *	Returns: none
 */
static void
SetTabs(Widget w, int tab_count, short* tabs)
{
  MultiSinkObject sink = (MultiSinkObject)w;
  int i;
  Atom XA_FIGURE_WIDTH;
  unsigned long figure_width = 0;
  XFontStruct *font;

  /*
   * Bug:
   *   Suppose the first font of fontset stores the unit of column.
   *
   * By Li Yuhong, Mar. 14, 1991
   */
  {
    XFontStruct **f_list;
    char	**f_name;

    (void)XFontsOfFontSet(sink->multi_sink.fontset, &f_list, &f_name);
    font = f_list[0];
  }

  /*
   * Find the figure width of the current font.
   */
  XA_FIGURE_WIDTH = XInternAtom(XtDisplayOfObject(w), "FIGURE_WIDTH", FALSE);
  if ((XA_FIGURE_WIDTH != None)
      && ((!XGetFontProperty(font, XA_FIGURE_WIDTH, &figure_width))
	  || (figure_width == 0)))
    {
      if (font->per_char && font->min_char_or_byte2 <= '$'
	  && font->max_char_or_byte2 >= '$')
	figure_width = font->per_char['$' - font->min_char_or_byte2].width;
      else
	figure_width = font->max_bounds.width;
    }

  if (tab_count > sink->text_sink.tab_count)
    {
      sink->text_sink.tabs = (Position *)
	XtRealloc((char *)sink->text_sink.tabs,
		  (Cardinal)(tab_count * sizeof(Position)));
      sink->text_sink.char_tabs = (short *)
	XtRealloc((char *)sink->text_sink.char_tabs,
		  (Cardinal)(tab_count * sizeof(short)));
    }

  for ( i = 0 ; i < tab_count ; i++ )
    {
      sink->text_sink.tabs[i] = tabs[i] * figure_width;
      sink->text_sink.char_tabs[i] = tabs[i];
    }

  sink->text_sink.tab_count = tab_count;

#ifndef NO_TAB_FIX
  ((TextWidget)XtParent(w))->text.redisplay_needed = True;
#endif
}

void
_XawMultiSinkPosToXY(Widget w, XawTextPosition pos, Position *x, Position *y)
{
  MultiSinkObject sink = (MultiSinkObject)((TextWidget)w)->text.sink;
  XFontSetExtents *ext = XExtentsOfFontSet(sink->multi_sink.fontset);

  _XawTextPosToXY(w, pos, x, y);
  *y += abs(ext->max_logical_extent.y);
}
