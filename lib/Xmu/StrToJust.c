/* $XConsortium: StrToJust.c,v 1.8 94/04/17 20:16:18 rws Exp $ */

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

/* $XFree86: xc/lib/Xmu/StrToJust.c,v 1.3 1998/06/28 12:32:32 dawes Exp $ */

#include <string.h>
#include <X11/Intrinsic.h>
#include "Converters.h"
#include "CharSet.h"

/*
 * Prototypes
 */
static void InitializeQuarks(void);

/*
 * Initialization
 */
static XrmQuark Qleft, Qcenter, Qright;
static Boolean haveQuarks;

/*
 * Implementation
 */
static void
InitializeQuarks(void)
{
  if (!haveQuarks)
    {
      Qleft = XrmPermStringToQuark(XtEleft);
      Qcenter = XrmPermStringToQuark(XtEcenter);
      Qright = XrmPermStringToQuark(XtEright);
      haveQuarks = True;
    }
}

/*ARGSUSED*/
void
XmuCvtStringToJustify(XrmValuePtr args, Cardinal *num_args,
		      XrmValuePtr fromVal, XrmValuePtr toVal)
{
    static XtJustify	e;
    XrmQuark    q;
  char *s = (char *)fromVal->addr;
  char name[7];

  if (s == NULL)
    return;

  InitializeQuarks();
  XmuNCopyISOLatin1Lowered(name, s, sizeof(name));

  q = XrmStringToQuark(name);

    toVal->size = sizeof(XtJustify);
  toVal->addr = (XPointer)&e;

  if (q == Qleft)
    e = XtJustifyLeft;
  else if (q == Qcenter)
    e = XtJustifyCenter;
  else if (q == Qright)
    e = XtJustifyRight;
  else
    {
    toVal->addr = NULL;
    XtStringConversionWarning((char *)fromVal->addr, XtRJustify);
    }
}

/*ARGSUSED*/
Boolean
XmuCvtJustifyToString(Display *dpy, XrmValue* args, Cardinal *num_args,
		      XrmValue *fromVal, XrmValue *toVal, XtPointer *data)
{
  static String buffer;
  Cardinal size;

  switch (*(XtJustify *)fromVal->addr)
    {
    case XtJustifyLeft:
      buffer = XtEleft;
      break;
    case XtJustifyCenter:
      buffer = XtEcenter;
      break;
    case XtJustifyRight:
      buffer = XtEright;
      break;
    default:
      XtWarning("Cannot convert Justify to String");
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
