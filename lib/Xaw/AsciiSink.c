/* $XConsortium: AsciiSink.c,v 1.62 94/04/17 20:11:41 kaleb Exp $ */

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
/* $XFree86: xc/lib/Xaw/AsciiSink.c,v 1.9 1998/08/20 13:58:54 dawes Exp $ */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/AsciiSinkP.h>
#include <X11/Xaw/AsciiSrcP.h>
#include <X11/Xaw/TextP.h>
#include "Private.h"

#ifdef GETLASTPOS
#undef GETLASTPOS		/* We will use our own GETLASTPOS */
#endif

#define GETLASTPOS	\
	XawTextSourceScan(source, 0, XawstAll, XawsdRight, 1, True)

/*
 * Class Methods
 */
static void XawAsciiSinkInitialize(Widget, Widget, ArgList, Cardinal*);
static void XawAsciiSinkDestroy(Widget);
static void XawAsciiSinkResize(Widget);
static Boolean XawAsciiSinkSetValues(Widget, Widget, Widget,
				     ArgList, Cardinal*);
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

/*
 * Prototypes
 */
static void GetGC(AsciiSinkObject);
static int CharWidth(Widget, int, unsigned int);
static unsigned int PaintText(Widget w, GC gc, int x, int y,
			      unsigned char *buf, int len);

/*
 * Initialization
 */
#define offset(field) XtOffsetOf(AsciiSinkRec, ascii_sink.field)
static XtResource resources[] = {
  {
    XtNfont,
    XtCFont,
    XtRFontStruct,
    sizeof(XFontStruct*),
    offset(font),
    XtRString,
    XtDefaultFont
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
    (XtPointer)
    True
  },
};
#undef offset

#define Superclass	(&textSinkClassRec)
AsciiSinkClassRec asciiSinkClassRec = {
  /* object */
  {
    (WidgetClass)Superclass,		/* superclass */
    "AsciiSink",		/* class_name */
    sizeof(AsciiSinkRec),	/* widget_size */
    XawInitializeWidgetSet,	/* class_initialize */
    NULL,			/* class_part_initialize */
    False,				/* class_inited	*/
    XawAsciiSinkInitialize,	/* initialize */
    NULL,			/* initialize_hook */
    NULL,			/* obj1 */
    NULL,			/* obj2 */
    0,				/* obj3 */
    resources,			/* resources */
    XtNumber(resources),	/* num_resources */
    NULLQUARK,			/* xrm_class */
    False,				/* obj4 */
    False,				/* obj5 */
    False,				/* obj6 */
    False,				/* obj7 */
    XawAsciiSinkDestroy,	/* destroy */
    (XtProc)XawAsciiSinkResize,	/* obj8 */
    NULL,			/* obj9 */
    XawAsciiSinkSetValues,	/* set_values */
    NULL,			/* set_values_hook */
    NULL,			/* obj10 */
    NULL,			/* get_values_hook */
    NULL,			/* obj11 */
    XtVersion,			/* version */
    NULL,			/* callback_private */
    NULL,			/* obj12 */
    NULL,			/* obj13 */
    NULL,			/* obj14 */
    NULL,			/* extension */
  },
  /* text_sink */
  {
    DisplayText,		/* DisplayText */
    InsertCursor,		/* InsertCursor */
    XtInheritClearToBackground,	/* ClearToBackground */
    FindPosition,		/* FindPosition */
    FindDistance,		/* FindDistance */
    Resolve,			/* Resolve */
    MaxLines,			/* MaxLines */
    MaxHeight,			/* MaxHeight */
    SetTabs,			/* SetTabs */
    GetCursorBounds,		/* GetCursorBounds */
  },
  /* ascii_sink */
  {
    NULL,				/* extension */
  }
};

WidgetClass asciiSinkObjectClass = (WidgetClass)&asciiSinkClassRec;

/*
 * Implementation
 */
