/* Id: varray.h,v 1.2 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: varray.h,v $
 * Revision 1.2  1999/02/26 08:52:39  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.4  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.3  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.2  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.1  1998/02/01 20:05:10  brianp
 * added glDrawRangeElements()
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */


#ifndef VARRAY_H
#define VARRAY_H


#include "types.h"


extern void gl_VertexPointer( GLcontext *ctx,
                              GLint size, GLenum type, GLsizei stride,
                              const GLvoid *ptr );


extern void gl_NormalPointer( GLcontext *ctx,
                              GLenum type, GLsizei stride, const GLvoid *ptr );


extern void gl_ColorPointer( GLcontext *ctx,
                             GLint size, GLenum type, GLsizei stride,
                             const GLvoid *ptr );


extern void gl_IndexPointer( GLcontext *ctx,
                                GLenum type, GLsizei stride,
                                const GLvoid *ptr );


extern void gl_TexCoordPointer( GLcontext *ctx,
                                GLint size, GLenum type, GLsizei stride,
                                const GLvoid *ptr );

/* GL_SGIS_multitexture */
extern void gl_MultiTexCoordPointer( GLcontext *ctx, GLenum target,
                                     GLint size, GLenum type, GLsizei stride,
                                     const GLvoid *ptr );


/* GL_EXT_multitexture */
extern void gl_InterleavedTextureCoordSets( GLcontext *ctx, GLint );


extern void gl_EdgeFlagPointer( GLcontext *ctx,
                                GLsizei stride, const GLboolean *ptr );


extern void gl_GetPointerv( GLcontext *ctx, GLenum pname, GLvoid **params );


extern void gl_ArrayElement( GLcontext *ctx, GLint i );

extern void gl_save_ArrayElement( GLcontext *ctx, GLint i );


extern void gl_DrawArrays( GLcontext *ctx,
                           GLenum mode, GLint first, GLsizei count );

extern void gl_save_DrawArrays( GLcontext *ctx,
                                GLenum mode, GLint first, GLsizei count );


extern void gl_DrawElements( GLcontext *ctx,
                             GLenum mode, GLsizei count,
                             GLenum type, const GLvoid *indices );

extern void gl_save_DrawElements( GLcontext *ctx,
                                  GLenum mode, GLsizei count,
                                  GLenum type, const GLvoid *indices );


extern void gl_InterleavedArrays( GLcontext *ctx,
                                  GLenum format, GLsizei stride,
                                  const GLvoid *pointer );

extern void gl_save_InterleavedArrays( GLcontext *ctx,
                                       GLenum format, GLsizei stride,
                                       const GLvoid *pointer );


extern void gl_DrawRangeElements( GLcontext *ctx, GLenum mode, GLuint start,
                                  GLuint end, GLsizei count, GLenum type,
                                  const GLvoid *indices );

extern void gl_save_DrawRangeElements( GLcontext *ctx, GLenum mode,
                                       GLuint start, GLuint end, GLsizei count,
                                       GLenum type, const GLvoid *indices );


#endif



