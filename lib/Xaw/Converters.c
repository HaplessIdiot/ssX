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

/* $XFree86: xc/lib/Xaw/Converters.c,v 3.2 1998/04/28 13:33:32 robin Exp $ */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "Simple.h"
#include "Private.h"

/*
 * Definitions
 */
#define done(type, value)			\
{						\
  if (toVal->addr != NULL)			\
    {						\
      if (toVal->size < sizeof(type))		\
	{					\
	  toVal->size = sizeof(type);		\
	  return (False);			\
	}					\
      *(type *)(toVal->addr) = (value);		\
    }						\
  else						\
    {						\
      static type static_val;			\
						\
      static_val = (value);			\
      toVal->addr = (XPointer)&static_val;	\
    }						\
  toVal->size = sizeof(type);			\
  return (True);				\
}

#define string_done(value)			\
{						\
  if (toVal->addr != NULL)			\
    {						\
      if (toVal->size < size)			\
	{					\
	  toVal->size = size;			\
	  return (False);			\
	}					\
      strcpy((char *)toVal->addr, (value));	\
    }						\
  else						\
    toVal->addr = (XPointer)(value);		\
  toVal->size = size;				\
  return (True);				\
}

/*
 * Prototypes
 */
static Boolean _XawCvtBooleanToString(Display*, XrmValue*, Cardinal*,
				      XrmValue*, XrmValue*, XtPointer*);
static Boolean _XawCvtPositionToString(Display*, XrmValue*, Cardinal*,
				       XrmValue*, XrmValue*, XtPointer*);
static Boolean _XawCvtDimensionToString(Display*, XrmValue*, Cardinal*,
					XrmValue*, XrmValue*, XtPointer*);
static Boolean _XawCvtPixelToString(Display*, XrmValue*, Cardinal*,
				    XrmValue*, XrmValue*, XtPointer*);
static Boolean _XawCvtFontStructToString(Display*, XrmValue*, Cardinal*,
					 XrmValue*, XrmValue*, XtPointer*);
static Boolean _XawCvtStringToDisplayList(Display*, XrmValue*, Cardinal*,
					  XrmValue*, XrmValue*, XtPointer*);

/*
 * Initialization
 */
static XtConvertArgRec PixelArgs[] = {
  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.colormap),
   sizeof(Colormap)},
};

static XtConvertArgRec DLArgs[] = {
  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.screen),
   sizeof(Screen *)},
  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.colormap),
   sizeof(Colormap)},
  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.depth),
   sizeof(int)},
};

static String XtCToolkitError = "ToolkitError";
static String XtNwrongParameters = "wrongParameters";
static String XtNconversionError = "conversionError";

/*
 * Implementation
 */
void
XawInitializeDefaultConverters(void)
{
  static Boolean first_time = True;

  if (first_time == False)
    return;

  first_time = False;

  XtSetTypeConverter(XtRBoolean, XtRString,
		     _XawCvtBooleanToString,
		     NULL, 0,
		     XtCacheNone,
		     NULL);
  XtSetTypeConverter(XtRPosition, XtRString,
		     _XawCvtPositionToString,
		     NULL, 0,
		     XtCacheNone,
		     NULL);
  XtSetTypeConverter(XtRDimension, XtRString,
		     _XawCvtDimensionToString,
		     NULL, 0,
		     XtCacheNone,
		     NULL);
  XtSetTypeConverter(XtRPixel, XtRString,
		     _XawCvtPixelToString,
		     &PixelArgs[0], XtNumber(PixelArgs),
		     XtCacheNone,
		     NULL);
  XtSetTypeConverter(XtRFontStruct, XtRString,
		     _XawCvtFontStructToString,
		     NULL, 0,
		     XtCacheNone,
		     NULL);
  XtSetTypeConverter(XtRString, XawRDisplayList,
		     _XawCvtStringToDisplayList,
		     &DLArgs[0], XtNumber(DLArgs),
		     XtCacheAll,
		     NULL);
}

/* ARGSUSED */
Boolean
_XawCvtBooleanToString(Display *dpy, XrmValue *args, Cardinal *num_args,
		       XrmValue *fromVal, XrmValue *toVal,
		       XtPointer *converter_data)
{
  static char buffer[6];
  Cardinal size;

  if (*num_args != 0)
    XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		    XtNwrongParameters, "cvtPositionToString",
		    XtCToolkitError,
		    "Boolean to String conversion needs no extra arguments",
		    (String *)NULL, (Cardinal *)NULL);

  sprintf(buffer, "%s", *(Boolean *)fromVal->addr ? "true" : "false");
  size = strlen(buffer);

  string_done(buffer);
}

