/* Id: span.h,v 1.2 1999/02/26 08:52:37 martin Exp $ */

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
 * Log: span.h,v $
 * Revision 1.2  1999/02/26 08:52:37  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1998/03/28 03:57:13  brianp
 * added CONST macro to fix IRIX compilation problems
 *
 * Revision 3.3  1998/02/20 04:50:44  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/03 04:26:07  brianp
 * removed const from lambda[] passed to gl_write_texture_span()
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:03:42  brianp
 * initial rev
 *
 */


#ifndef SPAN_H
#define SPAN_H


#include "types.h"


extern void gl_write_index_span( GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, const GLdepth z[],
				 GLuint index[], GLenum primitive );


extern void gl_write_monoindex_span( GLcontext *ctx,
                                     GLuint n, GLint x, GLint y,
                                     const GLdepth z[],
				     GLuint index, GLenum primitive );


extern void gl_write_rgba_span( GLcontext *ctx,
                                GLuint n, GLint x, GLint y, const GLdepth z[],
                                GLubyte rgba[][4], GLenum primitive );


extern void gl_write_monocolor_span( GLcontext *ctx,
                                     GLuint n, GLint x, GLint y,
                                     const GLdepth z[],
				     GLint r, GLint g, GLint b, GLint a,
                                     GLenum primitive );


extern void gl_write_texture_span( GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLdepth z[],
				   const GLfloat s[], const GLfloat t[],
                                   const GLfloat u[], GLfloat lambda[],
				   GLubyte rgba[][4], CONST GLubyte spec[][4],
                                   GLenum primitive );


extern void gl_write_multitexture_span( GLcontext *ctx, GLuint texSets,
                                        GLuint n, GLint x, GLint y,
                                        const GLdepth z[],
                                        CONST GLfloat s[][MAX_WIDTH],
                                        CONST GLfloat t[][MAX_WIDTH],
                                        CONST GLfloat u[][MAX_WIDTH],
                                        GLfloat lambda[][MAX_WIDTH],
                                        GLubyte rgba[][4],
                                        CONST GLubyte spec[][4],
                                        GLenum primitive );


extern void gl_read_rgba_span( GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
                               GLubyte rgba[][4] );


extern void gl_read_index_span( GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint indx[] );


#endif
