/* $XConsortium: StrToOrnt.c,v 1.8 94/04/17 20:16:19 rws Exp $ */

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

/* $XFree86: xc/lib/Xmu/StrToOrnt.c,v 1.3 1998/06/28 12:32:33 dawes Exp $ */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "Converters.h"
#include "CharSet.h"

/*
 * Prototypes
 */
static void InitializeQuarks(void);

/*
 * Initialization
 */
static	XrmQuark Qhorizontal, Qvertical;
static Boolean haveQuarks;

/*
 * Implementation
 */
static void
InitializeQuarks(void)
{
  if (!haveQuarks)
    {
      Qhorizontal = XrmPermStringToQuark(XtEhorizontal);
      Qvertical = XrmPermStringToQuark(XtEvertical);
      haveQuarks = True;
    }
}

/*ARGSUSED*/
void
XmuCvtStringToOrientation(XrmValuePtr args, Cardinal *num_args,
			  XrmValuePtr fromVal, XrmValuePtr toVal)
{
    static XtOrientation orient;
    XrmQuark	q;
  char name[11];

  InitializeQuarks();
  XmuNCopyISOLatin1Lowered(name, (char *)fromVal->addr, sizeof(name));
  q = XrmStringToQuark(name);

  toVal->size = sizeof(XtJustify);
  toVal->addr = (XPointer)&orient;

  if (q == Qhorizontal)
    	orient = XtorientHorizontal;
  else if (q == Qvertical)
    	orient = XtorientVertical;
  else
    {
      toVal->addr = NULL;
      XtStringConversionWarning((char *)fromVal->addr, XtROrientation);
    }
}

/*ARGSUSED*/
Boolean
XmuCvtOrientationToString(Display *dpy, XrmValuePtr args, Cardinal *num_args,
			  XrmValuePtr fromVal, XrmValuePtr toVal,
			  XtPointer *data)
{
  static String buffer;
  Cardinal size;

  switch (*(XtOrientation *)fromVal->addr)
    {
    case XtorientVertical:
      buffer = XtEvertical;
      break;
    case XtorientHorizontal:
      buffer = XtEhorizontal;
      break;
    default:
      XtWarning("Cannot convert Orientation to String");
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
