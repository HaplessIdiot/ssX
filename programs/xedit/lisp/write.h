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

/* $XFree86: xc/programs/xedit/lisp/write.h,v 1.6 2002/11/12 06:05:08 paulo Exp $ */

#ifndef Lisp_write_h
#define Lisp_write_h

#include "io.h"

/*
 * Prototypes
 */
void LispWriteInit(void);

LispObj *Lisp_Prin1(LispBuiltin*);
LispObj *Lisp_Princ(LispBuiltin*);
LispObj *Lisp_Print(LispBuiltin*);
LispObj *Lisp_Write(LispBuiltin*);

int LispGetColumn(LispObj*);
int LispGetEscape(LispObj*);
void LispSetEscape(LispObj*, int);

int LispWriteChar(LispObj*, int);
int LispWriteChars(LispObj*, int, int);
int LispWriteStr(LispObj*, char*, long);

	/* object must be an integer */
int LispWriteInteger(LispObj*, LispObj*);
	/* object must be a character */
int LispWriteCharacter(LispObj*, LispObj*);
	/* object must be a string */
int LispWriteString(LispObj*, LispObj*);
	/* object must be a float */
int LispWriteFloat(LispObj*, LispObj*);
	/* write any lisp object to stream */
int LispWriteObject(LispObj*, LispObj*);

/* formatted output */
	/* object must be an integer */
int LispFormatInteger(LispObj*, LispObj*, int, int, int, int, int, int, int);
	/* must be in range 1 to 3999 for new roman, 1 to 4999 for old roman */
int LispFormatRomanInteger(LispObj*, long, int);
	/* must be in range -9999999 to 9999999 */
int LispFormatEnglishInteger(LispObj*, long, int);
	/* object must be a character */
int LispFormatCharacter(LispObj*, LispObj*, int, int);
	/* object must be a float */
int LispFormatFixedFloat(LispObj*, LispObj*, int, int, int*, int, int, int);
	/* object must be a float */
int LispFormatExponentialFloat(LispObj*, LispObj*,
			       int, int, int*, int, int, int, int, int);
	/* object must be a float */
int LispFormatGeneralFloat(LispObj*, LispObj*, int,
			   int, int*, int, int, int, int, int);
int LispFormatDollarFloat(LispObj*, LispObj*, int, int, int, int, int, int);

#endif /* Lisp_write_h */