static int
CharWidth(Widget w, int x, unsigned int c)
{
  int i, width, nonPrinting;
  AsciiSinkObject sink = (AsciiSinkObject)w;
  XFontStruct *font = sink->ascii_sink.font;
  Position *tab;

  if (c == XawLF)
    return (0);

  if (c == XawTAB)
    {
      width = x;
      /* Adjust for Left Margin */
      x -= ((TextWidget)XtParent(w))->text.margin.left;

      i = 0;
      tab = sink->text_sink.tabs;
      /*CONSTCOND*/
      while (1)
	{
	  if (x < *tab)
		return (*tab - x);
	  /* Start again */
	  if (++i >= sink->text_sink.tab_count)
	    {
	      x -= *tab;
	      i = 0;
	      tab = sink->text_sink.tabs;
	      if (width == x)
		return (0);
	    }
	  else
	    ++tab;
	}
      /*NOTREACHED*/
    }

  if ((nonPrinting = (c < (unsigned char)XawSP)) != False)
    {
      if (sink->ascii_sink.display_nonprinting)
	c += '@';
      else
	{
	  c = XawSP;
	  nonPrinting = False;
	}
    }

  if (font->per_char
      && (c >= font->min_char_or_byte2 && c <= font->max_char_or_byte2))
    width = font->per_char[c - font->min_char_or_byte2].width;
  else
    width = font->min_bounds.width;

  if (nonPrinting)
    width += CharWidth(w, x, '^');

  return (width);
}

/*
 * Function:
 *	PaintText
 *
 * Parameters:
 *	w   - text sink object
 *	gc  - gc to paint text with
 *	x   - location to paint the text
 *	y   - ""
 *	buf - buffer and length of text to paint.
 *	len - ""
 *
 * Description:
 *	Actually paints the text into the window.
 *
 * Returns:
 *	the width of the text painted, or 0
 */
static unsigned int
PaintText(Widget w, GC gc, int x, int y, unsigned char *buf, int len)
{
  int max_x;
  AsciiSinkObject sink = (AsciiSinkObject)w;
  TextWidget ctx = (TextWidget)XtParent(w);
  unsigned int width = XTextWidth(sink->ascii_sink.font, (char *)buf, len);

  max_x = XtWidth(ctx);

  if ((int)width <= -x)		/* Don't draw if we can't see it */
    return (width);

  if (ctx->core.background_pixmap != XtUnspecifiedPixmap
      && gc == sink->ascii_sink.normgc)
    {
      XTextItem item;

      item.chars = (char *)buf;
      item.nchars = len;
      item.delta = 0;
      item.font = sink->ascii_sink.font->fid;

      XawTextSinkClearToBackground(w, x, y - sink->ascii_sink.font->ascent,
				   width, sink->ascii_sink.font->ascent
				   + sink->ascii_sink.font->descent);
      XDrawText(XtDisplay(ctx), XtWindow(ctx), gc, x, y, &item, 1);
    }
  else
    XDrawImageString(XtDisplay(ctx), XtWindow(ctx), gc,
		     x, y, (char *)buf, len);

  if ((int)width + x > max_x && ctx->text.margin.right != 0)
    return (0);

  return (width);
}

static void
DisplayText(Widget w, int x, int y,
	    XawTextPosition pos1, XawTextPosition pos2, Bool highlight)
{
  TextWidget ctx = (TextWidget)XtParent(w);
  AsciiSinkObject sink = (AsciiSinkObject)w;
  Widget source = XawTextGetSource(XtParent(w));
  unsigned char buf[BUFSIZ];
  int j, k;
  XawTextBlock blk;
  GC gc, invgc, tabgc;

  if (!sink->ascii_sink.echo || !ctx->text.lt.lines)
    return;

  gc = highlight ? sink->ascii_sink.invgc : sink->ascii_sink.normgc;
  invgc = highlight ? sink->ascii_sink.normgc : sink->ascii_sink.invgc;

  if (highlight && sink->ascii_sink.xorgc)
    tabgc = sink->ascii_sink.xorgc;
  else
    tabgc = invgc;

  y += sink->ascii_sink.font->ascent;
  for (j = 0; pos1 < pos2;)
    {
      pos1 = XawTextSourceRead(source, pos1, &blk, pos2 - pos1);
      for (k = 0; k < blk.length; k++)
	{
	  if (j >= BUFSIZ)		/* buffer full, dump the text */
	    {
	      x += PaintText(w, gc, x, y, buf, j);
	      j = 0;
	    }
	  buf[j] = blk.ptr[k];
	  if (buf[j] == XawLF)		/* line feeds ('\n') are not printed */
	    continue;

	  else if (buf[j] == '\t')
	    {
	      Position temp = 0;
	      Dimension width;

	      if (j != 0 && (temp = PaintText(w, gc, x, y, buf, j)) == 0)
		return;

	      x += temp;
	      width = CharWidth(w, x, '\t');
	      if (!highlight
		  && ctx->core.background_pixmap != XtUnspecifiedPixmap)
		XawTextSinkClearToBackground(w, x,
		   y - sink->ascii_sink.font->ascent,
		   width,
					     sink->ascii_sink.font->ascent
					     + sink->ascii_sink.font->descent);
	      else
		XFillRectangle(XtDisplayOfObject(w), XtWindowOfObject(w),
			       tabgc, x, y - sink->ascii_sink.font->ascent,
			       width, sink->ascii_sink.font->ascent
			       + sink->ascii_sink.font->descent);
	      x += width;
	      j = -1;
	    }
	  else if (buf[j] < ' ')
	    {
	      if (sink->ascii_sink.display_nonprinting)
		{
		  buf[j + 1] = buf[j] + '@';
		  buf[j] = '^';
		  j++;
		}
	      else
		buf[j] = ' ';
	    }
	  j++;
	}
    }

  if (j > 0)
    (void)PaintText(w, gc, x, y, buf, j);
}

