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

/* $XFree86: xc/programs/xedit/lisp/math.h,v 1.1 2002/01/30 21:00:58 paulo Exp $ */

#ifndef Lisp_math_h
#define Lisp_math_h

#include "internal.h"
#include "mp.h"

void LispMathInit(LispMac*);
LispObj *LispFloatCoerce(LispMac*, LispBuiltin*, LispObj*);

LispObj *Lisp_Mul(LispMac*, LispBuiltin*);		/* * */
LispObj *Lisp_Plus(LispMac*, LispBuiltin*);		/* + */
LispObj *Lisp_Minus(LispMac*, LispBuiltin*);		/* - */
LispObj *Lisp_Div(LispMac*, LispBuiltin*);		/* / */
LispObj *Lisp_OnePlus(LispMac*, LispBuiltin*);		/* 1+ */
LispObj *Lisp_OneMinus(LispMac*, LispBuiltin*);		/* 1- */
LispObj *Lisp_Less(LispMac*, LispBuiltin*);		/* < */
LispObj *Lisp_LessEqual(LispMac*, LispBuiltin*);	/* <= */
LispObj *Lisp_Equal_(LispMac*, LispBuiltin*);		/* = */
LispObj *Lisp_Greater(LispMac*, LispBuiltin*);		/* > */
LispObj *Lisp_GreaterEqual(LispMac*, LispBuiltin*);	/* >= */
LispObj *Lisp_NotEqual(LispMac*, LispBuiltin*);		/* /= */
LispObj *Lisp_Max(LispMac*, LispBuiltin*);		/* max */
LispObj *Lisp_Min(LispMac*, LispBuiltin*);		/* min */
LispObj *Lisp_Abs(LispMac*, LispBuiltin*);		/* abs */
LispObj *Lisp_Complex(LispMac*, LispBuiltin*);		/* complex */
LispObj *Lisp_Complexp(LispMac*, LispBuiltin*);		/* complexp */
LispObj *Lisp_Conjugate(LispMac*, LispBuiltin*);	/* conjugate */
LispObj *Lisp_Decf(LispMac*, LispBuiltin*);		/* decf */
LispObj *Lisp_Denominator(LispMac*, LispBuiltin*);	/* denominator */
LispObj *Lisp_Evenp(LispMac*, LispBuiltin*);		/* evenp */
LispObj *Lisp_Float(LispMac*, LispBuiltin*);		/* float */
LispObj *Lisp_Floatp(LispMac*, LispBuiltin*);		/* floatp */
LispObj *Lisp_Gcd(LispMac*, LispBuiltin*);		/* gcd */
LispObj *Lisp_Imagpart(LispMac*, LispBuiltin*);		/* imagpart */
LispObj *Lisp_Incf(LispMac*, LispBuiltin*);		/* incf */
LispObj *Lisp_Integerp(LispMac*, LispBuiltin*);		/* integerp */
LispObj *Lisp_Isqrt(LispMac*, LispBuiltin*);		/* isqrt */
LispObj *Lisp_Lcm(LispMac*, LispBuiltin*);		/* lcm */
LispObj *Lisp_Logand(LispMac*, LispBuiltin*);		/* logand */
LispObj *Lisp_Logeqv(LispMac*, LispBuiltin*);		/* logeqv */
LispObj *Lisp_Logior(LispMac*, LispBuiltin*);		/* logior */
LispObj *Lisp_Lognot(LispMac*, LispBuiltin*);		/* lognot */
LispObj *Lisp_Logxor(LispMac*, LispBuiltin*);		/* logxor */
LispObj *Lisp_Minusp(LispMac*, LispBuiltin*);		/* minusp */
LispObj *Lisp_Numberp(LispMac*, LispBuiltin*);		/* numberp */
LispObj *Lisp_Numerator(LispMac*, LispBuiltin*);	/* numerator */
LispObj *Lisp_Oddp(LispMac*, LispBuiltin*);		/* oddp */
LispObj *Lisp_Plusp(LispMac*, LispBuiltin*);		/* plusp */
LispObj *Lisp_Rational(LispMac*, LispBuiltin*);		/* rational */
#if 0
LispObj *Lisp_Rationalize(LispMac*, LispBuiltin*);	/* rationalize */
#endif
LispObj *Lisp_Rationalp(LispMac*, LispBuiltin*);	/* rationalp */
LispObj *Lisp_Realpart(LispMac*, LispBuiltin*);		/* realpart */
LispObj *Lisp_Sqrt(LispMac*, LispBuiltin*);		/* sqrt */
LispObj *Lisp_Zerop(LispMac*, LispBuiltin*);		/* zerop */

#endif /* Lisp_math_h */
