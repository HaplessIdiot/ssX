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

/* $XFree86: xc/programs/xedit/lisp/helper.h,v 1.10 2002/06/03 21:39:23 paulo Exp $ */

#ifndef Lisp_helper_h
#define Lisp_helper_h

#include "private.h"

/*
 * Prototypes
 */
void LispCheckSequenceStartEnd(LispMac*, LispBuiltin*, LispObj*,
			       LispObj*, LispObj*, long*, long*, long*);
long LispLength(LispMac*, LispObj*);
LispObj *LispCharacterCoerce(LispMac*, LispBuiltin*, LispObj*);
LispObj *LispStringCoerce(LispMac*, LispBuiltin*, LispObj*);
LispObj *LispCoerce(LispMac*, LispBuiltin*, LispObj*, LispObj*);

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

#define FEQ	1
#define FEQL	2
#define FEQUAL	3
#define FEQUALP	4
LispObj *LispObjectCompare(LispMac*, LispObj*, LispObj*, int);
#define XEQ(x, y)	LispObjectCompare(mac, x, y, FEQ)
#define XEQL(x, y)	LispObjectCompare(mac, x, y, FEQL)
#define XEQUAL(x, y)	LispObjectCompare(mac, x, y, FEQUAL)
#define XEQUALP(x, y)	LispObjectCompare(mac, x, y, FEQUALP)

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
LispObj *LispWriteString_(LispMac*, LispBuiltin*, int);

#endif	/* Lisp_helper_h */
