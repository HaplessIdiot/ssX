/* Id: pixel.h,v 1.2 1999/02/26 08:52:36 martin Exp $ */

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
 * Log: pixel.h,v $
 * Revision 1.2  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1998/07/17 03:23:47  brianp
 * added a bunch of const's
 *
 * Revision 3.3  1998/03/22 16:42:05  brianp
 * added 8-bit CI->RGBA pixel mapping tables
 *
 * Revision 3.2  1998/02/08 20:21:42  brianp
 * added a bunch of rgba, ci and stencil scaling, biasing and mapping functions
 *
 * Revision 3.1  1998/02/01 22:15:39  brianp
 * moved pixel zoom code into zoom.h
 *
 * Revision 3.0  1998/01/31 21:00:28  brianp
 * initial rev
 *
 */


#ifndef PIXEL_H
#define PIXEL_H


#include "types.h"


/*
 * API functions
 */


extern void gl_GetPixelMapfv( GLcontext *ctx, GLenum map, GLfloat *values );

extern void gl_GetPixelMapuiv( GLcontext *ctx, GLenum map, GLuint *values );

extern void gl_GetPixelMapusv( GLcontext *ctx, GLenum map, GLushort *values );


extern void gl_PixelMapfv( GLcontext *ctx,
                           GLenum map, GLint mapsize, const GLfloat *values );

extern void gl_PixelStorei( GLcontext *ctx, GLenum pname, GLint param );

extern void gl_PixelTransferf( GLcontext *ctx, GLenum pname, GLfloat param );

extern void gl_PixelZoom( GLcontext *ctx, GLfloat xfactor, GLfloat yfactor );


/*
 * Pixel processing functions
 */

extern void gl_scale_and_bias_color( const GLcontext *ctx, GLuint n,
                                     GLfloat red[], GLfloat green[],
                                     GLfloat blue[], GLfloat alpha[] );


extern void gl_scale_and_bias_rgba( const GLcontext *ctx, GLuint n,
                                    GLubyte rgba[][4] );


extern void gl_map_rgba( const GLcontext *ctx, GLuint n, GLubyte rgba[][4] );


extern void gl_map_color( const GLcontext *ctx, GLuint n,
                          GLfloat red[], GLfloat green[],
                          GLfloat blue[], GLfloat alpha[] );


extern void gl_shift_and_offset_ci( const GLcontext *ctx, GLuint n,
                                    GLuint indexes[] );


extern void gl_map_ci( const GLcontext *ctx, GLuint n, GLuint index[] );


extern void gl_map_ci_to_rgba( const GLcontext *ctx,
                               GLuint n, const GLuint index[],
                               GLubyte rgba[][4] );


extern void gl_map_ci8_to_rgba( const GLcontext *ctx,
                                GLuint n, const GLubyte index[],
                                GLubyte rgba[][4] );


extern void gl_map_ci_to_color( const GLcontext *ctx,
                                GLuint n, const GLuint index[],
                                GLfloat r[], GLfloat g[],
                                GLfloat b[], GLfloat a[] );


extern void gl_shift_and_offset_stencil( const GLcontext *ctx, GLuint n,
                                         GLstencil indexes[] );


extern void gl_map_stencil( const GLcontext *ctx, GLuint n, GLstencil index[] );


#endif
