/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
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

#ifndef Lisp_core_h
#define Lisp_core_h

#include "internal.h"

LispObj *Lisp_Mul(LispMac*, LispObj*, char*);		/* * */
LispObj *Lisp_Plus(LispMac*, LispObj*, char*);		/* + */
LispObj *Lisp_Minus(LispMac*, LispObj*, char*);		/* - */
LispObj *Lisp_Div(LispMac*, LispObj*, char*);		/* / */
LispObj *Lisp_OnePlus(LispMac*, LispObj*, char*);	/* 1+ */
LispObj *Lisp_OneMinus(LispMac*, LispObj*, char*);	/* 1- */
LispObj *Lisp_Less(LispMac*, LispObj*, char*);		/* < */
LispObj *Lisp_LessEqual(LispMac*, LispObj*, char*);	/* <= */
LispObj *Lisp_Equal_(LispMac*, LispObj*, char*);	/* = */
LispObj *Lisp_Greater(LispMac*, LispObj*, char*);	/* > */
LispObj *Lisp_GreaterEqual(LispMac*, LispObj*, char*);	/* >= */
LispObj *Lisp_Aref(LispMac*, LispObj*, char*);		/* aref */
LispObj *Lisp_Assoc(LispMac*, LispObj*, char*);		/* assoc */
LispObj *Lisp_And(LispMac*, LispObj*, char*);		/* and */
LispObj *Lisp_Append(LispMac*, LispObj*, char*);	/* append */
LispObj *Lisp_Apply(LispMac*, LispObj*, char*);		/* apply */
LispObj *Lisp_Atom(LispMac*, LispObj*, char*);		/* attom */
LispObj *Lisp_Butlast(LispMac*, LispObj*, char*);	/* butlast */
LispObj *Lisp_Car(LispMac*, LispObj*, char*);		/* car */
LispObj *Lisp_Catch(LispMac*, LispObj*, char*);		/* catch */
LispObj *Lisp_Cdr(LispMac*, LispObj*, char*);		/* cdr */
LispObj *Lisp_Coerce(LispMac*, LispObj*, char*);	/* coerce */
LispObj *Lisp_Cond(LispMac*, LispObj*, char*);		/* cond */
LispObj *Lisp_Cons(LispMac*, LispObj*, char*);		/* cons */
LispObj *Lisp_Defmacro(LispMac*, LispObj*, char*);	/* defmacro */
LispObj *Lisp_Defun(LispMac*, LispObj*, char*);		/* defun */
LispObj *Lisp_Defstruct(LispMac*, LispObj*, char*);	/* defstruct */
LispObj *Lisp_Equal(LispMac*, LispObj*, char*);		/* equal */
LispObj *Lisp_Eval(LispMac*, LispObj*, char*);		/* eval */
LispObj *Lisp_Funcall(LispMac*, LispObj*, char*);	/* funcall */
LispObj *Lisp_Gc(LispMac*, LispObj*, char*);		/* gc */
LispObj *Lisp_Get(LispMac*, LispObj*, char*);		/* get */
LispObj *Lisp_If(LispMac*, LispObj*, char*);		/* if */
LispObj *Lisp_Lambda(LispMac*, LispObj*, char*);	/* lambda */
LispObj *Lisp_Last(LispMac*, LispObj*, char*);		/* last */
LispObj *Lisp_Length(LispMac*, LispObj*, char*);	/* length */
LispObj *Lisp_Let(LispMac*, LispObj*, char*);		/* let */
LispObj *Lisp_LetP(LispMac*, LispObj*, char*);		/* let* */
LispObj *Lisp_List(LispMac*, LispObj*, char*);		/* list */
LispObj *Lisp_ListP(LispMac*, LispObj*, char*);		/* list* */
LispObj *Lisp_Listp(LispMac*, LispObj*, char*);		/* listp */
LispObj *Lisp_Makearray(LispMac*, LispObj*, char*);	/* make-array */
LispObj *Lisp_Mapcar(LispMac*, LispObj*, char*);	/* mapcar */
LispObj *Lisp_Max(LispMac*, LispObj*, char*);		/* max */
LispObj *Lisp_Member(LispMac*, LispObj*, char*);	/* member */
LispObj *Lisp_Min(LispMac*, LispObj*, char*);		/* min */
LispObj *Lisp_Nth(LispMac*, LispObj*, char*);		/* nth */
LispObj *Lisp_Nthcdr(LispMac*, LispObj*, char*);	/* nthcdr */
LispObj *Lisp_Null(LispMac*, LispObj*, char*);		/* null */
LispObj *Lisp_Numberp(LispMac*, LispObj*, char*);	/* numberp */
LispObj *Lisp_Or(LispMac*, LispObj*, char*);		/* or */
LispObj *Lisp_Pop(LispMac*, LispObj*, char*);		/* pop */
LispObj *Lisp_Print(LispMac*, LispObj*, char*);		/* print */
LispObj *Lisp_Prog1(LispMac*, LispObj*, char*);		/* prog1 */
LispObj *Lisp_Prog2(LispMac*, LispObj*, char*);		/* prog2 */
LispObj *Lisp_Progn(LispMac*, LispObj*, char*);		/* progn */
LispObj *Lisp_Provide(LispMac*, LispObj*, char*);	/* provide */
LispObj *Lisp_Push(LispMac*, LispObj*, char*);		/* push */
LispObj *Lisp_Quit(LispMac*, LispObj*, char*);		/* quit */
LispObj *Lisp_Quote(LispMac*, LispObj*, char*);		/* quote */
LispObj *Lisp_Reverse(LispMac*, LispObj*, char*);	/* reverse */
LispObj *Lisp_Rplaca(LispMac*, LispObj*, char*);	/* rplaca */
LispObj *Lisp_Rplacd(LispMac*, LispObj*, char*);	/* rplaca */
LispObj *Lisp_Set(LispMac*, LispObj*, char*);		/* set */
LispObj *Lisp_Setf(LispMac*, LispObj*, char*);		/* setf */
LispObj *Lisp_SetQ(LispMac*, LispObj*, char*);		/* setq */
LispObj *Lisp_Stringp(LispMac*, LispObj*, char*);	/* stringp */
LispObj *Lisp_Symbolp(LispMac*, LispObj*, char*);	/* symbolp */
LispObj *Lisp_SymbolPlist(LispMac*, LispObj*, char*);	/* symbol-plist */
LispObj *Lisp_Throw(LispMac*, LispObj*, char*);		/* throw */
LispObj *Lisp_Typep(LispMac*, LispObj*, char*);		/* typep */
LispObj *Lisp_Unless(LispMac*, LispObj*, char*);	/* unless */
LispObj *Lisp_Until(LispMac*, LispObj*, char*);		/* until */
LispObj *Lisp_Vector(LispMac*, LispObj*, char*);	/* vector */
LispObj *Lisp_When(LispMac*, LispObj*, char*);		/* when */
LispObj *Lisp_While(LispMac*, LispObj*, char*);		/* while */

#endif /* Lisp_core_h */
