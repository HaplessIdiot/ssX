/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
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
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86$ */

#ifndef Lisp_xedit_h
#define Lisp_xedit_h

#ifdef XEDIT_LISP_PRIVATE
#include "private.h"
#include "io.h"
#include "read.h"
#include "write.h"

LispObj *Xedit_AddEntity(LispMac*, LispBuiltin*);
LispObj *Xedit_AutoFill(LispMac*, LispBuiltin*);
LispObj *Xedit_Background(LispMac*, LispBuiltin*);
LispObj *Xedit_ClearEntities(LispMac*, LispBuiltin*);
LispObj *Xedit_ConvertPropertyList(LispMac*, LispBuiltin*);
LispObj *Xedit_Font(LispMac*, LispBuiltin*);
LispObj *Xedit_Foreground(LispMac*, LispBuiltin*);
LispObj *Xedit_HorizontalScrollbar(LispMac*, LispBuiltin*);
LispObj *Xedit_Insert(LispMac*, LispBuiltin*);
LispObj *Xedit_Justification(LispMac*, LispBuiltin*);
LispObj *Xedit_LeftColumn(LispMac*, LispBuiltin*);
LispObj *Xedit_Point(LispMac*, LispBuiltin*);
LispObj *Xedit_PointMax(LispMac*, LispBuiltin*);
LispObj *Xedit_PointMin(LispMac*, LispBuiltin*);
LispObj *Xedit_PropertyList(LispMac*, LispBuiltin*);
LispObj *Xedit_ReadText(LispMac*, LispBuiltin*);
LispObj *Xedit_ReplaceText(LispMac*, LispBuiltin*);
LispObj *Xedit_RightColumn(LispMac*, LispBuiltin*);
LispObj *Xedit_Scan(LispMac*, LispBuiltin*);
LispObj *Xedit_SearchBackward(LispMac*, LispBuiltin*);
LispObj *Xedit_SearchForward(LispMac*, LispBuiltin*);
LispObj *Xedit_VerticalScrollbar(LispMac*, LispBuiltin*);
LispObj *Xedit_WrapMode(LispMac*, LispBuiltin*);
LispObj *Xedit_XrmStringToQuark(LispMac*, LispBuiltin*);
#endif /* XEDIT_LISP_PRIVATE */

void LispXeditInitialize(LispMac*);
int XeditLispExecute(LispMac*, Widget, XawTextPosition, XawTextPosition);
void XeditLispSetEditMode(LispMac*, xedit_flist_item*);

#endif /* Lisp_xedit_h */