/*
 * Function:
 *	GetCursorBounds
 *
 * Parameters:
 *	w - text sink object
 *	rect - X rectangle to return the cursor bounds
 *
 * Description:
 *	Returns the size and location of the cursor.
 */
static void
GetCursorBounds(Widget w, XRectangle *rect)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;

  rect->width = CharWidth(w, 0, ' ');
  rect->height = (sink->ascii_sink.font->descent
		  + sink->ascii_sink.font->ascent);

  rect->x = sink->ascii_sink.cursor_x;
  rect->y = sink->ascii_sink.cursor_y - (short)rect->height;
}

static void
InsertCursor(Widget w, int x, int y, XawTextInsertState state)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;
  TextWidget ctx = (TextWidget)XtParent(w);
  XawTextPosition position = XawTextGetInsertionPoint((Widget)ctx);

  if (XtIsRealized((Widget)ctx))
    {
      int fheight;
      XawTextBlock block;
      XawTextPosition selection_start, selection_end;
      Boolean has_selection;

      XawTextGetSelectionPos((Widget)ctx, &selection_start, &selection_end);
      has_selection = selection_start != selection_end;

      fheight = sink->ascii_sink.font->ascent + sink->ascii_sink.font->descent;

      if ((sink->ascii_sink.cursor_position != position || state == XawisOff)
	  && !has_selection && sink->ascii_sink.laststate != XawisOff)
	{
	  char *ochar;

	  (void)XawTextSourceRead(XawTextGetSource((Widget)ctx),
				  sink->ascii_sink.cursor_position, &block, 1);
	  if (!block.length || block.ptr[0] == '\n')
	    ochar = NULL;
	  else if (block.ptr[0] == '\t')
	    ochar = " ";
	  else if (block.ptr[0] < XawSP)
	    {
	      if (sink->ascii_sink.display_nonprinting)
		ochar = "^";
	      else
		ochar = " ";
	    }
	  else
	    ochar = block.ptr;

	  if (!ochar)
	    XawTextSinkClearToBackground(w, sink->ascii_sink.cursor_x,
					 sink->ascii_sink.cursor_y
					 - 1 - fheight,
					 CharWidth(w, 0, ' '), fheight);
	  else
	    PaintText(w,  sink->ascii_sink.normgc, sink->ascii_sink.cursor_x,
		      sink->ascii_sink.cursor_y
		      - 1 - sink->ascii_sink.font->descent,
		      (unsigned char *)ochar, 1);
	}

      if (!has_selection && state != XawisOff)
	{
	  char *nchar;
	  Boolean focus = ctx->text.hasfocus;

	  (void)XawTextSourceRead(XawTextGetSource((Widget)ctx),
				  position, &block, 1);
	  if (!block.length || block.ptr[0] == '\n' || block.ptr[0] == '\t')
	    nchar = " ";
	  else if (block.ptr[0] < XawSP)
	    {
	      if (sink->ascii_sink.display_nonprinting)
		nchar = "^";
	      else
		nchar = " ";
	    }
	  else
	    nchar = block.ptr;

	  if (focus)
	    XDrawImageString(XtDisplay((Widget)ctx), XtWindow((Widget)ctx),
			     sink->ascii_sink.invgc,
			     x, y - 1 - sink->ascii_sink.font->descent,
			     nchar, 1);
	  else
	    XDrawRectangle(XtDisplay((Widget)ctx), XtWindow((Widget)ctx),
			   sink->ascii_sink.xorgc ? 
			   sink->ascii_sink.xorgc : sink->ascii_sink.normgc,
			   x, y - 1 - fheight,
			   CharWidth(w, 0, *nchar) - 1, fheight - 1);
	}
    }

  sink->ascii_sink.cursor_x = x;
  sink->ascii_sink.cursor_y = y;
  sink->ascii_sink.laststate = state;
  sink->ascii_sink.cursor_position = position;
}

