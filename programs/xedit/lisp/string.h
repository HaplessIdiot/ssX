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

/* $XFree86: xc/programs/xedit/lisp/string.h,v 1.6 2002/08/25 02:48:31 paulo Exp $ */

#ifndef Lisp_string_h
#define Lisp_string_h

#include "internal.h"

LispObj *Lisp_AlphaCharP(LispMac*, LispBuiltin*);	  /* alpha-char-p */
LispObj *Lisp_Char(LispMac*, LispBuiltin*);		  /* char */
LispObj *Lisp_CharLess(LispMac*, LispBuiltin*);		  /* char< */
LispObj *Lisp_CharLessEqual(LispMac*, LispBuiltin*);	  /* char<= */
LispObj *Lisp_CharEqual_(LispMac*, LispBuiltin*);	  /* char= */
LispObj *Lisp_CharGreater(LispMac*, LispBuiltin*);	  /* char> */
LispObj *Lisp_CharGreaterEqual(LispMac*, LispBuiltin*);	  /* char>= */
LispObj *Lisp_CharNotEqual_(LispMac*, LispBuiltin*);	  /* char/= */
LispObj *Lisp_CharLessp(LispMac*, LispBuiltin*);	  /* char-lessp */
LispObj *Lisp_CharNotGreaterp(LispMac*, LispBuiltin*);	  /* char-not-greaterp */
LispObj *Lisp_CharEqual(LispMac*, LispBuiltin*);	  /* char-equal */
LispObj *Lisp_CharGreaterp(LispMac*, LispBuiltin*);	  /* char-greaterp */
LispObj *Lisp_CharNotLessp(LispMac*, LispBuiltin*);	  /* char-not-lessp */
LispObj *Lisp_CharNotEqual(LispMac*, LispBuiltin*);	  /* char-not-equal */
LispObj *Lisp_Character(LispMac*, LispBuiltin*);	  /* character */
LispObj *Lisp_Characterp(LispMac*, LispBuiltin*);	  /* characterp */
LispObj *Lisp_CharDowncase(LispMac*, LispBuiltin*);	  /* char-downcase */
LispObj *Lisp_CharInt(LispMac*, LispBuiltin*);		  /* char-int */
LispObj *Lisp_CharUpcase(LispMac*, LispBuiltin*);	  /* char-upcase */
LispObj *Lisp_DigitCharP(LispMac*, LispBuiltin*);	  /* digit-char-p */
LispObj *Lisp_IntChar(LispMac*, LispBuiltin*);		  /* int-char */
LispObj *Lisp_MakeString(LispMac*, LispBuiltin*);	  /* make-string */
LispObj *Lisp_ParseInteger(LispMac*, LispBuiltin*);	  /* parse-integer */
LispObj *Lisp_ReadFromString(LispMac*, LispBuiltin*);	  /* read-from-string */
LispObj *Lisp_String(LispMac*, LispBuiltin*);		  /* string */
LispObj *Lisp_Stringp(LispMac*, LispBuiltin*);		  /* stringp */
LispObj *Lisp_StringTrim(LispMac*, LispBuiltin*);	  /* string-trim */
LispObj *Lisp_StringLeftTrim(LispMac*, LispBuiltin*);	  /* string-left-trim */
LispObj *Lisp_StringRightTrim(LispMac*, LispBuiltin*);	  /* string-right-trim */
LispObj *Lisp_StringEqual_(LispMac*, LispBuiltin*);	  /* string= */
LispObj *Lisp_StringLess(LispMac*, LispBuiltin*);	  /* string< */
LispObj *Lisp_StringGreater(LispMac*, LispBuiltin*);	  /* string> */
LispObj *Lisp_StringLessEqual(LispMac*, LispBuiltin*);	  /* string<= */
LispObj *Lisp_StringGreaterEqual(LispMac*, LispBuiltin*); /* string>= */
LispObj *Lisp_StringNotEqual_(LispMac*, LispBuiltin*);	  /* string/= */
LispObj *Lisp_StringEqual(LispMac*, LispBuiltin*);	  /* string-equal */
LispObj *Lisp_StringGreaterp(LispMac*, LispBuiltin*);	  /* string-greaterp */
LispObj *Lisp_StringLessp(LispMac*, LispBuiltin*);	  /* string-lessp */
LispObj *Lisp_StringNotLessp(LispMac*, LispBuiltin*);	  /* string-not-lessp */
LispObj *Lisp_StringNotGreaterp(LispMac*, LispBuiltin*);  /* string-not-greaterp */
LispObj *Lisp_StringNotEqual(LispMac*, LispBuiltin*);	  /* string-not-equal */
LispObj *Lisp_StringUpcase(LispMac*, LispBuiltin*);	  /* string-upcase */
LispObj *Lisp_StringDowncase(LispMac*, LispBuiltin*);	  /* string-downcase */
LispObj *Lisp_StringCapitalize(LispMac*, LispBuiltin*);	  /* string-capitalize */
LispObj *Lisp_StringConcat(LispMac*, LispBuiltin*);	  /* string-concat */
LispObj *Lisp_XeditCharStore(LispMac*, LispBuiltin*);	  /* xedit::char-store */

#endif /* Lisp_String_h */
