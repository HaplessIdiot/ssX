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

/* $XFree86: xc/programs/xedit/lisp/helper.h,v 1.3 2001/10/15 07:05:52 paulo Exp $ */

#ifndef Lisp_helper_h
#define Lisp_helper_h

#include "private.h"

/*
 * Prototypes
 */
LispObj *LispSet(LispMac*, LispObj*, LispObj*, char*, int);

/*
 do init test &rest body
 do* init test &rest body
 */
LispObj *LispDo(LispMac*, LispBuiltin*, int);

/*
 dolist init &rest body
 dotimes init &rest body
 */
LispObj *LispDoListTimes(LispMac*, LispBuiltin*, int);

LispObj *LispEqual(LispMac*, LispObj*, LispObj*);

LispObj *LispLoadFile(LispMac*, LispObj*, int, int, int);

/*
 string= string1 string2 &key start1 end1 start2 end2
 string< string1 string2 &key start1 end1 start2 end2
 string> string1 string2 &key start1 end1 start2 end2
 string<= string1 string2 &key start1 end1 start2 end2
 string>= string1 string2 &key start1 end1 start2 end2
 string/= string1 string2 &key start1 end1 start2 end2
 string-equal string1 string2 &key start1 end1 start2 end2
 string-lessp string1 string2 &key start1 end1 start2 end2
 string-greaterp string1 string2 &key start1 end1 start2 end2
 string-not-lessp string1 string2 &key start1 end1 start2 end2
 string-not-greaterp string1 string2 &key start1 end1 start2 end2
 string-not-equal string1 string2 &key start1 end1 start2 end2
*/
void LispGetStringArgs(LispMac*, LispBuiltin*,
			char**,	/* string1 */
			char**,	/* string2 */
			int*,	/* start1 */
			int*,	/* end1 */
			int*,	/* start2 */
			int*);	/* end2 */

/*
 string-trim character-bag string
 string-left-trim character-bag string
 string-right-trim character-bag string
*/
LispObj *LispStringTrim(LispMac*, LispBuiltin*, int, int);

/*
 string-upcase string &key start end
 string-downcase string &key start end
 string-capitalize string &key start end
*/
void LispGetStringCaseArgs(LispMac*, LispBuiltin*,
			   LispObj**, char**, int*, int*);

/*
 pathname-host pathname
 pathname-device pathname
 pathname-directory pathname
 pathname-name pathname
 pathname-type pathname
 pathname-version pathname
 */
LispObj *LispPathnameField(LispMac*, int, int);

/*
 truename pathname
 probe-file pathname
 */
LispObj *LispProbeFile(LispMac*, LispBuiltin*, int);

/*
 read-char &optional input-stream (eof-error-p t) eof-value recursive-p
 read-char-no-hang &optional input-stream (eof-error-p t) eof-value recursive-p
 */
LispObj *LispReadChar(LispMac*, LispBuiltin*, int);

/*
 write-string string &optional output-stream &key start end
 write-line string &optional output-stream &key start end
 */
LispObj *LispWriteString(LispMac*, LispBuiltin*, int);

#endif	/* Lisp_helper_h */
