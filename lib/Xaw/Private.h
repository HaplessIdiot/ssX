/*
 * $Id: Private.h,v 3.1 1998/04/26 17:03:27 robin Exp $
 *
 * Copyright (c) 1996 by The XFree86 Project, Inc.
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

#ifndef _XawPrivate_h
#define _XawPrivate_h

#define XawMax(a, b) ((a) > (b) ? (a) : (b))
#define XawMin(a, b) ((a) < (b) ? (a) : (b))
#define XawAbs(a)    ((a) < 0 ? -(a) : (a))

#ifndef XtX
#define XtX(w)            (((RectObj)w)->rectangle.x)
#endif
#ifndef XtY
#define XtY(w)            (((RectObj)w)->rectangle.y)
#endif
#ifndef XtWidth
#define XtWidth(w)        (((RectObj)w)->rectangle.width)
#endif
#ifndef XtHeight
#define XtHeight(w)       (((RectObj)w)->rectangle.height)
#endif
#ifndef XtBorderWidth
#define XtBorderWidth(w)  (((RectObj)w)->rectangle.border_width)
#endif

#define XAW_PRIV_VAR_PREFIX '$'

typedef Boolean (*XawParseBooleanProc)(Widget, String, XEvent*, Boolean*);

typedef struct _XawActionVarList XawActionVarList;
typedef struct _XawActionResList XawActionResList;

/* Boolean expressions */
extern Boolean XawParseBoolean(Widget, String, XEvent*, Boolean*);
extern Boolean XawBooleanExpression(Widget, String, XEvent*);

/* actions */
extern void XawPrintActionErrorMsg(String, Widget, String*, Cardinal*);
extern XawActionResList *XawGetActionResList(WidgetClass);
extern XawActionVarList *XawGetActionVarList(Widget);

extern void XawSetValuesAction(Widget, XEvent*, String*, Cardinal*);
extern void XawGetValuesAction(Widget, XEvent*, String*, Cardinal*);
extern void XawDeclareAction(Widget, XEvent*, String*, Cardinal*);
extern void XawCallProcAction(Widget, XEvent*, String*, Cardinal*);

/* display lists */
#ifndef XAW_DL_DEFINED
#define XAW_DL_DEFINED
typedef struct _XawDL XawDisplayList;
#endif
typedef struct _XawDLClass XawDLClass, XawDisplayListClass;

typedef void (*XawDisplayListProc)(Widget, XtPointer, XtPointer,
				   XEvent*, Region);
typedef void *(*XawDLArgsInitProc)(String, String*, Cardinal*,
				   Screen*, Colormap, int);
typedef void *(*XawDLDataInitProc)(String,
				   Screen*, Colormap, int);
typedef void (*XawDLArgsDestructor)(Display*, String, XtPointer,
				    String*, Cardinal*);
typedef void (*XawDLDataDestructor)(Display*, String, XtPointer);

void XawRunDisplayList(Widget, XawDisplayList*, XEvent*, Region);
void XawDisplayListInitialize();

XawDisplayList *XawCreateDisplayList(String, Screen*, Colormap, int);
XawDLClass *XawGetDisplayListClass(String);
XawDLClass *XawCreateDisplayListClass(String,
				      XawDLArgsInitProc, XawDLArgsDestructor,
				      XawDLDataInitProc, XawDLDataDestructor);
Boolean XawDeclareDisplayListProc(XawDLClass*, String, XawDisplayListProc);

#endif /* _XawPrivate_h */
