/* Id: clip.h,v 1.2 1999/02/26 08:52:31 martin Exp $ */

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
 * Log: clip.h,v $
 * Revision 1.2  1999/02/26 08:52:31  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.3  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.2  1998/06/21 02:00:55  brianp
 * cleaned up clip interpolation function code
 *
 * Revision 3.1  1998/02/03 23:46:40  brianp
 * added 'space' parameter to clip interpolation functions
 *
 * Revision 3.0  1998/01/31 20:48:09  brianp
 * initial rev
 *
 */


#ifndef CLIP_H
#define CLIP_H


#include "types.h"



#ifdef DEBUG
#  define GL_VIEWCLIP_POINT( V )   gl_viewclip_point( V )
#else
#  define GL_VIEWCLIP_POINT( V )			\
     (    (V)[0] <= (V)[3] && (V)[0] >= -(V)[3]		\
       && (V)[1] <= (V)[3] && (V)[1] >= -(V)[3]		\
       && (V)[2] <= (V)[3] && (V)[2] >= -(V)[3] )
#endif




extern GLuint gl_viewclip_point( const GLfloat v[] );

extern GLuint gl_viewclip_line( GLcontext* ctx, GLuint *i, GLuint *j );

extern GLuint gl_viewclip_polygon( GLcontext* ctx, GLuint n, GLuint vlist[] );



extern GLuint gl_userclip_point( GLcontext* ctx, const GLfloat v[] );

extern GLuint gl_userclip_line( GLcontext* ctx, GLuint *i, GLuint *j );

extern GLuint gl_userclip_polygon( GLcontext* ctx, GLuint n, GLuint vlist[] );


extern void gl_ClipPlane( GLcontext* ctx,
                          GLenum plane, const GLfloat *equation );

extern void gl_GetClipPlane( GLcontext* ctx,
                             GLenum plane, GLdouble *equation );


/*
 * Clipping interpolation functions
 */

extern void gl_clip_interp_all( GLcontext *ctx, GLuint space,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_color_tex( GLcontext *ctx, GLuint space,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_tex( GLcontext *ctx, GLuint space,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );

extern void gl_clip_interp_color( GLcontext *ctx, GLuint space,
                                GLuint dst, GLfloat t, GLuint in, GLuint out );


#endif
