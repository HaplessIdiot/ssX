/* $XConsortium: StrToShap.c,v 1.5 94/04/17 20:16:20 converse Exp $ */

/* 
 
Copyright (c) 1988  X Consortium

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

#include <string.h>
#include <X11/Intrinsic.h>
#include "Converters.h"
#include "CharSet.h"

/* ARGSUSED */
#define	done(type, value) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XtPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}


/*ARGSUSED*/
Boolean
XmuCvtStringToShapeStyle(Display *dpy, XrmValue *args, Cardinal *num_args,
			 XrmValue *from, XrmValue *toVal, XtPointer *data)
{
  String name = (String)from->addr;

  if (XmuCompareISOLatin1(name, XtERectangle) == 0)
    done(int, XmuShapeRectangle);
  if (XmuCompareISOLatin1(name, XtEOval) == 0)
    done(int, XmuShapeOval);
  if (XmuCompareISOLatin1(name, XtEEllipse) == 0)
    done(int, XmuShapeEllipse);
  if (XmuCompareISOLatin1(name, XtERoundedRectangle) == 0)
    done(int, XmuShapeRoundedRectangle);

  XtDisplayStringConversionWarning(dpy, name, XtRShapeStyle);

  return (False);
}

/*ARGSUSED*/
Boolean
XmuCvtShapeStyleToString(Display *dpy, XrmValue *args, Cardinal *num_args,
			 XrmValue *fromVal, XrmValue *toVal, XtPointer *data)
{
  static char *buffer;
  Cardinal size;

  switch (*(int *)fromVal->addr)
    {
    case XmuShapeRectangle:
      buffer = XtERectangle;
      break;
    case XmuShapeOval:
      buffer = XtEOval;
      break;
    case XmuShapeEllipse:
      buffer = XtEEllipse;
      break;
    case XmuShapeRoundedRectangle:
      buffer = XtERoundedRectangle;
      break;
    default:
      XtAppWarning(XtDisplayToApplicationContext(dpy),
		   "Cannot convert ShapeStyle to String");
      toVal->addr = NULL;
      toVal->size = 0;

      return (False);
	    }

  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
    {
      if (toVal->size <= size)
	{
	  toVal->size = size;
	  return (False);
	}
      strcpy((char *)toVal->addr, buffer);
    }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = size;

  return (True);
}