/*
 * Given two positions, find the distance between them
 */
static void
FindDistance(Widget w, XawTextPosition fromPos, int fromx,
	     XawTextPosition toPos, int *resWidth,
	     XawTextPosition *resPos, int *resHeight)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;
  Widget source = XawTextGetSource(XtParent(w));
  XawTextPosition idx, lastPos;
  unsigned char c;
  XawTextBlock blk;

  /* we may not need this */
  lastPos = GETLASTPOS;
  XawTextSourceRead(source, fromPos, &blk, toPos - fromPos);
  *resWidth = 0;
  for (idx = fromPos; idx != toPos && idx < lastPos; idx++)
    {
      if (idx - blk.firstPos >= blk.length)
	XawTextSourceRead(source, idx, &blk, toPos - fromPos);
      c = blk.ptr[idx - blk.firstPos];
      *resWidth += CharWidth(w, fromx + *resWidth, c);
      if (c == XawLF)
	{
	  idx++;
	  break;
	}
    }
  *resPos = idx;
  *resHeight = (sink->ascii_sink.font->ascent
		+ sink->ascii_sink.font->descent);
}

static void
FindPosition(Widget w, XawTextPosition fromPos, int fromx, int width,
	     Bool stopAtWordBreak, XawTextPosition *resPos,
	     int *resWidth, int *resHeight)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;
  Widget source = XawTextGetSource(XtParent(w));
  XawTextPosition lastPos, idx, whiteSpacePosition = 0;
  int lastWidth, whiteSpaceWidth;
  Boolean whiteSpaceSeen;
  unsigned char c;
  XawTextBlock blk;

  lastPos = GETLASTPOS;

  XawTextSourceRead(source, fromPos, &blk, BUFSIZ);
  *resWidth = lastWidth = whiteSpaceWidth = 0;
  whiteSpaceSeen = False;
  c = 0;

  for (idx = fromPos; *resWidth <= width && idx < lastPos; idx++)
    {
      lastWidth = *resWidth;
      if (idx - blk.firstPos >= blk.length)
	XawTextSourceRead(source, idx, &blk, BUFSIZ);
      c = blk.ptr[idx - blk.firstPos];
      *resWidth += CharWidth(w, fromx + *resWidth, c);

      if (c == XawLF)
	{
	  idx++;
	  break;
	}
      else if ((c == XawSP || c == XawTAB) && *resWidth <= width)
	{
	  whiteSpaceSeen = True;
	  whiteSpacePosition = idx;
	  whiteSpaceWidth = *resWidth;
	}
    }
  if (*resWidth > width && idx > fromPos)
    {
      idx--;
      *resWidth = lastWidth;
      if (stopAtWordBreak && whiteSpaceSeen)
	{
	  idx = whiteSpacePosition + 1;
	  *resWidth = whiteSpaceWidth;
	}
    }
  if (idx == lastPos && c != XawLF)
    idx = lastPos + 1;

  *resPos = idx;
  *resHeight = sink->ascii_sink.font->ascent + sink->ascii_sink.font->descent;
}

static void
Resolve(Widget w, XawTextPosition pos, int fromx, int width,
	XawTextPosition *pos_return)
{
  int resWidth, resHeight;
  Widget source = XawTextGetSource(XtParent(w));

  FindPosition(w, pos, fromx, width, False, pos_return, &resWidth, &resHeight);
  if (*pos_return > GETLASTPOS)
    *pos_return = GETLASTPOS;
}

