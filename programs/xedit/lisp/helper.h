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

#ifndef Lisp_helper_h
#define Lisp_helper_h

#include "private.h"

/*
 * Defines
 */
#define LESS		0
#define LESS_EQUAL	1
#define EQUAL		2
#define GREATER		3
#define GREATER_EQUAL	4

/*
 * Prototypes
 */
/*
 (equal x y)
 * called directly by several functions */
LispObj *_LispEqual(LispMac*, LispObj*, LispObj*);

/*
 (nth n list)
 (nthcdr n list)
 */
LispObj *_LispNth(LispMac*, LispObj*, char*, int);

/* When no <setf-place> is set, this function searchs for the one */
LispObj *_LispFindPlace(LispMac*, LispObj*, LispObj*);

/*
 (min number &rest more-numbers)
 (max number &rest more-numbers)
 */
LispObj *_LispMinMax(LispMac*, LispObj*, char*, int);

/*
 (< number &rest more-numbers)
 (<= number &rest more-numbers)
 (= number &rest more-numbers)
 (> number &rest more-numbers)
 (>= number &rest more-numbers)
 */
LispObj *_LispBoolCond(LispMac*, LispObj*, char*, int);

/*
 (defmacro name lambda-list [[ {declaration}* | doc-string ]] {form}*)
 (defun name lambda-list [[ {declaration}* | doc-string ]] {form}*)
 (lambda lambda-list {declaration | doc-string}* {form}*)
 * doc-string not yet implemented
 */
LispObj *_LispDefLambda(LispMac*, LispObj*, LispFunType);

/*
 (set symbol value)
 (setq {var form}*)
 * used also by setf when creating a new symbol
 * XXX current setq implementation expects only 2 arguments
 */
LispObj *_LispSet(LispMac*, LispObj*, LispObj*, char*, int);

/*
 (when test {form}*)
 (unless test {form}*)
 */
LispObj *_LispWhenUnless(LispMac*, LispObj*, int);

/*
 (while test {form}*)
 (until test {form}*)
 * XXX emacs identical code, should be rewritten to be just a test
 * condition of (loop)
 */
LispObj *_LispWhileUntil(LispMac*, LispObj*, int);

/*
 * Load and execute a file. Used by (load) and (require)
 */
LispObj *_LispLoadFile(LispMac*, char*, char*, int, int, int);

/*
 * Initialization
 */
extern char *ExpectingListAt;
extern char *ExpectingNumberAt;

#endif	/* Lisp_helper_h */
