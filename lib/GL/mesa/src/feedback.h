/* Id: feedback.h,v 1.2 1999/02/26 08:52:33 martin Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Log: feedback.h,v $
 * Revision 1.2  1999/02/26 08:52:33  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.1  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.0  1998/01/31 20:52:30  brianp
 * initial rev
 *
 */


#ifndef FEEDBACK_H
#define FEEDBACK_H


#include "types.h"


#define FEEDBACK_TOKEN( CTX, T )				\
	if (CTX->Feedback.Count < CTX->Feedback.BufferSize) {	\
	   CTX->Feedback.Buffer[CTX->Feedback.Count] = (T);	\
	}							\
	CTX->Feedback.Count++;


extern void gl_feedback_vertex( GLcontext *ctx,
                                GLfloat x, GLfloat y, GLfloat z, GLfloat w,
                                const GLfloat color[4], GLfloat index,
                                const GLfloat texcoord[4] );


extern void gl_update_hitflag( GLcontext *ctx, GLfloat z );


extern void gl_PassThrough( GLcontext *ctx, GLfloat token );

extern void gl_FeedbackBuffer( GLcontext *ctx, GLsizei size,
                               GLenum type, GLfloat *buffer );

extern void gl_SelectBuffer( GLcontext *ctx, GLsizei size, GLuint *buffer );

extern void gl_InitNames( GLcontext *ctx );

extern void gl_LoadName( GLcontext *ctx, GLuint name );

extern void gl_PushName( GLcontext *ctx, GLuint name );

extern void gl_PopName( GLcontext *ctx );

extern GLint gl_RenderMode( GLcontext *ctx, GLenum mode );



#endif

