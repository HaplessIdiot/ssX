/* $XConsortium: StrToLong.c,v 1.5 94/04/17 20:16:19 rws Exp $ */

/*
 
Copyright (c) 1989  X Consortium

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

#include <stdio.h>
#include <X11/Intrinsic.h>
#include "SysUtil.h"
#include "Converters.h"

void
XmuCvtStringToLong(XrmValuePtr args, Cardinal *num_args,
		   XrmValuePtr fromVal, XrmValuePtr toVal)
{
    static long l;

    if (*num_args != 0)
    XtWarning("String to Long conversion needs no extra arguments");
  if (sscanf((char *)fromVal->addr, "%ld", &l) == 1)
    {
      toVal->size = sizeof(long);
      toVal->addr = (XPointer)&l;
    }
  else
    XtStringConversionWarning((char *)fromVal->addr, XtRLong);
}

/*ARGSUSED*/
Boolean
XmuCvtLongToString(Display *dpy, XrmValuePtr args, Cardinal *num_args,
		   XrmValuePtr fromVal, XrmValuePtr toVal, XtPointer *data)
{
  static char buffer[32];
  int size;

  if (*num_args != 0)
    XtWarning("Long to String conversion needs no extra arguments");

  XmuSnprintf(buffer, sizeof(buffer), "%ld", *(long *)fromVal->addr);

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