static void
GetGC(AsciiSinkObject sink)
{
  XtGCMask valuemask = (GCFont | GCClipXOrigin | GCForeground | GCBackground);
  XGCValues values;

  /* XXX We dont want do share a gc that will change the clip-mask */
  values.clip_x_origin = (int)sink;
  values.clip_mask = None;
  values.font = sink->ascii_sink.font->fid;
  values.graphics_exposures = False;

  values.foreground = sink->text_sink.foreground;
  values.background = sink->text_sink.background;
  sink->ascii_sink.normgc = XtAllocateGC((Widget)sink, 0, valuemask, &values,
					 GCClipMask, 0);

  values.foreground = sink->text_sink.background;
  values.background = sink->text_sink.cursor_color;
  sink->ascii_sink.invgc = XtAllocateGC((Widget)sink, 0, valuemask, &values,
					GCClipMask, 0);

  if (sink->text_sink.cursor_color != sink->text_sink.foreground)
    {
      values.foreground = sink->text_sink.cursor_color;
      values.background = sink->text_sink.foreground;
      sink->ascii_sink.xorgc = XtAllocateGC((Widget)sink, 0, valuemask,
					    &values, GCClipMask, 0);
    }
  else
    sink->ascii_sink.xorgc = NULL;

  XawAsciiSinkResize((Widget)sink);
}

/* Function:
 *	XawAsciiSinkInitialize
 *
 * Parameters:
 *	request - the requested and new values for the object instance
 *	cnew	- ""
 *
 * Description:
 *	Initializes the TextSink Object.
 */
/*ARGSUSED*/
static void
XawAsciiSinkInitialize(Widget request, Widget cnew,
		       ArgList args, Cardinal *num_args)
{
  AsciiSinkObject sink = (AsciiSinkObject)cnew;

  GetGC(sink);

  sink->ascii_sink.cursor_position = 0;
  sink->ascii_sink.laststate = XawisOff;
  sink->ascii_sink.cursor_x = sink->ascii_sink.cursor_y = 0;
}

/*
 * Function:
 *	XawAsciiSinkDestroy
 *
 * Parameters:
 *	w - AsciiSink Object
 *
 * Description:
 *	This function cleans up when the object is destroyed.
 */
static void
XawAsciiSinkDestroy(Widget w)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;

  XtReleaseGC(w, sink->ascii_sink.normgc);
  XtReleaseGC(w, sink->ascii_sink.invgc);
  if (sink->ascii_sink.xorgc)
    XtReleaseGC(w, sink->ascii_sink.xorgc);

  sink->ascii_sink.normgc =
    sink->ascii_sink.invgc =
    sink->ascii_sink.xorgc = NULL;
}

static void
XawAsciiSinkResize(Widget w)
{
  TextWidget ctx = (TextWidget)XtParent(w);
  AsciiSinkObject sink = (AsciiSinkObject)w;
  XRectangle rect;
  int width, height;

  rect.x = ctx->text.r_margin.left;
  rect.y = ctx->text.r_margin.top;
  width = (int)XtWidth(ctx) - RHMargins(ctx);
  height = (int)XtHeight(ctx) - RVMargins(ctx);
  rect.width = width;
  rect.height = height;

  if (sink->ascii_sink.normgc)
    {
      if (width >= 0 && height >= 0)
	XSetClipRectangles(XtDisplay((Widget)ctx), sink->ascii_sink.normgc,
			   0, 0, &rect, 1, Unsorted);
      else
	XSetClipMask(XtDisplay((Widget)ctx), sink->ascii_sink.normgc, None);
    }
  if (sink->ascii_sink.invgc)
    {
      if (width >= 0 && height >= 0)
	XSetClipRectangles(XtDisplay((Widget)ctx), sink->ascii_sink.invgc,
			   0, 0, &rect, 1, Unsorted);
      else
	XSetClipMask(XtDisplay((Widget)ctx), sink->ascii_sink.invgc, None);
    }
  if (sink->ascii_sink.xorgc)
    {
      if (width >= 0 && height >= 0)
	XSetClipRectangles(XtDisplay((Widget)ctx), sink->ascii_sink.xorgc,
			   0, 0, &rect, 1, Unsorted);
      else
	XSetClipMask(XtDisplay((Widget)ctx), sink->ascii_sink.xorgc, None);
    }
}

