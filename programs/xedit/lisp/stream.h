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

/* $XFree86: xc/programs/xedit/lisp/stream.h,v 1.3 2002/08/05 03:56:24 paulo Exp $ */

#ifndef Lisp_stream_h
#define Lisp_stream_h

#include "io.h"
#include "internal.h"

void LispStreamInit(LispMac*);

LispObj *Lisp_InputStreamP(LispMac*, LispBuiltin*);		/* input-stream-p */
LispObj *Lisp_OpenStreamP(LispMac*, LispBuiltin*);		/* open-stream-p */
LispObj *Lisp_OutputStreamP(LispMac*, LispBuiltin*);		/* output-stream-p */
LispObj *Lisp_Open(LispMac*, LispBuiltin*);			/* open */
LispObj *Lisp_MakePipe(LispMac*, LispBuiltin*);			/* make-pipe */
LispObj *Lisp_PipeBroken(LispMac*, LispBuiltin*);		/* pipe-broken */
LispObj *Lisp_PipeErrorStream(LispMac*, LispBuiltin*);		/* pipe-error-stream */
LispObj *Lisp_PipeInputDescriptor(LispMac*, LispBuiltin*);	/* pipe-input-descriptor */
LispObj *Lisp_PipeErrorDescriptor(LispMac*, LispBuiltin*);	/* pipe-error-descriptor */
LispObj *Lisp_Close(LispMac*, LispBuiltin*);			/* close */
LispObj *Lisp_Listen(LispMac*, LispBuiltin*);			/* listen */
LispObj *Lisp_Read(LispMac*, LispBuiltin*);			/* read */
LispObj *Lisp_ReadChar(LispMac*, LispBuiltin*);			/* read-char */
LispObj *Lisp_ReadCharNoHang(LispMac*, LispBuiltin*);		/* read-char-no-hang */
LispObj *Lisp_Streamp(LispMac*, LispBuiltin*);			/* streamp */
LispObj *Lisp_WriteChar(LispMac*, LispBuiltin*);		/* write-char */
LispObj *Lisp_ReadLine(LispMac*, LispBuiltin*);			/* read-line */
LispObj *Lisp_WriteLine(LispMac*, LispBuiltin*);		/* write-line */
LispObj *Lisp_WriteString(LispMac*, LispBuiltin*);		/* write-string */
LispObj *Lisp_MakeStringInputStream(LispMac*, LispBuiltin*);	/* make-string-input-stream */
LispObj *Lisp_MakeStringOutputStream(LispMac*, LispBuiltin*);	/* make-string-output-stream */
LispObj *Lisp_GetOutputStreamString(LispMac*, LispBuiltin*);	/* get-output-stream-string */

#endif /* Lisp_stream_h */