/* ARGSUSED */
Boolean
_XawCvtPositionToString(Display *dpy, XrmValue *args, Cardinal *num_args,
			XrmValue *fromVal, XrmValue *toVal,
			XtPointer *converter_data)
{
  static char buffer[7];
  Cardinal size;

  if (*num_args != 0)
    XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		    XtNwrongParameters, "cvtPositionToString",
		    XtCToolkitError,
		    "Position to String conversion needs no extra arguments",
		    (String *)NULL, (Cardinal *)NULL);

  snprintf(buffer, sizeof(buffer), "%d", *(Position *)fromVal->addr);
  size = strlen(buffer);

  string_done(buffer);
}

/* ARGSUSED */
Boolean
_XawCvtDimensionToString(Display *dpy, XrmValue *args, Cardinal *num_args,
			 XrmValue *fromVal, XrmValue *toVal,
			 XtPointer *converter_data)
{
  static char buffer[6];
  Cardinal size;

  if (*num_args != 0)
    XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		    XtNwrongParameters, "cvtDimensionToString",
		    XtCToolkitError,
		    "Dimension to String conversion needs no extra arguments",
		    (String *)NULL, (Cardinal *)NULL);

  snprintf(buffer, sizeof(buffer), "%u", *(Dimension *)fromVal->addr);
  size = strlen(buffer);

  string_done(buffer);
}

/* ARGSUSED */
Boolean
_XawCvtPixelToString(Display *dpy, XrmValue *args, Cardinal *num_args,
		     XrmValue *fromVal, XrmValue *toVal,
		     XtPointer *converter_data)
{
  static char buffer[19];
  Cardinal size;
  Colormap colormap;
  XColor color;

  if (*num_args != 1)
    {
      XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		      XtNwrongParameters, "cvtPixelToString",
		      XtCToolkitError,
		      "Pixel to String conversion needs colormap argument",
		      (String *)NULL, (Cardinal *)NULL);
      return (False);
    }

  colormap = *(Colormap *)args[0].addr;
  color.pixel = *(Pixel *)fromVal->addr;

  /* Note:
   * If we know the visual type, we can calculate the xcolor
   * without asking Xlib.
   */
  XQueryColor(dpy, colormap, &color);
  sprintf(buffer, "rgb:%04hx/%04hx/%04hx", color.red, color.green, color.blue);
  size = strlen(buffer);

  string_done(buffer);
}

/* ARGSUSED */
Boolean
_XawCvtFontStructToString(Display *dpy, XrmValue *args, Cardinal *num_args,
			  XrmValue *fromVal, XrmValue *toVal,
			  XtPointer *converter_data)
{
  static char buffer[128];
  Cardinal size;
  Atom atom;
  unsigned long value;

  if (*num_args != 0)
    XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		    XtNwrongParameters, "cvtFontStructToString",
		    XtCToolkitError,
		    "FontStruct to String conversion needs no extra arguments",
		    (String *)NULL, (Cardinal *)NULL);

  if ((atom = XInternAtom(dpy, "FONT", True)) == None)
    return (False);

  size = 0;

  if (XGetFontProperty(*(XFontStruct **)fromVal->addr, atom, &value))
    {
      char *tmp = XGetAtomName(dpy, value);

      if (tmp)
	{
	  snprintf(buffer, sizeof(buffer), "%s", tmp);
	  size = strlen(tmp);
	  XFree(tmp);
	}
    }

  if (size)
    string_done(buffer);

  XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		  XtNconversionError, "cvtFontStructToString",
		  XtCToolkitError,
		  "Cannot convert FontStruct to String",
		  (String *)NULL, (Cardinal *)NULL);

  return (False);
}

Boolean
_XawCvtStringToDisplayList(Display *dpy, XrmValue *args, Cardinal *num_args,
			   XrmValue *fromVal, XrmValue *toVal,
			   XtPointer *converter_data)
{
  XawDisplayList *dlist;
  Screen *screen;
  Colormap colormap;
  int depth;
  String commands;

  if (*num_args != 3)
    {
      XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		      XtNwrongParameters, "cvtStringToDisplayList",
		      XtCToolkitError,
		      "String to DisplayList conversion needs screen, "
		      "colormap, and depth arguments",
		      (String *)NULL, (Cardinal *)NULL);
      return (False);
    }

  screen     = *(Screen **)args[0].addr;
  colormap   = *(Colormap *)args[1].addr;
  depth      = *(int *)args[2].addr;

  commands = (String)(fromVal[0].addr);

  dlist = XawCreateDisplayList(commands, screen, colormap, depth);

  if (!dlist)
    {
      String params[1];
      Cardinal num_params = 1;
      static XawDisplayList *sdl = NULL;

      params[0] = (String)fromVal->addr;
      XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
		      XtNconversionError, "cvtStringToDisplayList",
		      XtCToolkitError,
		      "Cannot convert \"%s\" to type DisplayList",
		      params, &num_params);
      toVal->addr = (XtPointer)sdl;
      toVal->size = sizeof(sdl);
      return (False);
    }

  done(XawDisplayList*, dlist);
}