/*
 * Function:
 *	XawAsciiSinkSetValues
 *
 * Parameters:
 *	current - current state of the object
 *	request - what was requested
 *	cnew    - what the object will become
 *
 * Description:
 *	Sets the values for the AsciiSink.
 *
 * Returns:
 *	True if redisplay is needed
 */
/*ARGSUSED*/
static Boolean
XawAsciiSinkSetValues(Widget current, Widget request, Widget cnew,
		      ArgList args, Cardinal *num_args)
{
  AsciiSinkObject w = (AsciiSinkObject)cnew;
  AsciiSinkObject old_w = (AsciiSinkObject)current;

  if (w->ascii_sink.font != old_w->ascii_sink.font
      || w->text_sink.background != old_w->text_sink.background
      || w->text_sink.foreground != old_w->text_sink.foreground
      || w->text_sink.cursor_color != old_w->text_sink.cursor_color)
    {
      XtReleaseGC(cnew, w->ascii_sink.normgc);
      XtReleaseGC(cnew, w->ascii_sink.invgc);
      if (w->ascii_sink.xorgc)
	XtReleaseGC(cnew, w->ascii_sink.xorgc);
      GetGC(w);
      ((TextWidget)XtParent(cnew))->text.redisplay_needed = True;
    }
  else if (w->ascii_sink.echo != old_w->ascii_sink.echo
	   || w->ascii_sink.display_nonprinting
	   != old_w->ascii_sink.display_nonprinting)
    ((TextWidget)XtParent(cnew))->text.redisplay_needed = True;

  return (False);
}

/*
 * Function:
 *	MaxLines
 *
 * Parameters:
 *	w      - AsciiSink Object
 *	height - height to fit lines into
 *
 * Description:
 *	Finds the Maximum number of lines that will fit in a given height.
 *
 * Returns:
 *	The number of lines that will fit
 */
/*ARGSUSED*/
static int
MaxLines(Widget w, unsigned int height)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;
  int font_height;

  font_height = sink->ascii_sink.font->ascent + sink->ascii_sink.font->descent;

  return ((int)height / font_height);
}

/*
 * Function:
 *	MaxHeight
 *
 * Parameters:
 *	w     - AsciiSink Object
 *	lines - number of lines
 *
 * Description:
 *	Finds the Minium height that will contain a given number lines.
 *
 * Returns:
 *	the height
 */
static int
MaxHeight(Widget w, int lines)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;

  return (lines * (sink->ascii_sink.font->ascent
		   + sink->ascii_sink.font->descent));
}

/*
 * Function:
 *	SetTabs
 *
 * Parameters:
 *	w	  - AsciiSink Object
 *	tab_count - number of tabs in the list
 *	tabs	  - text positions of the tabs
 *
 * Description:
 *	Sets the Tab stops.
 */
static void
SetTabs(Widget w, int tab_count, short *tabs)
{
  AsciiSinkObject sink = (AsciiSinkObject)w;
  int i;
  Atom XA_FIGURE_WIDTH;
  unsigned long figure_width = 0;
  XFontStruct *font = sink->ascii_sink.font;

  /*
   * Find the figure width of the current font
   */
  XA_FIGURE_WIDTH = XInternAtom(XtDisplayOfObject(w), "FIGURE_WIDTH", False);
  if (XA_FIGURE_WIDTH != None
      && (!XGetFontProperty(font, XA_FIGURE_WIDTH, &figure_width)
	  || figure_width == 0))
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
	XtRealloc((char*)sink->text_sink.tabs, tab_count * sizeof(Position));
      sink->text_sink.char_tabs = (short *)
	XtRealloc((char*)sink->text_sink.char_tabs, tab_count * sizeof(short));
  }

  for (i = 0 ; i < tab_count ; i++)
    {
      sink->text_sink.tabs[i] = tabs[i] * figure_width;
      sink->text_sink.char_tabs[i] = tabs[i];
    }

  sink->text_sink.tab_count = tab_count;

#ifndef NO_TAB_FIX
  {
    TextWidget ctx = (TextWidget)XtParent(w);
    ctx->text.redisplay_needed = True;
    _XawTextBuildLineTable(ctx, ctx->text.lt.top, True);
  }
#endif
}
